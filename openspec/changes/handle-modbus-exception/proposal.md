# ModBus 异常响应处理优化

**状态**: ✅ 已完成 (2024-11-29)

## 问题描述

从串口数据观察到大量异常响应帧：
```
01 83 02 C0 F1  - 从机1返回异常码02（非法数据地址）
01 83 01 80 F0  - 从机1返回异常码01（非法功能码）
```

**当前问题**：
1. `masterProcessResponse()` 只处理正常响应（功能码 0x03, 0x10, 0x06）
2. 异常响应（功能码 0x83, 0x90 等）被忽略，没有任何处理
3. 异常响应实际上表示**通信成功**（从机收到并处理了请求），但被当作无效帧丢弃
4. 可能导致不必要的重试和超时计数增加

## 异常响应帧格式

| 字节 | 内容 | 说明 |
|------|------|------|
| 0 | 从机地址 | 1-247 |
| 1 | 功能码+0x80 | 0x83=读异常, 0x90=写异常 |
| 2 | 异常码 | 01-04 |
| 3-4 | CRC | 校验 |

**异常码定义**：
| 码 | 名称 | 说明 |
|----|------|------|
| 0x01 | ILLEGAL_FUNCTION | 非法功能码 |
| 0x02 | ILLEGAL_DATA_ADDRESS | 非法数据地址 |
| 0x03 | ILLEGAL_DATA_VALUE | 非法数据值 |
| 0x04 | SLAVE_DEVICE_FAILURE | 从机设备故障 |

## 解决方案

### 1. 修改 `modbus_master.c` - 添加异常响应处理

```c
/**
 * @brief 处理主机响应
 */
static void masterProcessResponse(ModbusHandle* handle)
{
    uint8_t* frame = handle->rxBuf;
    uint16_t len = handle->rxLen;
    uint8_t slaveAddr;
    uint8_t funcCode;
    uint8_t idx;
    
    /* CRC校验 */
    if (!modbusCrcCheck(frame, len)) {
        handle->stats.crcErrCount++;
        return;
    }
    
    handle->stats.rxCount++;
    
    slaveAddr = frame[0];
    funcCode = frame[1];
    
    /* 获取从机索引 */
    idx = masterGetSlaveIndex(handle, slaveAddr);
    if (idx < MODBUS_MAX_SLAVES) {
        /* 清除失败计数 - 收到任何响应都表示通信成功 */
        handle->masterCfg.writeCache[idx].failCount = 0;
    }
    
    /* 检查是否为异常响应 (功能码最高位为1) */
    if (funcCode & 0x80) {
        /* 异常响应处理 */
        uint8_t exCode = frame[2];
        handle->stats.exceptionCount++;
        
        /* 异常响应回调 */
        if (handle->masterCb.onException) {
            handle->masterCb.onException(slaveAddr, funcCode & 0x7F, exCode);
        }
        return;
    }
    
    /* 正常响应回调 */
    if (handle->masterCb.onResponse) {
        if (funcCode == MODBUS_FC_READ_HOLDING) {
            handle->masterCb.onResponse(slaveAddr, funcCode, &frame[3], frame[2]);
        } else if (funcCode == MODBUS_FC_WRITE_MULTI) {
            handle->masterCb.onResponse(slaveAddr, funcCode, &frame[2], 4);
        } else if (funcCode == MODBUS_FC_WRITE_SINGLE) {
            handle->masterCb.onResponse(slaveAddr, funcCode, &frame[2], 4);
        }
    }
}
```

### 2. 修改 `modbus_types.h` - 添加回调类型和统计字段

```c
/**
 * @brief 异常响应回调
 * @param slaveAddr 从机地址
 * @param funcCode 原始功能码 (不含0x80)
 * @param exCode 异常码
 */
typedef void (*ModbusExceptionFunc)(uint8_t slaveAddr, uint8_t funcCode, uint8_t exCode);

/* 在 ModbusMasterCb 结构体中添加 */
typedef struct {
    ModbusResponseFunc onResponse;
    ModbusTimeoutFunc  onTimeout;
    ModbusOfflineFunc  onOffline;
    ModbusExceptionFunc onException;  /* 新增: 异常响应回调 */
} ModbusMasterCb;

/* 在 ModbusStats 结构体中添加 */
typedef struct {
    uint32_t txCount;
    uint32_t rxCount;
    uint32_t crcErrCount;
    uint32_t timeoutCount;
    uint32_t exceptionCount;  /* 新增: 异常响应计数 */
} ModbusStats;
```

### 3. 修改 `modbus_uart4.c` - 添加异常处理回调

```c
/**
 * @brief 主机收到异常响应回调
 */
static void uart4OnException(uint8_t slaveAddr, uint8_t funcCode, uint8_t exCode)
{
    /* 异常响应也表示从机在线 */
    if (slaveAddr < 12) {
        SlaveG[slaveAddr].Alive_Flag = OK;
        SlaveG[slaveAddr].Send_Flag = 0;
    }
    
    /* 可选: 记录异常信息用于调试 */
    /* 异常码01: 非法功能码 - 从机不支持该功能 */
    /* 异常码02: 非法数据地址 - 请求的寄存器地址不存在 */
    /* 异常码03: 非法数据值 - 写入的值超出范围 */
    /* 异常码04: 从机设备故障 - 从机内部错误 */
}

/* 在初始化时注册回调 */
static ModbusMasterCb uart4MasterCb = {
    .onResponse  = uart4OnResponse,
    .onTimeout   = uart4OnTimeout,
    .onOffline   = uart4OnOffline,
    .onException = uart4OnException  /* 新增 */
};
```

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `HARDWARE/modbus/modbus_types.h` | 添加 `ModbusExceptionFunc` 类型和 `exceptionCount` 统计 |
| `HARDWARE/modbus/modbus_master.c` | 在 `masterProcessResponse()` 中添加异常响应处理 |
| `HARDWARE/modbus/modbus_uart4.c` | 添加 `uart4OnException` 回调并注册 |

## 预期效果

1. **正确识别异常响应**：不再将异常响应当作无效帧丢弃
2. **维持在线状态**：收到异常响应的从机仍被视为在线
3. **统计异常次数**：方便诊断通信问题
4. **可扩展处理**：通过回调机制支持自定义异常处理逻辑

## 风险评估

- **风险等级**: 低
- **影响范围**: ModBus主机响应处理
- **兼容性**: 完全向后兼容，新增回调为可选

## 测试要点

1. 验证异常响应被正确识别（功能码 & 0x80）
2. 验证异常响应不影响从机在线状态
3. 验证 `exceptionCount` 统计正确递增
4. 验证正常响应处理不受影响

