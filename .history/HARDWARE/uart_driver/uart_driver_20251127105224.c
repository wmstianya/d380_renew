/**
 * @file    uart_driver.c
 * @brief   统一UART驱动实现 - DMA + 双缓冲 + IDLE中断
 * @author  System
 * @date    2025-11-26
 * @version 1.0
 * 
 * @note    实现DMA接收发送、IDLE帧检测、双缓冲切换
 * 
 * 修订历史:
 * - 1.0 (2025-11-26): 初始版本
 */

#include "uart_driver.h"
#include <string.h>

/*============================================================================*/
/*                              全局变量定义                                  */
/*============================================================================*/

/** @brief USART2驱动句柄 (显示屏通信) */
UartHandle uartDisplayHandle = {0};

/** @brief USART3驱动句柄 (主从机通信) */
UartHandle uartSlaveHandle = {0};


/*============================================================================*/
/*                              内部函数声明                                  */
/*============================================================================*/

static void uartGpioConfig(UartHandle* handle);
static void uartUsartConfig(UartHandle* handle);
static void uartDmaRxConfig(UartHandle* handle);
static void uartDmaTxConfig(UartHandle* handle);
static void uartNvicConfig(UartHandle* handle);


/*============================================================================*/
/*                              初始化函数实现                                */
/*============================================================================*/

/**
 * @brief  初始化UART驱动
 * @param  handle: UART句柄指针
 * @retval UART_OK/UART_ERROR
 */
uint8_t uartDriverInit(UartHandle* handle)
{
    if (handle == NULL || handle->config.usartx == NULL)
        return UART_ERROR;
    
    /* 使能DMA1时钟 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    /* 按顺序配置各模块 */
    uartGpioConfig(handle);
    uartUsartConfig(handle);
    uartDmaRxConfig(handle);
    uartDmaTxConfig(handle);
    uartNvicConfig(handle);
    
    /* 初始化缓冲区状态 */
    handle->buffer.activeRxIdx = 0;
    handle->buffer.rxComplete = 0;
    handle->buffer.txBusy = 0;
    handle->initialized = 1;
    
    return UART_OK;
}


/**
 * @brief  配置UART的GPIO引脚
 * @param  handle: UART句柄指针
 * @note   TX:复用推挽输出, RX:浮空输入
 */
static void uartGpioConfig(UartHandle* handle)
{
    GPIO_InitTypeDef gpioInit;
    
    /* 使能GPIO时钟 */
    RCC_APB2PeriphClockCmd(handle->config.rccGpioPeriph, ENABLE);
    
    /* 使能DMA1时钟 (USART2/3都使用DMA1) */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    /* TX引脚: 复用推挽输出 */
    gpioInit.GPIO_Pin   = handle->config.txGpioPin;
    gpioInit.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(handle->config.txGpioPort, &gpioInit);
    
    /* RX引脚: 上拉输入 (比浮空输入更抗干扰) */
    gpioInit.GPIO_Pin  = handle->config.rxGpioPin;
    gpioInit.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(handle->config.rxGpioPort, &gpioInit);
}


/**
 * @brief  配置USART外设
 * @param  handle: UART句柄指针
 * @note   8N1格式，使能IDLE中断和DMA请求
 */
