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

#define SAVE_PCM_TO_DESKTOP 1
#define MIN(a, b) (a) < (b) ? (a) : (b)
extern juce::FileLogger *globalLogger;

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
    char logFileName[64] = JucePlugin_Name;
    strcat(logFileName, "_VST_Plugin.log");
    juce::File logFile = tempDir.getChildFile(juce::String(logFileName));
    char logStartMsg[128] = {0};
    sprintf(logStartMsg, "%s VST Plugin %s", JucePlugin_Name, JucePlugin_VersionString);
    logger = std::make_unique<juce::FileLogger>(logFile, logStartMsg);
    globalLogger = logger.get();
    set_log_level(LOG_INFO);

    juce::File dllPath = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    LOG_MSG_CF(LOG_INFO, "dllPath= \"%s\"", dllPath.getFullPathName().toRawUTF8());
    pluginType = getPluginType(dllPath.getFullPathName().toStdString());
    hostAppName = extractHostAppName();
    hostAppVersion = getAuditionVersion();

    for (int i = 0; i < 2; i++) {
        write_buf[i] = (float *)calloc(block_size, sizeof(float));
        if (write_buf[i] == nullptr) {
            LOG_MSG(LOG_ERROR, "Failed to allocate write_buf[" + std::to_string(i) + "]");
        }
        read_buf[i] = (float *)calloc(block_size, sizeof(float));
        if (read_buf[i] == nullptr) {
            LOG_MSG(LOG_ERROR, "Failed to allocate read_buf[" + std::to_string(i) + "]");
        }
    }
    InputBuffer = (float *)calloc(block_size * 2, sizeof(float));
    if (InputBuffer == nullptr) {
        LOG_MSG(LOG_ERROR, "Failed to allocate InputBuffer");
    }
    OutputBuffer = (float *)calloc(block_size * 2, sizeof(float));
    if (OutputBuffer == nullptr) {
        LOG_MSG(LOG_ERROR, "Failed to allocate OutputBuffer");
    }

    char version[32] = {0};
    int ret = 0;
    ret = get_algo_version(version);
    if (ret != 0) {
        LOG_MSG(LOG_ERROR, "Failed to get_algo_version. ret = " + std::to_string(ret));
    } else {
        std::ostringstream oss;
        oss << "get_algo_version: " << version;
        LOG_MSG(LOG_INFO, oss.str());
    }

    algo_handle = algo_init();
    if (algo_handle == nullptr) {
        LOG_MSG(LOG_ERROR, "Failed to algo_init");
        return;
    }
}

DemoAudioProcessor::~DemoAudioProcessor()
{
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
    if (algo_handle != nullptr) {
        algo_deinit(algo_handle);
        algo_handle = nullptr;
    }

#if SAVE_PCM_TO_DESKTOP
        LOG_MSG(LOG_INFO, "saved PCM to desktop");
        for (int channel = 0; channel < getTotalNumInputChannels(); channel++) {
            convertPCMtoWAV("originalBuffer_" + std::to_string(channel) + ".pcm", 1, originalSampleRate, 32, 3);
        }
#endif

    LOG_MSG(LOG_INFO, "AudioProcessor destroyed. Log stop. Closed plugins or software");
    logger.reset();
    globalLogger = nullptr;
}

//==============================================================================
const juce::String DemoAudioProcessor::getName() const
{
    std::ostringstream oss;
    oss << "getName: " << JucePlugin_Name;
    LOG_MSG(LOG_INFO, oss.str());

    return JucePlugin_Name;
}

bool DemoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    LOG_MSG(LOG_DEBUG, "acceptsMidi: true");
    return true;
   #else
    LOG_MSG(LOG_DEBUG, "acceptsMidi: false");
    return false;
   #endif
}

bool DemoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
   LOG_MSG(LOG_DEBUG, "producesMidi: true");
    return true;
   #else
    LOG_MSG(LOG_DEBUG, "producesMidi: false");
    return false;
   #endif
}

bool DemoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    LOG_MSG(LOG_DEBUG, "isMidiEffect: true");
    return true;
   #else
    LOG_MSG(LOG_DEBUG, "isMidiEffect: false");
    return false;
   #endif
}

double DemoAudioProcessor::getTailLengthSeconds() const
{
    if (originalSampleRate == 0) {
        LOG_MSG(LOG_INFO, "getTailLengthSeconds: 0");
        return 0;
    }
    double tailLength = static_cast<double>(block_size) / originalSampleRate;
    LOG_MSG(LOG_INFO, "getTailLengthSeconds: " + std::to_string(tailLength));
    return tailLength;
}

int DemoAudioProcessor::getNumPrograms()
{
    LOG_MSG(LOG_DEBUG, "getNumPrograms: 1");
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DemoAudioProcessor::getCurrentProgram()
{
    LOG_MSG(LOG_DEBUG, "getCurrentProgram: 0");
    return 0;
}

void DemoAudioProcessor::setCurrentProgram (int index)
{
    LOG_MSG(LOG_INFO, "setCurrentProgram: index=" + std::to_string(index));
}

const juce::String DemoAudioProcessor::getProgramName (int index)
{
    LOG_MSG(LOG_INFO, "getProgramName: index=" + std::to_string(index));
    return {};
}

void DemoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    std::ostringstream oss;
    oss << "changeProgramName: index=" << index << ", newName=" << newName.toStdString();
    LOG_MSG(LOG_INFO, oss.str());
}

