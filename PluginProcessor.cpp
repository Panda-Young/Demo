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

    RegType_t regType = checkRegType();
    if (regType == NoReg) {
        juce::DialogWindow::LaunchOptions options;
        auto contactLabel = std::make_unique<juce::Label>("contactLabel", "Please contact Panda for license code.");
        auto nativeCodeLabel = std::make_unique<juce::Label>("nativeCodeLabel", "Machine Code:");
        auto NativeCodeEditor = std::make_unique<juce::TextEditor>("NativeCodeEditor");
        auto LicenseCodeLabel = std::make_unique<juce::Label>("LicenseCodeLabel", "License Code:");
        auto LicenseCodeEditor = std::make_unique<juce::TextEditor>("LicenseCodeEditor");
        auto registerButton = std::make_unique<juce::TextButton>("Register", "Register");
        auto cancelButton = std::make_unique<juce::TextButton>("Cancel", "Cancel");

        juce::String natevaCode = getSerial();
        NativeCodeEditor->setText(natevaCode, juce::dontSendNotification);
        NativeCodeEditor->setReadOnly(true);

        registerButton->onClick = [this, LicenseCodeEditor = LicenseCodeEditor.get()]() {
            juce::String licenseCode = LicenseCodeEditor->getText();
            RegType_t regType = regSoftware(licenseCode);
            if (regType == NoReg) {
                isLicenseValid == false;
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Registration Failed", "Invalid license code, please try again!");
            } else {
                isLicenseValid = true;
                LOG_MSG_CF(LOG_INFO, "Registration Successful, Regtype: %d", regType);
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Registration Successful", "Your software has been successfully registered!");
                juce::DialogWindow::getCurrentlyModalComponent()->exitModalState(0);
            }
        };

        cancelButton->onClick = [this]() {
            isLicenseValid = false;
            LOG_MSG(LOG_WARN, "register canceled");
            juce::DialogWindow::getCurrentlyModalComponent()->exitModalState(0);
        };

        auto contentComponent = std::make_unique<juce::Component>();
        contentComponent->addAndMakeVisible(contactLabel.get());
        contentComponent->addAndMakeVisible(nativeCodeLabel.get());
        contentComponent->addAndMakeVisible(NativeCodeEditor.get());
        contentComponent->addAndMakeVisible(LicenseCodeLabel.get());
        contentComponent->addAndMakeVisible(LicenseCodeEditor.get());
        contentComponent->addAndMakeVisible(registerButton.get());
        contentComponent->addAndMakeVisible(cancelButton.get());

        contactLabel->setBounds(50, 10, 200, 25);
        nativeCodeLabel->setBounds(10, 40, 100, 25);
        NativeCodeEditor->setBounds(120, 40, 250, 25);
        LicenseCodeLabel->setBounds(10, 80, 100, 25);
        LicenseCodeEditor->setBounds(120, 80, 250, 25);
        registerButton->setBounds(10, 130, 100, 30);
        cancelButton->setBounds(290, 130, 100, 30);

        options.content.setOwned(contentComponent.release());
        options.dialogTitle = "Registering the software";
        options.dialogBackgroundColour = juce::Colours::grey;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;

        options.content->setSize(400, 200);
        juce::DialogWindow::showModalDialog("Software Registration", options.content.release(),
                                            nullptr, juce::Colours::grey, true);
    } else {
        isLicenseValid = true;
        LOG_MSG_CF(LOG_INFO, "License check OK, regester type: %d", regType);
    }
    if (isLicenseValid == false) {
        return;
    }

#if 0
    char licenseFileName[64] = JucePlugin_Name;
    strcat(licenseFileName, "_VST_Plugin.lic");
    juce::File licenseFile = tempDir.getChildFile(licenseFileName);
    if (!checkLicenseFile(licenseFile)) {
        isLicenseValid = false;
        return;
    } else {
        isLicenseValid = true;
    }
