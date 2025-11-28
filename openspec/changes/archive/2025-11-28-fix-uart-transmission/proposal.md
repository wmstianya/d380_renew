# Change: 修复UART通信并清理调试代码

## Why

UART通信问题（发送全00）已解决，确认为波特率不匹配及可能的AFIO时钟问题。现在需要移除临时的调试代码，恢复生产就绪状态。

## What Changes

- **USER/main.c**: 移除串口1的临时旧驱动初始化和测试代码，恢复使用 `uartDebugInit`（新DMA驱动）。
- **HARDWARE/modbus/modbus_master.c**: 移除 `u1_printf` 调试打印，恢复纯净的ModBus发送逻辑。

## Impact

- Affected specs: uart-driver
- Affected code: `USER/main.c`, `HARDWARE/modbus/modbus_master.c`

## 验证

系统应继续保持正常的ModBus通信，串口1不再输出调试日志，而是作为RTU服务器正常工作。
