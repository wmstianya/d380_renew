# Communication Protocol Specification

通信协议规格说明 - UART通信、ModBus协议和多机通信架构。

## Requirements

### Requirement: UART端口分配
系统必须正确配置和使用5个UART端口，各有专用功能。

#### Scenario: UART1 - WiFi/调试通信
- **WHEN** 系统需要远程监控或调试
- **THEN** 使用 UART1
- **AND** 波特率：115200
- **AND** 连接：WiFi模块或调试串口
- **AND** 引脚：PA9(TX), PA10(RX)

#### Scenario: UART2 - 显示屏通信
- **WHEN** 系统需要更新显示内容
- **THEN** 使用 UART2
- **AND** 波特率：115200
- **AND** 连接：串口触摸屏（迪文或泰杰）
- **AND** 引脚：PA2(TX), PA3(RX)

#### Scenario: UART3 - 主从机通信
- **WHEN** 多机联控模式
- **THEN** 使用 UART3
- **AND** 波特率：115200
- **AND** 协议：自定义帧协议
- **AND** 引脚：PB10(TX), PB11(RX)
- **AND** 物理层：RS485

#### Scenario: UART4 - ModBus从机
- **WHEN** 接收外部控制命令
- **THEN** 使用 UART4
- **AND** 波特率：9600
- **AND** 协议：ModBus RTU
- **AND** 引脚：PC10(TX), PC11(RX)

#### Scenario: UART5 - 预留
- **WHEN** 扩展功能需要
- **THEN** 使用 UART5
- **AND** 引脚：PC12(TX), PD2(RX)

### Requirement: ModBus RTU协议
系统必须支持标准ModBus RTU协议，作为从站响应主站查询。

#### Scenario: ModBus地址配置
- **WHEN** 系统初始化
- **THEN** 从拨码开关读取设备地址
- **AND** 地址范围：1-247
- **AND** 广播地址0不响应

#### Scenario: 功能码03 - 读保持寄存器
- **WHEN** 收到功能码03请求
- **THEN** 返回指定寄存器的值
- **AND** 支持连续读取多个寄存器
- **AND** 最大一次读取125个寄存器

#### Scenario: 功能码06 - 写单个寄存器
- **WHEN** 收到功能码06请求
- **THEN** 写入指定寄存器
- **AND** 返回回显确认
- **AND** 实时生效

#### Scenario: CRC校验
- **WHEN** 收到或发送ModBus帧
- **THEN** 使用CRC16-MODBUS校验
- **AND** 多项式：0xA001
- **AND** 校验失败丢弃帧

### Requirement: 主从机通信协议
多机联控模式下，主机与从机必须使用自定义帧协议通信。

#### Scenario: 帧格式定义
- **WHEN** 发送通信帧
- **THEN** 使用以下格式：
  - 帧头：0xAA 0x55
  - 源地址：1字节
  - 目标地址：1字节
  - 数据长度：1字节
  - 数据内容：N字节
  - 校验和：1字节
  - 帧尾：0x0D 0x0A

#### Scenario: 主机广播查询
- **WHEN** 主机每秒广播一次
- **THEN** 目标地址 = 0xFF（广播）
- **AND** 包含主机状态信息
- **AND** 从机收到后更新主机信息

#### Scenario: 从机状态上报
- **WHEN** 从机收到主机查询
- **THEN** 响应自身状态
- **AND** 包含：压力、功率、故障码、运行状态
- **AND** 响应延迟 = 地址 × 10ms（避免冲突）

### Requirement: 通信超时处理
系统必须处理通信超时，确保系统安全。

#### Scenario: 从机通信超时
- **WHEN** 主机超过3秒未收到某从机响应
- **THEN** 标记该从机为离线 (Alive_Flag = 0)
- **AND** 不参与功率分配
- **AND** 重新计算需求台数

#### Scenario: 主机通信超时
- **WHEN** 从机超过10秒未收到主机指令
- **THEN** 从机进入独立运行模式
- **AND** 使用本地设定值
- **AND** 等待主机恢复

## Implementation

### 相关文件
- `HARDWARE/usart/bsp_usartx.c` - UART驱动
- `SYSTEM/ModBus/ModBus.c` - ModBus协议实现
- `SYSTEM/system_control/system_control.c` - 主从通信逻辑

### 寄存器映射（部分）
| 地址 | 名称 | 读写 | 说明 |
|------|------|------|------|
| 0x0000 | 运行状态 | R | 系统状态码 |
| 0x0001 | 当前压力 | R | ×0.01MPa |
| 0x0002 | 当前温度 | R | ×0.1℃ |
| 0x0003 | 故障代码 | R | 错误编号 |
| 0x0010 | 启停控制 | W | 1=启动, 0=停止 |
| 0x0011 | 目标压力 | W | ×0.01MPa |

## Version
- Version: 1.0
- Status: Active
- Last Updated: 2025-11-26

