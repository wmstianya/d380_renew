# Water Level Control Specification

水位控制系统规格说明 - 自动补水、水位监测和水位平衡逻辑。

## Requirements

### Requirement: 多水位传感器监测
系统必须监测4个水位传感器信号，实现分级水位控制。

#### Scenario: 水位传感器定义
- **WHEN** 系统正常运行
- **THEN** 持续监测以下水位：
  - water_protect：极低水位保护（PD5, INPUT_PIN_1）
  - water_mid：中水位（PD6, INPUT_PIN_2）
  - water_high：高水位（PD7, INPUT_PIN_3）
  - water_shigh：超高水位（PB3, INPUT_PIN_4）

**信号特点：**
- 所有水位信号都有取反操作（!PortPin_Scan）
- 配置为内部上拉输入（GPIO_Mode_IPU）
- 传感器闭合（接地）= 水位正常 = 1
- 传感器断开（悬空）= 水位缺失 = 0

### Requirement: 自动补水控制
系统必须根据水位状态自动控制水泵和补水阀。

#### Scenario: 正常补水逻辑（运行状态）
- **WHEN** 系统在运行状态
- **AND** (water_mid == WATER_LOSE OR water_protect == WATER_LOSE)
- **THEN** 启动补水泵 (Feed_Main_Pump_ON)
- **AND** 打开补水阀 (Second_Water_Valve_Open)

#### Scenario: 停止补水
- **WHEN** water_high == WATER_OK
- **AND** water_mid == WATER_OK
- **AND** water_protect == WATER_OK（所有水位正常）
- **THEN** 关闭补水泵 (Feed_Main_Pump_OFF)
- **AND** 关闭补水阀 (Second_Water_Valve_Close)

#### Scenario: 强制补水（点火或异常状态）
- **WHEN** Force_Supple_Water_Flag == OK
- **THEN** 无条件启动补水泵和补水阀
- **AND** 忽略常规补水逻辑

### Requirement: 变频补水功能（可选）
如果启用变频补水，系统应根据水位变化调整补水频率。

#### Scenario: 中水位变频补水
- **WHEN** Water_BianPin_Enabled == 1
- **AND** water_mid == WATER_OK
- **AND** water_high == WATER_LOSE
- **THEN** 间歇性启动补水泵
- **AND** 补水5秒，停止5-20秒（自适应调整）
- **AND** 根据水位上升速度调整间隔时间

#### Scenario: 变频时间自适应
- **WHEN** 补水泵停止时间内水位下降到中水位以下
- **THEN** 减少等待时间（Max_Wait_Time - 1）
- **AND** 最小等待时间为5秒

- **WHEN** 补水泵开启后快速到达高水位
- **THEN** 增加等待时间（Max_Wait_Time + 1）
- **AND** 最大等待时间为20秒

### Requirement: 水位逻辑错误检测
系统必须检测水位信号的逻辑一致性，防止传感器故障。

#### Scenario: 高水位逻辑错误
- **WHEN** water_high == WATER_OK（高水位信号有）
- **AND** (water_mid == WATER_LOSE OR water_protect == WATER_LOSE)
- **THEN** 判定为水位逻辑错误
- **AND** 设置 Error8_WaterLogic
- **AND** 停止补水，防止溢出

#### Scenario: 中水位逻辑错误
- **WHEN** water_mid == WATER_OK（中水位信号有）
- **AND** water_protect == WATER_LOSE（极低水位无）
- **THEN** 判定为水位逻辑错误
- **AND** 设置 Error8_WaterLogic

### Requirement: 补水超时保护
补水时间过长可能表示补水系统故障。

#### Scenario: 补水超时报警
- **WHEN** 补水泵持续运行
- **AND** 运行时间 > Sys_Admin.Supply_Max_Time（默认320秒）
- **AND** 水位仍未达到目标
- **THEN** 设置 Error18_SupplyWater_Error
- **AND** 停止补水，防止溢出或浪费

### Requirement: 后吹扫强制补水
后吹扫期间如果水位低于中水位，必须强制补水。

#### Scenario: 后吹扫期间补水
- **WHEN** 系统在后吹扫状态 (last_blow_flag == 1)
- **AND** water_mid == WATER_LOSE
- **THEN** 设置 Force_Supple_Water_Flag = OK
- **AND** 启动补水

#### Scenario: 后吹扫补水完成
- **WHEN** 后吹扫期间补水
- **AND** water_mid == WATER_OK
- **THEN** Force_Supple_Water_Flag = FALSE
- **AND** 停止补水

## 实现位置

- **主补水函数**: `Water_Balance_Function()` (2505-2629行)
- **变频补水函数**: `Water_BianPin_Function()` (3198-3388行)
- **IO获取函数**: `Get_IO_Inf()` (119-233行)
- **水位检测**: `bsp_parallel_serial.c` read_serial_data() (88-115行)

## 配置参数

| 参数 | 变量 | 默认值 | 说明 |
|------|------|--------|------|
| 补水超时 | Supply_Max_Time | 320秒 | 超时报警 |
| 变频补水启用 | Water_BianPin_Enabled | 0（关闭） | 是否启用变频补水 |
| 变频最大功率 | Water_Max_Percent | 45% | 补水泵最大功率 |

## 水位状态编码

```c
// 水位状态字节（Data_15L）
Bit 0: 极低水位 (water_protect)
Bit 1: 中水位 (water_mid)
Bit 2: 高水位 (water_high)
Bit 3-7: 保留
```

