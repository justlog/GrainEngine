/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GrainsAudioProcessor::makeLayout()
{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	scheduler.setup(layout);
	pool.setParamters(layout);
	//layout.add(std::make_unique<juce::AudioParameterFloat>("GSIZE",
	//	"Grain Size", 10.f, 300.f, 50.f));
	layout.add(std::make_unique<juce::AudioParameterBool>("Play Original", "Play Original", true));
	return layout;
}

GrainsAudioProcessor::GrainsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), scheduler(), vts(*this, nullptr, "Paramaters", makeLayout())
#endif
{
}

GrainsAudioProcessor::~GrainsAudioProcessor()
{
}

//==============================================================================
const juce::String GrainsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GrainsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GrainsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GrainsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GrainsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GrainsAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GrainsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GrainsAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GrainsAudioProcessor::getProgramName (int index)
{
    return {};
}

void GrainsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GrainsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	pool.sampleRate = sampleRate;
	pool.envelopeSetup();
}

void GrainsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GrainsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

#define FILE_PROCCESSING

void GrainsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

#ifdef FILE_PROCCESSING
	if (fileBuffer.loaded.get())
	{
		//TODO: safe realtime file loading.
		if (fileBuffer.newFile.get())
		{
			grainBuffer.makeCopyOf(fileBuffer.buffer);
			pool.grains.resize(grainBuffer.getNumChannels());
			scheduler.nextOnsets.resize(grainBuffer.getNumChannels(), 1);

			//TODO: when loading a file, if the sample rate mismatches with the playback sample rate we get pitch shifting.
			/*Might want to assert that we're loading a file that is the same playback atm and later on check how to
			convert the samplerate.*/

			//pool.sampleRate = fileBuffer.sampleRate;
			pool.setup(&grainBuffer, grainBuffer.getNumSamples());
			fileBuffer.newFile = false;
		}

		if (vts.getParameterAsValue("Play Original") == 1.f)
		{
			auto bufferSamplesRemaining = fileBuffer.buffer.getNumSamples() - fileBuffer.position;
			auto numInputChannels = fileBuffer.buffer.getNumChannels();
			auto samplesThisTime = juce::jmin(buffer.getNumSamples(), bufferSamplesRemaining);
			for (auto channel = 0; channel < totalNumOutputChannels; ++channel)
			{
				auto* channelDest = buffer.getWritePointer(channel);
				buffer.copyFrom(channel,
					0,
					fileBuffer.buffer,
					channel % numInputChannels,
					fileBuffer.position,
					samplesThisTime
				);
			}
			fileBuffer.position += samplesThisTime;
			if (fileBuffer.position == fileBuffer.buffer.getNumSamples())
				fileBuffer.position = 0;
		}//Play file
		else
		{
			if (grainBuffer.getNumChannels() >= 2)
			{
				int maxGrainBuffers = pool.grains.size();
				for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
				{
					float* channelWrite = buffer.getWritePointer(channel);
					for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
					{
						channelWrite[sample] = scheduler.synthesize(pool, channel % maxGrainBuffers);
					}
				}
			}
			else
			{
				float* channelLeft = buffer.getWritePointer(0);
				float* channelRight = buffer.getWritePointer(1);
				for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
				{
					float value = scheduler.synthesize(pool, 0);
					channelLeft[sample] = value;
					channelRight[sample] = value;
				}
			}
		}//Play Synth
	}//File loaded
	else
	{
	}
#else//Realtime processing.
	grainBuffer.makeCopyOf(fileBuffer.buffer);
	pool.grains.resize(grainBuffer.getNumChannels());
	scheduler.nextOnsets.resize(grainBuffer.getNumChannels(), 1);
	pool.initPool(grainBuffer, grainBuffer.getNumSamples());
	if (pool.lastWindow != pool.grainWindow->get())
	{
		pool.lastWindow = pool.grainWindow->get();
		pool.initPool(grainBuffer, grainBuffer.getNumSamples());
	}
	for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
	{
		float* channelWrite = buffer.getWritePointer(channel);
		for(auto sample = 0; sample < buffer.getNumSamples(); ++sample)
	}
#endif
}
//==============================================================================
bool GrainsAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GrainsAudioProcessor::createEditor()
{
    return new GrainsAudioProcessorEditor (*this, vts, &fileBuffer);
}

//==============================================================================
void GrainsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void GrainsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrainsAudioProcessor();
}
