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
	mTargetWaveshapers.ensureStorageAllocated(1);
	//mTargetWaveshapers.add (std::make_unique<dsp::LookupTableTransform<float>>([](float x) {return tanh(20 * x); }, -1.0f, 1.0f, 512));
	mTargetWaveshapers.add(std::make_unique<dsp::LookupTableTransform<float>>([](float x) {return tanh(20 * x); }, -1.0f, 1.0f, 512));
	mTargetWaveshapers.minimiseStorageOverheads();
	mSideChain.setSize(mNumChannels, mBlockSize);

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
