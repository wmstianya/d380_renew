## 1. 配置定义
- [x] 1.1 在 `main.h` 添加分级轮询配置宏
  - `UART4_HEARTBEAT_INTERVAL_MS` (100ms)
  - `UART4_FULLPOLL_INTERVAL_MS` (1000ms)
  - `UART4_HEARTBEAT_REG_ADDR` (100)
  - `UART4_HEARTBEAT_REG_COUNT` (1)
  - `UART4_TIERED_POLLING_ENABLE` (1=启用)

## 2. 数据结构
- [x] 2.1 在 `modbus_uart4.c` 添加分级轮询状态
  - `SlavePollingState` 结构体: lastHeartbeatMs, lastFullPollMs, heartbeatPending, fullPollPending
  - `uart4PollState[12]` 数组存储每个从机状态
  - `uart4WaitingResponse` 等待响应标志

## 3. 心跳轮询实现
- [x] 3.1 实现心跳请求发送 (`uart4SendHeartbeat`)
  - 读取寄存器100，只读1个寄存器
  - 超时时间使用 `UART4_RESP_TIMEOUT_MS`
- [x] 3.2 实现心跳响应处理 (`uart4OnResponse`)
  - 成功: 标记从机在线，触发完整轮询
  - 失败: 累计超时计数，达到阈值判定离线
- [x] 3.3 修改从机寄存器读取支持1个寄存器 (`uart4ReadLocalStatus`)

## 4. 完整轮询实现
- [x] 4.1 修改完整轮询逻辑 (`uart4SendFullRead`, `uart4SendFullWrite`)
  - 仅在从机在线时执行
  - 按 `UART4_FULLPOLL_INTERVAL_MS` 间隔执行
- [x] 4.2 处理异常响应
  - 异常响应不影响在线状态
  - 记录异常码用于诊断

## 5. 调度器整合
- [x] 5.1 修改 `modbusUart4Scheduler` 
  - 实现 `uart4TieredScheduler` 分级轮询调度
  - 心跳优先，完整轮询仅在线执行
  - 通过 `UART4_TIERED_POLLING_ENABLE` 宏切换新旧模式

