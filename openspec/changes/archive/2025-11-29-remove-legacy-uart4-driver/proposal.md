# Change: 移除UART4旧驱动代码

## Why
新的统一ModBus协议层 (`modbus_uart4.c`) 已验证与原驱动寄存器映射完全一致，现在可以安全地移除旧驱动的条件编译分支，简化代码结构。

## What Changes
- 移除 `main.c` 中 `USE_UNIFIED_MODBUS` 条件编译宏
- 移除所有 `#else` 分支中的旧驱动调用代码
- 直接调用新的统一调度器函数
- 保留 `usart4.c` 中的旧代码作为参考（暂不删除）

## Impact
- Affected specs: modbus-unified
- Affected code: `USER/main.c`

---

# 需要移除的旧驱动调用

## main.c 中的条件编译块

### 1. 头文件引用 (Line 38-47)
```c
#if USE_UNIFIED_MODBUS
#include "modbus.h"
extern ModbusError modbusUsart3Init(void);
extern void modbusUsart3Scheduler(void);
extern ModbusError modbusUart4Init(void);
extern void modbusUart4Scheduler(void);
// ...
#endif
```
→ 移除条件编译，保留头文件引用

### 2. 初始化 (Line 124-130)
```c
#if USE_UNIFIED_MODBUS
    modbusUsart3Init();
    modbusUart4Init();
#endif
```
→ 移除条件编译，保留初始化调用

### 3. USART1调度 (Line 236-240)
```c
#if USE_UNIFIED_MODBUS
    modbusUsart1Scheduler();
#else
    ModBus_Communication();
#endif
```
→ 移除条件编译，只保留 `modbusUsart1Scheduler()`

### 4. USART2调度 (Line 298-302)
```c
#if USE_UNIFIED_MODBUS
    modbusUsart2Scheduler();
#else
    Union_ModBus2_Communication();
#endif
```
→ 移除条件编译，只保留 `modbusUsart2Scheduler()`

### 5. USART3调度 - 主机模式 (Line 311-316)
```c
#if USE_UNIFIED_MODBUS
    modbusUsart3Scheduler();
#else
    Modbus3_UnionTx_Communication();
    ModBus_Uart3_LocalRX_Communication();
#endif
```
→ 移除条件编译，只保留 `modbusUsart3Scheduler()`

### 6. UART4调度 - 主机模式 (Line 320-330)
```c
#if USE_UNIFIED_MODBUS
    modbusUart4Scheduler();
#else
    if(sys_flag.LCD10_Connect) {
        Union_Modbus4_UnionTx_Communication();
    }
    Union_ModBus_Uart4_Local_Communication();
#endif
```
→ 移除条件编译，只保留 `modbusUart4Scheduler()`

### 7. USART3调度 - 从机模式 (Line 358-363)
```c
#if USE_UNIFIED_MODBUS
    modbusUsart3Scheduler();
#else
    Modbus3_UnionTx_Communication();
    ModBus_Uart3_LocalRX_Communication();
#endif
```
→ 移除条件编译，只保留 `modbusUsart3Scheduler()`

### 8. UART4调度 - 从机模式 (Line 365-369)
```c
#if USE_UNIFIED_MODBUS
    modbusUart4Scheduler();
#else
    ModBus_Uart4_Local_Communication();
#endif
```
→ 移除条件编译，只保留 `modbusUart4Scheduler()`

---

## 移除后的代码结构

移除条件编译后，`main.c` 将直接调用统一调度器：
- `modbusUsart1Scheduler()` - USART1 远程控制
- `modbusUsart2Scheduler()` - USART2 LCD屏幕通信
- `modbusUsart3Scheduler()` - USART3 变频进水阀
- `modbusUart4Scheduler()` - UART4 联控通信（双角色）

