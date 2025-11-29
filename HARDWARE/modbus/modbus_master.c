/**
 * @file modbus_master.c
 * @brief ModBus RTU 主机模块实现
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 */

#include "modbus.h"
#include <string.h>

/*============================================================================*/
/*                              内部函数声明                                   */
/*============================================================================*/

static void masterProcessResponse(ModbusHandle* handle);
static void masterHandleTimeout(ModbusHandle* handle);
static void masterNextAddress(ModbusHandle* handle);
static uint8_t masterCheckNeedWrite(ModbusHandle* handle, uint8_t slaveAddr);
static uint8_t masterGetSlaveIndex(ModbusHandle* handle, uint8_t slaveAddr);

/*============================================================================*/
/*                              主机发送接口                                   */
/*============================================================================*/

/**
 * @brief 发送读取请求 (功能码03)
 */
ModbusError modbusMasterRead(ModbusHandle* handle, 
                              uint8_t slaveAddr, 
                              uint16_t regAddr, 
                              uint16_t regCount)
{
    uint16_t crc;
    uint8_t* txBuf;
    
    if (handle == NULL) {
        return MODBUS_ERR_PARAM;
    }
    
    txBuf = handle->txBuf;
    
    txBuf[0] = slaveAddr;
    txBuf[1] = MODBUS_FC_READ_HOLDING;
    txBuf[2] = regAddr >> 8;
    txBuf[3] = regAddr & 0xFF;
    txBuf[4] = regCount >> 8;
    txBuf[5] = regCount & 0xFF;
    
    crc = modbusCrc16(txBuf, 8);
    txBuf[6] = crc >> 8;
    txBuf[7] = crc & 0xFF;
    
    uartSendDma(handle->uart, txBuf, 8);
    handle->stats.txCount++;
    
    return MODBUS_OK;
}

/**
 * @brief 发送多寄存器写入请求 (功能码10)
 */
ModbusError modbusMasterWrite(ModbusHandle* handle,
                               uint8_t slaveAddr,
                               uint16_t regAddr,
                               uint8_t* data,
                               uint16_t regCount)
{
    uint16_t crc;
    uint8_t* txBuf;
    uint16_t byteCount;
    uint16_t frameLen;
    
    if (handle == NULL || data == NULL) {
        return MODBUS_ERR_PARAM;
    }
    
    txBuf = handle->txBuf;
    byteCount = regCount * 2;
    
    txBuf[0] = slaveAddr;
    txBuf[1] = MODBUS_FC_WRITE_MULTI;
    txBuf[2] = regAddr >> 8;
    txBuf[3] = regAddr & 0xFF;
    txBuf[4] = regCount >> 8;
    txBuf[5] = regCount & 0xFF;
    txBuf[6] = (uint8_t)byteCount;
    
    /* 复制数据 */
    memcpy(&txBuf[7], data, byteCount);
    
    frameLen = 7 + byteCount + 2;
    crc = modbusCrc16(txBuf, frameLen);
    txBuf[7 + byteCount] = crc >> 8;
    txBuf[8 + byteCount] = crc & 0xFF;
    
    uartSendDma(handle->uart, txBuf, frameLen);
    handle->stats.txCount++;
    
    return MODBUS_OK;
}

/**
 * @brief 发送单寄存器写入请求 (功能码06)
 */
ModbusError modbusMasterWriteSingle(ModbusHandle* handle,
                                     uint8_t slaveAddr,
                                     uint16_t regAddr,
                                     uint16_t value)
{
    uint16_t crc;
    uint8_t* txBuf;
    
    if (handle == NULL) {
        return MODBUS_ERR_PARAM;
    }
    
    txBuf = handle->txBuf;
    
    txBuf[0] = slaveAddr;
    txBuf[1] = MODBUS_FC_WRITE_SINGLE;
    txBuf[2] = regAddr >> 8;
    txBuf[3] = regAddr & 0xFF;
    txBuf[4] = value >> 8;
    txBuf[5] = value & 0xFF;
    
    crc = modbusCrc16(txBuf, 8);
    txBuf[6] = crc >> 8;
    txBuf[7] = crc & 0xFF;
    
    uartSendDma(handle->uart, txBuf, 8);
    handle->stats.txCount++;
    
    return MODBUS_OK;
}

/*============================================================================*/
/*                              主机调度器                                     */
/*============================================================================*/

/**
 * @brief 主机调度器
 */
