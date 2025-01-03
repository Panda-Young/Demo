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

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define BYPASS_MDII_CONTROLLER_NUMBER 0x13
#define BYPASS_ENABLE_MDII_CONTROLLER_VALUE 0x7F
#define BYPASS_DISABLE_MDII_CONTROLLER_VALUE 0x00
#define BYPASS_MDII_CHANNEL 1
#define GAIN_MIDI_CONTROLLER_NUMBER 0x14
#define GAIN_MDII_CHANNEL 1
#define MAX_GAIN_VALUE 20.0f
#define MIN_GAIN_VALUE -20.0f
#define MAX_MIDI_CONTROL_VALUE 127
#define MIN_MIDI_CONTROL_VALUE 0

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
    for (int channel = 0; channel < MAX_SUPPORT_CHANNELS; channel++) {
        write_buf[channel] = std::make_unique<float[]>(block_size);
        if (!write_buf[channel]) {
            LOG_MSG(LOG_ERROR, "Failed to allocate memory for write_buf[" + std::to_string(channel) + "]");
            isInitDone = false;
            return;
        }
        read_buf[channel] = std::make_unique<float[]>(block_size);
        if (!read_buf[channel]) {
            LOG_MSG(LOG_ERROR, "Failed to allocate memory for read_buf[" + std::to_string(channel) + "]");
            isInitDone = false;
            return;
        }
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

        options.content->setSize(400, 200);
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
    LOG_MSG(LOG_INFO, "AudioProcessor initialized successfully.");
}

DemoAudioProcessor::~DemoAudioProcessor()
{
    for (int channel = 0; channel < MAX_SUPPORT_CHANNELS; channel++) {
        if (write_buf[channel]) {
            write_buf[channel].reset();
        }
        if (read_buf[channel]) {
            read_buf[channel].reset();
        }
    }
    if (algo_handle != nullptr) {
        algo_deinit(algo_handle);
        algo_handle = nullptr;
    }

    LOG_MSG(LOG_INFO, "AudioProcessor destroyed. Log stop. Closed plugins or software");
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
    double tailLength = static_cast<double>(block_size) / originalSampleRate;
    LOG_MSG(LOG_INFO, "getTailLengthSeconds: " + std::to_string(tailLength));
    return tailLength;
}

