# Ignition Control Specification

点火控制系统规格说明 - 管理锅炉的完整点火流程，包括前吹扫、点火、火焰检测和后吹扫。

## Requirements

### Requirement: 点火前准备流程
系统必须在点火前执行完整的安全检查和前吹扫流程。

#### Scenario: 点火前吹扫
- **WHEN** 系统接收到启动命令
- **AND** 安全检查通过
- **THEN** 开启风机
- **AND** 设置风机功率为100%
- **AND** 吹扫时间 = Sys_Admin.First_Blow_Time（默认25秒，可配置10-200秒）
- **AND** 燃气阀保持关闭状态

#### Scenario: 点火前安全检查
- **WHEN** 准备点火时
- **THEN** 系统必须检查：
  - 水位正常 (water_protect == WATER_OK)
  - 燃气压力正常 (gas_low_pressure == GAS_ON)
  - 风压正常 (Air_Door == AIR_OPEN)
  - 无火焰信号 (flame_state == FLAME_OUT)
- **AND** 任一条件不满足则拒绝点火

### Requirement: 点火执行流程
系统必须按照严格的时序执行点火操作，确保安全可靠。

#### Scenario: 正常点火流程
- **WHEN** 前吹扫完成
- **THEN** 执行以下步骤：
  1. 降低风机功率至点火功率（30%，可配置15-60%）
  2. 延迟稳定时间（3秒）
  3. 启动点火器 (Dian_Huo_Start)
  4. 延迟1秒
  5. 开启燃气阀 (WTS_Gas_One_Open)
  6. 维持3.5秒
  7. 开启主燃气阀 (Send_Gas_Open)
  8. 延迟4.8秒等待点火
  9. 检测火焰信号

#### Scenario: 点火成功
- **WHEN** 打开燃气阀后
- **AND** 延迟4.8秒后检测到火焰信号 (flame_state == FLAME_OK)
- **THEN** 关闭点火器
- **AND** 关闭点火燃气阀
- **AND** 进入稳火阶段
- **AND** 稳火时间 = Sys_Admin.Wen_Huo_Time（默认5秒，可配置）
- **AND** 转入正常运行状态

#### Scenario: 点火失败重试
- **WHEN** 打开燃气阀后
- **AND** 延迟4.8秒后未检测到火焰 (flame_state == FLAME_OUT)
- **THEN** 立即关闭燃气阀和点火器
- **AND** 点火计数器加1 (Ignition_Count++)
- **AND** 如果 Ignition_Count < 3（最大点火次数）
  - 执行前吹扫流程（25秒）
  - 重新尝试点火
- **AND** 如果 Ignition_Count >= 3
  - 设置错误代码 Error11_DianHuo_Bad
  - 停止点火流程

### Requirement: 火焰稳定监测
在稳火阶段，系统必须持续监测火焰状态，防止点火后立即熄灭。

#### Scenario: 稳火期间火焰熄灭
- **WHEN** 在稳火阶段（点火成功后5秒内）
- **AND** 检测到火焰熄灭 (flame_state == FLAME_OUT)
- **THEN** 立即关闭燃气阀
- **AND** 点火计数器加1
- **AND** 如果未超过最大重试次数，重新点火
- **AND** 如果超过最大重试次数，报警 Error11_DianHuo_Bad

### Requirement: 点火过程安全监控
点火过程中必须持续检查关键安全参数。

#### Scenario: 点火过程中燃气压力丢失
- **WHEN** 点火流程执行中
- **AND** 检测到燃气压力低 (gas_low_pressure == GAS_OUT)
- **THEN** 立即中止点火
- **AND** 关闭所有燃气阀
- **AND** 设置错误代码 Error3_LowGas
- **AND** 停止点火流程

#### Scenario: 点火过程中缺水
- **WHEN** 点火流程执行中
- **AND** 检测到极低水位 (water_protect == WATER_LOSE)
- **THEN** 立即中止点火
- **AND** 关闭所有燃气阀
- **AND** 设置错误代码 Error5_LowWater
- **AND** 如果水位在中水位以下，强制补水

### Requirement: 风速检测功能（可选）
如果启用风速检测，系统必须在点火前确认风机转速达到要求。

#### Scenario: 风速检测通过
- **WHEN** Fan_Speed_Check 启用
- **AND** 风机功率设置为30%后延迟20秒
- **AND** 风速在1000-6500 RPM范围内
- **THEN** 允许继续点火流程

#### Scenario: 风速检测失败
- **WHEN** Fan_Speed_Check 启用
- **AND** 等待超时（20秒）仍未达到目标转速
- **THEN** 设置错误代码 Error13_AirControlFail
- **AND** 停止点火流程

## 实现位置

- **点火主函数**: `Sys_Ignition_Fun()` (324-762行)
- **点火前检查**: `Ignition_Check_Fun()` (869-891行)
- **点火前准备**: `Before_Ignition_Prepare()` (243-299行)
- **状态切换变量**: `Ignition_Index` (0-10步骤)

## 配置参数

| 参数名称 | 变量 | 默认值 | 可配置范围 | 说明 |
|---------|------|--------|-----------|------|
| 前吹扫时间 | Sys_Admin.First_Blow_Time | 25000ms | 10000-200000ms | 点火前清除残余燃气 |
| 点火功率 | Sys_Admin.Dian_Huo_Power | 30% | 15-60% | 点火时的风机功率 |
| 稳火时间 | Sys_Admin.Wen_Huo_Time | 5000ms | 可配置 | 火焰稳定时间 |
| 最大点火次数 | Max_Ignition_Times | 3次 | 固定 | 超过则报警 |
| 风速检测 | Sys_Admin.Fan_Speed_Check | 1（启用） | 0/1 | 是否检测风速 |
| 目标点火转速 | Sys_Admin.Fan_Fire_Value | 6500 RPM | 可配置 | 点火时的目标转速 |

## 点火流程状态机

```
State 0: 初始化 → 开风机100%
       ↓
State 10: 前吹扫 (First_Blow_Time秒)
       ↓
State 1: 吹扫完成，延迟100ms
       ↓
State 2: 切换点火风速 (99% → 30%)
       ↓
State 3: 风速稳定检查（可选，20秒超时）
       ↓
State 4: 延迟1秒准备
       ↓
State 5: 启动点火器，开点火燃气阀，延迟3.5秒
       ↓
State 6: 关点火阀，开主燃气阀，延迟4.8秒
       ↓
State 7: 检测火焰
       ├─ 有火焰 → State 8: 稳火 → 成功
       └─ 无火焰 → State 9: 重试准备
              ├─ Count < 3 → 返回 State 4
              └─ Count >= 3 → Error11_DianHuo_Bad
```

## 安全设计原则

1. **防爆设计**: 必须执行前吹扫，清除残余燃气
2. **多重检查**: 点火前、点火中、点火后三重安全检查
3. **重试机制**: 最多3次点火尝试，防止燃气泄漏
4. **火焰确认**: 稳火阶段持续监测，防止假点火
5. **超时保护**: 所有延时都有明确的时间限制
