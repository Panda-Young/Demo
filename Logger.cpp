/***************************************************************************
 * Description: logger
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-20 02:01:56
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "Logger.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>

LogLevel globalLogLevel = LOG_INFO;
juce::FileLogger *globalLogger = nullptr;

void set_log_level(LogLevel level)
{
    globalLogLevel = level;
}

const char *getLogLevelString(LogLevel level)
{
    switch (level) {
    case LOG_DEBUG:
        return "DEBUG";
    case LOG_INFO:
        return "INFO";
    case LOG_WARN:
        return "WARN";
    case LOG_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string getCurrentTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm *now_tm = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%Y/%m/%d %H:%M:%S");
    return oss.str();
}

void logMsg(juce::FileLogger &logger, LogLevel level, const std::string &message, const char *file, const char *function, int line)
{
    if (level < globalLogLevel) {
        return;
    }
    std::string timeStr = getCurrentTimeString();
    int pid = static_cast<int>(::GetCurrentProcessId());
    std::string logLevelStr = getLogLevelString(level);

    std::ostringstream oss;
    oss << timeStr << " [" << pid << "] " << logLevelStr << " " << file << "@" << function << ":" << line;

    std::string logPrefix = oss.str();
    if (logPrefix.length() < 96) {
        logPrefix.append(96 - logPrefix.length(), ' ');
    }

    logPrefix += " " + message;

    logger.logMessage(logPrefix);
}

extern "C" {
void log_msg_c(LogLevel level, const char *message, const char *file, const char *function, int line)
{
    if (globalLogger != nullptr) {
        logMsg(*globalLogger, level, message, file, function, line);
    }
}
}
