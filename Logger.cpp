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
#include <juce_core/juce_core.h>

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
        return "INFO ";
    case LOG_WARN:
        return "WARN ";
    case LOG_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string getCurrentTimeString() {
    auto now = juce::Time::getCurrentTime();
    auto millis = now.toMilliseconds();
    auto seconds = millis / 1000;
    auto ms = millis % 1000;

    std::time_t timeT = static_cast<std::time_t>(seconds);
    std::tm localTime;
#if JUCE_WINDOWS
    localtime_s(&localTime, &timeT);
#else
    localtime_r(&timeT, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setw(3) << std::setfill('0') << ms;

    return oss.str();
}

void logMsg(juce::FileLogger *logger, LogLevel level, const std::string &message, const char *file, const char *function, int line)
{
    if (level < globalLogLevel) {
        return;
    }
    std::string timeStr = getCurrentTimeString();
    int pid = static_cast<int>(::GetCurrentProcessId());
    int tid = static_cast<int>(::GetCurrentThreadId());
    std::string logLevelStr = getLogLevelString(level);

    std::ostringstream oss;
    oss << timeStr << " [" << pid << "." << tid << "] " << logLevelStr << " " << file << "@" << function << ":" << line;

    std::string logPrefix = oss.str();
    if (logPrefix.length() < 96) {
        logPrefix.append(96 - logPrefix.length(), ' ');
    }

    logPrefix += " " + message;

    if (logger != nullptr) {
        logger->logMessage(logPrefix);
    }
}

extern "C" {
void log_msg_c(LogLevel level, const char *message, const char *file, const char *function, int line)
{
    if (globalLogger != nullptr) {
        logMsg(globalLogger, level, message, file, function, line);
    }
}
}
