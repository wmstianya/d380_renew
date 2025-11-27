# Change: UART驱动重构 - DMA+双缓冲+IDLE中断

## Why

### 当前问题

1. **大量重复代码**
   - `uart2_init()` 和 `uart3_init()` 代码几乎一模一样（仅GPIO引脚和USART外设不同）
   - 中断处理函数 `USART2_IRQHandler` 和 `USART3_IRQHandler` 完全重复
   - 发送函数 `U2_send_byte()`, 发送字符串等函数重复
   - ModBus协议处理代码分散在各串口文件中

2. **效率问题**
   - 当前使用单字节中断接收（USART_IT_RXNE），每收一个字节触发一次中断
   - 发送使用轮询等待（阻塞CPU），效率低
   - 通过软件超时检测帧结束，不够精确

3. **可维护性差**
   - 5个串口有5套几乎相同的代码
   - 修改一个功能需要改多个文件
   - 容易出现不一致的bug

### 预期收益

- 代码量减少约 60-70%
- CPU中断负载降低约 80%（DMA替代逐字节中断）
- 更精确的帧边界检测（IDLE中断）
- 统一的接口便于维护和扩展

---

## What Changes

### 1. 新建统一UART驱动框架

创建 `HARDWARE/uart_driver/` 目录，包含：

```
HARDWARE/uart_driver/
├── uart_driver.h      # 统一接口定义
├── uart_driver.c      # 统一驱动实现
├── uart_dma.h         # DMA配置
└── uart_dma.c         # DMA初始化和管理
```

### 2. 核心架构设计

#### 2.1 UART配置结构体

```c
typedef struct {
    USART_TypeDef*     usartx;          // USART外设 (USART2/USART3)
    uint32_t           baudRate;        // 波特率
    GPIO_TypeDef*      txPort;          // TX GPIO端口
    uint16_t           txPin;           // TX引脚
    GPIO_TypeDef*      rxPort;          // RX GPIO端口
    uint16_t           rxPin;           // RX引脚
    uint32_t           rccApbPeriph;    // USART时钟
    uint32_t           rccGpioPeriph;   // GPIO时钟
    uint8_t            nvicIrqChannel;  // NVIC中断通道
    uint8_t            nvicPriority;    // 中断优先级
    DMA_Channel_TypeDef* dmaRxChannel;  // DMA接收通道
    DMA_Channel_TypeDef* dmaTxChannel;  // DMA发送通道
    uint32_t           dmaRxFlag;       // DMA接收完成标志
    uint32_t           dmaTxFlag;       // DMA发送完成标志
} UartConfig;
```

#### 2.2 双缓冲数据结构

```c
#define UART_BUFFER_SIZE    256

typedef struct {
    uint8_t  rxBuffer[2][UART_BUFFER_SIZE];  // 双接收缓冲
    uint8_t  txBuffer[UART_BUFFER_SIZE];     // 发送缓冲
    uint8_t  activeRxBuffer;                 // 当前活动接收缓冲索引 (0/1)
    uint16_t rxLength;                       // 接收数据长度
    uint8_t  rxComplete;                     // 接收完成标志
    uint8_t  txBusy;                         // 发送忙标志
} UartBuffer;

typedef struct {
    UartConfig     config;                   // 配置信息
    UartBuffer     buffer;                   // 缓冲区
    void (*rxCallback)(uint8_t*, uint16_t);  // 接收完成回调
} UartHandle;
```

### 3. DMA通道分配

| UART | 功能 | DMA通道 | 说明 |
|------|------|---------|------|
| USART2 | RX | DMA1_Channel6 | 显示屏通信接收 |
| USART2 | TX | DMA1_Channel7 | 显示屏通信发送 |
| USART3 | RX | DMA1_Channel3 | 主从机通信接收 |
| USART3 | TX | DMA1_Channel2 | 主从机通信发送 |

### 4. 工作流程

#### 接收流程（DMA + IDLE中断）

```
                  IDLE中断触发
                      ↓
┌─────────┐    ┌─────────────┐    ┌─────────────┐
│ DMA接收 │───→│ 切换缓冲区  │───→│ 设置完成标志│
│ (后台)  │    │ (交换指针)  │    │ rxComplete=1│
└─────────┘    └─────────────┘    └─────────────┘
                                         ↓
                                  ┌─────────────┐
                                  │ 主循环处理  │
                                  │ (ModBus等) │
                                  └─────────────┘
```

