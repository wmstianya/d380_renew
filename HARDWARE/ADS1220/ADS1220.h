/**
 * @file    ADS1220.h
 * @brief   ADS1220 24位ADC驱动模块
 * @author  Refactored
 * @date    2024
 * @version 2.0
 * 
 * @details 重构内容:
 *          - 设备句柄结构，消除硬编码
 *          - DWT精确微秒延迟
 *          - 统一SPI通信(支持4种模式)
 *          - 寄存器读-改-写模式
 *          - 超时保护
 *          - 完全兼容旧API
 */

#ifndef ADS1220_H_
#define ADS1220_H_

#include "stm32f10x.h"

/*============================================================================*/
/*                              基础定义                                       */
/*============================================================================*/

#define ADS1220_ENABLE    1
#define ADS1220_DISABLE   0

/* SPI模式定义 */
#define ADS1220_SPI_MODE0   0  /* CPOL=0, CPHA=0 */
#define ADS1220_SPI_MODE1   1  /* CPOL=0, CPHA=1 */
#define ADS1220_SPI_MODE2   2  /* CPOL=1, CPHA=0 */
#define ADS1220_SPI_MODE3   3  /* CPOL=1, CPHA=1 */

/* 超时配置 */
#define ADS1220_TIMEOUT_DEFAULT   1000  /* 默认超时(ms) */
#define ADS1220_SPI_DELAY_US      5     /* SPI时序延迟(us) */

/*============================================================================*/
/*                              错误码定义                                     */
/*============================================================================*/

typedef enum {
    ADS1220_OK = 0,
    ADS1220_ERR_TIMEOUT = -1,
    ADS1220_ERR_PARAM = -2,
    ADS1220_ERR_NOT_INIT = -3
} Ads1220Status_e;

/* 兼容旧代码 */
#define ADS1220_NO_ERROR    ADS1220_OK
#define ADS1220_ERROR       ADS1220_ERR_PARAM

/*============================================================================*/
/*                              GPIO枚举                                       */
/*============================================================================*/

typedef enum {
    ADS1220_CS = 0,
    ADS1220_SCLK,
    ADS1220_DIN,
    ADS1220_DOUT,
    ADS1220_DRDY,
    ADS1220_GPIO_MAX
} Ads1220Gpio_e;

/*============================================================================*/
/*                              增益枚举                                       */
/*============================================================================*/

typedef enum {
    ADS1220_GAIN_X1   = 0x00,
    ADS1220_GAIN_X2   = 0x02,
    ADS1220_GAIN_X4   = 0x04,
    ADS1220_GAIN_X8   = 0x06,
    ADS1220_GAIN_X16  = 0x08,
    ADS1220_GAIN_X32  = 0x0A,
    ADS1220_GAIN_X64  = 0x0C,
    ADS1220_GAIN_X128 = 0x0E
} Ads1220Gain_e;

/* 兼容旧代码 */
typedef Ads1220Gain_e ADS1220_Gain;
#define GAIN_1    ADS1220_GAIN_X1
#define GAIN_2    ADS1220_GAIN_X2
#define GAIN_4    ADS1220_GAIN_X4
#define GAIN_8    ADS1220_GAIN_X8
#define GAIN_16   ADS1220_GAIN_X16
#define GAIN_32   ADS1220_GAIN_X32
#define GAIN_64   ADS1220_GAIN_X64
#define GAIN_128  ADS1220_GAIN_X128

/*============================================================================*/
/*                              设备句柄结构                                   */
/*============================================================================*/

typedef struct {
    GPIO_TypeDef* port[ADS1220_GPIO_MAX];
    uint16_t pin[ADS1220_GPIO_MAX];
    uint8_t spiMode;           /* SPI模式(0-3) */
    uint8_t spiDelayUs;        /* SPI时序延迟(us) */
    uint8_t regCache[4];       /* 寄存器缓存 */
    uint8_t isInitialized;     /* 初始化标志 */
} Ads1220Handle_t;

/* 兼容旧代码 */
typedef Ads1220Handle_t ADS1220_InitType;
extern Ads1220Handle_t ads1220Handle;
#define ADS1220_Init ads1220Handle

/*============================================================================*/
/*                              命令定义                                       */
/*============================================================================*/

#define ADS1220_CMD_RDATA       0x10
#define ADS1220_CMD_RREG        0x20
#define ADS1220_CMD_WREG        0x40
#define ADS1220_CMD_SYNC        0x08
#define ADS1220_CMD_SHUTDOWN    0x02
#define ADS1220_CMD_RESET       0x06

/*============================================================================*/
/*                              寄存器地址                                     */
/*============================================================================*/

#define ADS1220_REG0    0x00
#define ADS1220_REG1    0x01
#define ADS1220_REG2    0x02
#define ADS1220_REG3    0x03

