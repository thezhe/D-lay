/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//A "dynamic" waveshaper that loosely follows JUCE's dsp module format, interpolates between dry signal and waveshaped signal based on envelope response
class DynamicWaveshaper
{
public:
	DynamicWaveshaper(dsp::ProcessSpec spec)
		:mBlockSize(spec.maximumBlockSize),
		mNumChannels(spec.numChannels),
		mSampleRate(spec.sampleRate),
		mChunkSize(128),
		mNumChunks(mBlockSize / mChunkSize)
	{
		//set up y[n-1] for the IIR filtering of envelope of each channel
		mLastSample = new float[mNumChannels];
		for (int channel = 0; channel < mNumChannels; ++channel) {
			mLastSample[channel] = 0;
		}
		//use exaggerated waveshaper for testing:
		//mTargetWaveshaper.initialise([](float x) {return 0.2*std::tanh(25*x); }, -1.0f, 1.0f, 512);
		/*mTargetWaveshaper.initialise(
			[](float x) { return x - ((x * x) / 8) - (pow(x, 3) / 18) + 0.125; },
			-1.0f,
			1.0f,
			512
		);*/
		mTargetWaveshaper.initialise([](float x) {return tanh(x); }, -1.0f, 1.0f, 512);
		mSideChain.setSize(mNumChannels, mBlockSize);
		mEnvSmoother.setCutoffFrequencyHz(150.0f);
		mEnvSmoother.setMode(dsp::LadderFilter<float>::Mode::LPF12);
		mEnvSmoother.prepare(spec);
	}
	//take in threshold in dB and store in mThreshold as gain value
	void setThreshold(float threshold) noexcept;
	//take in attack time in ms and convert to IIR coefficients
	void setAttack(float attack) noexcept;
	//take in release time in ms and convert to IIR coefficients
	void setRelease(float release) noexcept;
	template <typename ProcessContext>
	void process(const ProcessContext& context) noexcept
	{
		const auto& inputBlock = context.getInputBlock();
		auto& outputBlock = context.getOutputBlock();
		if (context.isBypassed)
		{
			if (context.usesSeparateInputAndOutputBlocks())
				outputBlock.copyFrom(inputBlock);
		}
		else
		{
			//get max in chunks and smooth
			for (int channel = 0; channel < mNumChannels; ++channel) {
				int startChunk = 0;
				for (int chunk = 1; chunk < mNumChunks; ++chunk) {
					float maxInChunk = 0;
					for (int c = startChunk; c < startChunk + mChunkSize; ++c) {
						if (abs(inputBlock.getChannelPointer(channel)[c]) > maxInChunk)
							maxInChunk = abs(inputBlock.getChannelPointer(channel)[c]);
					}
					for (int i = startChunk; i < startChunk + mChunkSize; ++i) {
						mSideChain.setSample(channel, i, maxInChunk);
					}
					startChunk += mChunkSize;
				}
			}
			dsp::AudioBlock<float> block(mSideChain);
			dsp::ProcessContextReplacing<float> context(block);
			mEnvSmoother.process(context);
			//convert to square wave using threshold and use the step response of a one pole low pass filter (IIR) to apply attack and release parameters
			for (int channel = 0; channel < mNumChannels; ++channel) {
				auto mSideChainData = mSideChain.getWritePointer(channel);
				for (int i = 0; i < mBlockSize; ++i) {
					if (mSideChainData[i] > mThreshold) { mSideChainData[i] = 1; }
					else { mSideChainData[i] = 0; }
					if (mSideChainData[i] > mLastSample[channel]) {
						mSideChainData[i] = mAttackCoeff * mLastSample[channel] + mOneMinusAttackCoeff * mSideChainData[i];
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

private:
	//internal parameters modified through public methods
	float mThreshold;
	float mAttackCoeff;
	float mOneMinusAttackCoeff;
	float mReleaseCoeff;
	//const enviornment variables
	const uint32 mBlockSize, mNumChannels;
	const double mSampleRate;
	const int mChunkSize, mNumChunks;
	//LPF for envelope smoothing with minimal phase
	dsp::LadderFilter<float> mEnvSmoother;
	//dynamic variables
	dsp::LookupTableTransform<float> mTargetWaveshaper;
	AudioBuffer<float> mSideChain;
	float* mLastSample;
};