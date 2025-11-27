# ModBus统一协议层规格说明

## 概述

本规格定义了STM32F103联控板的统一ModBus RTU协议层实现。

- **版本**: 1.0
- **状态**: 已实现
- **文件位置**: `HARDWARE/modbus/`

---

## 一、架构设计

### 1.1 模块结构

```
HARDWARE/modbus/
├── modbus_types.h      # 类型定义
├── modbus.h            # 公共API
├── modbus_core.c       # 核心调度器
├── modbus_slave.c      # 从机处理
├── modbus_master.c     # 主机处理
├── modbus_port.c       # 硬件抽象
├── modbus_usart1.c     # USART1配置
├── modbus_usart2.c     # USART2配置
├── modbus_usart3.c     # USART3配置
└── modbus_uart4.c      # UART4配置
```

### 1.2 角色支持

| 角色 | 枚举值 | 说明 |
|------|--------|------|
| MODBUS_ROLE_SLAVE | 0x01 | 纯从机模式 |
| MODBUS_ROLE_MASTER | 0x02 | 纯主机模式 |
| MODBUS_ROLE_DUAL | 0x03 | 双角色模式 |

### 1.3 状态机

```
                    ┌────────────────┐
                    │  MODBUS_IDLE   │
                    └───────┬────────┘
                            │
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
    ┌───────────────┐ ┌──────────────┐ ┌──────────────┐
    │ SLAVE_PROCESS │ │ MASTER_SEND  │ │ MASTER_WAIT  │
    └───────┬───────┘ └──────┬───────┘ └──────┬───────┘
            │                │                │
            ▼                │                ▼
    ┌───────────────┐        │        ┌──────────────┐
    │ SLAVE_RESPOND │        │        │MASTER_PROCESS│
    └───────┬───────┘        │        └──────┬───────┘
            │                │                │
            └────────────────┴────────────────┘
                            │
                            ▼
                    ┌────────────────┐
                    │  MODBUS_IDLE   │
                    └────────────────┘
```

---

## 二、核心数据结构

### 2.1 ModbusHandle

```c
typedef struct ModbusHandle {
    /* 底层驱动 */
    UartHandle* uart;                       /* UART DMA驱动句柄 */
    
    /* 角色配置 */
    ModbusRole role;                        /* 角色 */
    
    /* 状态机 */
    ModbusState state;                      /* 当前状态 */
    uint32_t stateTimestamp;                /* 状态切换时间戳 */
    
    /* 从机配置 */
    uint8_t slaveAddr;                      /* 本机从机地址 */
    ModbusRegMap* regMap;                   /* 寄存器映射表 */
    ModbusSlaveCallback slaveCb;            /* 从机回调 */
    
    /* 主机配置 */
    ModbusMasterCfg masterCfg;              /* 主机配置 */
    ModbusMasterCallback masterCb;          /* 主机回调 */
    
    /* 缓冲区 */
    uint8_t txBuf[MODBUS_MAX_FRAME];        /* 发送缓冲 */
    uint8_t rxBuf[MODBUS_MAX_FRAME];        /* 接收缓冲 */
    uint16_t rxLen;                         /* 接收长度 */
    
    /* 统计 */
    ModbusStats stats;                      /* 通信统计 */
} ModbusHandle;
```

### 2.2 寄存器映射

```c
typedef struct {
    uint16_t startAddr;         /* 起始地址 */
    uint16_t endAddr;           /* 结束地址 */
    ModbusReadFunc readFunc;    /* 读取函数 */
    ModbusWriteFunc writeFunc;  /* 写入函数 (NULL表示只读) */
} ModbusRegEntry;

typedef struct {
    ModbusRegEntry* entries;    /* 映射项数组 */
    uint8_t entryCount;         /* 映射项数量 */
} ModbusRegMap;
```

### 2.3 主机配置

```c
typedef struct {
    uint8_t pollAddrs[MODBUS_MAX_SLAVES];   /* 轮询地址列表 */
    uint8_t pollCount;                       /* 轮询地址数量 */
    uint8_t currentIdx;                      /* 当前轮询索引 */
    uint8_t currentSendIdx;                  /* 读/写切换 */
    
    uint16_t readRegAddr;                    /* 读取寄存器地址 */
    uint16_t readRegCount;                   /* 读取寄存器数量 */
    uint16_t writeRegAddr;                   /* 写入寄存器地址 */
    
    uint16_t respTimeoutMs;                  /* 响应超时 (ms) */
    uint16_t pollIntervalMs;                 /* 轮询间隔 (ms) */
    
    uint8_t maxRetry;                        /* 最大重试次数 */
    uint8_t offlineThreshold;                /* 离线判定阈值 */
} ModbusMasterCfg;
```

---

## 三、公共API

### 3.1 初始化

```c
/**
 * @brief 初始化ModBus句柄
 * @param handle ModBus句柄
 * @param uart UART驱动句柄
 * @param role 角色
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusInit(ModbusHandle* handle, UartHandle* uart, ModbusRole role);
```

