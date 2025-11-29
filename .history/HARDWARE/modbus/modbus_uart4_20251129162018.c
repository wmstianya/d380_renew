/**
 * @file modbus_uart4.c
 * @brief UART4 ModBus双角色配置 (联控通信)
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 * 
 * @note UART4具有双角色：
 *       - 从机模式: 响应联控主机的查询
 *       - 主机模式: 轮询分机组1-10
 */

#include "modbus.h"
#include "system_control.h"
#include <string.h>

/*============================================================================*/
/*                              外部声明                                       */
/*============================================================================*/

extern UartHandle uartUnionHandle;  /* UART4 DMA驱动句柄 */

/* 分机通信状态 */
extern SLAVE_GG SlaveG[13];
extern USlave_Struct JiZu[12];

/*============================================================================*/
/*                              模块变量                                       */
/*============================================================================*/

/* ModBus句柄 */
static ModbusHandle modbusUart4;

/* 寄存器映射表 */
static ModbusRegEntry uart4RegEntries[2];
static ModbusRegMap uart4RegMap;

/* 主机配置 */
static ModbusMasterCfg uart4MasterCfg;
static ModbusMasterCallback uart4MasterCb;

/* 写入数据缓存 */
static uint8_t uart4WriteData[45];
static uint16_t uart4WriteLen;

/*============================================================================*/
/*                              从机寄存器读取                                 */
/*============================================================================*/

/**
 * @brief 读取本机状态 (地址100, 18寄存器)
 */
static uint16_t uart4ReadLocalStatus(uint16_t addr, uint16_t count, uint8_t* buf)
{
    uint16_t data16;
    
    if (addr != 100 || count != 18) {
        return 0;
    }
    
    /* 1: 工作状态 */
    buf[0] = 0;
    buf[1] = sys_data.Data_10H;
    
    /* 2: 压力值 */
    buf[2] = Temperature_Data.Pressure_Value >> 8;
    buf[3] = Temperature_Data.Pressure_Value & 0xFF;
    
    /* 3: 温度 (未使用) */
    buf[4] = 0;
    buf[5] = 0;
    
    /* 4: 水位状态 */
    buf[6] = 0;
    buf[7] = LCD10D.DLCD.Water_State;
    
    /* 5: 异常状态 */
    buf[8] = 0;
    buf[9] = sys_data.Data_12H;
    
    /* 6: 内部保护温度 */
    data16 = sys_flag.Protect_WenDu;
    buf[10] = data16 >> 8;
    buf[11] = data16 & 0xFF;
    
    /* 7: 故障代码 */
    buf[12] = 0;
    buf[13] = sys_flag.Error_Code;
    
    /* 8: 火焰状态 */
    buf[14] = 0;
    buf[15] = sys_flag.flame_state;
    
    /* 9: 风机转速 */
    data16 = sys_flag.Fan_Rpm;
    buf[16] = data16 >> 8;
    buf[17] = data16 & 0xFF;
    
    /* 10: 内部压力 */
    data16 = Temperature_Data.Inside_High_Pressure;
    buf[18] = data16 >> 8;
    buf[19] = data16 & 0xFF;
    
    /* 11: 工作功率 */
    buf[20] = 0;
    buf[21] = sys_data.Data_1FH;
    
    /* 12: 风机状态 */
    buf[22] = 0;
    buf[23] = Switch_Inf.air_on_flag;
    
    /* 13: 水阀 */
    buf[24] = 0;
    buf[25] = Switch_Inf.Water_Valve_Flag;
    
    /* 14: 排污阀状态 */
    buf[26] = 0;
    buf[27] = Switch_Inf.pai_wu_flag;
    
    /* 15: 连续排污状态 */
    buf[28] = 0;
    buf[29] = Switch_Inf.LianXu_PaiWu_flag;
    
    /* 16: 未使用 */
    buf[30] = 0;
    buf[31] = 0;
    
    /* 17: 点火/排污标志 (高字节=点火标志, 低字节=排污标志) */
    buf[32] = sys_flag.TimeDianHuo_Flag;
    buf[33] = sys_flag.Paiwu_Flag;
    
    /* 18: 输入状态 (16位) */
    buf[34] = sys_flag.Inputs_State >> 8;
    buf[35] = sys_flag.Inputs_State & 0xFF;
    
    return 36;  /* 18寄存器 = 36字节 */
}

/*============================================================================*/
/*                              从机寄存器写入                                 */
/*============================================================================*/

/**
 * @brief 写入控制命令 (地址200)
 */
