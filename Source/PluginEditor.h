#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class CrossFeedAudioProcessorEditor : public AudioProcessorEditor, public Slider::Listener, public Button::Listener
{
public:
	CrossFeedAudioProcessorEditor(CrossFeedAudioProcessor&);
	~CrossFeedAudioProcessorEditor();

	//==============================================================================
	void paint(Graphics&) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	CrossFeedAudioProcessor& processor;

	Slider gainSlider;
	Label gainLabel;

	Slider xGainSlider;
	Label xGainLabel;

	Slider angleSlider;
	Label angleLabel;

	ToggleButton bypassButton;

	void sliderValueChanged(Slider* ) override;
	void buttonStateChanged(Button* ) override;
	void buttonClicked(Button* ) override {};

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossFeedAudioProcessorEditor)
};
