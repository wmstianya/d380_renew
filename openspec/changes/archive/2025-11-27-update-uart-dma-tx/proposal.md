# Change: UART2/UART3发送改用DMA非阻塞方式

## Why

当前USART2和USART3的发送函数`Usart_SendStr_length()`使用阻塞方式逐字节发送，每发送一个字节都要等待TXE标志，导致：
1. CPU在发送期间被阻塞，无法处理其他任务
2. 虽然接收已使用DMA+IDLE，但发送仍是瓶颈
3. 在高频通信场景下影响系统实时性

## What Changes

- 将USART2的5处`Usart_SendStr_length(USART2,...)`调用改为`uartSendDma(&uartDisplayHandle,...)`
- 将USART3的4处`Usart_SendStr_length(USART3,...)`调用改为`uartSendDma(&uartSlaveHandle,...)`
- 使用条件编译`USE_NEW_UART_DRIVER`保持向后兼容

## Impact

- Affected specs: uart-driver
- Affected code:
  - `HARDWARE/USART2/usart2.c` (5处修改)
  - `HARDWARE/USART3/usart3.c` (4处修改)
- 风险：DMA发送是非阻塞的，需要确保上一次发送完成后再发起新发送（检查txBusy标志）

