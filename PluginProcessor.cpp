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
#include <windows.h>

#define MIN(a, b) (a) < (b) ? (a) : (b)

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
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File logFile = tempDir.getChildFile("Demo_Audition_VST_Plugin.log");
    logger = std::make_unique<juce::FileLogger>(logFile, "Demo version: 1.1.0");
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
    logger->logMessage("prepareToPlay: sampleRate=" + juce::String(sampleRate) + ", samplesPerBlock=" + juce::String(samplesPerBlock));

    for (int i = 0; i < 2; i++) {
        write_buf[i] = (float *)calloc(block_size, sizeof(float));
        if (write_buf[i] == nullptr) {
            logger->logMessage("Failed to allocate write_buf[" + juce::String(i) + "]");
        }
        read_buf[i] = (float *)calloc(block_size, sizeof(float));
        if (read_buf[i] == nullptr) {
            logger->logMessage("Failed to allocate read_buf[" + juce::String(i) + "]");
        }
    }
    InputBuffer = (float *)calloc(block_size * 2, sizeof(float));
    if (InputBuffer == nullptr) {
        logger->logMessage("Failed to allocate InputBuffer");
    }
    OutputBuffer = (float *)calloc(block_size * 2, sizeof(float));
    if (OutputBuffer == nullptr) {
        logger->logMessage("Failed to allocate OutputBuffer");
    }
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    logger->logMessage("releaseResources");
    for (int i = 0; i < 2; i++) {
        if (write_buf[i] != nullptr) {
            free(write_buf[i]);
            write_buf[i] = nullptr;
        }
        if (read_buf[i] != nullptr) {
            free(read_buf[i]);
            read_buf[i] = nullptr;
        }
    }
    if (InputBuffer != nullptr) {
        free(InputBuffer);
        InputBuffer = nullptr;
    }
    if (OutputBuffer != nullptr) {
        free(OutputBuffer);
        OutputBuffer = nullptr;
    }
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
    clock_t start = clock();
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();
    if (ProcessCounter++ == 0) {
        logger->logMessage("processBlock: numSamples=" + juce::String(numSamples));
    }

    int buffer_index = 0;
    float *p_write[2] = {0};
    float *p_read[2] = {0};

    while (buffer_index != numSamples) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            p_write[channel] = write_buf[channel];
            p_read[channel] = read_buf[channel];
        }
        int n_samples_to_write = 0;
        n_samples_to_write = MIN(numSamples - buffer_index, block_size - write_index);
        n_samples_to_write = MIN(n_samples_to_write, block_size - read_index);
        for (int i = 0; i < n_samples_to_write; i++) {
            for (int channel = 0; channel < totalNumInputChannels; ++channel) {
                write_buf[channel][write_index + i] = buffer.getSample(channel, buffer_index + i);
            }
        }
        write_index += n_samples_to_write;
        if (write_index == block_size) {
            for (int channel = 0; channel < totalNumInputChannels; ++channel) {
                memcpy(InputBuffer + (channel * block_size), write_buf[channel], block_size * sizeof(float));
                memset(write_buf[channel], 0, block_size * sizeof(float));
            }
        }
        if (write_index == block_size) {
            if (isFirstAlgoFrame++ == 0) {
                _sleep(600);
            } else {
                _sleep(32);
            }
            for (int j = 0; j < block_size * 2; j++) {
                OutputBuffer[j] = InputBuffer[j] / 2.0f;
            }
            for (int i = 0; i < 2; i++) {
                memcpy(write_buf[i], OutputBuffer + (i * block_size), block_size * sizeof(float));
            }
        }
        for (int i = 0; i < n_samples_to_write; i++) {
            for (int channel = 0; channel < totalNumInputChannels; ++channel) {
                buffer.setSample(channel, buffer_index + i, read_buf[channel][read_index + i]);
            }
        }
        buffer_index += n_samples_to_write;
        if (write_index == block_size) {
            for (int channel = 0; channel < totalNumInputChannels; ++channel) {
                write_buf[channel] = p_read[channel];
                read_buf[channel] = p_write[channel];
            }
            write_index = 0;
        }
        read_index += n_samples_to_write;
        if (read_index == block_size) {
            read_index = 0;
        }
    }
    clock_t stop = clock();
    logger->logMessage("ProcessCounter = " + juce::String(ProcessCounter) + ", elapsed time: " + juce::String((double)(stop - start)) + " ms");
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