static void uartUsartConfig(UartHandle* handle)
{
    USART_InitTypeDef usartInit;
    
    /* 使能USART时钟 */
    RCC_APB1PeriphClockCmd(handle->config.rccUsartPeriph, ENABLE);
    
    /* 复位USART */
    USART_DeInit(handle->config.usartx);
    
    /* 配置USART参数: 8N1 */
    usartInit.USART_BaudRate            = handle->config.baudRate;
    usartInit.USART_WordLength          = USART_WordLength_8b;
    usartInit.USART_StopBits            = USART_StopBits_1;
    usartInit.USART_Parity              = USART_Parity_No;
    usartInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usartInit.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(handle->config.usartx, &usartInit);
    
    /* 使能IDLE中断 (帧结束检测) */
    USART_ITConfig(handle->config.usartx, USART_IT_IDLE, ENABLE);
    
    /* 使能DMA请求 */
    USART_DMACmd(handle->config.usartx, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(handle->config.usartx, USART_DMAReq_Tx, ENABLE);
    
    /* 使能USART */
    USART_Cmd(handle->config.usartx, ENABLE);
    
    /* 清除可能存在的IDLE标志 (读SR, 读DR) */
    (void)handle->config.usartx->SR;
    (void)handle->config.usartx->DR;
}


/**
 * @brief  配置DMA接收通道
 * @param  handle: UART句柄指针
 * @note   外设->内存，单次模式，使能后立即开始接收
 */
static void uartDmaRxConfig(UartHandle* handle)
{
    DMA_InitTypeDef dmaInit;
    
    DMA_DeInit(handle->config.dmaRxChannel);
    
    dmaInit.DMA_PeripheralBaseAddr = (uint32_t)&handle->config.usartx->DR;
    dmaInit.DMA_MemoryBaseAddr     = (uint32_t)handle->buffer.rxBuffer[0];
    dmaInit.DMA_DIR                = DMA_DIR_PeripheralSRC;
    dmaInit.DMA_BufferSize         = UART_RX_BUFFER_SIZE;
    dmaInit.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dmaInit.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInit.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dmaInit.DMA_Mode               = DMA_Mode_Normal;
    dmaInit.DMA_Priority           = DMA_Priority_High;
    dmaInit.DMA_M2M                = DMA_M2M_Disable;
    
    DMA_Init(handle->config.dmaRxChannel, &dmaInit);
    DMA_Cmd(handle->config.dmaRxChannel, ENABLE);
}


/**
 * @brief  配置DMA发送通道
 * @param  handle: UART句柄指针
 * @note   内存->外设，单次模式，初始化时不启动
 */
static void uartDmaTxConfig(UartHandle* handle)
{
    DMA_InitTypeDef dmaInit;
    
    DMA_DeInit(handle->config.dmaTxChannel);
    
    dmaInit.DMA_PeripheralBaseAddr = (uint32_t)&handle->config.usartx->DR;
    dmaInit.DMA_MemoryBaseAddr     = (uint32_t)handle->buffer.txBuffer;
    dmaInit.DMA_DIR                = DMA_DIR_PeripheralDST;
    dmaInit.DMA_BufferSize         = 0;  /* 发送时再设置 */
    dmaInit.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dmaInit.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInit.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dmaInit.DMA_Mode               = DMA_Mode_Normal;
    dmaInit.DMA_Priority           = DMA_Priority_Medium;
    dmaInit.DMA_M2M                = DMA_M2M_Disable;
    
    DMA_Init(handle->config.dmaTxChannel, &dmaInit);
    
    /* 使能发送完成中断 */
    DMA_ITConfig(handle->config.dmaTxChannel, DMA_IT_TC, ENABLE);
}


/**
 * @brief  配置NVIC中断
 * @param  handle: UART句柄指针
 * @note   配置USART中断和DMA发送完成中断
 */
static void uartNvicConfig(UartHandle* handle)
{
    NVIC_InitTypeDef nvicInit;
    
    /* USART中断 (IDLE) */
    nvicInit.NVIC_IRQChannel                   = handle->config.nvicIrqChannel;
    nvicInit.NVIC_IRQChannelPreemptionPriority = handle->config.nvicPrePriority;
    nvicInit.NVIC_IRQChannelSubPriority        = handle->config.nvicSubPriority;
    nvicInit.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvicInit);
    
    /* DMA发送完成中断 */
    nvicInit.NVIC_IRQChannel                   = handle->config.dmaTxIrqChannel;
    nvicInit.NVIC_IRQChannelPreemptionPriority = handle->config.nvicPrePriority + 1;
    nvicInit.NVIC_IRQChannelSubPriority        = 0;
    NVIC_Init(&nvicInit);
}


/*============================================================================*/
/*                              发送函数实现                                  */
/*============================================================================*/

/**
 * @brief  DMA方式发送数据 (非阻塞)
 * @param  handle: UART句柄指针
 * @param  data: 待发送数据
 * @param  len: 数据长度
 * @retval UART_OK/UART_BUSY/UART_ERROR
 */
uint8_t uartSendDma(UartHandle* handle, uint8_t* data, uint16_t len)
{
    if (handle == NULL || data == NULL || len == 0)
        return UART_ERROR;
    
    if (handle->buffer.txBusy)
        return UART_BUSY;
    
    if (len > UART_TX_BUFFER_SIZE)
        len = UART_TX_BUFFER_SIZE;
    
    /* 复制数据到发送缓冲 */
    memcpy(handle->buffer.txBuffer, data, len);
    
    /* 设置忙标志 */
    handle->buffer.txBusy = 1;
    
    /* 配置并启动DMA发送 */
    DMA_Cmd(handle->config.dmaTxChannel, DISABLE);
    
    /* 清除DMA传输完成标志 (重要: 否则新传输无法启动) */
    DMA_ClearFlag(handle->config.dmaTxTcFlag);
    
    /* 设置传输长度并启动 */
    handle->config.dmaTxChannel->CNDTR = len;
    DMA_Cmd(handle->config.dmaTxChannel, ENABLE);
    
    return UART_OK;
}


