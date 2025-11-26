# ADS1220 ADC驱动模块规格

## 概述

ADS1220是TI的24位高精度ADC芯片，通过软件SPI与STM32F103通信。本模块负责ADC的初始化、配置和数据读取。

## 模块信息

- **文件**: `HARDWARE/ADS1220/ADS1220.c`, `HARDWARE/ADS1220/ADS1220.h`
- **版本**: 2.0 (重构版)
- **依赖**: STM32F10x标准库, sysTickCount(系统毫秒计数器)

## 硬件接口

### GPIO配置

| 引脚 | 端口 | 功能 | 模式 |
|-----|------|------|------|
| PE11 | CS | 片选 | 推挽输出 |
| PE12 | SCLK | SPI时钟 | 推挽输出 |
| PE15 | DIN | MOSI | 推挽输出 |
| PE14 | DOUT | MISO | 上拉输入 |
| PE13 | DRDY | 数据就绪 | 上拉输入 |

### SPI配置

- **通信方式**: 软件SPI (Bit-Bang)
- **默认模式**: MODE1 (CPOL=0, CPHA=1)
- **时序延迟**: 5us (DWT精确延迟)
- **支持模式**: MODE0-MODE3运行时可切换

## 核心数据结构

### 设备句柄

```c
typedef struct {
    GPIO_TypeDef* port[ADS1220_GPIO_MAX];  // GPIO端口数组
    uint16_t pin[ADS1220_GPIO_MAX];        // GPIO引脚数组
    uint8_t spiMode;                        // SPI模式(0-3)
    uint8_t spiDelayUs;                     // SPI时序延迟(us)
    uint8_t regCache[4];                    // 寄存器缓存
    uint8_t isInitialized;                  // 初始化标志
} Ads1220Handle_t;
```

### 错误码

```c
typedef enum {
    ADS1220_OK = 0,           // 成功
    ADS1220_ERR_TIMEOUT = -1, // 超时
    ADS1220_ERR_PARAM = -2,   // 参数错误
    ADS1220_ERR_NOT_INIT = -3 // 未初始化
} Ads1220Status_e;
```

## 寄存器映射

### REG0 (0x00)

| 位 | 名称 | 掩码 | 说明 |
|---|------|------|------|
| 7:4 | MUX | 0xF0 | 输入通道选择 |
| 3:1 | GAIN | 0x0E | 增益设置 |
| 0 | PGA_BYPASS | 0x01 | PGA旁路 |

### REG1 (0x01)

| 位 | 名称 | 掩码 | 说明 |
|---|------|------|------|
| 7:5 | DR | 0xE0 | 数据速率 |
| 4:3 | MODE | 0x18 | 工作模式 |
| 2 | CM | 0x04 | 转换模式 |
| 1 | TS | 0x02 | 温度传感器 |
| 0 | BCS | 0x01 | 烧断电流源 |

### REG2 (0x02)

| 位 | 名称 | 掩码 | 说明 |
|---|------|------|------|
| 7:6 | VREF | 0xC0 | 参考电压 |
| 5:4 | FIR | 0x30 | 滤波器 |
| 3 | PSW | 0x08 | 低边开关 |
| 2:0 | IDAC | 0x07 | IDAC电流 |

### REG3 (0x03)

| 位 | 名称 | 掩码 | 说明 |
|---|------|------|------|
| 7:5 | I1MUX | 0xE0 | IDAC1路由 |
| 4:2 | I2MUX | 0x1C | IDAC2路由 |
| 1 | DRDYM | 0x02 | DRDY模式 |

## 关键功能

### 1. DWT精确延迟

使用Cortex-M3的DWT周期计数器实现精确微秒延迟：

```c
void ads1220DwtInit(void);           // 初始化DWT模块
void ads1220DwtDelayUs(uint32_t us); // 微秒延迟
```

**实现原理**:
- 72MHz主频下，1us = 72个CPU周期
- 读取DWT_CYCCNT计数器实现精确计时
- 无中断开销，精度优于软件循环

### 2. 统一SPI通信

支持4种SPI模式，运行时可切换：

```c
void ads1220SetSpiMode(Ads1220Handle_t* handle, uint8_t mode);
```

