/*
  ==============================================================================
	Zhe Deng 2020
	thezhefromcenterville@gmail.com

	This file is part of D-lay which is released under the MIT license.
	See file LICENSE or go to https://github.com/thezhe/D-lay for full license details.
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

#if JUCE_USE_SIMD
	//prepare for channel interleaving
	mInterleaved = dsp::AudioBlock<dsp::SIMDRegister<float>>(interleavedBlockData, 1, mBlockSize);
	mZero = dsp::AudioBlock<float>(zeroData, dsp::SIMDRegister<float>::size(), mBlockSize);
	mZero.clear();
#else
	mChunkMaxIn = std::make_unique<float[]>(mNumChannels);
	mSideChainThreshIn = std::make_unique<float[]>(mNumChannels);
	mLastSample = std::make_unique<float[]>(mNumChannels); //set up zeroed y[n-1] array elements each channel to prepare for IIR envelope smoothing
#endif	

	//initialize waveshapers and side chain signal
	mTargetWaveshapers.ensureStorageAllocated(4);
	mTargetWaveshapers.add(std::make_unique<dsp::LookupTableTransform<float>>([](float x) {return x; }, -1.0f, 1.0f, 2)); //Linear
	mTargetWaveshapers.add (std::make_unique<dsp::LookupTableTransform<float>>([](float x) {return x - (pow(x,2)/8.0f) - (pow(x,3)/16.0f) + 0.125f; }, -1.0f, 1.0f, 512)); //BBD waveshaper approxmiation
	mTargetWaveshapers.add(std::make_unique<dsp::LookupTableTransform<float>>([this](float x) {return x + Decibels::decibelsToGain(-42.0f)*T_2(x) 
		+ Decibels::decibelsToGain(-68.0f) * T_3(x) + Decibels::decibelsToGain(-84.0f) * T_4(x); }, -1.0f, 1.0f, 512)); //Chebyshev Harmonic Matching to 6AU6A Pentode with -90dB noise floor, harmonics boosted 6dB
	mTargetWaveshapers.add(std::make_unique<dsp::LookupTableTransform<float>>([](float x) {return tanh(15 * x); }, -1.0f, 1.0f, 512)); //Smashed signal with boosted tanh
	mTargetWaveshapers.minimiseStorageOverheads();
	mSideChain.setSize(mNumChannels, mBlockSize);

	//initialize parameters
	setAttack(50.0f);
	setRelease(100.0f);
}

void DynamicWaveshaper::setTargetWaveshaper(int choice) noexcept
{
	jassert(choice >= 0 && choice <= 3);
	mTargetWaveshaper = choice;
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
