#include "elog.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* 移植接口声明 */
extern void elog_port_output(const char *log, size_t size);
extern const char *elog_port_get_time(void);

/* 颜色代码 */
#ifdef ELOG_COLOR_ENABLE
#define CSI_START                      "\033["
#define CSI_END                        "\033[0m"
#define F_RED                          "31m"
#define F_GREEN                        "32m"
#define F_YELLOW                       "33m"
#define F_BLUE                         "34m"
#define F_MAGENTA                      "35m"
#define F_CYAN                         "36m"
#define F_WHITE                        "37m"
#endif

/* 全局状态 */
static bool init_ok = false;
static bool output_enabled = true;
static char log_buf[ELOG_LINE_BUF_SIZE];

ElogErrCode elog_init(void)
{
    init_ok = true;
    return ELOG_NO_ERR;
}

void elog_start(void)
{
    if (!init_ok) return;
    output_enabled = true;
}

void elog_set_output_enabled(bool enable)
{
    output_enabled = enable;
}

void elog_output(uint8_t level, const char *tag, const char *file, const char *func, 
                 const long line, const char *format, ...)
{
    size_t len = 0;
    va_list args;
#ifdef ELOG_COLOR_ENABLE
    const char *color = "";
#endif
    
    if (!init_ok || !output_enabled) return;
    
    /* 1. 时间戳 */
#ifdef ELOG_OUTPUT_TIME_ENABLE
    len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "%s ", elog_port_get_time());
#endif

    /* 2. 级别与颜色 */
#ifdef ELOG_COLOR_ENABLE
    switch (level) {
        case ELOG_LVL_ASSERT: color = CSI_START F_MAGENTA; break;
        case ELOG_LVL_ERROR:  color = CSI_START F_RED; break;
        case ELOG_LVL_WARN:   color = CSI_START F_YELLOW; break;
        case ELOG_LVL_INFO:   color = CSI_START F_CYAN; break;
        case ELOG_LVL_DEBUG:  color = CSI_START F_GREEN; break;
        default: break;
    }
    len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "%s", color);
#endif

    switch (level) {
        case ELOG_LVL_ASSERT: len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "[A]"); break;
        case ELOG_LVL_ERROR:  len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "[E]"); break;
        case ELOG_LVL_WARN:   len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "[W]"); break;
        case ELOG_LVL_INFO:   len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "[I]"); break;
        case ELOG_LVL_DEBUG:  len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "[D]"); break;
        case ELOG_LVL_VERBOSE:len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "[V]"); break;
    }

    /* 3. 标签 */
#ifdef ELOG_OUTPUT_TAG_ENABLE
    if (tag) {
        len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "/%s", tag);
    }
#endif

    len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, ": ");

    /* 4. 内容 */
    va_start(args, format);
    len += vsnprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, format, args);
    va_end(args);

    /* 5. 颜色恢复 */
#ifdef ELOG_COLOR_ENABLE
    len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "%s", CSI_END);
#endif

    /* 6. 换行 */
    len += snprintf(log_buf + len, ELOG_LINE_BUF_SIZE - len, "%s", ELOG_NEWLINE_SIGN);

    /* 确保缓冲区不溢出并以null结尾 (snprintf会自动处理，但以防万一) */
    if (len >= ELOG_LINE_BUF_SIZE) {
        len = ELOG_LINE_BUF_SIZE - 1;
        log_buf[len] = '\0';
    }

    /* 输出 */
    elog_port_output(log_buf, len);
}
