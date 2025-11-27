/**
 * @file modbus_usart3.c
 * @brief USART3 ModBus主机配置 (变频供水阀)
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 * 
 * @note USART3作为主机，轮询变频供水阀设备 (地址8)
 *       - 读取: 地址0x0000, 故障状态
 *       - 写入: 地址0x0001, 开度值(0-1000)
 */

#include "modbus.h"
#include "system_control.h"
#include <string.h>

/*============================================================================*/
/*                              外部声明                                       */
/*============================================================================*/

extern UartHandle uartSlaveHandle;  /* USART3 DMA驱动句柄 */

/*============================================================================*/
/*                              模块变量                                       */
/*============================================================================*/

/* ModBus句柄 */
static ModbusHandle modbusUsart3;

/* 主机配置 (在init函数中初始化) */
static ModbusMasterCfg usart3MasterCfg;

/* 上次写入的开度值 (按需写入优化) */
static uint16_t lastWaterPercent = 0xFFFF;

/*============================================================================*/
/*                              回调函数                                       */
/*============================================================================*/

/**
 * @brief 响应接收回调
 */
static void usart3OnResponse(uint8_t slaveAddr, uint8_t funcCode, 
                              uint8_t* data, uint16_t dataLen)
{
    uint16_t value;
    
    if (slaveAddr != 8) {
        return;
    }
    
    /* 清除发送计数 */
    sys_flag.waterSend_Flag = FALSE;
    sys_flag.waterSend_Count = 0;
    
    if (funcCode == MODBUS_FC_READ_HOLDING && dataLen >= 2) {
        /* 读取响应: 获取故障状态 */
        value = (data[0] << 8) | data[1];
        
        if (value == 0x00EA) {
            sys_flag.Water_Error_Code = OK;  /* 故障状态 */
            sys_flag.Error_Code = Error18_SupplyWater_Error;
        } else {
            sys_flag.Water_Error_Code = 0;
        }
    }
}

/**
 * @brief 超时回调
 */
static void usart3OnTimeout(uint8_t slaveAddr)
{
    if (slaveAddr == 8) {
        sys_flag.waterSend_Count++;
    }
}

/**
 * @brief 离线回调
 */
static void usart3OnOffline(uint8_t slaveAddr)
{
    if (slaveAddr == 8) {
        sys_flag.Error_Code = Error19_SupplyWater_UNtalk;
    }
}

/**
 * @brief 获取写入数据回调
 */
static uint16_t usart3GetWriteData(uint8_t slaveAddr, uint16_t* regAddr,
                                    uint8_t* buffer, uint16_t maxLen)
{
    uint16_t percent;
    
    if (slaveAddr != 8 || maxLen < 2) {
        return 0;
    }
    
    /* 检查供水使能 */
    if (!Sys_Admin.Water_BianPin_Enabled) {
        return 0;
    }
    
    /* 计算开度值 */
    if (sys_flag.Water_Percent <= 100) {
        percent = sys_flag.Water_Percent * 10;  /* 0-1000 */
    } else {
        percent = 0;  /* 值异常则关闭 */
    }
    
    /* 按需写入: 只有值变化时才写入 */
    if (percent == lastWaterPercent) {
        return 0;  /* 无需写入 */
    }
    
    /* 更新缓存 */
    lastWaterPercent = percent;
    
    /* 填充数据 */
    *regAddr = 0x0001;
    buffer[0] = percent >> 8;
    buffer[1] = percent & 0xFF;
    
    return 2;  /* 1个寄存器 = 2字节 */
}

/* 主机回调集合 (在init函数中初始化) */
static ModbusMasterCallback usart3MasterCb;

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

/**
 * @brief 初始化USART3 ModBus主机
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusUsart3Init(void)
{
    ModbusError err;
    
    /* 初始化主机配置 */
    memset(&usart3MasterCfg, 0, sizeof(usart3MasterCfg));
    usart3MasterCfg.pollAddrs[0] = 8;       /* 变频供水阀地址 */
    usart3MasterCfg.pollCount = 1;
    usart3MasterCfg.currentIdx = 0;
    usart3MasterCfg.currentSendIdx = 0;
    usart3MasterCfg.readRegAddr = 0x0000;   /* 读取故障状态地址 */
    usart3MasterCfg.readRegCount = 1;
    usart3MasterCfg.writeRegAddr = 0x0001;  /* 写入开度值地址 */
    usart3MasterCfg.respTimeoutMs = 50;     /* 响应超时50ms */
    usart3MasterCfg.pollIntervalMs = 200;   /* 轮询间隔200ms */
    usart3MasterCfg.maxRetry = 3;
    usart3MasterCfg.offlineThreshold = 10;
    
    /* 初始化回调 */
    memset(&usart3MasterCb, 0, sizeof(usart3MasterCb));
    usart3MasterCb.onResponse = usart3OnResponse;
    usart3MasterCb.onTimeout = usart3OnTimeout;
    usart3MasterCb.onOffline = usart3OnOffline;
    usart3MasterCb.getWriteData = usart3GetWriteData;
    
    /* 初始化句柄 */
    err = modbusInit(&modbusUsart3, &uartSlaveHandle, MODBUS_ROLE_MASTER);
    if (err != MODBUS_OK) {
        return err;
    }
    
    /* 配置主机模式 */
    err = modbusConfigMaster(&modbusUsart3, &usart3MasterCfg, &usart3MasterCb);
    if (err != MODBUS_OK) {
        return err;
    }
    
    return MODBUS_OK;
}

/**
 * @brief USART3 ModBus调度 (主循环调用)
 */
void modbusUsart3Scheduler(void)
{
    /* 检查供水使能 */
    if (!Sys_Admin.Water_BianPin_Enabled) {
        sys_flag.waterSend_Count = 0;
        return;
    }
    
    /* 执行调度 */
    modbusScheduler(&modbusUsart3);
}

/**
 * @brief 获取USART3 ModBus句柄 (调试用)
 */
ModbusHandle* modbusUsart3GetHandle(void)
{
    return &modbusUsart3;
}

