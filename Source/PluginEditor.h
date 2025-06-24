/***************************************************************************
 * Description: 
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2025-01-06 21:05:02
 * Copyright (c) 2025 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class DemoAudioProcessor;

class DemoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    DemoAudioProcessorEditor(DemoAudioProcessor&);
    ~DemoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DemoAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoAudioProcessorEditor)
};
