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
}

void DelayLine::setRate(float msRate) noexcept
{
	mRate = static_cast<int>((msRate / 1000) * mSampleRate);
}

void DelayLine::setFeedback(float dbFeedback) noexcept
{
	mFeedback = Decibels::decibelsToGain(dbFeedback);
}

void DelayLine::setWet(int percentWet) noexcept
{
	mWet = static_cast<float> (percentWet) / 100;
}

void DelayLine::clearDelayBuffer() noexcept 
{
	mDelayBuffer.clear(0, mDelayBufferLength);
}