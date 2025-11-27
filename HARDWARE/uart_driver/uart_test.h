/**
 * @file    uart_test.h
 * @brief   UART回环压力测试模块
 * @author  AI Assistant
 * @date    2024
 * @version 1.0
 * 
 * @details 用于验证UART DMA驱动的稳定性和性能
 *          支持回环测试（TX->RX短接）和性能统计
 */

#ifndef __UART_TEST_H
#define __UART_TEST_H

#include "stm32f10x.h"
#include "uart_driver.h"

/*============================================================================*/
/*                              测试配置                                       */
/*============================================================================*/

/** @brief 测试数据包最大长度 */
#define UART_TEST_MAX_PACKET_SIZE   64

/** @brief 测试超时时间(ms) */
#define UART_TEST_TIMEOUT_MS        100

/** @brief 默认测试次数 */
#define UART_TEST_DEFAULT_COUNT     1000

/*============================================================================*/
/*                              测试结果结构                                   */
/*============================================================================*/

/** @brief 测试统计结果 */
typedef struct {
    uint32_t totalCount;        /**< 总测试次数 */
    uint32_t successCount;      /**< 成功次数 */
    uint32_t failCount;         /**< 失败次数 */
    uint32_t timeoutCount;      /**< 超时次数 */
    uint32_t crcErrorCount;     /**< CRC错误次数 */
    uint32_t lengthErrorCount;  /**< 长度错误次数 */
    uint32_t dataErrorCount;    /**< 数据错误次数 */
    uint32_t minLatencyMs;      /**< 最小延迟(ms) */
    uint32_t maxLatencyMs;      /**< 最大延迟(ms) */
    uint32_t avgLatencyMs;      /**< 平均延迟(ms) */
    uint8_t  isRunning;         /**< 测试进行中标志 */
    uint8_t  lastError;         /**< 最后一次错误类型 */
} UartTestResult_t;

/** @brief 错误类型枚举 */
typedef enum {
    UART_TEST_ERR_NONE = 0,     /**< 无错误 */
    UART_TEST_ERR_TIMEOUT,      /**< 超时 */
    UART_TEST_ERR_CRC,          /**< CRC校验失败 */
    UART_TEST_ERR_LENGTH,       /**< 长度不匹配 */
    UART_TEST_ERR_DATA,         /**< 数据不匹配 */
    UART_TEST_ERR_BUSY          /**< 驱动忙 */
} UartTestError_e;

/** @brief 测试模式枚举 */
typedef enum {
    UART_TEST_MODE_LOOPBACK,    /**< 回环测试(TX->RX短接) */
    UART_TEST_MODE_ECHO,        /**< 回显测试(需要对端设备) */
    UART_TEST_MODE_STRESS       /**< 压力测试(连续发送) */
} UartTestMode_e;

/*============================================================================*/
/*                              API函数声明                                    */
/*============================================================================*/

/**
 * @brief  初始化测试模块
 * @param  handle: UART句柄指针
 * @return 无
 */
void uartTestInit(UartHandle* handle);

/**
 * @brief  启动回环压力测试
 * @param  handle: UART句柄指针
 * @param  testCount: 测试次数 (0=无限循环)
 * @param  packetSize: 数据包大小 (1-64)
 * @return 0:成功启动, 其他:错误
 */
uint8_t uartTestStartLoopback(UartHandle* handle, uint32_t testCount, uint8_t packetSize);

/**
 * @brief  停止测试
 * @return 无
 */
void uartTestStop(void);

/**
 * @brief  测试处理函数 (需在主循环中调用)
 * @return 0:测试进行中, 1:测试完成
 */
uint8_t uartTestProcess(void);

/**
 * @brief  获取测试结果
 * @return 测试结果结构指针
 */
UartTestResult_t* uartTestGetResult(void);

/**
 * @brief  重置测试统计
 * @return 无
 */
void uartTestReset(void);

/**
 * @brief  单次回环测试
 * @param  handle: UART句柄指针
 * @param  data: 发送数据
 * @param  length: 数据长度
 * @param  timeoutMs: 超时时间(ms)
 * @return 错误类型
 */
UartTestError_e uartTestSingleLoopback(UartHandle* handle, 
                                        uint8_t* data, 
                                        uint8_t length,
                                        uint32_t timeoutMs);

/**
 * @brief  打印测试结果 (通过USART1输出)
 * @return 无
 */
void uartTestPrintResult(void);

#endif /* __UART_TEST_H */

