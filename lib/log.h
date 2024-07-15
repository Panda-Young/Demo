/***************************************************************************
 * Description: log
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-05-01 16:59:25
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <string.h>

#define LOG_FILE_PATH "C:\\Users\\young\\AppData\\Local\\Temp\\Demo_VST_Plugin.log"

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

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

#define LOGD2(fmt, ...)                                  \
    do {                                                 \
        if (log_level >= LOG_DEBUG) {                    \
            FILE *log_file = fopen(LOG_FILE_PATH, "a+"); \
            if (log_file) {                              \
                fprintf(log_file, fmt, ##__VA_ARGS__);   \
                fflush(log_file);                        \
                fclose(log_file);                        \
            }                                            \
        }                                                \
    } while (0)

#define LOGD(fmt, ...)                                                                                                  \
    do {                                                                                                                \
        if (log_level >= LOG_DEBUG) {                                                                                   \
            FILE *log_file = fopen(LOG_FILE_PATH, "a+");                                                                \
            if (log_file) {                                                                                             \
                fprintf(log_file, "DEBUG   %s@%s(%d): " fmt "\n", __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
                fflush(log_file);                                                                                       \
                fclose(log_file);                                                                                       \
            }                                                                                                           \
        }                                                                                                               \
    } while (0)

#define LOGI(fmt, ...)                                                                                                  \
    do {                                                                                                                \
        if (log_level >= LOG_NORMAL) {                                                                                  \
            FILE *log_file = fopen(LOG_FILE_PATH, "a+");                                                                \
            if (log_file) {                                                                                             \
                fprintf(log_file, "NORMAL  %s@%s(%d): " fmt "\n", __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
                fflush(log_file);                                                                                       \
                fclose(log_file);                                                                                       \
            }                                                                                                           \
        }                                                                                                               \
    } while (0)

#define LOGW(fmt, ...)                                                                                                  \
    do {                                                                                                                \
        if (log_level >= LOG_WARNING) {                                                                                 \
            FILE *log_file = fopen(LOG_FILE_PATH, "a+");                                                                \
            if (log_file) {                                                                                             \
                fprintf(log_file, "WARNING %s@%s(%d): " fmt "\n", __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
                fflush(log_file);                                                                                       \
                fclose(log_file);                                                                                       \
            }                                                                                                           \
        }                                                                                                               \
    } while (0)

#define LOGE(fmt, ...)                                                                                                  \
    do {                                                                                                                \
        if (log_level >= LOG_ERROR) {                                                                                   \
            FILE *log_file = fopen(LOG_FILE_PATH, "a+");                                                                \
            if (log_file) {                                                                                             \
                fprintf(log_file, "ERROR   %s@%s(%d): " fmt "\n", __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
                fflush(log_file);                                                                                       \
                fclose(log_file);                                                                                       \
            }                                                                                                           \
        }                                                                                                               \
    } while (0)

#endif
