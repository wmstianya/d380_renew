# UART驱动增强

## ADDED Requirements

### Requirement: 中断处理独占性

UART驱动的中断处理函数 MUST 独占管理对应的USART/UART外设中断。

#### Scenario: 中断处理流程
- **GIVEN** 发生USART中断（如IDLE或RXNE）
- **WHEN** 进入 `USARTx_IRQHandler`
- **THEN** 必须调用 `uartIdleIrqHandler` 处理DMA/IDLE逻辑
- **AND** 不得包含任何直接操作DR寄存器（`USART_ReceiveData`）的旧代码
- **AND** 不得包含任何直接清除标志位的旧代码（除非新驱动未处理）

### Requirement: 时钟配置完整性

初始化UART驱动时 MUST 开启所有必要的时钟，包括GPIO、USART/UART、DMA和AFIO。

#### Scenario: GPIO复用配置
- **GIVEN** 初始化UART引脚
- **WHEN** 配置TX/RX引脚复用功能
- **THEN** 必须开启对应的GPIO端口时钟
- **AND** 必须开启 `RCC_APB2Periph_AFIO` 时钟以确保复用功能正常

