# USART3 从机设备通信模块规格

## Purpose

USART3负责与从机设备（变频供水阀等）的ModBus RTU通信，实现设备控制和状态读取。
USART3 handles ModBus RTU communication with slave devices (variable frequency water supply valve, etc.) for device control and status reading.

## 模块信息

- **通信文件**: `HARDWARE/USART3/usart3.c`, `usart3.h`
- **驱动文件**: `HARDWARE/uart_driver/uart_driver.c`, `uart_driver.h`
- **中断处理**: `USER/stm32f10x_it.c`
- **版本**: 1.0

## 硬件配置

### 引脚配置

| 功能 | 引脚 | 模式 |
|------|------|------|
| TX | PB10 | 复用推挽输出 |
| RX | PB11 | 浮空输入 |

### 通信参数

| 参数 | 值 |
|------|-----|
| 波特率 | 9600 (可配置) |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 流控制 | 无 |

### DMA配置

| 功能 | DMA通道 | 标志位 |
|------|---------|--------|
| 接收 | DMA1_Channel3 | DMA1_FLAG_TC3 |
| 发送 | DMA1_Channel2 | DMA1_IT_TC2 |

### NVIC优先级

| 中断源 | 抢占优先级 | 子优先级 |
|--------|------------|----------|
| USART3_IRQn | 0 | 0 |
| DMA1_Channel2_IRQn | 3 | 0 |

## Requirements

### Requirement: DMA+IDLE接收模式

系统MUST使用DMA配合IDLE中断实现高效的帧接收。

#### Scenario: 正常帧接收

- **WHEN** 串口接收到完整ModBus帧
- **AND** 帧间隔≥1字节时间（检测到IDLE）
- **THEN** 触发IDLE中断
- **AND** 停止当前DMA传输
- **AND** 计算接收长度 = BUFFER_SIZE - DMA_CNDTR
- **AND** 切换到备用缓冲区
- **AND** 设置rxComplete标志
- **AND** 重新启动DMA接收

#### Scenario: 中断服务程序

```c
void USART3_IRQHandler(void)
{
    /* DMA + IDLE中断处理 */
    uartIdleIrqHandler(&uartSlaveHandle);
    
    /* 处理错误标志 */
    if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) == SET) {
        USART_ClearFlag(USART3, USART_FLAG_ORE);
        USART_ReceiveData(USART3);
    }
}
```

### Requirement: 数据发送机制

系统MUST支持DMA发送模式。

#### Scenario: DMA非阻塞发送

- **WHEN** 调用uartSendDma(&uartSlaveHandle, data, len)
- **AND** 当前txBusy == 0
- **THEN** 复制数据到发送缓冲区
- **AND** 设置txBusy = 1
- **AND** 配置DMA传输长度
- **AND** 启动DMA发送
- **AND** 立即返回

### Requirement: ModBus RTU主站协议

系统MUST实现ModBus RTU主站协议，主动轮询从机设备。

#### Scenario: 帧校验

- **WHEN** 接收到从机响应帧
- **THEN** 提取帧尾的CRC16校验码
- **AND** 计算数据部分的CRC16
- **AND** 校验码匹配则处理，否则丢弃

#### Scenario: 设备地址识别

- **WHEN** CRC校验通过
- **THEN** 解析设备地址（字节0）
- **AND** 地址=8 → 变频供水阀设备

### Requirement: 功能码03 - 读保持寄存器

系统MUST实现功能码03读取从机数据。

#### Scenario: 读取供水阀状态 (地址0x0000)

- **WHEN** 目标地址=8, 功能码=03, 数据地址=0x0000, 长度=1
- **THEN** 发送读取请求帧：[08 03 00 00 00 01 CRC]
- **AND** 等待从机响应
- **AND** 解析响应数据获取设备状态
- **AND** 如果状态值=0xEA，设置Water_Error_Code=OK

#### Scenario: 读取请求格式

