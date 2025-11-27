# UART4 寄存器地址表 (联控通信)

## 概述

- **通信对象**: 分机组 / 联控主机
- **角色**: **双角色** (主机 + 从机)
- **波特率**: 9600
- **功能码**: 03(读), 06(写单个), 10(写多个)

---

## 角色说明

```
┌─────────────┐                    ┌─────────────┐
│  联控主机   │◀── UART4(从机) ──▶│   本控制板   │
└─────────────┘                    └──────┬──────┘
                                          │
                                   UART4(主机)
                                          │
                                          ▼
                              ┌───────────────────┐
                              │   分机1~10        │
                              └───────────────────┘
```

| 场景 | UART4角色 | 通信对象 |
|------|-----------|----------|
| 响应上级联控主机查询 | **从机** | 联控主机 |
| 轮询下级分机状态 | **主机** | 分机1~10 |

---

## 一、从机模式 (响应联控主机)

### 功能码 0x03 (读保持寄存器)

#### 地址 100 (0x64) - 读取本机状态

| 字节偏移 | 寄存器号 | 数据源 | 含义 |
|----------|----------|--------|------|
| 3-4 | 1 | `sys_data.Data_10H` | 当前工作状态 (0/2/3) |
| 5-6 | 2 | `Temperature_Data.Pressure_Value` | 压力值 |
| 7-8 | 3 | - | 温度 (未使用) |
| 9-10 | 4 | `LCD10D.DLCD.Water_State` | 水位状态 |
| 11-12 | 5 | `sys_data.Data_12H` | 异常状态 |
| 13-14 | 6 | `sys_flag.Protect_WenDu` | 内部保护温度 |
| 15-16 | 7 | `sys_flag.Error_Code` | 故障代码 |
| 17-18 | 8 | `sys_flag.flame_state` | 火焰状态 |
| 19-20 | 9 | `sys_flag.Fan_Rpm` | 风机转速 |
| 21-22 | 10 | `Temperature_Data.Inside_High_Pressure` | 内部压力 |
| 23-24 | 11 | `sys_data.Data_1FH` | 工作功率 |
| 25-26 | 12 | `Switch_Inf.air_on_flag` | 风机状态 |
| 27-28 | 13 | `Switch_Inf.Water_Valve_Flag` | 水阀 |
| 29-30 | 14 | `Switch_Inf.pai_wu_flag` | 排污阀状态 |
| 31-32 | 15 | `Switch_Inf.LianXu_PaiWu_flag` | 连续排污状态 |
| 33-34 | 16 | - | 未使用 |
| 35-36 | 17 | `sys_flag.TimeDianHuo_Flag` / `sys_flag.Paiwu_Flag` | 点火/排污标志 |
| 37-38 | 18 | `sys_flag.Inputs_State` | 输入状态 |

**响应帧**: 41字节 (地址+功能码+长度+36字节数据+CRC)

### 功能码 0x10 (写多个寄存器)

#### 地址 200 (0xC8) - 控制命令

| 字节偏移 | 寄存器号 | 数据 | 含义 |
|----------|----------|------|------|
| 7-8 | 1 | 0/1/3 | 启停控制 |
| | | 0 | 关闭 |
| | | 1 | 自动启动 |
| | | 3 | 手动模式 |
| 9-10 | 2 | 0/1 | 故障复位 |
| 13-14 | 4 | - | 设备类型 |
| 14 | - | 0-100 | 功率设定 |
| 15-16 | 5 | bit flags | 继电器控制 |
| | | bit0 | 风机 |
| | | bit1 | 水泵 |
| | | bit2 | 排水阀 |
| | | bit3 | 燃气阀 |
| 18 | 6 | 0/1 | 空闲运行标志 |
| 20 | 7 | 0/1 | 排污标志 |
| 22 | 8 | - | 目标压力 |

---

## 二、主机模式 (轮询分机)

### 轮询调度

```c
// 轮询地址: 1 → 2 → 3 → ... → 10 → 1
// 每个分机交替执行: 读取(03) ↔ 写入(10)
// 超时检测: 100ms无响应视为失败，连续20次失败标记离线
```

### 功能码 0x03 - 读取分机状态

**请求**: `ModBus4_RTU_Read03(address, 100, 18)`

| 参数 | 值 | 含义 |
|------|-----|------|
| 目标地址 | 1-10 | 分机地址 |
| 寄存器地址 | 100 | 固定 |
| 寄存器数量 | 18 | 36字节 |

**响应解析** (`Modbus4_UnionRx_DataProcess`):

| 字节偏移 | 存储位置 | 含义 |
|----------|----------|------|
| 4 | `JiZu[addr].Slave_D.Device_State` | 设备状态 |
| 5-6 | `JiZu[addr].Slave_D.Dpressure` | 压力值 |
| 10 | `JiZu[addr].Slave_D.Water_State` | 水位状态 |
| 13-14 | `JiZu[addr].Slave_D.Inside_WenDu` | 内部温度 |
| 16 | `JiZu[addr].Slave_D.Error_Code` | 故障代码 |
| 18 | `JiZu[addr].Slave_D.Flame` | 火焰状态 |
| 19-20 | `JiZu[addr].Slave_D.Fan_Rpm` | 风机转速 |
| 24 | `JiZu[addr].Slave_D.Power` | 工作功率 |
| 28 | `JiZu[addr].Slave_D.Pump_State` | 水泵状态 |
| 35 | `SlaveG[addr].TimeDianHuo_Flag` | 点火标志 |
| 36 | (检查) | 排污完成标志 |

