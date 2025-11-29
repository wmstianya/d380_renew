# Change: 修复统一ModBus驱动UART4接收问题

## Why

在统一ModBus协议层 (`USE_UNIFIED_MODBUS=1`) 启用时，UART4主站发送轮询帧后无法接收从机响应，导致所有从机显示为超时/离线。回退到旧驱动 (`USE_UNIFIED_MODBUS=0`) 后，同样的硬件和连线下从机1/2/3均能正常响应读写请求。

这证明硬件链路正常，问题出在统一驱动的接收路径。

## What Changes

- 排查并修复 `modbusHasRxData()` 检测逻辑
- 检查 DMA RX / IDLE 中断是否正确触发
- 确保接收数据正确从 `UartHandle` 复制到 `ModbusHandle.rxBuf`
- 验证状态机在 `MODBUS_STATE_MASTER_WAIT` 时能正确检测响应

## Impact

- Affected specs: `modbus-unified`, `uart-driver`
- Affected code:
  - `HARDWARE/modbus/modbus_core.c` - `modbusHasRxData()`
  - `HARDWARE/modbus/modbus_master.c` - 主机调度器响应检测
  - `HARDWARE/uart_driver/uart_driver.c` - DMA RX / IDLE 中断处理
  - `USER/stm32f10x_it.c` - UART4 中断服务程序