static ModbusError uart4WriteControl(uint16_t addr, uint16_t count, 
                                      uint8_t* data, uint16_t dataLen)
{
    uint8_t saveFlag1 = 0;
    uint8_t saveFlag2 = 0;
    uint16_t data16;
    
    if (addr != 200 || dataLen < 36) {
        return MODBUS_ERR_PARAM;
    }
    
    /* 1: 启停控制 */
    if (data[1] == 1 || data[1] == 3) {
        sys_data.Data_10H = data[1];
    } else if (data[1] == 0) {
        sys_data.Data_10H = 0;
    }
    
    /* 2: 故障复位 */
    if (data[3] == 1) {
        sys_flag.Error_Code = 0;
    }
    
    /* 4: 功率设定 */
    if (data[7] >= 30 && data[7] <= 100) {
        LCD10D.DLCD.Air_Power = data[7];
    }
    
    /* 5: 继电器控制 */
    /* LCD10D.DLCD.Relays_Out - 通过独立处理 */
    
    /* 6: 空闲运行标志 */
    sys_flag.Idle_AirWork_Flag = data[11];
    
    /* 7: 排污标志 */
    sys_flag.Paiwu_Flag = data[13];
    
    /* 8: 目标压力 */
    if (sys_config_data.zhuan_huan_temperture_value != data[15]) {
        sys_config_data.zhuan_huan_temperture_value = data[15];
        saveFlag1 = 1;
    }
    
    /* 9: 停机压力 */
    if (sys_config_data.Auto_stop_pressure != data[17]) {
        sys_config_data.Auto_stop_pressure = data[17];
        saveFlag1 = 1;
    }
    
    /* 10: 启动压力 */
    if (sys_config_data.Auto_start_pressure != data[19]) {
        sys_config_data.Auto_start_pressure = data[19];
        saveFlag1 = 1;
    }
    
    /* 11: 额定压力 */
    if (Sys_Admin.DeviceMaxPressureSet != data[21]) {
        if (Sys_Admin.DeviceMaxPressureSet >= 80) {
            Sys_Admin.DeviceMaxPressureSet = data[21];
            saveFlag2 = 1;
        }
    }
    
    /* 12: 点火功率 */
    if (Sys_Admin.Dian_Huo_Power != data[23]) {
        if (data[23] >= 30 && data[23] <= 60) {
            Sys_Admin.Dian_Huo_Power = data[23];
            saveFlag2 = 1;
        }
    }
    
    /* 13: 最大功率 */
    if (Sys_Admin.Max_Work_Power != data[25]) {
        if (data[25] >= 30 && data[25] <= 100) {
            Sys_Admin.Max_Work_Power = data[25];
            saveFlag2 = 1;
        }
    }
    
    /* 14: 温度保护值 */
    data16 = (data[26] << 8) | data[27];
    if (Sys_Admin.Inside_WenDu_ProtectValue != data16) {
        if (data16 > 240 && data16 <= 350) {
            Sys_Admin.Inside_WenDu_ProtectValue = data16;
            saveFlag2 = 1;
        }
    }
    
    /* 15: 点火存活标志 */
    sys_flag.Union_DianHuo_Flag = data[29];
    
    /* 保存参数 */
    if (saveFlag1) {
        Write_Internal_Flash();
        // BEEP_TIME(100);  // 移除蜂鸣器提示
    }
    if (saveFlag2) {
        Write_Admin_Flash();
        // BEEP_TIME(100);  // 移除蜂鸣器提示
    }
    
    return MODBUS_OK;
}

/*============================================================================*/
/*                              主机回调函数                                   */
/*============================================================================*/

/**
 * @brief 主机收到响应回调
 */
