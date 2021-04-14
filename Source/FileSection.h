#pragma once
#include <JuceHeader.h>
#include "shared.h"
#define MAX_SAMPLES 48000*30
enum RangeHandleSide
{
	None,
	Start,
	End
};
struct RangeHandle : public juce::Component, public juce::ActionBroadcaster
{
	RangeHandle(juce::AudioProcessorValueTreeState& vts, SharedFileBuffer* buf);

	void paint(juce::Graphics& g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

	SharedFileBuffer* fileBuffer;
	struct grabContext
	{
		float relativeX;
		juce::NormalisableRange<float> range;
	} contexts[2];
	juce::Rectangle<int> grabArea[2];
	RangeHandleSide grabbed = RangeHandleSide::None;
	juce::ParameterAttachment startAtt;
	juce::ParameterAttachment endAtt;
};

class FileView : public juce::Component, public juce::ActionBroadcaster, public juce::ActionListener
{
public:
	FileView() = default;
	FileView(SharedFileBuffer* buf, juce::AudioProcessorValueTreeState& vts);
	void drawFile();

	void paint(juce::Graphics& g) override;
	void resized() override;

	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

	void actionListenerCallback(const juce::String& message) override;
	int mapPositionToSample(int x);
	SharedFileBuffer* fileBuffer;
	RangeHandle handle;
private:
	int samplesPerWindow = 50;
	RangeHandleSide grabbed = RangeHandleSide::None;
	bool firstTime = true;
	bool drawn = false;
	bool rangeChanged = true;
};