/**
 * @brief  阻塞方式发送数据
 * @param  handle: UART句柄指针
 * @param  data: 待发送数据
 * @param  len: 数据长度
 * @retval UART_OK
 * @note   兼容旧代码，逐字节轮询发送
 */
uint8_t uartSendBlocking(UartHandle* handle, uint8_t* data, uint16_t len)
{
    uint16_t i;
    
    if (handle == NULL || data == NULL)
        return UART_ERROR;
    
    for (i = 0; i < len; i++)
    {
        /* 等待发送缓冲区空 */
        while (USART_GetFlagStatus(handle->config.usartx, USART_FLAG_TXE) == RESET);
        USART_SendData(handle->config.usartx, data[i]);
    }
    
    /* 等待发送完成 */
    while (USART_GetFlagStatus(handle->config.usartx, USART_FLAG_TC) == RESET);
    
    return UART_OK;
}


/*============================================================================*/
/*                              中断处理函数                                  */
/*============================================================================*/

/**
 * @brief  USART IDLE中断处理
 * @param  handle: UART句柄指针
 * @note   帧接收完成时调用，切换双缓冲
 */
void uartIdleIrqHandler(UartHandle* handle)
{
    uint16_t rxLen;
    uint8_t  prevIdx;
    volatile uint32_t temp;
    
    if (handle == NULL)
        return;
    
    /* 检查IDLE标志 - 直接检查SR寄存器 */
    if ((handle->config.usartx->SR & USART_FLAG_IDLE) == 0)
        return;
    
    /* 清除IDLE标志: 读SR + 读DR */
    temp = handle->config.usartx->SR;
    temp = handle->config.usartx->DR;
    (void)temp;
    
    /* 停止DMA */
    DMA_Cmd(handle->config.dmaRxChannel, DISABLE);
    
    /* 计算接收长度 */
    rxLen = UART_RX_BUFFER_SIZE - DMA_GetCurrDataCounter(handle->config.dmaRxChannel);
    
    if (rxLen > 0)
    {
        /* 保存当前缓冲索引 */
        prevIdx = handle->buffer.activeRxIdx;
        handle->buffer.readyRxIdx = prevIdx;
        handle->buffer.rxDataLen = rxLen;
        
        /* 切换到另一个缓冲区 */
        handle->buffer.activeRxIdx = 1 - prevIdx;
        
        /* 重配置DMA到新缓冲区 */
        handle->config.dmaRxChannel->CMAR = 
            (uint32_t)handle->buffer.rxBuffer[handle->buffer.activeRxIdx];
        handle->config.dmaRxChannel->CNDTR = UART_RX_BUFFER_SIZE;
        
        /* 设置接收完成标志 */
        handle->buffer.rxComplete = 1;
        
        /* 调用回调函数 (如果已注册) */
        if (handle->rxCallback != NULL)
        {
            handle->rxCallback(handle->buffer.rxBuffer[prevIdx], rxLen);
        }
    }
    else
    {
        /* 无数据，重置DMA计数 */
        handle->config.dmaRxChannel->CNDTR = UART_RX_BUFFER_SIZE;
    }
    
    /* 重新使能DMA */
    DMA_Cmd(handle->config.dmaRxChannel, ENABLE);
}


/**
 * @brief  DMA发送完成中断处理
 * @param  handle: UART句柄指针
 */
void uartDmaTxIrqHandler(UartHandle* handle)
{
    if (handle == NULL)
        return;
    
    /* 检查并清除发送完成标志 */
    if (DMA_GetITStatus(handle->config.dmaTxTcFlag) != RESET)
    {
        DMA_ClearITPendingBit(handle->config.dmaTxTcFlag);
        handle->buffer.txBusy = 0;
    }
}


/*============================================================================*/
/*                              状态查询函数                                  */
/*============================================================================*/

/**
 * @brief  获取接收数据
 * @param  handle: UART句柄指针
 * @param  data: 输出数据指针
 * @param  len: 输出长度
 * @retval UART_OK/UART_ERROR
 */
uint8_t uartGetRxData(UartHandle* handle, uint8_t** data, uint16_t* len)
{
    if (handle == NULL || data == NULL || len == NULL)
        return UART_ERROR;
    
    if (handle->buffer.rxComplete == 0)
        return UART_ERROR;
    
    *data = handle->buffer.rxBuffer[handle->buffer.readyRxIdx];
    *len  = handle->buffer.rxDataLen;
    
    return UART_OK;
}


/**
 * @brief  清除接收完成标志
 * @param  handle: UART句柄指针
 */
void uartClearRxFlag(UartHandle* handle)
{
    if (handle != NULL)
        handle->buffer.rxComplete = 0;
}


