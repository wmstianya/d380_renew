/**
 * @file    ADS1220.c
 * @brief   ADS1220 24位ADC驱动模块实现
 * @author  Refactored
 * @date    2024
 * @version 2.0
 * 
 * @details 重构内容:
 *          - DWT周期计数器精确微秒延迟
 *          - 统一SPI通信(支持4种模式)
 *          - 寄存器读-改-写模式
 *          - 超时保护机制
 */

#include "stm32f10x.h"
#include "ADS1220.h"

/*============================================================================*/
/*                              全局设备句柄                                   */
/*============================================================================*/

Ads1220Handle_t ads1220Handle = {
    {GPIOE, GPIOE, GPIOE, GPIOE, GPIOE},           /* port[5] */
    {GPIO_Pin_11, GPIO_Pin_12, GPIO_Pin_15, GPIO_Pin_14, GPIO_Pin_13}, /* pin[5] */
    ADS1220_SPI_MODE1,    /* spiMode: 默认MODE1 (CPOL=0, CPHA=1) */
    ADS1220_SPI_DELAY_US, /* spiDelayUs */
    {0, 0, 0, 0},         /* regCache[4] */
    0                     /* isInitialized */
};

/*============================================================================*/
/*                              DWT寄存器定义                                  */
/*============================================================================*/

#define DWT_CTRL    (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t*)0xE0001004)
#define DEM_CR      (*(volatile uint32_t*)0xE000EDFC)

#define DEM_CR_TRCENA       (1 << 24)
#define DWT_CTRL_CYCCNTENA  (1 << 0)

static uint8_t dwtInitialized = 0;

/*============================================================================*/
/*                              外部时间函数                                   */
/*============================================================================*/

extern volatile uint32_t sysTickCount;

static uint32_t ads1220GetTickMs(void)
{
    return sysTickCount;
}

/*============================================================================*/
/*                              DWT延迟实现                                    */
/*============================================================================*/

/**
 * @brief  初始化DWT周期计数器
 */
void ads1220DwtInit(void)
{
    if (dwtInitialized)
        return;
    
    DEM_CR |= DEM_CR_TRCENA;        /* 使能DWT模块 */
    DWT_CYCCNT = 0;                 /* 清零计数器 */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA; /* 使能周期计数器 */
    
    dwtInitialized = 1;
}

/**
 * @brief  DWT微秒延迟
 * @param  us: 延迟微秒数
 * @note   72MHz主频: 1us = 72 cycles
 */
void ads1220DwtDelayUs(uint32_t us)
{
    uint32_t startCycle;
    uint32_t delayCycle;
    
    if (!dwtInitialized)
        ads1220DwtInit();
    
    startCycle = DWT_CYCCNT;
    delayCycle = us * (SystemCoreClock / 1000000);
    
    while ((DWT_CYCCNT - startCycle) < delayCycle)
    {
        /* 空等待 */
    }
}

/*============================================================================*/
/*                              统一SPI通信                                    */
/*============================================================================*/

/**
 * @brief  SPI发送单字节(支持4种模式)
 * @param  handle: 设备句柄
 * @param  data: 发送数据
 */
static void spiSendByte(Ads1220Handle_t* handle, uint8_t data)
{
    uint8_t i;
    uint8_t cpol = (handle->spiMode >> 1) & 0x01;
    uint8_t cpha = handle->spiMode & 0x01;
    
    /* 设置空闲电平 */
    if (cpol)
        GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
    else
        GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
    
    for (i = 0; i < 8; i++)
    {
        if (cpha == 0)
        {
            /* MODE 0/2: 数据在第一个边沿锁存 */
            if (data & 0x80)
                GPIO_WriteBit(handle->port[ADS1220_DIN], handle->pin[ADS1220_DIN], Bit_SET);
            else
                GPIO_WriteBit(handle->port[ADS1220_DIN], handle->pin[ADS1220_DIN], Bit_RESET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
            
            /* 第一个边沿 */
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
            
            /* 第二个边沿 */
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
        }
        else
        {
            /* MODE 1/3: 数据在第二个边沿锁存 */
            /* 第一个边沿 */
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            
            if (data & 0x80)
                GPIO_WriteBit(handle->port[ADS1220_DIN], handle->pin[ADS1220_DIN], Bit_SET);
            else
                GPIO_WriteBit(handle->port[ADS1220_DIN], handle->pin[ADS1220_DIN], Bit_RESET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
            
            /* 第二个边沿 */
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
        }
        data <<= 1;
    }
}

/**
 * @brief  SPI接收单字节(支持4种模式)
 * @param  handle: 设备句柄
 * @return 接收数据
 */
static uint8_t spiReceiveByte(Ads1220Handle_t* handle)
{
    uint8_t i;
    uint8_t data = 0;
    uint8_t cpol = (handle->spiMode >> 1) & 0x01;
    uint8_t cpha = handle->spiMode & 0x01;
    
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        
        if (cpha == 0)
        {
            /* MODE 0/2 */
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
            
            if (GPIO_ReadInputDataBit(handle->port[ADS1220_DOUT], handle->pin[ADS1220_DOUT]))
                data |= 0x01;
            
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
        }
        else
        {
            /* MODE 1/3 */
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            
            ads1220DwtDelayUs(handle->spiDelayUs);
            
            if (cpol)
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_SET);
            else
                GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
            
            if (GPIO_ReadInputDataBit(handle->port[ADS1220_DOUT], handle->pin[ADS1220_DOUT]))
                data |= 0x01;
            
            ads1220DwtDelayUs(handle->spiDelayUs);
        }
    }
    
    return data;
}

