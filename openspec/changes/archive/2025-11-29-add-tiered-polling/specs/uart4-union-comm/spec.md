## ADDED Requirements

### Requirement: 分级轮询机制

系统 SHALL 实现分级轮询机制，包含心跳检测和完整数据轮询两个层级。

#### Scenario: 心跳检测快速判定在线状态
- **WHEN** 主机向从机发送心跳请求（读取寄存器100，1个寄存器）
- **AND** 从机在150ms内响应
- **THEN** 从机被标记为在线
- **AND** 触发完整数据轮询

#### Scenario: 心跳超时判定离线
- **WHEN** 主机向从机发送心跳请求
- **AND** 从机连续5次未在150ms内响应
- **THEN** 从机被标记为离线
- **AND** 停止对该从机的完整数据轮询

#### Scenario: 完整数据轮询仅在线执行
- **WHEN** 从机处于在线状态
- **AND** 距离上次完整轮询已超过1000ms
- **THEN** 主机发送完整读写请求（18寄存器）
- **AND** 更新从机数据

### Requirement: 心跳寄存器配置

系统 SHALL 支持配置心跳检测使用的寄存器地址和数量。

#### Scenario: 默认心跳寄存器
- **WHEN** 系统初始化
- **THEN** 心跳寄存器地址默认为100
- **AND** 心跳寄存器数量默认为1

### Requirement: 异常响应处理

系统 SHALL 正确处理ModBus异常响应，不影响在线状态判定。

#### Scenario: 异常响应保持在线
- **WHEN** 主机收到从机的异常响应（功能码0x83等）
- **THEN** 从机保持在线状态
- **AND** 重置超时计数器
- **AND** 记录异常码用于诊断

### Requirement: 轮询时序配置

系统 SHALL 支持配置心跳间隔和完整轮询间隔。

#### Scenario: 可配置轮询间隔
- **WHEN** 用户修改 `UART4_HEARTBEAT_INTERVAL_MS` 宏
- **THEN** 心跳检测按新间隔执行
- **WHEN** 用户修改 `UART4_FULLPOLL_INTERVAL_MS` 宏
- **THEN** 完整数据轮询按新间隔执行

