/***************************************************************************
 * Description: Logger
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-20 01:29:20
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#pragma once

#include <JuceHeader.h>

#ifndef __FILE_NAME__ // __FILE_NAME__ is __FILE__ without path
#define __FILE_NAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif

#ifndef __FUNCTION_NAME__ // __FUNCTION_NAME__ is __FUNCTION__ without "::" prefix
#define __FUNCTION_NAME__ (strrchr(__FUNCTION__, ':') ? strrchr(__FUNCTION__, ':') + 1 : __FUNCTION__)
#endif

typedef enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

extern juce::FileLogger *globalLogger;
extern LogLevel globalLogLevel;

#define LOG_MSG(level, message) logMsg(*globalLogger, level, message, __FILE_NAME__, __FUNCTION_NAME__, __LINE__)

void set_log_level(LogLevel level);
void logMsg(juce::FileLogger &logger, LogLevel level, const std::string &message, const char *file, const char *function, int line);
