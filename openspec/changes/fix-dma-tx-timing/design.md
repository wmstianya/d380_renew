## Context

STM32 DMA 发送完成中断 (DMA_IT_TC) 与 USART 发送完成中断 (USART_IT_TC) 的时序差异导致通信异常。

### 硬件时序
1. **DMA TC**: 内存→DR 传输完成，但移位寄存器可能仍有数据
2. **USART TXE**: DR 为空，可写入下一字节
3. **USART TC**: 移位寄存器发送完毕，真正空闲

### 问题根源
当前代码在 DMA TC 中断就清除 `txBusy`，但此时最后一个字节仍在移位寄存器中。

## Goals / Non-Goals

**Goals:**
- 确保 `txBusy` 在数据完全发送后才清除
- 支持 RS485 半双工模式的正确时序
- 不改变现有 API 接口

**Non-Goals:**
- 不实现 RS485 DE 引脚控制 (后续单独处理)
- 不改变 DMA 配置

## Decisions

### Decision 1: 两阶段中断处理
- **DMA TC 中断**: 禁用 DMA，启用 USART TC 中断
- **USART TC 中断**: 清除 txBusy，禁用 TC 中断

**Rationale**: USART TC 表示移位寄存器空，是真正的发送完成点。

### Decision 2: 动态启用/禁用 TC 中断
每次发送仅在 DMA TC 时启用 TC 中断，发送完成后立即禁用。

**Rationale**: 避免 TC 中断持续触发，减少中断负载。

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 中断延迟增加 | TC 中断处理很轻量，影响可忽略 |
| 代码复杂度增加 | 清晰的两阶段分离，易于理解 |

## Migration Plan

1. 修改 `uartDmaTxIrqHandler()`: 移除 txBusy 清除，添加 TC 中断使能
2. 新增 `uartTxCompleteHandler()`: 处理 USART TC 中断
3. 修改 `stm32f10x_it.c`: 各 USART 中断添加 TC 处理
4. 编译测试验证

