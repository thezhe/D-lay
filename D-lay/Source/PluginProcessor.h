#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//Note: some methods are in header file to force inline
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
		//set up y[n-1] for the IIR filtering of envelope of each channel
		mLastSample = new float[mNumChannels];
		for (int channel = 0; channel < mNumChannels; ++channel) {
			mLastSample[channel] = 0;
		}
		mTargetWaveshaper.initialise([](float x) {return 0.2*std::tanh(25*x); }, -1.0f, 1.0f, 512);
		mSideChain.setSize(mNumChannels, mBlockSize);
		mEnvSmoother.setCutoffFrequencyHz(150.0f);
		mEnvSmoother.setMode(dsp::LadderFilter<float>::Mode::LPF12);
		mEnvSmoother.prepare(spec);
	}
	//take in threshold in dB and store in mThreshold as gain value
	void setThreshold(float threshold) noexcept;
	//take in attack time in ms and convert to IIR coefficients
	void setAttack(float attack) noexcept;
	//take in release time in ms and convert to IIR coefficients
	void setRelease(float release) noexcept;
	template <typename ProcessContext>
	void process(const ProcessContext& context) noexcept
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
			//convert to square wave using threshold and use the step response of a one pole low pass filter (IIR) to apply attack and release parameters
			for (int channel = 0; channel < mNumChannels; ++channel) {
				auto mSideChainData = mSideChain.getWritePointer(channel);
				for (int i = 0; i < mBlockSize; ++i) {
					if (mSideChainData[i] > mThreshold) { mSideChainData[i] = 1; }
					else { mSideChainData[i] = 0; }
					if (mSideChainData[i]>mLastSample[channel]){ 
						mSideChainData[i] = mAttackCoeff * mLastSample[channel] + mOneMinusAttackCoeff * mSideChainData[i];
					}
					else {
						mSideChainData[i] = mReleaseCoeff * mLastSample[channel];
					}
					mLastSample[channel] = mSideChainData[i];
				}
			}
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
	
private:
	//internal parameters modified through public methods
	float mThreshold;
	float mAttackCoeff;
	float mOneMinusAttackCoeff;
	float mReleaseCoeff;
	//const enviornment variables
	const uint32 mBlockSize, mNumChannels;
	const double mSampleRate;
	const int mChunkSize, mNumChunks;
	//LPF for envelope smoothing with minimal phase
	dsp::LadderFilter<float> mEnvSmoother;
	//dynamic variables
	dsp::LookupTableTransform<float> mTargetWaveshaper;
	AudioBuffer<float> mSideChain;
	float* mLastSample;
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
	int mRate;
	float mFeedback;
	float mWet;
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
