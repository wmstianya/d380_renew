# UART4 时序优化设计

## 一、当前问题分析

### 1.1 现有实现

```
当前轮询模式 (5分机):
分机1: [读取]─100ms─[写入]─100ms─[读取]...
分机2: [读取]─100ms─[写入]─100ms─[读取]...
...
分机5: [读取]─100ms─[写入]─100ms─[读取]...

完整周期: 5分机 × 2操作 × 100ms = 1000ms
每秒通信次数: 10次
```

### 1.2 问题清单

| 问题 | 描述 | 影响 |
|------|------|------|
| **固定读写交替** | 无论命令是否变化，都执行写入 | 50%带宽浪费 |
| **固定100ms间隔** | 响应快也要等100ms | 响应延迟大 |
| **无优先级区分** | 联控主机和分机轮询同等处理 | 联控主机响应可能延迟 |
| **隐式状态** | 依赖Send_Index切换读写 | 逻辑不清晰 |

### 1.3 时序冲突场景

```
场景: 联控主机在主机发送期间请求

时间轴:
0ms   ├── 联控板发送读取请求给分机1
2ms   ├── 联控主机发送请求给联控板  ← 冲突!
      │   (此时联控板正在发送，无法接收)
10ms  ├── 分机1响应
      │   联控主机请求丢失!
```

---

## 二、优化方案

### 方案A: 按需写入 (推荐)

**核心思想**: 只在控制命令变化时才写入，平时只读取

```c
// 优化前: 每次都读写交替
读→写→读→写→读→写...

// 优化后: 读为主，按需写
读→读→读→[命令变化]→写→读→读→读...
```

**实现**:

```c
typedef struct {
    uint8_t needWrite;       // 是否需要写入
    uint8_t lastStartFlag;   // 上次启停状态
    uint8_t lastPower;       // 上次功率
    uint16_t lastRelays;     // 上次继电器状态
} SlaveWriteCache;

uint8_t checkNeedWrite(uint8_t addr) {
    if (JiZu[addr].Slave_D.StartFlag != writeCache[addr].lastStartFlag)
        return TRUE;
    if (SlaveG[addr].Out_Power != writeCache[addr].lastPower)
        return TRUE;
    if (JiZu[addr].Slave_D.Realys_Out != writeCache[addr].lastRelays)
        return TRUE;
    return FALSE;
}
```

**收益**:

| 指标 | 优化前 | 优化后 | 改进 |
|------|--------|--------|------|
| 通信次数/秒 | 10次 | 5次(稳态) | **-50%** |
| 轮询周期 | 1000ms | 500ms | **2x faster** |
| CPU负载 | 高 | 低 | **-50%** |

---

### 方案B: 动态间隔

**核心思想**: 收到响应立即切换，超时才等100ms

```c
// 优化前: 固定100ms
发送 → [等待100ms] → 下一个

// 优化后: 动态等待
发送 → [收到响应/10ms超时] → 下一个
```

**实现**:

```c
typedef enum {
    POLL_IDLE,
    POLL_WAIT_RESPONSE,
    POLL_TIMEOUT
} PollState;

void pollStateMachine(void) {
    switch (pollState) {
        case POLL_IDLE:
            sendRequest(currentAddr);
            pollState = POLL_WAIT_RESPONSE;
            startTimeout(RESPONSE_TIMEOUT_MS);  // 10ms
            break;
            
        case POLL_WAIT_RESPONSE:
            if (hasResponse()) {
                processResponse();
                pollState = POLL_IDLE;  // 立即下一个
                nextAddress();
            } else if (isTimeout()) {
                pollState = POLL_IDLE;
                markOffline(currentAddr);
                nextAddress();
            }
            break;
    }
}
```

**收益**:

| 指标 | 优化前 | 优化后 | 改进 |
|------|--------|--------|------|
| 单次通信耗时 | 100ms固定 | ~15ms实际 | **6x faster** |
| 轮询周期 | 1000ms | ~150ms | **6x faster** |

---

### 方案C: 时间窗口划分 (解决冲突)

**核心思想**: 为联控主机预留响应窗口

```
每100ms周期划分:
├── 0-80ms: 主机轮询窗口 (轮询分机)
└── 80-100ms: 从机响应窗口 (响应联控主机)

时序图:
0ms    ├─────── 主机轮询分机1 ───────┤
15ms   ├─────── 主机轮询分机2 ───────┤
...
75ms   ├─────── 主机轮询分机5 ───────┤
80ms   ├══════ 从机窗口(检查联控主机请求) ══════┤
100ms  └───────────────────────────────────────┘
```

**实现**:

```c
#define MASTER_WINDOW_MS    80
#define SLAVE_WINDOW_MS     20

void uart4Scheduler(void) {
    uint32_t cycleTime = getSysTick() % 100;
    
    if (cycleTime < MASTER_WINDOW_MS) {
        // 主机窗口: 轮询分机
        masterPolling();
    } else {
        // 从机窗口: 检查并响应联控主机
        if (hasSlaveRequest()) {
            processSlaveRequest();
        }
    }
}
```

