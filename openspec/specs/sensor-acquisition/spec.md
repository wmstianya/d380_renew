# Sensor Acquisition Specification

传感器采集系统规格说明 - ADC采集、信号处理和数据滤波。

## Requirements

### Requirement: ADS1220高精度ADC
系统必须使用ADS1220进行高精度模拟信号采集。

#### Scenario: ADS1220配置
- **WHEN** 系统初始化
- **THEN** 配置ADS1220
- **AND** 分辨率：24位
- **AND** 采样率：20SPS（可配置）
- **AND** 通信接口：SPI
- **AND** 增益：根据通道配置

#### Scenario: 温度采集（PT100/PT1000）
- **WHEN** 采集温度信号
- **THEN** 使用ADS1220通道
- **AND** 配置为RTD模式
- **AND** 温度范围：0-300℃
- **AND** 精度：±0.5℃

#### Scenario: 压力采集（4-20mA/0-5V）
- **WHEN** 采集压力信号
- **THEN** 使用ADS1220通道
- **AND** 压力范围：0-1.6MPa
- **AND** 精度：±0.01MPa

### Requirement: 内部ADC采集
系统必须使用STM32内部ADC进行辅助信号采集。

#### Scenario: ADC配置
- **WHEN** 系统初始化
- **THEN** 配置ADC1
- **AND** 分辨率：12位
- **AND** 采样周期：55.5 cycles
- **AND** 参考电压：3.3V

#### Scenario: 多通道扫描
- **WHEN** ADC采集周期到达
- **THEN** 顺序采集多个通道
- **AND** 使用DMA传输
- **AND** 双缓冲切换

### Requirement: 数据滤波处理
系统必须对采集数据进行滤波，消除噪声干扰。

#### Scenario: 滑动平均滤波
- **WHEN** 收到新的ADC值
- **THEN** 加入滤波缓冲区
- **AND** 计算N点平均值
- **AND** N = 10（可配置）

#### Scenario: 中值滤波
- **WHEN** 需要去除尖峰干扰
- **THEN** 对N个采样值排序
- **AND** 取中间值作为结果
- **AND** 有效去除脉冲噪声

#### Scenario: 一阶低通滤波
- **WHEN** 需要平滑信号
- **THEN** 应用公式：Y(n) = α×X(n) + (1-α)×Y(n-1)
- **AND** α = 0.1-0.3（可配置）
- **AND** 响应速度与平滑度折中

### Requirement: 数字输入采集
系统必须正确采集所有数字输入信号。

#### Scenario: 并行输入读取
- **WHEN** 执行 Parallel_Serial_Get()
- **THEN** 读取所有数字输入
- **AND** 包括：
  - 水位信号（4个）
  - 火焰信号
  - 燃气压力信号
  - 风压信号
  - 过压信号
  - 限位开关

#### Scenario: 消抖处理
- **WHEN** 读取开关量信号
- **THEN** 连续读取3次
- **AND** 3次一致才确认状态改变
- **AND** 防止抖动误判

### Requirement: 压力值换算
系统必须正确将ADC值换算为实际压力值。

#### Scenario: 线性换算
- **WHEN** ADC采集完成
- **THEN** 应用线性公式
- **AND** 压力 = (ADC值 - 零点) × 量程 / (满量程ADC - 零点ADC)
- **AND** 单位：0.01MPa

#### Scenario: 压力校准
- **WHEN** 需要校准压力
- **THEN** 记录零点ADC值
- **AND** 记录满量程ADC值
- **AND** 存储到Flash
- **AND** 下次上电自动加载

### Requirement: 温度值换算
系统必须正确将ADC值换算为实际温度值。

#### Scenario: PT100换算表
- **WHEN** 采集PT100信号
- **THEN** 使用查表法换算
- **AND** 温度表范围：-50℃ ~ 300℃
- **AND** 线性插值计算中间值

#### Scenario: 温度异常检测
- **WHEN** 温度值异常
- **AND** 温度 < -40℃ OR 温度 > 350℃
- **THEN** 判断为传感器故障
- **AND** 显示"---"
- **AND** 报警提示

## Implementation

### 硬件配置
| 信号 | 类型 | 通道 | 范围 |
|------|------|------|------|
| 蒸汽压力 | 4-20mA | ADS1220 CH0 | 0-1.6MPa |
| 蒸汽温度 | PT100 | ADS1220 CH1 | 0-300℃ |
| 给水温度 | PT100 | ADS1220 CH2 | 0-100℃ |
| 排烟温度 | K热电偶 | ADS1220 CH3 | 0-500℃ |

### 关键函数
- `ADS1220Init()` - ADS1220初始化
- `Get_ADC_Value()` - 压力ADC采集
- `Get_ADC_Temperture()` - 温度ADC采集
- `Parallel_Serial_Get()` - 数字输入采集

### 采集周期
- 压力采集：100ms
- 温度采集：100ms
- 数字输入：10ms

## Version
- Version: 1.0
- Status: Active
- Last Updated: 2025-11-26

