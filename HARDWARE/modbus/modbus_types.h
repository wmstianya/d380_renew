/**
 * @file modbus_types.h
 * @brief ModBus RTU 协议层类型定义
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 * 
 * @note 统一ModBus协议层 - 类型定义
 */

#ifndef __MODBUS_TYPES_H
#define __MODBUS_TYPES_H

#include "sys.h"
#include "uart_driver.h"

/*============================================================================*/
/*                              基础定义                                       */
/*============================================================================*/

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

/*============================================================================*/
/*                              常量定义                                       */
/*============================================================================*/

#define MODBUS_MAX_FRAME        256     /* 最大帧长度 */
#define MODBUS_MAX_SLAVES       16      /* 最大从机数量 */
#define MODBUS_WRITE_CACHE_SIZE 64      /* 写入缓存大小 */
#define MODBUS_BROADCAST_ADDR   0xFF    /* 广播地址 */

/* 功能码定义 */
#define MODBUS_FC_READ_HOLDING  0x03    /* 读保持寄存器 */
#define MODBUS_FC_WRITE_SINGLE  0x06    /* 写单个寄存器 */
#define MODBUS_FC_WRITE_MULTI   0x10    /* 写多个寄存器 */

/* 异常码定义 */
#define MODBUS_EX_ILLEGAL_FUNC  0x01    /* 非法功能码 */
#define MODBUS_EX_ILLEGAL_ADDR  0x02    /* 非法数据地址 */
#define MODBUS_EX_ILLEGAL_VALUE 0x03    /* 非法数据值 */
#define MODBUS_EX_SLAVE_FAILURE 0x04    /* 从机故障 */

/*============================================================================*/
/*                              枚举定义                                       */
/*============================================================================*/

/**
 * @brief ModBus错误码
 */
typedef enum {
    MODBUS_OK = 0,              /* 成功 */
    MODBUS_ERR_CRC,             /* CRC错误 */
    MODBUS_ERR_ADDR,            /* 地址不匹配 */
    MODBUS_ERR_FUNC,            /* 功能码错误 */
    MODBUS_ERR_TIMEOUT,         /* 超时 */
    MODBUS_ERR_PARAM,           /* 参数错误 */
    MODBUS_ERR_BUSY             /* 忙 */
} ModbusError;

/**
 * @brief ModBus角色
 */
typedef enum {
    MODBUS_ROLE_SLAVE  = 0x01,  /* 纯从机 */
    MODBUS_ROLE_MASTER = 0x02,  /* 纯主机 */
    MODBUS_ROLE_DUAL   = 0x03   /* 双角色 (SLAVE | MASTER) */
} ModbusRole;

/**
 * @brief ModBus状态机
 */
typedef enum {
    MODBUS_STATE_IDLE,              /* 空闲 */
    MODBUS_STATE_SLAVE_PROCESS,     /* 从机处理中 */
    MODBUS_STATE_SLAVE_RESPOND,     /* 从机响应中 */
    MODBUS_STATE_MASTER_SEND,       /* 主机发送中 */
    MODBUS_STATE_MASTER_WAIT,       /* 主机等待响应 */
    MODBUS_STATE_MASTER_PROCESS     /* 主机处理响应 */
} ModbusState;

/*============================================================================*/
/*                              回调函数类型                                   */
/*============================================================================*/

/**
 * @brief 寄存器读取回调
 * @param address 寄存器起始地址
 * @param count 寄存器数量
 * @param buffer 输出缓冲区
 * @return 实际读取的字节数，0表示地址无效
 */
typedef uint16_t (*ModbusReadFunc)(uint16_t address, uint16_t count, uint8_t* buffer);

/**
 * @brief 寄存器写入回调
 * @param address 寄存器起始地址
 * @param count 寄存器数量
 * @param data 写入数据
 * @param dataLen 数据长度
 * @return MODBUS_OK 或错误码
 */
typedef ModbusError (*ModbusWriteFunc)(uint16_t address, uint16_t count, 
                                        uint8_t* data, uint16_t dataLen);

/**
 * @brief 写入完成回调
 * @param funcCode 功能码
 * @param regAddr 寄存器地址
 * @param data 写入的数据
 * @param dataLen 数据长度
 */
typedef void (*ModbusWriteCompleteFunc)(uint8_t funcCode, uint16_t regAddr, 
                                         uint8_t* data, uint16_t dataLen);

/**
 * @brief 主机响应回调
 * @param slaveAddr 从机地址
 * @param funcCode 功能码
 * @param data 响应数据
 * @param dataLen 数据长度
 */
typedef void (*ModbusMasterRespFunc)(uint8_t slaveAddr, uint8_t funcCode, 
                                      uint8_t* data, uint16_t dataLen);

/**
 * @brief 主机超时回调
 * @param slaveAddr 从机地址
 */
typedef void (*ModbusMasterTimeoutFunc)(uint8_t slaveAddr);

