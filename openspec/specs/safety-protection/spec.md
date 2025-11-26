# Safety Protection Specification

安全保护系统规格说明 - 23种故障检测和保护机制。

## Requirements

### Requirement: 燃气压力低保护 (Error3_LowGas)
系统必须持续监测燃气压力，当压力低于安全阈值时立即保护。

#### Scenario: 燃气压力正常运行
- **WHEN** GPIOB Pin 8 (PB8) 检测到低电平
- **THEN** gas_low_pressure = GAS_ON (0)
- **AND** 系统允许点火或继续运行

#### Scenario: 燃气压力低报警
- **WHEN** GPIOB Pin 8 (PB8) 检测到高电平
- **THEN** gas_low_pressure = GAS_OUT (1)
- **AND** 设置 Error_Code = Error3_LowGas
- **AND** 立即关闭所有燃气阀 (Send_Gas_Close)
- **AND** 如果在运行状态，执行后吹扫
- **AND** 转入报警状态

#### Scenario: 燃气压力传感器断线
- **WHEN** PB8引脚线路断开或传感器故障
- **THEN** 由于内部上拉，引脚读取为高电平
- **AND** 触发燃气压力低报警（Fail-Safe设计）

**硬件配置：**
- 引脚：PB8 (GPIOB Pin 8)
- 模式：GPIO_Mode_IPU（内部上拉输入）
- 正常状态：压力开关闭合接地（低电平）
- 故障状态：压力开关断开（高电平）

**检测位置：**
- 待机检查：`Auto_Check_Fun()` (784-790行)
- 点火检查：`Ignition_Check_Fun()` (877-884行)
- 运行检查：`Abnormal_Check_Fun()` (1640-1646行)

### Requirement: 缺水保护 (Error5_LowWater)
系统必须监测水位，防止干烧导致设备损坏。

#### Scenario: 极低水位立即停机
- **WHEN** water_protect == WATER_LOSE
- **AND** 持续检测5秒确认
- **THEN** 如果在待机状态，设置 Error5_LowWater 报警
- **AND** 如果在运行状态，设置异常标志 Data_12H = 6
- **AND** 关闭燃气阀，执行后吹扫

#### Scenario: 缺水后补水恢复
- **WHEN** 极低水位触发保护
- **THEN** 强制启动水泵补水 (Force_Supple_Water_Flag = OK)
- **AND** 等待水位恢复到中水位
- **AND** 清除异常标志

### Requirement: 火焰熄灭保护 (Error12_FlameLose)
系统运行中必须持续监测火焰，熄灭时立即保护。

#### Scenario: 运行中火焰熄灭
- **WHEN** 系统在运行状态 (Sys_Launch_Index == 2)
- **AND** 检测到火焰信号丢失 (flame_state == FLAME_OUT)
- **THEN** 立即关闭燃气阀
- **AND** FlameOut_Count 计数器加1
- **AND** 如果 FlameOut_Count >= 3，设置 Error12_FlameLose
- **AND** 如果 FlameOut_Count < 3，记录异常但尝试恢复

#### Scenario: 火焰恢复计数清零
- **WHEN** 火焰熄灭后重新点火成功
- **AND** 稳定燃烧600秒（10分钟）
- **THEN** FlameOut_Count 清零
- **AND** FlameRecover_Time 清零

**火焰检测防抖：**
- 火焰信号有：直接使用
- 火焰信号无：连续15次（1.5秒）确认后才认为熄灭

### Requirement: 火焰自检保护 (Error7_FlameZiJian)
待机状态下检测到火焰信号，说明火焰探测器故障。

#### Scenario: 待机状态误检火焰
- **WHEN** 系统在待机状态 (IDLE_INDEX == 0)
- **AND** 检测到火焰信号 (flame_state == FLAME_OK)
- **THEN** 设置 Error7_FlameZiJian 报警
- **AND** 拒绝启动，直到传感器修复

### Requirement: 压力保护 (Error1_YakongProtect, Error2_YaBianProtect)
系统必须监测蒸汽压力，防止超压危险。

#### Scenario: 机械压力开关保护
- **WHEN** hpressure_signal == PRESSURE_ERROR
- **AND** 持续检测1秒确认
- **THEN** 设置 Error1_YakongProtect
- **AND** 立即停炉

#### Scenario: 压力变送器超限保护
- **WHEN** Pressure_Value >= (DeviceMaxPressureSet - 3)
- **AND** Pressure_Value <= 250
- **THEN** 设置 Error2_YaBianProtect（压力变送器超限）

#### Scenario: 压力变送器断线
- **WHEN** Pressure_Value >= (DeviceMaxPressureSet - 3)
- **AND** Pressure_Value > 250（异常高值）
- **THEN** 设置 Error4_YaBianLoss（压力变送器故障/未连接）

