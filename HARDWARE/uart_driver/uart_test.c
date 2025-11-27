/**
 * @file    uart_test.c
 * @brief   UART回环压力测试模块实现
 * @author  AI Assistant
 * @date    2025
 * @version 1.0
 */

#include "uart_test.h"
#include "usart.h"
#include <string.h>

/*============================================================================*/
/*                              私有变量                                       */
/*============================================================================*/

/** @brief 测试状态结构 */
static struct {
    UartHandle* handle;           /**< 当前测试的UART句柄 */
    UartTestResult_t result;        /**< 测试结果 */
    uint32_t targetCount;           /**< 目标测试次数 */
    uint8_t  packetSize;            /**< 数据包大小 */
    uint8_t  txBuffer[UART_TEST_MAX_PACKET_SIZE];  /**< 发送缓冲 */
    uint8_t  expectedData[UART_TEST_MAX_PACKET_SIZE]; /**< 期望接收数据 */
    uint32_t sendTimestamp;         /**< 发送时间戳 */
    uint32_t totalLatency;          /**< 累计延迟 */
    uint8_t  waitingResponse;       /**< 等待响应标志 */
    uint8_t  sequenceNum;           /**< 序列号 */
} testCtx;

/** @brief 系统时钟计数 (需要外部提供) */
extern volatile uint32_t sysTickCount;

/*============================================================================*/
/*                              私有函数                                       */
/*============================================================================*/

/**
 * @brief  获取当前时间戳(ms)
 * @return 时间戳
 */
static uint32_t getTimestampMs(void)
{
    return sysTickCount;
}

/**
 * @brief  生成测试数据包
 * @param  buffer: 数据缓冲区
 * @param  size: 数据大小
 * @param  seqNum: 序列号
 * @return 无
 */
static void generateTestPacket(uint8_t* buffer, uint8_t size, uint8_t seqNum)
{
    uint8_t i;
    uint16_t checksum = 0;
    
    if (size < 4)
        size = 4;
    
    /* 帧头 */
    buffer[0] = 0xAA;
    buffer[1] = seqNum;
    buffer[2] = size;
    
    /* 填充数据 */
    for (i = 3; i < size - 2; i++)
    {
        buffer[i] = (uint8_t)(seqNum + i);
    }
    
    /* 计算校验和 */
    for (i = 0; i < size - 2; i++)
    {
        checksum += buffer[i];
    }
    
    /* 校验和 (低字节在前) */
    buffer[size - 2] = (uint8_t)(checksum & 0xFF);
    buffer[size - 1] = (uint8_t)((checksum >> 8) & 0xFF);
}

/**
 * @brief  验证接收数据包
 * @param  rxData: 接收数据
 * @param  rxLen: 接收长度
 * @param  expectedData: 期望数据
 * @param  expectedLen: 期望长度
 * @return 错误类型
 */
static UartTestError_e verifyPacket(uint8_t* rxData, uint16_t rxLen,
                                     uint8_t* expectedData, uint8_t expectedLen)
{
    uint8_t i;
    uint16_t rxChecksum;
    uint16_t calcChecksum = 0;
    
    /* 长度检查 */
    if (rxLen != expectedLen)
    {
        return UART_TEST_ERR_LENGTH;
    }
    
    /* 计算校验和 */
    for (i = 0; i < rxLen - 2; i++)
    {
        calcChecksum += rxData[i];
    }
    
    rxChecksum = rxData[rxLen - 2] | ((uint16_t)rxData[rxLen - 1] << 8);
    
    if (rxChecksum != calcChecksum)
    {
        return UART_TEST_ERR_CRC;
    }
    
    /* 数据比对 */
    for (i = 0; i < rxLen; i++)
    {
        if (rxData[i] != expectedData[i])
        {
            return UART_TEST_ERR_DATA;
        }
    }
    
    return UART_TEST_ERR_NONE;
}

