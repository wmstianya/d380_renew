# UART Driver - DMA + 双缓冲 + IDLE中断

统一UART驱动框架，支持USART2和USART3。

## 特性

- ✅ DMA接收，减少CPU中断负担（每帧1次中断 vs 每字节1次）
- ✅ IDLE中断检测帧结束，精确可靠
- ✅ 双缓冲机制，处理数据时不丢失新数据
- ✅ DMA非阻塞发送
- ✅ 兼容层支持旧代码平滑迁移

## 文件结构

```
HARDWARE/uart_driver/
├── uart_driver.h      # 驱动接口定义
├── uart_driver.c      # 驱动实现
├── uart_compat.h      # 兼容层接口
├── uart_compat.c      # 兼容层实现
└── README.md          # 本文档
```

## 快速开始

### 1. 初始化

```c
#include "uart_driver.h"

// 在main.c中初始化
uartDisplayInit(115200);  // USART2 - 显示屏
uartSlaveInit(115200);    // USART3 - 主从机
```

### 2. 修改中断处理函数

在 `stm32f10x_it.c` 中：

```c
#include "uart_driver.h"

// USART2中断处理 - 使用新驱动
void USART2_IRQHandler(void)
{
    uartIdleIrqHandler(&uartDisplayHandle);
}

// USART3中断处理 - 使用新驱动
void USART3_IRQHandler(void)
{
    uartIdleIrqHandler(&uartSlaveHandle);
}

// DMA1_Channel7中断 - USART2发送完成
void DMA1_Channel7_IRQHandler(void)
{
    uartDmaTxIrqHandler(&uartDisplayHandle);
}

// DMA1_Channel2中断 - USART3发送完成
void DMA1_Channel2_IRQHandler(void)
{
    uartDmaTxIrqHandler(&uartSlaveHandle);
}
```

### 3. 主循环处理

```c
// 在主循环中检查并处理接收数据
void main_loop(void)
{
    uint8_t* rxData;
    uint16_t rxLen;
    
    // 检查USART2是否有数据
    if (uartIsRxReady(&uartDisplayHandle))
    {
        if (uartGetRxData(&uartDisplayHandle, &rxData, &rxLen) == UART_OK)
        {
            // 处理ModBus数据
            processModbusFrame(rxData, rxLen);
            
            // 处理完毕，清除标志
            uartClearRxFlag(&uartDisplayHandle);
        }
    }
    
    // 检查USART3...
}
```

### 4. 发送数据

```c
uint8_t txBuf[10] = {0x01, 0x03, ...};

// 方式1: DMA非阻塞发送 (推荐)
if (uartSendDma(&uartDisplayHandle, txBuf, 10) == UART_BUSY)
{
    // 上次发送未完成，等待或使用阻塞发送
}

// 方式2: 阻塞发送 (兼容旧代码)
uartSendBlocking(&uartDisplayHandle, txBuf, 10);
```

---

## 兼容旧代码

如果需要最小化改动，使用兼容层：

### 1. 初始化

```c
#include "uart_compat.h"

// 替换旧初始化
// uart2_init(115200);  // 旧代码
uart2_init_new(115200);  // 新代码
```

### 2. 主循环

```c
// 在主循环开头调用同步函数
uartCompatSync();

// 然后可以继续使用旧的判断方式
if (U2_Compat.Recive_Ok_Flag)
{
    // 处理 U2_Compat.RX_Data, 长度 U2_Compat.RX_Length
    
    U2_ClearRxFlag();  // 清除标志
}
```

---

## DMA通道分配

| UART | 功能 | DMA通道 | 中断 |
|------|------|---------|------|
| USART2 | RX | DMA1_Channel6 | - |
| USART2 | TX | DMA1_Channel7 | DMA1_Channel7_IRQn |
| USART3 | RX | DMA1_Channel3 | - |
| USART3 | TX | DMA1_Channel2 | DMA1_Channel2_IRQn |

---

## 迁移步骤

### Phase 1: 添加新驱动（不影响旧代码）

1. 将 `uart_driver/` 目录添加到工程
2. 在Keil中添加源文件和包含路径
3. 编译验证无冲突

### Phase 2: 迁移USART2

1. 修改 `uart2_init()` 调用 `uart2_init_new()`
2. 修改 `USART2_IRQHandler` 使用新驱动
3. 添加 `DMA1_Channel7_IRQHandler`
4. 修改 `Union_ModBus2_Communication()` 使用新接口
5. 测试显示屏通信

### Phase 3: 迁移USART3

1. 同样步骤迁移USART3
2. 测试主从机通信

### Phase 4: 清理

1. 删除旧的 `usart2.c/usart3.c` 中的重复代码
2. 统一使用新驱动接口

---

## 注意事项

1. **中断优先级**: USART中断应高于或等于应用层任务
2. **缓冲区大小**: 默认256字节，可根据需要修改 `UART_RX_BUFFER_SIZE`
3. **DMA冲突**: 确保DMA通道未被其他外设占用
4. **IDLE检测**: 适用于帧间有间隔的协议（如ModBus）

---

## API参考

| 函数 | 说明 |
|------|------|
| `uartDriverInit()` | 初始化UART驱动 |
| `uartDisplayInit()` | 初始化USART2 |
| `uartSlaveInit()` | 初始化USART3 |
| `uartSendDma()` | DMA非阻塞发送 |
| `uartSendBlocking()` | 阻塞发送 |
| `uartIdleIrqHandler()` | IDLE中断处理 |
| `uartDmaTxIrqHandler()` | DMA发送完成中断处理 |
| `uartGetRxData()` | 获取接收数据 |
| `uartClearRxFlag()` | 清除接收标志 |
| `uartIsTxBusy()` | 查询发送状态 |
| `uartIsRxReady()` | 查询接收状态 |

---

## 版本历史

- **1.0** (2025-11-26): 初始版本

