/**
 * @file    uart_compat.c
 * @brief   UART驱动兼容层实现
 * @author  System
 * @date    2025-11-26
 * @version 1.0
 * 
 * 修订历史:
 * - 1.0 (2025-11-26): 初始版本
 */

#include "uart_compat.h"

/*============================================================================*/
/*                              全局变量定义                                  */
/*============================================================================*/

/** @brief USART2兼容数据 */
UartCompatData U2_Compat = {0};

/** @brief USART3兼容数据 */
UartCompatData U3_Compat = {0};


/*============================================================================*/
/*                              兼容层函数实现                                */
/*============================================================================*/

/**
 * @brief  初始化USART2 (兼容旧接口)
 * @param  bound: 波特率
 */
void uart2_init_new(uint32_t bound)
{
    /* 调用新驱动初始化 */
    uartDisplayInit(bound);
    
    /* 初始化兼容结构指针 */
    U2_Compat.TX_Data = uartDisplayHandle.buffer.txBuffer;
    U2_Compat.RX_Data = uartDisplayHandle.buffer.rxBuffer[0];
    U2_Compat.Recive_Ok_Flag = 0;
    U2_Compat.RX_Length = 0;
}


/**
 * @brief  初始化USART3 (兼容旧接口)
 * @param  bound: 波特率
 */
void uart3_init_new(uint32_t bound)
{
    /* 调用新驱动初始化 */
    uartSlaveInit(bound);
    
    /* 初始化兼容结构指针 */
    U3_Compat.TX_Data = uartSlaveHandle.buffer.txBuffer;
    U3_Compat.RX_Data = uartSlaveHandle.buffer.rxBuffer[0];
    U3_Compat.Recive_Ok_Flag = 0;
    U3_Compat.RX_Length = 0;
}


/**
 * @brief  同步兼容数据结构
 * @note   在主循环开头调用
 */
void uartCompatSync(void)
{
    uint8_t* rxData;
    uint16_t rxLen;
    
    /* 同步USART2 */
    if (uartIsRxReady(&uartDisplayHandle))
    {
        if (uartGetRxData(&uartDisplayHandle, &rxData, &rxLen) == UART_OK)
        {
            U2_Compat.RX_Data = rxData;
            U2_Compat.RX_Length = rxLen;
            U2_Compat.Rx_temp_length = rxLen;
            U2_Compat.Recive_Ok_Flag = 1;
        }
    }
    
    /* 同步USART3 */
    if (uartIsRxReady(&uartSlaveHandle))
    {
        if (uartGetRxData(&uartSlaveHandle, &rxData, &rxLen) == UART_OK)
        {
            U3_Compat.RX_Data = rxData;
            U3_Compat.RX_Length = rxLen;
            U3_Compat.Rx_temp_length = rxLen;
            U3_Compat.Recive_Ok_Flag = 1;
        }
    }
}


/**
 * @brief  USART2发送数据 (兼容)
 * @param  data: 数据指针
 * @param  len: 长度
 */
void U2_SendData_New(uint8_t* data, uint16_t len)
{
    /* 优先使用DMA发送 */
    if (uartSendDma(&uartDisplayHandle, data, len) == UART_BUSY)
    {
        /* DMA忙则使用阻塞发送 */
        uartSendBlocking(&uartDisplayHandle, data, len);
    }
}


/**
 * @brief  USART3发送数据 (兼容)
 * @param  data: 数据指针
 * @param  len: 长度
 */
void U3_SendData_New(uint8_t* data, uint16_t len)
{
    if (uartSendDma(&uartSlaveHandle, data, len) == UART_BUSY)
    {
        uartSendBlocking(&uartSlaveHandle, data, len);
    }
}


/**
 * @brief  清除USART2接收完成标志
 */
void U2_ClearRxFlag(void)
{
    uartClearRxFlag(&uartDisplayHandle);
    U2_Compat.Recive_Ok_Flag = 0;
}


/**
 * @brief  清除USART3接收完成标志
 */
void U3_ClearRxFlag(void)
{
    uartClearRxFlag(&uartSlaveHandle);
    U3_Compat.Recive_Ok_Flag = 0;
}

