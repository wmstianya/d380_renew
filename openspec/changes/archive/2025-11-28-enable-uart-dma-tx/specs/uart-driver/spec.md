# UART驱动增强

## MODIFIED Requirements

### Requirement: DMA发送

UART驱动 MUST 使用 DMA 通道进行数据发送，以实现非阻塞操作。

#### Scenario: DMA发送启动
- **GIVEN** 调用 `uartSendDma` 发送数据
- **WHEN** 发送缓冲区空闲
- **THEN** 数据被复制到 DMA 缓冲区
- **AND** 启动 DMA 传输
- **AND** 函数立即返回 `UART_OK`，不等待传输完成

