/***************************************************************************
 * Description: Header of PluginEditor
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

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        // Do not draw any background or border
    }
};

class DemoAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Button::Listener,
                                public juce::Slider::Listener, public juce::ComboBox::Listener
{
public:
    DemoAudioProcessorEditor (DemoAudioProcessor&);
    ~DemoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void updateParameterDisplays();
    void sliderValueChanged(juce::Slider* slider) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DemoAudioProcessor& audioProcessor;
    juce::TextButton VersionButton;
    juce::ComboBox logLevelComboBox;
    int DebugButtonClickedTimes = 0;
    CustomLookAndFeel customLookAndFeel;

    juce::TextButton BypassButton;
    juce::Slider GainSlider;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoAudioProcessorEditor)
};
