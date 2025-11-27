## ADDED Requirements

### Requirement: UART错误恢复

UART驱动 **SHALL** 自动检测并恢复以下错误：

- DMA传输错误(TE)
- 接收溢出错误(ORE)  
- 帧格式错误(FE)
- 噪声错误(NE)

当检测到错误时，驱动 **SHALL** 自动重新初始化DMA并继续接收。

#### Scenario: DMA传输错误恢复

- **GIVEN** UART正在DMA接收模式运行
- **WHEN** DMA发生传输错误(TE标志置位)
- **THEN** 驱动清除错误标志，重新初始化DMA接收
- **AND** 错误计数器`dmaErrors`增加1

#### Scenario: 接收溢出恢复

- **GIVEN** UART正在接收数据
- **WHEN** 新数据到达但上一字节未被读取(ORE)
- **THEN** 驱动清除溢出标志
- **AND** 错误计数器`overruns`增加1

---

### Requirement: 通信统计

每个UART **SHALL** 维护以下统计数据：

| 统计项 | 类型 | 说明 |
|--------|------|------|
| txFrames | uint32 | 发送帧计数 |
| rxFrames | uint32 | 接收帧计数 |
| crcErrors | uint32 | CRC校验错误 |
| timeouts | uint32 | 响应超时 |
| overruns | uint32 | 接收溢出 |
| dmaErrors | uint32 | DMA错误 |

#### Scenario: 统计数据更新

- **GIVEN** UART成功发送一帧数据
- **WHEN** DMA发送完成中断触发
- **THEN** `txFrames`计数器增加1

---

### Requirement: 通信活动监控

ModBus协议层 **SHALL** 监控通信活动，超过30秒无活动时记录告警。

#### Scenario: 活动超时检测

- **GIVEN** ModBus句柄正在运行
- **WHEN** 30秒内无任何发送或接收活动
- **THEN** `activityTimeouts`计数器增加1
- **AND** 可选触发系统复位

---

### Requirement: 上电通信响应

系统 **SHALL** 在上电自检期间响应屏幕ModBus请求，避免显示"通讯故障"。

#### Scenario: 上电期间屏幕通信

- **GIVEN** 系统正在执行上电自检(10秒)
- **WHEN** 屏幕发送ModBus请求
- **THEN** 系统正常响应请求
- **AND** 屏幕不显示"通讯故障"

---

## MODIFIED Requirements

### Requirement: 缓冲区大小配置

UART缓冲区大小 **SHALL** 根据实际需求差异化配置：

| UART | 用途 | 缓冲区大小 |
|------|------|-----------|
| USART1 | RTU服务器 | 128字节 |
| USART2 | 屏幕通信 | 256字节 |
| USART3 | 变频水泵 | 64字节 |
| UART4 | 联控通信 | 128字节 |

#### Scenario: 最大帧接收

- **GIVEN** USART2配置256字节缓冲区
- **WHEN** 屏幕发送200字节数据帧
- **THEN** 数据完整接收，无溢出

---

## ADDED Requirements (Flash保护)

### Requirement: Flash延迟写入

参数写入Flash **SHALL** 延迟5秒执行，合并多次修改为一次写入。

#### Scenario: 参数连续修改

- **GIVEN** 用户在屏幕上修改参数A
- **WHEN** 1秒后用户又修改参数B
- **THEN** 系统在参数B修改后5秒才执行一次Flash写入
- **AND** 参数A和B同时保存

### Requirement: Flash写入限制

系统 **SHALL** 限制每日Flash写入次数不超过100次。

#### Scenario: 达到写入限制

- **GIVEN** 当日已写入Flash 100次
- **WHEN** 用户尝试再次修改参数
- **THEN** 参数在RAM中更新，但不写入Flash
- **AND** 次日计数重置后再写入

