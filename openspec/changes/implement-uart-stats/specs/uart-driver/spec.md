# UART驱动增强

## ADDED Requirements

### Requirement: 通信统计

UART驱动 MUST 提供通信统计功能，包括收发帧数及硬件错误计数。

#### Scenario: 统计更新
- **GIVEN** UART驱动已初始化
- **WHEN** 成功发送或接收一帧数据
- **THEN** `stats.txFrames` 或 `stats.rxFrames` 必须增加
- **AND** 如果发生 Overrun (ORE) 或 Frame Error (FE)，对应错误计数器必须增加

