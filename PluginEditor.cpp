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

#define UI_WIDTH 400
#define UI_HEIGHT 300

#define MARGIN 10

#define BUTTON_WiDTH 100
#define BUTTON_HEIGHT 30

#define SLIDER_WIDTH 100
#define SLIDER_HEIGHT 100

#define SLIDER_TEXTBOX_WIDTH 100
#define SLIDER_TEXTBOX_HEIGHT 20

#define ENABLE_COLOR juce::Colours::lightgreen
#define DISABLE_COLOR juce::Colours::lightslategrey

//==============================================================================
CustomLookAndFeel customLookAndFeel;

DemoAudioProcessorEditor::DemoAudioProcessorEditor (DemoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    BypassButton.setButtonText("Bypass");
    DebugButton.setButtonText(JucePlugin_VersionString);
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
    DebugButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGBA(0, 0, 0, 0)); // Set to transparent
    BypassButton.addListener(this);
    DebugButton.addListener(this);
    addAndMakeVisible(BypassButton);
    addAndMakeVisible(DebugButton);

    // Initialize ComboBox but don't make it visible yet
    DebugButton.setLookAndFeel(&customLookAndFeel);
    logLevelComboBox.addItem("DEBUG", 1);
    logLevelComboBox.addItem("INFO", 2);
    logLevelComboBox.addItem("WARN", 3);
    logLevelComboBox.addItem("ERROR", 4);
    if (globalLogLevel == LOG_DEBUG) {
        logLevelComboBox.setSelectedId(1);
    } else if (globalLogLevel == LOG_INFO) {
        logLevelComboBox.setSelectedId(2);
    } else if (globalLogLevel == LOG_WARN) {
        logLevelComboBox.setSelectedId(3);
    } else if (globalLogLevel == LOG_ERROR) {
        logLevelComboBox.setSelectedId(4);
    }
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
    LOG_MSG(LOG_INFO, "UI initialized");
}

DemoAudioProcessorEditor::~DemoAudioProcessorEditor()
{
    BypassButton.removeListener(this);
    DebugButton.removeListener(this);
    logLevelComboBox.removeListener(this);
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
    auto bounds = getLocalBounds().reduced(MARGIN);
    GainSlider.setBounds((UI_WIDTH - SLIDER_WIDTH) / 2, UI_HEIGHT / 2 - SLIDER_HEIGHT, SLIDER_WIDTH, SLIDER_HEIGHT);
    BypassButton.setBounds((UI_WIDTH - BUTTON_WiDTH) / 2, UI_HEIGHT / 2 + BUTTON_HEIGHT + MARGIN, BUTTON_WiDTH, BUTTON_HEIGHT);
    DebugButton.setBounds(getWidth() - BUTTON_WiDTH - MARGIN, getHeight() - BUTTON_HEIGHT - MARGIN, BUTTON_WiDTH, BUTTON_HEIGHT);
    logLevelComboBox.setBounds((UI_WIDTH - BUTTON_WiDTH) / 2, getHeight() - BUTTON_HEIGHT - MARGIN, BUTTON_WiDTH, BUTTON_HEIGHT);

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    LOG_MSG(LOG_INFO, "UI resized");
}

void DemoAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &BypassButton) {
        audioProcessor.bypassEnable = !audioProcessor.bypassEnable;
        BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
        LOG_MSG(LOG_INFO, "Bypass is " + audioProcessor.bypassEnable ? "enabled" : "disabled");
    } else if (button == &DebugButton) {
        DebugButtonClickedTimes++;
        if (DebugButtonClickedTimes == 5) {
            logLevelComboBox.setVisible(true);
            DebugButtonClickedTimes = 0;
        }
    }
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
}

void DemoAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &logLevelComboBox) {
        int selectedId = logLevelComboBox.getSelectedId();
        switch (selectedId) {
            case 1:
                globalLogLevel = LOG_DEBUG;
                break;
            case 2:
                globalLogLevel = LOG_INFO;
                break;
            case 3:
                globalLogLevel = LOG_WARN;
                break;
            case 4:
                globalLogLevel = LOG_ERROR;
                break;
            default:
                break;
        }
        LOG_MSG(LOG_INFO, "Log level changed to " + logLevelComboBox.getText().toStdString());
    }
}

void DemoAudioProcessorEditor::updateParameterDisplays()
{
    GainSlider.setValue(audioProcessor.gain, juce::NotificationType::dontSendNotification);
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
    LOG_MSG(LOG_INFO, "Parameter displays updated from restored state");
}
