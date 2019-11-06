#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

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
	int delayTime = 500;
	float mFeedback = 0.5;
	float mWet = 0.5;
private:
	//prepareToPlay
	int totalNumInputChannels;
	int	totalNumOutputChannels;
	int bufferLength;
	int delayBufferLength;
	int mSampleRate{ 192000 };
	AudioBuffer<float> mDelayBuffer;
	AudioBuffer<float> mSendToDelayBuffer;
	//processBlock
	bool isActive = false;
	void getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, int bufferLength,
		int delayBufferLength, const float* delayBufferData, const float* bufferData);
	void fillDelayBuffer(int channel, int bufferLength, int delayBufferLength,
		const float* bufferData);
	int mWritePosition{ 0 };
	//getFromDelayBuffer
	
	//ResonantLP
	dsp::LadderFilter<float> LP;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
