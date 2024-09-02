/***************************************************************************
 * Description: PluginEditor
 * version: 0.1.0
 * Author: Panda-Young
 * Date: 2024-07-06 15:29:30
 * Copyright (c) 2024 by Panda-Young, All Rights Reserved.
 **************************************************************************/

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JucePluginDefines.h>
#include <Windows.h>

#define UI_WIDTH 400
#define UI_HEIGHT 300

#define MARGIN 10

#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 30

#define SLIDER_WIDTH 100
#define SLIDER_HEIGHT 100

#define SLIDER_TEXTBOX_WIDTH 100
#define SLIDER_TEXTBOX_HEIGHT 20

#define ENABLE_COLOR juce::Colours::lightgreen
#define DISABLE_COLOR juce::Colours::lightslategrey

//==============================================================================
DemoAudioProcessorEditor::DemoAudioProcessorEditor (DemoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    VersionButton.setButtonText(JucePlugin_VersionString);
    VersionButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGBA(0, 0, 0, 0)); // Set to transparent
    VersionButton.addListener(this);
    addAndMakeVisible(VersionButton);
    VersionButton.setLookAndFeel(&customLookAndFeel);

    BypassButton.setButtonText("Bypass");
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
    BypassButton.addListener(this);
    addAndMakeVisible(BypassButton);

    // Initialize ComboBox but don't make it visible yet
    logLevelComboBox.addItem("DEBUG", LOG_DEBUG);
    logLevelComboBox.addItem("INFO", LOG_INFO);
    logLevelComboBox.addItem("WARN", LOG_WARN);
    logLevelComboBox.addItem("ERROR", LOG_ERROR);
    logLevelComboBox.addItem("OFF", LOG_OFF);
    logLevelComboBox.setSelectedId(globalLogLevel);
    addAndMakeVisible(logLevelComboBox);
    logLevelComboBox.setVisible(false);
    logLevelComboBox.addListener(this);

    GainSlider.setSliderStyle(juce::Slider::Rotary);
    GainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, SLIDER_TEXTBOX_WIDTH, SLIDER_TEXTBOX_HEIGHT);
    GainSlider.setRange(-20.0f, 20.0f, 0.1f);
    GainSlider.setValue(audioProcessor.gain);
    GainSlider.addListener(this);
    addAndMakeVisible(GainSlider);

    setSize(UI_WIDTH, UI_HEIGHT);
    if (audioProcessor.pluginType == 3 &&
        audioProcessor.hostAppName == "Adobe Audition" &&
        audioProcessor.hostAppVersion >= 0 && audioProcessor.hostAppVersion <= 2020) {
        float scaleFactor = GetDpiForSystem() / 96.0f; // DPI scaling for Windows
        setSize(UI_WIDTH * scaleFactor, UI_HEIGHT * scaleFactor);
    }

    LOG_MSG(LOG_INFO, "UI initialized");
}

DemoAudioProcessorEditor::~DemoAudioProcessorEditor()
{
    VersionButton.removeListener(this);
    VersionButton.setLookAndFeel(nullptr);
    logLevelComboBox.removeListener(this);

    BypassButton.removeListener(this);
    GainSlider.removeListener(this);
    LOG_MSG(LOG_INFO, "UI destroyed");
}

//==============================================================================
void DemoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    LOG_MSG(LOG_DEBUG, "UI painted");
}

void DemoAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds().reduced(MARGIN);
    GainSlider.setBounds((UI_WIDTH - SLIDER_WIDTH) / 2, UI_HEIGHT / 2 - SLIDER_HEIGHT, SLIDER_WIDTH, SLIDER_HEIGHT);
    BypassButton.setBounds((UI_WIDTH - BUTTON_WIDTH) / 2, UI_HEIGHT / 2 + BUTTON_HEIGHT + MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT);
    VersionButton.setBounds(0, BypassButton.getY() + BypassButton.getHeight() + MARGIN * 4, BUTTON_WIDTH, BUTTON_HEIGHT);
    logLevelComboBox.setBounds((UI_WIDTH - BUTTON_WIDTH) / 2, VersionButton.getY(), BUTTON_WIDTH, BUTTON_HEIGHT);

    LOG_MSG(LOG_INFO, "UI resized");
}

void DemoAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &BypassButton) {
        audioProcessor.bypassEnable = !audioProcessor.bypassEnable;
        BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
        std::string msg = "Bypass is " + std::string(audioProcessor.bypassEnable ? "enabled" : "disabled");
        LOG_MSG(LOG_INFO, msg);
    } else if (button == &VersionButton) {
        DebugButtonClickedTimes++;
        if (DebugButtonClickedTimes == 5) {
            logLevelComboBox.setVisible(true);
            DebugButtonClickedTimes = 0;
        }
    }
    audioProcessor.anyParamChanged = true;
}

void DemoAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &GainSlider) {
        audioProcessor.gain = GainSlider.getValue();
        int ret = algo_set_param(audioProcessor.algo_handle, ALGO_PARAM2, &audioProcessor.gain, sizeof(float));
        if (ret != E_OK) {
            LOG_MSG(LOG_INFO, "algo_set_param failed. ret = " + std::to_string(ret));
        } else {
            LOG_MSG(LOG_INFO, "Gain has been set to " + std::to_string(audioProcessor.gain) + " dB");
        }
    }
    audioProcessor.anyParamChanged = true;
}

void DemoAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &logLevelComboBox) {
        if (logLevelComboBox.getSelectedId() != globalLogLevel) {
            globalLogLevel = (LogLevel)logLevelComboBox.getSelectedId();
            if (globalLogLevel < LOG_DEBUG || globalLogLevel > LOG_OFF) {
                globalLogLevel = LOG_INFO;
            }
            LOG_MSG(LOG_INFO, "Log level changed to " + logLevelComboBox.getText().toStdString());
        }
    }
    audioProcessor.anyParamChanged = true;
}

void DemoAudioProcessorEditor::updateParameterDisplays()
{
    GainSlider.setValue(audioProcessor.gain, juce::NotificationType::dontSendNotification);
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
    logLevelComboBox.setSelectedId(globalLogLevel, juce::NotificationType::dontSendNotification);
    LOG_MSG(LOG_INFO, "Parameter displays updated from restored state");
}
