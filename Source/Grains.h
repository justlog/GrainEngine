#pragma once
#define SMALLEST_WINDOW 0.001f
#define BIGGEST_WINDOW 0.3f
#define MAX_GRAINS 2000
struct ParameterMaker
{
	explicit ParameterMaker(juce::AudioProcessorValueTreeState::ParameterLayout& layoutToUse) : layout(layoutToUse)
	{
	}
	~ParameterMaker() { layout.add(params.begin(), params.end()); }

	template <typename T, typename paramName, typename... Args>
	T* addNamed(paramName& name, Args&&... args)
	{
		auto newParam = new T(name, name, std::forward<Args>(args)...);
		params.emplace_back(newParam);

		return newParam;
	}
	juce::AudioProcessorValueTreeState::ParameterLayout& layout;
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
};
enum GrainProperties
{
	none = 0,
	active = 1,
	reverse = 2
};
struct Grain
{
	int pos;
	int maxSample;
	int size;
	uint8_t properties;
};
struct GrainQueue
{
	GrainQueue();
	void insert(int position, int endingSample, uint8_t settings);
	void reset();
	void deactivate(int index);

	std::array<Grain, MAX_GRAINS> data;
	int activeGrains;
	int lastActive;
};
struct GrainPool
{
	int grainIdx = 0;
	float lastWindow;
	int startingSample = 0;
	int start = 0;
	int end;

	int bufSize;
	int startingGrain = 0;
	int endingGrain;

	juce::AudioParameterFloat* grainWindow;
	juce::AudioParameterInt* rangeStart;
	juce::AudioParameterInt* rangeEnd;

	double sampleRate;

	juce::AudioSampleBuffer* buffer;
	std::vector<GrainQueue> grains;
	std::vector<float> env;

	float (*envelopeFunction)(int, int) = nullptr;

	float sumGrains(int channel);
	void envelopeSetup();
	void setup(juce::AudioSampleBuffer *buffer, int bufferSize);
	void setParamters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);
	void activateGrain();
	void calculateEnvelopeLinear();
	void calculateEnvelopeParabolic();
	void calculateEnvelopeRaisedCosine();
};
struct GrainScheduler
{
	//TODO: CARE! SHARING UNIQUE_PTR MIGHT CAUSE SOME PROBLEMS ON SHUTDOWN!
	juce::Random rand;

	juce::AudioParameterBool* scheduleRandom;
	juce::AudioParameterInt* interonsetPtr;

	int lastMinInteronset;
	int lastMaxInteronset;
	int interonset;
	std::vector<int> nextOnsets;

	GrainScheduler();
	void setup(juce::AudioProcessorValueTreeState::ParameterLayout& layout);
	float synthesize(GrainPool& pool, int channel);
};
