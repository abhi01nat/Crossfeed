#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
//Constructor and destructor
CrossFeedAudioProcessor::CrossFeedAudioProcessor ()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor (BusesProperties ()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput ("Input", AudioChannelSet::stereo (), true)
#endif
		.withOutput ("Output", AudioChannelSet::stereo (), true)
#endif
	)
#endif
{
	addParameter (gaindB = new AudioParameterFloat ("GAIN", "Gain", { minGaindB, maxGaindB, 0.0f, 1.0f }, defaultGaindB, "dB"));
	addParameter (xGaindB = new AudioParameterFloat ("XGAIN", "Crossfeed Gain", { minXGaindB, maxXGaindB, 0.0f, 1.0f }, defaultXGaindB, "dB"));
	addParameter (angle = new AudioParameterFloat ("ANGLE", "Angle", { minAngle, maxAngle, 0.0f, 1.0f }, defaultAngle, "deg"));
	addParameter (bypass = new AudioParameterBool ("BYPASS", "Bypass", false));
	//fastNormalise.initialise ([](float x) { return 1.0f / std::sqrt (1.0f + x * x); }, 0.0f, 1.0f, 10000);
	dBToMagnitude.initialise ([](float x) { return std::pow (10.0f, x * 0.05f); }, -15.0f, 15.0f, 10000);
	sinXByTwo.initialise ([](float x) { return std::sin (pi * 0.005555555f * x * 0.5f); }, 30.0f, 90.0f, 10000); // 1/180 = 0.00555...
}

CrossFeedAudioProcessor::~CrossFeedAudioProcessor () {}

//==============================================================================
// Utility functions for DAW
const String CrossFeedAudioProcessor::getName () const { return JucePlugin_Name; }
bool CrossFeedAudioProcessor::acceptsMidi () const { return false; }
bool CrossFeedAudioProcessor::producesMidi () const { return false; }
bool CrossFeedAudioProcessor::isMidiEffect () const { return false; }
double CrossFeedAudioProcessor::getTailLengthSeconds () const { return 0.0; }
int CrossFeedAudioProcessor::getNumPrograms () { return 1; }
int CrossFeedAudioProcessor::getCurrentProgram () { return 0; }
void CrossFeedAudioProcessor::setCurrentProgram (int index) {}
const String CrossFeedAudioProcessor::getProgramName (int index) { return {}; }
void CrossFeedAudioProcessor::changeProgramName (int index, const String& newName) {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CrossFeedAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
	// check if input and output are enabled
	if (layouts.getMainInputChannelSet () == AudioChannelSet::disabled ()
		|| layouts.getMainOutputChannelSet () == AudioChannelSet::disabled ())
		return false;

	// only support stereo
	if (layouts.getMainOutputChannelSet () != AudioChannelSet::stereo ())
		return false;

	// check if input and output are both stereo
	if (layouts.getMainOutputChannelSet () != layouts.getMainInputChannelSet ())
		return false;

	return true;
}
#endif

AudioProcessorParameter* CrossFeedAudioProcessor::getBypassParameter () const { return bypass; }

//==============================================================================
// Main processing

void CrossFeedAudioProcessor::stereoToMidSide (AudioBuffer<float> buffer)
{
	jassert (buffer.getNumChannels () == 2);
	float* l = buffer.getWritePointer (0);
	float* r = buffer.getWritePointer (1);
	auto n = buffer.getNumSamples ();
	float temp;
	for (auto i = 0; i < n; ++i) {
		temp = (*l + *r) * inverseSqrtTwo;
		*r = (*l - *r) * inverseSqrtTwo;
		*l = temp;
		++l; ++r;
	}
}

void CrossFeedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	float Fs = float (sampleRate);

	// set up correct sampling rate, block size, and channel specifications for the filters
	auto channels = jmin (getMainBusNumInputChannels (), getMainBusNumOutputChannels ());
	dsp::ProcessSpec spec { sampleRate, static_cast<uint32>(samplesPerBlock), channels };

	// compute single-pole lowpass filter coefficients
	lpFilt.prepare (spec);
	float y = 1 - dsp::FastMathApproximations::cos (2.0f * pi * (wc / Fs));
	float a = -y + std::sqrt (y * y + y * 2.0f);
	*lpFilt.state = dsp::IIR::Coefficients<float> (a, 0, 1.0f, a - 1.0f);

	// delay compensation for the lowpass filter
	lpDelayComp.prepare (spec);
	minDelay = size_t (std::floor (sinXByTwo (minAngle) * headTime * Fs));
	lpDelay = size_t (std::floor (1.0f / a - 1.0f));
	lpDelayComp.setMaxDelayInSamples (lpDelay);
	lpDelayComp.setDelayInSamples (lpDelay);
	setLatencySamples (lpDelay);

	// delay filter 
	ITDFilt.prepare (spec);
	ITDFilt.setMaxDelayInSamples (size_t (std::floor (headTime* Fs)));

	// Mid and side shelf set-up
	midShelfFilt.prepare (spec);
	sideShelfFilt.prepare (spec);
	*midShelfFilt.coefficients = dsp::IIR::Coefficients<float>::Coefficients (1, 0, 1, 0);
	*sideShelfFilt.coefficients = dsp::IIR::Coefficients<float>::Coefficients (1, 0, 1, 0);
}