---

### 方案D: 显式状态机 (清晰架构)

```c
typedef enum {
    UART4_IDLE,
    UART4_MASTER_SENDING,
    UART4_MASTER_WAITING,
    UART4_SLAVE_RECEIVING,
    UART4_SLAVE_RESPONDING
} Uart4State;

typedef struct {
    Uart4State state;
    uint8_t currentSlaveAddr;
    uint8_t pollIndex;
    uint32_t lastSendTime;
    uint8_t retryCount;
} Uart4Context;

void uart4StateMachine(Uart4Context* ctx) {
    switch (ctx->state) {
        case UART4_IDLE:
            // 检查是否有从机请求 (高优先级)
            if (hasSlaveRequest()) {
                ctx->state = UART4_SLAVE_RECEIVING;
                break;
            }
            // 执行主机轮询
            ctx->state = UART4_MASTER_SENDING;
            break;
            
        case UART4_MASTER_SENDING:
            sendMasterRequest(ctx->currentSlaveAddr);
            ctx->lastSendTime = getSysTick();
            ctx->state = UART4_MASTER_WAITING;
            break;
            
        case UART4_MASTER_WAITING:
            if (hasResponse()) {
                processMasterResponse(ctx->currentSlaveAddr);
                ctx->state = UART4_IDLE;
                nextPollAddress(ctx);
            } else if (isTimeout(ctx->lastSendTime, 10)) {
                handleTimeout(ctx);
                ctx->state = UART4_IDLE;
                nextPollAddress(ctx);
            }
            break;
            
        case UART4_SLAVE_RECEIVING:
            parseSlaveRequest();
            ctx->state = UART4_SLAVE_RESPONDING;
            break;
            
        case UART4_SLAVE_RESPONDING:
            sendSlaveResponse();
            ctx->state = UART4_IDLE;
            break;
    }
}
```

---

## 三、推荐组合方案

**最佳实践: A + B + D 组合**

```
┌─────────────────────────────────────────────────────────┐
│                   显式状态机 (方案D)                     │
│  ┌─────────────────────────────────────────────────┐   │
│  │ IDLE → MASTER_SEND → MASTER_WAIT → IDLE        │   │
│  │   ↓                                             │   │
│  │ SLAVE_RX → SLAVE_TX → IDLE                     │   │
│  └─────────────────────────────────────────────────┘   │
│                        │                               │
│         ┌──────────────┼──────────────┐               │
│         ▼              ▼              ▼               │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐        │
│  │ 按需写入   │ │ 动态间隔   │ │ 优先级控制  │        │
│  │ (方案A)   │ │ (方案B)   │ │            │        │
│  └────────────┘ └────────────┘ └────────────┘        │
└─────────────────────────────────────────────────────────┘
```

### 预期收益

| 指标 | 当前 | 优化后 | 改进 |
|------|------|--------|------|
| 轮询周期 | 1000ms | **75ms** | **13x faster** |
| 通信次数/秒 | 10次 | **5次** (稳态) | **-50%** |
| 联控主机响应延迟 | 0-100ms | **<15ms** | **>6x** |
| 冲突概率 | ~20% | **<1%** | **-95%** |

---

## 四、实现优先级建议

| 阶段 | 方案 | 复杂度 | 收益 | 建议 |
|------|------|--------|------|------|
| **1** | 方案A (按需写入) | 低 | 高 | **优先实现** |
| **2** | 方案B (动态间隔) | 中 | 高 | 紧随其后 |
| **3** | 方案D (状态机) | 中 | 中 | 重构时实现 |
| **4** | 方案C (时间窗口) | 低 | 中 | 可选 |

---

## 五、与统一协议层的关系

这些优化可以作为统一ModBus协议层的一部分实现：

```c
// 统一协议层接口
typedef struct {
    UartHandle* uart;           // DMA驱动句柄
    ModbusRole role;            // MASTER / SLAVE / DUAL
    ModbusState state;          // 状态机状态
    
    // 主机配置
    uint8_t pollAddresses[16];  // 轮询地址列表
    uint8_t pollCount;          // 轮询地址数量
    WriteCache writeCache[16];  // 写入缓存(按需写入)
    
    // 从机配置
    uint8_t slaveAddress;       // 本机地址
    
    // 回调
    ModbusReadCallback onRead;
    ModbusWriteCallback onWrite;
} ModbusHandle;

// 统一调度器
void modbusScheduler(ModbusHandle* handle);
```

---

## 六、下一步行动

1. **评估**: 确认哪些优化方案需要实现
2. **设计**: 将选定方案纳入统一协议层设计
3. **实现**: 在Phase 3中一并实现