### Requirement: 热保护 (Error15_RebaoBad)
燃烧器热保护开关动作时必须立即停机。

#### Scenario: 热保护开关动作
- **WHEN** hot_protect == THERMAL_BAD
- **AND** 持续检测1秒确认
- **THEN** 设置 Error15_RebaoBad
- **AND** 立即停炉

### Requirement: 风压保护 (Error9_AirPressureBad)
系统必须检测风压开关状态，确保风机正常工作。

#### Scenario: 待机时风压异常
- **WHEN** 系统在待机状态
- **AND** Air_Door == AIR_CLOSE（风门关闭，高电平报警）
- **THEN** 设置 Error9_AirPressureBad

#### Scenario: 点火时风压检查
- **WHEN** 准备点火时（Ignition_Index == 2）
- **AND** Air_Door == AIR_CLOSE
- **THEN** 设置 Error9_AirPressureBad
- **AND** 停止点火流程

### Requirement: 水位逻辑错误检测 (Error8_WaterLogic)
系统必须检查水位信号的逻辑一致性。

#### Scenario: 高水位但低水位信号丢失
- **WHEN** water_high == WATER_OK
- **AND** (water_mid == WATER_LOSE OR water_protect == WATER_LOSE)
- **THEN** 设置 Error8_WaterLogic（水位探测逻辑错误）

#### Scenario: 中水位但极低水位信号丢失
- **WHEN** water_mid == WATER_OK
- **AND** water_protect == WATER_LOSE
- **THEN** 设置 Error8_WaterLogic

### Requirement: 点火失败保护 (Error11_DianHuo_Bad)
点火重试3次失败后必须报警并停止。

#### Scenario: 点火三次失败
- **WHEN** Ignition_Count >= Max_Ignition_Times (3)
- **THEN** 设置 Error11_DianHuo_Bad
- **AND** 执行后吹扫
- **AND** 转入报警状态
- **AND** 不允许自动重试

### Requirement: 风机控制失灵保护 (Error13_AirControlFail)
启用风速检测时，风速长时间达不到要求应报警。

#### Scenario: 风速超时
- **WHEN** 设置风机功率30%
- **AND** 等待20秒后
- **AND** 风速仍不在1000-6500 RPM范围
- **THEN** 设置 Error13_AirControlFail

## 故障优先级

系统按以下优先级检查和报告故障（Error_Code == 0时才设置新故障）：

1. **最高优先级**：燃气压力低、缺水、火焰熄灭
2. **高优先级**：压力超限、热保护
3. **中优先级**：风压异常、水位逻辑错误
4. **低优先级**：传感器故障、通信超时

## 23种故障代码清单

```c
Error1_YakongProtect       // 1 机械压力保护
Error2_YaBianProtect       // 2 压力变送器超限
Error3_LowGas              // 3 燃气压力低 ⚠️
Error4_YaBianLoss          // 4 压力变送器断线
Error5_LowWater            // 5 缺水位保护
Error6_BentiWenDu_Unconnect// 6 本体温度传感器未连接
Error7_FlameZiJian         // 7 火焰自检故障
Error8_WaterLogic          // 8 水位逻辑错误
Error9_AirPressureBad      // 9 风压异常
Error10_SteamValueBad      // 10 未使用
Error11_DianHuo_Bad        // 11 点火失败
Error12_FlameLose          // 12 运行中火焰熄灭
Error13_AirControlFail     // 13 风机控制异常
Error14_BenTiValueVBad     // 14 本体温度超限
Error15_RebaoBad           // 15 热保护动作
Error16_SmokeValueHigh     // 16 排烟温度超限
Error17_OutWenKong_TxBad   // 17 外部温控通讯超时
Error18_SupplyWater_Error  // 18 补水超时
Error19_SupplyWater_UNtalk // 19 补水泵通讯失败
Error20_XB_HighPressureYabian_Bad  // 20 相变高压侧压变超限
Error21_XB_HighPressureYAKONG_Bad  // 21 相变高压侧遥控超压
Error22_XB_HighPressureWater_Low   // 22 相变高压侧水位低
unused                     // 23 未使用
```

## 实现位置

- **安全检查函数**: `Auto_Check_Fun()` (772-861行)
- **异常检查函数**: `Abnormal_Check_Fun()` (1593-1671行)
- **点火检查函数**: `Ignition_Check_Fun()` (869-891行)
- **待机检查函数**: `Idel_Check_Fun()` (902-935行)
- **错误响应函数**: `Err_Response()` (1688-1739行)
- **待机错误响应**: `IDLE_Err_Response()` (1742-1819行)
