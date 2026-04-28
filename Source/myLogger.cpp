/***************************************************************************
 * Description:
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-11-22 20:01:57
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#include "myLogger.h"
#include <iomanip>
#include <sstream>

#if JUCE_WINDOWS
#include <Windows.h> // for GetCurrentProcessId
#else
#include <unistd.h>
#endif

myLogger::myLogger()
    : currentLogLevel(LOG_INFO)
{
    initializeLogger();
    if (fileLogger != nullptr) {
        workerRunning.store(true, std::memory_order_relaxed);
        workerThread = std::thread(&myLogger::workerLoop, this);
    }
}

myLogger::~myLogger()
{
    workerRunning.store(false, std::memory_order_relaxed);
    queueCv.notify_all();
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void myLogger::initializeLogger()
{
    tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File chosenDir = tempDir;
    if (!chosenDir.hasWriteAccess()) {
        chosenDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        chosenDir.createDirectory();
    }

    juce::String logFileName = juce::String(JucePlugin_Name) + "_VST_Plugin.log";
    logFile = chosenDir.getChildFile(logFileName);
    juce::String logStartMsg = juce::String(JucePlugin_Name) + " VST Plugin " + juce::String(JucePlugin_VersionString);
    fileLogger = std::make_unique<juce::FileLogger>(logFile, logStartMsg.toRawUTF8());

#if JUCE_WINDOWS
    processIdString = std::to_string(GetCurrentProcessId());
#else
    processIdString = std::to_string(getpid());
#endif
}

void myLogger::logMsg(LogLevel_t level, const std::string &message, const char *file, const char *function, int line)
{
    if (level < currentLogLevel.load(std::memory_order_relaxed) || fileLogger == nullptr) {
        return;
    }

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.threadId = std::this_thread::get_id();
    entry.level = level;
    entry.file = (file != nullptr) ? file : "unknown_file";
    entry.function = (function != nullptr) ? function : "unknown_function";
    entry.line = line;
    entry.message = message;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (logQueue.size() >= kMaxQueueSize) {
            ++droppedLogCount;
            return;
        }
        logQueue.emplace_back(std::move(entry));
    }

    queueCv.notify_one();
}

const char *myLogger::levelToString(LogLevel_t level) const
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

std::string myLogger::formatLogMessage(const LogEntry &entry) const
{
    auto now_time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()) % 1000;
    std::tm now_tm;
#if JUCE_WINDOWS
    localtime_s(&now_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &now_tm);
#endif

    std::ostringstream timestamp;
    timestamp << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

    std::ostringstream threadIdStr;
    threadIdStr << entry.threadId;

    std::ostringstream logPrefix;
    logPrefix << timestamp.str() << " [" << processIdString << "." << threadIdStr.str() << "] "
              << levelToString(entry.level) << " " << entry.file << ":" << entry.line << " @" << entry.function;

    std::string logPrefixStr = logPrefix.str();
    if (logPrefixStr.length() < 96) {
        logPrefixStr.append(96 - logPrefixStr.length(), ' ');
    }

    std::ostringstream logMessage;
    logMessage << logPrefixStr << " " << entry.message;
    return logMessage.str();
}

void myLogger::workerLoop()
{
    for (;;) {
        std::deque<LogEntry> localQueue;
        size_t droppedInBatch = 0;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait(lock, [this] {
                return !workerRunning.load(std::memory_order_relaxed) || !logQueue.empty();
            });

            if (!workerRunning.load(std::memory_order_relaxed) && logQueue.empty()) {
                break;
            }

            localQueue.swap(logQueue);
            droppedInBatch = droppedLogCount;
            droppedLogCount = 0;
        }

        if (fileLogger == nullptr) {
            continue;
        }

        if (droppedInBatch > 0) {
            std::ostringstream dropMsg;
            dropMsg << "logger queue overflow, dropped " << droppedInBatch << " messages";
            fileLogger->logMessage(dropMsg.str());
        }

        for (const auto &entry : localQueue) {
            fileLogger->logMessage(formatLogMessage(entry));
        }
    }
}

void myLogger::setLogLevel(LogLevel_t level)
{
    currentLogLevel.store(level, std::memory_order_relaxed);
}

void log_msg(LogLevel_t level, const std::string &message, const char *file, const char *function, int line)
{
    myLogger::getInstance().logMsg(level, message, file, function, line);
}

extern "C" {

void log_msg_c(LogLevel_t level, const char *message, const char *file, const char *function, int line)
{
    log_msg(level, message != nullptr ? message : "", file, function, line);
}
}
