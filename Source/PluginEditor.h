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

#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
/**
 */
class BorderlessButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        // Do not draw any background or border
    }
};

class ToggleButtonWithTextInsideLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = juce::jmin(15.0f, button.getHeight() * 0.75f);
        auto tickWidth = button.getWidth() * 0.8f;
        auto cornerSize = juce::jmin(button.getHeight(), button.getWidth()) * 0.15f;

        juce::Rectangle<float> tickBounds(4.0f, ((float)button.getHeight() - button.getHeight()) * 0.5f,
                                          tickWidth, button.getHeight());
        g.setColour(button.isEnabled() ? (button.getToggleState() ? juce::Colours::mediumseagreen : juce::Colours::grey)
                                       : juce::Colours::grey);
        g.fillRoundedRectangle(tickBounds, cornerSize);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(fontSize);

        g.drawFittedText(button.getButtonText(), tickBounds.toNearestInt(), juce::Justification::centred, 1);
    }
};

class DemoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 public juce::Button::Listener,
                                 public juce::Slider::Listener,
                                 public juce::ComboBox::Listener,
                                 public DemoAudioProcessor::Listener
{
public:
    DemoAudioProcessorEditor(DemoAudioProcessor &);
    ~DemoAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    void buttonClicked(juce::Button *button) override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;
    void sliderValueChanged(juce::Slider *slider) override;
    void initializeUIComponents();
    void updateUIComponentsFromProcessor();

    void bypassEnableChanged(bool newState) override
    {
        bypassButton.setToggleState(newState, juce::dontSendNotification);
        // don't sync notification, due to the fact that the button will trigger the bypassEnableChanged callback
    }
    void gainValueChanged(float newValue) override
    {
        gainSlider.setValue(newValue, juce::dontSendNotification);
    }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DemoAudioProcessor &audioProcessor;
    int versionButtonClickedTimes = 0;

    BorderlessButtonLookAndFeel borderlessButtonLookAndFeel;
    ToggleButtonWithTextInsideLookAndFeel toggleButtonWithTextInsideLookAndFeel;
    juce::TextButton versionButton;
    juce::ComboBox logLevelComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> logLevelAttachment;
    juce::ToggleButton dataDumpButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dataDumpAttachment;
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::Label gainLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoAudioProcessorEditor)
};
