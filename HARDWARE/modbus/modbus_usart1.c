/**
 * @file modbus_usart1.c
 * @brief USART1 ModBus从机配置 (RTU服务器)
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 * 
 * @note USART1作为从机，响应外部RTU客户端查询
 *       - 读取: 联控主数据、机组1-6数据
 *       - 写入: 启停控制、故障复位、压力参数
 */

#include "modbus.h"
#include "system_control.h"
#include <string.h>

/*============================================================================*/
/*                              外部声明                                       */
/*============================================================================*/

extern UartHandle uartDebugHandle;  /* USART1 DMA驱动句柄 */

/*============================================================================*/
/*                              模块变量                                       */
/*============================================================================*/

/* ModBus句柄 */
static ModbusHandle modbusUsart1;

/* 寄存器映射表 */
static ModbusRegEntry usart1RegEntries[4];
static ModbusRegMap usart1RegMap;

/*============================================================================*/
/*                              寄存器读取回调                                 */
/*============================================================================*/

/**
 * @brief 读取联控主数据 (地址0, 11寄存器)
 */
static uint16_t usart1ReadUnionData(uint16_t addr, uint16_t count, uint8_t* buf)
{
    uint16_t data1;
    
    if (addr != 0 || count != 11) {
        return 0;
    }
    
    /* 1: 联控启停状态 */
    buf[0] = 0;
    buf[1] = AUnionD.UnionStartFlag;
    
    /* 2: 当前压力值 */
    data1 = (uint16_t)(AUnionD.Big_Pressure * 100);
    buf[2] = data1 >> 8;
    buf[3] = data1 & 0xFF;
    
    /* 3: 在线设备数量 */
    buf[4] = 0;
    buf[5] = AUnionD.AliveOK_Numbers;
    
    /* 4: 故障设备数量 */
    buf[6] = 0;
    buf[7] = sys_flag.Device_ErrorNumbers;
    
    /* 5: 工作设备数量 */
    buf[8] = 0;
    buf[9] = sys_flag.Already_WorkNumbers;
    
    /* 6: 总控故障 */
    buf[10] = 0;
    buf[11] = 0;
    
    /* 7: 故障复位指令 */
    buf[12] = 0;
    buf[13] = 0;
    
    /* 8: 目标压力 */
    buf[14] = 0;
    buf[15] = (uint8_t)(AUnionD.Target_Value * 100);
    
    /* 9: 停机压力 */
    buf[16] = 0;
    buf[17] = (uint8_t)(AUnionD.Stop_Value * 100);
    
    /* 10: 启动压力 */
    buf[18] = 0;
    buf[19] = (uint8_t)(AUnionD.Start_Value * 100);
    
    /* 11: 未使用 */
    buf[20] = 0;
    buf[21] = 0;
    
    return 22;  /* 11寄存器 = 22字节 */
}

/**
 * @brief 读取机组数据 (地址0x0063起)
 */
static uint16_t usart1ReadJizuData(uint16_t addr, uint16_t count, uint8_t* buf)
{
    uint8_t jiZuIdx = 0;
    uint8_t bytes;
    uint8_t i;
    
    /* 计算机组索引 */
    if (addr == 0x0063) jiZuIdx = 1;
    else if (addr == 0x0077) jiZuIdx = 2;
    else if (addr == 0x008B) jiZuIdx = 3;
    else if (addr == 0x009F) jiZuIdx = 4;
    else if (addr == 0x00B3) jiZuIdx = 5;
    else if (addr == 0x00C7) jiZuIdx = 6;
    else return 0;
    
    /* 只启用机组1-2 */
    if (jiZuIdx > 2) {
        return 0;
    }
    
    bytes = sizeof(JiZu[jiZuIdx].Datas);
    
    /* 复制数据 (高低字节交换) */
    for (i = 0; i < bytes; i += 2) {
        buf[i] = JiZu[jiZuIdx].Datas[i + 1];
        buf[i + 1] = JiZu[jiZuIdx].Datas[i];
    }
    
    return bytes;
}

/**
 * @brief 读取设备地址 (特殊地址1000)
 */
static uint16_t usart1ReadDeviceAddr(uint16_t addr, uint16_t count, uint8_t* buf)
{
    if (addr != 1000 || count != 1) {
        return 0;
    }
    
    buf[0] = 0;
    buf[1] = Sys_Admin.ModBus_Address;
    
    return 2;
}

/*============================================================================*/
/*                              寄存器写入回调                                 */
/*============================================================================*/

