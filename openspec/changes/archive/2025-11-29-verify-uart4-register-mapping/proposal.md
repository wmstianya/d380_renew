# Change: 验证UART4寄存器映射一致性

## Why
新的 ModBus 驱动 (`modbus_uart4.c`) 需要与原驱动 (`usart4.c`) 保持寄存器映射完全一致，以确保通信协议兼容性。本提案进行精准比对并修复任何差异。

## What Changes
- 比对从机读取寄存器 (地址100, 18寄存器)
- 比对从机写入寄存器 (地址200, 18寄存器)
- 比对主机轮询发送的写入数据
- 比对主机接收响应的解析逻辑
- **修复发现的差异**

## Impact
- Affected specs: modbus-unified
- Affected code: `HARDWARE/modbus/modbus_uart4.c`

---

# 寄存器映射比对结果

## 1. 从机读取寄存器 (地址100, 18寄存器) - 03功能码

### 原驱动 (usart4.c:257-334)

| 寄存器 | 索引 | 高字节 | 低字节 | 说明 |
|--------|------|--------|--------|------|
| 1 | TX[3-4] | 0 | sys_data.Data_10H | 工作状态 |
| 2 | TX[5-6] | Pressure_Value >> 8 | & 0xFF | 压力值 |
| 3 | TX[7-8] | 0 | 0 | 温度(未使用) |
| 4 | TX[9-10] | 0 | LCD10D.DLCD.Water_State | 水位状态 |
| 5 | TX[11-12] | 0 | sys_data.Data_12H | 异常状态 |
| 6 | TX[13-14] | Protect_WenDu >> 8 | & 0xFF | 内部保护温度 |
| 7 | TX[15-16] | 0 | sys_flag.Error_Code | 故障代码 |
| 8 | TX[17-18] | 0 | sys_flag.flame_state | 火焰状态 |
| 9 | TX[19-20] | Fan_Rpm >> 8 | & 0xFF | 风机转速 |
| 10 | TX[21-22] | Inside_High_Pressure >> 8 | & 0xFF | 内部压力 |
| 11 | TX[23-24] | 0 | sys_data.Data_1FH | 工作功率 |
| 12 | TX[25-26] | 0 | Switch_Inf.air_on_flag | 风机状态 |
| 13 | TX[27-28] | 0 | Switch_Inf.Water_Valve_Flag | 水阀 |
| 14 | TX[29-30] | 0 | Switch_Inf.pai_wu_flag | 排污阀状态 |
| 15 | TX[31-32] | 0 | Switch_Inf.LianXu_PaiWu_flag | 连续排污状态 |
| 16 | TX[33-34] | 0 | 0 | 未使用 |
| 17 | TX[35-36] | **TimeDianHuo_Flag** | **Paiwu_Flag** | 点火/排污标志 |
| 18 | TX[37-38] | **Inputs_State >> 8** | **& 0xFF** | 输入状态 |

### 新驱动 (modbus_uart4.c:53-137)

| 寄存器 | 索引 | 高字节 | 低字节 | 说明 |
|--------|------|--------|--------|------|
| 1 | buf[0-1] | 0 | sys_data.Data_10H | 工作状态 ✓ |
| 2 | buf[2-3] | Pressure_Value >> 8 | & 0xFF | 压力值 ✓ |
| 3 | buf[4-5] | 0 | 0 | 温度(未使用) ✓ |
| 4 | buf[6-7] | 0 | LCD10D.DLCD.Water_State | 水位状态 ✓ |
| 5 | buf[8-9] | 0 | sys_data.Data_12H | 异常状态 ✓ |
| 6 | buf[10-11] | Protect_WenDu >> 8 | & 0xFF | 内部保护温度 ✓ |
| 7 | buf[12-13] | 0 | sys_flag.Error_Code | 故障代码 ✓ |
| 8 | buf[14-15] | 0 | sys_flag.flame_state | 火焰状态 ✓ |
| 9 | buf[16-17] | Fan_Rpm >> 8 | & 0xFF | 风机转速 ✓ |
| 10 | buf[18-19] | Inside_High_Pressure >> 8 | & 0xFF | 内部压力 ✓ |
| 11 | buf[20-21] | 0 | sys_data.Data_1FH | 工作功率 ✓ |
| 12 | buf[22-23] | 0 | Switch_Inf.air_on_flag | 风机状态 ✓ |
| 13 | buf[24-25] | 0 | Switch_Inf.Water_Valve_Flag | 水阀 ✓ |
| 14 | buf[26-27] | 0 | Switch_Inf.pai_wu_flag | 排污阀状态 ✓ |
| 15 | buf[28-29] | 0 | Switch_Inf.LianXu_PaiWu_flag | 连续排污状态 ✓ |
| 16 | buf[30-31] | 0 | 0 | 未使用 ✓ |
| 17 | buf[32-33] | **0** | **TimeDianHuo_Flag \| Paiwu_Flag** | ⚠️ **差异** |
| 18 | buf[34-35] | **0** | **sys_flag.Inputs_State** | ⚠️ **差异** |

