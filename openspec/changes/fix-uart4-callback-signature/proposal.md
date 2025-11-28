# Change: 修复UART4主机回调函数签名不匹配

## Why

串口4联控板不发送查询指令。经分析发现`uart4GetWriteData`回调函数签名与`ModbusGetWriteDataFunc`类型定义不匹配，缺少`uint16_t* regAddr`参数，导致主机调度器调用时栈混乱，进而引发主机轮询功能异常。

## What Changes

- **修复** `modbus_uart4.c` 中 `uart4GetWriteData` 函数签名，增加缺失的 `uint16_t* regAddr` 参数
- **修正** 函数内部数据偏移，将帧构造责任交回 `modbus_master.c`

## Impact

- Affected specs: modbus-unified
- Affected code: `HARDWARE/modbus/modbus_uart4.c`

## 问题分析

### 当前错误代码

```c
// modbus_uart4.c - 错误的签名 (缺少 regAddr 参数)
static uint16_t uart4GetWriteData(uint8_t slaveAddr, uint8_t* data, 
                                   uint16_t maxLen)
```

### 正确的类型定义

```c
// modbus_types.h:151 - 标准签名
typedef uint16_t (*ModbusGetWriteDataFunc)(uint8_t slaveAddr, uint16_t* regAddr,
                                            uint8_t* buffer, uint16_t maxLen);
```

### 调用点

```c
// modbus_master.c:177 - 按4参数调用
writeLen = handle->masterCb.getWriteData(slaveAddr, &writeRegAddr,
                                          writeData, sizeof(writeData));
```

### 后果

1. 编译时可能不报错（C语言函数指针类型检查松散）
2. 运行时参数传递错乱
3. 第一轮读取请求能发出，但写入阶段出错后调度器进入异常状态
4. 导致主机轮询停止工作

## 确认事项

修复后，主机设备（`Address_Number == 0`）应能看到如下查询帧：
```
01 03 00 64 00 12 xx xx  (读取从机1，地址100，18寄存器)
02 03 00 64 00 12 xx xx  (读取从机2)
...
```

