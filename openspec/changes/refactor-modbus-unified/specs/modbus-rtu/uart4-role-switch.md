# UART4 主从角色切换分析

## 概述

UART4 在系统中扮演 **双重角色**：
1. **主机模式**: 主动轮询分机1-10，收集状态、下发控制命令
2. **从机模式**: 被动响应联控主机的查询和控制

---

## 角色切换机制

### 关键发现：无显式切换

```c
// 主机发送和从机响应是 **并行处理** 的，不存在显式的主从切换
// 而是通过 **地址过滤** 来区分：

// 主机模式：主动发送给分机地址1-10
ModBus4_RTU_Read03(address, 100, 18);  // address = 1~10

// 从机模式：只响应自己的地址
if (U4_Inf.RX_Data[0] == sys_flag.Address_Number) {
    UART4_Server_Cmd_Analyse();  // 响应联控主机
}
```

### 处理流程

```
┌─────────────────────────────────────────────────────────────┐
│                    main() 主循环                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Union_Modbus4_UnionTx_Communication()               │   │
│  │ 主机模式: 轮询发送请求给分机                          │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↓                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ ModBus_Uart4_Local_Communication()                  │   │
│  │ 从机模式: 检查是否有联控主机请求，响应之             │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↓                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Union_ModBus_Uart4_Local_Communication()            │   │
│  │ 主机模式: 处理分机的响应数据                         │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 主机模式详解

### 轮询状态机

```c
// Union_Modbus4_UnionTx_Communication()
static uint8 Address = 1;  // 当前轮询的分机地址

switch (Address) {
    case 1: if (JiZu_ReadAndWrite_Function(1)) Address++; break;
    case 2: if (JiZu_ReadAndWrite_Function(2)) Address++; break;
    ...
    case 5: if (JiZu_ReadAndWrite_Function(5)) Address = 1; break;
}
```

### 读写交替机制

```c
// JiZu_ReadAndWrite_Function(address)
if (SlaveG[address].Send_Index) {
    // 上次是读取，这次写入
    SlaveG[address].Send_Index = 0;
    ModBus4RTU_Write0x10Function(address, 200, 18);
} else {
    // 上次是写入，这次读取
    SlaveG[address].Send_Index = OK;
    ModBus4_RTU_Read03(address, 100, 18);
}
```

### 时序图

```
时间 ──────────────────────────────────────────────────▶

分机1: [读取] ─100ms─ [写入] ─100ms─ [读取] ...
分机2:         [读取] ─100ms─ [写入] ─100ms─ ...
分机3:                 [读取] ─100ms─ [写入] ...
...

完整轮询周期 = 5分机 × 2操作 × 100ms = 1秒
```

---

## 从机模式详解

### 地址过滤

```c
// ModBus_Uart4_Local_Communication() / UART4_Server_Cmd_Analyse()
Device_ID = sys_flag.Address_Number;  // 本机地址

// 只响应发给自己的请求
if (U4_Inf.RX_Data[0] == Device_ID) {
    // 处理请求
}
```

### 支持的功能

| 功能码 | 地址 | 功能 |
|--------|------|------|
| 0x03 | 100 | 返回本机状态 (18寄存器) |
| 0x10 | 0xC8 | 接收控制命令 |

---

## 冲突处理

### 问题：半双工总线冲突

由于UART4同时作为主机和从机，可能出现：
1. 正在发送主机请求时，收到联控主机的请求
2. 正在响应联控主机时，轮询定时器触发

### 当前实现：轮询间隔保护

```c
// 100ms 轮询间隔提供时间窗口
case 1:
    if (U4_Inf.Flag_100ms) {  // 等待100ms
        U4_Inf.Flag_100ms = 0;
        index = 0;
        return_value = OK;
    }
    break;
```

### 时序分析

```
100ms 时间窗口分配:
├── 主机发送请求: ~2ms (8字节 @ 9600)
├── 等待分机响应: ~10ms
├── 处理响应数据: ~1ms
├── 空闲时间: ~87ms  ← 联控主机可在此期间请求
└── 下次轮询
```

---

## 改进建议

### 1. 显式状态机

```c
typedef enum {
    UART4_IDLE,
    UART4_MASTER_TX,
    UART4_MASTER_WAIT_RX,
    UART4_SLAVE_RX,
    UART4_SLAVE_TX
} Uart4State;

static Uart4State uart4State = UART4_IDLE;
```

### 2. 优先级控制

```c
// 联控主机优先级高于分机轮询
if (联控主机有请求) {
    暂停分机轮询;
    响应联控主机;
    恢复分机轮询;
}
```

### 3. 统一调度

```c
// 统一在一个函数中处理主从逻辑
void Uart4_ModbusScheduler(void) {
    // 1. 检查是否有从机请求需要响应
    if (hasSlaveRequest()) {
        processSlaveRequest();
        return;
    }
    
    // 2. 执行主机轮询
    processMasterPolling();
}
```

---

## 数据结构

### SlaveG (分机通信状态)

```c
typedef struct {
    uint8_t Alive_Flag;        // 在线标志
    uint8_t Send_Flag;         // 发送计数 (>20离线)
    uint8_t Send_Index;        // 读/写切换 (0=读, 1=写)
    uint8_t Command_SendFlag;  // 命令发送标志
    uint8_t Out_Power;         // 输出功率
    // ...
} SLAVE_GG;

SLAVE_GG SlaveG[13];  // 1-10对应分机, 0和11-12未用
```

### U4_Inf (UART4缓冲)

```c
typedef struct {
    uint8_t TX_Data[300];
    uint8_t RX_Data[300];
    uint16_t TX_Length;
    uint16_t RX_Length;
    uint8_t Recive_Ok_Flag;
    uint8_t Flag_100ms;        // 100ms轮询定时标志
} UART_INF;
```

---

## 总结

| 特性 | 当前实现 |
|------|----------|
| 切换方式 | 无显式切换，并行处理 |
| 冲突处理 | 依赖100ms间隔的时间窗口 |
| 主机轮询 | 1-5分机循环，读写交替 |
| 从机响应 | 地址过滤，按需响应 |
| 改进空间 | 可引入显式状态机提高可靠性 |