/**
 * @brief 写入控制参数
 */
static ModbusError usart1WriteControl(uint16_t addr, uint16_t count, 
                                       uint8_t* data, uint16_t dataLen)
{
    uint16_t value;
    uint8_t index;
    
    if (dataLen < 2) {
        return MODBUS_ERR_PARAM;
    }
    
    value = (data[0] << 8) | data[1];
    
    switch (addr) {
        case 0x0000:  /* 联控启停 */
            if (value <= 1) {
                AUnionD.UnionStartFlag = (uint8_t)value;
                UnionD.UnionStartFlag = (uint8_t)value;
            }
            break;
            
        case 0x0006:  /* 故障复位 */
            UnionLCD.UnionD.Error_Reset = (uint8_t)value;
            AUnionD.Error_Reset = UnionLCD.UnionD.Error_Reset;
            UnionLCD.UnionD.Union_Error = 0;
            
            /* 复位所有分机故障 */
            for (index = 1; index <= 10; index++) {
                if (JiZu[index].Slave_D.Error_Code) {
                    SlaveG[index].Command_SendFlag = 3;
                    JiZu[index].Slave_D.Error_Reset = OK;
                }
            }
            break;
            
        case 0x0007:  /* 目标压力 */
            if (value >= 20 && value < sys_config_data.Auto_stop_pressure) {
                sys_config_data.zhuan_huan_temperture_value = value;
                Write_Internal_Flash();
            }
            break;
            
        case 0x0008:  /* 停机压力 */
            if (value > sys_config_data.zhuan_huan_temperture_value && 
                value < Sys_Admin.DeviceMaxPressureSet) {
                sys_config_data.Auto_stop_pressure = value;
                Write_Internal_Flash();
            }
            break;
            
        case 0x0009:  /* 启动压力 */
            if (value < sys_config_data.Auto_stop_pressure) {
                sys_config_data.Auto_start_pressure = value;
                Write_Internal_Flash();
            }
            break;
            
        default:
            return MODBUS_ERR_ADDR;
    }
    
    return MODBUS_OK;
}

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

/**
 * @brief 初始化USART1 ModBus从机
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusUsart1Init(void)
{
    ModbusError err;
    
    /* 初始化寄存器映射表 */
    memset(usart1RegEntries, 0, sizeof(usart1RegEntries));
    
    /* 地址0: 联控主数据 */
    usart1RegEntries[0].startAddr = 0x0000;
    usart1RegEntries[0].endAddr = 0x000B;
    usart1RegEntries[0].readFunc = usart1ReadUnionData;
    usart1RegEntries[0].writeFunc = usart1WriteControl;
    
    /* 地址0x0063-0x00C7: 机组数据 */
    usart1RegEntries[1].startAddr = 0x0063;
    usart1RegEntries[1].endAddr = 0x00D0;
    usart1RegEntries[1].readFunc = usart1ReadJizuData;
    usart1RegEntries[1].writeFunc = NULL;  /* 只读 */
    
    /* 地址1000: 设备地址查询 */
    usart1RegEntries[2].startAddr = 1000;
    usart1RegEntries[2].endAddr = 1000;
    usart1RegEntries[2].readFunc = usart1ReadDeviceAddr;
    usart1RegEntries[2].writeFunc = NULL;  /* 只读 */
    
    usart1RegMap.entries = usart1RegEntries;
    usart1RegMap.entryCount = 3;
    
    /* 初始化句柄 */
    err = modbusInit(&modbusUsart1, &uartDebugHandle, MODBUS_ROLE_SLAVE);
    if (err != MODBUS_OK) {
        return err;
    }
    
    /* 配置从机模式 */
    err = modbusConfigSlave(&modbusUsart1, Sys_Admin.ModBus_Address, 
                            &usart1RegMap, NULL);
    if (err != MODBUS_OK) {
        return err;
    }
    
    return MODBUS_OK;
}

/**
 * @brief USART1 ModBus调度 (主循环调用)
 */
void modbusUsart1Scheduler(void)
{
    /* 更新从机地址 (支持动态修改) */
    modbusUsart1.slaveAddr = Sys_Admin.ModBus_Address;
    
    /* 执行调度 */
    modbusScheduler(&modbusUsart1);
}

/**
 * @brief 获取USART1 ModBus句柄 (调试用)
 */
ModbusHandle* modbusUsart1GetHandle(void)
{
    return &modbusUsart1;
}

