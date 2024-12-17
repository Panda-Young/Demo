/* **************************************************************************
 * @Description: GapTimeThread
 * @Version: 0.1.0
 * @Author: pandapan@aactechnologies.com
 * @Date: 2024-12-17 18:07:06
 * @Copyright (c) 2024 by @AAC Technologies, All Rights Reserved.
 **************************************************************************/

#pragma once

#include "Logger.h"
#include <JuceHeader.h>

class GapTimeThread : public juce::Thread
{
public:
    GapTimeThread() : juce::Thread("GapTimeThread") {}

    void run() override
    {
        while (!threadShouldExit()) {
            if (shouldSleep) {
                auto start = clock();
                juce::Thread::sleep(sleepTime);
                auto stop = clock();
                LOG_MSG(LOG_DEBUG, "elapsed time: " + std::to_string((double)(stop - start)) + " ms");
                shouldSleep = false;
            }
        }
    }

    void setSleepTime(int time)
    {
        sleepTime = time;
        shouldSleep = true;
    }

private:
    int sleepTime = 0;
    bool shouldSleep = false;
};
