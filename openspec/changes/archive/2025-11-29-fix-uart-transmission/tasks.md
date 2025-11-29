# Tasks: 清理调试代码

## 1. 移除调试打印

- [x] 1.1 修改 `HARDWARE/modbus/modbus_master.c`，移除 `u1_printf` 声明和调用
- [x] 1.2 修改 `USER/main.c`，恢复 `uartDebugInit(9600)` 初始化，移除 `uart_init` 和测试代码

## 2. 验证

- [x] 2.1 编译确认无错误
- [x] 2.2 确认串口4继续发送ModBus帧（通过外部监控）
- [x] 2.3 确认串口1功能恢复（不再输出调试日志）