/**
 * @brief  CS引脚控制
 * @param  handle: 设备句柄
 * @param  state: ADS1220_ENABLE/ADS1220_DISABLE
 */
static void spiCsControl(Ads1220Handle_t* handle, uint8_t state)
{
    if (state == ADS1220_ENABLE)
        GPIO_WriteBit(handle->port[ADS1220_CS], handle->pin[ADS1220_CS], Bit_RESET);
    else
        GPIO_WriteBit(handle->port[ADS1220_CS], handle->pin[ADS1220_CS], Bit_SET);
}

/*============================================================================*/
/*                              新API实现                                      */
/*============================================================================*/

/**
 * @brief  设置SPI模式
 * @param  handle: 设备句柄
 * @param  mode: SPI模式(0-3)
 */
void ads1220SetSpiMode(Ads1220Handle_t* handle, uint8_t mode)
{
    if (mode <= 3)
    {
        handle->spiMode = mode;
    }
}

/**
 * @brief  硬件初始化
 * @param  handle: 设备句柄
 */
void ads1220HwInit(Ads1220Handle_t* handle)
{
    uint8_t i;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 初始化DWT */
    ads1220DwtInit();
    
    /* 使能GPIO时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    
    /* 配置输出引脚: CS, SCLK, DIN */
    for (i = 0; i < 3; i++)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_InitStructure.GPIO_Pin = handle->pin[i];
        GPIO_Init(handle->port[i], &GPIO_InitStructure);
    }
    
    /* 配置输入引脚: DOUT, DRDY */
    for (i = 3; i < 5; i++)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Pin = handle->pin[i];
        GPIO_Init(handle->port[i], &GPIO_InitStructure);
    }
    
    /* 默认状态: CS高, CLK低 */
    GPIO_WriteBit(handle->port[ADS1220_CS], handle->pin[ADS1220_CS], Bit_SET);
    GPIO_WriteBit(handle->port[ADS1220_SCLK], handle->pin[ADS1220_SCLK], Bit_RESET);
    
    handle->isInitialized = 1;
}

/**
 * @brief  硬件配置
 * @param  handle: 设备句柄
 */
void ads1220HwConfig(Ads1220Handle_t* handle)
{
    unsigned temp;
    
    /* REG0: AIN1-AIN0, GAIN=16 */
    temp = ADS1220_MUX_1_0 | ADS1220_GAIN_16;
    ADS1220WriteRegister(ADS1220_REG0, 1, &temp);
    handle->regCache[0] = (uint8_t)temp;
    
    /* REG1: 连续转换 */
    temp = ADS1220_CC;
    ADS1220WriteRegister(ADS1220_REG1, 1, &temp);
    handle->regCache[1] = (uint8_t)temp;
    
    /* REG2: 外部参考, 50/60Hz滤波, IDAC=500uA */
    temp = ADS1220_IDAC_500 | 0x50;
    ADS1220WriteRegister(ADS1220_REG2, 1, &temp);
    handle->regCache[2] = (uint8_t)temp;
    
    /* REG3: IDAC1->AIN2, IDAC2->AIN3 */
    temp = ADS1220_IDAC1_AIN2 | ADS1220_IDAC2_AIN3;
    ADS1220WriteRegister(ADS1220_REG3, 1, &temp);
    handle->regCache[3] = (uint8_t)temp;
}

/**
 * @brief  等待数据就绪(带超时)
 * @param  handle: 设备句柄
 * @param  timeoutMs: 超时时间(ms)
 * @return 状态码
 */
