#include "../../../juce_code/juce/juce.h"
#include "ext.h"
#include "z_dsp.h"

void *jucemsp_class;

extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();

typedef struct _jucemsp
{
    t_pxobject x_obj;
	
	AudioProcessor* juceAudioProcessor;
	float* inputChannels[32]; // need to make this dynamic
	float* outputChannels[32]; // need to make this dynamic
	
	AudioSampleBuffer bufferSpace;
	
	MidiBuffer midiEvents;
		
} t_jucemsp;

t_int *jucemsp_perform(t_int *w);
void jucemsp_float(t_jucemsp *x, double f);
void jucemsp_int(t_jucemsp *x, long n);
void jucemsp_dsp(t_jucemsp *x, t_signal **sp, short *count);
void jucemsp_assist(t_jucemsp *x, void *b, long m, long a, char *s);
void *jucemsp_new(double val);
void jucemsp_free(t_jucemsp *x);


int main(void)
{
	setup((t_messlist **)&jucemsp_class, (method)jucemsp_new, (method)jucemsp_free, (short)sizeof(t_jucemsp), 0L, A_DEFFLOAT, 0);
	dsp_initclass();
	addmess((method)jucemsp_dsp, "dsp", A_CANT, 0);
	addfloat((method)jucemsp_float);
	addint((method)jucemsp_int);
	addmess((method)jucemsp_assist,"assist",A_CANT,0);
	
	return 0;
}

t_int *jucemsp_perform(t_int *w)
{
	t_jucemsp *x = (t_jucemsp *)(w[1]);
	int numSamples = (int)(w[2]);
	
	if (x->x_obj.z_disabled)
		return (w+3);
		
	// iterate through input channels
	for(int i = 0; i < x->juceAudioProcessor->getNumInputChannels(); i++) {
		x->bufferSpace.copyFrom(i, 0, x->inputChannels[i], numSamples);
	}
	
	x->juceAudioProcessor->processBlock(x->bufferSpace, x->midiEvents);
	
	AudioSampleBuffer outputBuffer(x->outputChannels, x->juceAudioProcessor->getNumOutputChannels(), numSamples);
	
	// need to iterate through output channels
	for(int i = 0; i < x->juceAudioProcessor->getNumOutputChannels(); i++) {
		outputBuffer.copyFrom(i, 0, x->bufferSpace, i, 0, numSamples);
	}
	
    return (w+3);
}



void jucemsp_dsp(t_jucemsp *x, t_signal **sp, short *count)
{
	dsp_add(jucemsp_perform, 2, x, sp[0]->s_n);
	
	post("input=%p output=%p", sp[0]->s_vec, sp[1]->s_vec);
	
	x->juceAudioProcessor->setPlayConfigDetails(1, // num inputs
												1, // num outputs
												sp[0]->s_sr, sp[0]->s_n);
	// setup our intermediate buffers
	x->bufferSpace.setSize(jmax(x->juceAudioProcessor->getNumInputChannels(), 
								x->juceAudioProcessor->getNumOutputChannels()),
						   sp[0]->s_n);
	
	// keep a record of an array of our output channels 
	x->inputChannels[0] = sp[0]->s_vec;
	
	// keep a record of an array of our output channels 
	x->outputChannels[0] = sp[1]->s_vec;
	
	x->juceAudioProcessor->prepareToPlay(sp[0]->s_sr, sp[0]->s_n);
}


// this routine covers both inlets. It doesn't matter which one is involved

void jucemsp_float(t_jucemsp *x, double f)
{
	x->juceAudioProcessor->setParameter(0, f);
}

void jucemsp_int(t_jucemsp *x, long n)
{
	jucemsp_float(x,(double)n);
}

void jucemsp_assist(t_jucemsp *x, void *b, long m, long a, char *s)
{
//	if (m==ASSIST_INLET) {
//		switch (a) {
//			case 0: sprintf(s,"(signal/float) This + Right Inlet"); break;
//			case 1: sprintf(s,"(signal/float) Left Inlet + This"); break;
//		}
//	}
//	else {
//		sprintf(s,"(signal) Addition Result");
//	}
}

void *jucemsp_new(double val)
{
    t_jucemsp *x = (t_jucemsp *)newobject(jucemsp_class);
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_pxobject *)x, "signal");
	
	x->juceAudioProcessor = createPluginFilter();
	
	jucemsp_float(x, val);
		
    return (x);
}

void jucemsp_free(t_jucemsp *x)
{
	dsp_free((t_pxobject*)x);
	delete x->juceAudioProcessor;
}



