# Multi-Device Coordination Specification

多机联控系统规格说明 - 实现多台锅炉协同工作，根据负荷需求自动启停机组。

## Requirements

### Requirement: 主从机通信架构
系统必须支持1台主机控制最多10台从机的联控模式。

#### Scenario: 主机识别
- **WHEN** 系统上电初始化
- **AND** 通过拨码开关读取地址
- **AND** Address_Number == 0
- **THEN** 系统识别为主机
- **AND** 负责协调所有从机

#### Scenario: 从机识别
- **WHEN** 系统上电初始化
- **AND** Address_Number != 0（1-10）
- **THEN** 系统识别为从机
- **AND** 接收主机指令运行

### Requirement: 在线设备检测
主机必须持续监测所有从机的在线状态和健康状况。

#### Scenario: 设备在线统计
- **WHEN** 每秒执行一次统计 (Union_1_Sec)
- **THEN** 遍历所有地址（1-10）
- **AND** 统计满足以下条件的机组数量：
  - UnionOn_Flag == 1（启用联控）
  - Alive_Flag == 1（通信正常）
  - Error_Code == 0（无故障）
- **AND** 更新 AliveOK_Numbers（可用机组数量）

#### Scenario: 故障机组排除
- **WHEN** 检测到某从机 Error_Code != 0
- **THEN** Device_ErrorNumbers++
- **AND** 该机组功率清零 (Out_Power = 0)
- **AND** 不参与负荷分配

### Requirement: 需求台数计算
主机必须根据总压力需求动态计算需要运行的机组台数。

#### Scenario: 启动联控
- **WHEN** UnionStartFlag == 1（启动命令）
- **AND** 烟道风阀开启完成
- **AND** Big_Pressure <= Start_Value（压力低于启动值）
- **AND** AliveOK_Numbers >= 1（至少1台可用）
- **THEN** Mode_Index = 1
- **AND** Need_Devices = 1（至少启动1台）

#### Scenario: 全负荷需要增加机组
- **WHEN** 所有运行中机组满负荷运行
- **AND** 满负荷持续时间 >= PID.Next_Time（默认90秒）
- **AND** AllPower_WorkDevices == Already_WorkNumbers
- **AND** AliveOK_Numbers > Already_WorkNumbers
- **THEN** Need_Devices = Already_WorkNumbers + 1
- **AND** Mode_Index++

#### Scenario: 低负荷需要减少机组
- **WHEN** 多台机组低负荷运行
- **AND** 低负荷持续时间 >= PID.Next_Time × 3（270秒）
- **AND** LowPower_WorkDevices >= 2
- **THEN** Need_Devices = Already_WorkNumbers - 1
- **AND** 选择运行时间最长的机组停机

### Requirement: 机组启停控制
主机必须按照优化算法选择启动或停止的机组。

#### Scenario: 选择待机机组启动
- **WHEN** Need_Devices > Already_WorkNumbers
- **THEN** 遍历所有待机设备
- **AND** 选择累计运行时间最短的机组（Min_Address）
- **AND** 发送启动命令 (StartFlag = 1)
- **AND** Command_SendFlag = 3（连续发送3次确保可靠性）

#### Scenario: 选择运行机组停止
- **WHEN** Need_Devices < Already_WorkNumbers
- **THEN** 遍历所有运行设备
- **AND** 选择累计运行时间最长的机组（Max_Address）
- **AND** 发送停止命令 (StartFlag = 0)
- **AND** Command_SendFlag = 3

### Requirement: 机组负荷统计
主机必须统计各机组的负荷状态，用于启停决策。

#### Scenario: 满负荷运行判定
- **WHEN** 机组功率 >= Max_Power（最大功率）
- **AND** 火焰正常 (Flame == 1)
- **AND** 持续时间 >= PID.Next_Time（90秒）
- **THEN** AllPower_WorkDevices++
- **AND** Big_time 计数器增加

#### Scenario: 低负荷运行判定
- **WHEN** 机组功率 <= (DianHuo_Value + 10%)
- **AND** 火焰正常
- **AND** 持续时间 >= PID.Next_Time × 3（270秒）
- **THEN** LowPower_WorkDevices++
- **AND** Small_time 计数器增加

