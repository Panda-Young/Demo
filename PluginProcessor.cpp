/***************************************************************************
 * Description: PluginProcessor
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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JucePluginDefines.h>

//==============================================================================
DemoAudioProcessor::DemoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    juce::File::SpecialLocationType LogDir = juce::File::SpecialLocationType::tempDirectory;
    juce::File logFile = juce::File::getSpecialLocation(LogDir).getChildFile("Demo_VST_Plugin.log");
    logger = std::make_unique<juce::FileLogger>(logFile, "");
}

DemoAudioProcessor::~DemoAudioProcessor()
{
    logger = nullptr;
}

//==============================================================================
const juce::String DemoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DemoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DemoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DemoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DemoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DemoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DemoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DemoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DemoAudioProcessor::getProgramName (int index)
{
    return {};
}

void DemoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DemoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    logger->logMessage("samplesPerBlock: " + juce::String(samplesPerBlock));
    InputBuffer = (float *)calloc(samplesPerBlock * 4, sizeof(float));
    if (InputBuffer == nullptr) {
        logger->logMessage("InputBuffer calloc failed");
    }
    OutputBuffer = (float *)calloc(samplesPerBlock * 4, sizeof(float));
    if (OutputBuffer == nullptr) {
        logger->logMessage("OutputBuffer calloc failed");
    }
    for (int i = 0; i < 2; i++) {
        TempBuffer_1[i] = (float *)calloc(samplesPerBlock, sizeof(float));
        if (TempBuffer_1[i] == nullptr) {
            logger->logMessage("TempBuffer_1[" + juce::String(i) + "] calloc failed");
        }
        TempBuffer_2[i] = (float *)calloc(samplesPerBlock, sizeof(float));
        if (TempBuffer_2[i] == nullptr) {
            logger->logMessage("TempBuffer_2[" + juce::String(i) + "] calloc failed");
        }
    }
    ProcessCount = 0;
    logger->logMessage("prepareToPlay done!");
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if (InputBuffer != nullptr) {
        free(InputBuffer);
        InputBuffer = nullptr;
    }
    if (OutputBuffer != nullptr) {
        free(OutputBuffer);
        OutputBuffer = nullptr;
    }
    for (int i = 0; i < 2; i++) {
        if (TempBuffer_1[i] != nullptr) {
            free(TempBuffer_1[i]);
            TempBuffer_1[i] = nullptr;
        }
        if (TempBuffer_2[i] != nullptr) {
            free(TempBuffer_2[i]);
            TempBuffer_2[i] = nullptr;
        }
    }
    logger->logMessage("releaseResources done!");
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DemoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    if (ProcessCount == 0) {
        logger->logMessage("numSamples: " + juce::String(numSamples));
    }

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < 2; ++channel) {
        for (int i = 0; i < numSamples; i++) {
            InputBuffer[channel * 2 * numSamples + i + (ProcessCount % 2) * numSamples] = buffer.getSample(channel, i);
        }
    }

    if (ProcessCount % 2 == 0) {
        for (int channel = 0; channel < 2; ++channel) {
            for (int i = 0; i < numSamples; i++) {
                buffer.setSample(channel, i, TempBuffer_2[channel][i]);
            }
        }
        ProcessCount++;
        return;
    }

    memcpy(OutputBuffer, InputBuffer, numSamples * 4 * sizeof(float));
    _sleep(30);

    for (int channel = 0; channel < 2; ++channel) {
        // for (int i = 0; i < numSamples; i++) {
        //     TempBuffer_2[channel][i] = OutputBuffer[channel * 2 * numSamples + i + numSamples];
        //     TempBuffer_1[channel][i] = OutputBuffer[channel * 2 * numSamples + i];
        // }
        memcpy(TempBuffer_2[channel], OutputBuffer + channel * 2 * numSamples + numSamples, numSamples * sizeof(float));
        memcpy(TempBuffer_1[channel], OutputBuffer + channel * 2 * numSamples, numSamples * sizeof(float));
    }

    if (ProcessCount % 2 != 0) {
        for (int channel = 0; channel < 2; ++channel) {
            for (int i = 0; i < numSamples; i++) {
                buffer.setSample(channel, i, TempBuffer_1[channel][i]);
            }
        }
    }
    ProcessCount++;
}

//==============================================================================
bool DemoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DemoAudioProcessor::createEditor()
{
    return new DemoAudioProcessorEditor (*this);
}

//==============================================================================
void DemoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DemoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoAudioProcessor();
}
