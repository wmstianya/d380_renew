/**
 * @file modbus_usart2.c
 * @brief USART2 ModBus从机配置 (10.1寸屏幕通信)
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 * 
 * @note USART2作为从机，响应屏幕查询
 *       - 读取: 联控板主数据、机组1-10数据
 *       - 写入: 启停控制、PID参数、压力设置等
 */

#include "modbus.h"
#include "system_control.h"
#include <string.h>

/*============================================================================*/
/*                              外部声明                                       */
/*============================================================================*/

extern UartHandle uartDisplayHandle;  /* USART2 DMA驱动句柄 */

/* 浮点数字节转换联合体 */
typedef union {
    float value;
    struct {
        uint8_t dataLl;
        uint8_t dataLh;
        uint8_t dataHl;
        uint8_t dataHh;
    } byte4;
} FloatIntUnion;

/*============================================================================*/
/*                              模块变量                                       */
/*============================================================================*/

/* ModBus句柄 */
static ModbusHandle modbusUsart2;

/* 寄存器映射表 */
static ModbusRegEntry usart2RegEntries[3];
static ModbusRegMap usart2RegMap;

/*============================================================================*/
/*                              寄存器读取回调                                 */
/*============================================================================*/

/**
 * @brief 读取联控板主数据 (地址0x0000)
 */
static uint16_t usart2ReadUnionData(uint16_t addr, uint16_t count, uint8_t* buf)
{
    uint8_t bytes;
    uint8_t i;
    
    if (addr != 0x0000) {
        return 0;
    }
    
    bytes = sizeof(UnionLCD.Datas);
    
    for (i = 0; i < bytes; i++) {
        buf[i] = UnionLCD.Datas[i];
    }
    
    return bytes;
}

/**
 * @brief 读取机组数据 (地址0x0063-0x0117)
 */
static uint16_t usart2ReadJizuData(uint16_t addr, uint16_t count, uint8_t* buf)
{
    uint8_t jiZuIdx = 0;
    uint8_t bytes;
    uint8_t i;
    
    /* 计算机组索引 */
    switch (addr) {
        case 0x0063: jiZuIdx = 1;  break;
        case 0x0077: jiZuIdx = 2;  break;
        case 0x008B: jiZuIdx = 3;  break;
        case 0x009F: jiZuIdx = 4;  break;
        case 0x00B3: jiZuIdx = 5;  break;
        case 0x00C7: jiZuIdx = 6;  break;
        case 0x00DB: jiZuIdx = 7;  break;
        case 0x00EF: jiZuIdx = 8;  break;
        case 0x0103: jiZuIdx = 9;  break;
        case 0x0117: jiZuIdx = 10; break;
        default: return 0;
    }
    
    bytes = sizeof(JiZu[jiZuIdx].Datas);
    
    for (i = 0; i < bytes; i++) {
        buf[i] = JiZu[jiZuIdx].Datas[i];
    }
    
    return bytes;
}

/*============================================================================*/
/*                              寄存器写入回调                                 */
/*============================================================================*/

/**
 * @brief 写入启停控制 (地址0x0000)
 */