### ⚠️ 读取寄存器差异

1. **寄存器17 (点火/排污标志)**:
   - 原驱动: 高字节=TimeDianHuo_Flag, 低字节=Paiwu_Flag
   - 新驱动: 高字节=0, 低字节=TimeDianHuo_Flag | Paiwu_Flag (OR合并)
   
2. **寄存器18 (输入状态)**:
   - 原驱动: 16位值 (Inputs_State >> 8, & 0xFF)
   - 新驱动: 仅低字节 (0, sys_flag.Inputs_State)

---

## 2. 从机写入寄存器 (地址200, 18寄存器) - 0x10功能码

### 原驱动 (usart4.c:351-573)
数据从 RX[7] 开始（RX[0-6] 是ModBus帧头）

| 寄存器 | 索引 | 处理逻辑 |
|--------|------|----------|
| 1 | RX[7-8] | 启停控制: 0/1/3 -> sys_data.Data_10H |
| 2 | RX[9-10] | 故障复位: ==1 -> sys_flag.Error_Code=0 |
| 3 | RX[11-12] | (未处理) |
| 4 | RX[13-14] | **功率设定**: 手动模式->PWM_Adjust; 工作模式->AirPower_Need |
| 5 | RX[15-16] | **继电器控制**: 手动模式下位操作控制风机/水泵/排水阀/燃气阀 |
| 6 | RX[17-18] | 空闲运行: sys_flag.Idle_AirWork_Flag |
| 7 | RX[19-20] | 排污: sys_flag.Paiwu_Flag |
| 8 | RX[21-22] | 目标压力: sys_config_data.zhuan_huan_temperture_value |
| 9 | RX[23-24] | 停机压力: sys_config_data.Auto_stop_pressure |
| 10 | RX[25-26] | 启动压力: sys_config_data.Auto_start_pressure |
| 11 | RX[27-28] | 额定压力: Sys_Admin.DeviceMaxPressureSet (>=80) |
| 12 | RX[29-30] | 点火功率: Sys_Admin.Dian_Huo_Power (30-60) |
| 13 | RX[31-32] | 最大功率: Sys_Admin.Max_Work_Power (30-100) |
| 14 | RX[33-34] | 温度保护值: Sys_Admin.Inside_WenDu_ProtectValue (241-350) |
| 15 | RX[35-36] | 点火存活: sys_flag.Union_DianHuo_Flag |

### 新驱动 (modbus_uart4.c:146-248)
data 是纯数据部分（已剥离ModBus帧头）

| 寄存器 | 索引 | 处理逻辑 | 状态 |
|--------|------|----------|------|
| 1 | data[0-1] | 启停控制 | ✓ |
| 2 | data[2-3] | 故障复位 | ✓ |
| 3 | data[4-5] | (未处理) | ✓ |
| 4 | data[6-7] | **功率设定**: 仅设置 LCD10D.DLCD.Air_Power (30-100) | ⚠️ **缺少状态判断** |
| 5 | data[8-9] | **继电器控制**: 注释"独立处理" | ⚠️ **完全缺失** |
| 6 | data[10-11] | 空闲运行 | ✓ |
| 7 | data[12-13] | 排污 | ✓ |
| 8 | data[14-15] | 目标压力 | ✓ |
| 9 | data[16-17] | 停机压力 | ✓ |
| 10 | data[18-19] | 启动压力 | ✓ |
| 11 | data[20-21] | 额定压力 | ✓ |
| 12 | data[22-23] | 点火功率 | ✓ |
| 13 | data[24-25] | 最大功率 | ✓ |
| 14 | data[26-27] | 温度保护值 | ✓ |
| 15 | data[28-29] | 点火存活 | ✓ |

