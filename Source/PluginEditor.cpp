#include "PluginEditor.h"
#include "PluginProcessor.h"

DemoAudioProcessorEditor::DemoAudioProcessorEditor (DemoAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (400, 300);
}

DemoAudioProcessorEditor::~DemoAudioProcessorEditor()
{
}

void DemoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Demo VST with 64-bit Child Process", getLocalBounds(), juce::Justification::centred, 1);
}

void DemoAudioProcessorEditor::resized()
{
}
