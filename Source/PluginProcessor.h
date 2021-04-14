/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "shared.h"
#include "Grains.h"

//==============================================================================
/**
*/
class GrainsAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    GrainsAudioProcessor();
    ~GrainsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
	void grainMap(juce::AudioSampleBuffer& dest, juce::AudioSampleBuffer src[], juce::AudioSampleBuffer& origin);
	juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

private:
	juce::AudioProcessorValueTreeState vts;
	GrainScheduler scheduler;
	GrainPool pool;
	SharedFileBuffer fileBuffer;
	juce::AudioSampleBuffer grainBuffer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainsAudioProcessor)
};
