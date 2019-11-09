#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DynamicWaveshaper::DynamicWaveshaper(const AudioBuffer<float>& dynamicWavetable, dsp::ProcessSpec spec)
	:mDynamicWavetable (dynamicWavetable), mBlockSize(spec.maximumBlockSize), 
	mNumChannels(spec.numChannels), mHalfTableSize (dynamicWavetable.getNumSamples()/2)
{
	jassert(mDynamicWavetable.getNumChannels() > 0);
}

void DynamicWaveshaper::process(AudioBuffer<float>& buffer) {
	auto* test = mDynamicWavetable.getReadPointer(0);
	for (int channel = 0; channel < mNumChannels; ++channel) {
		float* buf = buffer.getWritePointer(channel);
		for (int i = 0; i < mBlockSize; ++i) {
			float realIndex = buf[i]* 63.5 + 63.5;
			auto index0 = (unsigned int) realIndex;
			auto index1 = index0 + 1;
			float argT = realIndex - (float) index0;
			buf[i] = test[index0] + argT * (test[index1] - test[index0]);
		}
	}
}
//==============================================================================
DlayAudioProcessor::DlayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

DlayAudioProcessor::~DlayAudioProcessor()
{
}

//==============================================================================
const String DlayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DlayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DlayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DlayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DlayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DlayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DlayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DlayAudioProcessor::setCurrentProgram (int index)
{
}

const String DlayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DlayAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void DlayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	//environment variables and member buffers
	mTotalNumInputChannels = getTotalNumInputChannels();
	mTotalNumOutputChannels = getTotalNumOutputChannels();
	mBufferLength = samplesPerBlock;
	mDelayBufferLength = 2 * (sampleRate + samplesPerBlock);
	mSampleRate = sampleRate;
	mDelayBuffer.setSize(mTotalNumInputChannels, mDelayBufferLength);
	mSendToDelayBuffer.setSize(mTotalNumInputChannels, mBufferLength);
	//ResonantLP
	dsp::ProcessSpec spec{ sampleRate, static_cast<uint32>(mBufferLength), static_cast<uint32>(mTotalNumInputChannels) };
	LP.setCutoffFrequencyHz(3000.0f);
	LP.setResonance(0.2f);
	LP.setMode(dsp::LadderFilter<float>::Mode::LPF24);
	LP.prepare(spec);
	//Waveshaper
	
	tanH.setSize(1, 128);
	float* tanEdit = tanH.getWritePointer(0);
	for (int i = 0; i < 128; ++i)
		tanEdit[i] = tanh(2*(i/127)-1);
	mChebyshevWaveshaper = new DynamicWaveshaper(tanH, spec);
	mLock = false;
}

void DlayAudioProcessor::releaseResources()
{
	LP.reset();
	delete mChebyshevWaveshaper;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DlayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DlayAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	ScopedNoDenormals noDenormals;

	if (mLock == true)
		return;
	/*
	potential enhancement for stopping and clearing buffers when daw pauses play
	AudioPlayHead::CurrentPositionInfo info;
	getAudioPlayHead().getCurrentPosition(info);
	if (info.isPlaying)
		foo();*/

	//clear data
	for (auto i = mTotalNumInputChannels; i < mTotalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());
	//====================================================================processing
	//Read and Send
	mSendToDelayBuffer.makeCopyOf(buffer);
    for (int channel = 0; channel < mTotalNumInputChannels; ++channel)
    {
		const float* bufferData = buffer.getReadPointer(channel);
		const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
		getFromDelayBuffer(buffer, channel, mBufferLength, mDelayBufferLength, delayBufferData, bufferData);
    }
	//LPF24
	dsp::AudioBlock<float> block(mSendToDelayBuffer);
	dsp::ProcessContextReplacing<float> context(block);
	LP.process(context);
	//add in nonlinear here
	mChebyshevWaveshaper->process(mSendToDelayBuffer);

	//mChebyshevWaveshaper->process(mSendToDelayBuffer.getWritePointer(0));
	//mChebyshevWaveshaper->process(mSendToDelayBuffer.getWritePointer(1));
	//==================
	for (int channel = 0; channel < mTotalNumInputChannels; ++channel)
	{
		const float* bufData = mSendToDelayBuffer.getReadPointer(channel);
		fillDelayBuffer(channel, mBufferLength, mDelayBufferLength, bufData);
	}
	mWritePosition += mBufferLength;
	mWritePosition %= mDelayBufferLength;
}


void DlayAudioProcessor::getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, int bufferLength,
	int mDelayBufferLength, const float* delayBufferData, const float* bufferData)
{
	const int readPosition = static_cast<int> (mDelayBufferLength + mWritePosition - (mSampleRate * mRate / 1000))
		% mDelayBufferLength;
	if (mDelayBufferLength > mBufferLength + readPosition)
	{
		mSendToDelayBuffer.addFrom(channel, 0, delayBufferData + readPosition, mBufferLength, mFeedback);
		buffer.addFrom(channel, 0, delayBufferData + readPosition, mBufferLength, mWet);
	}
	else
	{
		const int bufferRemaining = mDelayBufferLength - readPosition;
		mSendToDelayBuffer.addFrom(channel, 0, delayBufferData + readPosition, bufferRemaining, mFeedback);
		mSendToDelayBuffer.addFrom(channel, bufferRemaining, delayBufferData, mBufferLength - bufferRemaining, mFeedback);
		buffer.addFrom(channel, 0, delayBufferData + readPosition, bufferRemaining, mWet);
		buffer.addFrom(channel, bufferRemaining, delayBufferData, mBufferLength - bufferRemaining, mWet);
	}
}
void DlayAudioProcessor::fillDelayBuffer(int channel, int bufferLength, int delayBufferLength, const float* bufferData)
{
	if (mDelayBufferLength > mBufferLength + mWritePosition)
	{
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, mBufferLength);
	}
	else {
		const int bufferRemaining = mDelayBufferLength - mWritePosition;
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferRemaining);
		mDelayBuffer.copyFrom(channel, 0, bufferData + bufferRemaining, mBufferLength - bufferRemaining);
	}

}


//==============================================================================
bool DlayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* DlayAudioProcessor::createEditor()
{
    return new DlayAudioProcessorEditor (*this);
}

//==============================================================================
void DlayAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DlayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DlayAudioProcessor();
}
