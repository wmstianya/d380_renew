/**
 * @file modbus.h
 * @brief ModBus RTU 协议层公共头文件
 * @author AI Assistant
 * @date 2024-11
 * @version 1.0
 */

#ifndef __MODBUS_H
#define __MODBUS_H

#include "modbus_types.h"

/*============================================================================*/
/*                              初始化接口                                     */
/*============================================================================*/

/**
 * @brief 初始化ModBus句柄
 * @param handle ModBus句柄
 * @param uart UART驱动句柄
 * @param role 角色 (MODBUS_ROLE_SLAVE/MASTER/DUAL)
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusInit(ModbusHandle* handle, UartHandle* uart, ModbusRole role);

/**
 * @brief 配置从机模式
 * @param handle ModBus句柄
 * @param address 从机地址
 * @param regMap 寄存器映射表
 * @param callback 从机回调 (可为NULL)
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusConfigSlave(ModbusHandle* handle, 
                               uint8_t address, 
                               ModbusRegMap* regMap,
                               ModbusSlaveCallback* callback);

/**
 * @brief 配置主机模式
 * @param handle ModBus句柄
 * @param config 主机配置
 * @param callback 主机回调
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusConfigMaster(ModbusHandle* handle,
                                ModbusMasterCfg* config,
                                ModbusMasterCallback* callback);

/*============================================================================*/
/*                              调度接口                                       */
/*============================================================================*/

/**
 * @brief ModBus调度器 (主循环调用)
 * @param handle ModBus句柄
 * @note 根据角色自动执行从机响应或主机轮询
 */
void modbusScheduler(ModbusHandle* handle);

/**
 * @brief 检查是否有待处理的接收数据
 * @param handle ModBus句柄
 * @return TRUE有数据 / FALSE无数据
 */
uint8_t modbusHasRxData(ModbusHandle* handle);

/*============================================================================*/
/*                              从机接口                                       */
/*============================================================================*/

/**
 * @brief 从机处理入口
 * @param handle ModBus句柄
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusSlaveProcess(ModbusHandle* handle);

/*============================================================================*/
/*                              主机接口                                       */
/*============================================================================*/

/**
 * @brief 发送读取请求 (功能码03)
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址
 * @param regCount 寄存器数量
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusMasterRead(ModbusHandle* handle, 
                              uint8_t slaveAddr, 
                              uint16_t regAddr, 
                              uint16_t regCount);

/**
 * @brief 发送写入请求 (功能码10)
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址
 * @param data 数据
 * @param regCount 寄存器数量
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusMasterWrite(ModbusHandle* handle,
                               uint8_t slaveAddr,
                               uint16_t regAddr,
                               uint8_t* data,
                               uint16_t regCount);

/**
 * @brief 发送单寄存器写入请求 (功能码06)
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @param regAddr 寄存器地址
 * @param value 寄存器值
 * @return MODBUS_OK 或错误码
 */
ModbusError modbusMasterWriteSingle(ModbusHandle* handle,
                                     uint8_t slaveAddr,
                                     uint16_t regAddr,
                                     uint16_t value);

/**
 * @brief 主机调度器
 * @param handle ModBus句柄
 */
void modbusMasterScheduler(ModbusHandle* handle);

/**
 * @brief 检查从机是否在线
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 * @return TRUE在线 / FALSE离线
 */
uint8_t modbusMasterIsOnline(ModbusHandle* handle, uint8_t slaveAddr);

/**
 * @brief 标记需要写入
 * @param handle ModBus句柄
 * @param slaveAddr 从机地址
 */
void modbusMasterMarkWrite(ModbusHandle* handle, uint8_t slaveAddr);

/*============================================================================*/
/*                              工具函数                                       */
/*============================================================================*/

/**
 * @brief CRC16校验计算
 * @param data 数据
 * @param len 长度
 * @return CRC16值 (高字节在前)
 */
uint16_t modbusCrc16(uint8_t* data, uint16_t len);

/**
 * @brief CRC校验
 * @param frame 帧数据
 * @param len 帧长度 (含CRC)
 * @return TRUE校验通过 / FALSE校验失败
 */
uint8_t modbusCrcCheck(uint8_t* frame, uint16_t len);

/**
 * @brief 获取系统时间戳 (ms)
 * @return 时间戳
 * @note 需要外部实现
 */
extern uint32_t modbusGetTick(void);

#endif /* __MODBUS_H */

