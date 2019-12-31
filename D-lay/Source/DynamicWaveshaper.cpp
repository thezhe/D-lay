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

	//keep track of max value in chunks sampled at 200Hz
	mChunkSize = spec.sampleRate / 100.0f;

	//envelope processing arrays
	mChunkMaxIn = std::make_unique<float[]>(mNumChannels);
	mChunkCounter = std::make_unique<int[]>(mNumChannels);
	mSideChainThreshIn = std::make_unique<float[]>(mNumChannels);
	mLastSample = std::make_unique<float[]>(mNumChannels); //set up zeroed y_n-1 elements for each channel to prepare for IIR envelope smoothing

	//initialize waveshapers and sidechain input
	mTargetWaveshaper.initialise([](float x) {return tanh(20*x); }, -1.0f, 1.0f, 512);

	//initialize parameters
	setAttack(50.0f);
	setRelease(100.0f);
}

void DynamicWaveshaper::setThreshold(float dbThreshold) noexcept 
{
	jassert(dbThreshold <= 0.0f);
	mThreshold = Decibels::decibelsToGain(dbThreshold);
}

void DynamicWaveshaper::setAttack(float msAttack) noexcept 
{
	jassert(msAttack >= 0);
	mAttackCoeff = exp(-1000 / (msAttack * mSampleRate));
}

void DynamicWaveshaper::setRelease(float msRelease) noexcept 
{
	jassert(msRelease >= 0);
	mReleaseCoeff = exp(-1000 / (msRelease * mSampleRate));
}
