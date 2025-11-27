# UART DMA 调试记录 (2025-11-27)

## 问题描述
USART2 (PA2/TX, PA3/RX) 在回环测试模式下无法接收数据。

## 现象
- `DMA Cnt` 维持初始值 (128)，显示 `Received 0`。
- `IDLE` 中断未触发。
- 使用阻塞发送 `uartSendBlocking` 也无法接收到数据。
- 串口1 (调试口) 输出正常，系统运行正常。

## 已验证点
1. **硬件连接**：PA2-PA3 短接确认无误。
2. **DMA时钟**：`RCC_AHBPeriph_DMA1` 已使能。
3. **GPIO配置**：PA2 (AF_PP), PA3 (IPU/IN_FLOATING 都试过)。
4. **DMA通道**：USART2_RX 映射到 DMA1_Channel6，配置无误。
5. **中断配置**：NVIC 和 USART_IT_IDLE 已开启。

## 怀疑点 (待查)
1. **TIM2 冲突**：`TIM2_Int_Init` 被调用，TIM2 的默认引脚是 PA2/PA3。虽然只用了 TimeBase，但需确认是否干扰了 GPIO 复用。
2. **电源/电气特性**：引脚是否被其他电路拉死？

## 下一步计划
1. 拉取远程代码，对比差异。
2. 尝试禁用 TIM2 初始化，验证是否恢复通信。

