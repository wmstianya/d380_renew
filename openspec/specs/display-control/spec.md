# Display Control Specification

显示控制系统规格说明 - 串口屏通信、界面更新和人机交互。

## Requirements

### Requirement: 串口屏通信协议
系统必须支持迪文串口屏通信协议进行显示控制。

#### Scenario: 写变量命令
- **WHEN** 需要更新显示数据
- **THEN** 发送写变量命令
- **AND** 帧头：0x5A 0xA5
- **AND** 长度：数据长度+3
- **AND** 命令：0x82（写）
- **AND** 地址：2字节
- **AND** 数据：N字节

#### Scenario: 读变量命令
- **WHEN** 需要读取触摸输入
- **THEN** 发送读变量命令
- **AND** 命令：0x83（读）
- **AND** 等待屏幕响应

### Requirement: 实时数据显示
系统必须实时更新关键运行参数的显示。

#### Scenario: 压力显示
- **WHEN** 100ms周期到达
- **THEN** 更新压力显示
- **AND** 地址：VP_PRESSURE
- **AND** 格式：x.xx MPa
- **AND** 精度：0.01MPa

#### Scenario: 温度显示
- **WHEN** 100ms周期到达
- **THEN** 更新温度显示
- **AND** 地址：VP_TEMPERATURE
- **AND** 格式：xxx.x ℃
- **AND** 精度：0.1℃

#### Scenario: 运行状态显示
- **WHEN** 系统状态改变
- **THEN** 更新状态图标/文字
- **AND** 显示当前状态名称
- **AND** 状态指示灯变化

### Requirement: 故障信息显示
系统必须清晰显示故障信息和故障码。

#### Scenario: 故障弹窗
- **WHEN** Error_Code != 0
- **THEN** 弹出故障提示窗口
- **AND** 显示故障编号
- **AND** 显示故障描述
- **AND** 闪烁提示

#### Scenario: 故障码映射
- **WHEN** 显示故障信息
- **THEN** 根据Error_Code查表
- **AND** 显示对应中文描述
- **AND** 例如：Error3 → "燃气压力低"

### Requirement: 参数设置界面
系统必须提供参数设置功能，允许用户修改运行参数。

#### Scenario: 进入设置界面
- **WHEN** 用户点击设置按钮
- **AND** 输入正确密码
- **THEN** 跳转到参数设置页面
- **AND** 显示当前参数值

#### Scenario: 参数修改
- **WHEN** 用户修改参数值
- **THEN** 实时显示新值
- **AND** 点击确认后生效
- **AND** 保存到Flash
- **AND** 下次上电自动加载

#### Scenario: 参数范围限制
- **WHEN** 用户输入参数
- **AND** 值超出允许范围
- **THEN** 自动限制到边界值
- **AND** 提示"超出范围"

### Requirement: 触摸控制响应
系统必须正确响应触摸屏的操作输入。

#### Scenario: 启动按钮
- **WHEN** 用户点击启动按钮
- **AND** 系统在待机状态
- **AND** 安全条件满足
- **THEN** 系统开始启动流程
- **AND** 按钮状态变为"运行中"

#### Scenario: 停止按钮
- **WHEN** 用户点击停止按钮
- **AND** 系统在运行状态
- **THEN** 系统开始停炉流程
- **AND** 执行后吹扫
- **AND** 按钮状态变为"停止"

#### Scenario: 复位按钮
- **WHEN** 用户点击复位按钮
- **AND** 系统在报警状态
- **THEN** 清除故障码
- **AND** 返回待机状态
- **AND** 允许重新启动

### Requirement: 运行记录显示
系统必须显示运行统计信息和历史记录。

#### Scenario: 运行时间显示
- **WHEN** 进入统计页面
- **THEN** 显示：
  - 累计运行时间（小时）
  - 累计点火次数
  - 累计故障次数
  - 本次运行时间

#### Scenario: 故障历史
- **WHEN** 进入故障记录页面
- **THEN** 显示最近20条故障
- **AND** 包含时间和故障码
- **AND** 支持翻页查看

## Implementation

### 显示地址映射（部分）
| 地址 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 0x1000 | 蒸汽压力 | 数值 | ×0.01MPa |
| 0x1002 | 蒸汽温度 | 数值 | ×0.1℃ |
| 0x1004 | 运行状态 | 图标 | 状态码 |
| 0x1006 | 故障代码 | 数值 | 错误号 |
| 0x2000 | 目标压力 | 输入 | ×0.01MPa |
| 0x2002 | 启动按钮 | 按键 | 触发值 |

### 关键函数
- `Condition_Show()` - 状态显示更新
- `Pressure_Values()` - 压力显示更新
- `LCD_Uart_Send()` - 串口屏发送
- `Touch_Process()` - 触摸输入处理

### 通信配置
- UART2：115200, 8N1
- 更新周期：100ms
- 帧超时：50ms

## Version
- Version: 1.0
- Status: Active
- Last Updated: 2025-11-26

