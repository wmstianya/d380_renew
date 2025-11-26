# Fan Control Specification

风机控制系统规格说明 - 变频风机功率调节、启停控制和功率限制逻辑。

## Requirements

### Requirement: 风机功率输出
系统必须通过DAC输出控制变频器，实现0-100%的无级功率调节。

#### Scenario: 风机功率范围
- **WHEN** 风机运行时
- **THEN** 功率范围：0-100%
- **AND** 对应DAC输出：0-4095（12位DAC）
- **AND** 输出电压范围：0-10V
- **AND** 变频器频率范围：0-50Hz

#### Scenario: 功率转换计算
- **WHEN** 设置风机功率 = Power（0-100）
- **THEN** DAC值 = Power × 40.95
- **AND** 输出电压 = Power × 0.1V
- **AND** 变频器频率 = Power × 0.5Hz

### Requirement: 功率限制保护
系统必须限制风机功率在安全范围内运行。

#### Scenario: 最大功率限制
- **WHEN** 计算出的功率 > 100%
- **THEN** 功率 = 100%
- **AND** 防止过载

#### Scenario: 最小功率限制
- **WHEN** 在点火或小火燃烧状态
- **AND** 功率 < Min_Power（默认20%）
- **THEN** 功率 = Min_Power
- **AND** 维持最小燃烧稳定性

#### Scenario: 点火功率设定
- **WHEN** 执行点火流程
- **THEN** 功率 = Sys_Admin.Dian_Huo_Power（默认30%）
- **AND** 可配置范围：15-60%

### Requirement: 风机启停控制
系统必须正确控制风机的启动和停止时序。

#### Scenario: 风机启动
- **WHEN** 系统从待机进入运行状态
- **THEN** 发送启动命令 (Wind_Control_Start)
- **AND** 设置初始功率
- **AND** 延迟等待风机建立转速

#### Scenario: 风机停止
- **WHEN** 系统停炉或报警
- **AND** 后吹扫完成
- **THEN** 功率降至0
- **AND** 发送停止命令 (Wind_Control_Stop)
- **AND** 等待风机完全停止

### Requirement: 前吹扫风机控制
系统在点火前必须以最大功率运行风机进行炉膛吹扫。

#### Scenario: 前吹扫功率
- **WHEN** 执行前吹扫流程
- **THEN** 风机功率 = 100%
- **AND** 吹扫时间 = Sys_Admin.First_Blow_Time（默认25秒）
- **AND** 清除炉膛内残留燃气

#### Scenario: 前吹扫到点火过渡
- **WHEN** 前吹扫时间结束
- **THEN** 功率从100%降至点火功率（30%）
- **AND** 延迟3秒稳定
- **AND** 准备点火

### Requirement: 后吹扫风机控制
系统在停炉后必须进行后吹扫，排出残留烟气。

#### Scenario: 后吹扫功率
- **WHEN** 执行后吹扫流程
- **THEN** 风机功率 = 100%
- **AND** 吹扫时间 = Sys_Admin.Last_Blow_Time（默认40秒）
- **AND** 可配置范围：5-200秒

#### Scenario: 故障后吹扫
- **WHEN** 系统进入报警状态
- **AND** 非风机相关故障
- **THEN** 执行后吹扫
- **AND** 功率 = 100%
- **AND** 排出残留燃气

## Implementation

### 相关函数
- `Wind_Fan_Put()` - 风机功率输出
- `Wind_Control_Start` - 风机启动
- `Wind_Control_Stop` - 风机停止
- `DAC1_Set_Vol()` - DAC电压设置

### 硬件配置
- DAC输出引脚：PA4（DAC Channel 1）
- 输出范围：0-3.3V → 经放大 → 0-10V
- 变频器输入：0-10V模拟量

### 关键参数
```c
Sys_Admin.Dian_Huo_Power    // 点火功率，默认30%
Sys_Admin.First_Blow_Time   // 前吹扫时间，默认25秒
Sys_Admin.Last_Blow_Time    // 后吹扫时间，默认40秒
Min_Power                   // 最小运行功率，默认20%
```

## Version
- Version: 1.0
- Status: Active
- Last Updated: 2025-11-26

