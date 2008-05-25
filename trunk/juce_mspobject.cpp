#include "../../../juce_code/juce/juce.h"
#include "ext.h"
#include "z_dsp.h"

void *plus_class;

extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();

typedef struct _plus
{
    t_pxobject x_obj;
	
	AudioProcessor* juceAudioProcessor;
	float* outputChannels[32]; // need to make this dynamic
	
	AudioSampleBuffer bufferSpace;
	
	MidiBuffer midiEvents;
		
} t_plus;

t_int *offset_perform(t_int *w);
void plus_float(t_plus *x, double f);
void plus_int(t_plus *x, long n);
void plus_dsp(t_plus *x, t_signal **sp, short *count);
void plus_assist(t_plus *x, void *b, long m, long a, char *s);
void *plus_new(double val);
void plus_free(t_plus *x);


int main(void)
{
	setup((t_messlist **)&plus_class, (method)plus_new, (method)plus_free, (short)sizeof(t_plus), 0L, A_DEFFLOAT, 0);
	dsp_initclass();
	addmess((method)plus_dsp, "dsp", A_CANT, 0);
	addfloat((method)plus_float);
	addint((method)plus_int);
	addmess((method)plus_assist,"assist",A_CANT,0);
	
	return 0;
}

t_int *offset_perform(t_int *w)
{
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
	t_plus *x = (t_plus *)(w[3]);
	int n = (int)(w[4]);
	
//	if (x->x_obj.z_disabled)
//		goto out;
	
    //while (n--) *out++ = val + *in++; 
	
	//while (n--) *out++ = *in++; 
	
//	x->channels[0] = in;
//	x->channels[1] = out;
	
	//AudioSampleBuffer buffer(x->channels, 2, n);
	
	x->bufferSpace.copyFrom(0, 0, in, n);
	
	x->juceAudioProcessor->processBlock(x->bufferSpace, x->midiEvents);
	
	AudioSampleBuffer tempBuffer(x->outputChannels, x->juceAudioProcessor->getNumOutputChannels(), n);
	
	// iterate through output channels?
	
	tempBuffer.copyFrom(0, 0, x->bufferSpace, 0, 0, n);
	
	
out:
    return (w+5);
}



void plus_dsp(t_plus *x, t_signal **sp, short *count)
{
	dsp_add(offset_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
	
	post("input=%p output=%p", sp[0]->s_vec, sp[1]->s_vec);
	
	x->juceAudioProcessor->setPlayConfigDetails(1, // num inputs
												1, // num outputs
												sp[0]->s_sr, sp[0]->s_n);
	// setup our intermediate buffers
	x->bufferSpace.setSize(jmax(x->juceAudioProcessor->getNumInputChannels(), 
								x->juceAudioProcessor->getNumOutputChannels()),
						   sp[0]->s_n);
	
	// keep a record of an array of out output channels 
	x->outputChannels[0] = sp[1]->s_vec;
	
	x->juceAudioProcessor->prepareToPlay(sp[0]->s_sr, sp[0]->s_n);
}


// this routine covers both inlets. It doesn't matter which one is involved

void plus_float(t_plus *x, double f)
{
	//post("plus_float=%f", f);
	x->juceAudioProcessor->setParameter(0, f);
}

void plus_int(t_plus *x, long n)
{
	plus_float(x,(double)n);
}

void plus_assist(t_plus *x, void *b, long m, long a, char *s)
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

void *plus_new(double val)
{
    t_plus *x = (t_plus *)newobject(plus_class);
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_pxobject *)x, "signal");
	
	x->juceAudioProcessor = createPluginFilter();
	
	plus_float(x, val);
		
    return (x);
}

void plus_free(t_plus *x)
{
	dsp_free((t_pxobject*)x);
	delete x->juceAudioProcessor;
}