**模式定义**:
- MODE0: CPOL=0, CPHA=0
- MODE1: CPOL=0, CPHA=1 (默认)
- MODE2: CPOL=1, CPHA=0
- MODE3: CPOL=1, CPHA=1

### 3. 寄存器读-改-写

所有Set函数采用读-改-写模式，只修改指定位：

```c
Ads1220Status_e ads1220SetRegBits(Ads1220Handle_t* handle, 
                                   uint8_t reg, 
                                   uint8_t mask, 
                                   uint8_t value);
```

**流程**:
1. 读取当前寄存器值
2. 清除mask指定的位
3. 设置新值
4. 写回寄存器
5. 更新缓存

### 4. 超时保护

数据就绪等待函数带超时检测：

```c
Ads1220Status_e ads1220WaitDataReady(Ads1220Handle_t* handle, 
                                      uint32_t timeoutMs);

Ads1220Status_e ads1220ReadDataSafe(Ads1220Handle_t* handle, 
                                     int32_t* data, 
                                     uint32_t timeoutMs);
```

## API参考

### 新API

| 函数 | 说明 |
|-----|------|
| `ads1220DwtInit()` | 初始化DWT延迟模块 |
| `ads1220DwtDelayUs(us)` | 精确微秒延迟 |
| `ads1220HwInit(handle)` | 硬件初始化 |
| `ads1220HwConfig(handle)` | 默认配置 |
| `ads1220WaitDataReady(handle, timeout)` | 等待数据就绪 |
| `ads1220ReadDataSafe(handle, &data, timeout)` | 安全读取数据 |
| `ads1220SetRegBits(handle, reg, mask, value)` | 设置寄存器位 |
| `ads1220SetMux(handle, mux)` | 设置输入通道 |
| `ads1220SetGainValue(handle, gain)` | 设置增益 |
| `ads1220SetDataRateValue(handle, dr)` | 设置数据速率 |

### 兼容旧API

所有旧API保持不变，内部调用新实现：

| 函数 | 说明 |
|-----|------|
| `ADS1220Init()` | 初始化(调用ads1220HwInit) |
| `ADS1220Config()` | 配置(调用ads1220HwConfig) |
| `ADS1220WaitForDataReady(timeout)` | 等待就绪(带超时) |
| `ADS1220SetGain(gain)` | 设置增益(读-改-写) |
| `ADS1220SetChannel(mux)` | 设置通道(读-改-写) |
| `ADS1220ReadData()` | 读取数据 |

## 使用示例

### 基本使用(兼容旧代码)

```c
ADS1220Init();
ADS1220Config();
ADS1220SendStartCommand();

while (1) {
    if (ADS1220WaitForDataReady(500) == ADS1220_OK) {
        long data = ADS1220ReadData();
        // 处理数据
    }
}
```

### 使用新API

```c
int32_t adcData;
Ads1220Status_e status;

ads1220HwInit(&ads1220Handle);
ads1220HwConfig(&ads1220Handle);
ADS1220SendStartCommand();

// 只修改增益，不影响其他配置
ads1220SetGainValue(&ads1220Handle, ADS1220_GAIN_X8);

// 安全读取(带超时)
status = ads1220ReadDataSafe(&ads1220Handle, &adcData, 500);
if (status == ADS1220_OK) {
    // 处理数据
} else if (status == ADS1220_ERR_TIMEOUT) {
    // 超时处理
}
```

## 重构历史

### v2.0 (2024)

- ✅ 使用DWT周期计数器替代软件循环延迟
- ✅ 统一SPI通信代码，消除4套重复实现
- ✅ 寄存器操作改为读-改-写模式
- ✅ ADS1220WaitForDataReady()添加超时保护
- ✅ 引入设备句柄结构消除硬编码
- ✅ 添加寄存器位掩码定义
- ✅ 完全兼容旧API

### v1.0 (原始版本)

- 软件循环延迟
- 4套SPI模式代码(条件编译)
- 寄存器直接覆盖写入
- 无超时保护

## 注意事项

1. **DWT初始化**: `ads1220DwtInit()`在`ads1220HwInit()`中自动调用
2. **sysTickCount**: 需要外部提供1ms递增的系统计数器
3. **SPI模式**: 默认MODE1，如需更改调用`ads1220SetSpiMode()`
4. **超时值**: 根据数据速率设置，20SPS时建议≥100ms

