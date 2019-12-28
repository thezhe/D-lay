/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#include "DynamicWaveshaper.h"

void DynamicWaveshaper::prepare(dsp::ProcessSpec spec)
{
	//get environment variables
	mBlockSize = spec.maximumBlockSize;
	mNumChannels = spec.numChannels;
	mSampleRate = spec.sampleRate;
	//keep track of max value in chunks sampled at 100Hz
	mChunkSize = spec.sampleRate / 100.0f;
	mChunkMaxIn = std::make_unique<float[]>(mNumChannels);
	mChunkCounter = std::make_unique<int[]>(mNumChannels);

	mSideChain.setSize(mNumChannels, mBlockSize);
	//set up zeroed y[n-1] array elements each channel to prepare for IIR envelope smoothing
	mLastSample = std::make_unique<float[]>(mNumChannels);   

	mSideChainThreshIn = std::make_unique<float[]>(mNumChannels);

	for (int channel = 0; channel < mNumChannels; ++channel) { mLastSample[channel] = 0; }
	//initialize waveshapers
	mTargetWaveshaper.initialise([](float x) {return tanh(20*x); }, -1.0f, 1.0f, 512);
	//initialize parameters
	setAttack(50.0f);
	setRelease(100.0f);
}

void DynamicWaveshaper::setThreshold(float threshold) noexcept 
{
	mThreshold = Decibels::decibelsToGain(threshold);
}

void DynamicWaveshaper::setAttack(float attack) noexcept 
{
	mAttackCoeff = exp(-1000 / (attack * mSampleRate));
}

void DynamicWaveshaper::setRelease(float release) noexcept 
{
	mReleaseCoeff = exp(-1000 / (release * mSampleRate));
}