Ads1220Status_e ads1220WaitDataReady(Ads1220Handle_t* handle, uint32_t timeoutMs)
{
    uint32_t startTick = ads1220GetTickMs();
    
    while (GPIO_ReadInputDataBit(handle->port[ADS1220_DRDY], handle->pin[ADS1220_DRDY]) != 0)
    {
        if ((ads1220GetTickMs() - startTick) > timeoutMs)
        {
            return ADS1220_ERR_TIMEOUT;
        }
    }
    
    return ADS1220_OK;
}

/**
 * @brief  安全读取数据(带超时)
 * @param  handle: 设备句柄
 * @param  data: 输出数据指针
 * @param  timeoutMs: 超时时间(ms)
 * @return 状态码
 */
Ads1220Status_e ads1220ReadDataSafe(Ads1220Handle_t* handle, int32_t* data, uint32_t timeoutMs)
{
    Ads1220Status_e status;
    
    status = ads1220WaitDataReady(handle, timeoutMs);
    if (status != ADS1220_OK)
    {
        return status;
    }
    
    *data = ADS1220ReadData();
    
    return ADS1220_OK;
}

/**
 * @brief  同步寄存器缓存
 * @param  handle: 设备句柄
 */
void ads1220SyncRegCache(Ads1220Handle_t* handle)
{
    unsigned temp;
    uint8_t i;
    
    for (i = 0; i < 4; i++)
    {
        ADS1220ReadRegister(i, 1, &temp);
        handle->regCache[i] = (uint8_t)temp;
    }
}

/**
 * @brief  设置寄存器位域(读-改-写模式)
 * @param  handle: 设备句柄
 * @param  reg: 寄存器索引(0-3)
 * @param  mask: 位掩码
 * @param  value: 新值
 * @return 状态码
 */
Ads1220Status_e ads1220SetRegBits(Ads1220Handle_t* handle, uint8_t reg, uint8_t mask, uint8_t value)
{
    unsigned temp;
    
    if (reg > 3)
    {
        return ADS1220_ERR_PARAM;
    }
    
    /* 读取当前值 */
    ADS1220ReadRegister(reg, 1, &temp);
    
    /* 修改指定位 */
    temp = (temp & ~mask) | (value & mask);
    
    /* 写回 */
    ADS1220WriteRegister(reg, 1, &temp);
    
    /* 更新缓存 */
    handle->regCache[reg] = (uint8_t)temp;
    
    return ADS1220_OK;
}

/**
 * @brief  获取寄存器位域
 * @param  handle: 设备句柄
 * @param  reg: 寄存器索引
 * @param  mask: 位掩码
 * @param  value: 输出值指针
 * @return 状态码
 */
Ads1220Status_e ads1220GetRegBits(Ads1220Handle_t* handle, uint8_t reg, uint8_t mask, uint8_t* value)
{
    unsigned temp;
    
    if (reg > 3)
    {
        return ADS1220_ERR_PARAM;
    }
    
    ADS1220ReadRegister(reg, 1, &temp);
    *value = (uint8_t)(temp & mask);
    
    return ADS1220_OK;
}

/*============================================================================*/
/*                              便捷配置函数                                   */
/*============================================================================*/

Ads1220Status_e ads1220SetMux(Ads1220Handle_t* handle, uint8_t mux)
{
    return ads1220SetRegBits(handle, ADS1220_REG0, ADS1220_REG0_MUX_MASK, mux);
}

Ads1220Status_e ads1220SetGainValue(Ads1220Handle_t* handle, Ads1220Gain_e gain)
{
    return ads1220SetRegBits(handle, ADS1220_REG0, ADS1220_REG0_GAIN_MASK, (uint8_t)gain);
}

Ads1220Status_e ads1220SetDataRateValue(Ads1220Handle_t* handle, uint8_t dr)
{
    return ads1220SetRegBits(handle, ADS1220_REG1, ADS1220_REG1_DR_MASK, dr);
}

Ads1220Status_e ads1220SetPgaBypass(Ads1220Handle_t* handle, uint8_t bypass)
{
    return ads1220SetRegBits(handle, ADS1220_REG0, ADS1220_REG0_PGA_MASK, bypass ? 0x01 : 0x00);
}

Ads1220Status_e ads1220SetVref(Ads1220Handle_t* handle, uint8_t vref)
{
    return ads1220SetRegBits(handle, ADS1220_REG2, ADS1220_REG2_VREF_MASK, vref);
}

