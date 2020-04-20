#pragma once

#include <JuceHeader.h>
#include "Delay.h"

//==============================================================================
/**
*/
class CrossFeedAudioProcessor : public AudioProcessor
{
	using MultiChannelIIRFilter = dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>>;
	using MultiChannelFIRFilter = dsp::ProcessorDuplicator <dsp::FIR::Filter <float>, dsp::FIR::Coefficients<float>>;

public:
	//==============================================================================
	//Constructor and destructor
	CrossFeedAudioProcessor ();
	~CrossFeedAudioProcessor ();

	//==============================================================================
	// Main processing
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources () override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	void processBlockBypassed (AudioBuffer<float>&, MidiBuffer&) override;

	//==============================================================================
	// Create or check GUI
	AudioProcessorEditor* createEditor () override;
	bool hasEditor () const override;

	//==============================================================================
	// Utility functions for DAW
	const String getName () const override;

	bool acceptsMidi () const override;
	bool producesMidi () const override;
	bool isMidiEffect () const override;
	double getTailLengthSeconds () const override;
	int getNumPrograms () override;
	int getCurrentProgram () override;
	void setCurrentProgram (int index) override;
	const String getProgramName (int index) override;
	void changeProgramName (int index, const String& newName) override;
	AudioProcessorParameter* getBypassParameter () const override;

	//==============================================================================
	void getStateInformation (MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;

	// User editable parameters
	AudioParameterFloat* gaindB;
	AudioParameterFloat* xGaindB;
	AudioParameterFloat* angle;
	AudioParameterBool* bypass;

	// default parameters
	static constexpr float defaultGaindB { 0.0f };
	static constexpr float maxGaindB { 6.0f };
	static constexpr float minGaindB { -12.0f };

	static constexpr float defaultXGaindB { -4.5f };
	static constexpr float maxXGaindB { 0.0f };
	static constexpr float minXGaindB { -9.0f };

	static constexpr float defaultAngle { 60.0f };
	static constexpr float minAngle { 30.0f };
	static constexpr float maxAngle { 90.0f };

private:

	static constexpr float pi = MathConstants<float>::pi;

	// Output gain
	float gain { 1.0f };
	// Crossfeed gain before being added to input
	float xGain { 0.5f };
	// Normalisation factor to eliminate level change when crossfeed is added to input
	float normalise { 1.0f / std::sqrt (1.0f + xGain * xGain) };

	/* Parameters for delay */
	// Number of samples of delay
	size_t nsamps { 0 };
	// Interaural separation in seconds using speed of sound = 340 m/s and head width = 16cm
	static constexpr float headTime { static_cast<float> (0.0004705882352941176470588L) };

	/* Lowpass filter */
	// Cutoff frequency in Hz
	static constexpr float wc = { 700.0f };
	// Lowpass filter object.
	MultiChannelIIRFilter lpFilt;
	// Amount of delay compensation applied
	size_t lpDelay;

	/* Shelving filters for mid-side processing of output */
	dsp::IIR::Filter<float> midShelfFilt;
	dsp::IIR::Filter<float> sideShelfFilt;
	

	// Delay filter
	Delay<float> lpDelayComp, ITDFilt;
	// Minimum delay
	size_t minDelay;

	// Mid-side transcoder
	void stereoToMidSide (AudioBuffer<float> buffer);

	void inline updateParameters (float sampleRate);
	bool bypassReset = false; // when set to false the plugin has been bypassed and filters need to be reset

	// lookup tables for fast computation of functions
	static constexpr float inverseSqrtTwo { static_cast <float> (0.70710678118654752440L) };
	static constexpr float sqrtTwo { static_cast <float> (1.4142135623730950488L) };
	//dsp::LookupTableTransform<float> fastNormalise;
	dsp::LookupTableTransform<float> dBToMagnitude;
	dsp::LookupTableTransform<float> sinXByTwo;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrossFeedAudioProcessor)
};