void inline CrossFeedAudioProcessor::updateParameters (float sampleRate)
{
	//update gain parameters
	gain = dBToMagnitude (*gaindB);
	xGain = dBToMagnitude (*xGaindB);

	// update delay amount and delay filter
	nsamps = size_t (std::floor (sinXByTwo (*angle) * headTime * sampleRate)); // 1/180 = 0.00555...
	ITDFilt.setDelayInSamples (nsamps);

	// update shelving filter coefficients
	float g = dBToMagnitude (*xGaindB-2);
	float a = lpFilt.state->getRawCoefficients ()[0];
	*midShelfFilt.coefficients = dsp::IIR::Coefficients<float>::Coefficients (1.0f, (a - 1.0f), 1.0f + g * a, a - 1.0f);
	g = dBToMagnitude (*xGaindB-6);
	*sideShelfFilt.coefficients = dsp::IIR::Coefficients<float>::Coefficients (1.0f, (a - 1.0f), 1.0f - g * a, a - 1.0f);
}

void CrossFeedAudioProcessor::releaseResources ()
{
	lpFilt.reset ();
	lpDelayComp.reset ();
	ITDFilt.reset ();
	midShelfFilt.reset ();
	sideShelfFilt.reset ();
}

void CrossFeedAudioProcessor::processBlock (AudioBuffer<float>& ioBuffer, MidiBuffer&)
{
	ScopedNoDenormals noDenormals;
	int numSamples = ioBuffer.getNumSamples ();

	// update shelving and delay filter parameters
	updateParameters (static_cast<float> (getSampleRate ()));

	// apply delay compensation to main signal 
	dsp::AudioBlock<float> ioBlock (ioBuffer);
	lpDelayComp.process (dsp::ProcessContextReplacing<float> (ioBlock));

	// store crossfeed into an auxilliary buffer
	auto auxBuffer = AudioBuffer<float> (2, numSamples);
	auxBuffer.copyFrom (0, 0, ioBuffer, 1, 0, numSamples);
	auxBuffer.copyFrom (1, 0, ioBuffer, 0, 0, numSamples);

	// lowpass and delay the crossfeed
	dsp::AudioBlock<float> auxBlock (auxBuffer);
	lpFilt.process (dsp::ProcessContextReplacing<float> (auxBlock));
	ITDFilt.process (dsp::ProcessContextReplacing<float> (auxBlock));
	auxBuffer.applyGain (xGain);	

	// add the crossfeed to the main signal
	ioBlock.add (auxBlock);
	//ioBlock.multiplyBy (normalise);

	// mid side processing on the output signal
	stereoToMidSide (ioBuffer);
	midShelfFilt.process (dsp::ProcessContextReplacing<float> (ioBlock.getSingleChannelBlock (0)));
	sideShelfFilt.process (dsp::ProcessContextReplacing<float> (ioBlock.getSingleChannelBlock (1)));
	stereoToMidSide (ioBuffer);

	// output gain adjustment
	ioBlock.multiplyBy (gain);
	bypassReset = false;
}

void CrossFeedAudioProcessor::processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer&)
{
	if (!bypassReset)
	{
		lpFilt.reset ();
		ITDFilt.reset ();
		lpDelayComp.reset ();
		midShelfFilt.reset ();
		sideShelfFilt.reset ();
		bypassReset = true;
	}
	return;
}

//==============================================================================
// Create or check GUI
bool CrossFeedAudioProcessor::hasEditor () const { return true; }
AudioProcessorEditor* CrossFeedAudioProcessor::createEditor () { return new CrossFeedAudioProcessorEditor (*this); }

//==============================================================================
void CrossFeedAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void CrossFeedAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin.
AudioProcessor* JUCE_CALLTYPE createPluginFilter () { return new CrossFeedAudioProcessor (); }
