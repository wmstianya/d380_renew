/**
 * @file    uart_compat.h
 * @brief   UART驱动兼容层 - 提供与旧代码兼容的接口
 * @author  System
 * @date    2025-11-26
 * @version 1.0
 * 
 * @note    包含此头文件可使旧代码无缝迁移到新驱动
 *          提供U2_Inf/U3_Inf结构体的映射
 * 
 * 修订历史:
 * - 1.0 (2025-11-26): 初始版本
 */

#ifndef __UART_COMPAT_H
#define __UART_COMPAT_H

#include "uart_driver.h"

/*============================================================================*/
/*                              兼容性数据结构                                */
/*============================================================================*/

/**
 * @brief 旧代码兼容的UART数据结构
 * @note  映射到新驱动的缓冲区
 */
typedef struct {
    uint8_t* TX_Data;           /**< 发送数据指针 (映射到txBuffer) */
    uint8_t* RX_Data;           /**< 接收数据指针 (映射到当前rxBuffer) */
    uint8_t  Rx_temp_length;    /**< 临时接收长度 (兼容用，实际使用rxDataLen) */
    uint8_t  RX_Length;         /**< 接收数据长度 */
    uint8_t  send_flag;         /**< 发送标志 */
    uint8_t  Recive_Time;       /**< 接收超时计数 (新驱动不需要) */
    uint8_t  Recive_Flag;       /**< 接收中标志 */
    uint8_t  Recive_Ok_Flag;    /**< 接收完成标志 */
    uint8_t  Flag_100ms;        /**< 100ms标志 */
} UartCompatData;


/*============================================================================*/
/*                              兼容性全局变量                                */
/*============================================================================*/

/** @brief USART2兼容数据结构 */
extern UartCompatData U2_Compat;

/** @brief USART3兼容数据结构 */
extern UartCompatData U3_Compat;


/*============================================================================*/
/*                              兼容性宏定义                                  */
/*============================================================================*/

/**
 * @brief 将U2_Inf映射到兼容结构
 * @note  旧代码中的 U2_Inf.RX_Data 将自动映射
 */
#define U2_Inf_NEW  U2_Compat

/**
 * @brief 将U3_Inf映射到兼容结构
 */
#define U3_Inf_NEW  U3_Compat


/*============================================================================*/
/*                              兼容性函数声明                                */
/*============================================================================*/

/**
 * @brief  初始化USART2 (兼容旧接口)
 * @param  bound: 波特率
 * @note   内部调用新驱动 uartDisplayInit()
 */
void uart2_init_new(uint32_t bound);


/**
 * @brief  初始化USART3 (兼容旧接口)
 * @param  bound: 波特率
 * @note   内部调用新驱动 uartSlaveInit()
 */
void uart3_init_new(uint32_t bound);


/**
 * @brief  同步兼容数据结构
 * @note   在主循环中调用，将新驱动数据同步到兼容结构
 */
void uartCompatSync(void);


/**
 * @brief  USART2发送数据 (兼容旧接口)
 * @param  data: 数据指针
 * @param  len: 数据长度
 */
void U2_SendData_New(uint8_t* data, uint16_t len);


/**
 * @brief  USART3发送数据 (兼容旧接口)
 * @param  data: 数据指针
 * @param  len: 数据长度
 */
void U3_SendData_New(uint8_t* data, uint16_t len);


/**
 * @brief  清除USART2接收完成标志
 */
void U2_ClearRxFlag(void);


/**
 * @brief  清除USART3接收完成标志
 */
void U3_ClearRxFlag(void);


/*============================================================================*/
/*                         新旧中断处理切换宏                                 */
/*============================================================================*/

/**
 * @brief 启用新UART驱动的中断处理
 * @note  在stm32f10x_it.c中定义此宏以使用新驱动
 */
/* #define USE_NEW_UART_DRIVER */

#ifdef USE_NEW_UART_DRIVER

/**
 * @brief USART2中断处理宏 (使用新驱动)
 */
#define USART2_IRQ_HANDLER_NEW()  uartIdleIrqHandler(&uartDisplayHandle)

/**
 * @brief USART3中断处理宏 (使用新驱动)
 */
#define USART3_IRQ_HANDLER_NEW()  uartIdleIrqHandler(&uartSlaveHandle)

/**
 * @brief DMA1_Channel7中断处理宏 (USART2 TX完成)
 */
#define DMA1_CH7_IRQ_HANDLER_NEW()  uartDmaTxIrqHandler(&uartDisplayHandle)

/**
 * @brief DMA1_Channel2中断处理宏 (USART3 TX完成)
 */
#define DMA1_CH2_IRQ_HANDLER_NEW()  uartDmaTxIrqHandler(&uartSlaveHandle)

#endif /* USE_NEW_UART_DRIVER */


#endif /* __UART_COMPAT_H */

