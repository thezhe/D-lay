/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DlayAudioProcessorEditor::DlayAudioProcessorEditor (DlayAudioProcessor& p, AudioProcessorValueTreeState& apvts)
    : AudioProcessorEditor (&p), processor (p), valueTreeState(apvts)
{
	//change UI appearance
	mDelay.setText("Delay", dontSendNotification);
	mDelay.setJustificationType(Justification::centred);
	mRateLabel.setText("Rate", dontSendNotification);
	mRate.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mRate.setTextValueSuffix("ms");
	mFeedbackLabel.setText("Feedback", dontSendNotification);
	mFeedback.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mFeedback.setTextValueSuffix("dB");
	mWetLabel.setText("Wet", dontSendNotification);
	mWet.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mWet.setTextValueSuffix("%");
	
	mAAfilter.setText("Anti-Aliasing Filter", dontSendNotification);
	mAAfilter.setJustificationType(Justification::centred);
	mCutoffLabel.setText("Cutoff", dontSendNotification);
	mCutoff.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mCutoff.setTextValueSuffix("Hz");
	mResonanceLabel.setText("Resonance", dontSendNotification);
	mResonance.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	
	mDynamicWaveshaper.setText("Dynamic Waveshaper", dontSendNotification);
	mDynamicWaveshaper.setJustificationType(Justification::centred);
	mThresholdLabel.setText("Threshold", dontSendNotification);
	mThreshold.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mThreshold.setTextValueSuffix("dB");
	mAttackLabel.setText("Attack", dontSendNotification);
	mAttack.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mAttack.setTextValueSuffix("ms");
	mReleaseLabel.setText("Release", dontSendNotification);
	mRelease.setTextBoxStyle(Slider::TextBoxRight, false, labelWidth, labelHeight);
	mRelease.setTextValueSuffix("ms");

	mAnalogLabel.setText("Analog", dontSendNotification);

	mTargetWaveshaperLabel.setText("Target Waveshaper", dontSendNotification);
	mTargetWaveshaper.setJustificationType(Justification::centred);
	mTargetWaveshaper.addItem("BBD", bbd);
	mTargetWaveshaper.addItem("Tube", chebyshev);
	mTargetWaveshaper.addItem("Smashed", smashed);

	//change processing parameters via lambdas
	mRate.onValueChange = [this] { processor.mEchoProcessor.setRate(mRate.getValue());};
	mFeedback.onValueChange = [this] { processor.mEchoProcessor.setFeedback (mFeedback.getValue()); };
	mWet.onValueChange = [this] { processor.mEchoProcessor.setWet( mWet.getValue()); };
	
	mCutoff.onValueChange = [this] {processor.mAAfilter.setCutoffFrequencyHz(mCutoff.getValue()); };
	mResonance.onValueChange = [this] {processor.mAAfilter.setResonance(mResonance.getValue()); };
	
	mThreshold.onValueChange = [this] {processor.mDynamicWaveshaper.setThreshold(mThreshold.getValue()); };
	mAttack.onValueChange = [this] {processor.mDynamicWaveshaper.setAttack(mAttack.getValue()); };
	mRelease.onValueChange = [this] {processor.mDynamicWaveshaper.setRelease(mRelease.getValue()); };

	mAnalog.onClick = [this] {processor.setAnalog(mAnalog.getToggleState()); };

	mTargetWaveshaper.onChange = [this] {processor.mDynamicWaveshaper.setTargetWaveshaper(mTargetWaveshaper.getSelectedId()); };

	//make visible
	addAndMakeVisible(mDelay);
	addAndMakeVisible(mRateLabel);
	addAndMakeVisible(mRate);
	addAndMakeVisible(mFeedbackLabel);
	addAndMakeVisible(mFeedback);
	addAndMakeVisible(mWetLabel);
	addAndMakeVisible(mWet);

	addAndMakeVisible(mAAfilter);
	addAndMakeVisible(mCutoffLabel);
	addAndMakeVisible(mCutoff);
	addAndMakeVisible(mResonanceLabel);
	addAndMakeVisible(mResonance);

	addAndMakeVisible(mDynamicWaveshaper);
	addAndMakeVisible(mThresholdLabel);
	addAndMakeVisible(mThreshold);
	addAndMakeVisible(mAttackLabel);
	addAndMakeVisible(mAttack);
	addAndMakeVisible(mReleaseLabel);
	addAndMakeVisible(mRelease);

	addAndMakeVisible(mAnalogLabel);
	addAndMakeVisible(mAnalog);

	addAndMakeVisible(mTargetWaveshaperLabel);
	addAndMakeVisible(mTargetWaveshaper);

	//attach after UI elements to ensure attahments are deleted first in editor's destructor 
	mRateAttachment = std::make_unique<SliderAttachment>(valueTreeState, "rate", mRate);
	mFeedbackAttachment = std::make_unique<SliderAttachment>(valueTreeState, "feedback", mFeedback);
	mWetAttachment = std::make_unique<SliderAttachment>(valueTreeState, "wet", mWet);

	mCutoffAttachment = std::make_unique<SliderAttachment>(valueTreeState, "cutoff", mCutoff);
	mResonanceAttachment = std::make_unique<SliderAttachment>(valueTreeState, "resonance", mResonance);
	
	mThresholdAttachment = std::make_unique<SliderAttachment>(valueTreeState, "threshold", mThreshold);
	mAttackAttachment = std::make_unique<SliderAttachment>(valueTreeState, "attack", mAttack);
	mReleaseAttachment = std::make_unique<SliderAttachment>(valueTreeState, "release", mRelease);

	mAnalogAttachment = std::make_unique<ButtonAttachment>(valueTreeState, "analog", mAnalog);

	mTargetWaveshaperAttachment = std::make_unique<ComboBoxAttachment>(valueTreeState, "targetWaveshaper", mTargetWaveshaper);

	//set Window
	setSize(600, 320);
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
	const int sectionLabelX = (getWidth() / 2) - (sectionLabelWidth/2);
	const int sliderWidth = getWidth() - sliderX - margin;

	//Delay section
	mDelay.setBounds(sectionLabelX, margin, sectionLabelWidth, sectionLabelHeight);
	mRateLabel.setBounds(margin, 50, labelWidth, labelHeight);
	mRate.setBounds(sliderX, 50, sliderWidth, sliderHeight);
	mFeedbackLabel.setBounds(margin, 70, labelWidth, labelHeight);
	mFeedback.setBounds(sliderX, 70, sliderWidth, sliderHeight);
	mWetLabel.setBounds(margin, 90, labelWidth, labelHeight);
	mWet.setBounds(sliderX, 90, sliderWidth, sliderHeight);

	//Anti Aliasing Filter section
	mAAfilter.setBounds(sectionLabelX, 110, sectionLabelWidth, sectionLabelHeight);
	mCutoffLabel.setBounds(margin, 150, labelWidth, labelHeight);
	mCutoff.setBounds(sliderX, 150, sliderWidth, sliderHeight);
	mResonanceLabel.setBounds(margin, 170, labelWidth, labelHeight);
	mResonance.setBounds(sliderX, 170, sliderWidth, sliderHeight);

	//Dynamic Waveshaper section
	mDynamicWaveshaper.setBounds(sectionLabelX, 190, sectionLabelWidth, sectionLabelHeight);
	mTargetWaveshaperLabel.setBounds(margin, 230, labelWidth + 30, labelHeight);
	mTargetWaveshaper.setBounds(sectionLabelX - 20, 230, sectionLabelWidth + 20, sliderHeight);
	mThresholdLabel.setBounds(margin, 250, labelWidth, labelHeight);
	mThreshold.setBounds(sliderX, 250, sliderWidth, sliderHeight);
	mAttackLabel.setBounds(margin, 270, labelWidth, labelHeight);
	mAttack.setBounds(sliderX, 270, sliderWidth, sliderHeight);
	mReleaseLabel.setBounds(margin, 290, labelWidth, labelHeight);
	mRelease.setBounds(sliderX, 290, sliderWidth, sliderHeight);

	//Analog On/Off
	mAnalogLabel.setBounds(getWidth() - margin - labelWidth - buttonWidth, 110, labelWidth, labelHeight);
	mAnalog.setBounds(getWidth() - margin - buttonWidth, 110, buttonWidth, buttonWidth);
}

//TODO make sliders lag and scale appropriately per parameter
//TODO real time graph of waveshaping transfer function
//TODO grey out AA filter and DynamicWaveshaper sections when Analog is off