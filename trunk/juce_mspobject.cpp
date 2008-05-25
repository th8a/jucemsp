#include "../../../juce_code/juce/juce.h"
#include "JucePluginCharacteristics.h"
#include "ext.h"
#include "z_dsp.h"
#include "ext_strings.h"     // String Functions
#include "commonsyms.h"      // Common symbols used by the Max 4.5 API
#include "ext_obex.h"        // Max Object Extensions

t_class *jucemsp_class;

static const short channelConfigs[][2] = { JucePlugin_PreferredChannelConfigurations };
extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();

typedef struct _jucemsp
{
    t_pxobject x_obj;
	void        *obex;       // Pointer to Obex object
	
	AudioProcessor* juceAudioProcessor;
	float* inputChannels[32]; // need to make this dynamic
	float* outputChannels[32]; // need to make this dynamic
	
	AudioSampleBuffer bufferSpace;
	
	MidiBuffer midiEvents;
		
} t_jucemsp;

t_int *jucemsp_perform(t_int *w);
void jucemsp_list(t_jucemsp *x, t_symbol* s, short argc, t_atom* argv);
void jucemsp_dsp(t_jucemsp *x, t_signal **sp, short *count);
void jucemsp_assist(t_jucemsp *x, void *b, long m, long a, char *s);
void *jucemsp_new(void);
void jucemsp_free(t_jucemsp *x);


//int main(void)
//{
//	setup((t_messlist **)&jucemsp_class, (method)jucemsp_new, (method)jucemsp_free, (short)sizeof(t_jucemsp), 0L, 0);
//	dsp_initclass();
//	addmess((method)jucemsp_dsp, "dsp", A_CANT, 0);
//	addmess((method)jucemsp_list,"list",A_GIMME,0);
//	addmess((method)jucemsp_assist,"assist",A_CANT,0);
//	
//	return 0;
//}

int main(void)
{
	long attrflags = 0;
	t_class *c;
	t_object *attr;
	
	common_symbols_init();
	
	// Define our class
	c = class_new("juce_mspobject~",(method)jucemsp_new, (method)jucemsp_free, 
				  (short)sizeof(t_jucemsp), (method)0L, 
				  0); // no args
		
	class_obexoffset_set(c, calcoffset(t_jucemsp, obex));
	
	// Make methods accessible for our class: 
	class_addmethod(c, (method)jucemsp_dsp,          "dsp",			A_CANT, 0L);
	class_addmethod(c, (method)jucemsp_list,         "list",		A_GIMME, 0L);        
	class_addmethod(c, (method)jucemsp_assist,       "assist",		A_CANT, 0L); 
	class_addmethod(c, (method)object_obex_dumpout,  "dumpout",		A_CANT,0);  
	class_addmethod(c, (method)object_obex_quickref, "quickref",	A_CANT, 0);
	
	// Setup our class to work with MSP
	class_dspinit(c);
	
	// Finalize our class
	class_register(CLASS_BOX, c);
	jucemsp_class = c;
	
	
	
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
		
	// setup our intermediate buffers
	x->bufferSpace.setSize(jmax(x->juceAudioProcessor->getNumInputChannels(), 
								x->juceAudioProcessor->getNumOutputChannels()),
						   sp[0]->s_n);
	
	int signalIndex = 0;
	
	// keep a record of an array of our input channels
	for(int i = 0; i < x->juceAudioProcessor->getNumInputChannels(); i++, signalIndex++) {
		x->inputChannels[i] = sp[signalIndex]->s_vec;
	}
	
	// keep a record of an array of our output channels 
	for(int i = 0; i < x->juceAudioProcessor->getNumOutputChannels(); i++, signalIndex++) {
		x->outputChannels[i] = sp[signalIndex]->s_vec;
	}
	
	x->juceAudioProcessor->prepareToPlay(sp[0]->s_sr, sp[0]->s_n);
}
	
void jucemsp_list(t_jucemsp *x, t_symbol* s, short argc, t_atom* argv)
{
	if(argc != 2) return;
	if(argv[0].a_type != A_LONG) return;
	if(argv[1].a_type != A_FLOAT) return;
	
	int index = argv[0].a_w.w_long;
	
	if(index >= x->juceAudioProcessor->getNumParameters()) return;
	
	float value = argv[1].a_w.w_float;
	
	x->juceAudioProcessor->setParameter(index, value);
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


//void *jucemsp_new(void)
//{
//    t_jucemsp *x = (t_jucemsp *)newobject(jucemsp_class);
//	
//	x->juceAudioProcessor = createPluginFilter();
//	
//	// using the first (zeroth) channel configs from "JucePluginCharacteristics.h"
//	// must think of an elegant way to select these in MaxMSP
//	x->juceAudioProcessor->setPlayConfigDetails(channelConfigs[0][0], // num inputs
//												channelConfigs[0][1], // num outputs
//												sys_getsr(), sys_getblksize());
//	
//	// inputs
//    dsp_setup((t_pxobject *)x,  x->juceAudioProcessor->getNumInputChannels());
//	
//	// outputs
//	for(int i = 0; i < x->juceAudioProcessor->getNumOutputChannels(); i++) {
//		outlet_new((t_pxobject *)x, "signal");
//	}
//			
//    return (x);
//}

void *jucemsp_new(void)
{
	t_jucemsp *x;
	
	x = (t_jucemsp *)object_alloc(jucemsp_class);
	if(x){
		object_obex_store((void *)x, _sym_dumpout, 
						  (t_object *)outlet_new(x,NULL));
		
		x->juceAudioProcessor = createPluginFilter();
		
		// using the first (zeroth) channel configs from "JucePluginCharacteristics.h"
		// must think of an elegant way to select these in MaxMSP
		x->juceAudioProcessor->setPlayConfigDetails(channelConfigs[0][0], // num inputs
													channelConfigs[0][1], // num outputs
													sys_getsr(), sys_getblksize());
		
		// inputs
		dsp_setup((t_pxobject *)x,  x->juceAudioProcessor->getNumInputChannels());
		
		// outputs
		for(int i = 0; i < x->juceAudioProcessor->getNumOutputChannels(); i++) {
			outlet_new((t_pxobject *)x, "signal");
		}
		
	}
	
	return(x);                     // return pointer to the new instance 
}
	


void jucemsp_free(t_jucemsp *x)
{
	dsp_free((t_pxobject*)x);
	delete x->juceAudioProcessor;
}



