## MODIFIED Requirements

### Requirement: DMA发送完成处理

系统SHALL使用两阶段中断处理DMA发送完成：
1. **DMA TC中断**: 禁用DMA通道，启用USART TC中断
2. **USART TC中断**: 清除txBusy标志，禁用TC中断

系统SHALL在USART移位寄存器发送完毕后才清除txBusy标志。

系统SHALL NOT在DMA TC中断中直接清除txBusy标志。

#### Scenario: DMA发送完成时序
- **WHEN** DMA TC中断触发
- **THEN** 禁用DMA通道
- **AND** 启用USART TC中断
- **AND** txBusy保持为1

#### Scenario: USART发送完成
- **WHEN** USART TC中断触发
- **THEN** 清除txBusy标志为0
- **AND** 禁用USART TC中断
- **AND** 可以安全发起下一次发送

#### Scenario: RS485时序兼容
- **WHEN** RS485半双工模式发送
- **THEN** 在USART TC中断后切换DE方向
- **AND** 确保最后一字节完整发送

## ADDED Requirements

### Requirement: USART发送完成中断处理

系统SHALL提供 `uartTxCompleteHandler()` 函数处理USART TC中断。

该函数SHALL:
- 检查并清除TC中断标志
- 禁用TC中断
- 清除txBusy标志
- 更新发送帧统计

#### Scenario: TC中断处理
- **WHEN** 在USART中断服务函数中检测到TC中断
- **THEN** 调用 `uartTxCompleteHandler()` 处理
- **AND** 完成发送状态清理