```c
uint8 ModBus3_RTU_Read03(uint8 Target_Address, uint16 Data_Address, uint8 Data_Length)
{
    U3_Inf.TX_Data[0] = Target_Address;
    U3_Inf.TX_Data[1] = 0x03;
    U3_Inf.TX_Data[2] = Data_Address >> 8;
    U3_Inf.TX_Data[3] = Data_Address & 0xFF;
    U3_Inf.TX_Data[4] = Data_Length >> 8;
    U3_Inf.TX_Data[5] = Data_Length & 0xFF;
    checksum = ModBusCRC16(U3_Inf.TX_Data, 8);
    U3_Inf.TX_Data[6] = checksum >> 8;
    U3_Inf.TX_Data[7] = checksum & 0xFF;
    uartSendDma(&uartSlaveHandle, U3_Inf.TX_Data, 8);
}
```

### Requirement: 功能码06 - 写单寄存器

系统MUST实现功能码06写入单个寄存器。

#### Scenario: 写入供水阀开度 (地址0x0001)

- **WHEN** 目标地址=8, 功能码=06, 数据地址=0x0001
- **AND** 数据值=Water_Percent * 10 (0-1000)
- **THEN** 发送写入请求帧：[08 06 00 01 高字节 低字节 CRC]
- **AND** 等待从机确认响应

#### Scenario: 写入请求格式

```c
uint8 ModBus3_RTU_Write06(uint8 Target_Address, uint16 Data_Address, uint16 Data16)
{
    U3_Inf.TX_Data[0] = Target_Address;
    U3_Inf.TX_Data[1] = 0x06;
    U3_Inf.TX_Data[2] = Data_Address >> 8;
    U3_Inf.TX_Data[3] = Data_Address & 0xFF;
    U3_Inf.TX_Data[4] = Data16 >> 8;
    U3_Inf.TX_Data[5] = Data16 & 0xFF;
    checksum = ModBusCRC16(U3_Inf.TX_Data, 8);
    U3_Inf.TX_Data[6] = checksum >> 8;
    U3_Inf.TX_Data[7] = checksum & 0xFF;
    uartSendDma(&uartSlaveHandle, U3_Inf.TX_Data, 8);
}
```

### Requirement: 变频供水阀通信

系统MUST支持与变频供水阀设备的通信。

#### Scenario: 轮询周期

- **WHEN** 变频功能启用 (Water_BianPin_Enabled=1)
- **AND** 每200ms触发一次 (WaterAJ_Flag)
- **THEN** 按轮询序列发送请求
- **AND** 序列：读状态(5次)→写开度(1次)→循环

#### Scenario: 读取供水阀状态

- **WHEN** Jump_Index=0
- **THEN** 调用ModBus3_RTU_Read03(8, 0x0000, 1)
- **AND** 读取当前故障状态

#### Scenario: 写入目标开度

- **WHEN** Jump_Index=1
- **AND** Water_Percent ≤ 100
- **THEN** 计算Percent = Water_Percent * 10
- **AND** 调用ModBus3_RTU_Write06(8, 0x0001, Percent)

### Requirement: 通信超时处理

系统MUST检测通信超时并报警。

#### Scenario: 供水设备通信超时

- **WHEN** waterSend_Flag=OK
- **AND** waterSend_Count ≥ 10 (10次无响应)
- **THEN** 设置Error_Code = Error19_SupplyWater_UNtalk
- **AND** 表示供水设备通信失败

#### Scenario: 通信恢复

- **WHEN** 收到有效响应帧
- **AND** 地址匹配且CRC校验通过
- **THEN** 清除waterSend_Flag
- **AND** 重置waterSend_Count = 0

### Requirement: 故障状态检测

系统MUST检测从机设备故障状态。

#### Scenario: 供水阀故障检测

- **WHEN** 功能码03响应
- **AND** 数据长度=2
- **AND** 数据值=0x00EA
- **THEN** 设置Water_Error_Code = OK
- **AND** 设置Error_Code = Error18_SupplyWater_Error

## 通信设备列表

| 设备地址 | 设备类型 | 功能 |
|----------|----------|------|
| 8 | 变频供水阀 | 控制供水开度、读取故障状态 |

## 寄存器地址定义

### 变频供水阀 (地址8)

