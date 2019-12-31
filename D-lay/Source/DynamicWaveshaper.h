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

	//save environment variables and waveshaper/sidechain
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
			//process sidechain and apply amount of waveshaping proportional to it

			

			for (int channel = 0; channel < mNumChannels; ++channel)
			{
				for (int i = 0; i < mBlockSize; ++i)
				{
					//use step response of a one pole low pass filter (IIR) to apply attack and release parameters to binary threshold input
					if (mSideChainThreshIn[channel] > mLastSample[channel]) {
						mLastSample[channel] = mBufAttackCoeff * mLastSample[channel] + (1 - mBufAttackCoeff) * mSideChainThreshIn[channel]; // y_n = a*y_n-1 + (1-a)*x_n
					}
					else {
						mLastSample[channel] = mBufReleaseCoeff * mLastSample[channel]; //y_n = r*y_n-1
					}
					//apply waveshaping with smoothed envelope the weight
					outputBlock.getChannelPointer(channel)[i] = std::lerp(
						inputBlock.getChannelPointer(channel)[i],
						mTargetWaveshaper.processSampleUnchecked(inputBlock.getChannelPointer(channel)[i]),
						mLastSample[channel]
					);
					//update max value in chunk
					if (abs(inputBlock.getChannelPointer(channel)[i]) > mChunkMaxIn[channel])
						mChunkMaxIn[channel] = abs(inputBlock.getChannelPointer(channel)[i]);
					//save threshold of max value and reset for next chunk
					if (++mChunkCounter[channel] == mChunkSize)
					{
						if (mChunkMaxIn[channel] > mBufThreshold) { mSideChainThreshIn[channel] = 1.0f; }
						else { mSideChainThreshIn[channel] = 0.0f; }
						mChunkCounter[channel] = 0;
						mChunkMaxIn[channel] = 0.0f;
					}
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

	//target waveshapers
	dsp::LookupTableTransform<float> mTargetWaveshaper;

	//envelope variables
	std::unique_ptr<float[]> mLastSample, mChunkMaxIn, mSideChainThreshIn; //use smart pointer to construct array of zeros in prepare and auto delete in destructor
	std::unique_ptr<int[]> mChunkCounter;
	int mChunkSize;

	//called once per process
	void updateBufParams() noexcept
	{
		mBufThreshold = mThreshold.get();
		mBufAttackCoeff = mAttackCoeff.get();
		mBufReleaseCoeff = mReleaseCoeff.get();
	}

	//parameters updated via Atomic loads once per buffer
	float mBufThreshold, mBufAttackCoeff, mBufReleaseCoeff;

	//instantaneous processing parameters wrapped in Atomic for thread safety (units: gain, coefficient, coefficient)
	Atomic<float> mThreshold = 0.1f, mAttackCoeff, mReleaseCoeff;

	//environment variables
	int mBlockSize, mNumChannels, mSampleRate;
};
//TODO SIMD