void modbusMasterScheduler(ModbusHandle* handle)
{
    ModbusMasterCfg* cfg;
    uint8_t slaveAddr;
    uint32_t elapsed;
    uint8_t writeData[MODBUS_WRITE_CACHE_SIZE];
    uint16_t writeRegAddr;
    uint16_t writeLen;
    
    if (handle == NULL) {
        return;
    }
    
    cfg = &handle->masterCfg;
    
    if (cfg->pollCount == 0) {
        return;
    }
    
    switch (handle->state) {
        case MODBUS_STATE_IDLE:
            /* 获取当前轮询地址 */
            slaveAddr = cfg->pollAddrs[cfg->currentIdx];
            
            /* 按需写入检查 */
            if (cfg->currentSendIdx == 0) {
                /* 读取操作 */
                modbusMasterRead(handle, slaveAddr, 
                                  cfg->readRegAddr, cfg->readRegCount);
                cfg->currentSendIdx = 1;
            } else {
                /* 写入操作 (按需) */
                if (handle->masterCb.getWriteData) {
                    writeLen = handle->masterCb.getWriteData(slaveAddr, &writeRegAddr,
                                                              writeData, sizeof(writeData));
                    if (writeLen > 0) {
                        modbusMasterWrite(handle, slaveAddr, writeRegAddr,
                                           writeData, writeLen / 2);
                    } else {
                        /* 无需写入，直接下一个 */
                        masterNextAddress(handle);
                        return;
                    }
                } else {
                    /* 没有写入回调，直接下一个 */
                    masterNextAddress(handle);
                    return;
                }
                cfg->currentSendIdx = 0;
            }
            
            /* 更新状态 */
            handle->state = MODBUS_STATE_MASTER_WAIT;
            handle->stateTimestamp = modbusGetTick();
            
            /* 增加失败计数 */
            cfg->writeCache[cfg->currentIdx].failCount++;
            break;
            
        case MODBUS_STATE_MASTER_WAIT:
            /* 检查响应 */
            if (modbusHasRxData(handle)) {
                masterProcessResponse(handle);
                uartClearRxFlag(handle->uart);
                
                /* 动态间隔: 收到响应立即切换 */
                if (cfg->pollIntervalMs == 0) {
                    masterNextAddress(handle);
                    handle->state = MODBUS_STATE_IDLE;
                } else {
                    handle->state = MODBUS_STATE_MASTER_PROCESS;
                    handle->stateTimestamp = modbusGetTick();
                }
            } else {
                /* 超时检测 */
                elapsed = modbusGetTick() - handle->stateTimestamp;
                if (elapsed >= cfg->respTimeoutMs) {
                    masterHandleTimeout(handle);
                    masterNextAddress(handle);
                    handle->state = MODBUS_STATE_IDLE;
                }
            }
            break;
            
        case MODBUS_STATE_MASTER_PROCESS:
            /* 固定间隔模式: 等待间隔时间 */
            elapsed = modbusGetTick() - handle->stateTimestamp;
            if (elapsed >= cfg->pollIntervalMs) {
                masterNextAddress(handle);
                handle->state = MODBUS_STATE_IDLE;
            }
            break;
            
        default:
            handle->state = MODBUS_STATE_IDLE;
            break;
    }
}

/*============================================================================*/
/*                              内部函数                                       */
/*============================================================================*/

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
            /* 03响应: 地址+功能码+长度+数据+CRC */
            handle->masterCb.onResponse(slaveAddr, funcCode, &frame[3], frame[2]);
        } else if (funcCode == MODBUS_FC_WRITE_MULTI) {
            /* 10响应: 确认帧 */
            handle->masterCb.onResponse(slaveAddr, funcCode, &frame[2], 4);
        } else if (funcCode == MODBUS_FC_WRITE_SINGLE) {
            /* 06响应: 回显帧 */
            handle->masterCb.onResponse(slaveAddr, funcCode, &frame[2], 4);
        }
    }
}

/**
 * @brief 处理超时
 */
static void masterHandleTimeout(ModbusHandle* handle)
{
    ModbusMasterCfg* cfg = &handle->masterCfg;
    uint8_t idx = cfg->currentIdx;
    uint8_t slaveAddr = cfg->pollAddrs[idx];
    
    handle->stats.timeoutCount++;
    
    /* 超时回调 */
    if (handle->masterCb.onTimeout) {
        handle->masterCb.onTimeout(slaveAddr);
    }
    
    /* 检查离线 */
    if (cfg->writeCache[idx].failCount >= cfg->offlineThreshold) {
        if (handle->masterCb.onOffline) {
            handle->masterCb.onOffline(slaveAddr);
        }
    }
}

/**
 * @brief 切换到下一个地址
 */
static void masterNextAddress(ModbusHandle* handle)
{
    ModbusMasterCfg* cfg = &handle->masterCfg;
    
    /* 如果当前是读取，下次还是这个地址写入 */
    if (cfg->currentSendIdx == 1) {
        return;  /* 下次同地址写入 */
    }
    
    /* 切换到下一个地址 */
    cfg->currentIdx++;
    if (cfg->currentIdx >= cfg->pollCount) {
        cfg->currentIdx = 0;
    }
}

/**
 * @brief 获取从机索引
 */
static uint8_t masterGetSlaveIndex(ModbusHandle* handle, uint8_t slaveAddr)
{
    ModbusMasterCfg* cfg = &handle->masterCfg;
    uint8_t i;
    
    for (i = 0; i < cfg->pollCount; i++) {
        if (cfg->pollAddrs[i] == slaveAddr) {
            return i;
        }
    }
    
    return 0xFF;  /* 未找到 */
}

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

/**
 * @brief 检查从机是否在线
 */
uint8_t modbusMasterIsOnline(ModbusHandle* handle, uint8_t slaveAddr)
{
    uint8_t idx;
    
    if (handle == NULL) {
        return FALSE;
    }
    
    idx = masterGetSlaveIndex(handle, slaveAddr);
    if (idx >= MODBUS_MAX_SLAVES) {
        return FALSE;
    }
    
    return (handle->masterCfg.writeCache[idx].failCount < 
            handle->masterCfg.offlineThreshold) ? TRUE : FALSE;
}

/**
 * @brief 标记需要写入
 */
void modbusMasterMarkWrite(ModbusHandle* handle, uint8_t slaveAddr)
{
    uint8_t idx;
    
    if (handle == NULL) {
        return; 
    }
    
    idx = masterGetSlaveIndex(handle, slaveAddr);
    if (idx < MODBUS_MAX_SLAVES) {
        handle->masterCfg.writeCache[idx].needWrite = TRUE;
    }
}

