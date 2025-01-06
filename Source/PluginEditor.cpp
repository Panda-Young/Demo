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

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <JucePluginDefines.h> // Include this for JucePlugin_VersionString
#if JUCE_WINDOWS
#include <windows.h> // Include this for GetDpiForSystem
#endif

#define UI_WIDTH 400
#define UI_HEIGHT 220

#define MARGIN 10

#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 30

#define SLIDER_WIDTH 100
#define SLIDER_HEIGHT 100

#define SLIDER_TEXTBOX_WIDTH 100
#define SLIDER_TEXTBOX_HEIGHT 20

#define LABEL_WIDTH 100
#define LABEL_HEIGHT 20

//==============================================================================
void DemoAudioProcessorEditor::initializeUIComponents()
{
    addAndMakeVisible(versionButton);
    versionButton.setButtonText(JucePlugin_VersionString);
    versionButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGBA(0, 0, 0, 0)); // Set to transparent
    versionButton.setLookAndFeel(&borderlessButtonLookAndFeel);
    versionButton.addListener(this);

    addAndMakeVisible(logLevelComboBox);
    logLevelComboBox.addItem("DEBUG", LOG_DEBUG);
    logLevelComboBox.addItem("INFO", LOG_INFO);
    logLevelComboBox.addItem("WARN", LOG_WARN);
    logLevelComboBox.addItem("ERROR", LOG_ERROR);
    logLevelComboBox.addItem("OFF", LOG_OFF);
    logLevelComboBox.setVisible(false);
    logLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getApvts(), "logLevel", logLevelComboBox);
    logLevelComboBox.addListener(this);

    addAndMakeVisible(dataDumpButton);
    dataDumpButton.setButtonText("Data Dump");
    dataDumpButton.setVisible(false);
    dataDumpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getApvts(), "dataDumpEnable", dataDumpButton);
    dataDumpButton.addListener(this);

    addAndMakeVisible(bypassButton);
    bypassButton.setButtonText("Bypass");
    bypassButton.setLookAndFeel(&toggleButtonWithTextInsideLookAndFeel);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getApvts(), "bypassEnable", bypassButton);
    bypassButton.addListener(this);

    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, SLIDER_TEXTBOX_WIDTH, SLIDER_TEXTBOX_HEIGHT);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getApvts(), "gain", gainSlider);
    gainSlider.addListener(this);

    addAndMakeVisible(&gainLabel);
    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.attachToComponent(&gainSlider, true);
    gainLabel.setJustificationType(juce::Justification::centred);
}

DemoAudioProcessorEditor::DemoAudioProcessorEditor(DemoAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    initializeUIComponents();
    setSize(UI_WIDTH, UI_HEIGHT);

#if JUCE_WINDOWS
    if (audioProcessor.getUsedPluginType() == 3 &&
        audioProcessor.getUsedHostAppName() == "Adobe Audition" &&
        audioProcessor.getUsedHostAppVersion() >= 0 && audioProcessor.getUsedHostAppVersion() <= 2020) {
        float scaleFactor = static_cast<float>(GetDpiForSystem()) / 96.0f; // DPI scaling for Windows
        setSize(static_cast<int>(UI_WIDTH * scaleFactor), static_cast<int>(UI_HEIGHT * scaleFactor));
    }
#endif

    LOG_MSG(LOG_INFO, "UI initialized");
}

DemoAudioProcessorEditor::~DemoAudioProcessorEditor()
{
    versionButton.removeListener(this);
    versionButton.setLookAndFeel(nullptr);
    logLevelComboBox.removeListener(this);
    dataDumpButton.removeListener(this);

    bypassButton.removeListener(this);
    bypassButton.setLookAndFeel(nullptr);
    gainSlider.removeListener(this);
    LOG_MSG(LOG_INFO, "UI destroyed");
}

//==============================================================================
void DemoAudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    LOG_MSG(LOG_DEBUG, "UI painted");
}

void DemoAudioProcessorEditor::resized()
{
    int X_OFFSET = (UI_WIDTH - SLIDER_WIDTH) / 2;
    int Y_OFFSET = MARGIN;
    gainSlider.setBounds(X_OFFSET, Y_OFFSET, SLIDER_WIDTH, SLIDER_HEIGHT);
    X_OFFSET = (UI_WIDTH - BUTTON_WIDTH) / 2;
    Y_OFFSET += SLIDER_HEIGHT + MARGIN;
    bypassButton.setBounds(X_OFFSET, Y_OFFSET, BUTTON_WIDTH, BUTTON_HEIGHT);

    int bottom = UI_HEIGHT - BUTTON_HEIGHT - MARGIN;
    versionButton.setBounds(MARGIN, bottom, BUTTON_WIDTH, BUTTON_HEIGHT);
    logLevelComboBox.setBounds((int)((UI_WIDTH - BUTTON_WIDTH) / 2), bottom, BUTTON_WIDTH, BUTTON_HEIGHT);
    dataDumpButton.setBounds(UI_WIDTH - BUTTON_WIDTH - MARGIN, bottom, BUTTON_WIDTH, BUTTON_HEIGHT);

    LOG_MSG(LOG_DEBUG, "UI resized");
}

void DemoAudioProcessorEditor::buttonClicked(juce::Button *button)
{
    if (button == &versionButton) {
        versionButtonClickedTimes++;
        if (versionButtonClickedTimes == 5) {
            logLevelComboBox.setVisible(true);
            dataDumpButton.setVisible(true);
            versionButtonClickedTimes = -5;
        } else if (versionButtonClickedTimes == 0) {
            logLevelComboBox.setVisible(false);
            dataDumpButton.setVisible(false);
        }
    } else if (button == &dataDumpButton) {
        audioProcessor.setDataDumpState(dataDumpButton.getToggleState());
        LOG_MSG(LOG_INFO, "Data dump is " + std::string(audioProcessor.getDataDumpState() ? "enabled" : "disabled"));
    } else if (button == &bypassButton) {
        audioProcessor.setBypassState(bypassButton.getToggleState());
        LOG_MSG(LOG_INFO, "Bypass is " + std::string(audioProcessor.getBypassState() ? "enabled" : "disabled"));
    } else {
        // program should not reach here
        LOG_MSG(LOG_WARN, "Unknown button clicked");
    }
    audioProcessor.setAnyParamChanged(true);
}

void DemoAudioProcessorEditor::sliderValueChanged(juce::Slider *slider)
{
    if (slider == &gainSlider) {
        LOG_MSG(LOG_INFO, "Gain has been set to " + std::to_string(audioProcessor.getGainValue()) + " dB");
        audioProcessor.setGainValue(static_cast<float>(gainSlider.getValue()));
        float gainValue = audioProcessor.getGainValue();
        int ret = algo_set_param(audioProcessor.getAlgoHandle(), ALGO_PARAM2, &gainValue, (int)sizeof(float));
        if (ret != E_OK) {
            LOG_MSG(LOG_ERROR, "algo_set_param failed. ret = " + std::to_string(ret));
        }
    }
    audioProcessor.setAnyParamChanged(true);
}

void DemoAudioProcessorEditor::comboBoxChanged(juce::ComboBox *comboBox)
{
    if (comboBox == &logLevelComboBox) {
        audioProcessor.logger.setLogLevel(static_cast<LogLevel_t>(logLevelComboBox.getSelectedId()));
        LOG_MSG(LOG_INFO, "Log level changed to " + logLevelComboBox.getText().toStdString());
    }
    audioProcessor.setAnyParamChanged(true);
}
