#ifndef _ELOG_CFG_H_
#define _ELOG_CFG_H_

/* 启用日志输出 */
#define ELOG_OUTPUT_ENABLE

/* 设置日志级别: 0:Off, 1:Error, 2:Warn, 3:Info, 4:Debug, 5:Verbose */
#define ELOG_OUTPUT_LVL                      3

/* 启用断言检查 */
#define ELOG_ASSERT_ENABLE

/* 单行日志缓冲区大小 */
#define ELOG_LINE_BUF_SIZE                   256

/* 换行符 (Windows/STM32常用 \r\n) */
#define ELOG_NEWLINE_SIGN                    "\r\n"

/* 启用颜色输出 (需终端支持ANSI转义序列) */
/* #define ELOG_COLOR_ENABLE */  /* 禁用: 串口工具不支持ANSI */

/* 启用时间戳输出 */
#define ELOG_OUTPUT_TIME_ENABLE

/* 启用标签输出 */
#define ELOG_OUTPUT_TAG_ENABLE

#endif /* _ELOG_CFG_H_ */