**优点：**
- DMA在后台自动接收，不占用CPU
- IDLE中断准确检测帧结束（3.5字符时间无数据）
- 双缓冲保证数据处理期间不丢失新数据

#### 发送流程（DMA非阻塞）

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ 填充TX缓冲  │───→│ 启动DMA发送 │───→│ 返回(非阻塞)│
└─────────────┘    └─────────────┘    └─────────────┘
                          ↓
                   ┌─────────────┐
                   │ DMA发送完成 │
                   │  中断/标志  │
                   └─────────────┘
```

---

## How (实现步骤)

### Phase 1: 创建统一驱动框架（不影响现有代码）

1. 创建 `HARDWARE/uart_driver/` 目录
2. 实现统一初始化函数 `uartDriverInit()`
3. 实现DMA配置函数 `uartDmaConfig()`
4. 实现双缓冲管理函数

### Phase 2: 迁移USART2（显示屏通信）

1. 创建USART2配置实例
2. 修改 `uart2_init()` 调用新驱动
3. 修改中断处理使用IDLE中断
4. 更新 `Union_ModBus2_Communication()` 使用新缓冲

### Phase 3: 迁移USART3（主从机通信）

1. 创建USART3配置实例
2. 修改 `uart3_init()` 调用新驱动
3. 更新相关通信函数

### Phase 4: 清理和优化

1. 删除重复的旧代码
2. 统一错误处理
3. 更新文档

---

## 代码示例

### 统一初始化函数

```c
/**
 * @brief  初始化UART驱动（DMA+IDLE中断模式）
 * @param  handle: UART句柄指针
 * @return 0:成功, 非0:失败
 */
