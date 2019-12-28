/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "DelayLine.h"
#include "DynamicWaveshaper.h"


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
	DelayLine mEchoProcessor;

	//Resonant low pass: simulates anti-aliasing filter and reconstruction filter of BBD delays
	dsp::LadderFilter<float> mAAfilter;

	//DynamicWaveshaper: simulates BBD internal distortion
	DynamicWaveshaper mDynamicWaveshaper;

	//set mAAfilter and mDynamicWaveshaper On/Off
	void setAnalog(bool onOffAnalog) noexcept;

private:
	//enable/disable mAAfilter and mDynamicWaveshaper flag
	bool mAnalog = true;

	//UI-synced parameters
	AudioProcessorValueTreeState parameters;
	
	//environment variables
	int mTotalNumInputChannels, mTotalNumOutputChannels;
	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessor)
};
