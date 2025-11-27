# Change: 统一ModBus RTU协议层重构

## Why

当前系统存在以下问题：
- 4个串口各自实现一套ModBus RTU逻辑（usart.c, usart2.c, usart3.c, usart4.c）
- 代码重复率高，维护困难
- 新增串口需要复制整个文件
- Bug修复需要改4个地方
- 主从角色逻辑分散，难以理解

## What Changes

### Phase 1: 寄存器地址梳理 (前置工作)

梳理4个串口的ModBus寄存器地址表：

| 串口 | 角色 | 寄存器范围 | 用途 |
|------|------|------------|------|
| USART1 | 从机 | 待梳理 | RTU服务器响应 |
| USART2 | 从机 | 0x0000-0x0117 | 响应屏幕请求 |
| USART3 | 主机 | 待梳理 | 轮询从机设备 |
| UART4 | **双角色** | 待梳理 | 主机(轮询分机) + 从机(响应联控主机) |

### Phase 2: 主从角色分析

**联控板角色变化**：
```
屏幕 ──(USART2)──> 联控板(从机) ──(UART4/主机)──> 分机1~10
                        │
                        └── UART4也可作为从机响应上级联控主机
```

### Phase 3: 统一ModBus协议层

```
HARDWARE/modbus/
├── modbus_rtu.h       # 统一接口
├── modbus_rtu.c       # CRC、帧解析
├── modbus_slave.c     # 从机通用处理
└── modbus_master.c    # 主机轮询处理
```

### Phase 4: 业务层重构

将各串口文件简化为：
- 初始化配置
- 寄存器读写回调
- 业务逻辑

## Impact

- **Affected code**: 
  - `SYSTEM/usart/usart.c`
  - `HARDWARE/USART2/usart2.c`
  - `HARDWARE/USART3/usart3.c`
  - `HARDWARE/USART4/usart4.c`
  
- **New modules**:
  - `HARDWARE/modbus/modbus_rtu.h`
  - `HARDWARE/modbus/modbus_rtu.c`
  - `HARDWARE/modbus/modbus_slave.c`
  - `HARDWARE/modbus/modbus_master.c`

## 风险与注意事项

1. **主从角色切换**：UART4需要支持动态主从切换
2. **寄存器兼容性**：重构后必须保证与屏幕、分机的寄存器地址完全兼容
3. **分步实施**：建议先完成梳理文档，再逐个串口重构
4. **测试覆盖**：每个串口重构后需要完整通信测试

