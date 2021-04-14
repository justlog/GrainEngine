#include <JuceHeader.h>
#include "Grains.h"
#define MAX_SAMPLES 48000*30
namespace Envelopes
{
	//TODO:Better calcualtion.
	float calculateEnvelopeParabolic(int currentSample, int numSamples)
	{
		const float grainAmplitude = 1.0f;
		float amplitude = 0.f;
		const float rdur = 1.0f / numSamples;
		const float rdur2 = rdur * rdur;
		const float curve = -8.0f * grainAmplitude * rdur2;
		float slope = 4.0f * grainAmplitude * (rdur - rdur2);

		for (int i = 0; i < currentSample; ++i)
		{
			amplitude += slope;
			slope += curve;
		}
		return amplitude;
	}
	void calculateEnvelopeParabolicArray(std::vector<float>& write)
	{
		const int numSamples = write.size();
		const float grainAmplitude = 1.0f;
		float amplitude = 0.f;
		const float rdur = 1.0f / numSamples;
		const float rdur2 = rdur * rdur;
		const float curve = -8.0f * grainAmplitude * rdur2;
		float slope = 4.0f * grainAmplitude * (rdur - rdur2);

		for (int i = 0; i < numSamples; ++i)
		{
			write[i] = amplitude;
			amplitude += slope;
			slope += curve;
		}
	}
}

//GrainQueue
//===============================================================================================
GrainQueue::GrainQueue() : activeGrains(0), lastActive(-1){}
void GrainQueue::insert(int position, int endingSample, uint8_t settings)
{
	if (position >= endingSample && !(settings & GrainProperties::reverse)) data[-1];
	for (int j = lastActive+1; j < MAX_GRAINS; ++j)
	{
		if (!(data[j].properties & GrainProperties::active))
		{
			data[j].pos = position;
			data[j].maxSample = endingSample;
			data[j].properties = settings;
			data[j].size = abs(endingSample - position);
			++activeGrains;

			lastActive = j == MAX_GRAINS-1 ? -1 : j;
			break;
		}
	}
}
void GrainQueue::reset()
{
	for (Grain& g : data) g.properties = GrainProperties::none;
}
void GrainQueue::deactivate(int index)
{
	data[index].properties = GrainProperties::none;
	--activeGrains;
}
//===============================================================================================


//GrainPool
//===============================================================================================
void GrainPool::setParamters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
	ParameterMaker maker(layout);
	grainWindow = maker.addNamed<juce::AudioParameterFloat>("Grain Window", juce::NormalisableRange<float>(SMALLEST_WINDOW, BIGGEST_WINDOW, 0.001f), 0.05f, juce::String(),
		juce::AudioProcessorParameter::genericParameter,
		[](float value, int maximumStringLength)
		{
			return juce::String(value * 1000.f);
		},
		[](const juce::String& text)
		{
			return text.getFloatValue() / 1000.f;
		});

	rangeStart = maker.addNamed<juce::AudioParameterInt>("Range Start", 0, MAX_SAMPLES, 0);
	rangeEnd = maker.addNamed<juce::AudioParameterInt>("Range End", 0, MAX_SAMPLES, MAX_SAMPLES);

	lastWindow = grainWindow->get();
	start = rangeStart->get();
	end = rangeEnd->get();
}
//TODO: Translate into freestanding envelopes
#if 0
void GrainPool::calculateEnvelopeLinear()
{
	int numSamples = envelope.size();
	float increment = 1.0f / numSamples;
	float current = 0.f;
	for (int sample = 0; sample < numSamples; ++sample, current += increment)
	{
		envelope[sample] = current;
	}
}
void GrainPool::calculateEnvelopeRaisedCosine()
{
	int grainSize = envelope.size();
	int attackSamples = grainSize * 0.30;
	int releaseSmaples = grainSize * 0.10;
	int sustainSamples = grainSize * 0.60;
	float grainAmp = 1.0f;

	auto sample = 0;
	while (sample != attackSamples)
	{
		envelope[sample++] = (1.0f + cosf(juce::MathConstants<float>::pi + (juce::MathConstants<float>::pi * ((float)sample / attackSamples) * (grainAmp / 2.0f) ) ) );
	}
	while (sample != (attackSamples + sustainSamples))
	{
		envelope[sample++] = grainAmp;
	}
	while (sample < envelope.size())
	{
		//envelope[sample++] = (1.0 + cosf(juce::MathConstants<float>::pi * ((sample - attackSamples - sustainSamples) / releaseSmaples)) * (grainAmp / 2.0f));
		envelope[sample++] = (1.0 + cosf(juce::MathConstants<float>::pi * ((sample - attackSamples - sustainSamples) / (float)releaseSmaples)) * (grainAmp / 2.0f));
	}
}
#endif

