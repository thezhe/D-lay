/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//Approximately one second delay line with ability to write, read, and modify written memory
class DelayLine 
{
public:

	// Essential Methods
	//==============================================================================
	
	//save environment variables and setup delay line 
	void prepare(const dsp::ProcessSpec spec);

	//copy buffer to mWriteBlock's data in delay line 
	void fillDelayBuffer(AudioBuffer<float>& buffer) noexcept
	{
		//use smart pointer to automatically manage garbage for every new assignment 
		mWriteBlock = std::make_unique<dsp::AudioBlock<float>>(mDelayBufferBlock.getSubBlock(mWritePosition, mBlockSize));
		mWriteBlock->copyFrom(buffer, 0, 0, mBlockSize);
	}

	//add delayed signal to buffer and mWriteBlock's data in delay line
	void getFromDelayBuffer(AudioBuffer<float>& buffer) noexcept
	{
		updateBufParams();

		//set mReadPosition
		mReadPosition = (mDelayBufferLength + mWritePosition - mBufRate) % mDelayBufferLength;
		
		//normal get case
		if (mDelayBufferLength > mBlockSize + mReadPosition)
		{
			for (int channel = 0; channel < mNumChannels; ++channel) {
				//get read data
				const float* delayBufferReadPos = mDelayBuffer.getReadPointer(channel, mReadPosition);
				mDelayBuffer.addFrom(channel, mWritePosition, delayBufferReadPos, mBlockSize, mBufFeedback);
				buffer.addFrom(channel, 0, delayBufferReadPos, mBlockSize, mBufWet);
			}
		}
		//circular buffer wrap-around
		else 
		{
			for (int channel = 0; channel < mNumChannels; ++channel) {
				//get read data and remaining samples in mDelayBuffer
				const float* delayBufferReadPos = mDelayBuffer.getReadPointer(channel, mReadPosition);
				const int bufferRemaining = mDelayBufferLength - mReadPosition;
				mDelayBuffer.addFrom(channel, mWritePosition, delayBufferReadPos, bufferRemaining, mBufFeedback);
				buffer.addFrom(channel, 0, delayBufferReadPos, bufferRemaining, mBufWet);
				mDelayBuffer.addFrom(channel, mWritePosition + bufferRemaining, delayBufferReadPos - mReadPosition, mBlockSize - bufferRemaining, mBufFeedback);
				buffer.addFrom(channel, bufferRemaining, delayBufferReadPos - mReadPosition, mBlockSize - bufferRemaining, mBufWet);
			}
		}
		//update mWritePosition
		mWritePosition += mBlockSize;
		if (mWritePosition == mDelayBufferLength)
			mWritePosition = 0;
	}
	
	// Parameters, mWriteBlock, and Extras
	//==============================================================================

	//set Rate using ms value between 0 and 1000
	void setRate(float msRate) noexcept;

	//set Feedback using decibel value between -INF and 0.0f
	void setFeedback(float dbFeedback) noexcept;

	//set Wet using percent value between 0 and 100
	void setWet(int percentWet) noexcept;
	
	//process after call to fillDelayLine and before call to getFromDelayLine to simulate delay line insertion effects
	std::unique_ptr<dsp::AudioBlock<float>> mWriteBlock;

private:
	
	//called once per getFromDelayBuffer
	void updateBufParams() noexcept
	{
		mBufRate = mRate.get();
		mBufFeedback = mFeedback.get();
		mBufWet = mWet.get();
	}

	//parameters updated via Atomic loads once per buffer
	int mBufRate = 22050;
	float mBufFeedback = 0.6f, mBufWet = 0.75f;


	//instantaneous processing parameters wrapped in Atomic for thread safety (units: num samples, gain, gain)
	Atomic<int> mRate = 22050; 
	Atomic<float> mFeedback = 0.6f, mWet = 0.75f;

	//delay buffer variables
	int mWritePosition = 0, mReadPosition, mDelayBufferLength;
	dsp::AudioBlock<float> mDelayBufferBlock;
	AudioBuffer<float> mDelayBuffer;

	//environment variables
	int mSampleRate, mBlockSize, mNumChannels;
};