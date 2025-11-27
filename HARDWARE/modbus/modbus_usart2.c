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

/* 外部函数声明 */
extern void ZongKong_YanFa_Open(void);
extern void ZongKong_YanFa_Close(void);
extern void Write_Admin_Flash(void);

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
    uint8_t index;
    uint8_t jiZuIdx;
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
            
        /* ============ 系统控制地址 30-41 ============ */
        case 30:  /* 故障复位 */
            UnionLCD.UnionD.Error_Reset = (uint8_t)value;
            AUnionD.Error_Reset = UnionLCD.UnionD.Error_Reset;
            UnionLCD.UnionD.Union_Error = 0;
            for (index = 1; index <= 10; index++) {
                if (JiZu[index].Slave_D.Error_Code) {
                    SlaveG[index].Command_SendFlag = 3;
                    JiZu[index].Slave_D.Error_Reset = OK;
                }
            }
            break;
            
        case 31:  /* 报警关闭 */
            UnionLCD.UnionD.Alarm_OFF = (uint8_t)value;
            AUnionD.Alarm_OFF = UnionLCD.UnionD.Alarm_OFF;
            break;
            
        case 32:  /* 设备类型 */
            UnionLCD.UnionD.Devive_Style = (uint8_t)value;
            AUnionD.Devive_Style = UnionLCD.UnionD.Devive_Style;
            for (index = 1; index <= 10; index++) {
                if (SlaveG[index].Alive_Flag && JiZu[index].Slave_D.Error_Code) {
                    SlaveG[index].Command_SendFlag = 3;
                    JiZu[index].Slave_D.Error_Code = 0;
                }
            }
            break;
            
        case 33:  /* 额定设备数量 */
            if (value <= 10) {
                UnionLCD.UnionD.Max_Address = (uint8_t)value;
                AUnionD.Max_Address = UnionLCD.UnionD.Max_Address;
            }
            break;
            
        case 35:  /* ModBus地址 */
            if (value <= 250) {
                UnionLCD.UnionD.ModBus_Address = (uint8_t)value;
                AUnionD.ModBus_Address = UnionLCD.UnionD.ModBus_Address;
                Sys_Admin.ModBus_Address = UnionLCD.UnionD.ModBus_Address;
            }
            break;
            
        case 36:  /* 禁止设备数量 */
            if (value <= 10) {
                UnionLCD.UnionD.OFFlive_Numbers = (uint8_t)value;
                AUnionD.OFFlive_Numbers = UnionLCD.UnionD.OFFlive_Numbers;
            }
            break;
            
        case 38:  /* 排烟温度报警值 */
            if (value >= 80 && value <= 200) {
                UnionLCD.UnionD.PaiYan_AlarmValue = (uint8_t)value;
                AUnionD.PaiYan_AlarmValue = UnionLCD.UnionD.PaiYan_AlarmValue;
                Sys_Admin.Danger_Smoke_Value = value * 10;
                Write_Admin_Flash();
            }
            break;
            
        case 40:  /* 报警允许标志 */
            UnionLCD.UnionD.Alarm_Allow_Flag = (uint8_t)value;
            AUnionD.Alarm_Allow_Flag = UnionLCD.UnionD.Alarm_Allow_Flag;
            break;
            
        case 41:  /* 总控继电器输出 */
            UnionLCD.UnionD.ZongKong_RelaysOut = value;
            AUnionD.ZongKong_RelaysOut = UnionLCD.UnionD.ZongKong_RelaysOut;
            if (value & 0x0001) {
                ZongKong_YanFa_Open();
            } else {
                ZongKong_YanFa_Close();
            }
            break;
            
        /* ============ 机组1控制 (101-116) ============ */
        case 101: SlaveG[1].Out_Power = (uint8_t)value; JiZu[1].Slave_D.Power = (uint8_t)value; break;
        case 108: JiZu[1].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[1].Paiwu_Flag = (uint8_t)value; break;
        case 113: JiZu[1].Slave_D.Realys_Out = value; break;
        case 114: if (value >= 30 && value < 60) JiZu[1].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 115: if (value >= 30 && value <= 100) JiZu[1].Slave_D.Max_Power = (uint8_t)value; break;
        case 116: if (value < 350) JiZu[1].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组2控制 (121-136) ============ */
        case 121: SlaveG[2].Out_Power = (uint8_t)value; JiZu[2].Slave_D.Power = (uint8_t)value; break;
        case 128: JiZu[2].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[2].Paiwu_Flag = (uint8_t)value; break;
        case 133: JiZu[2].Slave_D.Realys_Out = value; break;
        case 134: if (value >= 30 && value < 60) JiZu[2].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 135: if (value >= 30 && value <= 100) JiZu[2].Slave_D.Max_Power = (uint8_t)value; break;
        case 136: if (value < 350) JiZu[2].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组3控制 (141-156) ============ */
        case 141: SlaveG[3].Out_Power = (uint8_t)value; JiZu[3].Slave_D.Power = (uint8_t)value; break;
        case 148: JiZu[3].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[3].Paiwu_Flag = (uint8_t)value; break;
        case 153: JiZu[3].Slave_D.Realys_Out = value; break;
        case 154: if (value >= 30 && value < 60) JiZu[3].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 155: if (value >= 30 && value <= 100) JiZu[3].Slave_D.Max_Power = (uint8_t)value; break;
        case 156: if (value < 350) JiZu[3].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组4控制 (161-176) ============ */
        case 161: SlaveG[4].Out_Power = (uint8_t)value; JiZu[4].Slave_D.Power = (uint8_t)value; break;
        case 168: JiZu[4].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[4].Paiwu_Flag = (uint8_t)value; break;
        case 173: JiZu[4].Slave_D.Realys_Out = value; break;
        case 174: if (value >= 30 && value < 60) JiZu[4].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 175: if (value >= 30 && value <= 100) JiZu[4].Slave_D.Max_Power = (uint8_t)value; break;
        case 176: if (value < 350) JiZu[4].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组5控制 (181-196) ============ */
        case 181: SlaveG[5].Out_Power = (uint8_t)value; JiZu[5].Slave_D.Power = (uint8_t)value; break;
        case 188: JiZu[5].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[5].Paiwu_Flag = (uint8_t)value; break;
        case 193: JiZu[5].Slave_D.Realys_Out = value; break;
        case 194: if (value >= 30 && value < 60) JiZu[5].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 195: if (value >= 30 && value <= 100) JiZu[5].Slave_D.Max_Power = (uint8_t)value; break;
        case 196: if (value < 350) JiZu[5].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组6控制 (201-216) ============ */
        case 201: SlaveG[6].Out_Power = (uint8_t)value; JiZu[6].Slave_D.Power = (uint8_t)value; break;
        case 208: JiZu[6].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[6].Paiwu_Flag = (uint8_t)value; break;
        case 213: JiZu[6].Slave_D.Realys_Out = value; break;
        case 214: if (value >= 30 && value < 60) JiZu[6].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 215: if (value >= 30 && value <= 100) JiZu[6].Slave_D.Max_Power = (uint8_t)value; break;
        case 216: if (value < 350) JiZu[6].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组7控制 (221-236) ============ */
        case 221: SlaveG[7].Out_Power = (uint8_t)value; JiZu[7].Slave_D.Power = (uint8_t)value; break;
        case 228: JiZu[7].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[7].Paiwu_Flag = (uint8_t)value; break;
        case 233: JiZu[7].Slave_D.Realys_Out = value; break;
        case 234: if (value >= 30 && value < 60) JiZu[7].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 235: if (value >= 30 && value <= 100) JiZu[7].Slave_D.Max_Power = (uint8_t)value; break;
        case 236: if (value < 350) JiZu[7].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组8控制 (241-256) ============ */
        case 241: SlaveG[8].Out_Power = (uint8_t)value; JiZu[8].Slave_D.Power = (uint8_t)value; break;
        case 248: JiZu[8].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[8].Paiwu_Flag = (uint8_t)value; break;
        case 253: JiZu[8].Slave_D.Realys_Out = value; break;
        case 254: if (value >= 30 && value < 60) JiZu[8].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 255: if (value >= 30 && value <= 100) JiZu[8].Slave_D.Max_Power = (uint8_t)value; break;
        case 256: if (value < 350) JiZu[8].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组9控制 (261-276) ============ */
        case 261: SlaveG[9].Out_Power = (uint8_t)value; JiZu[9].Slave_D.Power = (uint8_t)value; break;
        case 268: JiZu[9].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[9].Paiwu_Flag = (uint8_t)value; break;
        case 273: JiZu[9].Slave_D.Realys_Out = value; break;
        case 274: if (value >= 30 && value < 60) JiZu[9].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 275: if (value >= 30 && value <= 100) JiZu[9].Slave_D.Max_Power = (uint8_t)value; break;
        case 276: if (value < 350) JiZu[9].Slave_D.Inside_WenDu_Protect = value; break;
        
        /* ============ 机组10控制 (281-296) ============ */
        case 281: SlaveG[10].Out_Power = (uint8_t)value; JiZu[10].Slave_D.Power = (uint8_t)value; break;
        case 288: JiZu[10].Slave_D.PaiwuFa_State = (uint8_t)value; SlaveG[10].Paiwu_Flag = (uint8_t)value; break;
        case 293: JiZu[10].Slave_D.Realys_Out = value; break;
        case 294: if (value >= 30 && value < 60) JiZu[10].Slave_D.DianHuo_Value = (uint8_t)value; break;
        case 295: if (value >= 30 && value <= 100) JiZu[10].Slave_D.Max_Power = (uint8_t)value; break;
        case 296: if (value < 350) JiZu[10].Slave_D.Inside_WenDu_Protect = value; break;
            
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
    
    /* 联控板主数据区 (含系统控制30-41) */
    usart2RegEntries[0].startAddr = 0x0000;
    usart2RegEntries[0].endAddr = 0x0050;
    usart2RegEntries[0].readFunc = usart2ReadUnionData;
    usart2RegEntries[0].writeFunc = usart2WriteControl;
    
    /* 机组数据区 (含机组控制101-296) */
    usart2RegEntries[1].startAddr = 0x0063;
    usart2RegEntries[1].endAddr = 0x0130;
    usart2RegEntries[1].readFunc = usart2ReadJizuData;
    usart2RegEntries[1].writeFunc = usart2WriteControl;  /* 支持写入 */
    
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

