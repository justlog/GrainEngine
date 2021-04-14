/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GrainsAudioProcessorEditor::GrainsAudioProcessorEditor (GrainsAudioProcessor& p, 
	juce::AudioProcessorValueTreeState& vts,
	SharedFileBuffer *fileBuffer)
    : AudioProcessorEditor (&p), audioProcessor (p), fileBufferRef(fileBuffer), fileView(fileBuffer, vts), vtsRef(vts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
	formatManager.registerBasicFormats();
	addAndMakeVisible(&button);
	addAndMakeVisible(&grainSize);
	addAndMakeVisible(&interonset);
	addAndMakeVisible(&randomScheduling);
	addAndMakeVisible(&original);
	addAndMakeVisible(&fileView);

	fileView.handle.addActionListener(this);

	interonset.setSliderStyle(juce::Slider::SliderStyle::Rotary);
	interonset.setSkewFactor(0.01);
	grainSizeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vts, "Grain Window", grainSize);
	interonsetAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vts, "Interonset", interonset);
	originalAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(vts, "Play Original", original);
	randomAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(vts, "Random Scheduling", randomScheduling);

	button.onClick = [this]()
	{
		fileBufferRef->loaded = false;
		juce::FileChooser chooser("Select a wave file shorter than 2 seconds to play.",
			{},
			"*.wav");

		if (chooser.browseForFileToOpen())
		{
			auto file = chooser.getResult();
			std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

			DBG(reader->sampleRate);
			if (reader.get() != nullptr)
			{
				if (reader->sampleRate > 48000)
				{
					DBG("Can't handle files with SR above 48000Hz!");
					return;
				}
				auto duration = (float)reader->lengthInSamples / reader->sampleRate;
				DBG(reader->lengthInSamples);
				if (duration < 30)
				{
					fileBufferRef->buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
					reader->read(&fileBufferRef->buffer,
						0,
						(int)reader->lengthInSamples,
						0,
						true,
						true);
					fileBufferRef->sampleRate = reader->sampleRate;
					fileBufferRef->position = 0;
					fileBufferRef->loaded = true;
					fileBufferRef->newFile = true;
					fileView.drawFile();
					button.setButtonText(file.getFileName());
					repaint();
				}
			}
			else
			{
			}
		}
		else
		{
		}
	};
}

GrainsAudioProcessorEditor::~GrainsAudioProcessorEditor()
{
}


void GrainsAudioProcessorEditor::actionListenerCallback(const juce::String& message)
{
	juce::String range = message.upToFirstOccurrenceOf("=", false, false) == "end" ? "Range End" : "Range Start";
	juce::var newValue = message.fromFirstOccurrenceOf("=", false, false).getIntValue();
	vtsRef.getParameterAsValue(range).setValue(newValue);
}
//==============================================================================
void GrainsAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::black);
    g.fillRect(fileView.getBounds());
}

void GrainsAudioProcessorEditor::resized()
{
	setResizable(true, true);
	auto localbounds = getLocalBounds();

	auto bottom = localbounds.removeFromBottom(50);
	button.setBounds(bottom.removeFromLeft(bottom.getWidth()/2));
	original.setBounds(bottom.removeFromBottom(bottom.getHeight()/2));
	randomScheduling.setBounds(bottom);

	bottom = localbounds.removeFromBottom(100);
	grainSize.setBounds(bottom.removeFromLeft(bottom.getWidth() / 2));
	interonset.setBounds(bottom);

	fileView.setBounds(localbounds.reduced(10));
}
