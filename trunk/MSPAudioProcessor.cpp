/*
 *  MSPAudioProcessor.cpp
 *  juce_mspobject
 *
 *  Created by Martin Robinson on 24/05/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "../../../juce_code/juce/juce.h"
#include "ext.h"
#include "MSPAudioProcessor.h"

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
    // amount that our volume parameter is set to.
//    for (int channel = 0; channel < getNumInputChannels(); ++channel)
//    {
//        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);
//    }
	
//    // in case we have more outputs than inputs, we'll clear any output
//    // channels that didn't contain input data, (because these aren't
//    // guaranteed to be empty - they may contain garbage).
//    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
//    {
//        buffer.clear (i, 0, buffer.getNumSamples());
//    }
	
    for (int channel = 0; channel < 1; ++channel)
    {
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);
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
			
			post("MSPAudioProcessor::setParameter %d = %f", index, newValue);
			
            //sendChangeMessage (this); // do we need to use this to send to MaxMSP outlets?
        }
    }
}

const String MSPAudioProcessor::getParameterName (int index)
{
	if (index == 0)
        return String (gain, 2);
	
    return String::empty;
}