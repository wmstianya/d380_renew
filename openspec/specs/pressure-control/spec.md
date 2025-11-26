# Pressure Control Specification

压力控制系统规格说明 - PID压力调节、风机功率控制和压力平衡逻辑。

## Requirements

### Requirement: 压力设定值配置
系统必须支持三个关键压力设定值的配置。

#### Scenario: 压力参数定义
- **WHEN** 系统配置压力参数
- **THEN** 必须设置：
  - zhuan_huan_temperture_value：目标运行压力（默认50 = 0.5MPa）
  - Auto_stop_pressure：自动停炉压力（默认60 = 0.6MPa）
  - Auto_start_pressure：自动启炉压力（默认40 = 0.4MPa）
  - DeviceMaxPressureSet：设备额定压力（默认100 = 1.0MPa）

#### Scenario: 压力参数边界保护
- **WHEN** 读取Flash中的压力配置
- **AND** 目标压力 < 10 OR >= DeviceMaxPressureSet
- **THEN** 目标压力 = 55（默认0.55MPa）

- **WHEN** 停炉压力 >= DeviceMaxPressureSet
- **THEN** 停炉压力 = DeviceMaxPressureSet - 5

### Requirement: 单机压力平衡控制
系统必须使用PID算法调节风机功率，维持目标压力稳定。

#### Scenario: 压力远低于目标（压差 > 0.16MPa）
- **WHEN** Real_Pressure < SetPoint
- **AND** Abs_Value >= 160（压差 >= 0.16MPa）
- **AND** 压力上升速度 < 30（0.001MPa/s × 10）
- **THEN** 快速增加功率
- **AND** Output = Old_Put + P×2 + I + D×2
- **AND** 功率增加加速

#### Scenario: 压力接近目标（压差 0.03-0.16MPa）
- **WHEN** 30 < Abs_Value < 160
- **AND** 压力上升速度 >= 20
- **THEN** 减速增加功率
- **AND** Output = Old_Put - P - D

- **WHEN** 30 < Abs_Value < 160
- **AND** 压力上升速度 < 20
- **THEN** 适中速度增加
- **AND** Output = Old_Put + P×2 或 + P + I

#### Scenario: 压力达到目标范围（压差 <= 0.03MPa）
- **WHEN** Abs_Value <= 30
- **AND** 压力上升速度 >= 10
- **THEN** 降低功率
- **AND** Output = Old_Put - P×2

- **WHEN** Abs_Value <= 30
- **AND** 压力下降趋势 (Down_Flag == 1)
- **THEN** 增加功率防止下降
- **AND** Output = Old_Put + P×3 + I + D

#### Scenario: 压力超过目标
- **WHEN** Real_Pressure > SetPoint
- **AND** 20 < Abs_Value <= 40
- **THEN** 根据趋势快速降功率
- **AND** Output = Old_Put - P×3 到 P×5

### Requirement: 压力变化速率监测
系统必须监测压力变化速率，用于PID参数调整。

#### Scenario: 压力上升速率计算
- **WHEN** 每秒检查一次压力值 (Pressure_1sFlag)
- **AND** 当前压力 > 上次压力
- **THEN** Change_Speed = (当前压力 - 上次压力) / 时间间隔 × 100
- **AND** Up_Flag = 1, Down_Flag = 0

#### Scenario: 压力下降速率计算
- **WHEN** 每秒检查一次压力值
- **AND** 当前压力 < 上次压力
- **THEN** Change_Speed = (上次压力 - 当前压力) / 时间间隔 × 100
- **AND** Up_Flag = 0, Down_Flag = 1

#### Scenario: 压力未变化超时
- **WHEN** 压力值连续5秒未变化
- **THEN** 重置计数器
- **AND** Change_Speed = 0
- **AND** Up_Flag = 1（默认上升趋势）

### Requirement: 功率限制保护
PID输出功率必须在安全范围内。

#### Scenario: 功率下限保护
- **WHEN** PID计算输出 < 最小功率（点火功率）
- **THEN** Output = 最小功率
- **AND** 最小功率通常为30%

