# Timer Tasks Specification

定时任务规格说明 - 系统定时器配置和周期性任务调度。

## Requirements

### Requirement: 系统时基定时器
系统必须配置精确的时基定时器，为所有定时任务提供时间基准。

#### Scenario: SysTick配置
- **WHEN** 系统初始化
- **THEN** 配置SysTick定时器
- **AND** 中断周期 = 10ms
- **AND** 提供 delay_ms() 延时函数
- **AND** 提供全局时基计数

#### Scenario: TIM3定时器
- **WHEN** 系统需要周期性任务调度
- **THEN** 使用TIM3
- **AND** 中断周期 = 1ms
- **AND** 驱动主循环任务标志

### Requirement: 100ms周期任务
系统必须每100ms执行一次关键监测任务。

#### Scenario: 100ms任务内容
- **WHEN** 100ms定时标志触发
- **THEN** 执行以下任务：
  1. 读取ADC温度值 (Get_ADC_Temperture)
  2. 读取ADC压力值 (Get_ADC_Value)
  3. 更新压力显示 (Pressure_Values)
  4. 更新状态显示 (Condition_Show)
  5. 外部Flash数据存储

#### Scenario: ADC数据采集
- **WHEN** 100ms周期到达
- **THEN** 采集ADS1220数据
- **AND** 进行数字滤波
- **AND** 更新全局变量

### Requirement: 500ms周期任务
系统必须每500ms执行一次中频任务。

#### Scenario: 500ms任务内容
- **WHEN** 500ms定时标志触发
- **THEN** 执行以下任务：
  1. 统计运行时间 (running_times_half_Sec)
  2. 检查累计运行时间
  3. 燃烧时间统计

#### Scenario: 运行时间累计
- **WHEN** 系统在运行状态
- **AND** 500ms周期触发
- **THEN** Run_Mins_Second += 500
- **AND** 每60秒更新运行分钟数
- **AND** 存储到Flash

### Requirement: 1秒周期任务
系统必须每秒执行一次低频监测任务。

#### Scenario: 1秒任务内容（主机）
- **WHEN** 系统为主机 (Address_Number == 0)
- **AND** 1秒定时标志触发
- **THEN** 执行以下任务：
  1. 多机统计 (Union_1_Sec)
  2. 在线设备检测
  3. 需求台数计算
  4. 功率分配计算

#### Scenario: 1秒任务内容（从机）
- **WHEN** 系统为从机 (Address_Number != 0)
- **AND** 1秒定时标志触发
- **THEN** 执行以下任务：
  1. 检查主机通信状态
  2. 更新显示信息
  3. 看门狗喂狗

### Requirement: 看门狗定时器
系统必须配置独立看门狗，防止程序跑飞。

#### Scenario: IWDG配置
- **WHEN** 系统初始化
- **THEN** 配置IWDG
- **AND** 超时时间 = 1.6秒
- **AND** 预分频 = 64
- **AND** 重载值 = 625

#### Scenario: 喂狗操作
- **WHEN** 主循环正常运行
- **AND** 每次循环
- **THEN** 调用 IWDG_ReloadCounter()
- **AND** 复位看门狗计数器

#### Scenario: 看门狗复位
- **WHEN** 程序卡死超过1.6秒
- **THEN** 看门狗触发硬件复位
- **AND** 系统重启
- **AND** 进入安全状态

### Requirement: 蜂鸣器定时器
系统必须配置专用定时器驱动蜂鸣器。

#### Scenario: TIM2蜂鸣器配置
- **WHEN** 系统初始化
- **THEN** 配置TIM2
- **AND** 参数：Period=1000, Prescaler=71
- **AND** 中断频率 ≈ 1000Hz
- **AND** 实际蜂鸣频率 = 500Hz（翻转驱动）

#### Scenario: 蜂鸣器PWM控制
- **WHEN** TIM2中断触发
- **THEN** 翻转蜂鸣器引脚电平
- **AND** GPIO = PD4
- **AND** 产生方波驱动蜂鸣器

## Implementation

### 定时器分配表
| 定时器 | 功能 | 频率 | 说明 |
|--------|------|------|------|
| SysTick | 系统时基 | 100Hz | 10ms周期 |
| TIM2 | 蜂鸣器 | 1kHz | 方波输出 |
| TIM3 | 任务调度 | 1kHz | 1ms周期 |
| IWDG | 看门狗 | - | 1.6s超时 |

### 任务标志
```c
sys_flag.Times_100Ms_Flag   // 100ms任务标志
sys_flag.Times_500Ms_Flag   // 500ms任务标志
sys_flag.Times_1S_Flag      // 1秒任务标志
```

### 关键函数
- `TIM3_Int_Init()` - TIM3初始化
- `TIM2_Int_Init()` - TIM2蜂鸣器初始化
- `IWDG_Init()` - 看门狗初始化
- `SysTick_Init()` - 系统时基初始化

## Version
- Version: 1.0
- Status: Active
- Last Updated: 2025-11-26

