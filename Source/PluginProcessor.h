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

#include "myLogger.h"
#include "myUtils.h"
#include <JuceHeader.h>

#ifdef __cplusplus
extern "C" {
#include "algo_example.h"
}
#endif

#define MAX_SUPPORT_CHANNELS 2

//==============================================================================
/**
 */
class DemoAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    DemoAudioProcessor();
    ~DemoAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    //==============================================================================
    int getUsedPluginType() const { return pluginType; }
    const std::string &getUsedHostAppName() const { return hostAppName; }
    int getUsedHostAppVersion() const { return hostAppVersion; }
    void setAnyParamChanged(bool state) { anyParamChanged = state; }
    bool getAnyParamChanged() const { return anyParamChanged; }
    void setDataDumpState(bool state) { dataDumpEnable = state; }
    bool getDataDumpState() const { return dataDumpEnable; }

    void initializeBuffers();

    void setBypassState(bool state) { bypassEnable = state; }
    bool getBypassState() const { return bypassEnable; }
    void setGainValue(float value) { gain = value; }
    float getGainValue() const { return gain; }
    void *getAlgoHandle() const { return algo_handle; }

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void bypassEnableChanged(bool newState) = 0;
        virtual void gainValueChanged(float newValue) = 0;
    };

    void addListener(Listener *listener) { listeners.add(listener); }
    void removeListener(Listener *listener) { listeners.remove(listener); }

    juce::AudioProcessorValueTreeState &getApvts() { return apvts; }
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    myLogger &logger = myLogger::getInstance();

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameters()};
    int pluginType = -1;
    std::string hostAppName = "";
    int hostAppVersion = -1;
    bool bypassEnable = false;
    bool anyParamChanged = false;
    bool dataDumpEnable = false;
    bool isLicenseValid = false;
    bool isInitDone = false;
    bool sendZeroValue = false;
    double midiControllerTimeElapsed = 0.0;
    const double midiControllerinterval = 10.0;

    uint64_t processBlockCounter = 0;
    uint64_t algoFrameCounter = 0;

    const int block_size = 2048;
    void *algo_handle = nullptr;
    float gain = 0.0f;
    float midiGain = 0.0f;

    std::unique_ptr<float[]> write_buf[MAX_SUPPORT_CHANNELS];
    std::unique_ptr<float[]> read_buf[MAX_SUPPORT_CHANNELS];
    int write_index = 0;
    int read_index = 0;
    juce::File dataDumpDir, processedDataDumpFile;

    double originalSampleRate = 0;
    int originalChannels = 0;

    juce::ListenerList<Listener> listeners;

    void notifyBypassEnableChanged()
    {
        listeners.call([this](Listener &l) { l.bypassEnableChanged(bypassEnable); });
    }
    void DemoAudioProcessor::notifyGainValueChanged()
    {
        listeners.call([this](Listener &l) { l.gainValueChanged(gain); });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoAudioProcessor)
};
