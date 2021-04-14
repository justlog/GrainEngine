/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FileSection.h"
#include "shared.h"

//==============================================================================
/**
*/
class GrainAmount : public juce::Slider
{
	//TODO:Number of grains according to samplerate. for now 48,000 considered default.
	virtual juce::String getTextFromValue(double value) override
	{
		return juce::String(48000.0/getValue()) +  " Grains/sec";
	}
	virtual double getValueFromText(const juce::String& value) override
	{
		return 48000.0 / value.getDoubleValue();
	}

};
class GrainsAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::ActionListener
{
public:
    GrainsAudioProcessorEditor (GrainsAudioProcessor&, juce::AudioProcessorValueTreeState& vts, SharedFileBuffer *fileBuffer);
    ~GrainsAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void actionListenerCallback(const juce::String& message) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
	GrainsAudioProcessor& audioProcessor;
	juce::AudioProcessorValueTreeState& vtsRef;



	FileView fileView;
	juce::TextButton button;
	juce::Slider grainSize;
	GrainAmount interonset;
	juce::ToggleButton original;
	juce::ToggleButton randomScheduling;

	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> originalAtt;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> randomAtt;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAtt;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> interonsetAtt;

	juce::AudioFormatManager formatManager;
	SharedFileBuffer *fileBufferRef;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainsAudioProcessorEditor)
};
