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

#define ENABLE_COLOR juce::Colours::lightgreen
#define DISABLE_COLOR juce::Colours::lightslategrey

//==============================================================================
DemoAudioProcessorEditor::DemoAudioProcessorEditor (DemoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    BypassButton.setButtonText("Bypass");
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
    BypassButton.addListener(this);
    addAndMakeVisible(BypassButton);

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
    LOG_MSG(LOG_INFO, "UI destroyed");
}

//==============================================================================
void DemoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // LOG_MSG(LOG_INFO, "UI painted");
}

void DemoAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(MARGIN);
    BypassButton.setBounds((UI_WIDTH - BUTTON_WiDTH) / 2, UI_HEIGHT - (BUTTON_HEIGHT + MARGIN), BUTTON_WiDTH, BUTTON_HEIGHT);
    GainSlider.setBounds((UI_WIDTH - SLIDER_WIDTH) / 2, UI_HEIGHT / 2 - SLIDER_HEIGHT, SLIDER_WIDTH, SLIDER_HEIGHT);

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

void DemoAudioProcessorEditor::updateParameterDisplays()
{
    GainSlider.setValue(audioProcessor.gain, juce::NotificationType::dontSendNotification);
    BypassButton.setColour(juce::TextButton::buttonColourId, audioProcessor.bypassEnable ? ENABLE_COLOR : DISABLE_COLOR);
    LOG_MSG(LOG_INFO, "Parameter displays updated from restored state");
}
