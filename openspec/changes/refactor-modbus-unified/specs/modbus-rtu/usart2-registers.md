# USART2 寄存器地址表 (屏幕通信)

## 概述

- **通信对象**: 10.1寸触摸屏
- **角色**: ModBus从机
- **从机地址**: 0x01 (联控板) / LCD4013_Address (分机)
- **波特率**: 115200
- **功能码**: 03(读), 10(写)

---

## 功能码 0x03 (读保持寄存器)

### 联控板模式 (从机地址=0x01)

| 地址 | 长度(寄存器) | 数据源 | 含义 |
|------|-------------|--------|------|
| 0x0000 | 48 (96字节) | `UnionLCD.Datas` | 联控板主数据 |
| 0x0063 | 19 (38字节) | `JiZu[1].Datas` | 机组1数据 |
| 0x0077 | 19 (38字节) | `JiZu[2].Datas` | 机组2数据 |
| 0x008B | 19 (38字节) | `JiZu[3].Datas` | 机组3数据 |
| 0x009F | 19 (38字节) | `JiZu[4].Datas` | 机组4数据 |
| 0x00B3 | 19 (38字节) | `JiZu[5].Datas` | 机组5数据 |
| 0x00C7 | 19 (38字节) | `JiZu[6].Datas` | 机组6数据 |
| 0x00DB | 19 (38字节) | `JiZu[7].Datas` | 机组7数据 |
| 0x00EF | 19 (38字节) | `JiZu[8].Datas` | 机组8数据 |
| 0x0103 | 19 (38字节) | `JiZu[9].Datas` | 机组9数据 |
| 0x0117 | 19 (38字节) | `JiZu[10].Datas` | 机组10数据 |

### 分机模式 (从机地址=LCD4013_Address)

| 地址 | 长度(寄存器) | 数据源 | 含义 |
|------|-------------|--------|------|
| 0x0000 | 可变 | `LCD4013X.Datas` | 分机主数据 |

---

## 功能码 0x10 (写多个寄存器)

### 联控板模式写入寄存器

| 地址 | 长度 | 数据类型 | 范围 | 含义 | 变量 |
|------|------|----------|------|------|------|
| 0x0000 | 1 | uint16 | 0/1/3 | 启停控制 | `AUnionD.UnionStartFlag` |
| | | | 0 | 关闭联控 | |
| | | | 1 | 启动联控 | |
| | | | 3 | 手动模式 | |
| 0x0001 | 1 | uint16 | 0-10 | 需用台数 | `UnionLCD.UnionD.Need_Numbers` |
| 0x0005 | 1 | uint16 | 5-100 | PID转换时间 | `UnionLCD.UnionD.PID_Next_Time` |
| 0x0006 | 1 | uint16 | 0-50 | PID P值 | `UnionLCD.UnionD.PID_Pvalue` |
| 0x0007 | 1 | uint16 | 0-10 | PID I值 | `UnionLCD.UnionD.PID_Ivalue` |
| 0x0008 | 1 | uint16 | 0-30 | PID D值 | `UnionLCD.UnionD.PID_Dvalue` |
| 0x0009 | 1 | uint16 | 0-65535 | 16台设备使能标志 | `UnionLCD.UnionD.Union16_Flag` |
| 0x000C | 2 | float32 | - | 目标压力 (MPa) | `UnionLCD.UnionD.Target_Value` |
| 0x000E | 2 | float32 | - | 停止压力 (MPa) | `UnionLCD.UnionD.Stop_Value` |
| 0x0010 | 2 | float32 | - | 启动压力 (MPa) | `UnionLCD.UnionD.Start_Value` |
| 0x0012 | 2 | float32 | ≤2.5 | 额定压力 (MPa) | `UnionLCD.UnionD.Max_Pressure` |
| 0x0014 (20) | 1 | uint16 | - | A1工作时间 | `AUnionD.A1_WorkTime` |
| ... | | | | A2-A10工作时间 | |

### 分机模式写入寄存器

