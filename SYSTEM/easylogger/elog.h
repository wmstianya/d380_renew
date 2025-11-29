#ifndef __ELOG_H__
#define __ELOG_H__

#include "elog_cfg.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别定义 */
#define ELOG_LVL_ASSERT                      0
#define ELOG_LVL_ERROR                       1
#define ELOG_LVL_WARN                        2
#define ELOG_LVL_INFO                        3
#define ELOG_LVL_DEBUG                       4
#define ELOG_LVL_VERBOSE                     5

typedef enum {
    ELOG_NO_ERR = 0,
} ElogErrCode;

/* 公共接口 */
ElogErrCode elog_init(void);
void elog_start(void);
void elog_set_output_enabled(bool enable);
void elog_output(uint8_t level, const char *tag, const char *file, const char *func, 
                 const long line, const char *format, ...);

/* 简易宏定义 */
#if ELOG_OUTPUT_LVL >= ELOG_LVL_ASSERT
#define elog_a(tag, ...) elog_output(ELOG_LVL_ASSERT, tag, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define elog_a(tag, ...)
#endif

#if ELOG_OUTPUT_LVL >= ELOG_LVL_ERROR
#define elog_e(tag, ...) elog_output(ELOG_LVL_ERROR, tag, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define elog_e(tag, ...)
#endif

#if ELOG_OUTPUT_LVL >= ELOG_LVL_WARN
#define elog_w(tag, ...) elog_output(ELOG_LVL_WARN, tag, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define elog_w(tag, ...)
#endif

#if ELOG_OUTPUT_LVL >= ELOG_LVL_INFO
#define elog_i(tag, ...) elog_output(ELOG_LVL_INFO, tag, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define elog_i(tag, ...)
#endif

#if ELOG_OUTPUT_LVL >= ELOG_LVL_DEBUG
#define elog_d(tag, ...) elog_output(ELOG_LVL_DEBUG, tag, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define elog_d(tag, ...)
#endif

/* 简单宏 (使用默认TAG) */
#define log_a(...) elog_a("SYS", __VA_ARGS__)
#define log_e(...) elog_e("SYS", __VA_ARGS__)
#define log_w(...) elog_w("SYS", __VA_ARGS__)
#define log_i(...) elog_i("SYS", __VA_ARGS__)
#define log_d(...) elog_d("SYS", __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __ELOG_H__ */
