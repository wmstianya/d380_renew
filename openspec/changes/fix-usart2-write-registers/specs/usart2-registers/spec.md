## MODIFIED Requirements

### Requirement: USART2协议层选择

USART2屏幕通信 **SHALL** 暂时使用旧协议层实现，直到新协议层补全所有写入寄存器支持。

- USART2 **SHALL** 使用 `Union_ModBus2_Communication()` 处理通信
- 其他串口(USART1/USART3/UART4) **SHALL** 继续使用新统一协议层
- 新协议层 `modbusUsart2Scheduler()` **SHALL** 暂不调用

#### Scenario: USART2使用旧代码

- **GIVEN** 系统启动完成
- **WHEN** 屏幕发送ModBus请求
- **THEN** 使用 `Union_ModBus2_Communication()` 处理，返回正确响应

#### Scenario: 其他串口使用新协议层

- **GIVEN** 系统启动完成且 `USE_UNIFIED_MODBUS = 1`
- **WHEN** USART1/USART3/UART4 收到ModBus请求
- **THEN** 使用新统一协议层处理

### Requirement: USART2写入寄存器完整性 (待补全)

USART2新协议层 **SHALL** 支持以下写入寄存器地址（后续版本实现）：

| 地址范围 | 用途 |
|----------|------|
| 30-41 | 系统级控制 |
| 101-116 | 机组1控制 |
| 121-136 | 机组2控制 |
| 141-156 | 机组3控制 |
| 161-176 | 机组4控制 |
| 181-296 | 机组5-10控制 |

#### Scenario: 写入机组功率 (待实现)

- **GIVEN** 屏幕发送写入请求到地址101
- **WHEN** 数据值为50
- **THEN** 设置机组1功率为50%

