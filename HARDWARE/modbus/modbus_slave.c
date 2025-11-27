/**
 * @file modbus_slave.c
 * @brief ModBus RTU 从机模块实现
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 */

#include "modbus.h"
#include <string.h>

/*============================================================================*/
/*                              内部函数声明                                   */
/*============================================================================*/

static void slaveHandleRead(ModbusHandle* handle, uint16_t regAddr, uint16_t regCount);
static void slaveHandleWriteSingle(ModbusHandle* handle, uint16_t regAddr, uint16_t value);
static void slaveHandleWriteMulti(ModbusHandle* handle, uint16_t regAddr, 
                                   uint16_t regCount, uint8_t* data, uint16_t dataLen);
static void slaveSendException(ModbusHandle* handle, uint8_t funcCode, uint8_t exCode);
static ModbusRegEntry* slaveFindRegEntry(ModbusHandle* handle, uint16_t addr);

/*============================================================================*/
/*                              从机处理入口                                   */
/*============================================================================*/

/**
 * @brief 从机处理入口
 */
ModbusError modbusSlaveProcess(ModbusHandle* handle)
{
    uint8_t* frame;
    uint16_t len;
    uint8_t funcCode;
    uint16_t regAddr;
    uint16_t regCount;
    uint16_t value;
    
    if (handle == NULL || handle->rxLen < 4) {
        return MODBUS_ERR_PARAM;
    }
    
    frame = handle->rxBuf;
    len = handle->rxLen;
    
    /* 地址过滤 */
    if (frame[0] != handle->slaveAddr && frame[0] != MODBUS_BROADCAST_ADDR) {
        return MODBUS_ERR_ADDR;
    }
    
    /* CRC校验 */
    if (!modbusCrcCheck(frame, len)) {
        handle->stats.crcErrCount++;
        return MODBUS_ERR_CRC;
    }
    
    handle->stats.rxCount++;
    
    /* 解析功能码和地址 */
    funcCode = frame[1];
    regAddr = (frame[2] << 8) | frame[3];
    
    /* 功能码分发 */
    switch (funcCode) {
        case MODBUS_FC_READ_HOLDING:    /* 03 读保持寄存器 */
            regCount = (frame[4] << 8) | frame[5];
            slaveHandleRead(handle, regAddr, regCount);
            break;
            
        case MODBUS_FC_WRITE_SINGLE:    /* 06 写单个寄存器 */
            value = (frame[4] << 8) | frame[5];
            slaveHandleWriteSingle(handle, regAddr, value);
            break;
            
        case MODBUS_FC_WRITE_MULTI:     /* 10 写多个寄存器 */
            regCount = (frame[4] << 8) | frame[5];
            slaveHandleWriteMulti(handle, regAddr, regCount, &frame[7], frame[6]);
            break;
            
        default:
            slaveSendException(handle, funcCode, MODBUS_EX_ILLEGAL_FUNC);
            break;
    }
    
    return MODBUS_OK;
}

/*============================================================================*/
/*                              功能码处理                                     */
/*============================================================================*/

/**
 * @brief 处理读取请求 (功能码03)
 */
static void slaveHandleRead(ModbusHandle* handle, uint16_t regAddr, uint16_t regCount)
{
    ModbusRegEntry* entry;
    uint16_t dataLen;
    uint16_t crc;
    uint8_t* txBuf = handle->txBuf;
    
    /* 查找寄存器映射 */
    entry = slaveFindRegEntry(handle, regAddr);
    if (entry == NULL || entry->readFunc == NULL) {
        slaveSendException(handle, MODBUS_FC_READ_HOLDING, MODBUS_EX_ILLEGAL_ADDR);
        return;
    }
    
    /* 调用读取回调 */
    dataLen = entry->readFunc(regAddr, regCount, &txBuf[3]);
    if (dataLen == 0) {
        slaveSendException(handle, MODBUS_FC_READ_HOLDING, MODBUS_EX_ILLEGAL_ADDR);
        return;
    }
    
    /* 构建响应帧 */
    txBuf[0] = handle->slaveAddr;
    txBuf[1] = MODBUS_FC_READ_HOLDING;
    txBuf[2] = (uint8_t)dataLen;
    
    /* 计算CRC */
    crc = modbusCrc16(txBuf, dataLen + 5);
    txBuf[dataLen + 3] = crc >> 8;
    txBuf[dataLen + 4] = crc & 0xFF;
    
    /* 发送响应 */
    uartSendDma(handle->uart, txBuf, dataLen + 5);
    handle->stats.txCount++;
}

