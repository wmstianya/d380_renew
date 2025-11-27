# UART Driver DMA TX Update

## MODIFIED Requirements

### Requirement: DMA发送模式
The system MUST use DMA for UART data transmission to enable non-blocking sends. All USART2 and USART3 send operations MUST use `uartSendDma()` when `USE_NEW_UART_DRIVER` is defined. (系统必须使用DMA进行串口数据发送，实现非阻塞发送。当定义了USE_NEW_UART_DRIVER时，所有USART2和USART3的发送操作必须使用uartSendDma()。)

#### Scenario: 非阻塞发送
- **WHEN** 调用 `uartSendDma(&handle, data, len)`
- **THEN** 数据复制到发送缓冲区
- **AND** 启动DMA发送
- **AND** 立即返回，不等待发送完成
- **AND** CPU可执行其他任务

#### Scenario: 发送忙检测
- **WHEN** 调用发送函数
- **AND** 上次发送未完成 (txBusy == 1)
- **THEN** 返回错误码UART_BUSY
- **AND** 不覆盖发送缓冲区

#### Scenario: 发送完成
- **WHEN** DMA发送完成
- **THEN** 触发DMA传输完成中断
- **AND** 清除 txBusy 标志
- **AND** 允许下次发送

#### Scenario: USART2显示屏通信发送
- **WHEN** 需要向显示屏发送ModBus响应
- **AND** 定义了USE_NEW_UART_DRIVER
- **THEN** 使用 `uartSendDma(&uartDisplayHandle, data, len)`
- **AND** 替代原有的 `Usart_SendStr_length(USART2, data, len)`

#### Scenario: USART3从机通信发送
- **WHEN** 需要向从机发送ModBus数据
- **AND** 定义了USE_NEW_UART_DRIVER
- **THEN** 使用 `uartSendDma(&uartSlaveHandle, data, len)`
- **AND** 替代原有的 `Usart_SendStr_length(USART3, data, len)`

