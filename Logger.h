/***************************************************************************
 * Description: Logger
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-20 01:29:20
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#pragma once

#ifdef __cplusplus
#include <JuceHeader.h>
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

#ifndef __FUNCTION_NAME__
#define __FUNCTION_NAME__ (strrchr(__FUNCTION__, ':') ? strrchr(__FUNCTION__, ':') + 1 : __FUNCTION__)
#endif

typedef enum LogLevel {
    LOG_DEBUG = 1,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_OFF
} LogLevel;

#ifdef __cplusplus
extern juce::FileLogger *globalLogger;
extern "C" {
#endif

extern LogLevel globalLogLevel;

#define LOG_MSG_C(level, message) log_msg_c(level, message, __FILE_NAME__, __FUNCTION_NAME__, __LINE__)

void log_msg_c(LogLevel level, const char *message, const char *file, const char *function, int line);

#define LOG_MSG_CF(level, format, ...)                                       \
    do {                                                                     \
        char log_temp_buf[256] = {0};                                        \
        snprintf(log_temp_buf, sizeof(log_temp_buf), format, ##__VA_ARGS__); \
        LOG_MSG_C(level, log_temp_buf);                                      \
    } while (0)

#ifdef __cplusplus
}
void set_log_level(LogLevel level);
void logMsg(juce::FileLogger &logger, LogLevel level, const std::string &message, const char *file, const char *function, int line);
#define LOG_MSG(level, message) logMsg(*globalLogger, level, message, __FILE_NAME__, __FUNCTION_NAME__, __LINE__)
#endif
