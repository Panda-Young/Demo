/***************************************************************************
 * Description:
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2025-01-06 21:05:02
 * Copyright (c) 2025 by Panda-Young, All Rights Reserved.
 **************************************************************************/

#pragma once

#include <JuceHeader.h>
#include <JucePluginDefines.h>
#include <memory>
#include <thread>
#include <vector>
#include <windows.h>

// IPCProcess for handling 32/64-bit child process and IPC
class IPCProcess
{
public:
    IPCProcess();
    ~IPCProcess();

    bool start();
    void stop();

    // Send and receive audio data (blocking)
    bool process(float *data, int numSamples, int numChannels);

private:
    HANDLE hPipe;
    HANDLE hEventToChild;
    HANDLE hEventToParent;
    HANDLE childProcess;

    bool started = false;
};

//==============================================================================
// Main plugin processor class
class DemoAudioProcessor : public juce::AudioProcessor
{
public:
    DemoAudioProcessor();
    ~DemoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

private:
    std::unique_ptr<IPCProcess> ipc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoAudioProcessor)
};
