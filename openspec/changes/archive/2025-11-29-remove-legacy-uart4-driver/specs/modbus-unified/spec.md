## ADDED Requirements

### Requirement: 移除旧驱动条件编译

系统SHALL直接使用统一ModBus协议层调度器，移除所有条件编译切换代码。

#### Scenario: 直接调用统一调度器
- **WHEN** 系统初始化完成后进入主循环
- **THEN** 直接调用 `modbusUartXScheduler()` 函数
- **AND** 不再使用条件编译切换新旧驱动

#### Scenario: 移除USE_UNIFIED_MODBUS宏
- **WHEN** 完成驱动切换后
- **THEN** 移除 `#define USE_UNIFIED_MODBUS` 宏定义
- **AND** 移除所有相关的 `#if/#else/#endif` 条件编译块

