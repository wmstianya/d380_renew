# 统一ModBus RTU协议层设计

## 一、架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                        应用层 (Application)                      │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│  │ 屏幕通信  │ │ RTU服务器 │ │ 供水阀控制 │ │ 分机控制  │           │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘           │
│       │            │            │            │                  │
├───────┴────────────┴────────────┴────────────┴──────────────────┤
│                   ModBus协议层 (Protocol)                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    ModbusHandle                          │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │
│  │  │ 从机模块    │  │ 主机模块    │  │ 调度器      │      │   │
│  │  │ (Slave)    │  │ (Master)   │  │ (Scheduler) │      │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                  │
├──────────────────────────────┴──────────────────────────────────┤
│                     UART驱动层 (Driver)                          │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    UartHandle                            │   │
│  │  DMA接收 + 双缓冲 + IDLE检测 + DMA发送                    │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 二、核心数据结构

### 2.1 ModbusHandle (协议句柄)

```c
/**
 * @brief ModBus RTU 协议句柄
 * @note 每个串口一个实例
 */
typedef struct ModbusHandle {
    /* 底层驱动 */
    UartHandle* uart;                    // UART DMA驱动句柄
    
    /* 角色配置 */
    ModbusRole role;                     // MODBUS_SLAVE / MODBUS_MASTER / MODBUS_DUAL
    
    /* 状态机 */
    ModbusState state;                   // 当前状态
    uint32_t stateTimestamp;             // 状态切换时间戳
    
    /* 从机配置 (role包含SLAVE时有效) */
    uint8_t slaveAddress;                // 本机从机地址
    ModbusRegisterMap* regMap;           // 寄存器映射表
    ModbusSlaveCallback slaveCallback;   // 从机回调函数
    
    /* 主机配置 (role包含MASTER时有效) */
    ModbusMasterConfig masterConfig;     // 主机轮询配置
    ModbusMasterCallback masterCallback; // 主机回调函数
    
    /* 缓冲区 */
    uint8_t txBuffer[MODBUS_MAX_FRAME];  // 发送缓冲
    uint8_t rxBuffer[MODBUS_MAX_FRAME];  // 接收缓冲
    uint16_t rxLength;                   // 接收长度
    
    /* 统计信息 */
    ModbusStats stats;                   // 通信统计
} ModbusHandle;
```

### 2.2 角色定义

```c
typedef enum {
    MODBUS_SLAVE  = 0x01,   // 纯从机
    MODBUS_MASTER = 0x02,   // 纯主机
    MODBUS_DUAL   = 0x03    // 双角色 (SLAVE | MASTER)
} ModbusRole;
```

### 2.3 状态机定义

```c
typedef enum {
    /* 空闲状态 */
    MODBUS_IDLE,
    
    /* 从机状态 */
    MODBUS_SLAVE_PROCESSING,    // 处理从机请求
    MODBUS_SLAVE_RESPONDING,    // 发送从机响应
    
    /* 主机状态 */
    MODBUS_MASTER_SENDING,      // 发送主机请求
    MODBUS_MASTER_WAITING,      // 等待从机响应
    MODBUS_MASTER_PROCESSING    // 处理从机响应
} ModbusState;
```

### 2.4 寄存器映射表

```c
/**
 * @brief 寄存器读取回调
 * @param address 寄存器起始地址
 * @param count 寄存器数量
 * @param buffer 输出缓冲区
 * @return 实际读取的字节数，0表示地址无效
 */
typedef uint16_t (*ModbusReadFunc)(uint16_t address, uint16_t count, uint8_t* buffer);

/**
 * @brief 寄存器写入回调
 * @param address 寄存器起始地址
 * @param count 寄存器数量
 * @param data 写入数据
 * @return MODBUS_OK 或错误码
 */
typedef ModbusError (*ModbusWriteFunc)(uint16_t address, uint16_t count, uint8_t* data);

/**
 * @brief 寄存器映射项
 */
typedef struct {
    uint16_t startAddress;      // 起始地址
    uint16_t endAddress;        // 结束地址
    ModbusReadFunc readFunc;    // 读取函数
    ModbusWriteFunc writeFunc;  // 写入函数 (NULL表示只读)
} ModbusRegisterEntry;

/**
 * @brief 寄存器映射表
 */
typedef struct {
    ModbusRegisterEntry* entries;
    uint8_t entryCount;
} ModbusRegisterMap;
```

