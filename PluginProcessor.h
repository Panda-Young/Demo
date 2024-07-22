/***************************************************************************
 * Description: Header of PluginProcessor
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-06 15:29:30
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Logger.h"

#ifdef __cplusplus
extern "C" {
#include "algo_example.h"
}
#endif

//==============================================================================
/**
*/
class DemoAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DemoAudioProcessor();
    ~DemoAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    std::unique_ptr<juce::FileLogger> logger;
    int pluginType = -1;
    int hostAppVersion = -1;

    void *algo_handle = nullptr;
    bool bypassEnable = false;
    float gain = 0.0f;

private:
    //==============================================================================
    bool EditorCreated = false;
    double sampleRate = 0; // default sample rate

    const int block_size = 2048;
    uint32_t isFirstAlgoFrame = 0;
    uint32_t ProcessCounter = 0;

    float *write_buf[2] = {0};
    float *read_buf[2] = {0};
    int write_index = 0;
    int read_index = 0;
    float *InputBuffer = nullptr;
    float *OutputBuffer = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoAudioProcessor)
};
