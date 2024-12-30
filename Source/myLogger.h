/***************************************************************************
 * Description: myLogger
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-20 01:29:20
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#pragma once

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
#include <JuceHeader.h>
#include <JucePluginDefines.h>
#include <mutex>
#include <string>

class myLogger
{
public:
    static myLogger &getInstance()
    {
        static myLogger instance;
        return instance;
    }

    void logMsg(LogLevel level, const std::string &message, const char *file, const char *function, int line);
    void setLogLevel(LogLevel level);

private:
    myLogger();
    ~myLogger();
    myLogger(const myLogger &) = delete;
    myLogger &operator=(const myLogger &) = delete;

    void initializeLogger();

    std::unique_ptr<juce::FileLogger> fileLogger;
    LogLevel currentLogLevel;
    std::mutex logMutex;
};

void log_msg(LogLevel level, const std::string &message, const char *file, const char *function, int line);
#define LOG_MSG(level, message) log_msg(level, message, __FILE_NAME__, __FUNCTION_NAME__, __LINE__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_MSG_C(level, message) log_msg_c(level, message, __FILE_NAME__, __FUNCTION_NAME__, __LINE__)

#define LOG_MSG_CF(level, format, ...)                                         \
    do {                                                                       \
        char _log_temp_buf[2048] = {0};                                        \
        snprintf(_log_temp_buf, sizeof(_log_temp_buf), format, ##__VA_ARGS__); \
        LOG_MSG_C(level, _log_temp_buf);                                       \
    } while (0)

void log_msg_c(LogLevel level, const char *message, const char *file, const char *function, int line);

#ifdef __cplusplus
}
#endif
