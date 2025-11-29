/**
 * @file    uart_driver.h
 * @brief   统一UART驱动接口定义 - DMA + 双缓冲 + IDLE中断
 * @author  System
 * @date    2025-11-26
 * @version 2.1 (dma-only)
 * 
 * @note    本驱动支持USART1/USART2/USART3/UART4，使用DMA接收和发送
 *          通过IDLE中断检测帧结束，双缓冲保证数据处理期间不丢失
 * 
 * @warning 此版本为DMA专用版本，已移除旧的逐字节中断驱动支持
 * 
 * 修订历史:
 * - 2.1 (2025-11-27): 添加USART1和UART4的DMA支持
 * - 2.0 (2025-11-27): dma-only分支，移除旧驱动条件编译
 * - 1.0 (2025-11-26): 初始版本，支持DMA+IDLE+双缓冲
 */

#ifndef __UART_DRIVER_H
#define __UART_DRIVER_H

#include "stm32f10x.h"

/*============================================================================*/
/*                              宏定义                                        */
/*============================================================================*/

/** @brief 接收缓冲区大小 (ModBus常用帧长度，可根据需要调整) */
#define UART_RX_BUFFER_SIZE     256

/** @brief 发送缓冲区大小 */
#define UART_TX_BUFFER_SIZE     256

/** @brief 双缓冲数量 */
#define UART_DOUBLE_BUFFER_NUM  2

/** @brief 驱动返回值定义 */
#define UART_OK                 0
#define UART_ERROR              1
#define UART_BUSY               2
#define UART_TIMEOUT            3

/**
 * @brief UART通信统计结构体
 */
typedef struct {
    uint32_t txFrames;      /**< 发送帧数 */
    uint32_t rxFrames;      /**< 接收帧数 */
    uint32_t rxBytes;       /**< 接收字节数 */
    uint32_t overruns;      /**< 接收溢出错误 (ORE) */
    uint32_t frameErrors;   /**< 帧格式错误 (FE) */
    uint32_t noiseErrors;   /**< 噪声错误 (NE) */
    uint32_t dmaErrors;     /**< DMA传输错误 */
} UartStats;

/*============================================================================*/
/*                              类型定义                                      */
/*============================================================================*/

/**
 * @brief UART硬件配置结构体
 * @note  包含所有硬件相关配置，初始化时一次性设置
 */
typedef struct {
    USART_TypeDef*        usartx;           /**< USART外设指针 (USART2/USART3) */
    uint32_t              baudRate;         /**< 波特率 */
    
    /* GPIO配置 */
    GPIO_TypeDef*         txGpioPort;       /**< TX引脚GPIO端口 */
    uint16_t              txGpioPin;        /**< TX引脚编号 */
    GPIO_TypeDef*         rxGpioPort;       /**< RX引脚GPIO端口 */
    uint16_t              rxGpioPin;        /**< RX引脚编号 */
    
    /* 时钟配置 */
    uint32_t              rccUsartPeriph;   /**< USART时钟 (APB1) */
    uint32_t              rccGpioPeriph;    /**< GPIO时钟 (APB2) */
    
    /* 中断配置 */
    uint8_t               nvicIrqChannel;   /**< NVIC中断通道 */
    uint8_t               nvicPrePriority;  /**< 抢占优先级 */
    uint8_t               nvicSubPriority;  /**< 子优先级 */
    
    /* DMA配置 */
    DMA_Channel_TypeDef*  dmaRxChannel;     /**< DMA接收通道 */
    DMA_Channel_TypeDef*  dmaTxChannel;     /**< DMA发送通道 */
    uint32_t              dmaRxTcFlag;      /**< DMA接收完成标志 */
    uint32_t              dmaTxTcFlag;      /**< DMA发送完成标志 */
    uint8_t               dmaRxIrqChannel;  /**< DMA接收中断通道 */
    uint8_t               dmaTxIrqChannel;  /**< DMA发送中断通道 */
    
} UartHwConfig;


/**
 * @brief UART双缓冲数据结构体
 * @note  实现乒乓缓冲，保证数据处理期间不丢失新数据
 */
typedef struct {
    uint8_t  rxBuffer[UART_DOUBLE_BUFFER_NUM][UART_RX_BUFFER_SIZE]; /**< 双接收缓冲 */
    uint8_t  txBuffer[UART_TX_BUFFER_SIZE];   /**< 发送缓冲 */
    
    uint8_t  activeRxIdx;      /**< 当前DMA使用的接收缓冲索引 (0或1) */
    uint8_t  readyRxIdx;       /**< 已接收完成待处理的缓冲索引 */
    uint16_t rxDataLen;        /**< 接收数据长度 */
    
    volatile uint8_t rxComplete;  /**< 接收完成标志 (1:有数据待处理) */
    volatile uint8_t txBusy;      /**< 发送忙标志 (1:正在发送) */
    
} UartBufferData;


/**
 * @brief UART接收完成回调函数类型
 * @param data  接收数据指针
 * @param len   数据长度
 */
typedef void (*UartRxCallback)(uint8_t* data, uint16_t len);


/**
 * @brief UART驱动句柄结构体
 * @note  每个UART端口对应一个句柄实例
 */
typedef struct {
    UartHwConfig    config;     /**< 硬件配置 */
    UartBufferData  buffer;     /**< 缓冲区数据 */
    UartStats       stats;      /**< 通信统计 */
    UartRxCallback  rxCallback; /**< 接收完成回调函数 (可选) */
    uint8_t         initialized;/**< 初始化完成标志 */
} UartHandle;


