#include "elog.h"
#include "stm32f10x.h"
#include <stdio.h>

/* 引用系统滴答计数器 */
extern volatile uint32_t sysTickCount; 

static char time_buf[32];

/**
 * @brief 获取系统时间字符串
 */
const char *elog_port_get_time(void)
{
    uint32_t now = sysTickCount;
    
    /* 格式: [秒.毫秒] */
    snprintf(time_buf, sizeof(time_buf), "[%lu.%03lu]", 
             now / 1000, now % 1000);
             
    return time_buf;
}

/**
 * @brief 端口输出函数
 * @param log 日志内容
 * @param size 日志长度
 */
void elog_port_output(const char *log, size_t size)
{
    const char *p = log;
    
    /* 阻塞发送到 USART1 (调试口) */
    while (size--) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *p++);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}
