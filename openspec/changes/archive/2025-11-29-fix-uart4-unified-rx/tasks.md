## 1. 诊断

- [ ] 1.1 在 `UART4_IRQHandler` 中添加调试计数，确认 IDLE 中断是否触发
- [ ] 1.2 在 `uartIdleIrqHandler` 中添加调试输出，确认 DMA RX 数据长度
- [ ] 1.3 在 `modbusHasRxData` 中添加调试输出，确认是否检测到数据
- [ ] 1.4 在 `modbusMasterScheduler` 的 `MASTER_WAIT` 分支添加调试，确认状态机流转

## 2. 修复

- [ ] 2.1 根据诊断结果修复 UART4 IDLE 中断处理
- [ ] 2.2 确保 `uartIsRxReady` / `uartGetRxData` 对 UART4 正确工作
- [ ] 2.3 确保 `modbusHasRxData` 正确复制数据到 `ModbusHandle.rxBuf`
- [ ] 2.4 确保主机调度器在收到响应后正确调用 `masterProcessResponse`

## 3. 验证

- [ ] 3.1 启用 `USE_UNIFIED_MODBUS=1`，确认从机1/2/3响应被正确接收
- [ ] 3.2 确认日志显示 `result=resp` 而非 `result=timeout`
- [ ] 3.3 确认联控屏幕显示分机在线

