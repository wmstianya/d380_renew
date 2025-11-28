# ModBus统一协议层 - UART4回调签名修复

## MODIFIED Requirements

### Requirement: 主机回调接口一致性

UART4主机模式的`getWriteData`回调函数 MUST 符合`ModbusGetWriteDataFunc`类型签名：

```c
typedef uint16_t (*ModbusGetWriteDataFunc)(uint8_t slaveAddr, uint16_t* regAddr,
                                            uint8_t* buffer, uint16_t maxLen);
```

回调函数:
1. MUST 接收4个参数: `slaveAddr`, `regAddr`, `buffer`, `maxLen`
2. MUST 通过 `*regAddr` 输出写入的寄存器地址
3. MUST 返回纯数据字节数（不含帧头和CRC）
4. SHALL NOT 在回调内构造ModBus帧头或计算CRC（由`modbus_master.c`负责）

#### Scenario: UART4主机正常轮询分机

- **GIVEN** 设备为主控模式 (`Address_Number == 0`)
- **WHEN** `modbusUart4Scheduler()` 执行主机调度
- **THEN** 应依次向从机1-10发送读取请求 (功能码03, 地址100, 18寄存器)
- **AND** 收到响应后发送写入请求 (功能码10, 地址200, 18寄存器)

#### Scenario: 回调返回纯数据

- **GIVEN** 主机调度器调用 `getWriteData` 回调
- **WHEN** 回调准备写入数据
- **THEN** 应设置 `*regAddr = 200`
- **AND** 在 `buffer[0..35]` 填充18个寄存器的数据
- **AND** 返回36（字节数）
- **AND** 不应包含地址码、功能码、长度字节或CRC

