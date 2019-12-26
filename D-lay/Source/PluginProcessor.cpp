#include "PluginProcessor.h"
#include "PluginEditor.h"


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
	dsp::ProcessSpec spec{ sampleRate, static_cast<uint32>(samplesPerBlock), static_cast<uint32>(mTotalNumInputChannels) };
	//mAAfilter
	mAAfilter.setCutoffFrequencyHz(2500.0f);
	mAAfilter.setResonance(0.3f);
	mAAfilter.setMode(dsp::LadderFilter<float>::Mode::LPF24);
	mAAfilter.prepare(spec);
	//mDynamicWaveshaper
	//mDynamicWaveshaper = new DynamicWaveshaper(spec);
	//mEchoProcessor
	mEchoProcessor.prepare(spec);
}

void DlayAudioProcessor::releaseResources()
{
	//mAAfilter.reset();
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
	mEchoProcessor.fillDelayBuffer(buffer);

		//dsp::ProcessContextReplacing<float> writeBlock(mEchoProcessor->mWriteBlock);
		//mAAfilter.process(writeBlock);
		//mDynamicWaveshaper->process(writeBlock);

	mEchoProcessor.getFromDelayBuffer(buffer);
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