| 地址 | 长度 | 数据类型 | 范围 | 含义 | 变量 |
|------|------|----------|------|------|------|
| 0x0000 | 1 | uint16 | 0/1/3 | 启停控制 | `sys_data.Data_10H` |
| | | | 0 | 关闭 | |
| | | | 1 | 自动启动 | |
| | | | 3 | 手动模式 | |
| 0x0003 | 1 | uint16 | 1 | 故障复位 | `LCD4013X.DLCD.Error_Code` |
| 0x0004 | 1 | uint16 | 0-100 | 手动功率(%) | `LCD4013X.DLCD.Air_Power` |
| 0x000E | 1 | uint16 | 30-60 | 点火功率 | `Sys_Admin.Dian_Huo_Power` |
| 0x000F | 1 | uint16 | 30-100 | 最大工作功率 | `Sys_Admin.Max_Work_Power` |
| 0x0012 | 1 | uint16 | 200-350 | 温度保护值 | `Sys_Admin.Inside_WenDu_ProtectValue` |
| 0x0016 | 1 | uint16 | - | 手动继电器输出 | `LCD4013X.DLCD.Relays_Out` |
| 0x0020 | 1 | uint16 | 1-254 | 设备地址修改 | `Sys_Admin.ModBus_Address` |
| 0x002A | 1 | uint16 | - | 参数A | - |
| 0x002C | 2 | float32 | - | 停止压力 | - |
| 0x002E | 2 | float32 | - | 启动压力 | - |
| 0x0030 | 2 | float32 | - | 额定压力 | - |

---

## 数据结构

### UnionLCD (联控板LCD数据)

```c
typedef struct {
    uint8_t UnionStartFlag;      // 启停标志 0/1/3
    uint8_t Need_Numbers;        // 需用台数
    uint8_t PID_Next_Time;       // PID转换时间
    uint8_t PID_Pvalue;          // PID P值
    uint8_t PID_Ivalue;          // PID I值
    uint8_t PID_Dvalue;          // PID D值
    uint16_t Union16_Flag;       // 16台设备使能标志
    float Target_Value;          // 目标压力
    float Stop_Value;            // 停止压力
    float Start_Value;           // 启动压力
    float Max_Pressure;          // 额定压力
    // ... 其他字段
} UnionLCD_TypeDef;
```

### JiZu (机组数据)

```c
typedef struct {
    uint8_t Device_State;        // 设备状态
    uint8_t StartFlag;           // 启动标志
    uint8_t Realys_Out;          // 继电器输出
    // ... 38字节数据
} JiZu_TypeDef;
```

### LCD4013X (分机数据)

```c
typedef struct {
    uint8_t Address;             // 设备地址
    uint8_t Device_State;        // 设备状态
    uint8_t Error_Code;          // 故障代码
    uint8_t Air_Power;           // 功率
    uint8_t Relays_Out;          // 继电器输出
    uint8_t Start_Close_Cmd;     // 启停命令
    // ... 其他字段
} LCD4013X_TypeDef;
```

---

## 地址计算规则

机组地址 = 0x0063 + (机组号 - 1) × 0x14

| 机组号 | 计算 | 地址 |
|--------|------|------|
| 1 | 0x0063 + 0×0x14 | 0x0063 |
| 2 | 0x0063 + 1×0x14 | 0x0077 |
| 3 | 0x0063 + 2×0x14 | 0x008B |
| 4 | 0x0063 + 3×0x14 | 0x009F |
| 5 | 0x0063 + 4×0x14 | 0x00B3 |
| 6 | 0x0063 + 5×0x14 | 0x00C7 |
| 7 | 0x0063 + 6×0x14 | 0x00DB |
| 8 | 0x0063 + 7×0x14 | 0x00EF |
| 9 | 0x0063 + 8×0x14 | 0x0103 |
| 10 | 0x0063 + 9×0x14 | 0x0117 |

---

## 通信时序

```
屏幕轮询周期: ~2秒

[时序示例]
屏幕 → 01 03 00 00 00 30 (读联控板主数据)
     ← 01 03 60 xx xx ... (返回96字节)

屏幕 → 01 03 00 63 00 13 (读机组1)
     ← 01 03 26 xx xx ... (返回38字节)

屏幕 → 01 03 00 77 00 13 (读机组2)
     ← 01 03 26 xx xx ... (返回38字节)

... 依次轮询机组3-10 ...

屏幕 → 01 10 00 12 00 02 04 xx xx xx xx (写额定压力)
     ← 01 10 00 12 00 02 (确认响应)
```

---

## 待补充

- [ ] UnionLCD完整字段定义
- [ ] LCD4013X完整字段定义  
- [ ] 各数据字段的字节偏移
- [ ] 浮点数编码方式(IEEE754小端序)

