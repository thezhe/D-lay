#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//A "2D" waveshaper that loosely follows JUCE's dsp module format, does not account for intermodulation distortion (ie form slew rate limits) or inharmonic overtones, transient properties and harmonic content only as accurate as data provided
class DynamicWaveshaper 
{
public:
	DynamicWaveshaper(dsp::ProcessSpec spec)
		:mBlockSize(spec.maximumBlockSize),
		mNumChannels(spec.numChannels) {};
	void addToTable(dsp::LookupTableTransform<float>* table) {
		mDynamicWavetable.add(table);
	}
	template <typename ProcessContext>
	forcedinline void process(const ProcessContext& context) const noexcept
	{
		if (context.isBypassed)
		{
			if (context.usesSeparateInputAndOutputBlocks())
				context.getOutputBlock().copyFrom(context.getInputBlock());
		}
		else
		{
			for (int channel = 0; channel < mNumChannels; ++channel) {
				for (int i = 0; i < mBlockSize; ++i) {
					if (context.getInputBlock().getSample(channel,0)>0.1)
						context.getOutputBlock().setSample(channel, i, mDynamicWavetable.getUnchecked(0)->processSampleUnchecked(context.getInputBlock().getSample(channel, i)));
				}
			}
		}
	}

private:
	OwnedArray<dsp::LookupTableTransform<float>> mDynamicWavetable;
	const uint32 mBlockSize, mNumChannels;
};

class DlayAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    DlayAudioProcessor();
    ~DlayAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	//user parameters
	int mRate = 500;
	float mFeedback = 0.5;
	float mWet = 0.5;
	//clear mDelayBuffer on Slider value change
	forcedinline void clearDelayBuffer() {
		mDelayBuffer.clear(0, mDelayBufferLength);
	}
private:
	//environment variables and member buffers
	bool mLock = true;
	int mTotalNumInputChannels;
	int	mTotalNumOutputChannels;
	int mBufferLength;
	int mDelayBufferLength;
	int mSampleRate{ 192000 };
	AudioBuffer<float> mDelayBuffer;
	AudioBuffer<float> mSendToDelayBuffer;
	//ResonantLP
	dsp::LadderFilter<float> LP;
	//Waveshaper
	DynamicWaveshaper* mChebyshevWaveshaper;
	//processBlock
	int mWritePosition{ 0 };
	void getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, int bufferLength,
		int delayBufferLength, const float* delayBufferData, const float* bufferData);
	void fillDelayBuffer(int channel, int bufferLength, int delayBufferLength,
		const float* bufferData);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
