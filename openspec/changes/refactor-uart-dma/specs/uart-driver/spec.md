# UART Driver Specification

统一UART驱动规格说明 - DMA + 双缓冲 + IDLE中断架构

## Requirements

### Requirement: 统一驱动接口
系统必须提供统一的UART驱动接口，支持多个UART端口使用相同的API。

#### Scenario: 驱动初始化
- **WHEN** 调用 `uartDriverInit(&handle)`
- **THEN** 配置GPIO引脚
- **AND** 配置USART外设
- **AND** 配置DMA通道
- **AND** 使能IDLE中断
- **AND** 返回0表示成功

#### Scenario: 配置灵活性
- **WHEN** 创建不同UART的配置
- **THEN** 通过 `UartConfig` 结构体指定参数
- **AND** 支持不同波特率、引脚、DMA通道
- **AND** 同一驱动代码适配多个UART

### Requirement: DMA接收模式
系统必须使用DMA进行串口数据接收，减少CPU中断负担。

#### Scenario: 自动接收
- **WHEN** 串口收到数据
- **THEN** DMA自动将数据搬运到接收缓冲区
- **AND** CPU不参与每字节中断
- **AND** 后台静默接收

#### Scenario: IDLE中断帧结束检测
- **WHEN** 串口线路空闲时间 >= 1字节时间
- **THEN** 触发IDLE中断
- **AND** 表示一帧数据接收完成
- **AND** 精确检测ModBus帧边界

#### Scenario: 接收长度计算
- **WHEN** IDLE中断触发
- **THEN** 读取DMA剩余计数器
- **AND** 计算 rxLength = BUFFER_SIZE - DMA_CNDTR
- **AND** 保存到 `handle.buffer.rxLength`

### Requirement: 双缓冲机制
系统必须使用双缓冲，保证数据处理期间不丢失新数据。

#### Scenario: 缓冲区切换
- **WHEN** IDLE中断表示一帧完成
- **THEN** 当前缓冲区保留给应用层处理
- **AND** DMA切换到另一个缓冲区继续接收
- **AND** activeRxBuffer = 1 - activeRxBuffer

#### Scenario: 并行处理
- **WHEN** 应用层处理 rxBuffer[0] 的数据
- **THEN** DMA可同时向 rxBuffer[1] 接收新数据
- **AND** 互不干扰
- **AND** 避免数据覆盖

#### Scenario: 缓冲区大小
- **WHEN** 定义缓冲区
- **THEN** 每个缓冲区 256 字节
- **AND** 满足ModBus最大帧长度 (256字节)
- **AND** 双缓冲共 512 字节

### Requirement: DMA发送模式
系统必须使用DMA进行串口数据发送，实现非阻塞发送。

#### Scenario: 非阻塞发送
- **WHEN** 调用 `uartSendDma(&handle, data, len)`
- **THEN** 数据复制到发送缓冲区
- **AND** 启动DMA发送
- **AND** 立即返回，不等待发送完成

#### Scenario: 发送忙检测
- **WHEN** 调用发送函数
- **AND** 上次发送未完成 (txBusy == 1)
- **THEN** 返回错误码1
- **AND** 不覆盖发送缓冲区

#### Scenario: 发送完成
- **WHEN** DMA发送完成
- **THEN** 清除 txBusy 标志
- **AND** 允许下次发送

### Requirement: 错误处理
系统必须正确处理串口通信中的各种错误。

#### Scenario: 溢出错误 (ORE)
- **WHEN** 接收数据溢出
- **THEN** 清除ORE标志
- **AND** 读取DR寄存器丢弃数据
- **AND** 不影响后续接收

#### Scenario: 帧错误 (FE)
- **WHEN** 检测到帧错误
- **THEN** 清除FE标志
- **AND** 丢弃错误帧
- **AND** 继续接收

#### Scenario: 奇偶校验错误 (PE)
- **WHEN** 检测到校验错误
- **THEN** 清除PE标志
- **AND** 丢弃错误数据

### Requirement: USART2 配置 (显示屏通信)
USART2必须配置为与串口屏通信。

#### Scenario: USART2 硬件配置
- **WHEN** 初始化USART2
- **THEN** 配置参数：
  - 引脚: PA2(TX), PA3(RX)
  - 波特率: 115200
  - DMA RX: DMA1_Channel6
  - DMA TX: DMA1_Channel7
  - NVIC优先级: 2

#### Scenario: USART2 协议
- **WHEN** 与显示屏通信
- **THEN** 使用ModBus RTU协议
- **AND** 地址: 1 (显示屏) 或 2 (LCD4013)
- **AND** 支持功能码: 03(读), 10(写多寄存器)

### Requirement: USART3 配置 (主从机通信)
USART3必须配置为主从机通信。

#### Scenario: USART3 硬件配置
- **WHEN** 初始化USART3
- **THEN** 配置参数：
  - 引脚: PB10(TX), PB11(RX)
  - 波特率: 115200
  - DMA RX: DMA1_Channel3
  - DMA TX: DMA1_Channel2
  - NVIC优先级: 0 (高优先级)

#### Scenario: USART3 协议
- **WHEN** 与从机或变频器通信
- **THEN** 使用ModBus RTU协议
- **AND** 支持读写操作
- **AND** 轮询各从机地址

## Implementation

### 文件结构
```
HARDWARE/uart_driver/
├── uart_driver.h      # 接口定义和数据结构
├── uart_driver.c      # 驱动实现
└── uart_config.c      # USART2/USART3配置实例
```

### DMA通道映射 (STM32F103)
| 外设 | 方向 | DMA通道 | 中断 |
|------|------|---------|------|
| USART2_RX | 接收 | DMA1_Channel6 | DMA1_Channel6_IRQn |
| USART2_TX | 发送 | DMA1_Channel7 | DMA1_Channel7_IRQn |
| USART3_RX | 接收 | DMA1_Channel3 | DMA1_Channel3_IRQn |
| USART3_TX | 发送 | DMA1_Channel2 | DMA1_Channel2_IRQn |

### 关键函数
```c
// 初始化
uint8_t uartDriverInit(UartHandle* handle);

// 发送
uint8_t uartSendDma(UartHandle* handle, uint8_t* data, uint16_t len);
uint8_t uartSendBlocking(UartHandle* handle, uint8_t* data, uint16_t len);

// 接收处理
void uartIdleIrqHandler(UartHandle* handle);
uint8_t uartGetRxData(UartHandle* handle, uint8_t** data, uint16_t* len);

// 状态查询
uint8_t uartIsTxBusy(UartHandle* handle);
uint8_t uartIsRxComplete(UartHandle* handle);
```

### 中断处理流程
```
USART2_IRQHandler()
    └── uartIdleIrqHandler(&uart2Handle)
            ├── 清除IDLE标志
            ├── 停止DMA
            ├── 计算接收长度
            ├── 切换双缓冲
            ├── 重配置DMA
            ├── 设置rxComplete标志
            └── 重新使能DMA

主循环处理:
if (uart2Handle.buffer.rxComplete) {
    processModbusFrame(uart2Handle.buffer.rxBuffer[prev], len);
    uart2Handle.buffer.rxComplete = 0;
}
```

## Version
- Version: 1.0
- Status: Draft
- Last Updated: 2025-11-26

