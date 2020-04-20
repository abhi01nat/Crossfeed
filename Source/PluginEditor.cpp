#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CrossFeedAudioProcessorEditor::CrossFeedAudioProcessorEditor (CrossFeedAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // editor size
    setSize (550, 180);

    // gain slider params
    addAndMakeVisible (&gainSlider);
    gainSlider.setSliderStyle (Slider::LinearHorizontal);
    gainSlider.setRange (processor.minGaindB, processor.maxGaindB);
    gainSlider.setTextValueSuffix (" dB");
    gainSlider.setNumDecimalPlacesToDisplay (1);
    gainSlider.setValue (processor.defaultGaindB);
    gainSlider.setDoubleClickReturnValue (true, processor.defaultGaindB);
    gainSlider.addListener (this);

    // gain label
    addAndMakeVisible (&gainLabel);
    gainLabel.setText ("Gain", dontSendNotification);
    gainLabel.attachToComponent(&gainSlider, true);

    // crossfeed gain slider params
    addAndMakeVisible(&xGainSlider);
    xGainSlider.setSliderStyle(Slider::LinearHorizontal);
    xGainSlider.setRange(processor.minXGaindB, processor.maxXGaindB);
    xGainSlider.setTextValueSuffix(" dB");
    xGainSlider.setNumDecimalPlacesToDisplay(1);
    xGainSlider.setValue(processor.defaultXGaindB);
    xGainSlider.setDoubleClickReturnValue(true, processor.defaultXGaindB);
    xGainSlider.addListener(this);

    // crossfeed gain label
    addAndMakeVisible(&xGainLabel);
    xGainLabel.setText("Crossfeed", dontSendNotification);
    xGainLabel.attachToComponent(&xGainSlider, true);

    // angle slider params
    addAndMakeVisible(&angleSlider);
    angleSlider.setSliderStyle(Slider::LinearHorizontal);
    angleSlider.setRange(processor.minAngle, processor.maxAngle, 1);
    angleSlider.setNumDecimalPlacesToDisplay(0);
    angleSlider.setTextValueSuffix(" deg");
    angleSlider.setValue(processor.defaultAngle);
    angleSlider.setDoubleClickReturnValue(true, processor.defaultAngle);
    angleSlider.addListener(this);

    // angle label
    addAndMakeVisible(&angleLabel);
    angleLabel.setText("Angle", dontSendNotification);
    angleLabel.attachToComponent(&angleSlider, true);

    // bypass button
    addAndMakeVisible(bypassButton);
    bypassButton.setButtonText("Bypass");
    bypassButton.addListener(this);
}

CrossFeedAudioProcessorEditor::~CrossFeedAudioProcessorEditor()
{
}

//==============================================================================
void CrossFeedAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (15.0f);
    //g.drawFittedText ("CrossFeed", getLocalBounds(), Justification::centred, 1);
}

void CrossFeedAudioProcessorEditor::resized()
{
    auto left = 80;
    gainSlider.setBounds(left, 20, getWidth() - left - 10, 20);
    xGainSlider.setBounds(left, 50, getWidth() - left - 10, 20);
    angleSlider.setBounds(left, 80, getWidth() - left - 10, 20);
    bypassButton.setBounds(left, 110, 120, 20);
}

void CrossFeedAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
    if (slider == &gainSlider)
    {
        *processor.gaindB = gainSlider.getValue();
    }
    else if (slider == &xGainSlider)
    {
        *processor.xGaindB = xGainSlider.getValue();
    }
    else
    {
        *processor.angle = angleSlider.getValue();
    }
}

void CrossFeedAudioProcessorEditor::buttonStateChanged(Button* )
{
    *processor.bypass = bypassButton.getToggleState();
}
