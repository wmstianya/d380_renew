/**
 * @file modbus_port.c
 * @brief ModBus RTU 移植层实现
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 * 
 * @note 此文件需要根据具体平台实现时间戳获取函数
 */

#include "modbus.h"
#include "sys.h"

/*============================================================================*/
/*                              时间戳实现                                     */
/*============================================================================*/

/* 毫秒计数器 (需要在SysTick或定时器中断中递增) */
static volatile uint32_t modbusTickMs = 0;

/**
 * @brief 获取系统时间戳 (ms)
 * @return 时间戳
 */
uint32_t modbusGetTick(void)
{
    return modbusTickMs;
}

/**
 * @brief 时间戳递增 (在1ms定时器中断中调用)
 * @note 需要在SysTick_Handler或TIM中断中调用此函数
 */
void modbusTickInc(void)
{
    modbusTickMs++;
}

/**
 * @brief 计算时间差
 * @param startTick 起始时间戳
 * @return 经过的毫秒数
 */
uint32_t modbusGetElapsed(uint32_t startTick)
{
    uint32_t current = modbusTickMs;
    
    if (current >= startTick) {
        return current - startTick;
    } else {
        /* 溢出处理 */
        return (0xFFFFFFFF - startTick) + current + 1;
    }
}