Ads1220Status_e ads1220SetIdac(Ads1220Handle_t* handle, uint8_t idac)
{
    return ads1220SetRegBits(handle, ADS1220_REG2, ADS1220_REG2_IDAC_MASK, idac);
}

/*============================================================================*/
/*                              兼容旧API实现                                  */
/*============================================================================*/

/**
 * @brief  初始化(兼容旧API)
 */
void ADS1220Init(void)
{
    ads1220HwInit(&ads1220Handle);
}

/**
 * @brief  等待数据就绪(兼容旧API,已修复添加超时)
 * @param  Timeout: 超时时间(ms), 0表示使用默认值
 * @return 状态码
 */
int ADS1220WaitForDataReady(int Timeout)
{
    uint32_t timeoutMs = (Timeout > 0) ? (uint32_t)Timeout : ADS1220_TIMEOUT_DEFAULT;
    
    return (int)ads1220WaitDataReady(&ads1220Handle, timeoutMs);
}

/**
 * @brief  CS控制(兼容旧API)
 */
void ADS1220CsStatus(uint8_t state)
{
    spiCsControl(&ads1220Handle, state);
}

/**
 * @brief  发送字节(兼容旧API)
 */
void ADS1220SendByte(unsigned char cData)
{
    spiSendByte(&ads1220Handle, cData);
}

/**
 * @brief  接收字节(兼容旧API)
 */
unsigned char ADS1220ReceiveByte(void)
{
    return spiReceiveByte(&ads1220Handle);
}

/**
 * @brief  读取转换数据
 * @return 24位有符号数据
 */
long ADS1220ReadData(void)
{
    long Data;
    
    spiCsControl(&ads1220Handle, ADS1220_ENABLE);
    spiSendByte(&ads1220Handle, ADS1220_CMD_RDATA);
    
    Data = spiReceiveByte(&ads1220Handle);
    Data = (Data << 8) | spiReceiveByte(&ads1220Handle);
    Data = (Data << 8) | spiReceiveByte(&ads1220Handle);
    
    /* 符号扩展 */
    if (Data & 0x800000)
        Data |= 0xFF000000;
    
    spiCsControl(&ads1220Handle, ADS1220_DISABLE);
    
    return Data;
}

/**
 * @brief  读取寄存器
 */
void ADS1220ReadRegister(int StartAddress, int NumRegs, unsigned* pData)
{
    int i;
    
    spiCsControl(&ads1220Handle, ADS1220_ENABLE);
    spiSendByte(&ads1220Handle, ADS1220_CMD_RREG | (((StartAddress << 2) & 0x0C) | ((NumRegs - 1) & 0x03)));
    
    for (i = 0; i < NumRegs; i++)
    {
        *pData++ = spiReceiveByte(&ads1220Handle);
    }
    
    spiCsControl(&ads1220Handle, ADS1220_DISABLE);
}

/**
 * @brief  写入寄存器
 */
void ADS1220WriteRegister(int StartAddress, int NumRegs, unsigned* pData)
{
    int i;
    
    spiCsControl(&ads1220Handle, ADS1220_ENABLE);
    spiSendByte(&ads1220Handle, ADS1220_CMD_WREG | (((StartAddress << 2) & 0x0C) | ((NumRegs - 1) & 0x03)));
    
    for (i = 0; i < NumRegs; i++)
    {
        spiSendByte(&ads1220Handle, *pData++);
    }
    
    spiCsControl(&ads1220Handle, ADS1220_DISABLE);
}

/**
 * @brief  发送复位命令
 */
void ADS1220SendResetCommand(void)
{
    spiCsControl(&ads1220Handle, ADS1220_ENABLE);
    spiSendByte(&ads1220Handle, ADS1220_CMD_RESET);
    spiCsControl(&ads1220Handle, ADS1220_DISABLE);
}

/**
 * @brief  发送启动/同步命令
 */
void ADS1220SendStartCommand(void)
{
    spiCsControl(&ads1220Handle, ADS1220_ENABLE);
    spiSendByte(&ads1220Handle, ADS1220_CMD_SYNC);
    spiCsControl(&ads1220Handle, ADS1220_DISABLE);
}

/**
 * @brief  发送关机命令
 */
void ADS1220SendShutdownCommand(void)
{
    spiCsControl(&ads1220Handle, ADS1220_ENABLE);
    spiSendByte(&ads1220Handle, ADS1220_CMD_SHUTDOWN);
    spiCsControl(&ads1220Handle, ADS1220_DISABLE);
}

/**
 * @brief  配置(兼容旧API)
 */
void ADS1220Config(void)
{
    ads1220HwConfig(&ads1220Handle);
}