/* 兼容旧定义 */
#define ADS1220_0_REGISTER   ADS1220_REG0
#define ADS1220_1_REGISTER   ADS1220_REG1
#define ADS1220_2_REGISTER   ADS1220_REG2
#define ADS1220_3_REGISTER   ADS1220_REG3

/*============================================================================*/
/*                              寄存器位掩码                                   */
/*============================================================================*/

/* REG0: MUX[7:4], GAIN[3:1], PGA_BYPASS[0] */
#define ADS1220_REG0_MUX_MASK       0xF0
#define ADS1220_REG0_GAIN_MASK      0x0E
#define ADS1220_REG0_PGA_MASK       0x01

/* REG1: DR[7:5], MODE[4:3], CM[2], TS[1], BCS[0] */
#define ADS1220_REG1_DR_MASK        0xE0
#define ADS1220_REG1_MODE_MASK      0x18
#define ADS1220_REG1_CM_MASK        0x04
#define ADS1220_REG1_TS_MASK        0x02
#define ADS1220_REG1_BCS_MASK       0x01

/* REG2: VREF[7:6], FIR[5:4], PSW[3], IDAC[2:0] */
#define ADS1220_REG2_VREF_MASK      0xC0
#define ADS1220_REG2_FIR_MASK       0x30
#define ADS1220_REG2_PSW_MASK       0x08
#define ADS1220_REG2_IDAC_MASK      0x07

/* REG3: I1MUX[7:5], I2MUX[4:2], DRDYM[1] */
#define ADS1220_REG3_I1MUX_MASK     0xE0
#define ADS1220_REG3_I2MUX_MASK     0x1C
#define ADS1220_REG3_DRDYM_MASK     0x02

/*============================================================================*/
/*                              MUX配置值                                      */
/*============================================================================*/

#define ADS1220_MUX_0_1     0x00
#define ADS1220_MUX_0_2     0x10
#define ADS1220_MUX_0_3     0x20
#define ADS1220_MUX_1_2     0x30
#define ADS1220_MUX_1_3     0x40
#define ADS1220_MUX_2_3     0x50
#define ADS1220_MUX_1_0     0x60
#define ADS1220_MUX_3_2     0x70
#define ADS1220_MUX_0_G     0x80
#define ADS1220_MUX_1_G     0x90
#define ADS1220_MUX_2_G     0xA0
#define ADS1220_MUX_3_G     0xB0
#define ADS1220_MUX_EX_VREF 0xC0
#define ADS1220_MUX_AVDD    0xD0
#define ADS1220_MUX_DIV2    0xE0

/*============================================================================*/
/*                              其他配置值(保持兼容)                            */
/*============================================================================*/

/* GAIN */
#define ADS1220_GAIN_1      0x00
#define ADS1220_GAIN_2      0x02
#define ADS1220_GAIN_4      0x04
#define ADS1220_GAIN_8      0x06
#define ADS1220_GAIN_16     0x08
#define ADS1220_GAIN_32     0x0A
#define ADS1220_GAIN_64     0x0C
#define ADS1220_GAIN_128    0x0E
#define ADS1220_PGA_BYPASS  0x01

/* Data Rate */
#define ADS1220_DR_20       0x00
#define ADS1220_DR_45       0x20
#define ADS1220_DR_90       0x40
#define ADS1220_DR_175      0x60
#define ADS1220_DR_330      0x80
#define ADS1220_DR_600      0xA0
#define ADS1220_DR_1000     0xC0

/* Mode */
#define ADS1220_MODE_NORMAL 0x00
#define ADS1220_MODE_DUTY   0x08
#define ADS1220_MODE_TURBO  0x10
#define ADS1220_MODE_DCT    0x18
#define ADS1220_CC          0x04
#define ADS1220_TEMP_SENSOR 0x02
#define ADS1220_BCS         0x01

/* VREF */
#define ADS1220_VREF_INT    0x00
#define ADS1220_VREF_EX_DED 0x40
#define ADS1220_VREF_EX_AIN 0x80
#define ADS1220_VREF_SUPPLY 0xC0

/* Filter */
#define ADS1220_REJECT_OFF  0x00
#define ADS1220_REJECT_BOTH 0x10
#define ADS1220_REJECT_50   0x20
#define ADS1220_REJECT_60   0x30
#define ADS1220_PSW_SW      0x08

/* IDAC */
#define ADS1220_IDAC_OFF    0x00
#define ADS1220_IDAC_10     0x01
#define ADS1220_IDAC_50     0x02
#define ADS1220_IDAC_100    0x03
#define ADS1220_IDAC_250    0x04
#define ADS1220_IDAC_500    0x05
#define ADS1220_IDAC_1000   0x06
#define ADS1220_IDAC_2000   0x07

