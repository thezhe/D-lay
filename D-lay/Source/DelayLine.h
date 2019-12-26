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
		updateSmoothers();
		//set mReadPosition
		mReadPosition = (mDelayBufferLength + mWritePosition - mRate.get()) % mDelayBufferLength;
		
		//normal get case
		if (mDelayBufferLength > mBlockSize + mReadPosition)
		{
			for (int channel = 0; channel < mNumChannels; ++channel) {
				//get read data
				const float* delayBufferReadPos = mDelayBuffer.getReadPointer(channel, mReadPosition);
				mDelayBuffer.addFrom(channel, mWritePosition, delayBufferReadPos, mBlockSize, mFeedback);
				buffer.addFrom(channel, 0, delayBufferReadPos, mBlockSize, mWet);
			}
		}
		//circular buffer wrap-around
		else 
		{
			for (int channel = 0; channel < mNumChannels; ++channel) {
				//get read data and remaining samples in mDelayBuffer
				const float* delayBufferReadPos = mDelayBuffer.getReadPointer(channel, mReadPosition);
				const int bufferRemaining = mDelayBufferLength - mReadPosition;
				mDelayBuffer.addFrom(channel, mWritePosition, delayBufferReadPos, bufferRemaining, mFeedback);
				buffer.addFrom(channel, 0, delayBufferReadPos, bufferRemaining, mWet);
				mDelayBuffer.addFrom(channel, mWritePosition + bufferRemaining, delayBufferReadPos - mReadPosition, mBlockSize - bufferRemaining, mFeedback);
				buffer.addFrom(channel, bufferRemaining, delayBufferReadPos - mReadPosition, mBlockSize - bufferRemaining, mWet);
			}
		}
		//update mWritePosition
		mWritePosition += mBlockSize;
		if (mWritePosition == mDelayBufferLength)
			mWritePosition = 0;
	}
	
	// Parameters, mWriteBlock, and Extras
	//==============================================================================

	//set Rate using ms value
	void setRate(float msRate) noexcept;

	//set Feedback using decibel value
	void setFeedback(float dbFeedback) noexcept;

	//set Wet using percent value
	void setWet(int percentWet) noexcept;
	
	//process after call to fillDelayLine and before call to getFromDelayLine to simulate delay line insertion effects
	std::unique_ptr<dsp::AudioBlock<float>> mWriteBlock;

	//clear mDelayBuffer
	void clearDelayBuffer() noexcept;

private:

	//instantaneous processing parameters (units: num samples, gain, gain)
	Atomic<int> mRate = 22050; //Rate is not smoothed, wrap in Atomic for thread safety
	float mFeedback = 0.6f, mWet = 0.75f;

	//use  lock-free and thread safe smoothed parameters
	LinearSmoothedValue<float>mFeedbackSmoothed, mWetSmoothed;

	//call in prepare to setup smoothers
	void initSmoothers(double rampTime);

	//call once at start of process to update instantaneous parameters
	void updateSmoothers() noexcept
	{
		mFeedback = mFeedbackSmoothed.skip(mBlockSize);
		mWet = mFeedbackSmoothed.skip(mBlockSize);
	}

	//delay buffer variables
	int mWritePosition = 0, mReadPosition;
	dsp::AudioBlock<float> mDelayBufferBlock;
	AudioBuffer<float> mDelayBuffer;

	//environment variables
	int mSampleRate, mBlockSize, mNumChannels, mDelayBufferLength;
};