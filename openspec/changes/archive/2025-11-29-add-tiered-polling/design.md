## Context

UART4连接联控板，作为主机轮询分机1-10的状态。当前实现每次发送完整的读写请求（18寄存器），导致：
- 从机不支持的寄存器返回异常响应
- 状态判断不稳定，频繁在线/离线切换

## Goals / Non-Goals

**Goals:**
- 快速检测从机在线状态（<200ms）
- 减少异常响应，提高通讯效率
- 状态稳定，避免频繁切换

**Non-Goals:**
- 不改变现有寄存器映射
- 不修改从机协议

## Decisions

### 决策1: 心跳寄存器选择

使用寄存器100（设备状态），读取1个寄存器：
- 这是从机必定支持的寄存器
- 数据量最小（8字节请求+7字节响应）
- 响应时间快

**请求帧**: `01 03 00 64 00 01 C5 D5` (8字节)
**响应帧**: `01 03 02 XX XX CRC` (7字节)

### 决策2: 轮询时序

```
时间线 (ms):
0    100   200   300   400   500   600   700   800   900   1000
|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
 HB1   HB2   HB3   HB4   HB5   HB6   HB7   HB8   HB9   HB10  FULL

HB = 心跳 (快速检测)
FULL = 完整轮询 (数据同步)
```

### 决策3: 状态机

```
┌─────────────────────────────────────────────────────────────┐
│                        从机状态机                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌─────────┐  心跳成功×2   ┌─────────┐                     │
│   │  离线   │─────────────→│  在线   │                     │
│   │ OFFLINE │              │ ONLINE  │                     │
│   └────┬────┘              └────┬────┘                     │
│        │                        │                          │
│        │  心跳超时×5            │  心跳超时×5               │
│        │  (保持离线)            │                          │
│        └────────────────────────┘                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 决策4: 完整轮询触发条件

- 从机在线 (`Alive_Flag == OK`)
- 距离上次完整轮询 >= 1000ms
- 当前轮询周期为"完整轮询"周期

## Risks / Trade-offs

| 风险 | 缓解措施 |
|------|----------|
| 心跳寄存器不被支持 | 使用最基础的寄存器100 |
| 完整轮询间隔太长 | 可配置，默认1000ms |
| 状态切换延迟 | 心跳100ms，最快200ms检测到离线 |

## Implementation Notes

### 关键数据结构

```c
/* 分级轮询状态 */
typedef struct {
    uint32_t lastHeartbeatMs;   /* 上次心跳时间 */
    uint32_t lastFullPollMs;    /* 上次完整轮询时间 */
    uint8_t  heartbeatFails;    /* 连续心跳失败次数 */
    uint8_t  pollMode;          /* 0=心跳, 1=完整 */
} SlavePollingState;

static SlavePollingState uart4PollState[12];
```

### 调度逻辑

```c
void modbusUart4Scheduler(void)
{
    uint32_t now = HAL_GetTick();
    
    for (uint8_t i = 1; i <= UART4_POLL_COUNT; i++) {
        /* 心跳优先 */
        if (now - uart4PollState[i].lastHeartbeatMs >= UART4_HEARTBEAT_INTERVAL_MS) {
            sendHeartbeat(i);
            uart4PollState[i].lastHeartbeatMs = now;
        }
        
        /* 在线时执行完整轮询 */
        if (SlaveG[i].Alive_Flag == OK) {
            if (now - uart4PollState[i].lastFullPollMs >= UART4_FULLPOLL_INTERVAL_MS) {
                sendFullPoll(i);
                uart4PollState[i].lastFullPollMs = now;
            }
        }
    }
}
```