/**
 * @brief 主机离线回调
 * @param slaveAddr 从机地址
 */
typedef void (*ModbusMasterOfflineFunc)(uint8_t slaveAddr);

/**
 * @brief 获取写入数据回调
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址 (输出)
 * @param buffer 数据缓冲 (输出)
 * @param maxLen 最大长度
 * @return 实际数据长度，0表示无需写入
 */
typedef uint16_t (*ModbusGetWriteDataFunc)(uint8_t slaveAddr, uint16_t* regAddr,
                                            uint8_t* buffer, uint16_t maxLen);

/*============================================================================*/
/*                              结构体定义                                     */
/*============================================================================*/

/**
 * @brief 寄存器映射项
 */
typedef struct {
    uint16_t startAddr;         /* 起始地址 */
    uint16_t endAddr;           /* 结束地址 */
    ModbusReadFunc readFunc;    /* 读取函数 */
    ModbusWriteFunc writeFunc;  /* 写入函数 (NULL表示只读) */
} ModbusRegEntry;

/**
 * @brief 寄存器映射表
 */
typedef struct {
    ModbusRegEntry* entries;    /* 映射项数组 */
    uint8_t entryCount;         /* 映射项数量 */
} ModbusRegMap;

/**
 * @brief 从机回调集合
 */
typedef struct {
    ModbusWriteCompleteFunc onWriteComplete;  /* 写入完成回调 */
} ModbusSlaveCallback;

/**
 * @brief 主机回调集合
 */
typedef struct {
    ModbusMasterRespFunc onResponse;          /* 响应接收回调 */
    ModbusMasterTimeoutFunc onTimeout;        /* 超时回调 */
    ModbusMasterOfflineFunc onOffline;        /* 离线回调 */
    ModbusGetWriteDataFunc getWriteData;      /* 获取写入数据 */
} ModbusMasterCallback;

/**
 * @brief 写入缓存 (按需写入优化)
 */
typedef struct {
    uint8_t needWrite;                              /* 是否需要写入 */
    uint8_t failCount;                              /* 失败计数 */
    uint8_t lastData[MODBUS_WRITE_CACHE_SIZE];      /* 上次写入数据 */
    uint16_t lastDataLen;                           /* 上次数据长度 */
} ModbusWriteCache;

/**
 * @brief 主机配置
 */
typedef struct {
    uint8_t pollAddrs[MODBUS_MAX_SLAVES];   /* 轮询地址列表 */
    uint8_t pollCount;                       /* 轮询地址数量 */
    uint8_t currentIdx;                      /* 当前轮询索引 */
    uint8_t currentSendIdx;                  /* 当前发送索引 (读/写切换) */
    
    uint16_t readRegAddr;                    /* 读取寄存器地址 */
    uint16_t readRegCount;                   /* 读取寄存器数量 */
    uint16_t writeRegAddr;                   /* 写入寄存器地址 */
    
    uint16_t respTimeoutMs;                  /* 响应超时 (ms) */
    uint16_t pollIntervalMs;                 /* 轮询间隔 (ms), 0=动态 */
    
    uint8_t maxRetry;                        /* 最大重试次数 */
    uint8_t offlineThreshold;                /* 离线判定阈值 */
    
    ModbusWriteCache writeCache[MODBUS_MAX_SLAVES];  /* 写入缓存 */
} ModbusMasterCfg;

/**
 * @brief 通信统计
 */
typedef struct {
    uint32_t txCount;           /* 发送帧数 */
    uint32_t rxCount;           /* 接收帧数 */
    uint32_t crcErrCount;       /* CRC错误数 */
    uint32_t timeoutCount;      /* 超时次数 */
} ModbusStats;

/**
 * @brief ModBus句柄
 */
typedef struct ModbusHandle {
    /* 底层驱动 */
    UartHandle* uart;                       /* UART DMA驱动句柄 */
    
    /* 角色配置 */
    ModbusRole role;                        /* 角色 */
    
    /* 状态机 */
    ModbusState state;                      /* 当前状态 */
    uint32_t stateTimestamp;                /* 状态切换时间戳 */
    
    /* 从机配置 */
    uint8_t slaveAddr;                      /* 本机从机地址 */
    ModbusRegMap* regMap;                   /* 寄存器映射表 */
    ModbusSlaveCallback slaveCb;            /* 从机回调 */
    
    /* 主机配置 */
    ModbusMasterCfg masterCfg;              /* 主机配置 */
    ModbusMasterCallback masterCb;          /* 主机回调 */
    
    /* 缓冲区 */
    uint8_t txBuf[MODBUS_MAX_FRAME];        /* 发送缓冲 */
    uint8_t rxBuf[MODBUS_MAX_FRAME];        /* 接收缓冲 */
    uint16_t rxLen;                         /* 接收长度 */
    
    /* 统计 */
    ModbusStats stats;                      /* 通信统计 */
} ModbusHandle;

#endif /* __MODBUS_TYPES_H */

