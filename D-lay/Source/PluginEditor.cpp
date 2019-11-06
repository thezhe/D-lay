/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DlayAudioProcessorEditor::DlayAudioProcessorEditor (DlayAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

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

	mRate.addListener(this);
	mFeedback.addListener(this);
	mWet.addListener(this);

	addAndMakeVisible(&mRate);
	addAndMakeVisible(&mFeedback);
	addAndMakeVisible(&mWet);
}

void DlayAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
	processor.delayTime = mRate.getValue();
	processor.mFeedback = mFeedback.getValue();
	processor.mWet = mWet.getValue();
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
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
}

void DlayAudioProcessorEditor::resized()
{
	mRate.setBounds(40, 30, getWidth() - 60, 20);
	mFeedback.setBounds(40, 130, getWidth() - 60, 20);
	mWet.setBounds(40, 230, getWidth() - 60, 20);
}
