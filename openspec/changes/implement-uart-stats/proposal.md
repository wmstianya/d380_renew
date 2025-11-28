# Change: 实现UART通信统计

## Why

为了增强系统可维护性和故障诊断能力，需要对UART通信进行硬件级监控，特别是针对屏幕通信(UART2)和联控通信(UART4)的丢包、溢出等异常情况。

## What Changes

- **HARDWARE/uart_driver/uart_driver.h**: 
  - 新增 `UartStats` 结构体
  - 在 `UartHandle` 中添加统计字段
- **HARDWARE/uart_driver/uart_driver.c**: 
  - 在 `uartDriverInit` 中初始化统计
  - 在 `uartIdleIrqHandler` 中统计接收帧、字节及 ORE/FE/NE 错误
  - 在 `uartDmaTxIrqHandler` 中统计发送帧

## Impact

- Affected specs: uart-driver
- Affected code: `uart_driver.h`, `uart_driver.c`

## 验证

- 通过调试器观察 `uartDisplayHandle.stats` 和 `uartUnionHandle.stats`，确认收发计数增加，且无错误计数。

