/*
 *  MSPAudioProcessor.h
 *  juce_mspobject
 *
 *
 */

// just a dummy test class, the idea is that you should be able to use any AudioProcessor subclass in the codebase as
// with the other plugin wrappers

#ifndef _MSPAUDIOPROCESSOR_H_
#define _MSPAUDIOPROCESSOR_H_

class MSPAudioProcessor  :	public AudioProcessor,
							public ChangeBroadcaster
{
public:
	MSPAudioProcessor();
	
	const String getName() const;// { return T("JucePlugin_Name"); }
	void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock);
	void releaseResources() { }
	void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
	const String getInputChannelName (const int channelIndex) const; //{ return String::empty; }
	const String getOutputChannelName (const int channelIndex) const;// { return String::empty; }
	bool isInputChannelStereoPair (int index) const { return false; }
	bool isOutputChannelStereoPair (int index) const { return false; }
	bool acceptsMidi() const { return JucePlugin_WantsMidiInput; }
	bool producesMidi() const { return JucePlugin_ProducesMidiOutput; }
	AudioProcessorEditor* createEditor() { return NULL; }
	int getNumParameters(); 
	const String getParameterName (int parameterIndex);
	float getParameter (int parameterIndex);
	const String getParameterText (int parameterIndex) { return T("0.0"); }
	void setParameter (int parameterIndex, float newValue);
	int getNumPrograms() { return 0; }
	int getCurrentProgram() { return 0; }
	void setCurrentProgram (int index) { }
	const String getProgramName (int index) { return String::empty; }
	void changeProgramName (int index, const String& newName) { }
	void getStateInformation (MemoryBlock& destData) { }
	void setStateInformation (const void* data, int sizeInBytes) { }
	
private:
	float gain;
};

#endif // _MSPAUDIOPROCESSOR_H_