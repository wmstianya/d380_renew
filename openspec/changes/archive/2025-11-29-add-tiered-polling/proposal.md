# Change: 实现分级轮询机制

## Why

当前UART4的ModBus轮询策略是"全功能轮询"，每次都发送读取18个寄存器和写入18个寄存器的请求。这导致：
1. 从机可能不支持某些寄存器，返回异常响应（0x83 非法数据地址）
2. 频繁的异常响应影响通讯效率和状态稳定性
3. 无法快速检测从机在线状态

## What Changes

- 实现**分级轮询机制**，类似CAN总线的心跳包：
  - **快速心跳** (100ms): 只读1个寄存器，用于在线检测
  - **完整数据轮询** (1000ms): 读取/写入全部寄存器，仅在从机在线时执行
- 添加心跳寄存器地址配置（使用寄存器100，读取1个寄存器）
- 优化状态机：心跳失败立即判定离线，心跳成功才执行完整轮询

## Impact

- Affected specs: `uart4-union-comm`
- Affected code:
  - `HARDWARE/modbus/modbus_uart4.c` - 分级轮询逻辑
  - `HARDWARE/modbus/modbus_master.c` - 支持心跳模式
  - `USER/main.h` - 新增配置宏

