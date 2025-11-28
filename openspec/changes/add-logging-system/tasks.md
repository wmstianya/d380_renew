# Tasks: 移植 EasyLogger

## 1. 库移植

- [ ] 1.1 创建 `SYSTEM/easylogger` 目录
- [ ] 1.2 创建 `elog_cfg.h`：配置日志级别、缓冲区大小、输出格式（启用时间、级别、标签）
- [ ] 1.3 创建 `elog.h` 和 `elog.c`：实现核心 API (`elog_i`, `elog_e` 等)
- [ ] 1.4 创建 `elog_port.c`：实现 `elog_port_output` (对接 `u1_printf`) 和初始化

## 2. 系统集成

- [ ] 2.1 在 `USER/main.c` 中初始化 EasyLogger
- [ ] 2.2 在 `USER/main.c` 主循环中添加 UART 统计信息打印任务 (使用 `log_i`)

## 3. 验证

- [ ] 3.1 编译无误
- [ ] 3.2 确认串口1输出标准化的日志信息