/**
 * @brief  发送下一个测试包
 * @return 无
 */
static void sendNextPacket(void)
{
    /* 生成测试数据 */
    generateTestPacket(testCtx.txBuffer, testCtx.packetSize, testCtx.sequenceNum);
    
    /* 保存期望数据用于验证 */
    memcpy(testCtx.expectedData, testCtx.txBuffer, testCtx.packetSize);
    
    /* 记录发送时间 */
    testCtx.sendTimestamp = getTimestampMs();
    
    /* 发送数据 */
    uartSendDma(testCtx.handle, testCtx.txBuffer, testCtx.packetSize);
    
    testCtx.waitingResponse = 1;
    testCtx.sequenceNum++;
}

/**
 * @brief  更新延迟统计
 * @param  latency: 本次延迟(ms)
 * @return 无
 */
static void updateLatencyStats(uint32_t latency)
{
    if (latency < testCtx.result.minLatencyMs || testCtx.result.minLatencyMs == 0)
    {
        testCtx.result.minLatencyMs = latency;
    }
    
    if (latency > testCtx.result.maxLatencyMs)
    {
        testCtx.result.maxLatencyMs = latency;
    }
    
    testCtx.totalLatency += latency;
    
    if (testCtx.result.successCount > 0)
    {
        testCtx.result.avgLatencyMs = testCtx.totalLatency / testCtx.result.successCount;
    }
}

/*============================================================================*/
/*                              公共函数实现                                    */
/*============================================================================*/

void uartTestInit(UartHandle* handle)
{
    memset(&testCtx, 0, sizeof(testCtx));
    testCtx.handle = handle;
}

uint8_t uartTestStartLoopback(UartHandle* handle, uint32_t testCount, uint8_t packetSize)
{
    if (testCtx.result.isRunning)
    {
        return 1;  /* 已在运行 */
    }
    
    /* 参数检查 */
    if (packetSize < 4)
        packetSize = 4;
    if (packetSize > UART_TEST_MAX_PACKET_SIZE)
        packetSize = UART_TEST_MAX_PACKET_SIZE;
    
    /* 初始化测试上下文 */
    memset(&testCtx.result, 0, sizeof(UartTestResult_t));
    testCtx.handle = handle;
    testCtx.targetCount = testCount;
    testCtx.packetSize = packetSize;
    testCtx.sequenceNum = 0;
    testCtx.totalLatency = 0;
    testCtx.waitingResponse = 0;
    testCtx.result.isRunning = 1;
    
    /* 清除接收标志 */
    uartClearRxFlag(handle);
    
    /* 发送第一个包 */
    sendNextPacket();
    
    return 0;
}

void uartTestStop(void)
{
    testCtx.result.isRunning = 0;
    testCtx.waitingResponse = 0;
}

