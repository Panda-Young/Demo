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
#include "JucePluginDefines.h"

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
    LOGI("DemoAudioProcessor()");
}

DemoAudioProcessor::~DemoAudioProcessor()
{
    LOGI("~DemoAudioProcessor()");
    logger = nullptr;
}

//==============================================================================
const juce::String DemoAudioProcessor::getName() const
{
    LOGI("getName(): %s", JucePlugin_Name);
    return JucePlugin_Name;
}

bool DemoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    LOGI("acceptsMidi(): true");
    return true;
   #else
    LOGI("acceptsMidi(): false");
    return false;
   #endif
}

bool DemoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    LOGD("producesMidi(): true");
    return true;
   #else
    LOGD("producesMidi(): false");
    return false;
   #endif
}

bool DemoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    LOGD("isMidiEffect(): true");
    return true;
   #else
    LOGD("isMidiEffect(): false");
    return false;
   #endif
}

double DemoAudioProcessor::getTailLengthSeconds() const
{
    LOGI("getTailLengthSeconds(): 0.0");
    return 0.0;
}

int DemoAudioProcessor::getNumPrograms()
{
    LOGI("getNumPrograms(): 1");
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DemoAudioProcessor::getCurrentProgram()
{
    LOGI("getCurrentProgram(): 0");
    return 0;
}

void DemoAudioProcessor::setCurrentProgram (int index)
{
    LOGI("setCurrentProgram(): %d", index);
}

const juce::String DemoAudioProcessor::getProgramName (int index)
{
    LOGI("getProgramName(): %d", index);
    return {};
}

void DemoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    LOGI("changeProgramName(): %d, %s", index, newName.toStdString().c_str());
}

//==============================================================================
void DemoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    LOGI("prepareToPlay(): sampleRate: %f, samplesPerBlock: %d", sampleRate, samplesPerBlock);
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    LOGI("releaseResources()");
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

    LOGD("isBusesLayoutSupported(): true");
    return true;
  #endif
}
#endif

void DemoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();
    LOGD("processBlock(): totalNumInputChannels: %d, totalNumOutputChannels: %d, numSamples: %d",
         totalNumInputChannels, totalNumOutputChannels, numSamples);

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
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool DemoAudioProcessor::hasEditor() const
{
    LOGI("hasEditor(): true");
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DemoAudioProcessor::createEditor()
{
    LOGI("createEditor()");
    return new DemoAudioProcessorEditor (*this);
}

//==============================================================================
void DemoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    LOGI("getStateInformation()");
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DemoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    LOGI("setStateInformation()");
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoAudioProcessor();
}
