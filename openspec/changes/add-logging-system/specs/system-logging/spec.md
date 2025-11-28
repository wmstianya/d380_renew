# 系统日志能力

## ADDED Requirements

### Requirement: 分级日志输出

系统 MUST 提供分级日志接口，允许按严重程度过滤输出。

#### Scenario: 日志格式
- **GIVEN** 调用 `log_info("SYS", "Booting...")`
- **WHEN** 日志级别允许 INFO
- **THEN** 输出格式应为 `[时间戳] [INFO] [SYS] Booting...`

### Requirement: 统一输出后端

系统日志 MUST 统一汇聚到单一输出通道（当前为 UART1），避免多处争抢硬件资源。

### Requirement: 串口状态监控

系统 MUST 定期通过日志输出关键串口（UART2/UART4）的通信统计数据（RX/TX帧数、错误数）。

