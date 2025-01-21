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

#define MAX_GAIN_VALUE 20.0f
#define MIN_GAIN_VALUE -20.0f

//==============================================================================
class RegistrationComponent : public juce::Component
{
public:
    RegistrationComponent()
    {
        addAndMakeVisible(contactLabel);
        addAndMakeVisible(nativeCodeLabel);
        addAndMakeVisible(NativeCodeEditor);
        addAndMakeVisible(LicenseCodeLabel);
        addAndMakeVisible(LicenseCodeEditor);
        addAndMakeVisible(registerButton);
        addAndMakeVisible(cancelButton);

        NativeCodeEditor.setReadOnly(true);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        contactLabel.setBounds(area.removeFromTop(25));
        nativeCodeLabel.setBounds(area.removeFromTop(25).removeFromLeft(100));
        NativeCodeEditor.setBounds(area.removeFromTop(25));
        LicenseCodeLabel.setBounds(area.removeFromTop(25).removeFromLeft(100));
        LicenseCodeEditor.setBounds(area.removeFromTop(25));
        area.removeFromTop(30);
        auto buttonArea = area.removeFromTop(30);
        registerButton.setBounds(buttonArea.removeFromLeft(100));
        cancelButton.setBounds(buttonArea.removeFromRight(100));
    }

    juce::Label contactLabel{"contactLabel", "Please contact Panda for license code."};
    juce::Label nativeCodeLabel{"nativeCodeLabel", "Machine Code:"};
    juce::TextEditor NativeCodeEditor{"NativeCodeEditor"};
    juce::Label LicenseCodeLabel{"LicenseCodeLabel", "License Code:"};
    juce::TextEditor LicenseCodeEditor{"LicenseCodeEditor"};
    juce::TextButton registerButton{"Register", "Register"};
    juce::TextButton cancelButton{"Cancel", "Cancel"};
};

void DemoAudioProcessor::initializeBuffers()
{
    try {
        for (int channel = 0; channel < MAX_SUPPORT_CHANNELS; channel++) {
            writeBuf[channel] = std::make_unique<float[]>(blockSize);
            readBuf[channel] = std::make_unique<float[]>(blockSize);
        }
    } catch (const std::bad_alloc& e) {
        LOG_MSG(LOG_ERROR, "Failed to allocate memory: " + std::string(e.what()));
        isInitDone = false;
        return;
    }
}

DemoAudioProcessor::DemoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
    pluginType = getPluginType();
    hostAppName = getHostAppName();
    if (hostAppName == "Adobe Audition") {
        hostAppVersion = getAuditionVersion();
    }

    RegType_t regType = checkRegType();
    if (regType == NoReg) {
        auto contentComponent = std::make_unique<RegistrationComponent>();

        contentComponent->NativeCodeEditor.setText(getSerial(), juce::dontSendNotification);

        contentComponent->registerButton.onClick = [this, contentComponent = contentComponent.get()]() {
            juce::String licenseCode = contentComponent->LicenseCodeEditor.getText();
            RegType_t regType = regSoftware(licenseCode);
            if (regType == NoReg) {
                isLicenseValid = false;
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

        contentComponent->cancelButton.onClick = [this]() {
            isLicenseValid = false;
            LOG_MSG(LOG_WARN, "register canceled");
            juce::DialogWindow::getCurrentlyModalComponent()->exitModalState(0);
        };

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(contentComponent.release());
        options.dialogTitle = "Registering the software";
        options.dialogBackgroundColour = juce::Colours::darkgrey;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;

        options.content->setSize(420, 200);
        options.launchAsync();

    } else {
        isLicenseValid = true;
        LOG_MSG_CF(LOG_INFO, "License check OK, regester type: %d", regType);
    }
    if (!isLicenseValid) {
        return;
    }

    char version[32] = {0};
    int ret = get_algo_version(version);
    if (ret != 0) {
        LOG_MSG(LOG_ERROR, "Failed to get_algo_version. ret = " + std::to_string(ret));
        return;
    } else {
        LOG_MSG(LOG_INFO, "get_algo_version: " + std::string(version));
    }

    algo_handle = algo_init();
    if (algo_handle == nullptr) {
        LOG_MSG(LOG_ERROR, "Failed to algo_init");
        return;
    }

    initializeBuffers();

    isInitDone = true;
    LOG_MSG_CF(LOG_INFO, "AudioProcessor 0x%p initialized successfully.", this);
}

DemoAudioProcessor::~DemoAudioProcessor()
{
    for (int channel = 0; channel < MAX_SUPPORT_CHANNELS; channel++) {
        writeBuf[channel].reset();
        readBuf[channel].reset();
    }
    if (algo_handle != nullptr) {
        algo_deinit(algo_handle);
        algo_handle = nullptr;
    }

    LOG_MSG_CF(LOG_INFO, "AudioProcessor 0x%p destroyed. Closed plugins or software", this);
}

//==============================================================================
const juce::String DemoAudioProcessor::getName() const
{
    LOG_MSG(LOG_INFO, "getName: " + juce::String(JucePlugin_Name).toStdString());

    return JucePlugin_Name;
}

bool DemoAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    LOG_MSG(LOG_INFO, "acceptsMidi: true");
    return true;
#else
    LOG_MSG(LOG_INFO, "acceptsMidi: false");
    return false;
#endif
}

