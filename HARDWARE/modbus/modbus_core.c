/**
 * @file modbus_core.c
 * @brief ModBus RTU 协议层核心实现
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 */

#include "modbus.h"
#include <string.h>

/*============================================================================*/
/*                              CRC16计算                                      */
/*============================================================================*/

/**
 * @brief CRC16校验计算 (ModBus RTU)
 * @param data 数据
 * @param len 长度
 * @return CRC16值 (高字节在前)
 */
uint16_t modbusCrc16(uint8_t* data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    uint16_t temp;
    
    for (i = 0; i < len - 2; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            temp = crc & 0x0001;
            crc >>= 1;
            if (temp) {
                crc ^= 0xA001;
            }
        }
    }
    
    /* 返回高字节在前的格式 */
    return ((crc & 0x00FF) << 8) | ((crc & 0xFF00) >> 8);
}

/**
 * @brief CRC校验
 * @param frame 帧数据
 * @param len 帧长度 (含CRC)
 * @return TRUE校验通过 / FALSE校验失败
 */
uint8_t modbusCrcCheck(uint8_t* frame, uint16_t len)
{
    uint16_t recvCrc;
    uint16_t calcCrc;
    
    if (len < 4) {
        return FALSE;
    }
    
    /* 提取接收到的CRC */
    recvCrc = (frame[len - 2] << 8) | frame[len - 1];
    
    /* 计算CRC */
    calcCrc = modbusCrc16(frame, len);
    
    return (recvCrc == calcCrc) ? TRUE : FALSE;
}

/*============================================================================*/
/*                              初始化                                         */
/*============================================================================*/

/**
 * @brief 初始化ModBus句柄
 */
ModbusError modbusInit(ModbusHandle* handle, UartHandle* uart, ModbusRole role)
{
    if (handle == NULL || uart == NULL) {
        return MODBUS_ERR_PARAM;
    }
    
    /* 清零 */
    memset(handle, 0, sizeof(ModbusHandle));
    
    /* 配置 */
    handle->uart = uart;
    handle->role = role;
    handle->state = MODBUS_STATE_IDLE;
    
    return MODBUS_OK;
}

/**
 * @brief 配置从机模式
 */
ModbusError modbusConfigSlave(ModbusHandle* handle, 
                               uint8_t address, 
                               ModbusRegMap* regMap,
                               ModbusSlaveCallback* callback)
{
    if (handle == NULL) {
        return MODBUS_ERR_PARAM;
    }
    
    if (!(handle->role & MODBUS_ROLE_SLAVE)) {
        return MODBUS_ERR_PARAM;
    }
    
    handle->slaveAddr = address;
    handle->regMap = regMap;
    
    if (callback != NULL) {
        handle->slaveCb = *callback;
    }
    
    return MODBUS_OK;
}

/**
 * @brief 配置主机模式
 */
ModbusError modbusConfigMaster(ModbusHandle* handle,
                                ModbusMasterCfg* config,
                                ModbusMasterCallback* callback)
{
    if (handle == NULL || config == NULL) {
        return MODBUS_ERR_PARAM;
    }
    
    if (!(handle->role & MODBUS_ROLE_MASTER)) {
        return MODBUS_ERR_PARAM;
    }
    
    handle->masterCfg = *config;
    
    if (callback != NULL) {
        handle->masterCb = *callback;
    }
    
    return MODBUS_OK;
}

/*============================================================================*/
/*                              接收检测                                       */
/*============================================================================*/

/**
 * @brief 检查是否有待处理的接收数据
 */
uint8_t modbusHasRxData(ModbusHandle* handle)
{
    uint8_t* rxData;
    uint16_t rxLen;
    
    if (handle == NULL || handle->uart == NULL) {
        return FALSE;
    }
    
    /* 检查DMA接收 */
    if (uartIsRxReady(handle->uart)) {
        if (uartGetRxData(handle->uart, &rxData, &rxLen) == UART_OK) {
            /* 复制到内部缓冲 */
            if (rxLen > 0 && rxLen <= MODBUS_MAX_FRAME) {
                memcpy(handle->rxBuf, rxData, rxLen);
                handle->rxLen = rxLen;
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

/*============================================================================*/
/*                              调度器                                         */
/*============================================================================*/

/**
 * @brief ModBus调度器 (主循环调用)
 */
void modbusScheduler(ModbusHandle* handle)
{
    if (handle == NULL) {
        return;
    }
    
    /* 检查接收数据 */
    if (modbusHasRxData(handle)) {
        /* 根据角色处理 */
        if (handle->role & MODBUS_ROLE_SLAVE) {
            /* 尝试作为从机处理 */
            if (handle->rxBuf[0] == handle->slaveAddr || 
                handle->rxBuf[0] == MODBUS_BROADCAST_ADDR) {
                modbusSlaveProcess(handle);
                uartClearRxFlag(handle->uart);
                return;
            }
        }
        
        if (handle->role & MODBUS_ROLE_MASTER) {
            /* 作为主机处理响应 */
            if (handle->state == MODBUS_STATE_MASTER_WAIT) {
                /* 在主机调度器中处理 */
            }
        }
    }
    
    /* 主机模式调度 */
    if (handle->role & MODBUS_ROLE_MASTER) {
        modbusMasterScheduler(handle);
    }
}

