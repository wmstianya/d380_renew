# USART1 寄存器地址表 (RTU服务器)

## 概述

- **通信对象**: 外部RTU客户端 / 调试设备
- **角色**: ModBus **从机**
- **从机地址**: `Sys_Admin.ModBus_Address` (可配置) 或 地址254(特殊)
- **波特率**: 9600
- **功能码**: 03(读), 06(写单个)

---

## 通信架构

```
┌─────────────────┐          ┌─────────────┐
│  外部RTU客户端   │── USART1 ──▶│   本控制板   │
│  (调试/监控)    │◀─────────│   (从机)    │
└─────────────────┘          └─────────────┘
```

---

## 地址匹配规则

```c
// 地址1: 默认响应
// 地址>1: 必须匹配 Sys_Admin.ModBus_Address
// 地址254: 特殊地址(设备地址查询)

if (U1_Inf.RX_Data[0] > 1) {
    Device_ID = Sys_Admin.ModBus_Address;
    if (U1_Inf.RX_Data[0] != Sys_Admin.ModBus_Address)
        return 0;  // 不匹配则忽略
}
```

---

## 功能码 0x03 (读保持寄存器)

### 地址 0 - 联控主数据

**请求**: `[addr] 03 00 00 00 0B [CRC]` (读取11个寄存器)

**响应** (22字节数据):

| 字节偏移 | 寄存器号 | 数据源 | 含义 |
|----------|----------|--------|------|
| 3-4 | 1 | `AUnionD.UnionStartFlag` | 联控启停状态 |
| 5-6 | 2 | `AUnionD.Big_Pressure * 100` | 当前压力值 |
| 7-8 | 3 | `AUnionD.AliveOK_Numbers` | 在线设备数量 |
| 9-10 | 4 | `sys_flag.Device_ErrorNumbers` | 故障设备数量 |
| 11-12 | 5 | `sys_flag.Already_WorkNumbers` | 工作设备数量 |
| 13-14 | 6 | 0 | 总控故障 (未使用) |
| 15-16 | 7 | 0 | 故障复位指令 (未使用) |
| 17-18 | 8 | `AUnionD.Target_Value * 100` | 目标压力 |
| 19-20 | 9 | `AUnionD.Stop_Value * 100` | 停机压力 |
| 21-22 | 10 | `AUnionD.Start_Value * 100` | 启动压力 |
| 23-24 | 11 | 0 | 未使用 |

### 地址 0x0063 - 机组1数据

**请求**: `[addr] 03 00 63 00 ?? [CRC]`

**响应**: `JiZu[1].Datas` 完整数据 (高低字节交换)

### 地址 0x0077 - 机组2数据

**请求**: `[addr] 03 00 77 00 ?? [CRC]`

**响应**: `JiZu[2].Datas` 完整数据

### 机组地址汇总

| 地址 | 机组 | 备注 |
|------|------|------|
| 0x0063 | 机组1 | 已启用 |
| 0x0077 | 机组2 | 已启用 |
| 0x008B | 机组3 | 已注释 |
| 0x009F | 机组4 | 已注释 |
| 0x00B3 | 机组5 | 已注释 |
| 0x00C7 | 机组6 | 已注释 |

**地址计算**: 机组地址 = 0x0063 + (机组号-1) × 0x14

### 特殊地址 1000 (设备地址查询)

**请求**: `FE 03 03 E8 00 01 [CRC]` (地址254, 读地址1000)

**响应**: 返回当前设备的ModBus地址

| 字节 | 值 | 含义 |
|------|-----|------|
| 0 | 0xFE | 地址254 |
| 1 | 0x03 | 功能码 |
| 2 | 0x02 | 数据长度 |
| 3-4 | Address | `Sys_Admin.ModBus_Address` |
| 5-6 | CRC | 校验 |

---

## 功能码 0x06 (写单个寄存器)

| 地址 | 数据类型 | 范围 | 含义 | 变量 |
|------|----------|------|------|------|
| 0x0000 | uint16 | 0/1 | 联控启停 | `AUnionD.UnionStartFlag` |
| 0x0006 | uint16 | - | 故障复位 | `UnionLCD.UnionD.Error_Reset` |
| 0x0007 | uint16 | ≥20 | 目标压力 | `sys_config_data.zhuan_huan_temperture_value` |
| 0x0008 | uint16 | - | 停机压力 | `sys_config_data.Auto_stop_pressure` |
| 0x0009 | uint16 | - | 启动压力 | `sys_config_data.Auto_start_pressure` |

### 写入约束

| 地址 | 约束条件 |
|------|----------|
| 0x0000 | Value ≤ 1 |
| 0x0007 | Value < Auto_stop_pressure 且 Value ≥ 20 |
| 0x0008 | Value < DeviceMaxPressureSet 且 Value > zhuan_huan_temperture_value |
| 0x0009 | Value < Auto_stop_pressure |

---

## 与USART2对比

| 特性 | USART1 | USART2 |
|------|--------|--------|
| 通信对象 | RTU客户端/调试 | 10.1寸屏幕 |
| 波特率 | 9600 | 115200 |
| 功能码 | 03, 06 | 03, 10 |
| 机组数据 | 仅1-2 | 1-10 |
| 数据字节序 | 高低交换 | 正常顺序 |
| 主数据长度 | 22字节 | 96字节 |

---

## 数据结构

### AUnionD (联控数据)

```c
typedef struct {
    uint8_t UnionStartFlag;       // 联控启停状态
    uint8_t AliveOK_Numbers;      // 在线设备数
    float Big_Pressure;           // 当前压力
    float Target_Value;           // 目标压力
    float Stop_Value;             // 停机压力
    float Start_Value;            // 启动压力
    uint8_t Error_Reset;          // 故障复位
    // ...
} AUnionD_TypeDef;
```

---

## 通信时序示例

```
RTU客户端 → 01 03 00 00 00 0B (读取联控主数据)
         ← 01 03 16 xx xx ... (返回22字节)

RTU客户端 → 01 06 00 00 00 01 (启动联控)
         ← 01 06 00 00 00 01 (确认响应)

RTU客户端 → FE 03 03 E8 00 01 (查询设备地址)
         ← FE 03 02 xx xx (返回设备地址)
```

---

## 待补充

- [ ] 完整的机组数据结构定义
- [ ] 高低字节交换的原因说明
- [ ] RTU客户端的具体用途

