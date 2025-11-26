# System State Machine Specification

系统主状态机规格说明 - 管理锅炉系统的整体运行状态和状态转换逻辑。

## Requirements

### Requirement: 系统状态定义
系统必须支持5种运行状态，每种状态有明确的功能和转换条件。

#### Scenario: 待机状态 (SYS_IDLE = 0)
- **WHEN** 系统初始化完成或停炉后
- **THEN** 系统进入待机状态
- **AND** 关闭所有执行器（燃气阀、点火器）
- **AND** 风机可选择性运行（防止烟气回流）
- **AND** 监控基本输入信号（水位、燃气压力）

#### Scenario: 运行状态 (SYS_WORK = 2)
- **WHEN** 接收到启动命令
- **AND** 所有安全条件满足（水位正常、燃气压力正常）
- **THEN** 系统进入运行状态
- **AND** 执行点火流程或维持燃烧

#### Scenario: 报警状态 (SYS_ALARM = 4)
- **WHEN** 检测到任何故障（Error_Code != 0）
- **THEN** 系统立即进入报警状态
- **AND** 执行安全停炉流程
- **AND** 启动蜂鸣器报警
- **AND** 锁定错误状态直到人工复位

#### Scenario: 手动模式 (SYS_MANUAL = 3)
- **WHEN** 用户选择手动模式
- **THEN** 系统允许手动控制各执行器
- **AND** 保持基本安全检查功能

### Requirement: 状态转换控制
系统状态转换必须遵循严格的安全逻辑，防止非法状态跳转。

#### Scenario: 待机到运行的转换
- **WHEN** 在待机状态收到启动命令
- **AND** 燃气压力正常 (gas_low_pressure == GAS_ON)
- **AND** 极低水位正常 (water_protect == WATER_OK)
- **AND** 无故障报警 (Error_Code == 0)
- **THEN** 允许进入运行状态
- **AND** 开始执行点火前准备流程

#### Scenario: 运行到待机的转换
- **WHEN** 在运行状态收到停止命令
- **OR** 达到停炉压力 (Pressure >= Auto_stop_pressure)
- **OR** 发生异常事件 (Data_12H != 0)
- **THEN** 关闭燃气阀
- **AND** 执行后吹扫流程
- **AND** 转入待机状态

#### Scenario: 任意状态到报警的转换
- **WHEN** 检测到故障 (Error_Code != 0)
- **THEN** 立即转入报警状态
- **AND** 锁定错误代码 (Lock_Error = 1)
- **AND** 执行紧急停机流程
- **AND** 不允许自动恢复（需人工确认）

### Requirement: 主控制循环
系统必须在主循环中持续执行状态检查和控制逻辑。

#### Scenario: 系统主循环执行
- **WHEN** 系统正常运行
- **THEN** 按顺序执行以下任务：
  1. 读取传感器数据 (SpiReadData, ADC_Process)
  2. 更新IO状态 (read_serial_data)
  3. 执行状态机逻辑 (System_All_Control)
  4. 处理串口通信
  5. 更新显示数据
  6. 喂看门狗 (IWDG_Feed)

## 实现位置

- **主控制函数**: `System_All_Control()` (1860-2019行)
- **状态变量**: `Sys_Staus`, `sys_data.Data_10H`
- **状态枚举定义**: `system_control.h` (92-99行)
- **主循环**: `main.c` (138-244行)

## 相关代码

### 状态枚举
```c
enum
{
    SYS_IDLE  = 0,  // 待机
    SYS_ALARM,      // 报警
    SYS_WORK,       // 运行
    SYS_MANUAL,     // 手动
    SYS_CLEAN_MODE  // 清洗模式
};
```

### 状态切换逻辑
```c
// 在 System_All_Control() 中
switch(Sys_Staus)
{
    case 0:  // 待机
        System_Idel_Function();
        break;
    case 2:  // 运行
        Sys_Launch_Function();
        Err_Response();
        break;
    case 3:  // 手动
        // 手动控制逻辑
        break;
    case 4:  // 报警
        // 报警处理
        break;
}
```
