## 1. 修改 DMA 发送完成中断

- [x] 1.1 修改 `uartDmaTxIrqHandler()`: 移除 `txBusy = 0`
- [x] 1.2 在 DMA TC 中断中启用 `USART_IT_TC`

## 2. 新增 USART TC 中断处理

- [x] 2.1 在 `uart_driver.h` 声明 `uartTxCompleteHandler()`
- [x] 2.2 在 `uart_driver.c` 实现 `uartTxCompleteHandler()`
- [x] 2.3 TC 中断中清除 `txBusy`，禁用 TC 中断

## 3. 修改中断服务函数

- [x] 3.1 `USART1_IRQHandler` 添加 TC 中断调用
- [x] 3.2 `USART2_IRQHandler` 添加 TC 中断调用
- [x] 3.3 `USART3_IRQHandler` 添加 TC 中断调用
- [x] 3.4 `UART4_IRQHandler` 添加 TC 中断调用

## 4. 验证

- [x] 4.1 编译无错误
- [ ] 4.2 串口2/4通讯测试正常
- [ ] 4.3 从机响应无异常码

