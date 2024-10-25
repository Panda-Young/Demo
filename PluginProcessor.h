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
#include "Utils.h"

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

    //==============================================================================
    int getPluginType() const { return pluginType; };
    std::string getHostAppName() const { return hostAppName; };
    int getHostAppVersion() const { return hostAppVersion; };
    void setBypassState(bool state) { bypassEnable = state; };
    bool getBypassState() const { return bypassEnable; };
    void setAnyParamChanged(bool state) { anyParamChanged = state; };
    bool getAnyParamChanged() const { return anyParamChanged; };
    void setDataDumpEnable(bool state) { dataDumpEnable = state; };
    bool getDataDumpEnable() const { return dataDumpEnable; };
    void setGainValue(float value) { gain = value; };
    float getGainValue() const { return gain; };
    void* getAlgoHandle() const { return algo_handle; };

private:
    //==============================================================================
    std::unique_ptr<juce::FileLogger> logger;
    int pluginType = -1;
    std::string hostAppName;
    int hostAppVersion = -1;
    bool bypassEnable = false;
    bool anyParamChanged = false;
    bool dataDumpEnable = false;
    bool isLicenseValid = false;

    uint32_t ProcessBlockCounter = 0;
    uint32_t AlgoFrameCounter = 0;

    const int block_size = 2048;
    void *algo_handle = nullptr;
    float gain = 0.0f;

    float *write_buf[2] = {0};
    float *read_buf[2] = {0};
    int write_index = 0;
    int read_index = 0;
    float *InputBuffer = nullptr;
    float *OutputBuffer = nullptr;
    juce::File DataDumpDir, OriginDataDumpFile[2], DownSampleDataDumpFile[2], UpSampleDataDumpFile[2];

    double originalSampleRate = 0; // default sample rate
    double targetSampleRate = 16000.0f;
    juce::WindowedSincInterpolator downSampler[2];
    juce::WindowedSincInterpolator upSampler[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoAudioProcessor)
};
