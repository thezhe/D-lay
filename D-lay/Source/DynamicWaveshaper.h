/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class DynamicWaveshaper
{
public:

	// Essential Methods
	//==============================================================================

	//save environment variables and waveshaper/en
	void prepare(dsp::ProcessSpec spec);

	//extract thresholded, smoothed signal envelope and apply weighted waveshaper of choice
	template <typename ProcessContext>
	void process(const ProcessContext& context) noexcept
	{
		//get I/O blocks and handle bypass
		const auto& inputBlock = context.getInputBlock();
		auto& outputBlock = context.getOutputBlock();
		if (context.isBypassed)
		{
			if (context.usesSeparateInputAndOutputBlocks())
				outputBlock.copyFrom(inputBlock);
		}
		//process
		else
		{
			updateBufParams();
			updateSideChain(inputBlock);
			//apply amount of waveshaping proportional to sidechain signal
			for (int channel = 0; channel < mNumChannels; ++channel) {
				for (int i = 0; i < mBlockSize; ++i) {
					outputBlock.getChannelPointer(channel)[i] = std::lerp(
						inputBlock.getChannelPointer(channel)[i],
						mTargetWaveshapers.getUnchecked(0)->processSampleUnchecked(inputBlock.getChannelPointer(channel)[i]),
						mSideChain.getSample(channel, i)
					);
				}
			}
		}
	}

	// Parameters
	//==============================================================================

	//set Threshold using decibel value <= 0.0f
	void setThreshold(float dbThreshold) noexcept;

	//set Attack using  ms value >= 0.0f
	void setAttack(float msAttack) noexcept;

	//set Release using ms value >= 0.0f
	void setRelease(float msRelease) noexcept;

private:

	//save an audio block's signal envelope to a side chain buffer using Threshold, Attack, and Release parameters(call once per process after updateBufParams)
	void updateSideChain(const dsp::AudioBlock<const float>& inputBlock) noexcept
	{
#if JUCE_USE_SIMD
		//=======================interleave inputBlock for processing
		auto* inout = mChannelPointers.getData();
		for (size_t channel = 0; channel < dsp::SIMDRegister<float>::size(); ++channel)
			inout[channel] = (channel < inputBlock.getNumChannels()) ? const_cast<float*> (inputBlock.getChannelPointer(channel)) : mZero.getChannelPointer(channel); //fill extra channels with zero data
		AudioDataConverters::interleaveSamples(inout, reinterpret_cast<float*> (mInterleaved.getChannelPointer(0)), //make sure to set float granularity
			mBlockSize, static_cast<int> (dsp::SIMDRegister<float>::size()));
		//=======================process mInterleaved data
		//process first samples
		mChunkMaxIn = dsp::SIMDRegister<float>::max(dsp::SIMDRegister<float>::abs(mInterleaved.getChannelPointer(0)[0]), mChunkMaxIn); //get max before processing
		auto iirMask = dsp::SIMDRegister<float>::greaterThan(mSideChainThreshIn, mLastSample);
		mInterleaved.getChannelPointer(0)[0] = ((mBufAttackCoeff * mLastSample + ((ONE - mBufAttackCoeff) * mSideChainThreshIn)) & iirMask) 
			+ ((mBufReleaseCoeff*mLastSample) & (~iirMask));
		if (++mChunkCounter == mChunkSize)
		{
			mChunkCounter = 0;
			auto maxMask = dsp::SIMDRegister<float>::greaterThan(mChunkMaxIn, mBufThreshold);
			mSideChainThreshIn = (ONE & maxMask) + (ZERO & (~maxMask));
			mChunkMaxIn = 0.0f;
		}
		for (int i = 1; i < mBlockSize; ++i) {
			mChunkMaxIn = dsp::SIMDRegister<float>::max(dsp::SIMDRegister<float>::abs(mInterleaved.getChannelPointer(0)[i]), mChunkMaxIn);
			auto iirLoopMask = dsp::SIMDRegister<float>::greaterThan(mSideChainThreshIn, mInterleaved.getChannelPointer(0)[i-1]);
			mInterleaved.getChannelPointer(0)[i] = ((mBufAttackCoeff * mInterleaved.getChannelPointer(0)[i - 1] + ((ONE - mBufAttackCoeff) * mSideChainThreshIn)) & iirLoopMask) 
				+ ((mBufReleaseCoeff * mInterleaved.getChannelPointer(0)[i - 1]) & (~iirLoopMask));
			if (++mChunkCounter == mChunkSize)
			{
				mChunkCounter = 0;
				auto maxMask = dsp::SIMDRegister<float>::greaterThan(mChunkMaxIn, mBufThreshold);
				mSideChainThreshIn = (ONE & maxMask) + (ZERO & (~maxMask));
				mChunkMaxIn = 0.0f;
			}
		}
		mLastSample = mInterleaved.getChannelPointer(0)[mBlockSize - 1];
		//=======================deinterleave
		for (size_t channel = 0; channel < inputBlock.getNumChannels(); ++channel)
			inout[channel] = mSideChain.getWritePointer(channel);
		AudioDataConverters::deinterleaveSamples(reinterpret_cast<float*> (mInterleaved.getChannelPointer(0)),
				const_cast<float**> (inout), mBlockSize, static_cast<int> (dsp::SIMDRegister<float>::size()));
#else
		//process first samples
		for (int channel = 0; channel < mNumChannels; ++channel)
		{
			//use step response of a one pole low pass filter (IIR) to apply attack and release parameters to binary side chain
			if (mSideChainThreshIn[channel] > mLastSample[channel]) {	
				mSideChain.getWritePointer(channel)[0] = mBufAttackCoeff * mLastSample[channel] + ((1 - mBufAttackCoeff) * mSideChainThreshIn[channel]);
			}
			else {
				mSideChain.getWritePointer(channel)[0] = mBufReleaseCoeff * mLastSample[channel];
			}
			//update max value in chunk
			if (abs(inputBlock.getChannelPointer(channel)[0]) > mChunkMaxIn[channel])
				mChunkMaxIn[channel] = abs(inputBlock.getChannelPointer(channel)[0]);
		}
		//threshold max value and reset for next chunk
		if (++mChunkCounter == mChunkSize)
		{
			mChunkCounter = 0;
			for (int channel = 0; channel < mNumChannels; ++channel)
			{
				if (mChunkMaxIn[channel] > mBufThreshold) { mSideChainThreshIn[channel] = 1.0f; }
				else { mSideChainThreshIn[channel] = 0.0f; }
				mChunkMaxIn[channel] = 0.0f;
			}
		}
		//process rest of samples
		for (int i = 1; i < mBlockSize; ++i)
		{
			for (int channel = 0; channel < mNumChannels; ++channel)
			{
				if (mSideChainThreshIn[channel] > mSideChain.getWritePointer(channel)[i-1]) {
					//use step response of a one pole low pass filter (IIR) to apply attack and release parameters to binary side chain
					mSideChain.getWritePointer(channel)[i] = mBufAttackCoeff * mSideChain.getWritePointer(channel)[i - 1] + ((1 - mBufAttackCoeff) * mSideChainThreshIn[channel]);
				}
				else {
					mSideChain.getWritePointer(channel)[i] = mBufReleaseCoeff * mSideChain.getWritePointer(channel)[i - 1];
				}
				//update max value in chunk
				if (abs(inputBlock.getChannelPointer(channel)[i]) > mChunkMaxIn[channel])
					mChunkMaxIn[channel] = abs(inputBlock.getChannelPointer(channel)[i]);
			}
			//threshold max value and reset for next chunk
			if (++mChunkCounter == mChunkSize)
			{
				mChunkCounter = 0;
				for (int channel = 0; channel < mNumChannels; ++channel)
				{
					if (mChunkMaxIn[channel] > mBufThreshold) { mSideChainThreshIn[channel] = 1.0f; }
					else { mSideChainThreshIn[channel] = 0.0f; }
					mChunkMaxIn[channel] = 0.0f;
				}
			}
		}
		//save last sample
		for (int channel = 0; channel < mNumChannels; ++channel)
		{
			mLastSample[channel] = mSideChain.getWritePointer(channel)[mBlockSize-1];
		}
#endif
	}

	//dynamic waveshaping variables
	OwnedArray<dsp::LookupTableTransform<float>> mTargetWaveshapers; //smart array that deletes objects in destructor
	AudioBuffer<float> mSideChain;

	//envelope variables
	int mChunkSize, mChunkCounter = 0;
#if JUCE_USE_SIMD
	dsp::SIMDRegister<float> mLastSample = 0, mChunkMaxIn = 0, mSideChainThreshIn = 0;
	//data used to interleave mChannelPointers data into interleavedBlockData
	dsp::AudioBlock<dsp::SIMDRegister<float>> mInterleaved;
	dsp::AudioBlock<float> mZero;
	HeapBlock<char> interleavedBlockData, zeroData;
	HeapBlock<const float*> mChannelPointers{ dsp::SIMDRegister<float>::size() };
#else
	std::unique_ptr<float[]> mLastSample, mChunkMaxIn, mSideChainThreshIn; //use smart pointer to construct in prepare and auto delete in destructor
#endif

	//called once per process
	void updateBufParams() noexcept
	{
		mBufThreshold = mThreshold.get();
		mBufAttackCoeff = mAttackCoeff.get();
		mBufReleaseCoeff = mReleaseCoeff.get();
	}

	//parameters updated via Atomic loads once per buffer
#if JUCE_USE_SIMD
	dsp::SIMDRegister<float> mBufThreshold, mBufAttackCoeff, mBufReleaseCoeff;
	const dsp::SIMDRegister<float> ONE = 1.0f, ZERO = 0.0f;
#else
	float mBufThreshold, mBufAttackCoeff, mBufReleaseCoeff;
#endif
	//instantaneous processing parameters wrapped in Atomic for thread safety (units: gain, coefficient, coefficient)
	Atomic<float> mThreshold = 0.1f, mAttackCoeff = 0.99f, mReleaseCoeff = 0.99f;

	//environment variables
	int mBlockSize, mNumChannels, mSampleRate;
};