#endif

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
        downSampler[i].reset();
        upSampler[i].reset();
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

    setLatencySamples(block_size);
    ProcessBlockCounter = 0;

    juce::File UserDesktop = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    DataDumpDir = UserDesktop.getChildFile(JucePlugin_Name + juce::String("_DataDump"));
    DataDumpDir.createDirectory();

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char timeStr[20] = {0};
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H%M%S_", std::localtime(&now_time));
    juce::String TimeStamp = juce::String(timeStr);

    for (auto channel = 0; channel < 2; channel++) {
        juce::String Suffix = juce::String(channel) + ".pcm";
        OriginDataDumpFile[channel] = DataDumpDir.getChildFile(TimeStamp + "original_" + Suffix);
        OriginDataDumpFile[channel].create();
        DownSampleDataDumpFile[channel] = DataDumpDir.getChildFile(TimeStamp + "downSampled_" + Suffix);
        DownSampleDataDumpFile[channel].create();
        UpSampleDataDumpFile[channel] = DataDumpDir.getChildFile(TimeStamp + "upSampled_" + Suffix);
        UpSampleDataDumpFile[channel].create();
    }
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if (ProcessBlockCounter) {
        for (int channel = 0; channel < 2; channel++) {
            convertPCMtoWAV(OriginDataDumpFile[channel], 1, originalSampleRate, 32, 3);
            convertPCMtoWAV(DownSampleDataDumpFile[channel], 1, targetSampleRate, 32, 3);
            convertPCMtoWAV(UpSampleDataDumpFile[channel], 1, originalSampleRate, 32, 3);
        }
    }
    if (DataDumpDir.isDirectory()) {
        juce::Array<juce::File> files;
        DataDumpDir.findChildFiles(files, juce::File::findFiles, false);
        if (files.size() == 0) {
            DataDumpDir.deleteRecursively();
        }
    }
    ProcessBlockCounter = 0;

    LOG_MSG(LOG_INFO, "released Resources");
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
    if (isLicenseValid == false) {
        return;
    }
    clock_t start = clock();
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();
    if (ProcessBlockCounter++ == 0) {
        LOG_MSG(LOG_INFO, "processBlock: numSamples=" + std::to_string(numSamples) +
                              ", totalNumInputChannels=" + std::to_string(totalNumInputChannels) +
                              ", totalNumOutputChannels=" + std::to_string(totalNumOutputChannels));
    }

    int buffer_index = 0;
    float *p_write[2] = {0};
    float *p_read[2] = {0};

    while (buffer_index != numSamples) {
        for (int channel = 0; channel < totalNumInputChannels; channel++) {
            p_write[channel] = write_buf[channel];
            p_read[channel] = read_buf[channel];
        }
        int n_samples_to_write = 0;
        n_samples_to_write = MIN(numSamples - buffer_index, block_size - write_index);
        n_samples_to_write = MIN(n_samples_to_write, block_size - read_index);
        for (int i = 0; i < n_samples_to_write; i++) {
            for (int channel = 0; channel < totalNumInputChannels; channel++) {
                write_buf[channel][write_index + i] = buffer.getSample(channel, buffer_index + i);
            }
        }
        write_index += n_samples_to_write;
        if (write_index == block_size) {
            if (bypassEnable) {
                // do nothing or copy the input buffer to the output buffer
            } else {
                if (dataDumpEnable) {
                    for (int channel = 0; channel < totalNumInputChannels; channel++) {
                        dumpFloatPCMData(OriginDataDumpFile[channel], write_buf[channel], block_size);
                    }
                }
                const float downSampledFactor = originalSampleRate / targetSampleRate;
                 // Round up to prevent boundary value loss
                const int downSampledNumSamples = (block_size + downSampledFactor - 1) / downSampledFactor;
                juce::AudioBuffer<float> downSampledBuffer(totalNumInputChannels, downSampledNumSamples);
                const float upSampledFactor = targetSampleRate / originalSampleRate;
                juce::AudioBuffer<float> upSampledBuffer(totalNumInputChannels, block_size);
                for (int channel = 0; channel < totalNumInputChannels; channel++) {
                    float *downSampledpWptr = downSampledBuffer.getWritePointer(channel);
                    const float *downSampledpRptr = downSampledBuffer.getReadPointer(channel);
                    float *upSampledpWptr = upSampledBuffer.getWritePointer(channel);
                    const float *upSampledpRptr = upSampledBuffer.getReadPointer(channel);
                    downSampler[channel].reset();
                    downSampler[channel].process(downSampledFactor, write_buf[channel], downSampledpWptr, downSampledNumSamples);
                    if (dataDumpEnable) {
                        dumpFloatPCMData(DownSampleDataDumpFile[channel], downSampledpRptr, downSampledNumSamples);
                    }
                    int ret = algo_process(algo_handle, downSampledpRptr, downSampledpWptr, downSampledNumSamples);
                    if (ret != 0) {
                        LOG_MSG(LOG_ERROR, "Failed to algo_process. ret = " + std::to_string(ret));
                    }
                    upSampler[channel].reset();
                    upSampler[channel].process(upSampledFactor, downSampledpRptr, upSampledpWptr, block_size);
                    if (dataDumpEnable) {
                        dumpFloatPCMData(UpSampleDataDumpFile[channel], upSampledpRptr, block_size);
                    }
                    for (int i = 0; i < block_size; i++) {
                        write_buf[channel][i] = upSampledBuffer.getSample(channel, i);
                    }
                }
            }
        }
        for (int i = 0; i < n_samples_to_write; i++) {
            for (int channel = 0; channel < totalNumInputChannels; channel++) {
                buffer.setSample(channel, buffer_index + i, read_buf[channel][read_index + i]);
            }
        }
        buffer_index += n_samples_to_write;
        if (write_index == block_size) {
            for (int channel = 0; channel < totalNumInputChannels; channel++) {
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
    if (isLicenseValid == false) {
        return false;
    }
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
    tree.setProperty("dataDumpEnable", dataDumpEnable, nullptr);
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
        dataDumpEnable = tree.getProperty("dataDumpEnable", dataDumpEnable);
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
