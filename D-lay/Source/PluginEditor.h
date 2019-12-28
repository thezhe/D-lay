/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
class DlayAudioProcessorEditor  : public AudioProcessorEditor
{
public:

	typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
	typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

	DlayAudioProcessorEditor(DlayAudioProcessor& p, AudioProcessorValueTreeState& apvts);
    ~DlayAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
	//processor reference
    DlayAudioProcessor& processor;

	//apvts from processor
	AudioProcessorValueTreeState& valueTreeState;
	
	//UI element bounds
	enum
	{
		margin = 10,
		sectionLabelWidth = 180,
		sectionLabelHeight = 40,
		labelWidth = 90,
		labelHeight = 20,
		sliderX = 100,
		sliderHeight = 20,
		buttonWidth = 30
	};


	//labels
	Label mDelay, mAAfilter, mDynamicWaveshaper;
	Label mRateLabel, mFeedbackLabel, mWetLabel, mCutoffLabel, mResonanceLabel, mThresholdLabel, mAttackLabel, mReleaseLabel, mAnalogLabel;

	//UI parameters
	Slider mRate, mFeedback, mWet, mCutoff, mResonance, mThreshold, mAttack, mRelease;
	ToggleButton mAnalog;

	//parameter attachments
	std::unique_ptr<SliderAttachment> mRateAttachment, mFeedbackAttachment, mWetAttachment, mCutoffAttachment, mResonanceAttachment, mThresholdAttachment, mAttackAttachment, mReleaseAttachment;
	std::unique_ptr<ButtonAttachment> mAnalogAttachment;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DlayAudioProcessorEditor)
};

//TODO have proper slider skews and drag