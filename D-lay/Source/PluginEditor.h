#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
class DlayAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    DlayAudioProcessorEditor (DlayAudioProcessor&);
    ~DlayAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    DlayAudioProcessor& processor;
	
	//user parameters
	Slider mRate, mFeedback, mWet, mLPFcutoff, mLPFresonance, mThreshold, mAttack, mRelease;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessorEditor)
};

//TODO make user parameters linear smoothed
//TODO dB mWet
//TODO add sliders for mAAfilter