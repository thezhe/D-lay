#include "PluginProcessor.h"
#include "PluginEditor.h"

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
	totalNumInputChannels = getTotalNumInputChannels();
	totalNumOutputChannels = getTotalNumOutputChannels();
	bufferLength = samplesPerBlock;
	delayBufferLength = 2 * (sampleRate + samplesPerBlock);
	mSampleRate = sampleRate;
	mDelayBuffer.setSize(totalNumInputChannels, delayBufferLength);
	//ResonantLP
	dsp::ProcessSpec spec{ sampleRate, static_cast<uint32>(bufferLength), static_cast<uint32>(totalNumInputChannels) };
	LP.setCutoffFrequencyHz(3000.0f);
	LP.setResonance(0.2f);
	LP.setDrive(2);
	LP.setMode(dsp::LadderFilter<float>::Mode::LPF12);
	LP.prepare(spec);
	isActive = true;
}

void DlayAudioProcessor::releaseResources()
{
	LP.reset();
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

	if (isActive == false)
		return;
	/*
	potential enhancement for stopping and clearing buffers when daw pauses play
	AudioPlayHead::CurrentPositionInfo info;
	getAudioPlayHead().getCurrentPosition(info);
	if (info.isPlaying)
		foo();*/
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	//====================================================================processing
	//Read
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
		const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
		getFromDelayBuffer(buffer, channel, bufferLength, delayBufferLength, delayBufferData);
    }
	//LPF24 and fill delay line
	dsp::AudioBlock<float> block(buffer);
	dsp::ProcessContextReplacing<float> context(block);
	LP.process(context);
	for (int channel = 0; channel < totalNumInputChannels; ++channel)
	{
		const float* bufferData = buffer.getReadPointer(channel);
		fillDelayBuffer(channel, bufferLength, delayBufferLength, bufferData);
	}
	mWritePosition += bufferLength;
	mWritePosition %= delayBufferLength;
}


void DlayAudioProcessor::getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, int bufferLength,
	int delayBufferLength, const float* delayBufferData)
{
	const int readPosition = static_cast<int> (delayBufferLength + mWritePosition - (mSampleRate * delayTime / 1000))
		% delayBufferLength;

	if (delayBufferLength > bufferLength + readPosition)
	{
		buffer.addFromWithRamp(channel, 0, delayBufferData + readPosition, bufferLength, mFeedback, mFeedback);
	}
	else
	{
		const int bufferRemaining = delayBufferLength - readPosition;
		buffer.addFromWithRamp(channel, 0, delayBufferData + readPosition, bufferRemaining, mFeedback, mFeedback);
		buffer.addFromWithRamp(channel, bufferRemaining, delayBufferData, bufferLength - bufferRemaining, mFeedback, mFeedback);
	}
}
void DlayAudioProcessor::fillDelayBuffer(int channel, int bufferLength, int delayBufferLength, const float* bufferData)
{
	if (delayBufferLength > bufferLength + mWritePosition)
	{
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferLength);
	}
	else {
		const int bufferRemaining = delayBufferLength - mWritePosition;
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferRemaining);
		mDelayBuffer.copyFrom(channel, 0, bufferData + bufferRemaining, bufferLength - bufferRemaining);
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