/*============================================================================*/
/*                              外部变量声明                                  */
/*============================================================================*/

/** @brief USART1驱动句柄 (调试/RTU服务器通信) */
extern UartHandle uartDebugHandle;

/** @brief USART2驱动句柄 (显示屏通信) */
extern UartHandle uartDisplayHandle;

/** @brief USART3驱动句柄 (主从机通信) */
extern UartHandle uartSlaveHandle;

/** @brief UART4驱动句柄 (联控通信) */
extern UartHandle uartUnionHandle;


/*============================================================================*/
/*                              函数声明                                      */
/*============================================================================*/

/**
 * @brief  初始化UART驱动 (DMA + IDLE中断模式)
 * @param  handle: UART句柄指针
 * @retval UART_OK: 成功, UART_ERROR: 失败
 * @note   配置GPIO/USART/DMA/NVIC，使能IDLE中断和DMA
 */
uint8_t uartDriverInit(UartHandle* handle);


/**
 * @brief  DMA方式发送数据 (非阻塞)
 * @param  handle: UART句柄指针
 * @param  data: 待发送数据指针
 * @param  len: 数据长度
 * @retval UART_OK: 发送启动成功
 *         UART_BUSY: 上次发送未完成
 *         UART_ERROR: 参数错误
 * @note   数据会被复制到内部缓冲区，调用后立即返回
 */
uint8_t uartSendDma(UartHandle* handle, uint8_t* data, uint16_t len);


/**
 * @brief  阻塞方式发送数据 (兼容旧代码)
 * @param  handle: UART句柄指针
 * @param  data: 待发送数据指针
 * @param  len: 数据长度
 * @retval UART_OK: 发送完成
 * @note   会等待发送完成后返回，适用于需要顺序发送的场景
 */
uint8_t uartSendBlocking(UartHandle* handle, uint8_t* data, uint16_t len);


/**
 * @brief  USART IDLE中断处理函数 (在中断服务程序中调用)
 * @param  handle: UART句柄指针
 * @note   处理帧接收完成，切换双缓冲，设置完成标志
 */
void uartIdleIrqHandler(UartHandle* handle);


/**
 * @brief  DMA发送完成中断处理函数
 * @param  handle: UART句柄指针
 * @note   禁用DMA，启用USART TC中断等待真正发送完成
 */
void uartDmaTxIrqHandler(UartHandle* handle);


/**
 * @brief  USART发送完成中断处理函数
 * @param  handle: UART句柄指针
 * @note   移位寄存器发送完毕后调用，清除txBusy标志
 */
void uartTxCompleteHandler(UartHandle* handle);


/**
 * @brief  获取接收数据 (主循环中调用)
 * @param  handle: UART句柄指针
 * @param  data: 输出数据指针的指针
 * @param  len: 输出数据长度指针
 * @retval UART_OK: 有数据可读
 *         UART_ERROR: 无数据
 * @note   返回后需及时处理数据，处理完调用uartClearRxFlag()
 */
uint8_t uartGetRxData(UartHandle* handle, uint8_t** data, uint16_t* len);


/**
 * @brief  清除接收完成标志
 * @param  handle: UART句柄指针
 * @note   数据处理完成后调用，允许接收新数据
 */
void uartClearRxFlag(UartHandle* handle);


/**
 * @brief  查询发送是否忙
 * @param  handle: UART句柄指针
 * @retval 1: 忙, 0: 空闲
 */
uint8_t uartIsTxBusy(UartHandle* handle);


/**
 * @brief  查询是否有接收数据待处理
 * @param  handle: UART句柄指针
 * @retval 1: 有数据, 0: 无数据
 */
uint8_t uartIsRxReady(UartHandle* handle);


/**
 * @brief  注册接收完成回调函数
 * @param  handle: UART句柄指针
 * @param  callback: 回调函数指针
 * @note   回调在中断上下文执行，应尽量简短
 */
void uartRegisterRxCallback(UartHandle* handle, UartRxCallback callback);


/*============================================================================*/
/*                         各串口专用初始化函数                               */
/*============================================================================*/

/**
 * @brief  初始化USART1 (调试/RTU服务器通信)
 * @param  baudRate: 波特率 (默认9600)
 * @retval UART_OK: 成功
 * @note   使用DMA1_Channel5(RX)/DMA1_Channel4(TX)
 */
uint8_t uartDebugInit(uint32_t baudRate);


/**
 * @brief  初始化USART2 (显示屏通信)
 * @param  baudRate: 波特率 (默认115200)
 * @retval UART_OK: 成功
 * @note   使用DMA1_Channel6(RX)/DMA1_Channel7(TX)
 */
uint8_t uartDisplayInit(uint32_t baudRate);


/**
 * @brief  初始化USART3 (主从机通信)
 * @param  baudRate: 波特率 (默认115200)
 * @retval UART_OK: 成功
 * @note   使用DMA1_Channel3(RX)/DMA1_Channel2(TX)
 */
uint8_t uartSlaveInit(uint32_t baudRate);


/**
 * @brief  初始化UART4 (联控通信)
 * @param  baudRate: 波特率 (默认9600)
 * @retval UART_OK: 成功
 * @note   使用DMA2_Channel3(RX)/DMA2_Channel5(TX)
 */
uint8_t uartUnionInit(uint32_t baudRate);


#endif /* __UART_DRIVER_H */

