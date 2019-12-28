/*
  ==============================================================================
	Zhe Deng 2019
	thezhefromcenterville@gmail.com
  ==============================================================================
*/

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
                       ),
	parameters(*this, nullptr, Identifier("Dlay"),
		{
			std::make_unique<AudioParameterFloat>("rate", //ms
												"Rate",
												NormalisableRange<float>(0.0f, 1000.0f, 0.01f, 0.25f),
												150.0f),
			std::make_unique<AudioParameterFloat>("feedback", //dB
												"Feedback",
												-40.0f,
												0.0f,
												-15.0f),
			std::make_unique<AudioParameterInt>("wet", //percent
												"Wet",
												0,
												100,
												50),
			std::make_unique<AudioParameterFloat>("cutoff", //Hz
												"Cutoff",
												1000.0f,
												3000.0f,
												2000.0f),
			std::make_unique<AudioParameterFloat>("resonance", //[0,1]
												"Resonance",
												NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.25f),
												0.1f),
			std::make_unique<AudioParameterFloat>("threshold",//dB
												"Threshold",
												-60.0f,
												0.0f,
												-30.0f),
			std::make_unique<AudioParameterFloat>("attack", //ms
												"Attack",
												NormalisableRange<float>(0.0f, 500.0f, 0.01f, 0.25f),
												50.0f),
			std::make_unique<AudioParameterFloat>("release", //ms
												"Release",
												NormalisableRange<float>(0.0f, 1000.0f, 0.01f, 0.25f),
												100.0f),
			std::make_unique<AudioParameterBool>("analog", //On/Off
												"Analog",
												true)
		})
	
#endif
{
	//set default filter parameters
	mAAfilter.setCutoffFrequencyHz(2500.0f);
	mAAfilter.setResonance(0.3f);
	mAAfilter.setMode(dsp::LadderFilter<float>::Mode::LPF24);
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
	//get environment variables
	mTotalNumInputChannels = getTotalNumInputChannels();
	mTotalNumOutputChannels = getTotalNumOutputChannels();
	dsp::ProcessSpec spec{ sampleRate, static_cast<uint32>(samplesPerBlock), static_cast<uint32>(mTotalNumInputChannels) };
	
	//mEchoProcessor
	mEchoProcessor.prepare(spec);
	
	//mAAfilter
	mAAfilter.prepare(spec);
	
	//mDynamicWaveshaper
	mDynamicWaveshaper.prepare(spec);
}

void DlayAudioProcessor::releaseResources()
{
	mAAfilter.reset();
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

	//clear data
	for (auto i = mTotalNumInputChannels; i < mTotalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	//process
	mEchoProcessor.fillDelayBuffer(buffer);
	if (mAnalog)
	{
		dsp::ProcessContextReplacing<float> writeBlock(*(mEchoProcessor.mWriteBlock));
		mAAfilter.process(writeBlock);
		mDynamicWaveshaper.process(writeBlock); //place after LPF to prevent aliasing from harmonic generation
	}
	mEchoProcessor.getFromDelayBuffer(buffer);
}



//==============================================================================
bool DlayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* DlayAudioProcessor::createEditor()
{
    return new DlayAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void DlayAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	auto state = parameters.copyState();
	std::unique_ptr<XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void DlayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState.get() != nullptr)
		if (xmlState->hasTagName(parameters.state.getType()))
			parameters.replaceState(ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DlayAudioProcessor();
}

void DlayAudioProcessor::setAnalog(bool onOffAnalog) noexcept
{
	mAnalog = onOffAnalog;
}