void GrainPool::envelopeSetup()
{
	env.resize(BIGGEST_WINDOW * sampleRate);
	Envelopes::calculateEnvelopeParabolicArray(env);
}
void GrainPool::setup(juce::AudioSampleBuffer *localBuffer, int bufferSize)
{
	//TODO: sample rate conversion.
	bufSize = bufferSize;
	buffer = localBuffer;
	envelopeFunction = Envelopes::calculateEnvelopeParabolic;
	for (GrainQueue& queue : grains) 
	{
		queue.reset();
	}
	//envelope.resize(grainSize);

	//calculateEnvelopeParabolic();
	//calculateEnvelopeRaisedCosine();
	//calculateEnvelopeLinear();

}
float GrainPool::sumGrains(int channel)
{
	float sum = 0.0f;
	auto& grainBuffer = grains[channel];
	const float* channelRead = buffer->getReadPointer(channel);

	//TODO: Currently stupid summing. need better technique to go over the queue without blowing through 300 "maybe" active grains.
	//TODO: Grain mixer, equal weights, most to last created, most to first created etc.
	//Basic equal weight mixing.
	float grainWeight = 1.0f / (float)grainBuffer.activeGrains;
	int envelopeMax = env.size() - 1;
	for (int i = 0; i < MAX_GRAINS; ++i)
	{
		Grain& grain = grainBuffer.data[i];
		if (grain.properties & GrainProperties::active)
		{
			if (grain.pos != grain.maxSample)
			{
				const int increment = (grain.properties & GrainProperties::reverse) ? -1 : 1;
				int envelopeIdx =  (int)roundf( (abs(grain.maxSample - grain.pos)/(float)grain.size) * (envelopeMax) );
				if (envelopeIdx == envelopeMax) --envelopeIdx;
				sum += channelRead[grain.pos] * env[envelopeIdx] * grainWeight;
				grain.pos += increment;
				//sum += channelRead[grain.pos] * env[envelopeIdx];
			}
			else
			{
				grainBuffer.deactivate(i);
				/*
				grain.properties = GrainProperties::none;
				--grainBuffer.activeGrains;
				*/
			}
		}
	}
	return sum;
}
//TODO: Implement random
//TODO: Implement no loopback option(no recreating grain regions which are already playing)
void GrainPool::activateGrain()
{
	const int grainWindowInSamples = sampleRate * grainWindow->get();

	const int userMax = rangeEnd->get();
	const int maxBound = userMax > bufSize ? bufSize : userMax;

	const int userMin = rangeStart->get();
	//TODO: Minimum bound meets maximum bound, decide behaviour.
	const int minBound = userMin >= maxBound ? maxBound - 2 : userMin;

	if (startingSample >= maxBound) startingSample = minBound;
	else if (startingSample < minBound) startingSample = minBound;

	int endingSample = startingSample + grainWindowInSamples;
	if (endingSample >= maxBound) endingSample = maxBound - 1;
	for (auto& channel : grains)
	{
		//TODO: Loop around?
		//TODO: Reverse
		//If reverse:
		//channel.insert(endingSample, startingSample, GrainProperties::active | GrainProperties::reverse);
		channel.insert(startingSample, endingSample, GrainProperties::active);
	}
	//TODO:Non-linear jumps
	startingSample += grainWindowInSamples;

	//TODO:Need to change behaviour such that bound checking does not fail if parameters were changed during the function call.
	if (startingSample >= rangeEnd->get()) startingSample = minBound;
}
#if 0
void GrainPool::activateGrain()
{
	//TODO: CLEAN THIS UP!
	int newStart = rangeStart->get();
	int newEnd = rangeEnd->get();

	if (start != newStart) 
	{ 
		start = newStart >= bufSize ? bufSize : newStart; 
		grainIdx = (int)floorf( (start / (float)bufSize) * (grains[0].size()-1));
		startingGrain = grainIdx;
	}
	if (end != newEnd) 
	{ 
		end = newEnd >= bufSize ? bufSize : newEnd; 
		endingGrain = (int)floorf((end / (float)bufSize) * grains[0].size());
		if (grainIdx >= endingGrain) grainIdx = startingGrain;
	}
	jassert(start < end);

	for (auto& grainBuf : grains)
	{
		grainBuf[grainIdx].active = true;
	}
	grainIdx++;
	if (grainIdx == endingGrain) grainIdx = startingGrain;
}
void GrainPool::activateGrain(int index, int channel)
{
	if (grains[channel][index].active)
	{
		for (auto& grain : grains[channel])
			if (!grain.active)
			{
				grain.active = true;
				break;
			}
	}
	else 
	{
		grains[channel][index].active = true;
	}
}
#endif

//TODO: Scheduling according to number of grains in a given time.(#grain/sec)
//TODO: Quanta's definition is pretty good, Number of new grains created per second.
GrainScheduler::GrainScheduler() : rand(1)
{
}
void GrainScheduler::setup(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
	ParameterMaker maker(layout);
	interonsetPtr = maker.addNamed<juce::AudioParameterInt>("Interonset", 1, 20000, 200);

	scheduleRandom = maker.addNamed<juce::AudioParameterBool>("Random Scheduling", false);

	interonset = interonsetPtr->get();
}
float GrainScheduler::synthesize(GrainPool& pool, int channel)
{

	if (--nextOnsets[channel] == 0)
	{
		/*
		if (scheduleRandom->get())
			pool.activateGrain(rand.nextInt(pool.grains[0].size()), channel);
		
		else
			pool.activateGrain();
			*/
		pool.activateGrain();
		nextOnsets[channel] += interonset;
	}
	//TODO: proper parameter modulation inside loop.
#if 0
	if (lastMinInteronset != minInteronset->get() || lastMaxInteronset != maxInteronset->get())
	{
		lastMinInteronset = minInteronset->get(); lastMaxInteronset = maxInteronset->get();
		//interonset = lastMinInteronset + (int)(rand.nextFloat() * (lastMaxInteronset - lastMinInteronset));
		interonset = lastMinInteronset + lastMaxInteronset;
	}
#else
	if (interonset != interonsetPtr->get()) interonset = interonsetPtr->get();
#endif
	return pool.sumGrains(channel);
}
