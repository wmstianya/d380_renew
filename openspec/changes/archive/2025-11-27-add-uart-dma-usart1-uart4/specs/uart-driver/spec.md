## ADDED Requirements

### Requirement: USART1 DMA驱动支持

系统SHALL为USART1（调试/RTU服务器通信）提供DMA+IDLE线检测驱动。

#### Scenario: USART1硬件配置

- **WHEN** 初始化USART1
- **THEN** 配置参数：
  - 引脚: PA9(TX), PA10(RX)
  - 波特率: 可配置（默认9600）
  - DMA RX: DMA1_Channel5
  - DMA TX: DMA1_Channel4
  - NVIC优先级: 2:0

#### Scenario: USART1 DMA接收

- **WHEN** 串口接收到完整帧
- **AND** 检测到IDLE空闲
- **THEN** 触发IDLE中断
- **AND** 自动停止DMA传输
- **AND** 切换双缓冲区
- **AND** 设置rxReady标志

#### Scenario: USART1初始化调用

- **WHEN** 调用 `uartDebugInit(baudRate)`
- **THEN** 初始化GPIO、USART、DMA、NVIC
- **AND** 使能IDLE中断
- **AND** 启动DMA接收
- **AND** 返回UART_OK

### Requirement: UART4 DMA驱动支持

系统SHALL为UART4（联控通信）提供DMA+IDLE线检测驱动。

#### Scenario: UART4硬件配置

- **WHEN** 初始化UART4
- **THEN** 配置参数：
  - 引脚: PC10(TX), PC11(RX)
  - 波特率: 可配置（默认9600）
  - DMA RX: DMA2_Channel3
  - DMA TX: DMA2_Channel5
  - NVIC优先级: 2:2

#### Scenario: UART4 DMA接收

- **WHEN** 串口接收到完整帧
- **AND** 检测到IDLE空闲
- **THEN** 触发IDLE中断
- **AND** 自动停止DMA传输
- **AND** 切换双缓冲区
- **AND** 设置rxReady标志

#### Scenario: UART4初始化调用

- **WHEN** 调用 `uartUnionInit(baudRate)`
- **THEN** 初始化GPIO、USART、DMA、NVIC
- **AND** 使能DMA2时钟（RCC_AHBPeriph_DMA2）
- **AND** 使能IDLE中断
- **AND** 启动DMA接收
- **AND** 返回UART_OK

### Requirement: 统一驱动句柄

系统SHALL提供统一的驱动句柄访问所有串口。

#### Scenario: 驱动句柄定义

- **WHEN** 使用DMA驱动
- **THEN** 提供以下全局句柄：
  - `uartDebugHandle` - USART1（调试/RTU）
  - `uartDisplayHandle` - USART2（显示屏）
  - `uartSlaveHandle` - USART3（从机设备）
  - `uartUnionHandle` - UART4（联控通信）

#### Scenario: 统一API调用

- **WHEN** 操作任意串口
- **THEN** 使用相同的API：
  - `uartSendDma(&handle, data, len)` - DMA发送
  - `uartIsRxReady(&handle)` - 检查接收就绪
  - `uartGetRxData(&handle, &data, &len)` - 获取接收数据
  - `uartClearRxFlag(&handle)` - 清除接收标志

## MODIFIED Requirements

### Requirement: DMA通道分配表

系统MUST按照STM32F103硬件映射正确分配DMA通道。

#### Scenario: 完整DMA通道分配

- **WHEN** 系统运行
- **THEN** DMA通道分配如下：

| DMA | 通道 | 用途 | 中断 |
|-----|------|------|------|
| DMA1 | Channel1 | ADC | - |
| DMA1 | Channel2 | USART3_TX | DMA1_Channel2_IRQn |
| DMA1 | Channel3 | USART3_RX | - |
| DMA1 | Channel4 | USART1_TX | DMA1_Channel4_IRQn |
| DMA1 | Channel5 | USART1_RX | - |
| DMA1 | Channel6 | USART2_RX | - |
| DMA1 | Channel7 | USART2_TX | DMA1_Channel7_IRQn |
| DMA2 | Channel3 | UART4_RX | - |
| DMA2 | Channel5 | UART4_TX | DMA2_Channel5_IRQn |

#### Scenario: 中断服务程序

- **WHEN** 配置中断
- **THEN** 实现以下中断处理：

```c
// USART1 IDLE中断
void USART1_IRQHandler(void) {
    uartIdleIrqHandler(&uartDebugHandle);
}

// USART1 TX DMA完成
void DMA1_Channel4_IRQHandler(void) {
    uartDmaTxIrqHandler(&uartDebugHandle);
}

// UART4 IDLE中断
void UART4_IRQHandler(void) {
    uartIdleIrqHandler(&uartUnionHandle);
}

// UART4 TX DMA完成
void DMA2_Channel5_IRQHandler(void) {
    uartDmaTxIrqHandler(&uartUnionHandle);
}
```

