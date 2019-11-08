#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class DynamicWaveshaper
{
public:
	DynamicWaveshaper();
	void prepare(dsp::ProcessSpec spec, const AudioBuffer<float>& dynamicWavetable) {
		mDynamicWavetable = dynamicWavetable;
		mBlockSize = spec.maximumBlockSize;
		mNumChannels = spec.numChannels;
		mHalfTableSize = mDynamicWavetable.getNumSamples() / 2;
	}
	forcedinline void process(AudioBuffer<float>& buffer) {
		for (int channel = 0; channel < mNumChannels; ++channel) {
			float* buf = buffer.getWritePointer(channel);
			const float* test = mDynamicWavetable.getReadPointer(0);
			for (int i = 0; i < mBlockSize; ++i) {
				float realIndex = (buf[i] * (mHalfTableSize + 0.5)) + mHalfTableSize - 0.5;
				auto index0 = static_cast<uint32> (realIndex);
				auto index1 = index0 + 1;
				auto argT = realIndex - static_cast<float> (index0);
				//buf[i] *= std::lerp(test[index0], test[index1], argT);
			}
		}
	}
private:
	AudioBuffer<float>& mDynamicWavetable;
	uint32 mBlockSize, mNumChannels, mHalfTableSize;
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
	DynamicWaveshaper* ChebyshevWaveshaper;
	//processBlock
	int mWritePosition{ 0 };
	void getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, int bufferLength,
		int delayBufferLength, const float* delayBufferData, const float* bufferData);
	void fillDelayBuffer(int channel, int bufferLength, int delayBufferLength,
		const float* bufferData);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