### 功能码 0x10 - 写入分机命令

**请求**: `ModBus4RTU_Write0x10Function(address, 200, 18)`

| 字节偏移 | 寄存器号 | 数据源 | 含义 |
|----------|----------|--------|------|
| 7-8 | 1 | `JiZu[addr].Slave_D.StartFlag` | 启停命令 |
| 9-10 | 2 | `JiZu[addr].Slave_D.Error_Reset` | 故障复位 |
| 11-12 | 3 | `AUnionD.Devive_Style` | 设备类型 |
| 13-14 | 4 | `SlaveG[addr].Out_Power` | 输出功率 |
| 15-16 | 5 | `JiZu[addr].Slave_D.Realys_Out` | 继电器输出 |
| 17-18 | 6 | `SlaveG[addr].Idle_AirWork_Flag` | 空闲运行标志 |
| 19-20 | 7 | `SlaveG[addr].Paiwu_Flag` | 排污命令 |
| 21-22 | 8 | `sys_config_data.zhuan_huan_temperture_value` | 目标压力 |
| 23-24 | 9 | `sys_config_data.Auto_stop_pressure` | 停机压力 |
| 25-26 | 10 | `sys_config_data.Auto_start_pressure` | 启动压力 |
| 27-28 | 11 | `Sys_Admin.DeviceMaxPressureSet` | 额定压力 |
| 29-30 | 12 | `JiZu[addr].Slave_D.DianHuo_Value` | 点火功率 |
| 31-32 | 13 | `JiZu[addr].Slave_D.Max_Power` | 最大功率 |
| 33-34 | 14 | `JiZu[addr].Slave_D.Inside_WenDu_Protect` | 温度保护值 |
| 35-36 | 15 | `sys_flag.DianHuo_Alive` | 点火存活标志 |
| 37-42 | 16-18 | 0 | 预留 |

---

## 三、数据结构

### SlaveG (分机通信状态)

```c
typedef struct {
    uint8_t Alive_Flag;           // 在线标志
    uint8_t Send_Flag;            // 发送计数 (>20视为离线)
    uint8_t Send_Index;           // 发送索引 (读/写切换)
    uint8_t Out_Power;            // 输出功率
    uint8_t Idle_AirWork_Flag;    // 空闲运行标志
    uint8_t Paiwu_Flag;           // 排污标志
    uint8_t TimeDianHuo_Flag;     // 点火标志
    uint16_t Work_Time;           // 工作时间
    // ...
} SLAVE_GG;

SLAVE_GG SlaveG[13];  // 索引1-10对应分机1-10
```

### JiZu (机组数据)

```c
typedef struct {
    uint8_t Device_State;         // 设备状态 (0/1/2/3)
    uint8_t StartFlag;            // 启动标志
    uint8_t Error_Code;           // 故障代码
    uint8_t Error_Reset;          // 故障复位命令
    uint8_t Flame;                // 火焰状态
    uint8_t Power;                // 工作功率
    uint8_t Water_State;          // 水位状态
    uint8_t Pump_State;           // 水泵状态
    uint16_t Fan_Rpm;             // 风机转速
    uint16_t Inside_WenDu;        // 内部温度
    uint16_t Inside_WenDu_Protect;// 温度保护值
    uint16_t Realys_Out;          // 继电器输出
    float Dpressure;              // 压力值
    uint8_t DianHuo_Value;        // 点火功率
    uint8_t Max_Power;            // 最大功率
} JiZu_Slave_TypeDef;

USlave_Struct JiZu[12];  // 索引1-10对应机组1-10
```

---

## 四、通信时序

### 主机轮询时序

```
联控板(主机) → 分机1: 03 64 00 12 (读取地址100, 18个寄存器)
分机1 → 联控板: 03 24 xx xx ... (返回36字节数据)

联控板(主机) → 分机1: 10 C8 00 12 ... (写入地址200, 18个寄存器)
分机1 → 联控板: 10 C8 00 12 (确认响应)

[等待100ms后轮询分机2]
...
```

### 从机响应时序

```
联控主机 → 本机: 03 64 00 12 (读取状态)
本机 → 联控主机: 03 24 xx xx ... (返回36字节)

联控主机 → 本机: 10 C8 00 12 ... (写入命令)
本机 → 联控主机: 10 C8 00 12 (确认响应)
```

---

## 五、离线检测

```c
// 每次发送后 Send_Flag++
// 收到响应后 Send_Flag = 0
// 当 Send_Flag > 20 时标记离线
if (SlaveG[i].Send_Flag > 20) {
    SlaveG[i].Alive_Flag = FALSE;
    JiZu[i].Slave_D.Device_State = 0;
    memset(&JiZu[i].Datas, 0, sizeof(JiZu[i].Datas));
}
```

---

## 待补充

- [ ] 设备状态值定义 (0/1/2/3的具体含义)
- [ ] 故障代码定义表
- [ ] 继电器位定义

