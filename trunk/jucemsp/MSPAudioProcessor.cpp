/*
 *  MSPAudioProcessor.cpp
 *  juce_mspobject
 *
 *
 */

#include "../../../juce_code/juce/juce.h"
#include "ext.h"
#include "MSPAudioProcessor.h"
#include "MSPEditorComponent.h"


AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MSPAudioProcessor();
}

MSPAudioProcessor::MSPAudioProcessor()
{
	gain = 1.0f;
}

void MSPAudioProcessor::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
{
	// called by the MaxMSP peer dsp method (usually where dsp_add() is called)
}

void MSPAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	// for each of our input channels, we'll attenuate its level by the
    // amount that our gain parameter is set to.
    for (int channel = 0; channel < getNumInputChannels(); ++channel)
    {
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);
    }
	
    // in case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
	for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

int MSPAudioProcessor::getNumParameters()
{
	return 1;
}

float MSPAudioProcessor::getParameter (int index)
{
	return (index == 0) ? gain : 0.0f;
}

void MSPAudioProcessor::setParameter (int index, float newValue)
{
	if (index == 0) {
        if (gain != newValue) {
            gain = newValue;
			
            sendChangeMessage (this);
        }
    }
}

const String MSPAudioProcessor::getParameterName (int index)
{
	if (index == 0)
        return T("gain");
	
    return String::empty;
}

const String MSPAudioProcessor::getInputChannelName (const int channelIndex) const 
{ 
	return String (channelIndex + 1);
}

const String MSPAudioProcessor::getOutputChannelName (const int channelIndex) const 
{ 
	return String (channelIndex + 1); 
}

const String MSPAudioProcessor::getName() const 
{
	return T(JucePlugin_Name); 
}

AudioProcessorEditor* MSPAudioProcessor::createEditor()
{
	return new MSPEditorComponent(this);
}