bool DemoAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    LOG_MSG(LOG_INFO, "producesMidi: true");
    return true;
#else
    LOG_MSG(LOG_INFO, "producesMidi: false");
    return false;
#endif
}

bool DemoAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    if (processBlockCounter == 0) {
        LOG_MSG(LOG_DEBUG, "isMidiEffect: true");
    }
    return true;
#else
    if (processBlockCounter == 0) {
        LOG_MSG(LOG_DEBUG, "isMidiEffect: false");
    }
    return false;
#endif
}

double DemoAudioProcessor::getTailLengthSeconds() const
{
    // we don't konw the original sample rate before prepareToPlay
    if (originalSampleRate == 0) {
        LOG_MSG(LOG_INFO, "getTailLengthSeconds: 0");
        return 0;
    }
    double tailLength = static_cast<double>(blockSize) / originalSampleRate;
    LOG_MSG(LOG_INFO, "getTailLengthSeconds: " + std::to_string(tailLength));
    return tailLength;
}

int DemoAudioProcessor::getNumPrograms()
{
    LOG_MSG(LOG_INFO, "getNumPrograms: 1");
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int DemoAudioProcessor::getCurrentProgram()
{
    LOG_MSG(LOG_INFO, "getCurrentProgram: 0");
    return 0;
}

void DemoAudioProcessor::setCurrentProgram(int index)
{
    LOG_MSG(LOG_INFO, "setCurrentProgram: index=" + std::to_string(index));
}

const juce::String DemoAudioProcessor::getProgramName(int index)
{
    LOG_MSG(LOG_INFO, "getProgramName: index=" + std::to_string(index));
    return {};
}

void DemoAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    std::ostringstream oss;
    oss << "changeProgramName: index=" << index << ", newName=" << newName.toStdString();
    LOG_MSG(LOG_INFO, oss.str());
}

//==============================================================================
class OpenLogCallback : public juce::ModalComponentManager::Callback
{
public:
    void modalStateFinished(int /*returnValue*/) override
    {
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        if (tempDir.isDirectory()) {
            if (tempDir.startAsProcess()) {
                LOG_MSG(LOG_DEBUG, "Opened the temp directory: " + tempDir.getFullPathName().toStdString());
            } else {
                LOG_MSG(LOG_ERROR, "Failed to open the temp directory: " + tempDir.getFullPathName().toStdString());
            }
        }
        juce::File logFile = tempDir.getChildFile(JucePlugin_Name "_VST_Plugin.log");
        if (logFile.existsAsFile()) {
            if (logFile.startAsProcess()) {
                LOG_MSG(LOG_DEBUG, "Opened the log file: " + logFile.getFullPathName().toStdString());
            } else {
                LOG_MSG(LOG_ERROR, "Failed to open the log file: " + logFile.getFullPathName().toStdString());
            }
        }
    }
};

void DemoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    if (!isInitDone) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Initialization Failed",
            "Failed to initialize the plugin! "
            "Please check the last part of the log file: \n" +
                logger.getLogFile().getFullPathName() + "\n"
                                                        "You can also send the log file to the developers for further assistance.",
            "OK",
            nullptr,
            new OpenLogCallback());
    }

    if (!toReleaseResources) {
        originalSampleRate = sampleRate;
        originalChannels = getTotalNumInputChannels();
        LOG_MSG(LOG_INFO, "prepareToPlay: sampleRate=" + std::to_string(sampleRate) +
                              ", samplesPerBlock=" + std::to_string(samplesPerBlock) +
                              ", about " + std::to_string(samplesPerBlock * 1000.0f / sampleRate) + " milliseconds");

        setLatencySamples(blockSize);
        LOG_MSG(LOG_INFO, "set latency samples: " + std::to_string(blockSize));
        processBlockCounter = 0;

        juce::File UserDesktop = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
        dataDumpDir = UserDesktop.getChildFile(JucePlugin_Name + juce::String("_DataDump"));
        if (dataDumpDir.createDirectory()) {
            LOG_MSG(LOG_DEBUG, "Created data dump folder: \"" + dataDumpDir.getFullPathName().toStdString() + "\"");
        } else {
            LOG_MSG(LOG_ERROR, "Failed to create data dump folder: \"" + dataDumpDir.getFullPathName().toStdString() + "\"");
        }

        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char timeStr[20] = {0};
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H%M%S_", std::localtime(&now_time));
        juce::String TimeStamp = juce::String(timeStr);
        processedDataDumpFile = dataDumpDir.getChildFile(TimeStamp + "processed.pcm");
        if (processedDataDumpFile.create().wasOk()) {
            LOG_MSG(LOG_DEBUG, "Created data dump file: \"" +
                                   processedDataDumpFile.getFullPathName().toStdString() + "\"");
        } else {
            LOG_MSG(LOG_ERROR, "Failed to create data dump file: \"" +
                                   processedDataDumpFile.getFullPathName().toStdString() + "\"");
        }
        toReleaseResources = true;
    } else {
        LOG_MSG(LOG_WARN, "prepareToPlay: nothing to prepare");
    }
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if (toReleaseResources) {
        if (processBlockCounter) {
            convertPCMtoWAV(processedDataDumpFile, static_cast<uint16_t>(validChannels),
                            static_cast<uint32_t>(originalSampleRate), 32, 3);
        }
        if (dataDumpDir.isDirectory()) {
            juce::Array<juce::File> files;
            dataDumpDir.findChildFiles(files, juce::File::findFiles, false);
            for (auto &file : files) {
                if (file.getSize() == 0) {
                    if (file.deleteFile()) {
                        LOG_MSG_CF(LOG_DEBUG, "The file \"%s\" is empty and deleted successfully.",
                                   file.getFullPathName().toRawUTF8());
                    } else {
                        LOG_MSG_CF(LOG_ERROR, "Failed to delete the empty file \"%s\"",
                                   file.getFullPathName().toRawUTF8());
                    }
                }
            }
            dataDumpDir.findChildFiles(files, juce::File::findFiles, false);
            if (files.size() == 0) {
                if (dataDumpDir.deleteRecursively()) {
                    LOG_MSG_CF(LOG_DEBUG, "The folder \"%s\" is empty and deleted successfully.",
                               dataDumpDir.getFullPathName().toRawUTF8());
                } else {
                    LOG_MSG_CF(LOG_ERROR, "Failed to delete the empty folder \"%s\"",
                               dataDumpDir.getFullPathName().toRawUTF8());
                }
            }
        }
        processBlockCounter = 0;
        toReleaseResources = false;
        LOG_MSG(LOG_INFO, "released Resources");
    } else {
        LOG_MSG(LOG_WARN, "nothing to release");
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    LOG_MSG(LOG_DEBUG, "OutputChannelSet=" + std::to_string(layouts.getMainOutputChannelSet().size()) +
                           ", InputChannelSet=" + std::to_string(layouts.getMainInputChannelSet().size()));
    return true;
#endif
}
#endif

void DemoAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    if (!isLicenseValid || !isInitDone) {
        return;
    }

    // auto startTime = juce::Time::getMillisecondCounterHiRes();
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    validChannels = juce::jmin(totalNumInputChannels, MAX_SUPPORT_CHANNELS);
    int numSamples = buffer.getNumSamples();
    if (processBlockCounter++ == 0) {
        originalChannels = totalNumInputChannels;
        LOG_MSG(LOG_INFO, "processBlock: numSamples=" + std::to_string(numSamples) +
                              ", totalNumInputChannels=" + std::to_string(totalNumInputChannels) +
                              ", totalNumOutputChannels=" + std::to_string(totalNumOutputChannels) +
                              ", about " + std::to_string(numSamples * 1000.0f / originalSampleRate) + " milliseconds");
    }

    int bufferIndex = 0;
    float *pWrite[MAX_SUPPORT_CHANNELS] = {0};
    float *pRead[MAX_SUPPORT_CHANNELS] = {0};
    while (bufferIndex != numSamples) {
        for (int channel = 0; channel < validChannels; channel++) {
            pWrite[channel] = writeBuf[channel].get();
            pRead[channel] = readBuf[channel].get();
        }
        int numSamplesWrite = 0;
        numSamplesWrite = juce::jmin(numSamples - bufferIndex, blockSize - writeIndex);
        numSamplesWrite = juce::jmin(numSamplesWrite, blockSize - readIndex);
        for (int channel = 0; channel < validChannels; channel++) {
            std::memcpy(pWrite[channel] + writeIndex,
                        buffer.getReadPointer(channel, bufferIndex),
                        numSamplesWrite * sizeof(float));
        }
        writeIndex += numSamplesWrite;
        if (writeIndex == blockSize) {
            if (bypassEnable) {
                // do nothing or copy the input buffer to the output buffer
            } else {
                auto frameStart = juce::Time::getMillisecondCounterHiRes();
                for (int channel = 0; channel < validChannels; channel++) {
                    int ret = algo_process(algo_handle, pWrite[channel], pWrite[channel], blockSize);
                    if (ret != 0) {
                        LOG_MSG(LOG_ERROR, "Failed to algo_process. ret = " + std::to_string(ret));
                    }
                }
                auto frameStop = juce::Time::getMillisecondCounterHiRes();
                LOG_MSG(LOG_DEBUG, "algo_process frame " + std::to_string(algoFrameCounter++) +
                                       " elapsed time: " + std::to_string(frameStop - frameStart) + " ms");
                if (dataDumpEnable) {
                    if (validChannels == 1) {
                        dumpFloatPCMData(processedDataDumpFile, writeBuf[0].get(), blockSize);
                    } else if (validChannels == MAX_SUPPORT_CHANNELS) {
                        dumpFloatPCMData(processedDataDumpFile, writeBuf[0].get(), writeBuf[1].get(), blockSize);
                    }
                }
            }
        }
        for (int channel = 0; channel < validChannels; channel++) {
            std::memcpy(buffer.getWritePointer(channel, bufferIndex),
                        pRead[channel] + readIndex,
                        numSamplesWrite * sizeof(float));
        }
        bufferIndex += numSamplesWrite;
        if (writeIndex == blockSize) {
            for (int channel = 0; channel < validChannels; channel++) {
                writeBuf[channel].swap(readBuf[channel]);
            }
            writeIndex = 0;
        }
        readIndex += numSamplesWrite;
        if (readIndex == blockSize) {
            readIndex = 0;
        }
    }

    // auto stopTime = juce::Time::getMillisecondCounterHiRes();
    // auto elapsedTime = stopTime - startTime;
    // LOG_MSG(LOG_DEBUG, "processBlockCounter = " + std::to_string(processBlockCounter) +
    //                        ", elapsed time: " + std::to_string(elapsedTime) + " ms");
}

