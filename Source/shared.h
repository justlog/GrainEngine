#pragma once
#include <JuceHeader.h>
struct SharedFileBuffer
{
	juce::AudioSampleBuffer buffer;
	int position;
	double sampleRate;
	juce::Atomic<bool> loaded = false;
	juce::Atomic<bool> newFile = false;
};
