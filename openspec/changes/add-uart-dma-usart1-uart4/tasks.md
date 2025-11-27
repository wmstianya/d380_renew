# Tasks: 为USART1和UART4添加DMA+双缓冲支持

## 1. USART1 DMA驱动实现

- [x] 1.1 在 `uart_driver.h` 中添加 `uartDebugHandle` 声明
- [x] 1.2 在 `uart_driver.h` 中添加 `uartDebugInit()` 函数声明
- [x] 1.3 在 `uart_driver.c` 中实现 `uartDebugInit()` 函数
  - 配置GPIO: PA9(TX), PA10(RX)
  - 配置DMA: DMA1_Channel4(TX), DMA1_Channel5(RX)
  - 配置NVIC和IDLE中断
- [x] 1.4 在 `stm32f10x_it.c` 中更新 `USART1_IRQHandler`
- [x] 1.5 在 `stm32f10x_it.c` 中添加 `DMA1_Channel4_IRQHandler`

## 2. UART4 DMA驱动实现

- [x] 2.1 在 `uart_driver.h` 中添加 `uartUnionHandle` 声明
- [x] 2.2 在 `uart_driver.h` 中添加 `uartUnionInit()` 函数声明
- [x] 2.3 在 `uart_driver.c` 中实现 `uartUnionInit()` 函数
  - 配置GPIO: PC10(TX), PC11(RX)
  - 配置DMA: DMA2_Channel5(TX), DMA2_Channel3(RX)
  - 配置NVIC和IDLE中断
- [x] 2.4 在 `stm32f10x_it.c` 中更新 `UART4_IRQHandler`
- [x] 2.5 在 `stm32f10x_it.c` 中添加 `DMA2_Channel4_5_IRQHandler`

## 3. main.c集成

- [x] 3.1 替换 `uart_init(9600)` 为 `uartDebugInit(9600)`
- [x] 3.2 替换 `uart4_init(9600)` 为 `uartUnionInit(9600)`
- [x] 3.3 更新 `ModBus_Communication()` 使用新驱动API
- [x] 3.4 更新 `ModBus_Uart4_Local_Communication()` 使用新驱动API

## 4. 兼容层更新

- [x] 4.1 在 `uart_compat.c` 中添加USART1兼容函数 (跳过-保持原有uart_init)
- [x] 4.2 在 `uart_compat.c` 中添加UART4兼容函数 (已完成)
- [x] 4.3 更新 `uart_compat.h` 头文件声明 (已完成)

## 5. 文档更新

- [x] 5.1 更新 `uart-driver/spec.md` 添加USART1和UART4配置
- [x] 5.2 更新 `README.md` 使用说明

## 6. BUG修复

- [x] 6.1 修复 `uartUsartConfig()` 中USART1时钟使能错误 (APB1→APB2条件判断)
- [x] 6.2 移除串口2调试输出

## 完成状态

✅ **2025-11-27 完成验证**
- USART2 (屏幕通信 115200) - DMA+IDLE正常
- USART3 (从机设备 9600) - DMA+IDLE正常
- UART4 (联控通信 9600) - DMA+IDLE正常
- USART1 (调试/RTU 9600) - DMA+IDLE正常

## 注意事项

1. **DMA2时钟使能**: UART4使用DMA2，需要额外使能 `RCC_AHBPeriph_DMA2`
2. **NVIC优先级**: 保持与现有串口一致的优先级分配
3. **调试输出**: USART1同时用于调试输出，需保留 `u1_printf()` 的阻塞发送功能
4. **测试**: 逐步迁移，先测试USART1，再测试UART4