uint8_t uartDriverInit(UartHandle* handle)
{
    GPIO_InitTypeDef  gpioInit;
    USART_InitTypeDef usartInit;
    NVIC_InitTypeDef  nvicInit;
    DMA_InitTypeDef   dmaInit;
    
    // 1. 使能时钟
    RCC_APB1PeriphClockCmd(handle->config.rccApbPeriph, ENABLE);
    RCC_APB2PeriphClockCmd(handle->config.rccGpioPeriph, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 2. 配置GPIO (TX: 复用推挽, RX: 浮空输入)
    gpioInit.GPIO_Pin   = handle->config.txPin;
    gpioInit.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(handle->config.txPort, &gpioInit);
    
    gpioInit.GPIO_Pin  = handle->config.rxPin;
    gpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(handle->config.rxPort, &gpioInit);
    
    // 3. 配置USART
    USART_DeInit(handle->config.usartx);
    usartInit.USART_BaudRate            = handle->config.baudRate;
    usartInit.USART_WordLength          = USART_WordLength_8b;
    usartInit.USART_StopBits            = USART_StopBits_1;
    usartInit.USART_Parity              = USART_Parity_No;
    usartInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usartInit.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(handle->config.usartx, &usartInit);
    
    // 4. 配置DMA接收
    DMA_DeInit(handle->config.dmaRxChannel);
    dmaInit.DMA_PeripheralBaseAddr = (uint32_t)&handle->config.usartx->DR;
    dmaInit.DMA_MemoryBaseAddr     = (uint32_t)handle->buffer.rxBuffer[0];
    dmaInit.DMA_DIR                = DMA_DIR_PeripheralSRC;
    dmaInit.DMA_BufferSize         = UART_BUFFER_SIZE;
    dmaInit.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dmaInit.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInit.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dmaInit.DMA_Mode               = DMA_Mode_Normal;
    dmaInit.DMA_Priority           = DMA_Priority_High;
    dmaInit.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(handle->config.dmaRxChannel, &dmaInit);
    
    // 5. 配置NVIC (IDLE中断)
    nvicInit.NVIC_IRQChannel                   = handle->config.nvicIrqChannel;
    nvicInit.NVIC_IRQChannelPreemptionPriority = handle->config.nvicPriority;
    nvicInit.NVIC_IRQChannelSubPriority        = 0;
    nvicInit.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvicInit);
    
    // 6. 使能IDLE中断和DMA请求
    USART_ITConfig(handle->config.usartx, USART_IT_IDLE, ENABLE);
    USART_DMACmd(handle->config.usartx, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(handle->config.usartx, USART_DMAReq_Tx, ENABLE);
    
    // 7. 使能DMA和USART
    DMA_Cmd(handle->config.dmaRxChannel, ENABLE);
    USART_Cmd(handle->config.usartx, ENABLE);
    
    return 0;
}
```

### IDLE中断处理（双缓冲切换）

```c
/**
 * @brief  USART IDLE中断处理（通用）
 * @param  handle: UART句柄指针
 */
void uartIdleIrqHandler(UartHandle* handle)
{
    uint16_t rxLen;
    
    if (USART_GetITStatus(handle->config.usartx, USART_IT_IDLE) != RESET)
    {
        // 1. 清除IDLE标志（读SR+DR）
        (void)handle->config.usartx->SR;
        (void)handle->config.usartx->DR;
        
        // 2. 停止DMA
        DMA_Cmd(handle->config.dmaRxChannel, DISABLE);
        
        // 3. 计算接收长度
        rxLen = UART_BUFFER_SIZE - DMA_GetCurrDataCounter(handle->config.dmaRxChannel);
        
        if (rxLen > 0)
        {
            // 4. 保存接收长度和缓冲索引
            handle->buffer.rxLength = rxLen;
            
            // 5. 切换双缓冲
            uint8_t currentBuffer = handle->buffer.activeRxBuffer;
            handle->buffer.activeRxBuffer = 1 - currentBuffer;
            
            // 6. 重新配置DMA到另一个缓冲区
            handle->config.dmaRxChannel->CMAR = 
                (uint32_t)handle->buffer.rxBuffer[handle->buffer.activeRxBuffer];
            handle->config.dmaRxChannel->CNDTR = UART_BUFFER_SIZE;
            
            // 7. 设置接收完成标志
            handle->buffer.rxComplete = 1;
            
            // 8. 调用回调函数（如果已注册）
            if (handle->rxCallback != NULL)
            {
                handle->rxCallback(handle->buffer.rxBuffer[currentBuffer], rxLen);
            }
        }
        
        // 9. 重新使能DMA
        DMA_Cmd(handle->config.dmaRxChannel, ENABLE);
    }
}
```

### DMA非阻塞发送

```c
/**
 * @brief  DMA方式发送数据（非阻塞）
 * @param  handle: UART句柄指针
 * @param  data: 待发送数据指针
 * @param  len: 数据长度
 * @return 0:成功, 1:忙
 */
uint8_t uartSendDma(UartHandle* handle, uint8_t* data, uint16_t len)
{
    if (handle->buffer.txBusy)
        return 1;  // 上次发送未完成
    
    if (len > UART_BUFFER_SIZE)
        len = UART_BUFFER_SIZE;
    
    // 1. 复制数据到发送缓冲
    memcpy(handle->buffer.txBuffer, data, len);
    
    // 2. 设置忙标志
    handle->buffer.txBusy = 1;
    
    // 3. 配置DMA发送
    DMA_Cmd(handle->config.dmaTxChannel, DISABLE);
    handle->config.dmaTxChannel->CMAR  = (uint32_t)handle->buffer.txBuffer;
    handle->config.dmaTxChannel->CNDTR = len;
    DMA_Cmd(handle->config.dmaTxChannel, ENABLE);
    
    return 0;
}
```

---

## 兼容性考虑

### 保留旧接口

为保证平滑过渡，保留现有函数接口作为包装：

```c
// 兼容旧代码的包装函数
void uart2_init(u32 bound)
{
    uart2Handle.config.baudRate = bound;
    uartDriverInit(&uart2Handle);
}

// U2_Inf 映射到新结构
#define U2_Inf  uart2Handle.buffer
```

### 分阶段迁移

1. 先实现新驱动，与旧代码并存
2. 逐个串口迁移测试
3. 确认稳定后删除旧代码

---

## 影响范围

### 修改的文件
- `USER/stm32f10x_it.c` - 中断处理函数
- `HARDWARE/USART2/usart2.c` - USART2初始化和通信
- `HARDWARE/USART3/usart3.c` - USART3初始化和通信

### 新增的文件
- `HARDWARE/uart_driver/uart_driver.h`
- `HARDWARE/uart_driver/uart_driver.c`

### 不变的文件
- `SYSTEM/system_control/system_control.c` - 业务逻辑不变
- 协议处理代码结构不变，仅数据源变化

---

## 测试计划

1. **单元测试**
   - DMA初始化测试
   - 双缓冲切换测试
   - IDLE中断触发测试

2. **集成测试**
   - USART2 与显示屏通信测试
   - USART3 与从机通信测试
   - 高频数据收发压力测试

3. **回归测试**
   - 现有ModBus协议功能验证
   - 多机联控功能验证

---

## 版本信息

- **创建日期**: 2025-11-26
- **状态**: Draft
- **优先级**: High
- **预计工时**: 3-5天