uint8_t uartTestProcess(void)
{
    uint8_t* rxData;
    uint16_t rxLen;
    uint32_t currentTime;
    uint32_t latency;
    UartTestError_e err;
    
    if (!testCtx.result.isRunning)
    {
        return 1;  /* 测试未运行 */
    }
    
    currentTime = getTimestampMs();
    
    /* 等待响应中 */
    if (testCtx.waitingResponse)
    {
        /* 检查超时 */
        if ((currentTime - testCtx.sendTimestamp) > UART_TEST_TIMEOUT_MS)
        {
            testCtx.result.totalCount++;
            testCtx.result.failCount++;
            testCtx.result.timeoutCount++;
            testCtx.result.lastError = UART_TEST_ERR_TIMEOUT;
            testCtx.waitingResponse = 0;
        }
        /* 检查是否收到数据 */
        else if (uartIsRxReady(testCtx.handle))
        {
            if (uartGetRxData(testCtx.handle, &rxData, &rxLen) == UART_OK)
            {
                latency = currentTime - testCtx.sendTimestamp;
                
                /* 验证数据 */
                err = verifyPacket(rxData, rxLen, 
                                   testCtx.expectedData, testCtx.packetSize);
                
                testCtx.result.totalCount++;
                
                if (err == UART_TEST_ERR_NONE)
                {
                    testCtx.result.successCount++;
                    updateLatencyStats(latency);
                }
                else
                {
                    testCtx.result.failCount++;
                    testCtx.result.lastError = err;
                    
                    switch (err)
                    {
                        case UART_TEST_ERR_CRC:
                            testCtx.result.crcErrorCount++;
                            break;
                        case UART_TEST_ERR_LENGTH:
                            testCtx.result.lengthErrorCount++;
                            break;
                        case UART_TEST_ERR_DATA:
                            testCtx.result.dataErrorCount++;
                            break;
                        default:
                            break;
                    }
                }
                
                testCtx.waitingResponse = 0;
            }
        }
    }
    
    /* 发送下一个包 */
    if (!testCtx.waitingResponse)
    {
        /* 检查是否完成 */
        if (testCtx.targetCount > 0 && 
            testCtx.result.totalCount >= testCtx.targetCount)
        {
            testCtx.result.isRunning = 0;
            return 1;  /* 测试完成 */
        }
        
        /* 发送下一包 */
        sendNextPacket();
    }
    
    return 0;  /* 测试进行中 */
}

UartTestResult_t* uartTestGetResult(void)
{
    return &testCtx.result;
}

void uartTestReset(void)
{
    memset(&testCtx.result, 0, sizeof(UartTestResult_t));
    testCtx.sequenceNum = 0;
    testCtx.totalLatency = 0;
}

UartTestError_e uartTestSingleLoopback(UartHandle* handle, 
                                        uint8_t* data, 
                                        uint8_t length,
                                        uint32_t timeoutMs)
{
    uint8_t* rxData;
    uint16_t rxLen;
    uint32_t startTime;
    uint8_t i;
    
    /* 清除接收标志 */
    uartClearRxFlag(handle);
    
    /* 发送数据 */
    uartSendDma(handle, data, length);
    
    startTime = getTimestampMs();
    
    /* 等待接收 */
    while ((getTimestampMs() - startTime) < timeoutMs)
    {
        if (uartIsRxReady(handle))
        {
            if (uartGetRxData(handle, &rxData, &rxLen) == UART_OK)
            {
                /* 长度检查 */
                if (rxLen != length)
                {
                    return UART_TEST_ERR_LENGTH;
                }
                
                /* 数据比对 */
                for (i = 0; i < length; i++)
                {
                    if (rxData[i] != data[i])
                    {
                        return UART_TEST_ERR_DATA;
                    }
                }
                
                return UART_TEST_ERR_NONE;
            }
        }
    }
    
    return UART_TEST_ERR_TIMEOUT;
}

void uartTestPrintResult(void)
{
    UartTestResult_t* r = &testCtx.result;
    uint32_t successRate;
    
    if (r->totalCount > 0)
    {
        successRate = (r->successCount * 100) / r->totalCount;
    }
    else
    {
        successRate = 0;
    }
    
    printf("\r\n========== UART Test Result ==========\r\n");
    printf("Total:    %lu\r\n", r->totalCount);
    printf("Success:  %lu (%lu%%)\r\n", r->successCount, successRate);
    printf("Failed:   %lu\r\n", r->failCount);
    printf("  - Timeout: %lu\r\n", r->timeoutCount);
    printf("  - CRC Err: %lu\r\n", r->crcErrorCount);
    printf("  - Len Err: %lu\r\n", r->lengthErrorCount);
    printf("  - Data Err:%lu\r\n", r->dataErrorCount);
    printf("Latency(ms): min=%lu, max=%lu, avg=%lu\r\n", 
           r->minLatencyMs, r->maxLatencyMs, r->avgLatencyMs);
    printf("Status:   %s\r\n", r->isRunning ? "Running" : "Stopped");
    printf("=======================================\r\n");
}