static void uart4OnResponse(uint8_t slaveAddr, uint8_t funcCode, 
                            uint8_t* data, uint16_t dataLen)
{
    uint8_t addr = slaveAddr;
    uint16_t bufData;
    float bufFloat;
    
    SlaveG[addr].Send_Flag = 0;
    SlaveG[addr].Alive_Flag = OK;
    
    if (funcCode == 0x03 && dataLen >= 36) {
        /* 解析读取响应 */
        
        /* 设备状态 */
        if (SlaveG[addr].Send_Flag > 20) {
            JiZu[addr].Slave_D.Device_State = 0;
        } else {
            if (data[1] == 2)
                JiZu[addr].Slave_D.Device_State = 2;  /* 工作状态 */
            else if (data[1] == 0)
                JiZu[addr].Slave_D.Device_State = 1;  /* 待机模式 */
            else if (data[1] == 3)
                JiZu[addr].Slave_D.Device_State = 1;  /* 手动状态 */
            
            if (data[13])  /* 故障代码 */
                JiZu[addr].Slave_D.Device_State = 3;  /* 故障状态 */
        }
        
        /* 压力值 */
        bufFloat = (float)((data[2] << 8) | data[3]) / 100.0f;
        JiZu[addr].Slave_D.Dpressure = bufFloat;
        
        /* 水位状态 */
        JiZu[addr].Slave_D.Water_State = data[7];
        
        /* 内部温度 */
        JiZu[addr].Slave_D.Inside_WenDu = (data[10] << 8) | data[11];
        
        /* 故障代码 */
        JiZu[addr].Slave_D.Error_Code = data[13];
        if (JiZu[addr].Slave_D.Error_Code == 0) {
            JiZu[addr].Slave_D.Error_Reset = 0;
        }
        
        /* 火焰状态 */
        JiZu[addr].Slave_D.Flame = data[15];
        
        /* 风机转速 */
        JiZu[addr].Slave_D.Fan_Rpm = (data[16] << 8) | data[17];
        
        /* 工作功率 */
        JiZu[addr].Slave_D.Power = data[21];
        
        /* 水泵状态 */
        JiZu[addr].Slave_D.Pump_State = data[25];
        
        /* 点火标志 */
        SlaveG[addr].TimeDianHuo_Flag = data[32];
        
        /* 排污完成检查 */
        if (data[33] == 3 && SlaveG[addr].Paiwu_Flag != 3) {
            SlaveG[addr].Paiwu_Flag = 0;
        }
        
    } else if (funcCode == 0x10) {
        /* 写入响应确认 */
        SlaveG[addr].Command_SendFlag = 0;
    }
}

/**
 * @brief 主机超时回调
 */
static void uart4OnTimeout(uint8_t slaveAddr)
{
    SlaveG[slaveAddr].Send_Flag++;
    
    if (SlaveG[slaveAddr].Send_Flag > 20) {
        SlaveG[slaveAddr].Alive_Flag = FALSE;
        JiZu[slaveAddr].Slave_D.Device_State = 0;
        memset(&JiZu[slaveAddr].Datas, 0, sizeof(JiZu[slaveAddr].Datas));
    }
}

/**
 * @brief 主机设备离线回调
 */
static void uart4OnOffline(uint8_t slaveAddr)
{
    SlaveG[slaveAddr].Alive_Flag = FALSE;
    JiZu[slaveAddr].Slave_D.Device_State = 0;
}

/**
 * @brief 主机异常响应回调
 * @note 异常响应也表示通信成功，从机在线
 */
static void uart4OnException(uint8_t slaveAddr, uint8_t funcCode, uint8_t exCode)
{
    /* 异常响应表示从机在线且通信正常 */
    if (slaveAddr < 12) {
        SlaveG[slaveAddr].Send_Flag = 0;
        SlaveG[slaveAddr].Alive_Flag = OK;
    }
    
    /* 异常码说明:
     * 0x01: 非法功能码 - 从机不支持该功能
     * 0x02: 非法数据地址 - 请求的寄存器地址不存在
     * 0x03: 非法数据值 - 写入的值超出范围
     * 0x04: 从机设备故障 - 从机内部错误
     */
    (void)funcCode;
    (void)exCode;
}

/**
 * @brief 获取写入数据回调
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址 (输出)
 * @param data 数据缓冲 (输出)
 * @param maxLen 最大长度
 * @return 数据长度，0表示无需写入
 */
static uint16_t uart4GetWriteData(uint8_t slaveAddr, uint16_t* regAddr,
                                   uint8_t* data, uint16_t maxLen)
{
    uint8_t addr = slaveAddr;
    
    if (maxLen < 36) {
        return 0;
    }
    
    /* 输出寄存器地址 */
    *regAddr = 200;
    
    /* 1: 启停命令 */
    data[0] = 0;
    data[1] = JiZu[addr].Slave_D.StartFlag;
    
    /* 2: 故障复位 */
    data[2] = 0;
    data[3] = JiZu[addr].Slave_D.Error_Reset;
    
    /* 3: 设备类型 */
    data[4] = 0;
    data[5] = AUnionD.Devive_Style;
    
    /* 4: 输出功率 */
    data[6] = 0;
    data[7] = SlaveG[addr].Out_Power;
    
    /* 5: 继电器输出 */
    data[8] = JiZu[addr].Slave_D.Realys_Out >> 8;
    data[9] = JiZu[addr].Slave_D.Realys_Out & 0xFF;
    
    /* 6: 空闲运行标志 */
    data[10] = 0;
    data[11] = SlaveG[addr].Idle_AirWork_Flag;
    
    /* 7: 排污命令 */
    data[12] = 0;
    data[13] = SlaveG[addr].Paiwu_Flag;
    
    /* 8: 目标压力 */
    data[14] = 0;
    data[15] = sys_config_data.zhuan_huan_temperture_value;
    
    /* 9: 停机压力 */
    data[16] = 0;
    data[17] = sys_config_data.Auto_stop_pressure;
    
    /* 10: 启动压力 */
    data[18] = 0;
    data[19] = sys_config_data.Auto_start_pressure;
    
    /* 11: 额定压力 */
    data[20] = 0;
    data[21] = Sys_Admin.DeviceMaxPressureSet;
    
    /* 12: 点火功率 */
    data[22] = 0;
    data[23] = JiZu[addr].Slave_D.DianHuo_Value;
    
    /* 13: 最大功率 */
    data[24] = 0;
    data[25] = JiZu[addr].Slave_D.Max_Power;
    
    /* 14: 温度保护值 */
    data[26] = JiZu[addr].Slave_D.Inside_WenDu_Protect >> 8;
    data[27] = JiZu[addr].Slave_D.Inside_WenDu_Protect & 0xFF;
    
    /* 15: 点火存活标志 */
    data[28] = 0;
    data[29] = sys_flag.DianHuo_Alive;
    
    /* 16-18: 预留 */
    data[30] = 0; data[31] = 0;
    data[32] = 0; data[33] = 0;
    data[34] = 0; data[35] = 0;
    
    /* 返回纯数据字节数 (18寄存器 = 36字节) */
    return 36;
}

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