/**
 * @brief 处理单寄存器写入 (功能码06)
 */
static void slaveHandleWriteSingle(ModbusHandle* handle, uint16_t regAddr, uint16_t value)
{
    ModbusRegEntry* entry;
    ModbusError err;
    uint16_t crc;
    uint8_t* txBuf = handle->txBuf;
    uint8_t data[2];
    
    /* 查找寄存器映射 */
    entry = slaveFindRegEntry(handle, regAddr);
    if (entry == NULL || entry->writeFunc == NULL) {
        slaveSendException(handle, MODBUS_FC_WRITE_SINGLE, MODBUS_EX_ILLEGAL_ADDR);
        return;
    }
    
    /* 准备数据 */
    data[0] = value >> 8;
    data[1] = value & 0xFF;
    
    /* 调用写入回调 */
    err = entry->writeFunc(regAddr, 1, data, 2);
    if (err != MODBUS_OK) {
        slaveSendException(handle, MODBUS_FC_WRITE_SINGLE, MODBUS_EX_ILLEGAL_VALUE);
        return;
    }
    
    /* 回显响应 */
    txBuf[0] = handle->slaveAddr;
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
    
    /* 写入完成回调 */
    if (handle->slaveCb.onWriteComplete) {
        handle->slaveCb.onWriteComplete(MODBUS_FC_WRITE_SINGLE, regAddr, data, 2);
    }
}

/**
 * @brief 处理多寄存器写入 (功能码10)
 */
static void slaveHandleWriteMulti(ModbusHandle* handle, uint16_t regAddr, 
                                   uint16_t regCount, uint8_t* data, uint16_t dataLen)
{
    ModbusRegEntry* entry;
    ModbusError err;
    uint16_t crc;
    uint8_t* txBuf = handle->txBuf;
    
    /* 查找寄存器映射 */
    entry = slaveFindRegEntry(handle, regAddr);
    if (entry == NULL || entry->writeFunc == NULL) {
        slaveSendException(handle, MODBUS_FC_WRITE_MULTI, MODBUS_EX_ILLEGAL_ADDR);
        return;
    }
    
    /* 调用写入回调 */
    err = entry->writeFunc(regAddr, regCount, data, dataLen);
    if (err != MODBUS_OK) {
        slaveSendException(handle, MODBUS_FC_WRITE_MULTI, MODBUS_EX_ILLEGAL_VALUE);
        return;
    }
    
    /* 确认响应 */
    txBuf[0] = handle->slaveAddr;
    txBuf[1] = MODBUS_FC_WRITE_MULTI;
    txBuf[2] = regAddr >> 8;
    txBuf[3] = regAddr & 0xFF;
    txBuf[4] = regCount >> 8;
    txBuf[5] = regCount & 0xFF;
    
    crc = modbusCrc16(txBuf, 8);
    txBuf[6] = crc >> 8;
    txBuf[7] = crc & 0xFF;
    
    uartSendDma(handle->uart, txBuf, 8);
    handle->stats.txCount++;
    
    /* 写入完成回调 */
    if (handle->slaveCb.onWriteComplete) {
        handle->slaveCb.onWriteComplete(MODBUS_FC_WRITE_MULTI, regAddr, data, dataLen);
    }
}

/**
 * @brief 发送异常响应
 */
static void slaveSendException(ModbusHandle* handle, uint8_t funcCode, uint8_t exCode)
{
    uint16_t crc;
    uint8_t* txBuf = handle->txBuf;
    
    txBuf[0] = handle->slaveAddr;
    txBuf[1] = funcCode | 0x80;  /* 功能码+0x80表示异常 */
    txBuf[2] = exCode;
    
    crc = modbusCrc16(txBuf, 5);
    txBuf[3] = crc >> 8;
    txBuf[4] = crc & 0xFF;
    
    uartSendDma(handle->uart, txBuf, 5);
    handle->stats.txCount++;
}

/**
 * @brief 查找寄存器映射项
 */
static ModbusRegEntry* slaveFindRegEntry(ModbusHandle* handle, uint16_t addr)
{
    uint8_t i;
    ModbusRegMap* map = handle->regMap;
    
    if (map == NULL || map->entries == NULL) {
        return NULL;
    }
    
    for (i = 0; i < map->entryCount; i++) {
        if (addr >= map->entries[i].startAddr && addr <= map->entries[i].endAddr) {
            return &map->entries[i];
        }
    }
    
    return NULL;
}

