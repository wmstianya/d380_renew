# Change: 移植 EasyLogger 日志库

## Why

用户明确要求引入 **EasyLogger** 日志库，以获得更强大、标准化的日志功能（支持异步、多后端、色彩输出等），替代现有的简易 printf 调试，并用于监控串口健康状况。

## What Changes

- **新建 SYSTEM/easylogger/**: 移植 EasyLogger 核心文件。
  - `elog.h/c`: 核心功能实现。
  - `elog_cfg.h`: 裁剪配置（根据 STM32F103 资源定制）。
  - `elog_port.c`: 移植层，对接 UART1 输出和系统时间戳。
- **集成到 USER/main.c**: 
  - 初始化 EasyLogger (`elog_init`, `elog_start`)。
  - 替换启动信息打印。
  - 添加 UART 统计信息的定时日志输出。

## Impact

- Affected specs: system-logging
- Affected code: `USER/main.c`, `SYSTEM/easylogger/*`

## 验证

- 系统启动输出带有 EasyLogger 格式（时间戳、级别、标签）的日志。
- 串口1输出包含 UART2/UART4 的统计数据。
