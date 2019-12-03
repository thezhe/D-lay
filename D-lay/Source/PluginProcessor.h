#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//A "2D" waveshaper that loosely follows JUCE's dsp module format, does not account for intermodulation distortion (ie form slew rate limits) or inharmonic overtones, transient properties and harmonic content only as accurate as data provided
/*class DynamicWaveshaper 
{
public:
	DynamicWaveshaper(dsp::ProcessSpec spec)
		:mBlockSize(spec.maximumBlockSize),
		mNumChannels(spec.numChannels) {}
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
};*/

//Approximately 2 second delay line with write (fill) and read (get)
class DelayLine {
public:
	//constructor that also serves as preparation for processing
	DelayLine(dsp::ProcessSpec spec)
	:mBlockSize(spec.maximumBlockSize),
	mNumChannels(spec.numChannels),
	mSampleRate (spec.sampleRate),
	mDelayBufferLength ((2 * (mSampleRate + mBlockSize)) - (static_cast<int>(2 * (mSampleRate + mBlockSize))%mBlockSize)), //make sure mDelayBuffer is aligned to mBlockSize
	mWritePosition(0)
	{
		mDelayBuffer.setSize(mNumChannels, mDelayBufferLength);
		mDelayBufferBlock = dsp::AudioBlock<float>(mDelayBuffer);
	}
	//copy buffer to mWriteBlock (pointer to a block in mDelayBuffer starting at mWritePosition)
	forcedinline void fillFromDelayBuffer(AudioBuffer<float>& buffer) {
		 mWriteBlock = mDelayBufferBlock.getSubBlock(mWritePosition, mBlockSize);
		 mWriteBlock.copyFrom(buffer, 0, 0, mBlockSize);
	}
	//add delayed signal to mWriteBlock and buffer 
	forcedinline void getFromDelayBuffer(AudioBuffer<float>& buffer)
	{
		//set mReadPosition
		mReadPosition = static_cast<int> (mDelayBufferLength + mWritePosition - (mSampleRate * mRate / 1000))
			% mDelayBufferLength;
		for (int channel = 0; channel < mNumChannels; ++channel) {
			//get read data
			const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
			//normal read case
			if (mDelayBufferLength > mBlockSize + mReadPosition)
			{
				mDelayBuffer.addFrom(channel, mWritePosition, delayBufferData + mReadPosition, mBlockSize, mFeedback);
				buffer.addFrom(channel, 0, delayBufferData + mReadPosition, mBlockSize, mWet);
			}
			//circular buffer wrap-around
			else
			{
				const int bufferRemaining = mDelayBufferLength - mReadPosition;
				mDelayBuffer.addFrom(channel, mWritePosition, delayBufferData + mReadPosition, bufferRemaining, mFeedback);
				mDelayBuffer.addFrom(channel, mWritePosition + bufferRemaining, delayBufferData, mBlockSize - bufferRemaining, mFeedback);
				buffer.addFrom(channel, 0, delayBufferData + mReadPosition, bufferRemaining, mWet);
				buffer.addFrom(channel, bufferRemaining, delayBufferData, mBlockSize - bufferRemaining, mWet);
			}
		}
		//update mWritePosition
		mWritePosition += mBlockSize;
		mWritePosition %= mDelayBufferLength;
	}
	//user parameters
	int mRate = 500;
	float mFeedback = 0.8;
	float mWet = 0.8;
	//clear mDelayBuffer on Slider value change
	forcedinline void clearDelayBuffer() {
		mDelayBuffer.clear(0, mDelayBufferLength);
	}
	//can be processed before a call to get and after a call to fill to simulate pre-delay line effects
	dsp::AudioBlock<float> mWriteBlock;
private:
	//const environment variables
	const double mSampleRate;
	const uint32 mBlockSize, mNumChannels;
	const int mDelayBufferLength;
	//dynamic variables
	int mWritePosition, mReadPosition;
	dsp::AudioBlock<float> mDelayBufferBlock;
	AudioBuffer<float> mDelayBuffer;
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
	//==============================================================================
	//DelayLine
	DelayLine* mEchoProcessor;
	//ResonantLP to simulate anti-aliasing filter and reconstruction filter of BBD delays
	dsp::LadderFilter<float> mAAfilter;
private:
	//environment variables and member buffers
	bool mLock = true;
	int mTotalNumInputChannels;
	int	mTotalNumOutputChannels;

	//Waveshaper
	//DynamicWaveshaper* mChebyshevWaveshaper;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
