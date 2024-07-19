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

#define UI_WIDTH 400
#define UI_HEIGHT 300

#define MARGIN 10

#define BUTTON_WiDTH 100
#define BUTTON_HEIGHT 30

#define SLIDER_WIDTH 100
#define SLIDER_HEIGHT 100

#define SLIDER_TEXTBOX_WIDTH 100
#define SLIDER_TEXTBOX_HEIGHT 20

//==============================================================================
DemoAudioProcessorEditor::DemoAudioProcessorEditor (DemoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    BypassButton.setButtonText("Bypass");
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? juce::Colours::lightgreen : juce::Colours::lightslategrey);
    BypassButton.setBounds(UI_WIDTH / 2 - BUTTON_WiDTH / 2, ((UI_HEIGHT / 4) * 3) - (BUTTON_HEIGHT / 2), BUTTON_WiDTH, BUTTON_HEIGHT);
    BypassButton.addListener(this);
    addAndMakeVisible(BypassButton);

    GainSlider.setSliderStyle(juce::Slider::Rotary);
    GainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, SLIDER_TEXTBOX_WIDTH, SLIDER_TEXTBOX_HEIGHT);
    GainSlider.setBounds(UI_WIDTH / 2 - SLIDER_WIDTH / 2, MARGIN + BUTTON_HEIGHT + MARGIN, SLIDER_WIDTH, SLIDER_HEIGHT);
    GainSlider.setRange(-20.0, 20.0, 0.1);
    GainSlider.setValue(audioProcessor.gain);
    GainSlider.addListener(this);
    addAndMakeVisible(GainSlider);

    setSize(UI_WIDTH, UI_HEIGHT);
    audioProcessor.logger->logMessage("UI initialized");
}

DemoAudioProcessorEditor::~DemoAudioProcessorEditor()
{
    audioProcessor.logger->logMessage("UI destroyed");
}

//==============================================================================
void DemoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // audioProcessor.logger->logMessage("UI painted");
}

void DemoAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    audioProcessor.logger->logMessage("UI resized");
}

void DemoAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &BypassButton) {
        audioProcessor.bypassEnable = !audioProcessor.bypassEnable;
        BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? juce::Colours::lightgreen : juce::Colours::lightslategrey);
        audioProcessor.logger->logMessage("Bypass is " + juce::String(audioProcessor.bypassEnable ? "enabled" : "disabled"));
    }
}

void DemoAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &GainSlider) {
        audioProcessor.gain = GainSlider.getValue();
        int ret = algo_set_param(audioProcessor.algo_handle, ALGO_PARAM2, &audioProcessor.gain, sizeof(float));
        if (ret != E_OK) {
            audioProcessor.logger->logMessage("algo_set_param failed. ret = " + juce::String(ret));
        } else {
            audioProcessor.logger->logMessage("Gain has been set to " + juce::String(audioProcessor.gain) + " dB");
        }
    }
}

void DemoAudioProcessorEditor::updateParameterDisplays()
{
    GainSlider.setValue(audioProcessor.gain, juce::NotificationType::dontSendNotification);
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? juce::Colours::lightgreen : juce::Colours::lightslategrey);
    audioProcessor.logger->logMessage("Parameter displays updated from restored state");
}
