/***************************************************************************
 * Description:
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-11-22 20:01:57
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "myLogger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

myLogger::myLogger()
    : currentLogLevel(LOG_INFO)
{
    initializeLogger();
}

myLogger::~myLogger()
{
}

void myLogger::initializeLogger()
{
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    char logFileName[64] = JucePlugin_Name;
    strcat(logFileName, "_VST_Plugin.log");
    juce::File logFile = tempDir.getChildFile(juce::String(logFileName));
    char logStartMsg[128] = {0};
    sprintf(logStartMsg, "%s VST Plugin %s", JucePlugin_Name, JucePlugin_VersionString);
    fileLogger = std::make_unique<juce::FileLogger>(logFile, logStartMsg);
}

void myLogger::logMsg(LogLevel level, const std::string &message, const char *file, const char *function, int line)
{
    if (level < currentLogLevel || fileLogger == nullptr) {
        return;
    }
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm now_tm;
#ifdef _WIN32
    localtime_s(&now_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &now_tm);
#endif
    std::ostringstream timestamp;
    timestamp << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

    auto thread_id = std::this_thread::get_id();
    std::ostringstream thread_id_str;
    thread_id_str << thread_id;

    std::string level_str;
    switch (level) {
    case LOG_DEBUG:
        level_str = "DEBUG";
        break;
    case LOG_INFO:
        level_str = "INFO";
        break;
    case LOG_WARN:
        level_str = "WARN";
        break;
    case LOG_ERROR:
        level_str = "ERROR";
        break;
    default:
        level_str = "UNKNOWN";
        break;
    }

    std::ostringstream logPrefix;
    logPrefix << timestamp.str() << " [" << thread_id_str.str() << "] " << level_str << " "
              << file << "@" << function << ":" << line;

    std::string logPrefixStr = logPrefix.str();
    if (logPrefixStr.length() < 96) {
        logPrefixStr.append(96 - logPrefixStr.length(), ' ');
    }

    std::ostringstream logMessage;
    logMessage << logPrefixStr << " " << message;

    {
        std::lock_guard<std::mutex> lock(logMutex);
        fileLogger->logMessage(logMessage.str());
    }
}

void myLogger::setLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(logMutex);
    currentLogLevel = level;
}

void log_msg(LogLevel level, const std::string &message, const char *file, const char *function, int line)
{
    myLogger::getInstance().logMsg(level, message, file, function, line);
}

extern "C" {

void log_msg_c(LogLevel level, const char *message, const char *file, const char *function, int line)
{
    log_msg(level, message, file, function, line);
}
}
