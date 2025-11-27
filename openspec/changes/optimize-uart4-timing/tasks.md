# Tasks: 优化UART4时序控制

## Phase 1: 启用动态间隔 ⬜

### 1.1 修改UART4配置
- [ ] 1.1.1 将 `pollIntervalMs` 从 `100` 改为 `0` (启用动态间隔)
- [ ] 1.1.2 将 `respTimeoutMs` 从 `100ms` 改为 `50ms`
- [ ] 1.1.3 编译验证

### 1.2 验证测试
- [ ] 1.2.1 烧录测试，观察分机轮询速度
- [ ] 1.2.2 长时间运行测试（>1小时），监控断联情况
- [ ] 1.2.3 记录通信统计（成功/超时/CRC错误）

## Phase 2: 验证按需写入 ⬜

### 2.1 检查getWriteData回调
- [ ] 2.1.1 验证 `uart4GetWriteData` 是否正确实现按需逻辑
- [ ] 2.1.2 确认只在命令变化时发送写入请求

---

## 快速修改方案

最小改动（仅修改2行）：

**文件**: `HARDWARE/modbus/modbus_uart4.c` 第497-498行

```c
// 修改前:
uart4MasterCfg.pollIntervalMs = 100;
uart4MasterCfg.respTimeoutMs = 100;

// 修改后:
uart4MasterCfg.pollIntervalMs = 0;    // 启用动态间隔
uart4MasterCfg.respTimeoutMs = 50;    // 缩短超时
```

**预期效果**:
- 轮询周期: 1000ms → ~150ms
- 响应延迟: 100ms → <15ms
- 断联风险: 大幅降低

