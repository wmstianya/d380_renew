# Change: 启用全DMA发送

## Why

串口通信基础（波特率/时钟/AFIO）已修复。为了实现最高效的通信性能并释放CPU资源，现在移除临时的阻塞发送代码，全面启用DMA发送。

## What Changes

- **HARDWARE/uart_driver/uart_driver.c**: 移除 `uartSendDma` 中的 `uartSendBlocking` 回退代码。

## Impact

- Affected specs: uart-driver
- Affected code: `HARDWARE/uart_driver/uart_driver.c`

## 验证

- 串口1/2/3/4 发送数据应正常。
- CPU占用率应降低（发送期间不阻塞）。