### ⚠️ 写入寄存器差异

1. **寄存器4 (功率设定)**:
   - 原驱动: 根据 sys_data.Data_10H 状态分别处理
     - 手动模式(==3): 直接调用 PWM_Adjust(power)
     - 工作模式(==2) + 有火焰: 设置 sys_flag.AirPower_Need
   - 新驱动: 仅设置 LCD10D.DLCD.Air_Power，无状态判断

2. **寄存器5 (继电器控制) - 严重缺失**:
   - 原驱动 (手动模式下):
     - Bit0: 风机控制 (Send_Air_Open/Close)
     - Bit1: 水泵/水阀控制 (Feed_Main_Pump_ON/OFF, Second_Water_Valve)
     - Bit2: 排水阀控制 (Pai_Wu_Door_Open/Close)
     - Bit3: 燃气阀控制 (WTS_Gas_One_Open/Close)
   - 新驱动: 完全未实现

---

## 3. 主机轮询写入数据 (uart4GetWriteData)

### 原驱动 (usart4.c:1002-1082) ModBus4RTU_Write0x10Function

| 寄存器 | 索引 | 数据来源 |
|--------|------|----------|
| 1 | TX[7-8] | JiZu[addr].Slave_D.StartFlag |
| 2 | TX[9-10] | JiZu[addr].Slave_D.Error_Reset |
| 3 | TX[11-12] | AUnionD.Devive_Style |
| 4 | TX[13-14] | SlaveG[addr].Out_Power |
| 5 | TX[15-16] | JiZu[addr].Slave_D.Realys_Out (16bit) |
| 6 | TX[17-18] | SlaveG[addr].Idle_AirWork_Flag |
| 7 | TX[19-20] | SlaveG[addr].Paiwu_Flag |
| 8 | TX[21-22] | sys_config_data.zhuan_huan_temperture_value |
| 9 | TX[23-24] | sys_config_data.Auto_stop_pressure |
| 10 | TX[25-26] | sys_config_data.Auto_start_pressure |
| 11 | TX[27-28] | Sys_Admin.DeviceMaxPressureSet |
| 12 | TX[29-30] | JiZu[addr].Slave_D.DianHuo_Value |
| 13 | TX[31-32] | JiZu[addr].Slave_D.Max_Power |
| 14 | TX[33-34] | JiZu[addr].Slave_D.Inside_WenDu_Protect (16bit) |
| 15 | TX[35-36] | sys_flag.DianHuo_Alive |
| 16-18 | TX[37-42] | 0 (预留) |

### 新驱动 (modbus_uart4.c:380-459) uart4GetWriteData

| 寄存器 | 索引 | 数据来源 | 状态 |
|--------|------|----------|------|
| 1 | data[0-1] | JiZu[addr].Slave_D.StartFlag | ✓ |
| 2 | data[2-3] | JiZu[addr].Slave_D.Error_Reset | ✓ |
| 3 | data[4-5] | AUnionD.Devive_Style | ✓ |
| 4 | data[6-7] | SlaveG[addr].Out_Power | ✓ |
| 5 | data[8-9] | JiZu[addr].Slave_D.Realys_Out (16bit) | ✓ |
| 6 | data[10-11] | SlaveG[addr].Idle_AirWork_Flag | ✓ |
| 7 | data[12-13] | SlaveG[addr].Paiwu_Flag | ✓ |
| 8 | data[14-15] | sys_config_data.zhuan_huan_temperture_value | ✓ |
| 9 | data[16-17] | sys_config_data.Auto_stop_pressure | ✓ |
| 10 | data[18-19] | sys_config_data.Auto_start_pressure | ✓ |
| 11 | data[20-21] | Sys_Admin.DeviceMaxPressureSet | ✓ |
| 12 | data[22-23] | JiZu[addr].Slave_D.DianHuo_Value | ✓ |
| 13 | data[24-25] | JiZu[addr].Slave_D.Max_Power | ✓ |
| 14 | data[26-27] | JiZu[addr].Slave_D.Inside_WenDu_Protect (16bit) | ✓ |
| 15 | data[28-29] | sys_flag.DianHuo_Alive | ✓ |
| 16-18 | data[30-35] | 0 (预留) | ✓ |

