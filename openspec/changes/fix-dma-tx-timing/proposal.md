# Change: 修复DMA发送完成时序问题

## Why
当前 DMA 发送完成中断 (`DMA_IT_TC`) 触发时，立即清除 `txBusy` 标志。但此时数据只是从内存搬到了 USART 的 DR 寄存器，**最后一个字节仍在移位寄存器中发送**。

这导致：
1. 主循环可能立即发起下一次发送 → **数据冲突**
2. RS485 模式下 DE 引脚过早切换 → **最后一字节被截断**
3. 从机收到错误数据 (如 `01 83 ...` 异常响应)

## What Changes
- **DMA TC中断**: 仅禁用DMA，启用 USART TC 中断
- **新增 USART TC中断处理**: 等待移位寄存器发送完毕后才清除 `txBusy`
- **修改中断服务函数**: 添加 TC 中断调用

## Impact
- Affected specs: uart-driver
- Affected code: 
  - `HARDWARE/uart_driver/uart_driver.c`
  - `HARDWARE/uart_driver/uart_driver.h`
  - `USER/stm32f10x_it.c`

---

## 问题复盘

### 错误的时序 (当前实现)
```
DMA TC 触发:  [数据全部到DR] → txBusy = 0 (太早!)
                    ↓
            [移位寄存器仍在发送最后1字节]
                    ↓
            主循环发起新发送 → 数据冲突!
```

### 正确的时序 (修复后)
```
DMA TC 触发:  [数据全部到DR] → 启用 USART_IT_TC
                    ↓
            [移位寄存器发送最后1字节]
                    ↓
USART TC 触发: [移位寄存器空] → txBusy = 0 ✅
                    ↓
            主循环可以安全发起新发送
```

### 关键区别
| 中断 | 含义 | 何时触发 |
|------|------|----------|
| DMA TC | 数据全部搬到DR | 最后1字节在移位寄存器 |
| USART TC | 移位寄存器发送完毕 | 真正发送结束 |

