/***************************************************************************
 * Description: myLogger
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-20 01:29:20
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#pragma once

#include <stdio.h>
#include <string.h>

#ifndef __FILE_NAME__
#define __FILE_NAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__))
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
} LogLevel_t;

#ifdef __cplusplus
#include <JuceHeader.h>
#include <JucePluginDefines.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

class myLogger
{
public:
    static myLogger &getInstance()
    {
        static myLogger instance;
        return instance;
    }

    void logMsg(LogLevel_t level, const std::string &message, const char *file, const char *function, int line);
    void setLogLevel(LogLevel_t level);
    LogLevel_t getLogLevel() const { return currentLogLevel.load(std::memory_order_relaxed); }
    juce::File getLogFile() const { return logFile; }
    juce::File getTempDir() const { return tempDir; }

private:
    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        std::thread::id threadId;
        LogLevel_t level;
        std::string file;
        std::string function;
        int line = 0;
        std::string message;
    };

    myLogger();
    ~myLogger();
    myLogger(const myLogger &) = delete;
    myLogger &operator=(const myLogger &) = delete;

    void initializeLogger();
    void workerLoop();
    std::string formatLogMessage(const LogEntry &entry) const;
    const char *levelToString(LogLevel_t level) const;

    std::unique_ptr<juce::FileLogger> fileLogger;
    juce::File tempDir, logFile;
    std::atomic<LogLevel_t> currentLogLevel;

    std::atomic<bool> workerRunning{false};
    std::thread workerThread;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::deque<LogEntry> logQueue;
    size_t droppedLogCount = 0;
    std::string processIdString;

    static constexpr size_t kMaxQueueSize = 4096;
};

void log_msg(LogLevel_t level, const std::string &message, const char *file, const char *function, int line);
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

void log_msg_c(LogLevel_t level, const char *message, const char *file, const char *function, int line);

#ifdef __cplusplus
}
#endif
