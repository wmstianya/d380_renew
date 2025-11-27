# Change: 统一ModBus RTU协议层重构

**状态**: ✅ Phase 4 实现完成，待硬件测试

## Why

当前系统存在以下问题：
- 4个串口各自实现一套ModBus RTU逻辑（usart.c, usart2.c, usart3.c, usart4.c）
- 代码重复率高，维护困难
- 新增串口需要复制整个文件
- Bug修复需要改4个地方
- 主从角色逻辑分散，难以理解

## What Changes

### Phase 1: 寄存器地址梳理 ✅ 完成

| 串口 | 角色 | 寄存器范围 | 用途 | 文档 |
|------|------|------------|------|------|
| USART1 | 从机 | 0/0x63-0xC7/1000 | RTU服务器响应 | ✅ |
| USART2 | 从机 | 0x0000-0x0117 | 响应屏幕请求 | ✅ |
| USART3 | 主机 | 0x0000-0x0001 | 轮询供水阀 | ✅ |
| UART4 | **双角色** | 100/200 | 主从双角色 | ✅ |

### Phase 2: 主从角色分析 ✅ 完成

**联控板角色变化**：
```
屏幕 ──(USART2)──> 联控板(从机) ──(UART4/主机)──> 分机1~10
                        │
                        └── UART4也可作为从机响应上级联控主机
```

### Phase 3: 统一ModBus协议层 ✅ 完成

```
HARDWARE/modbus/
├── modbus_types.h      # 类型定义
├── modbus.h            # 公共接口
├── modbus_core.c       # CRC计算、初始化、调度器
├── modbus_slave.c      # 从机处理 (03/06/10)
├── modbus_master.c     # 主机轮询 (按需写入+动态间隔)
└── modbus_port.c       # 硬件抽象
```

### Phase 4: 业务层重构 ✅ 完成

各串口配置文件：
```
HARDWARE/modbus/
├── modbus_usart1.c     # USART1 (从机-RTU服务器)
├── modbus_usart2.c     # USART2 (从机-10.1屏幕)
├── modbus_usart3.c     # USART3 (主机-供水阀)
└── modbus_uart4.c      # UART4 (双角色-联控)
```

## Impact

- **Affected code**: 
  - `SYSTEM/usart/usart.c` - 保留，条件编译切换
  - `HARDWARE/USART2/usart2.c` - 保留，条件编译切换
  - `HARDWARE/USART3/usart3.c` - 保留，条件编译切换
  - `HARDWARE/USART4/usart4.c` - 保留，条件编译切换
  
- **New modules** (10个文件):
  - `HARDWARE/modbus/modbus_types.h`
  - `HARDWARE/modbus/modbus.h`
  - `HARDWARE/modbus/modbus_core.c`
  - `HARDWARE/modbus/modbus_slave.c`
  - `HARDWARE/modbus/modbus_master.c`
  - `HARDWARE/modbus/modbus_port.c`
  - `HARDWARE/modbus/modbus_usart1.c`
  - `HARDWARE/modbus/modbus_usart2.c`
  - `HARDWARE/modbus/modbus_usart3.c`
  - `HARDWARE/modbus/modbus_uart4.c`

- **Configuration**:
  - `USER/main.c` - 添加条件编译开关 `USE_UNIFIED_MODBUS`
  - `USER/UART.uvprojx` - 添加MODBUS文件组

## 实现成果

### 代码统计

| 项目 | 数值 |
|------|------|
| 新增文件 | 10个 |
| 代码行数 | ~2500行 |
| 编译大小 | ~54KB |
| 编译错误 | 0 |
| 编译警告 | 4 |

### 功能对比

| 功能 | 旧实现 | 新实现 |
|------|--------|--------|
| 从机处理 | 4处重复 | 统一modbus_slave.c |
| 主机轮询 | 2处重复 | 统一modbus_master.c |
| CRC校验 | 分散 | 统一ModBusCRC16 |
| 寄存器映射 | 硬编码 | 回调函数表 |
| 角色切换 | 隐式 | 显式MODBUS_ROLE_DUAL |

### 优化特性

1. **按需写入**: 只在命令变化时发送写入请求
2. **动态间隔**: 收到响应后立即切换，无固定等待
3. **离线检测**: 连续失败自动标记离线
4. **统一调度**: modbusScheduler() 单一入口

## 切换方式

```c
// USER/main.c
#define USE_UNIFIED_MODBUS 1  // 1=使用新协议层, 0=使用旧代码
```

## 待办事项

1. **硬件测试** - 验证与实际设备的通信
2. **回归测试** - 确保所有功能正常
3. **长时间稳定性测试** - 连续运行验证
4. **清理旧代码** - 测试通过后可移除条件编译

## 风险与注意事项

1. **主从角色切换**：UART4已实现MODBUS_ROLE_DUAL支持
2. **寄存器兼容性**：已按原有地址表实现，保证兼容
3. **快速回退**：通过USE_UNIFIED_MODBUS=0可立即回退
4. **测试覆盖**：建议逐串口进行硬件通信测试