//==============================================================================
void DemoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    this->originalSampleRate = sampleRate;
    LOG_MSG(LOG_INFO, "prepareToPlay: sampleRate=" + std::to_string(sampleRate) +
                       ", samplesPerBlock=" + std::to_string(samplesPerBlock) +
                       ", about " + std::to_string(samplesPerBlock * 1000.0f / sampleRate ) + " milliseconds");
    ProcessBlockCounter = 0;
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    LOG_MSG(LOG_INFO, "released Resources");
    ProcessBlockCounter = 0;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    LOG_MSG(LOG_DEBUG, "OutputChannelSet=" + std::to_string(layouts.getMainOutputChannelSet().size()) +
                       ", InputChannelSet=" + std::to_string(layouts.getMainInputChannelSet().size()));
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
    if (ProcessBlockCounter++ == 0) {
        LOG_MSG(LOG_INFO, "processBlock: numSamples=" + std::to_string(numSamples));
        LOG_MSG(LOG_INFO, "totalNumInputChannels=" + std::to_string(totalNumInputChannels) +
                           ", totalNumOutputChannels=" + std::to_string(totalNumOutputChannels));
    }

#if SAVE_PCM_TO_DESKTOP
    for (int channel = 0; channel < totalNumInputChannels; channel++) {
        savePCMDatatoDesktop("originalBuffer_" + std::to_string(channel) + ".pcm", buffer.getReadPointer(channel), numSamples);
    }
#endif

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
            }
            if (bypassEnable) {
                memcpy(OutputBuffer, InputBuffer, block_size * 2 * sizeof(float));
            } else {
                if (isFirstAlgoFrame++ == 0) {
                    _sleep(600);
                } else {
                    _sleep(12);
                }
                int ret = algo_process(algo_handle, InputBuffer, OutputBuffer, block_size * 2);
                if (ret != 0) {
                    LOG_MSG(LOG_ERROR, "Failed to algo_process. ret = " + std::to_string(ret));
                }
            }
            for (int channel = 0; channel < totalNumInputChannels; channel++) {
                memcpy(write_buf[channel], OutputBuffer + (channel * block_size), block_size * sizeof(float));
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
    LOG_MSG(LOG_DEBUG, "ProcessBlockCounter = " + std::to_string(ProcessBlockCounter) +
                       ", elapsed time: " + std::to_string((double)(stop - start)) + " ms");
}

//==============================================================================
bool DemoAudioProcessor::hasEditor() const
{
    LOG_MSG(LOG_INFO, "hasEditor: true");
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DemoAudioProcessor::createEditor()
{
    LOG_MSG(LOG_INFO, "create editor");
    return new DemoAudioProcessorEditor (*this);
}

//==============================================================================
void DemoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    if (anyParamChanged == false) {
        // in VST3 plugin, before restore parameters, call getStateInformation will store the default parameters
        // This will overwrite the user's last selection.
        LOG_MSG(LOG_INFO, "none of the parameters have been changed, skip store parameters to memory block");
        return;
    }
    char dataTreeName[64] = JucePlugin_Name;
    strcat(dataTreeName, "AudioProcessor");
    juce::ValueTree tree(dataTreeName);
    tree.setProperty("bypassEnable", bypassEnable, nullptr);
    tree.setProperty("gain", gain, nullptr);
    tree.setProperty("globalLogLevel", globalLogLevel, nullptr);
    juce::MemoryOutputStream stream(destData, false);
    tree.writeToStream(stream);

    LOG_MSG(LOG_INFO, "store parameters to memory block:\n    " + tree.toXmlString().toStdString());
}

void DemoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        bypassEnable = tree.getProperty("bypassEnable", bypassEnable);
        gain = tree.getProperty("gain", gain);
        int logLevel = tree.getProperty("globalLogLevel", globalLogLevel);
        if (logLevel != globalLogLevel) {
            globalLogLevel = (LogLevel)logLevel;
            if (globalLogLevel < LOG_DEBUG || globalLogLevel > LOG_OFF) {
                globalLogLevel = LOG_INFO;
            }
            set_log_level(globalLogLevel);
        }
        if (algo_handle != nullptr) {
            int ret = algo_set_param(algo_handle, ALGO_PARAM2, &gain, sizeof(float));
            if (ret != 0) {
                LOG_MSG(LOG_ERROR, "Failed to algo_set_param. ret = " + std::to_string(ret));
            }
        }
    }

    LOG_MSG(LOG_INFO, "restore parameters from memory block:\n    " + tree.toXmlString().toStdString());

    // in VST2 plugin, the editor is not created when setStateInformation is called
    if (auto *editor = dynamic_cast<DemoAudioProcessorEditor *>(getActiveEditor())) {
        editor->updateParameterDisplays();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoAudioProcessor();
}