### 2.5 主机配置

```c
/**
 * @brief 主机轮询配置
 */
typedef struct {
    /* 轮询地址列表 */
    uint8_t pollAddresses[MODBUS_MAX_SLAVES];
    uint8_t pollCount;
    uint8_t currentIndex;
    
    /* 时序参数 */
    uint16_t responseTimeoutMs;     // 响应超时 (默认10ms)
    uint16_t pollIntervalMs;        // 轮询间隔 (动态模式下为最小间隔)
    
    /* 按需写入缓存 */
    ModbusWriteCache writeCache[MODBUS_MAX_SLAVES];
    
    /* 重试配置 */
    uint8_t maxRetry;               // 最大重试次数
    uint8_t offlineThreshold;       // 离线判定阈值
} ModbusMasterConfig;

/**
 * @brief 写入缓存 (按需写入优化)
 */
typedef struct {
    uint8_t needWrite;              // 是否需要写入
    uint8_t lastData[MODBUS_WRITE_CACHE_SIZE];  // 上次写入数据
    uint16_t lastDataLen;           // 上次写入长度
} ModbusWriteCache;
```

---

## 三、统一API接口

### 3.1 初始化接口

```c
/**
 * @brief 初始化ModBus句柄
 * @param handle ModBus句柄
 * @param uart UART驱动句柄
 * @param role 角色 (SLAVE/MASTER/DUAL)
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusInit(ModbusHandle* handle, UartHandle* uart, ModbusRole role);

/**
 * @brief 配置从机模式
 * @param handle ModBus句柄
 * @param address 从机地址
 * @param regMap 寄存器映射表
 * @param callback 从机回调
 */
ModbusError modbusConfigSlave(ModbusHandle* handle, 
                              uint8_t address, 
                              ModbusRegisterMap* regMap,
                              ModbusSlaveCallback callback);

/**
 * @brief 配置主机模式
 * @param handle ModBus句柄
 * @param config 主机配置
 * @param callback 主机回调
 */
ModbusError modbusConfigMaster(ModbusHandle* handle,
                               ModbusMasterConfig* config,
                               ModbusMasterCallback callback);
```

### 3.2 调度接口

```c
/**
 * @brief ModBus调度器 (主循环调用)
 * @param handle ModBus句柄
 * @note 根据角色自动执行从机响应或主机轮询
 */
void modbusScheduler(ModbusHandle* handle);

/**
 * @brief 检查是否有待处理的请求
 * @param handle ModBus句柄
 * @return TRUE/FALSE
 */
uint8_t modbusHasPendingRequest(ModbusHandle* handle);
```

### 3.3 主机接口

```c
/**
 * @brief 发送读取请求 (功能码03)
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址
 * @param regCount 寄存器数量
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusMasterRead(ModbusHandle* handle, 
                             uint8_t slaveAddr, 
                             uint16_t regAddr, 
                             uint16_t regCount);

/**
 * @brief 发送写入请求 (功能码06/10)
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址
 * @param data 数据
 * @param dataLen 数据长度
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusMasterWrite(ModbusHandle* handle,
                              uint8_t slaveAddr,
                              uint16_t regAddr,
                              uint8_t* data,
                              uint16_t dataLen);

/**
 * @brief 检查是否需要写入 (按需写入优化)
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @param data 当前数据
 * @param dataLen 数据长度
 * @return TRUE需要写入 / FALSE跳过
 */
uint8_t modbusMasterNeedWrite(ModbusHandle* handle,
                              uint8_t slaveAddr,
                              uint8_t* data,
                              uint16_t dataLen);

/**
 * @brief 标记从机在线/离线
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @return TRUE在线 / FALSE离线
 */
uint8_t modbusMasterIsOnline(ModbusHandle* handle, uint8_t slaveAddr);
```

---

## 四、从机模块设计

### 4.1 从机回调接口