| 寄存器地址 | 功能 | 读/写 | 数据格式 |
|------------|------|-------|----------|
| 0x0000 | 设备状态 | 只读 | uint16, 0xEA=故障 |
| 0x0001 | 目标开度 | 读写 | uint16, 0-1000 (0.1%精度) |

## 主循环处理流程

```c
// 主循环中调用
void ModBus_Uart3_LocalRX_Communication(void)
{
    // 1. 检查通信超时
    if (Sys_Admin.Water_BianPin_Enabled) {
        if (sys_flag.waterSend_Count >= 10)
            sys_flag.Error_Code = Error19_SupplyWater_UNtalk;
    }
    
    // 2. 检查DMA接收就绪
    if (uartIsRxReady(&uartSlaveHandle)) {
        uartGetRxData(&uartSlaveHandle, &rxData, &rxLen);
        // 复制数据到U3_Inf兼容结构
        U3_Inf.Recive_Ok_Flag = 1;
    }
    
    // 3. 处理接收数据
    if (U3_Inf.Recive_Ok_Flag) {
        U3_Inf.Recive_Ok_Flag = 0;
        
        // CRC校验
        if (checksum == ModBusCRC16(...)) {
            Target_Address = U3_Inf.RX_Data[0];
            if (Target_Address == 8) {
                // 处理变频供水阀响应
                sys_flag.waterSend_Flag = FALSE;
                sys_flag.waterSend_Count = 0;
                // 解析故障状态
            }
        }
        
        // 清空缓冲区
        memset(U3_Inf.RX_Data, 0, 200);
        uartClearRxFlag(&uartSlaveHandle);
    }
}

// 发送任务调度
uint8 Modbus3_UnionTx_Communication(void)
{
    if (Sys_Admin.Water_BianPin_Enabled) {
        if (sys_flag.Special_100msFlag) {
            FuNiSen_Read_WaterDevice_Function();
            sys_flag.Special_100msFlag = 0;
        }
    }
}
```

## API参考

### 初始化

| 函数 | 说明 |
|------|------|
| `uart3_init(bound)` | 旧驱动初始化 |
| `uartSlaveInit(baudRate)` | 新驱动初始化 |

### 数据收发

| 函数 | 说明 |
|------|------|
| `uartSendDma(&uartSlaveHandle, data, len)` | DMA发送 |
| `uartIsRxReady(&uartSlaveHandle)` | 检查接收就绪 |
| `uartGetRxData(&uartSlaveHandle, &data, &len)` | 获取接收数据 |
| `uartClearRxFlag(&uartSlaveHandle)` | 清除接收标志 |

### 协议处理

| 函数 | 说明 |
|------|------|
| `ModBus_Uart3_LocalRX_Communication()` | 接收处理主函数 |
| `Modbus3_UnionTx_Communication()` | 发送调度函数 |
| `FuNiSen_Read_WaterDevice_Function()` | 供水阀轮询函数 |
| `ModBus3_RTU_Read03()` | 发送03读请求 |
| `ModBus3_RTU_Write06()` | 发送06写请求 |

## 与USART2对比

| 特性 | USART2 (显示屏) | USART3 (从机设备) |
|------|-----------------|-------------------|
| 角色 | ModBus从站 | ModBus主站 |
| 通信方向 | 被动响应 | 主动轮询 |
| 驱动句柄 | uartDisplayHandle | uartSlaveHandle |
| TX引脚 | PA2 | PB10 |
| RX引脚 | PA3 | PB11 |
| TX DMA | DMA1_Channel7 | DMA1_Channel2 |
| RX DMA | DMA1_Channel6 | DMA1_Channel3 |
| 主要功能码 | 03, 10 | 03, 06 |
| 典型设备 | LCD10, LCD4013 | 变频供水阀 |

## 版本历史

### v1.0 (2025-11-27)

- ✅ 创建串口3通信规格文档
- ✅ 文档化DMA+IDLE接收机制
- ✅ 文档化ModBus主站协议处理
- ✅ 文档化功能码03/06实现
- ✅ 文档化变频供水阀通信协议
- ✅ 文档化通信超时和故障检测

