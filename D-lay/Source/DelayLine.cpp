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
	mBlockSize = spec.maximumBlockSize;
	mNumChannels = spec.numChannels;
	mSampleRate = spec.sampleRate;
	//initialize 1 second mDelayBuffer with some extra overhead for safety
	mDelayBufferLength = (mSampleRate + mBlockSize) - (mSampleRate % mBlockSize); //make sure mDelayBuffer is aligned to mBlockSize
	mDelayBuffer.setSize(mNumChannels, mDelayBufferLength);
	mDelayBufferBlock = dsp::AudioBlock<float>(mDelayBuffer);
}

void DelayLine::setRate(float msRate) noexcept
{
	jassert(msRate >= 0.0f && msRate <= 1000.0f);
	mRate = (msRate / 1000.0f) * static_cast<float>(mSampleRate);
}

void DelayLine::setFeedback(float dbFeedback) noexcept
{
	jassert(dbFeedback <= 0.0f);
	mFeedback = Decibels::decibelsToGain(dbFeedback);
}

void DelayLine::setWet(int percentWet) noexcept
{
	jassert(percentWet >= 0 && percentWet <= 100);
	mWet = static_cast<float> (percentWet) / 100.0f;
}