### 3.2 配置

```c
/**
 * @brief 配置从机模式
 */
ModbusError modbusConfigSlave(ModbusHandle* handle, uint8_t addr, 
                               ModbusRegMap* regMap, ModbusSlaveCallback* cb);

/**
 * @brief 配置主机模式
 */
ModbusError modbusConfigMaster(ModbusHandle* handle, ModbusMasterCfg* cfg,
                                ModbusMasterCallback* cb);
```

### 3.3 调度

```c
/**
 * @brief 主调度函数 (主循环调用)
 */
void modbusScheduler(ModbusHandle* handle);

/**
 * @brief 时间戳递增 (1ms定时器调用)
 */
void modbusTickInc(void);
```

---

## 四、串口配置

### 4.1 USART1 - RTU服务器

| 属性 | 值 |
|------|-----|
| 角色 | 从机 |
| 地址 | Sys_Admin.ModBus_Address (动态) |
| 波特率 | 9600 |

**寄存器映射**:
- 读: 0x0000 (联控数据), 0x0063-0x00C7 (机组), 1000 (地址查询)
- 写: 0x0000 (启停), 0x0006-0x0009 (复位/压力)

### 4.2 USART2 - 10.1寸屏幕

| 属性 | 值 |
|------|-----|
| 角色 | 从机 |
| 地址 | 0x01 |
| 波特率 | 115200 |

**寄存器映射**:
- 读: 0x0000 (联控数据), 0x0063-0x0117 (机组1-10)
- 写: 0x0000-0x001F (控制参数), 20-29 (工作时间)

### 4.3 USART3 - 变频供水阀

| 属性 | 值 |
|------|-----|
| 角色 | 主机 |
| 轮询地址 | 8 |
| 波特率 | 9600 |
| 轮询间隔 | 500ms |

**寄存器**:
- 读: 0x0000 (故障状态)
- 写: 0x0001 (开度值 0-1000)

### 4.4 UART4 - 联控通信

| 属性 | 值 |
|------|-----|
| 角色 | **双角色** |
| 从机地址 | Sys_Admin.ModBus_Address (动态) |
| 轮询地址 | 1-10 (分机组) |
| 波特率 | 9600 |
| 轮询间隔 | 100ms |

**从机寄存器**:
- 读: 100 (本机状态, 18寄存器)
- 写: 200 (控制命令, 18寄存器)

**主机轮询**:
- 读: 地址100, 18寄存器
- 写: 地址200, 18寄存器

---

## 五、使用示例

### 5.1 从机配置示例

```c
static ModbusHandle modbusUsart1;
static ModbusRegEntry usart1RegEntries[3];
static ModbusRegMap usart1RegMap;

ModbusError modbusUsart1Init(void)
{
    /* 初始化寄存器映射 */
    usart1RegEntries[0].startAddr = 0x0000;
    usart1RegEntries[0].endAddr = 0x000B;
    usart1RegEntries[0].readFunc = readUnionData;
    usart1RegEntries[0].writeFunc = writeControl;
    
    usart1RegMap.entries = usart1RegEntries;
    usart1RegMap.entryCount = 1;
    
    /* 初始化句柄 */
    modbusInit(&modbusUsart1, &uartDebugHandle, MODBUS_ROLE_SLAVE);
    modbusConfigSlave(&modbusUsart1, 0x01, &usart1RegMap, NULL);
    
    return MODBUS_OK;
}
```

### 5.2 主机配置示例

```c
static ModbusHandle modbusUsart3;
static ModbusMasterCfg usart3MasterCfg;
static ModbusMasterCallback usart3MasterCb;

ModbusError modbusUsart3Init(void)
{
    /* 配置轮询 */
    usart3MasterCfg.pollAddrs[0] = 8;
    usart3MasterCfg.pollCount = 1;
    usart3MasterCfg.pollIntervalMs = 500;
    usart3MasterCfg.respTimeoutMs = 100;
    
    /* 配置回调 */
    usart3MasterCb.onResponse = onValveResponse;
    usart3MasterCb.onTimeout = onValveTimeout;
    usart3MasterCb.getWriteData = getValveWriteData;
    
    /* 初始化 */
    modbusInit(&modbusUsart3, &uartSlaveHandle, MODBUS_ROLE_MASTER);
    modbusConfigMaster(&modbusUsart3, &usart3MasterCfg, &usart3MasterCb);
    
    return MODBUS_OK;
}
```

---

## 六、切换开关

```c
// USER/main.c
#define USE_UNIFIED_MODBUS 1  // 1=使用新协议层, 0=使用旧代码
```

---

## 七、版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024-11-27 | 初始版本，完成四个串口迁移 |
| 1.1 | 2024-11-27 | USART2写入寄存器补全 (30-41, 101-296) |
| 1.2 | 2024-11-27 | UART4时序优化 (动态轮询+缩短超时) |