static void usart2WriteStartStop(uint16_t value)
{
    uint8_t idx;
    
    switch (value) {
        case 0:  /* 关闭联控 */
            if (AUnionD.UnionStartFlag == 1) {
                AUnionD.UnionStartFlag = 0;
                UnionLCD.UnionD.UnionStartFlag = 0;
            }
            if (AUnionD.UnionStartFlag == 3) {
                for (idx = 1; idx <= 10; idx++) {
                    JiZu[idx].Slave_D.StartFlag = 0;
                    JiZu[idx].Slave_D.Realys_Out = 0;
                    SlaveG[idx].Out_Power = 0;
                }
                AUnionD.UnionStartFlag = 0;
                UnionLCD.UnionD.UnionStartFlag = 0;
                UnionLCD.UnionD.ZongKong_RelaysOut = 0;
            }
            break;
            
        case 1:  /* 启动联控 */
            if (AUnionD.UnionStartFlag == 0) {
                AUnionD.UnionStartFlag = 1;
                UnionLCD.UnionD.UnionStartFlag = 1;
            }
            break;
            
        case 3:  /* 手动模式 */
            if (AUnionD.UnionStartFlag == 0) {
                for (idx = 1; idx <= 10; idx++) {
                    JiZu[idx].Slave_D.StartFlag = 3;
                    JiZu[idx].Slave_D.Realys_Out = 0;
                    SlaveG[idx].Out_Power = 0;
                }
                AUnionD.UnionStartFlag = 3;
                UnionLCD.UnionD.UnionStartFlag = 3;
                UnionLCD.UnionD.ZongKong_RelaysOut = 0;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 写入控制参数 (地址0x0000-0x001F)
 */
static ModbusError usart2WriteControl(uint16_t addr, uint16_t count, 
                                       uint8_t* data, uint16_t dataLen)
{
    uint16_t value;
    uint16_t bufInt;
    FloatIntUnion floatInt;
    
    if (dataLen < 2) {
        return MODBUS_ERR_PARAM;
    }
    
    value = (data[1] << 8) | data[0];  /* 低字节在前 */
    
    switch (addr) {
        case 0x0000:  /* 启停控制 */
            usart2WriteStartStop(value);
            break;
            
        case 0x0001:  /* 需用台数 */
            if (value <= 10) {
                UnionLCD.UnionD.Need_Numbers = (uint8_t)value;
                AUnionD.Need_Numbers = UnionLCD.UnionD.Need_Numbers;
            }
            break;
            
        case 0x0005:  /* PID转换时间 */
            if (value >= 5 && value <= 100) {
                UnionLCD.UnionD.PID_Next_Time = (uint8_t)value;
                AUnionD.PID_Next_Time = UnionLCD.UnionD.PID_Next_Time;
            }
            break;
            
        case 0x0006:  /* PID P值 */
            if (value <= 50) {
                UnionLCD.UnionD.PID_Pvalue = (uint8_t)value;
                AUnionD.PID_Pvalue = UnionLCD.UnionD.PID_Pvalue;
            }
            break;
            
        case 0x0007:  /* PID I值 */
            if (value <= 10) {
                UnionLCD.UnionD.PID_Ivalue = (uint8_t)value;
                AUnionD.PID_Ivalue = UnionLCD.UnionD.PID_Ivalue;
            }
            break;
            
        case 0x0008:  /* PID D值 */
            if (value <= 30) {
                UnionLCD.UnionD.PID_Dvalue = (uint8_t)value;
                AUnionD.PID_Dvalue = UnionLCD.UnionD.PID_Dvalue;
            }
            break;
            
        case 0x0009:  /* 16台设备使能标志 */
            UnionLCD.UnionD.Union16_Flag = value;
            AUnionD.Union16_Flag = UnionLCD.UnionD.Union16_Flag;
            break;
            
        case 0x000C:  /* 目标压力 (float32, 2寄存器) */
            if (count == 2 && dataLen >= 4) {
                floatInt.byte4.dataLl = data[0];
                floatInt.byte4.dataLh = data[1];
                floatInt.byte4.dataHl = data[2];
                floatInt.byte4.dataHh = data[3];
                bufInt = (uint16_t)(floatInt.value * 100);
                
                if (bufInt >= 10 && bufInt <= Sys_Admin.DeviceMaxPressureSet) {
                    sys_config_data.zhuan_huan_temperture_value = bufInt;
                    UnionLCD.UnionD.Target_Value = floatInt.value;
                    
                    if (sys_config_data.zhuan_huan_temperture_value > sys_config_data.Auto_stop_pressure) {
                        sys_config_data.Auto_stop_pressure = sys_config_data.zhuan_huan_temperture_value + 10;
                        if (sys_config_data.Auto_stop_pressure >= Sys_Admin.DeviceMaxPressureSet) {
                            sys_config_data.Auto_stop_pressure = Sys_Admin.DeviceMaxPressureSet - 1;
                        }
                        UnionLCD.UnionD.Stop_Value = (float)sys_config_data.Auto_stop_pressure / 100;
                    }
                    Write_Internal_Flash();
                }
            }
            break;
            
        case 0x000E:  /* 停止压力 (float32, 2寄存器) */
            if (count == 2 && dataLen >= 4) {
                floatInt.byte4.dataLl = data[0];
                floatInt.byte4.dataLh = data[1];
                floatInt.byte4.dataHl = data[2];
                floatInt.byte4.dataHh = data[3];
                bufInt = (uint16_t)(floatInt.value * 100);
                
                if (bufInt >= sys_config_data.zhuan_huan_temperture_value && 
                    bufInt <= Sys_Admin.DeviceMaxPressureSet) {
                    sys_config_data.Auto_stop_pressure = bufInt;
                    UnionLCD.UnionD.Stop_Value = floatInt.value;
                    Write_Internal_Flash();
                }
            }
            break;
            
        case 0x0010:  /* 启动压力 (float32, 2寄存器) */
            if (count == 2 && dataLen >= 4) {
                floatInt.byte4.dataLl = data[0];
                floatInt.byte4.dataLh = data[1];
                floatInt.byte4.dataHl = data[2];
                floatInt.byte4.dataHh = data[3];
                bufInt = (uint16_t)(floatInt.value * 100);
                
                if (bufInt < sys_config_data.Auto_stop_pressure) {
                    sys_config_data.Auto_start_pressure = bufInt;
                    UnionLCD.UnionD.Start_Value = floatInt.value;
                    Write_Internal_Flash();
                }
            }
            break;
            
        case 0x0012:  /* 额定压力 (float32, 2寄存器) */
            if (count == 2 && dataLen >= 4) {
                floatInt.byte4.dataLl = data[0];
                floatInt.byte4.dataLh = data[1];
                floatInt.byte4.dataHl = data[2];
                floatInt.byte4.dataHh = data[3];
                bufInt = (uint16_t)(floatInt.value * 100);
                
                AUnionD.Max_Pressure = floatInt.value;
                
                if (bufInt <= 250) {
                    Sys_Admin.DeviceMaxPressureSet = bufInt;
                    UnionLCD.UnionD.Max_Pressure = floatInt.value;
                    Write_Admin_Flash();
                }
            }
            break;
            
        /* 工作时间设置 20-29 */
        case 20:
            UnionLCD.UnionD.A1_WorkTime = value;
            AUnionD.A1_WorkTime = value;
            SlaveG[1].Work_Time = value;
            break;
        case 21:
            UnionLCD.UnionD.A2_WorkTime = value;
            AUnionD.A2_WorkTime = value;
            SlaveG[2].Work_Time = value;
            break;
        case 22:
            UnionLCD.UnionD.A3_WorkTime = value;
            AUnionD.A3_WorkTime = value;
            SlaveG[3].Work_Time = value;
            break;
        case 23:
            UnionLCD.UnionD.A4_WorkTime = value;
            AUnionD.A4_WorkTime = value;
            SlaveG[4].Work_Time = value;
            break;
        case 24:
            UnionLCD.UnionD.A5_WorkTime = value;
            AUnionD.A5_WorkTime = value;
            SlaveG[5].Work_Time = value;
            break;
        case 25:
            UnionLCD.UnionD.A6_WorkTime = value;
            AUnionD.A6_WorkTime = value;
            SlaveG[6].Work_Time = value;
            break;
        case 26:
            UnionLCD.UnionD.A7_WorkTime = value;
            AUnionD.A7_WorkTime = value;
            SlaveG[7].Work_Time = value;
            break;
        case 27:
            UnionLCD.UnionD.A8_WorkTime = value;
            AUnionD.A8_WorkTime = value;
            SlaveG[8].Work_Time = value;
            break;
        case 28:
            UnionLCD.UnionD.A9_WorkTime = value;
            AUnionD.A9_WorkTime = value;
            SlaveG[9].Work_Time = value;
            break;
        case 29:
            UnionLCD.UnionD.A10_WorkTime = value;
            AUnionD.A10_WorkTime = value;
            SlaveG[10].Work_Time = value;
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
 * @brief 初始化USART2 ModBus从机
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusUsart2Init(void)
{
    ModbusError err;
    
    /* 初始化寄存器映射表 */
    memset(usart2RegEntries, 0, sizeof(usart2RegEntries));
    
    /* 联控板主数据区 */
    usart2RegEntries[0].startAddr = 0x0000;
    usart2RegEntries[0].endAddr = 0x0050;
    usart2RegEntries[0].readFunc = usart2ReadUnionData;
    usart2RegEntries[0].writeFunc = usart2WriteControl;
    
    /* 机组数据区 */
    usart2RegEntries[1].startAddr = 0x0063;
    usart2RegEntries[1].endAddr = 0x0130;
    usart2RegEntries[1].readFunc = usart2ReadJizuData;
    usart2RegEntries[1].writeFunc = NULL;  /* 只读 */
    
    usart2RegMap.entries = usart2RegEntries;
    usart2RegMap.entryCount = 2;
    
    /* 初始化句柄 */
    err = modbusInit(&modbusUsart2, &uartDisplayHandle, MODBUS_ROLE_SLAVE);
    if (err != MODBUS_OK) {
        return err;
    }
    
    /* 配置从机模式, 地址为0x01 */
    err = modbusConfigSlave(&modbusUsart2, 0x01, &usart2RegMap, NULL);
    if (err != MODBUS_OK) {
        return err;
    }
    
    return MODBUS_OK;
}

/**
 * @brief USART2 ModBus调度 (主循环调用)
 */
void modbusUsart2Scheduler(void)
{
    /* 执行调度 */
    modbusScheduler(&modbusUsart2);
    
    /* 屏幕连接状态更新 */
    if (modbusUsart2.state == MODBUS_STATE_IDLE) {
        sys_flag.LCD10_Connect = OK;
    }
}

/**
 * @brief 获取USART2 ModBus句柄 (调试用)
 */
ModbusHandle* modbusUsart2GetHandle(void)
{
    return &modbusUsart2;
}

