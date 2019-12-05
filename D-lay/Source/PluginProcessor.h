#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//A "dynamic" waveshaper that loosely follows JUCE's dsp module format, interpolates between dry signal and waveshaped signal based on envelope response
class DynamicWaveshaper 
{
public:
	DynamicWaveshaper(dsp::ProcessSpec spec)
		:mBlockSize(spec.maximumBlockSize),
		mNumChannels(spec.numChannels),
		mSampleRate(spec.sampleRate),
		mChunkSize (128),
		mNumChunks (mBlockSize/mChunkSize)
	{
		mTargetWaveshaper.initialise([](float x) {return std::tanh(25*x); }, -1.0f, 1.0f, 512);
		mSideChain.setSize(mNumChannels, mBlockSize);
		mEnvSmoother.setCutoffFrequencyHz(150.0f);
		mEnvSmoother.setMode(dsp::LadderFilter<float>::Mode::LPF12);
		mEnvSmoother.prepare(spec);
	}
	template <typename ProcessContext>
	forcedinline void process(const ProcessContext& context) noexcept
	{
		const auto& inputBlock = context.getInputBlock();
		auto& outputBlock = context.getOutputBlock();
		if (context.isBypassed)
		{
			if (context.usesSeparateInputAndOutputBlocks())
				outputBlock.copyFrom(inputBlock);
		}
		else
		{
			//get max in chunks and smooth
			for (int channel = 0; channel < mNumChannels; ++channel) {
				int startChunk = 0;
				for (int chunk = 1; chunk < mNumChunks; ++chunk) {
					float maxInChunk = 0;
					for (int c = startChunk; c < startChunk + mChunkSize; ++c) {
						if (abs(inputBlock.getChannelPointer(channel)[c]) > maxInChunk)
							maxInChunk = abs(inputBlock.getChannelPointer(channel)[c]);
					}
					for (int i = startChunk; i < startChunk + mChunkSize; ++i) {
						mSideChain.setSample(channel, i, maxInChunk);
					}
					startChunk += mChunkSize;
				}
			}
			dsp::AudioBlock<float> block(mSideChain);
			dsp::ProcessContextReplacing<float> context(block);
			mEnvSmoother.process(context);
			//convert to side chain signal
			/*
			
			
			TODO
			
			*/
			//apply amount of waveshaping proportional to sidechain signal
			for (int channel = 0; channel < mNumChannels; ++channel) {
				for (int i = 0; i < mBlockSize; ++i) {
					
					outputBlock.getChannelPointer(channel)[i] = std::lerp(
							inputBlock.getChannelPointer(channel)[i],
							mTargetWaveshaper.processSampleUnchecked(inputBlock.getChannelPointer(channel)[i]),
							mSideChain.getSample(channel, i)
						);
				}
			}
		}
	}
	float mThreshold;
private:
	//const enviornment variables
	const uint32 mBlockSize, mNumChannels;
	const double mSampleRate;
	const int mChunkSize, mNumChunks;
	//LPF for envelope smoothing with minimal phase
	dsp::LadderFilter<float> mEnvSmoother;
	//dynamic variables
	dsp::LookupTableTransform<float> mTargetWaveshaper;
	AudioBuffer<float> mSideChain;
};


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
	//DynamicWaveshaper
	DynamicWaveshaper* mDynamicWaveshaper;
	//ResonantLP to simulate anti-aliasing filter and reconstruction filter of BBD delays
	dsp::LadderFilter<float> mAAfilter;
private:
	//environment variables and member buffers
	bool mLock = true;
	int mTotalNumInputChannels;
	int	mTotalNumOutputChannels;

	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