/*============================================================================*/
/*                              修复后的Set函数(读-改-写模式)                   */
/*============================================================================*/

int ADS1220SetChannel(int Mux)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG0, ADS1220_REG0_MUX_MASK, (uint8_t)Mux);
}

void ADS1220SetGain(ADS1220_Gain Gain)
{
    ads1220SetRegBits(&ads1220Handle, ADS1220_REG0, ADS1220_REG0_GAIN_MASK, (uint8_t)Gain);
}

int ADS1220SetPGABypass(int Bypass)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG0, ADS1220_REG0_PGA_MASK, Bypass ? 0x01 : 0x00);
}

int ADS1220SetDataRate(int DataRate)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG1, ADS1220_REG1_DR_MASK, (uint8_t)DataRate);
}

int ADS1220SetClockMode(int ClockMode)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG1, ADS1220_REG1_MODE_MASK, (uint8_t)ClockMode);
}

int ADS1220SetPowerDown(int PowerDown)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG1, ADS1220_REG1_CM_MASK, PowerDown ? 0x04 : 0x00);
}

int ADS1220SetTemperatureMode(int TempMode)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG1, ADS1220_REG1_TS_MASK, TempMode ? 0x02 : 0x00);
}

int ADS1220SetBurnOutSource(int BurnOut)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG1, ADS1220_REG1_BCS_MASK, BurnOut ? 0x01 : 0x00);
}

int ADS1220SetVoltageReference(int VoltageRef)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG2, ADS1220_REG2_VREF_MASK, (uint8_t)VoltageRef);
}

int ADS1220Set50_60Rejection(int Rejection)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG2, ADS1220_REG2_FIR_MASK, (uint8_t)Rejection);
}

int ADS1220SetLowSidePowerSwitch(int PowerSwitch)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG2, ADS1220_REG2_PSW_MASK, PowerSwitch ? 0x08 : 0x00);
}

int ADS1220SetCurrentDACOutput(int CurrentOutput)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG2, ADS1220_REG2_IDAC_MASK, (uint8_t)CurrentOutput);
}

int ADS1220SetIDACRouting(int IDACRoute)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG3, 
                                   ADS1220_REG3_I1MUX_MASK | ADS1220_REG3_I2MUX_MASK, 
                                   (uint8_t)IDACRoute);
}

int ADS1220SetDRDYMode(int DRDYMode)
{
    return (int)ads1220SetRegBits(&ads1220Handle, ADS1220_REG3, ADS1220_REG3_DRDYM_MASK, DRDYMode ? 0x02 : 0x00);
}

/*============================================================================*/
/*                              Get函数                                        */
/*============================================================================*/

int ADS1220GetChannel(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG0, 1, &temp);
    return (temp >> 4);
}

int ADS1220GetGain(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG0, 1, &temp);
    return ((temp & 0x0E) >> 1);
}

int ADS1220GetPGABypass(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG0, 1, &temp);
    return (temp & 0x01);
}

int ADS1220GetDataRate(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG1, 1, &temp);
    return (temp >> 5);
}

int ADS1220GetClockMode(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG1, 1, &temp);
    return ((temp & 0x18) >> 3);
}

int ADS1220GetPowerDown(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG1, 1, &temp);
    return ((temp & 0x04) >> 2);
}

int ADS1220GetTemperatureMode(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG1, 1, &temp);
    return ((temp & 0x02) >> 1);
}

int ADS1220GetBurnOutSource(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG1, 1, &temp);
    return (temp & 0x01);
}

int ADS1220GetVoltageReference(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG2, 1, &temp);
    return (temp >> 6);
}

int ADS1220Get50_60Rejection(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG2, 1, &temp);
    return ((temp & 0x30) >> 4);
}

int ADS1220GetLowSidePowerSwitch(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG2, 1, &temp);
    return ((temp & 0x08) >> 3);
}

int ADS1220GetCurrentDACOutput(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG2, 1, &temp);
    return (temp & 0x07);
}

int ADS1220GetIDACRouting(int WhichOne)
{
    unsigned temp;
    
    if (WhichOne > 1)
        return ADS1220_ERROR;
    
    ADS1220ReadRegister(ADS1220_REG3, 1, &temp);
    
    if (WhichOne)
        return ((temp & 0x1C) >> 2);
    else
        return (temp >> 5);
}

int ADS1220GetDRDYMode(void)
{
    unsigned temp;
    ADS1220ReadRegister(ADS1220_REG3, 1, &temp);
    return ((temp & 0x02) >> 1);
}
