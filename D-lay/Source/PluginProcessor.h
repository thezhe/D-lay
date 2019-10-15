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

private:
	//prepareToPlay
	int totalNumInputChannels;
	int	totalNumOutputChannels;
	int bufferLength;
	int delayBufferLength;
	int mSampleRate{ 192000 };
	AudioBuffer<float> mDelayBuffer;
	//processBlock
	void getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, int bufferLength,
		int delayBufferLength, const float* delayBufferData);
	void limitSlew(float* buffer, int channel);
	void fillDelayBuffer(int channel, int bufferLength, int delayBufferLength,
		const float* bufferData);
	int mWritePosition{ 0 };
	//getFromDelayBuffer
	int delayTime = 500;
	float mFeedback = 0.5;
	//limitSlew
	float slewRise = 10000000;
	float slewFall = 15000000;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