/**
 * @brief 初始化UART4 ModBus双角色
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusUart4Init(void)
{
    ModbusError err;
    uint8_t i;
    
    /* 初始化寄存器映射表 */
    memset(uart4RegEntries, 0, sizeof(uart4RegEntries));
    
    /* 从机寄存器: 地址100读取 */
    uart4RegEntries[0].startAddr = 100;
    uart4RegEntries[0].endAddr = 118;
    uart4RegEntries[0].readFunc = uart4ReadLocalStatus;
    uart4RegEntries[0].writeFunc = NULL;
    
    /* 从机寄存器: 地址200写入 */
    uart4RegEntries[1].startAddr = 200;
    uart4RegEntries[1].endAddr = 218;
    uart4RegEntries[1].readFunc = NULL;
    uart4RegEntries[1].writeFunc = uart4WriteControl;
    
    uart4RegMap.entries = uart4RegEntries;
    uart4RegMap.entryCount = 2;
    
    /* 初始化句柄 (双角色) */
    err = modbusInit(&modbusUart4, &uartUnionHandle, MODBUS_ROLE_DUAL);
    if (err != MODBUS_OK) {
        return err;
    }
    
    /* 配置从机模式 */
    err = modbusConfigSlave(&modbusUart4, Sys_Admin.ModBus_Address, 
                            &uart4RegMap, NULL);
    if (err != MODBUS_OK) {
        return err;
    }
    
    /* 配置主机模式 */
    memset(&uart4MasterCfg, 0, sizeof(uart4MasterCfg));
    
    /* 轮询地址1-10 */
    for (i = 0; i < 10; i++) {
        uart4MasterCfg.pollAddrs[i] = i + 1;
    }
    uart4MasterCfg.pollCount = 10;
    uart4MasterCfg.pollIntervalMs = 0;     /* 动态间隔: 收到响应立即切换 */
    uart4MasterCfg.respTimeoutMs = 1000;   /* 响应超时: 1000ms (1秒) */
    uart4MasterCfg.maxRetry = 20;
    uart4MasterCfg.readRegAddr = 100;
    uart4MasterCfg.readRegCount = 18;
    uart4MasterCfg.writeRegAddr = 200;
    
    /* 回调函数 */
    memset(&uart4MasterCb, 0, sizeof(uart4MasterCb));
    uart4MasterCb.onResponse = uart4OnResponse;
    uart4MasterCb.onTimeout = uart4OnTimeout;
    uart4MasterCb.onOffline = uart4OnOffline;
    uart4MasterCb.onException = uart4OnException;
    uart4MasterCb.getWriteData = uart4GetWriteData;
    
    err = modbusConfigMaster(&modbusUart4, &uart4MasterCfg, &uart4MasterCb);
    if (err != MODBUS_OK) {
        return err;
    }
    
    return MODBUS_OK;
}

/**
 * @brief UART4 ModBus调度 (主循环调用)
 */
void modbusUart4Scheduler(void)
{
    /* 更新从机地址 */
    modbusUart4.slaveAddr = Sys_Admin.ModBus_Address;
    
    /* 执行调度 */
    modbusScheduler(&modbusUart4);
}

/**
 * @brief 获取UART4 ModBus句柄 (调试用)
 */
ModbusHandle* modbusUart4GetHandle(void)
{
    return &modbusUart4;
}

