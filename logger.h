/* **************************************************************
 * @Description: logger
 * @Date: 2024-09-23 10:05:00
 * @Version: 0.1.0
 * @Author: pandapan@aactechnologies.com
 * @Copyright (c) 2024 by @AAC Technologies, All Rights Reserved.
 **************************************************************/

#ifndef _LOGGER_H
#define _LOGGER_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <windows.h>

typedef enum {
    LOG_OFF = 0,
    LOG_ERROR,
    LOG_WARNING,
    LOG_NORMAL,
    LOG_DEBUG,
} LOG_TYPE;

#ifndef log_level
#define log_level LOG_DEBUG
#endif

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))
#endif

#define LOG_FILE_NAME "C:\\Users\\60061804\\AppData\\Local\\Temp\\Demo_VST_Plugin.log"

#define LOG(level, level_str, fmt, ...)                                                              \
    do {                                                                                             \
        if (log_level >= level) {                                                                    \
            FILE *log_file = fopen(LOG_FILE_NAME, "a+");                                             \
            if (log_file) {                                                                          \
                SYSTEMTIME st;                                                                       \
                GetLocalTime(&st);                                                                   \
                int pid = (int)GetCurrentProcessId();                                                \
                int tid = (int)GetCurrentThreadId();                                                 \
                char buf[256];                                                                       \
                snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d [%d.%d] %s %s@%s:%d", \
                         st.wYear, st.wMonth, st.wDay,                                               \
                         st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,                         \
                         pid, tid, level_str, __FILENAME__, __FUNCTION__, __LINE__);                 \
                fprintf(log_file, "%-128s" fmt "\n", buf, ##__VA_ARGS__);                            \
                fflush(log_file);                                                                    \
                fclose(log_file);                                                                    \
            }                                                                                        \
        }                                                                                            \
    } while (0)

#define LOGD(fmt, ...) LOG(LOG_DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG(LOG_NORMAL, "INFO", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG(LOG_WARNING, "WARN", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG(LOG_ERROR, "ERROR", fmt, ##__VA_ARGS__)

#endif // _LOGGER_H
