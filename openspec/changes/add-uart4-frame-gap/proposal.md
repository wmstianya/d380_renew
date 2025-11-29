# Change Proposal: UART4 添加 ModBus 帧间延时

## Summary

修复 UART4 动态间隔模式下缺少 3.5 字符时间帧间延时的问题，确保符合 ModBus RTU 标准。

## Problem

当前 UART4 配置为动态间隔模式 (`pollIntervalMs = 0`)，收到从机响应后**立即**发送下一帧请求：

```c
if (cfg->pollIntervalMs == 0) {
    masterNextAddress(handle);
    handle->state = MODBUS_STATE_IDLE;  // 立即切换，无延时
}
```

**违反 ModBus RTU 标准**：两帧之间必须至少间隔 3.5 字符时间。

**导致的问题**：
1. 从机可能还在发送最后字节，主机新请求导致总线冲突
2. 从机未完成状态切换，无法正确接收新请求
3. 数据错位、CRC 错误、通信超时

## Solution

在 `modbus_uart4.c` 中为 UART4 配置最小帧间延时：

1. 定义最小帧间延时常量 `UART4_T35_MS = 5` (9600bps 下 3.5 字符 ≈ 4ms，留余量)
2. 将 `pollIntervalMs` 从 0 改为 `UART4_T35_MS`
3. 动态间隔特性保留：收到响应后等待最小间隔即切换，而非固定长间隔

## Impact

- **文件**: `HARDWARE/modbus/modbus_uart4.c`
- **风险**: 低 - 仅增加 5ms 延时，不影响功能
- **兼容性**: 完全向后兼容

## Specs Modified

- `modbus-unified`: 更新 UART4 主机配置说明

