# UART DMA驱动模块规格

## 概述

统一的UART通信驱动，使用DMA+IDLE线检测实现高效的ModBus RTU通信。

## 模块信息

- **文件**: `HARDWARE/uart_driver/uart_driver.c`, `uart_driver.h`
- **兼容层**: `uart_compat.c`, `uart_compat.h`
- **测试模块**: `uart_test.c`, `uart_test.h`
- **版本**: 1.0

## 硬件配置

### 支持的UART通道

| 通道 | 用途 | 波特率 | DMA RX | DMA TX |
|------|------|--------|--------|--------|
| USART2 | 显示屏通信 | 9600 | DMA1_CH6 | DMA1_CH7 |
| USART3 | 从设备通信 | 9600 | DMA1_CH3 | DMA1_CH2 |

### GPIO配置

| USART | TX引脚 | RX引脚 |
|-------|--------|--------|
| USART2 | PA2 | PA3 |
| USART3 | PB10 | PB11 |

## 核心数据结构

### 设备句柄

```c
typedef struct {
    USART_TypeDef* usart;           // USART外设
    DMA_Channel_TypeDef* dmaRxCh;   // RX DMA通道
    DMA_Channel_TypeDef* dmaTxCh;   // TX DMA通道
    uint8_t rxBuffer[2][UART_RX_BUFFER_SIZE]; // 双缓冲
    uint8_t txBuffer[UART_TX_BUFFER_SIZE];    // 发送缓冲
    volatile uint8_t activeRxBuf;   // 当前接收缓冲索引
    volatile uint16_t rxLength;     // 接收数据长度
    volatile uint8_t rxReady;       // 接收完成标志
    volatile uint8_t txBusy;        // 发送忙标志
    UartHwConfig_t hwConfig;        // 硬件配置
} UartHandle_t;
```

### 返回状态

```c
typedef enum {
    UART_OK = 0,
    UART_ERR_BUSY,
    UART_ERR_TIMEOUT,
    UART_ERR_PARAM
} UartStatus_e;
```

## 功能要求

### Requirement: DMA接收

系统SHALL使用DMA进行UART数据接收，配合IDLE线检测自动识别帧结束。

#### Scenario: ModBus帧接收

- **WHEN** 串口接收到完整ModBus帧
- **AND** 检测到IDLE空闲
- **THEN** 触发IDLE中断
- **AND** 自动停止DMA传输
- **AND** 切换到备用缓冲区
- **AND** 设置rxReady标志

### Requirement: 双缓冲机制

系统SHALL实现双缓冲机制，避免数据覆盖。

#### Scenario: 连续接收

- **WHEN** 当前缓冲区数据正在处理
- **AND** 新数据到达
- **THEN** DMA写入备用缓冲区
- **AND** 不影响正在处理的数据

### Requirement: DMA发送

系统SHALL使用DMA进行UART数据发送。

#### Scenario: 发送数据

- **WHEN** 调用uartSendDma()
- **THEN** 数据通过DMA发送
- **AND** CPU可执行其他任务
- **AND** 发送完成触发中断清除txBusy标志

### Requirement: 超时保护

系统SHALL提供发送超时保护。

#### Scenario: 发送超时

- **WHEN** DMA发送未在指定时间完成
- **THEN** 返回UART_ERR_TIMEOUT
- **AND** 不阻塞系统

## API参考

### 初始化函数

| 函数 | 说明 |
|-----|------|
| `uartDisplayInit(baudrate)` | 初始化USART2(显示屏) |
| `uartSlaveInit(baudrate)` | 初始化USART3(从设备) |

### 数据操作

| 函数 | 说明 |
|-----|------|
| `uartSendDma(handle, data, len)` | DMA发送数据 |
| `uartIsRxReady(handle)` | 检查接收是否就绪 |
| `uartGetRxData(handle, &data, &len)` | 获取接收数据 |
| `uartClearRxFlag(handle)` | 清除接收标志 |

### 中断处理

| 函数 | 说明 |
|-----|------|
| `uartIdleIrqHandler(handle)` | IDLE中断处理 |
| `uartDmaTxIrqHandler(handle)` | DMA TX完成中断 |

## 中断配置

### NVIC优先级

| 中断 | 优先级 |
|------|--------|
| USART2_IRQn | 2:1 |
| USART3_IRQn | 2:2 |
| DMA1_Channel7_IRQn | 2:3 |
| DMA1_Channel2_IRQn | 2:3 |

## 使用示例

### 初始化

```c
#include "uart_driver.h"

// 初始化显示屏通信
uartDisplayInit(9600);

// 初始化从设备通信
uartSlaveInit(9600);
```

### 发送数据

```c
uint8_t txData[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
uartSendDma(&uartDisplayHandle, txData, sizeof(txData));
```

### 接收数据

```c
if (uartIsRxReady(&uartDisplayHandle))
{
    uint8_t* rxData;
    uint16_t rxLen;
    
    if (uartGetRxData(&uartDisplayHandle, &rxData, &rxLen) == UART_OK)
    {
        // 处理接收数据
        processModbusFrame(rxData, rxLen);
    }
}
```

## 兼容层

为保持与旧代码兼容，提供`uart_compat.c`模块：

```c
// 条件编译开关
#define USE_NEW_UART_DRIVER

// 兼容初始化
#ifdef USE_NEW_UART_DRIVER
    uartDisplayInit(9600);
#else
    uart2_init(9600);
#endif
```

## 测试模块

`uart_test.c`提供回环压力测试功能：

### 测试API

| 函数 | 说明 |
|-----|------|
| `uartTestInit(handle)` | 初始化测试 |
| `uartTestStartLoopback(handle, count, size)` | 启动回环测试 |
| `uartTestProcess()` | 测试处理(主循环调用) |
| `uartTestGetResult()` | 获取测试结果 |
| `uartTestPrintResult()` | 打印测试报告 |

### 测试结果结构

```c
typedef struct {
    uint32_t totalCount;       // 总测试次数
    uint32_t successCount;     // 成功次数
    uint32_t failCount;        // 失败次数
    uint32_t timeoutCount;     // 超时次数
    uint32_t crcErrorCount;    // CRC错误
    uint32_t lengthErrorCount; // 长度错误
    uint32_t dataErrorCount;   // 数据错误
    uint32_t minLatencyMs;     // 最小延迟
    uint32_t maxLatencyMs;     // 最大延迟
    uint32_t avgLatencyMs;     // 平均延迟
} UartTestResult_t;
```

## 版本历史

### v1.0 (2024)

- ✅ DMA+IDLE线检测接收
- ✅ 双缓冲机制
- ✅ DMA发送
- ✅ 兼容层支持渐进迁移
- ✅ 回环压力测试模块

