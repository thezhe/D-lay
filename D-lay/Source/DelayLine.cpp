/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#include "DelayLine.h"

void DelayLine::prepare(const dsp::ProcessSpec spec) 
{
	//save spec
	mBlockSize = static_cast<int>(spec.maximumBlockSize);
	mNumChannels = static_cast<int>(spec.numChannels);
	mSampleRate = static_cast<int>(spec.sampleRate);
	//initialize 1 second mDelayBuffer with some extra overhead for safety
	mDelayBufferLength = (mSampleRate + mBlockSize) - (mSampleRate % mBlockSize); //make sure mDelayBuffer is aligned to mBlockSize
	mDelayBuffer.setSize(mNumChannels, mDelayBufferLength);
	mDelayBufferBlock = dsp::AudioBlock<float>(mDelayBuffer);
	//initialize smoothers
	initSmoothers(0.01);
}

void DelayLine::setRate(float msRate) noexcept
{
	mRate = (msRate / 1000.0f) * static_cast<float>(mSampleRate);
}

void DelayLine::setFeedback(float dbFeedback) noexcept
{
	mFeedbackSmoothed.setTargetValue(Decibels::decibelsToGain(dbFeedback));
}

void DelayLine::setWet(int percentWet) noexcept
{
	mWetSmoothed.setTargetValue (static_cast<float> (percentWet) / 100.0f);
}

void DelayLine::clearDelayBuffer() noexcept 
{
	mDelayBuffer.clear(0, mDelayBufferLength);
}

void DelayLine::initSmoothers(double rampTime) 
{
	mFeedbackSmoothed.reset(mSampleRate, rampTime);
	mFeedbackSmoothed.setCurrentAndTargetValue(mFeedback);
	mWetSmoothed.reset(mSampleRate, rampTime);
	mWetSmoothed.setCurrentAndTargetValue(mWet);
}