/* IDAC Routing */
#define ADS1220_IDAC1_OFF   0x00
#define ADS1220_IDAC1_AIN0  0x20
#define ADS1220_IDAC1_AIN1  0x40
#define ADS1220_IDAC1_AIN2  0x60
#define ADS1220_IDAC1_AIN3  0x80
#define ADS1220_IDAC1_REFP0 0xA0
#define ADS1220_IDAC1_REFN0 0xC0
#define ADS1220_IDAC2_OFF   0x00
#define ADS1220_IDAC2_AIN0  0x04
#define ADS1220_IDAC2_AIN1  0x08
#define ADS1220_IDAC2_AIN2  0x0C
#define ADS1220_IDAC2_AIN3  0x10
#define ADS1220_IDAC2_REFP0 0x14
#define ADS1220_IDAC2_REFN0 0x18
#define ADS1220_DRDY_MODE   0x02

/*============================================================================*/
/*                              新API函数声明                                  */
/*============================================================================*/

/* DWT延迟初始化 */
void ads1220DwtInit(void);
void ads1220DwtDelayUs(uint32_t us);

/* 初始化与配置 */
void ads1220HwInit(Ads1220Handle_t* handle);
void ads1220HwConfig(Ads1220Handle_t* handle);
void ads1220SetSpiMode(Ads1220Handle_t* handle, uint8_t mode);

/* 数据读取(带超时) */
Ads1220Status_e ads1220WaitDataReady(Ads1220Handle_t* handle, uint32_t timeoutMs);
Ads1220Status_e ads1220ReadDataSafe(Ads1220Handle_t* handle, int32_t* data, uint32_t timeoutMs);

/* 寄存器操作(读-改-写模式) */
Ads1220Status_e ads1220SetRegBits(Ads1220Handle_t* handle, uint8_t reg, uint8_t mask, uint8_t value);
Ads1220Status_e ads1220GetRegBits(Ads1220Handle_t* handle, uint8_t reg, uint8_t mask, uint8_t* value);
void ads1220SyncRegCache(Ads1220Handle_t* handle);

/* 便捷配置函数 */
Ads1220Status_e ads1220SetMux(Ads1220Handle_t* handle, uint8_t mux);
Ads1220Status_e ads1220SetGainValue(Ads1220Handle_t* handle, Ads1220Gain_e gain);
Ads1220Status_e ads1220SetDataRateValue(Ads1220Handle_t* handle, uint8_t dr);
Ads1220Status_e ads1220SetPgaBypass(Ads1220Handle_t* handle, uint8_t bypass);
Ads1220Status_e ads1220SetVref(Ads1220Handle_t* handle, uint8_t vref);
Ads1220Status_e ads1220SetIdac(Ads1220Handle_t* handle, uint8_t idac);

/*============================================================================*/
/*                              兼容旧API(保留)                                */
/*============================================================================*/

void ADS1220Init(void);
int ADS1220WaitForDataReady(int Timeout);
void ADS1220CsStatus(uint8_t state);
void ADS1220SendByte(unsigned char cData);
unsigned char ADS1220ReceiveByte(void);
long ADS1220ReadData(void);
void ADS1220ReadRegister(int StartAddress, int NumRegs, unsigned* pData);
void ADS1220WriteRegister(int StartAddress, int NumRegs, unsigned* pData);
void ADS1220SendResetCommand(void);
void ADS1220SendStartCommand(void);
void ADS1220SendShutdownCommand(void);
void ADS1220Config(void);
int ADS1220SetChannel(int Mux);
void ADS1220SetGain(ADS1220_Gain Gain);
int ADS1220SetPGABypass(int Bypass);
int ADS1220SetDataRate(int DataRate);
int ADS1220SetClockMode(int ClockMode);
int ADS1220SetPowerDown(int PowerDown);
int ADS1220SetTemperatureMode(int TempMode);
int ADS1220SetBurnOutSource(int BurnOut);
int ADS1220SetVoltageReference(int VoltageRef);
int ADS1220Set50_60Rejection(int Rejection);
int ADS1220SetLowSidePowerSwitch(int PowerSwitch);
int ADS1220SetCurrentDACOutput(int CurrentOutput);
int ADS1220SetIDACRouting(int IDACRoute);
int ADS1220SetDRDYMode(int DRDYMode);
int ADS1220GetChannel(void);
int ADS1220GetGain(void);
int ADS1220GetPGABypass(void);
int ADS1220GetDataRate(void);
int ADS1220GetClockMode(void);
int ADS1220GetPowerDown(void);
int ADS1220GetTemperatureMode(void);
int ADS1220GetBurnOutSource(void);
int ADS1220GetVoltageReference(void);
int ADS1220Get50_60Rejection(void);
int ADS1220GetLowSidePowerSwitch(void);
int ADS1220GetCurrentDACOutput(void);
int ADS1220GetIDACRouting(int WhichOne);
int ADS1220GetDRDYMode(void);

#endif /* ADS1220_H_ */