#### Scenario: 功率上限保护
- **WHEN** PID计算输出 > 最大功率
- **THEN** Output = Max_Work_Power
- **AND** Max_Work_Power 默认100%，可配置

#### Scenario: 联动时功率限制
- **WHEN** 其他机组在点火 (Union_DianHuo_Flag == 1)
- **AND** 本机在运行
- **THEN** 最大功率限制为80%
- **AND** 为点火机组预留功率

### Requirement: 自动停炉压力控制
达到停炉压力时必须自动停炉。

#### Scenario: 达到停炉压力
- **WHEN** Pressure_Value >= Auto_stop_pressure
- **THEN** 设置异常标志 Data_12H |= Set_Bit_4
- **AND** 记录异常事件
- **AND** 系统转入异常停炉流程

#### Scenario: 达到目标压力后维持
- **WHEN** Pressure_Value >= zhuan_huan_temperture_value
- **THEN** PID进入压力维持模式
- **AND** 根据压力变化微调功率

### Requirement: PID参数配置
系统必须使用可配置的PID参数进行压力调节。

#### Scenario: PID参数设置
- **WHEN** 系统初始化PID控制
- **THEN** 设置PID参数：
  - Proportion (P) = 8
  - Integral (I) = 3
  - Derivative (D) = 10

#### Scenario: PID首次启动
- **WHEN** Pid_First_Start == 0
- **THEN** 初始化 PID.Out_Put = Data_1FH × 100
- **AND** PID.Old_Put = PID.Out_Put
- **AND** Pid_First_Start = 1

## 实现位置

- **单机压力控制**: `System_Pressure_Balance_Function()` (943-1011行)
- **相变机组压力控制**: `XB_System_Pressure_Balance_Function()` (1014-1211行)
- **PID计算函数**: `Solo_Pid_Cal_Function()` (3942-4369行)
- **压力变化监测**: `Speed_Pressure_Function()` (3511-3570行)

## PID控制逻辑

### 压力区间与控制策略

| 压差范围 | 速率要求 | 控制策略 | 说明 |
|---------|---------|---------|------|
| > 0.16MPa | < 30 | 快速加速 | P×2 + I + D×2 |
| > 0.16MPa | >= 30 | 防止超调 | P + 速度补偿 |
| 0.10-0.16MPa | < 20 | 适中加速 | P×2 或 P + I |
| 0.10-0.16MPa | >= 20 | 适中减速 | P - D |
| 0.03-0.10MPa | < 20 | 精细调节 | P + I 或 P |
| 0.03-0.10MPa | >= 20 | 减速 | -P |
| <= 0.03MPa | < 10 | 微调增加 | P |
| <= 0.03MPa | >= 10 | 微调减少 | -P×2 |
| 超过目标 | 任意 | 降功率 | -P×2 到 -P×5 |

### 压力变化速率定义

```
Speed = 10 × 10 × ΔP / Δt
其中：
- ΔP: 压力变化值（单位：0.001MPa）
- Δt: 时间间隔（单位：0.1秒）
- Speed: 速率值（放大100倍）

例如：
Speed = 30 表示 3.0 (0.001MPa/0.1s) = 0.03MPa/s
Speed = 10 表示 1.0 (0.001MPa/0.1s) = 0.01MPa/s
```

## 配置参数

| 参数 | 变量 | 默认值 | 可配置范围 | 说明 |
|------|------|--------|-----------|------|
| 目标压力 | zhuan_huan_temperture_value | 50 (0.5MPa) | 10-100 | 运行压力设定 |
| 停炉压力 | Auto_stop_pressure | 60 (0.6MPa) | 10-DeviceMax | 超过则停炉 |
| 启炉压力 | Auto_start_pressure | 40 (0.4MPa) | 10-100 | 压力低于此值可启动 |
| 额定压力 | DeviceMaxPressureSet | 100 (1.0MPa) | 最大250 | 设备额定压力 |
| 最大功率 | Max_Work_Power | 100% | 20-100% | 风机最大功率 |