```c
/**
 * @brief 从机事件回调
 */
typedef struct {
    /**
     * @brief 请求预处理 (可选)
     * @param funcCode 功能码
     * @param regAddr 寄存器地址
     * @return MODBUS_OK允许处理 / 错误码拒绝
     */
    ModbusError (*onRequestPre)(uint8_t funcCode, uint16_t regAddr);
    
    /**
     * @brief 写入完成回调
     * @param funcCode 功能码 (06/10)
     * @param regAddr 寄存器地址
     * @param data 写入的数据
     * @param dataLen 数据长度
     */
    void (*onWriteComplete)(uint8_t funcCode, uint16_t regAddr, 
                            uint8_t* data, uint16_t dataLen);
} ModbusSlaveCallback;
```

### 4.2 从机处理流程

```c
void modbusSlaveProcess(ModbusHandle* handle) {
    uint8_t* frame = handle->rxBuffer;
    uint16_t len = handle->rxLength;
    
    // 1. 地址过滤
    if (frame[0] != handle->slaveAddress && frame[0] != 0xFF)
        return;
    
    // 2. CRC校验
    if (!modbusCrcCheck(frame, len))
        return;
    
    // 3. 功能码分发
    uint8_t funcCode = frame[1];
    uint16_t regAddr = (frame[2] << 8) | frame[3];
    
    switch (funcCode) {
        case MODBUS_FC_READ_HOLDING:    // 03
            modbusSlaveHandleRead(handle, regAddr, frame, len);
            break;
        case MODBUS_FC_WRITE_SINGLE:    // 06
            modbusSlaveHandleWriteSingle(handle, regAddr, frame, len);
            break;
        case MODBUS_FC_WRITE_MULTIPLE:  // 10
            modbusSlaveHandleWriteMultiple(handle, regAddr, frame, len);
            break;
        default:
            modbusSlaveException(handle, funcCode, MODBUS_EX_ILLEGAL_FUNC);
            break;
    }
}
```

---

## 五、主机模块设计

### 5.1 主机回调接口

```c
/**
 * @brief 主机事件回调
 */
typedef struct {
    /**
     * @brief 响应接收回调
     * @param slaveAddr 从机地址
     * @param funcCode 功能码
     * @param data 响应数据
     * @param dataLen 数据长度
     */
    void (*onResponse)(uint8_t slaveAddr, uint8_t funcCode, 
                       uint8_t* data, uint16_t dataLen);
    
    /**
     * @brief 超时回调
     * @param slaveAddr 从机地址
     */
    void (*onTimeout)(uint8_t slaveAddr);
    
    /**
     * @brief 从机离线回调
     * @param slaveAddr 从机地址
     */
    void (*onOffline)(uint8_t slaveAddr);
    
    /**
     * @brief 获取写入数据 (按需写入)
     * @param slaveAddr 从机地址
     * @param buffer 输出缓冲
     * @param maxLen 最大长度
     * @return 实际数据长度，0表示无需写入
     */
    uint16_t (*getWriteData)(uint8_t slaveAddr, uint8_t* buffer, uint16_t maxLen);
} ModbusMasterCallback;
```

### 5.2 主机调度器 (含优化)

```c
void modbusMasterScheduler(ModbusHandle* handle) {
    ModbusMasterConfig* cfg = &handle->masterConfig;
    
    switch (handle->state) {
        case MODBUS_IDLE:
            // 检查是否有从机请求需要响应 (DUAL模式优先)
            if ((handle->role & MODBUS_SLAVE) && modbusHasSlaveRequest(handle)) {
                handle->state = MODBUS_SLAVE_PROCESSING;
                break;
            }
            
            // 执行主机轮询
            uint8_t addr = cfg->pollAddresses[cfg->currentIndex];
            
            // 按需写入检查
            if (modbusMasterNeedWrite(handle, addr, ...)) {
                modbusMasterWrite(handle, addr, ...);
            } else {
                modbusMasterRead(handle, addr, 100, 18);
            }
            
            handle->state = MODBUS_MASTER_WAITING;
            handle->stateTimestamp = getSysTick();
            break;
            
        case MODBUS_MASTER_WAITING:
            // 动态间隔: 收到响应立即处理
            if (uartIsRxReady(handle->uart)) {
                modbusMasterProcessResponse(handle);
                modbusMasterNextAddress(handle);
                handle->state = MODBUS_IDLE;
            }
            // 超时检测
            else if (getElapsed(handle->stateTimestamp) > cfg->responseTimeoutMs) {
                modbusMasterHandleTimeout(handle);
                modbusMasterNextAddress(handle);
                handle->state = MODBUS_IDLE;
            }
            break;
    }
}
```