//==============================================================================
bool DemoAudioProcessor::hasEditor() const
{
    if (!isLicenseValid || !isInitDone) {
        return false;
    }
    LOG_MSG(LOG_INFO, "hasEditor: true");
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *DemoAudioProcessor::createEditor()
{
    LOG_MSG(LOG_INFO, "create editor");
    return new DemoAudioProcessorEditor(*this);
}

//==============================================================================
void DemoAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    if (anyParamChanged == false) {
        // in VST3 plugin, before restore parameters, call getStateInformation will store the default parameters
        // This will overwrite the user's last selection.
        LOG_MSG(LOG_WARN, "none of the parameters have been changed, skip store parameters to memory block");
        return;
    }
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);

    LOG_MSG(LOG_INFO, "store parameters to memory block:\n" + apvts.state.toXmlString().toStdString());
}

void DemoAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid() && tree.hasType("Parameters")) {
        apvts.state = tree;
        LOG_MSG(LOG_INFO, "restore parameters from memory block:\n" + apvts.state.toXmlString().toStdString());
        int lastLogLevelValue = static_cast<int>(apvts.getRawParameterValue("logLevel")->load() + 1);
        if (logger.getLogLevel() != static_cast<LogLevel_t>(lastLogLevelValue)) {
            logger.setLogLevel(static_cast<LogLevel_t>(lastLogLevelValue));
            LOG_MSG(LOG_INFO, "Log level has been set to " +
                                  std::to_string(apvts.getRawParameterValue("logLevel")->load()) + " by last state");
        }
        bool lastDataDumpEnableState = static_cast<bool>(apvts.getRawParameterValue("dataDumpEnable")->load());
        if (dataDumpEnable != lastDataDumpEnableState) {
            dataDumpEnable = lastDataDumpEnableState;
            LOG_MSG(LOG_INFO, "Data dump is " + std::string(dataDumpEnable ? "enabled" : "disabled") + " by last state");
        }
        bool lastBypassEnableState = static_cast<bool>(apvts.getRawParameterValue("bypassEnable")->load());
        if (bypassEnable != lastBypassEnableState) {
            bypassEnable = lastBypassEnableState;
            LOG_MSG(LOG_INFO, "Bypass is " + std::string(bypassEnable ? "enabled" : "disabled") + " by last state");
        }
        float lastGainValue = apvts.getRawParameterValue("gain")->load();
        if (gain != lastGainValue) {
            gain = lastGainValue;
        }
    } else {
        if (!tree.hasType("Parameters")) {
            LOG_MSG(LOG_DEBUG, "Read from memory block: " + tree.toXmlString().toStdString());
            LOG_MSG(LOG_ERROR, "Due to a major version update, you may need to save your settings as a preset again.");
        } else {
            LOG_MSG(LOG_ERROR, "Failed to restore parameters from memory block");
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout DemoAudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout paramsLayout;
    paramsLayout.add(std::make_unique<juce::AudioParameterChoice>(
        "logLevel", "Log Level", juce::StringArray{"DEBUG", "INFO", "WARN", "ERROR", "OFF"}, 1));
    paramsLayout.add(std::make_unique<juce::AudioParameterBool>("dataDumpEnable", "Data Dump", false));
    paramsLayout.add(std::make_unique<juce::AudioParameterBool>("bypassEnable", "Bypass", false));
    paramsLayout.add(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain", juce::NormalisableRange<float>(MIN_GAIN_VALUE, MAX_GAIN_VALUE, 0.001f), 0.0f));
    return paramsLayout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new DemoAudioProcessor();
}
