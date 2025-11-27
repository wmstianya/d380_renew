# Change: UART通信健壮性与可维护性优化

**状态**: 📋 待评审

## Why

当前串口重构已完成核心功能(DMA+双缓冲、统一ModBus协议层)，但在生产环境长期运行中仍存在以下风险：

1. **无错误恢复机制**: DMA传输错误、总线溢出时无自动恢复
2. **缺乏诊断手段**: 现场故障难以定位，无通信统计
3. **看门狗覆盖不完整**: DMA中断卡死时主循环仍运行
4. **上电通信故障**: 10秒上电延迟期间屏幕显示通信故障
5. **内存使用未优化**: 4个UART各512字节双缓冲，占用4KB RAM
6. **Flash磨损风险**: 参数频繁写入可能导致Flash寿命耗尽

## What Changes

### Phase 1: 错误恢复机制 (高优先级)

**文件**: `HARDWARE/uart_driver/uart_driver.c`

```c
/**
 * @brief UART错误处理与恢复
 * @param handle UART句柄
 */
void uartErrorHandler(UartHandle* handle)
{
    /* 检查DMA传输错误 */
    if (DMA_GetFlagStatus(handle->config.dmaRxCh, DMA_FLAG_TE)) {
        DMA_ClearFlag(handle->config.dmaRxCh, DMA_FLAG_TE);
        uartDmaRxReinit(handle);
        handle->stats.dmaErrors++;
    }
    
    /* 检查UART溢出错误 */
    if (USART_GetFlagStatus(handle->config.usartx, USART_FLAG_ORE)) {
        USART_ClearFlag(handle->config.usartx, USART_FLAG_ORE);
        handle->stats.overruns++;
    }
    
    /* 检查帧错误 */
    if (USART_GetFlagStatus(handle->config.usartx, USART_FLAG_FE)) {
        USART_ClearFlag(handle->config.usartx, USART_FLAG_FE);
        handle->stats.frameErrors++;
    }
}
```

### Phase 2: 通信统计与诊断 (中优先级)

**文件**: `HARDWARE/uart_driver/uart_driver.h`

```c
typedef struct {
    uint32_t txFrames;      /* 发送帧数 */
    uint32_t rxFrames;      /* 接收帧数 */
    uint32_t crcErrors;     /* CRC校验错误 */
    uint32_t timeouts;      /* 响应超时次数 */
    uint32_t overruns;      /* 接收溢出次数 */
    uint32_t dmaErrors;     /* DMA传输错误 */
    uint32_t frameErrors;   /* 帧格式错误 */
    uint32_t lastRxMs;      /* 最后接收时间戳 */
} UartStats;

/* 在UartHandle中添加 */
typedef struct {
    // ... existing fields ...
    UartStats stats;
} UartHandle;
```

### Phase 3: 看门狗集成 (高优先级)

**文件**: `HARDWARE/modbus/modbus_core.c`

```c
#define MODBUS_ACTIVITY_TIMEOUT_MS  30000  /* 30秒无活动超时 */

void modbusCheckActivity(ModbusHandle* handle)
{
    uint32_t now = modbusGetTimestampMs();
    
    /* 超过30秒无任何通信活动，记录告警 */
    if ((now - handle->lastActivityMs) > MODBUS_ACTIVITY_TIMEOUT_MS) {
        handle->stats.activityTimeouts++;
        handle->lastActivityMs = now;  /* 重置，避免重复触发 */
        
        /* 可选: 触发系统复位 */
        // NVIC_SystemReset();
    }
}
```

### Phase 4: 上电通信故障修复 (中优先级)

**文件**: `USER/main.c`

```c
/* 修改上电自检循环 */
while(sys_flag.Check_Finsh)
{
    IWDG_Feed();
    
#if USE_UNIFIED_MODBUS
    modbusUsart2Scheduler();  /* 上电阶段也响应屏幕通讯 */
#else
    Union_ModBus2_Communication();
#endif
    
    if(Power_ON_Begin_Check_Function())
        sys_flag.Check_Finsh = FALSE;
}
```

### Phase 5: 内存优化 (低优先级)

**文件**: `HARDWARE/uart_driver/uart_driver.h`

```c
/* 根据实际需求调整缓冲区大小 */
#define UART_DEBUG_BUF_SIZE    128   /* USART1: 调试/RTU服务器 */
#define UART_DISPLAY_BUF_SIZE  256   /* USART2: 屏幕通信，最大帧~100字节 */
#define UART_SLAVE_BUF_SIZE    64    /* USART3: 变频水泵，帧长固定 */
#define UART_UNION_BUF_SIZE    128   /* UART4: 联控通信 */

/* 总计: 128+256+64+128 = 576字节 (原4KB，节省87%) */
```

### Phase 6: Flash磨损保护 (低优先级)

**文件**: `SYSTEM/flash/flash_manager.c` (新建)

```c
#define FLASH_WRITE_DELAY_MS    5000   /* 延迟5秒写入 */
#define FLASH_DAILY_LIMIT       100    /* 每日最大写入次数 */

typedef struct {
    uint8_t  pendingWrite;      /* 是否有待写入数据 */
    uint32_t lastWriteMs;       /* 上次写入时间 */
    uint16_t dailyWriteCount;   /* 当日写入计数 */
    uint8_t  dailyResetHour;    /* 计数重置小时 */
} FlashManager;

/**
 * @brief 请求写入Flash (延迟合并)
 */
void flashRequestWrite(void)
{
    flashMgr.pendingWrite = 1;
    flashMgr.lastWriteMs = modbusGetTimestampMs();
}

/**
 * @brief Flash写入调度 (主循环调用)
 */
void flashWriteScheduler(void)
{
    if (!flashMgr.pendingWrite) return;
    
    uint32_t elapsed = modbusGetTimestampMs() - flashMgr.lastWriteMs;
    
    if (elapsed >= FLASH_WRITE_DELAY_MS) {
        if (flashMgr.dailyWriteCount < FLASH_DAILY_LIMIT) {
            Write_Admin_Flash();
            flashMgr.dailyWriteCount++;
        }
        flashMgr.pendingWrite = 0;
    }
}
```

## Impact

| 优化项 | 影响文件 | 风险 | 收益 |
|--------|----------|------|------|
| 错误恢复 | uart_driver.c | 低 | 提高稳定性 |
| 通信统计 | uart_driver.h/c | 低 | 便于诊断 |
| 看门狗集成 | modbus_core.c | 低 | 防止假死 |
| 上电故障 | main.c | 低 | 用户体验 |
| 内存优化 | uart_driver.h | 中 | 节省3.4KB |
| Flash保护 | 新建文件 | 低 | 延长寿命 |

## 预估工时

| Phase | 工时 | 依赖 |
|-------|------|------|
| Phase 1 | 2h | 无 |
| Phase 2 | 2h | 无 |
| Phase 3 | 1h | Phase 2 |
| Phase 4 | 0.5h | 无 |
| Phase 5 | 1h | 需测试验证 |
| Phase 6 | 4h | 无 |
| **总计** | **10.5h** | |