**主机写入数据: ✓ 完全一致**

---

## 4. 主机接收响应解析 (uart4OnResponse)

### 原驱动 (usart4.c:882-998) Modbus4_UnionRx_DataProcess
数据从 RX[3] 开始（RX[2] 是字节数）

| 数据 | 索引 | 解析目标 |
|------|------|----------|
| 状态 | RX[4] | Device_State (0/1/2/3) |
| 压力 | RX[5-6] | **RX[5]*255 + RX[6]** -> Dpressure/100 |
| 水位 | RX[10] | Water_State |
| 温度 | RX[13-14] | Inside_WenDu (16bit) |
| 故障 | RX[16] | Error_Code |
| 火焰 | RX[18] | Flame |
| 转速 | RX[19-20] | Fan_Rpm (16bit) |
| 功率 | RX[24] | Power |
| 水泵 | RX[28] | Pump_State |
| 点火标志 | RX[35] | TimeDianHuo_Flag |
| 排污标志 | RX[36] | Paiwu_Flag 处理逻辑 |

### 新驱动 (modbus_uart4.c:257-325) uart4OnResponse
data 是纯数据部分

| 数据 | 索引 | 解析目标 | 状态 |
|------|------|----------|------|
| 状态 | data[1] | Device_State | ✓ |
| 压力 | data[2-3] | **(data[2]<<8)\|data[3]** -> Dpressure/100 | ⚠️ **计算差异** |
| 水位 | data[7] | Water_State | ✓ |
| 温度 | data[10-11] | Inside_WenDu | ✓ |
| 故障 | data[13] | Error_Code | ✓ |
| 火焰 | data[15] | Flame | ✓ |
| 转速 | data[16-17] | Fan_Rpm | ✓ |
| 功率 | data[21] | Power | ✓ |
| 水泵 | data[25] | Pump_State | ✓ |
| 点火标志 | data[32] | TimeDianHuo_Flag | ✓ |
| 排污标志 | data[33] | Paiwu_Flag 处理逻辑 | ⚠️ **逻辑差异** |

### ⚠️ 主机响应解析差异

1. **压力值计算**:
   - 原驱动: `RX[5] * 255 + RX[6]` (错误的乘法因子)
   - 新驱动: `(data[2] << 8) | data[3]` (正确的16位组合)
   - **注意**: 这是原驱动的BUG，新驱动修复了它，不需要还原

2. **排污标志处理逻辑**:
   - 原驱动: `if(RX[36]) SlaveG[addr].Paiwu_Flag = 0`
   - 新驱动: `if(data[33] == 3 && SlaveG[addr].Paiwu_Flag != 3) SlaveG[addr].Paiwu_Flag = 0`
   - **差异**: 新驱动增加了条件判断

---

## 总结

### 需要修复的差异

| 位置 | 差异 | 严重程度 | 修复方案 |
|------|------|----------|----------|
| 从机读取寄存器17 | 高低字节分配不同 | 中 | 恢复原格式 |
| 从机读取寄存器18 | 缺少高字节 | 中 | 恢复16位格式 |
| 从机写入寄存器4 | 功率处理逻辑缺失 | 高 | 添加状态判断和PWM调用 |
| 从机写入寄存器5 | 继电器控制完全缺失 | **严重** | 实现完整控制逻辑 |
| 主机响应排污标志 | 逻辑变更 | 低 | 需确认是优化还是错误 |

### 保持的改进

- 压力值计算修复 (原驱动*255是BUG)

