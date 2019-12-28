/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//A "dynamic" waveshaper that interpolates between dry signal and waveshaped signal based on input signal envelope. Loosely follows JUCE's dsp processor format.
class DynamicWaveshaper
{
public:

	// Essential Methods
	//==============================================================================

	void prepare(dsp::ProcessSpec spec);

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
		else
		{
			updateSideChain(inputBlock);
			//use step response of a one pole low pass filter (IIR) to apply attack and release parameters to binary side chain
			for (int channel = 0; channel < mNumChannels; ++channel) {
				auto mSideChainData = mSideChain.getWritePointer(channel);
				for (int i = 0; i < mBlockSize; ++i) {
					if (mSideChainData[i] > mLastSample[channel]) {
						mSideChainData[i] = mAttackCoeff * mLastSample[channel] + ((1 - mAttackCoeff) * mSideChainData[i]);
					}
					else {
						mSideChainData[i] = mReleaseCoeff * mLastSample[channel];
					}
					mLastSample[channel] = mSideChainData[i];
				}
			}
			//apply amount of waveshaping proportional to sidechain signal
			for (int channel = 0; channel < mNumChannels; ++channel) {
				for (int i = 0; i < mBlockSize; ++i) {
					outputBlock.getChannelPointer(channel)[i] = std::lerp(
						inputBlock.getChannelPointer(channel)[i],
						mTargetWaveshaper.processSampleUnchecked(inputBlock.getChannelPointer(channel)[i]),
						mSideChain.getSample(channel, i)
					);
				}
			}
		}
	}

	// Parameters
	//==============================================================================

	//take in threshold in dB and store in mThreshold as gain value
	void setThreshold(float threshold) noexcept;
	//take in attack time in ms and convert to IIR coefficients, adds on to preexisting delay in envelope 
	void setAttack(float attack) noexcept;
	//take in release time in ms and convert to IIR coefficients
	void setRelease(float release) noexcept;

private:

	//store input into sidechain as a binary envelope using threshold, call once per process
	void updateSideChain(const dsp::AudioBlock<const float>& inputBlock) noexcept
	{
		for (int channel = 0; channel < mNumChannels; ++channel)
		{
			auto mSideChainData = mSideChain.getWritePointer(channel);
			const auto channelPointer = inputBlock.getChannelPointer(channel);
			for (int i = 0; i < mBlockSize; ++i)
			{
				mSideChain.setSample(channel, i, mSideChainThreshIn[channel]);
				//update max value in chunk
				if (abs(channelPointer[i]) > mChunkMaxIn[channel])
					mChunkMaxIn[channel] = abs(channelPointer[i]);
				//threshold max value and reset for next chunk
				if (++mChunkCounter[channel] == mChunkSize)
				{
					if (mChunkMaxIn[channel] > mThreshold) { mSideChainThreshIn[channel] = 1.0f; }
					else { mSideChainThreshIn[channel] = 0.0f; }
					mChunkCounter[channel] = 0;
					mChunkMaxIn[channel] = 0.0f;
				}
			}
		}
	}
	
	//internal parameters modified through public methods
	float mThreshold = 0.1f;
	float mAttackCoeff;
	float mReleaseCoeff;

	//custom union type that enables float bitmasking (as a faster alternative to branching)
	union intFloat
	{
	public:
		int i;
		float f;
		intFloat(float F) 
		{
			f = F;
		}
		intFloat(int I)
		{
			i = I;
		}
		intFloat operator& (intFloat const& arg) noexcept
		{
			return intFloat(i & arg.i);
		}
		intFloat operator+ (intFloat const& arg) noexcept
		{
			return intFloat(f + arg.f);
		}
	};

	//dynamic waveshaping variables
	dsp::LookupTableTransform<float> mTargetWaveshaper;
	AudioBuffer<float> mSideChain; //smoothed sidechain signal

	//envelope variables
	int mChunkSize;
	std::unique_ptr<float[]> mLastSample, mChunkMaxIn, mSideChainThreshIn; //use smart pointer to construct in prepare and auto delete in destructor
	std::unique_ptr<int[]> mChunkCounter;
		
	//environment variables
	int mBlockSize, mNumChannels, mSampleRate;
};
//TODO SIMD and masking if not using SIMD