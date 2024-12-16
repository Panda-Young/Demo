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

#define LABEL_WIDTH 100
#define LABEL_HEIGHT 20

//==============================================================================
DemoAudioProcessorEditor::DemoAudioProcessorEditor(DemoAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    versionButton.setButtonText(JucePlugin_VersionString);
    versionButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGBA(0, 0, 0, 0)); // Set to transparent
    versionButton.addListener(this);
    addAndMakeVisible(versionButton);
    versionButton.setLookAndFeel(&borderlessButtonLookAndFeel);

    // Initialize ComboBox but don't make it visible yet
    logLevelComboBox.addItem("DEBUG", LOG_DEBUG);
    logLevelComboBox.addItem("INFO", LOG_INFO);
    logLevelComboBox.addItem("WARN", LOG_WARN);
    logLevelComboBox.addItem("ERROR", LOG_ERROR);
    logLevelComboBox.addItem("OFF", LOG_OFF);
    addAndMakeVisible(logLevelComboBox);
    logLevelComboBox.setVisible(false);
    logLevelComboBox.addListener(this);
    logLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "logLevel", logLevelComboBox);

    dataDumpButton.setButtonText("Data Dump");
    dataDumpButton.addListener(this);
    addAndMakeVisible(dataDumpButton);
    dataDumpButton.setVisible(false);
    dataDumpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "dataDumpEnable", dataDumpButton);

    bypassButton.setButtonText("Bypass");
    bypassButton.addListener(this);
    addAndMakeVisible(bypassButton);
    bypassButton.setLookAndFeel(&toggleButtonWithTextInsideLookAndFeel);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "bypassEnable", bypassButton);

    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, SLIDER_TEXTBOX_WIDTH, SLIDER_TEXTBOX_HEIGHT);
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "gain", gainSlider);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.attachToComponent(&gainSlider, true);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&gainLabel);

    setSize(UI_WIDTH, UI_HEIGHT);
    if (audioProcessor.getUsedPluginType() == 3 &&
        audioProcessor.getUsedHostAppName() == "Adobe Audition" &&
        audioProcessor.getUsedHostAppVersion() >= 0 && audioProcessor.getUsedHostAppVersion() <= 2020) {
        float scaleFactor = GetDpiForSystem() / 96.0f; // DPI scaling for Windows
        setSize(UI_WIDTH * scaleFactor, UI_HEIGHT * scaleFactor);
    }

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
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds().reduced(MARGIN);
    gainSlider.setBounds((UI_WIDTH - SLIDER_WIDTH) / 2, MARGIN, SLIDER_WIDTH, SLIDER_HEIGHT);
    bypassButton.setBounds((UI_WIDTH - BUTTON_WIDTH) / 2, MARGIN * 2 + SLIDER_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);

    int bottom = UI_HEIGHT - BUTTON_HEIGHT - MARGIN;
    versionButton.setBounds(MARGIN, bottom, BUTTON_WIDTH, BUTTON_HEIGHT);
    logLevelComboBox.setBounds((UI_WIDTH - BUTTON_WIDTH) / 2, bottom, BUTTON_WIDTH, BUTTON_HEIGHT);
    dataDumpButton.setBounds(UI_WIDTH - BUTTON_WIDTH - MARGIN, bottom, BUTTON_WIDTH, BUTTON_HEIGHT);

    LOG_MSG(LOG_INFO, "UI resized");
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
        audioProcessor.setBypassState(!audioProcessor.getBypassState());
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
        audioProcessor.setGainValue(gainSlider.getValue());
        float gainValue = audioProcessor.getGainValue();
        int ret = algo_set_param(audioProcessor.getAlgoHandle(), ALGO_PARAM2, &gainValue, sizeof(float));
        if (ret != E_OK) {
            LOG_MSG(LOG_ERROR, "algo_set_param failed. ret = " + std::to_string(ret));
        }
    }
    audioProcessor.setAnyParamChanged(true);
}

void DemoAudioProcessorEditor::comboBoxChanged(juce::ComboBox *comboBox)
{
    if (comboBox == &logLevelComboBox) {
        audioProcessor.logger.setLogLevel(static_cast<LogLevel>(logLevelComboBox.getSelectedId()));
        LOG_MSG(LOG_INFO, "Log level changed to " + logLevelComboBox.getText().toStdString());
    }
    audioProcessor.setAnyParamChanged(true);
}