---

## 六、四个串口的配置示例

### 6.1 USART1 (RTU服务器)

```c
ModbusHandle modbusUsart1;
ModbusRegisterMap usart1RegMap;

void usart1ModbusInit(void) {
    modbusInit(&modbusUsart1, &uartDebugHandle, MODBUS_SLAVE);
    modbusConfigSlave(&modbusUsart1, Sys_Admin.ModBus_Address, 
                      &usart1RegMap, &usart1SlaveCallback);
}
```

### 6.2 USART2 (屏幕通信)

```c
ModbusHandle modbusUsart2;
ModbusRegisterMap usart2RegMap;

void usart2ModbusInit(void) {
    modbusInit(&modbusUsart2, &uartDisplayHandle, MODBUS_SLAVE);
    modbusConfigSlave(&modbusUsart2, 0x01, 
                      &usart2RegMap, &usart2SlaveCallback);
}
```

### 6.3 USART3 (供水阀)

```c
ModbusHandle modbusUsart3;
ModbusMasterConfig usart3MasterCfg = {
    .pollAddresses = {8},
    .pollCount = 1,
    .responseTimeoutMs = 10,
    .pollIntervalMs = 200
};

void usart3ModbusInit(void) {
    modbusInit(&modbusUsart3, &uartSlaveHandle, MODBUS_MASTER);
    modbusConfigMaster(&modbusUsart3, &usart3MasterCfg, &usart3MasterCallback);
}
```

### 6.4 UART4 (联控 - 双角色)

```c
ModbusHandle modbusUart4;
ModbusRegisterMap uart4RegMap;
ModbusMasterConfig uart4MasterCfg = {
    .pollAddresses = {1, 2, 3, 4, 5},
    .pollCount = 5,
    .responseTimeoutMs = 10,
    .pollIntervalMs = 0,  // 动态间隔
    .offlineThreshold = 20
};

void uart4ModbusInit(void) {
    modbusInit(&modbusUart4, &uartUnionHandle, MODBUS_DUAL);
    
    // 配置从机模式 (响应联控主机)
    modbusConfigSlave(&modbusUart4, sys_flag.Address_Number,
                      &uart4RegMap, &uart4SlaveCallback);
    
    // 配置主机模式 (轮询分机)
    modbusConfigMaster(&modbusUart4, &uart4MasterCfg, &uart4MasterCallback);
}
```

---

## 七、文件结构

```
HARDWARE/modbus/
├── modbus.h              // 公共头文件
├── modbus_types.h        // 类型定义
├── modbus_core.c         // 核心实现 (CRC, 帧解析)
├── modbus_slave.c        // 从机模块
├── modbus_master.c       // 主机模块
├── modbus_scheduler.c    // 调度器
│
├── modbus_usart1.c       // USART1 寄存器映射
├── modbus_usart2.c       // USART2 寄存器映射
├── modbus_usart3.c       // USART3 主机配置
└── modbus_uart4.c        // UART4 双角色配置
```

---

## 八、与现有代码的兼容策略

### 8.1 渐进式迁移

```
阶段1: 新增modbus模块，与现有代码并存
阶段2: 逐个串口迁移 (USART3 → UART4 → USART1 → USART2)
阶段3: 删除旧代码
```

### 8.2 寄存器映射复用

```c
// 复用现有的数据结构
// usart2寄存器读取回调
uint16_t usart2ReadUnionLCD(uint16_t addr, uint16_t count, uint8_t* buf) {
    if (addr == 0x0000 && count <= 48) {
        memcpy(buf, UnionLCD.Datas, count * 2);
        return count * 2;
    }
    return 0;
}
```

