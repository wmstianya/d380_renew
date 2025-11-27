# Change: 为USART1和UART4添加DMA+双缓冲支持

## Why

当前USART2和USART3已成功迁移到DMA+IDLE线检测模式，获得了显著的性能提升：
- 帧检测延迟从10-20ms降至≈1ms（10倍提升）
- CPU中断开销降低87%-99%
- 双缓冲消除数据竞争，高频通讯无丢帧

USART1和UART4仍使用逐字节中断方式，为保持架构一致性和性能优化，需要将DMA驱动扩展到这两个串口。

## What Changes

### 1. 新增USART1 DMA驱动
- 添加 `uartDebugHandle` 句柄
- 添加 `uartDebugInit()` 初始化函数
- 配置DMA1_Channel4(TX)和DMA1_Channel5(RX)
- 添加IDLE中断处理

### 2. 新增UART4 DMA驱动
- 添加 `uartUnionHandle` 句柄
- 添加 `uartUnionInit()` 初始化函数
- 配置DMA2_Channel5(TX)和DMA2_Channel3(RX)
- 添加IDLE中断处理

### 3. 更新中断服务程序
- 添加 `DMA1_Channel4_IRQHandler` (USART1 TX完成)
- 添加 `DMA2_Channel5_IRQHandler` (UART4 TX完成)
- 更新 `USART1_IRQHandler` 和 `UART4_IRQHandler` 使用IDLE中断

## Impact

- **Affected specs**: uart-driver
- **Affected code**: 
  - `HARDWARE/uart_driver/uart_driver.c`
  - `HARDWARE/uart_driver/uart_driver.h`
  - `USER/stm32f10x_it.c`
  - `USER/main.c`
  - `SYSTEM/usart/usart.c`
  - `HARDWARE/USART4/usart4.c`

## DMA通道分配

### STM32F103VCT6 DMA资源

| DMA | 通道 | 当前用途 | 新增用途 |
|-----|------|----------|----------|
| DMA1 | Channel1 | ADC | - |
| DMA1 | Channel2 | USART3_TX | - |
| DMA1 | Channel3 | USART3_RX | - |
| DMA1 | Channel4 | 空闲 | **USART1_TX** |
| DMA1 | Channel5 | 空闲 | **USART1_RX** |
| DMA1 | Channel6 | USART2_RX | - |
| DMA1 | Channel7 | USART2_TX | - |
| DMA2 | Channel3 | 空闲 | **UART4_RX** |
| DMA2 | Channel5 | 空闲 | **UART4_TX** |

### 串口用途汇总

| 串口 | 用途 | 引脚 | DMA RX | DMA TX |
|------|------|------|--------|--------|
| USART1 | 调试/RTU服务器 | PA9(TX), PA10(RX) | DMA1_CH5 | DMA1_CH4 |
| USART2 | 显示屏通信 | PA2(TX), PA3(RX) | DMA1_CH6 | DMA1_CH7 |
| USART3 | 从机设备通信 | PB10(TX), PB11(RX) | DMA1_CH3 | DMA1_CH2 |
| UART4 | 联控通信 | PC10(TX), PC11(RX) | DMA2_CH3 | DMA2_CH5 |

## 兼容性

- 保留旧初始化函数 `uart_init()` 和 `uart4_init()` 供回退使用
- 通过宏定义 `USE_NEW_UART_DRIVER` 控制新旧驱动切换
- 新驱动API与USART2/USART3完全一致

