#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DlayAudioProcessorEditor::DlayAudioProcessorEditor (DlayAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
	//setup GUI
    setSize (400, 800);

	mRate.setSliderStyle(Slider::LinearBar);
	mRate.setRange(0, 2000);
	mRate.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mRate.setPopupDisplayEnabled(true, false, this);
	mRate.setTextValueSuffix("Rate");
	mRate.setValue(500);

	mFeedback.setSliderStyle(Slider::LinearBar);
	mFeedback.setRange(0, 1);
	mFeedback.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mFeedback.setPopupDisplayEnabled(true, false, this);
	mFeedback.setTextValueSuffix("Feedback");
	mFeedback.setValue(0.8);

	mWet.setSliderStyle(Slider::LinearBar);
	mWet.setRange(0, 1);
	mWet.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mWet.setPopupDisplayEnabled(true, false, this);
	mWet.setTextValueSuffix("Wet");
	mWet.setValue(0.8);

	mLPFcutoff.setSliderStyle(Slider::LinearBar);
	mLPFcutoff.setRange(1000, 3000);
	mLPFcutoff.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mLPFcutoff.setPopupDisplayEnabled(true, false, this);
	mLPFcutoff.setTextValueSuffix("LPF Cutoff");
	mLPFcutoff.setValue(2500);

	mLPFresonance.setSliderStyle(Slider::LinearBar);
	mLPFresonance.setRange(0, 1);
	mLPFresonance.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mLPFresonance.setPopupDisplayEnabled(true, false, this);
	mLPFresonance.setTextValueSuffix("LPF Resonance");
	mLPFresonance.setValue(0.3);

	mThreshold.setSliderStyle(Slider::LinearBar);
	mThreshold.setRange(-100, 0);
	mThreshold.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mThreshold.setPopupDisplayEnabled(true, false, this);
	mThreshold.setTextValueSuffix("Threshold");
	mThreshold.setValue(-10);

	mAttack.setSliderStyle(Slider::LinearBar);
	mAttack.setRange(0, 1000);
	mAttack.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mAttack.setPopupDisplayEnabled(true, false, this);
	mAttack.setTextValueSuffix("Attack");
	mAttack.setValue(100);

	mRelease.setSliderStyle(Slider::LinearBar);
	mRelease.setRange(0, 1000);
	mRelease.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
	mRelease.setPopupDisplayEnabled(true, false, this);
	mRelease.setTextValueSuffix("Release");
	mRelease.setValue(200);
	
	//change processing parameters via lambdas
	mRate.onValueChange = [this] { 
		processor.mEchoProcessor->mRate = mRate.getValue();
		processor.mEchoProcessor->clearDelayBuffer();
	};
	mFeedback.onValueChange = [this] { processor.mEchoProcessor->mFeedback = mFeedback.getValue(); };
	mWet.onValueChange = [this] { processor.mEchoProcessor->mWet = mWet.getValue(); };
	mLPFcutoff.onValueChange = [this] {processor.mAAfilter.setCutoffFrequencyHz(mLPFcutoff.getValue()); };
	mLPFresonance.onValueChange = [this] {processor.mAAfilter.setResonance(mLPFresonance.getValue()); };
	mThreshold.onValueChange = [this] {processor.mDynamicWaveshaper->setThreshold(mThreshold.getValue()); };
	mAttack.onValueChange = [this] {processor.mDynamicWaveshaper->setAttack(mAttack.getValue()); };
	mRelease.onValueChange = [this] {processor.mDynamicWaveshaper->setRelease(mRelease.getValue()); };
	//make visible
	addAndMakeVisible(&mRate);
	addAndMakeVisible(&mFeedback);
	addAndMakeVisible(&mWet);
	addAndMakeVisible(&mLPFcutoff);
	addAndMakeVisible(&mLPFresonance);
	addAndMakeVisible(&mThreshold);
	addAndMakeVisible(&mAttack);
	addAndMakeVisible(&mRelease);
}


DlayAudioProcessorEditor::~DlayAudioProcessorEditor()
{
}

//==============================================================================
void DlayAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
}

void DlayAudioProcessorEditor::resized()
{
	//setup slider bounds
	mRate.setBounds(40, 30, getWidth() - 60, 20);
	mFeedback.setBounds(40, 130, getWidth() - 60, 20);
	mWet.setBounds(40, 230, getWidth() - 60, 20);
	mLPFcutoff.setBounds(40, 330, getWidth() - 60, 20);
	mLPFresonance.setBounds(40, 430, getWidth() - 60, 20);
	mThreshold.setBounds(40, 530, getWidth() - 60, 20);
	mAttack.setBounds(40, 630, getWidth() - 60, 20);
	mRelease.setBounds(40, 730, getWidth() - 60, 20);
}
