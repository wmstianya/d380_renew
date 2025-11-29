## MODIFIED Requirements

### Requirement: UART4 主机轮询帧间延时

UART4 主机配置 MUST 设置最小帧间延时，以符合 ModBus RTU 标准要求的 3.5 字符时间间隔。

#### Scenario: 正常轮询通信

- Given: UART4 作为主机轮询分机组 1-10
- When: 收到从机响应后准备发送下一帧
- Then: 必须等待至少 5ms 帧间延时后再发送
- And: 配置 `pollIntervalMs = 5` 而非 `0`

```c
/* 修改前 (有问题) */
uart4MasterCfg.pollIntervalMs = 0;     /* 动态间隔: 收到响应立即切换 */

/* 修改后 (符合标准) */
uart4MasterCfg.pollIntervalMs = 5;     /* 最小帧间延时: 5ms (>3.5字符@9600bps) */
```