### Requirement: 停炉压力控制
达到停炉压力时，所有机组必须停止。

#### Scenario: 联控停炉
- **WHEN** Big_Pressure >= Stop_Value（停炉压力）
- **THEN** Mode_Index = 0（回到待机模式）
- **AND** Need_Devices = 0
- **AND** 10秒后发送关闭指令给所有机组

### Requirement: 烟道风阀控制
联控启动前必须确保烟道风阀开启。

#### Scenario: 烟道风阀开启流程
- **WHEN** UnionStartFlag == 1
- **AND** YanDao_FengFa_Index == 0
- **THEN** 打开烟道风阀 (ZongKong_YanFa_Open)
- **AND** 延迟10秒等待风阀完全开启
- **AND** YanDao_FengFa_Index = 1

#### Scenario: 待机风机防回流
- **WHEN** 烟道风阀开启完成
- **THEN** 所有待机机组启动风机（30%功率）
- **AND** Idle_AirWork_Flag = OK
- **AND** 防止烟气回流

#### Scenario: 联控停止后关闭风阀
- **WHEN** UnionStartFlag == 0（停止联控）
- **AND** 所有机组风机停止
- **AND** 延迟15秒
- **THEN** 关闭烟道风阀 (ZongKong_YanFa_Close)

### Requirement: 命令重发机制
主机发送的控制命令必须可靠送达从机。

#### Scenario: 命令三次重发
- **WHEN** 需要启动或停止某从机
- **THEN** 设置 Command_SendFlag = 3
- **AND** 在通信循环中连续发送3次
- **AND** 每次间隔约3秒

### Requirement: 负荷平衡时间跟踪
系统必须跟踪每台机组的运行时间，实现均衡使用。

#### Scenario: 运行时间统计
- **WHEN** 机组在运行状态 (Device_State == 2)
- **AND** 每秒更新一次
- **THEN** Work_Time++（累计运行时间）

#### Scenario: 满负荷时间统计
- **WHEN** 功率 >= Max_Power
- **AND** 火焰正常
- **THEN** Big_time++（满负荷运行时间）
- **AND** Small_time = 0

#### Scenario: 低负荷时间统计
- **WHEN** 功率 <= DianHuo_Value + 10%
- **AND** 火焰正常
- **THEN** Small_time++（低负荷运行时间）
- **AND** Big_time = 0

## 实现位置

- **联控主函数**: `D50L_SoloPressure_Union_MuxJiZu_Control_Function()` (4374-4980行)
- **联控配置**: `Union_Check_Config_Data_Function()` (3740-3914行)
- **相变机组压力控制**: `XB_System_Pressure_Balance_Function()` (1014-1211行)

## 配置参数

| 参数 | 变量 | 默认值 | 说明 |
|------|------|--------|------|
| 联控启动标志 | UnionStartFlag | 0 | 0=停止，1=启动 |
| 需求机组数 | Need_Numbers | 可配置 | 最大需求台数 |
| PID调节间隔 | PID_Next_Time | 90秒 | 负荷判定周期 |
| 设备类型 | Device_Style | 0 | 0=常规，1=相变 |
| 最大从机数 | Max_Address | 10 | 最多10台从机 |

## 联控模式状态机

```
Mode 0: 待机模式
  - 所有机组关闭
  - 监测启动条件
  - 压力 <= Start_Value 时启动
       ↓
Mode 1-10: 运行模式（数字表示需要的台数）
  - 统计在线和运行机组
  - 每10秒评估一次负荷
  - 根据负荷调整机组数量
       ↓
  满负荷时间到 → 增加1台
  低负荷时间到 → 减少1台
       ↓
  压力 >= Stop_Value → 回到 Mode 0
```

## 机组选择策略

**启动选择：** 选择累计运行时间最短的待机机组
**停止选择：** 选择累计运行时间最长的运行机组
**目标：** 实现各机组运行时间均衡，延长设备寿命