int DemoAudioProcessor::getNumPrograms()
{
    LOG_MSG(LOG_DEBUG, "getNumPrograms: 1");
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int DemoAudioProcessor::getCurrentProgram()
{
    LOG_MSG(LOG_DEBUG, "getCurrentProgram: 0");
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
void DemoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    originalSampleRate = sampleRate;
    originalChannels = getTotalNumInputChannels();
    LOG_MSG(LOG_INFO, "prepareToPlay: sampleRate=" + std::to_string(sampleRate) +
                          ", samplesPerBlock=" + std::to_string(samplesPerBlock) +
                          ", about " + std::to_string(samplesPerBlock * 1000.0f / sampleRate) + " milliseconds");

    setLatencySamples(block_size);
    LOG_MSG(LOG_INFO, "set latency samples: " + std::to_string(block_size));
    processBlockCounter = 0;

    juce::File UserDesktop = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    dataDumpDir = UserDesktop.getChildFile(JucePlugin_Name + juce::String("_DataDump"));
    dataDumpDir.createDirectory();

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
}

void DemoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if (processBlockCounter) {
        convertPCMtoWAV(processedDataDumpFile, originalChannels, originalSampleRate, 32, 3);
    }
    if (dataDumpDir.isDirectory()) {
        juce::Array<juce::File> files;
        dataDumpDir.findChildFiles(files, juce::File::findFiles, false);
        for (auto &file : files) {
            if (file.getSize() == 0) {
                file.deleteFile();
            }
        }
        dataDumpDir.findChildFiles(files, juce::File::findFiles, false);
        if (files.size() == 0) {
            dataDumpDir.deleteRecursively();
        }
    }
    processBlockCounter = 0;

    LOG_MSG(LOG_INFO, "released Resources");
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
    int numSamples = buffer.getNumSamples();
    if (processBlockCounter++ == 0) {
        LOG_MSG(LOG_INFO, "processBlock: numSamples=" + std::to_string(numSamples) +
                              ", totalNumInputChannels=" + std::to_string(totalNumInputChannels) +
                              ", totalNumOutputChannels=" + std::to_string(totalNumOutputChannels) +
                              ", about " + std::to_string(numSamples * 1000.0f / originalSampleRate) + " milliseconds");
    }

    for (const auto &midiEvent : midiMessages) {
        const auto &message = midiEvent.getMessage();
        LOG_MSG(LOG_DEBUG, "Midi Message: " + message.getDescription().toStdString());
        if (message.isController()) {
            int controllerNumber = message.getControllerNumber();
            int controllerValue = message.getControllerValue();
            int channel = message.getChannel();
            LOG_MSG(LOG_DEBUG, "MIDI Controller Number: " + std::to_string(controllerNumber) +
                                   ", Controller Value: " + std::to_string(controllerValue) +
                                   ", Channel: " + std::to_string(channel));
            if (controllerNumber == BYPASS_MDII_CONTROLLER_NUMBER) {
                if (controllerValue == BYPASS_ENABLE_MDII_CONTROLLER_VALUE && channel == BYPASS_MDII_CHANNEL) {
                    if (!bypassEnable) {
                        bypassEnable = true;
                        notifyBypassEnableChanged();
                        LOG_MSG(LOG_INFO, "Bypass is enabled by MIDI Controller");
                    }
                } else if (controllerValue == BYPASS_DISABLE_MDII_CONTROLLER_VALUE && channel == BYPASS_MDII_CHANNEL) {
                    if (bypassEnable) {
                        bypassEnable = false;
                        notifyBypassEnableChanged();
                        LOG_MSG(LOG_INFO, "Bypass is disabled by MIDI Controller");
                    }
                } else {
                    LOG_MSG(LOG_WARN, "Invalid MIDI Controller Value: " + std::to_string(controllerValue) +
                                          ", or Channel: " + std::to_string(channel) +
                                          ", for Controller Number: " + std::to_string(controllerNumber));
                }
            } else if (controllerNumber == GAIN_MIDI_CONTROLLER_NUMBER && channel == GAIN_MDII_CHANNEL) {
                float newGain = (static_cast<float>(controllerValue) / MAX_MIDI_CONTROL_VALUE) *
                                    (MAX_GAIN_VALUE - MIN_GAIN_VALUE) +
                                MIN_GAIN_VALUE;
                if (newGain != gain) {
                    gain = newGain;
                    int ret = algo_set_param(algo_handle, ALGO_PARAM2, &gain, sizeof(float));
                    if (ret != 0) {
                        LOG_MSG(LOG_ERROR, "Failed to algo_set_param. ret = " + std::to_string(ret));
                    }
                    notifyGainValueChanged();
                    LOG_MSG(LOG_INFO, "Gain is set to " + std::to_string(gain) + " dB by MIDI Controller");
                }
            }
        }
    }

    int buffer_index = 0;
    float *p_write[MAX_SUPPORT_CHANNELS] = {0};
    float *p_read[MAX_SUPPORT_CHANNELS] = {0};
    int valid_channels = MIN(totalNumInputChannels, MAX_SUPPORT_CHANNELS);
    if (originalChannels != totalNumInputChannels) {
        originalChannels = totalNumInputChannels;
    }

    while (buffer_index != numSamples) {
        for (int channel = 0; channel < valid_channels; channel++) {
            p_write[channel] = write_buf[channel].get();
            p_read[channel] = read_buf[channel].get();
        }
        int n_samples_to_write = 0;
        n_samples_to_write = MIN(numSamples - buffer_index, block_size - write_index);
        n_samples_to_write = MIN(n_samples_to_write, block_size - read_index);
        for (int sample = 0; sample < n_samples_to_write; sample++) {
            for (int channel = 0; channel < valid_channels; channel++) {
                write_buf[channel][write_index + sample] = buffer.getSample(channel, buffer_index + sample);
            }
        }
        write_index += n_samples_to_write;
        if (write_index == block_size) {
            auto frameStart = juce::Time::getMillisecondCounterHiRes();
            if (bypassEnable) {
                // do nothing or copy the input buffer to the output buffer
            } else {
                for (int channel = 0; channel < valid_channels; channel++) {
                    int ret = algo_process(algo_handle, write_buf[channel].get(), write_buf[channel].get(), block_size);
                    if (ret != 0) {
                        LOG_MSG(LOG_ERROR, "Failed to algo_process. ret = " + std::to_string(ret));
                    }
                }
                if (dataDumpEnable) {
                    if (valid_channels == 1) {
                        dumpFloatPCMData(processedDataDumpFile, write_buf[0].get(), block_size);
                    } else if (valid_channels == MAX_SUPPORT_CHANNELS) {
                        dumpFloatPCMData(processedDataDumpFile, write_buf[0].get(), write_buf[1].get(), block_size);
                    }
                }
            }
            auto frameStop = juce::Time::getMillisecondCounterHiRes();
            LOG_MSG(LOG_DEBUG, "algo_process frame " + std::to_string(algoFrameCounter++) +
                                   " elapsed time: " + std::to_string(frameStop - frameStart) + " ms");
        }
        for (int sample = 0; sample < n_samples_to_write; sample++) {
            for (int channel = 0; channel < valid_channels; channel++) {
                buffer.setSample(channel, buffer_index + sample, read_buf[channel][read_index + sample]);
            }
        }
        buffer_index += n_samples_to_write;
        if (write_index == block_size) {
            for (int channel = 0; channel < valid_channels; channel++) {
                write_buf[channel].swap(read_buf[channel]);
            }
            write_index = 0;
        }
        read_index += n_samples_to_write;
        if (read_index == block_size) {
            read_index = 0;
        }
    }

    midiMessages.clear();

    double sampleRate = getSampleRate();
    double blockDuration = buffer.getNumSamples() / sampleRate;
    midiControllerTimeElapsed += blockDuration;

    if (midiControllerTimeElapsed >= midiControllerinterval) {
        midiControllerTimeElapsed -= midiControllerinterval;
        juce::MidiMessage BypassmidiMessage, gainMidiMessage;
        if (sendZeroValue) {
            BypassmidiMessage = juce::MidiMessage::controllerEvent(BYPASS_MDII_CHANNEL, BYPASS_MDII_CONTROLLER_NUMBER,
                                                                   BYPASS_DISABLE_MDII_CONTROLLER_VALUE);
        } else {
            BypassmidiMessage = juce::MidiMessage::controllerEvent(BYPASS_MDII_CHANNEL, BYPASS_MDII_CONTROLLER_NUMBER,
                                                                   BYPASS_ENABLE_MDII_CONTROLLER_VALUE);
        }
        midiMessages.addEvent(BypassmidiMessage, 0);
        sendZeroValue = !sendZeroValue;
        midiGain += 0.5f;
        if (midiGain > 20.0f) {
            midiGain = -20.0f;
        }
        gainMidiMessage = juce::MidiMessage::controllerEvent(
            GAIN_MDII_CHANNEL, GAIN_MIDI_CONTROLLER_NUMBER,
            static_cast<int>(std::round((midiGain - MIN_GAIN_VALUE) / (MAX_GAIN_VALUE - MIN_GAIN_VALUE) *
                                        MAX_MIDI_CONTROL_VALUE)));
        midiMessages.addEvent(gainMidiMessage, 0);
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
        bypassEnable = static_cast<bool>(apvts.getRawParameterValue("bypassEnable")->load());
        dataDumpEnable = static_cast<bool>(apvts.getRawParameterValue("dataDumpEnable")->load());
        gain = apvts.getRawParameterValue("gain")->load();
        int ret = algo_set_param(algo_handle, ALGO_PARAM2, &gain, sizeof(float));
        if (ret != 0) {
            LOG_MSG(LOG_ERROR, "Failed to algo_set_param. ret = " + std::to_string(ret));
        }
        int logLevelValue = static_cast<int>(apvts.getRawParameterValue("logLevel")->load() + 1);
        logger.setLogLevel(static_cast<LogLevel>(logLevelValue));
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