/**
 * @brief  查询发送是否忙
 * @param  handle: UART句柄指针
 * @retval 1:忙, 0:空闲
 */
uint8_t uartIsTxBusy(UartHandle* handle)
{
    if (handle == NULL)
        return 0;
    return handle->buffer.txBusy;
}


/**
 * @brief  查询是否有数据待处理
 * @param  handle: UART句柄指针
 * @retval 1:有数据, 0:无数据
 */
uint8_t uartIsRxReady(UartHandle* handle)
{
    if (handle == NULL)
        return 0;
    return handle->buffer.rxComplete;
}


/**
 * @brief  注册接收完成回调
 * @param  handle: UART句柄指针
 * @param  callback: 回调函数
 */
void uartRegisterRxCallback(UartHandle* handle, UartRxCallback callback)
{
    if (handle != NULL)
        handle->rxCallback = callback;
}


/*============================================================================*/
/*                         USART2/USART3便捷初始化函数                        */
/*============================================================================*/

/**
 * @brief  初始化USART2 (显示屏通信)
 * @param  baudRate: 波特率
 * @retval UART_OK
 */
uint8_t uartDisplayInit(uint32_t baudRate)
{
    /* USART2配置: PA2(TX), PA3(RX), DMA1_CH6(RX), DMA1_CH7(TX) */
    uartDisplayHandle.config.usartx         = USART2;
    uartDisplayHandle.config.baudRate       = baudRate;
    uartDisplayHandle.config.txGpioPort     = GPIOA;
    uartDisplayHandle.config.txGpioPin      = GPIO_Pin_2;
    uartDisplayHandle.config.rxGpioPort     = GPIOA;
    uartDisplayHandle.config.rxGpioPin      = GPIO_Pin_3;
    uartDisplayHandle.config.rccUsartPeriph = RCC_APB1Periph_USART2;
    uartDisplayHandle.config.rccGpioPeriph  = RCC_APB2Periph_GPIOA;
    uartDisplayHandle.config.nvicIrqChannel = USART2_IRQn;
    uartDisplayHandle.config.nvicPrePriority= 2;
    uartDisplayHandle.config.nvicSubPriority= 0;
    uartDisplayHandle.config.dmaRxChannel   = DMA1_Channel6;
    uartDisplayHandle.config.dmaTxChannel   = DMA1_Channel7;
    uartDisplayHandle.config.dmaRxTcFlag    = DMA1_FLAG_TC6;
    uartDisplayHandle.config.dmaTxTcFlag    = DMA1_IT_TC7;
    uartDisplayHandle.config.dmaRxIrqChannel= DMA1_Channel6_IRQn;
    uartDisplayHandle.config.dmaTxIrqChannel= DMA1_Channel7_IRQn;
    
    return uartDriverInit(&uartDisplayHandle);
}


/**
 * @brief  初始化USART3 (主从机通信)
 * @param  baudRate: 波特率
 * @retval UART_OK
 */
uint8_t uartSlaveInit(uint32_t baudRate)
{
    /* USART3配置: PB10(TX), PB11(RX), DMA1_CH3(RX), DMA1_CH2(TX) */
    uartSlaveHandle.config.usartx         = USART3;
    uartSlaveHandle.config.baudRate       = baudRate;
    uartSlaveHandle.config.txGpioPort     = GPIOB;
    uartSlaveHandle.config.txGpioPin      = GPIO_Pin_10;
    uartSlaveHandle.config.rxGpioPort     = GPIOB;
    uartSlaveHandle.config.rxGpioPin      = GPIO_Pin_11;
    uartSlaveHandle.config.rccUsartPeriph = RCC_APB1Periph_USART3;
    uartSlaveHandle.config.rccGpioPeriph  = RCC_APB2Periph_GPIOB;
    uartSlaveHandle.config.nvicIrqChannel = USART3_IRQn;
    uartSlaveHandle.config.nvicPrePriority= 0;
    uartSlaveHandle.config.nvicSubPriority= 0;
    uartSlaveHandle.config.dmaRxChannel   = DMA1_Channel3;
    uartSlaveHandle.config.dmaTxChannel   = DMA1_Channel2;
    uartSlaveHandle.config.dmaRxTcFlag    = DMA1_FLAG_TC3;
    uartSlaveHandle.config.dmaTxTcFlag    = DMA1_IT_TC2;
    uartSlaveHandle.config.dmaRxIrqChannel= DMA1_Channel3_IRQn;
    uartSlaveHandle.config.dmaTxIrqChannel= DMA1_Channel2_IRQn;
    
    return uartDriverInit(&uartSlaveHandle);
}

