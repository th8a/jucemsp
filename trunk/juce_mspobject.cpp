#include "../../../juce_code/juce/juce.h"
#include "JucePluginCharacteristics.h"
#include "ext.h"
#include "z_dsp.h"
#include "ext_strings.h"     // String Functions
#include "commonsyms.h"      // Common symbols used by the Max 4.5 API
#include "ext_obex.h"        // Max Object Extensions

#define VALIDCHARS "ABCDEFGHIJKLMNOPQRTSUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789"

t_class *jucemsp_class;

static const short channelConfigs[][2] = { JucePlugin_PreferredChannelConfigurations };
static const int numChannelConfigs = numElementsInArray (channelConfigs);
extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#pragma mark -
#pragma mark t_jucemsp definition

typedef struct _jucemsp
{
    t_pxobject	x_obj;
	void        *obex;       // Pointer to Obex object
	
	t_symbol*	name;
	
	AudioProcessor* juceAudioProcessor;
	class JuceMSPListener* juceAudioProcessorListener;
	
	float* inputChannels[32]; // need to make this dynamic
	float* outputChannels[32]; // need to make this dynamic
	
	AudioSampleBuffer bufferSpace;
	
	MidiBuffer midiEvents;
	
	float params[1];
	//.. extra space will be alloacted for the number of params
} t_jucemsp;

#pragma mark -
#pragma mark t_jucemsp function prototypes 

void jucemsp_initattrs(t_class *c, AudioProcessor* juceAudioProcessor);
t_int *jucemsp_perform(t_int *w);
void jucemsp_list(t_jucemsp *x, t_symbol* s, short argc, t_atom* argv);
void jucemsp_updateattr(t_jucemsp *x, int parameterIndex, float newValue);
t_max_err jucemsp_attr_get(t_jucemsp *x, void *attr, long *argc, t_atom **argv); 
t_max_err jucemsp_attr_set(t_jucemsp *x, void *attr, long argc, t_atom *argv); 
void jucemsp_dsp(t_jucemsp *x, t_signal **sp, short *count);
void jucemsp_assist(t_jucemsp *x, void *b, long m, long a, char *s);
void *jucemsp_new(t_symbol *s, long argc, t_atom *argv);
void jucemsp_free(t_jucemsp *x);

#pragma mark -
#pragma mark JuceMSPListener class 

class JuceMSPListener : public AudioProcessorListener
{
public:
	JuceMSPListener(t_jucemsp* x) 
	:	AudioProcessorListener(),
		juceMsp(x)
	{
	}
	void audioProcessorParameterChanged (AudioProcessor *processor, int parameterIndex, float newValue)
	{
		jucemsp_updateattr(juceMsp, parameterIndex, newValue);
	}
	
	void audioProcessorChanged (AudioProcessor *processor) { }
	
private:
	t_jucemsp* juceMsp; 
};

#pragma mark -
#pragma mark t_jucemsp function definitions 

int main(void)
{
	long attrflags = 0;
	t_class *c;
	t_object *attr;
	
	// a dummy so we can inspect it (it won't know the channel configs but we don't need that here)
	AudioProcessor* juceAudioProcessor = createPluginFilter(); 
	
	common_symbols_init();
	
	// Define our class
	String className;
	
	className << String(JucePlugin_Name).replaceCharacter (' ', '_')
										.replaceCharacter ('.', '_')
										.retainCharacters (T(VALIDCHARS));
	className << "~";
	
	post("Juce MSP Wrapper: '%s' plugin loaded as '%s'", JucePlugin_Name, (char*)(const char*)className);
	
	c = class_new((char*)(const char*)className,(method)jucemsp_new, (method)jucemsp_free, 
				  // vaiable size float array for the last member of the struct
				  (short)sizeof(t_jucemsp) + ((juceAudioProcessor->getNumParameters()-1) * sizeof(float)), 
				  (method)0L, 
				  A_GIMME, 0);
		
	class_obexoffset_set(c, calcoffset(t_jucemsp, obex));
	
	// Make methods accessible for our class: 
	class_addmethod(c, (method)jucemsp_dsp,          "dsp",			A_CANT, 0L);
	class_addmethod(c, (method)jucemsp_list,         "list",		A_GIMME, 0L);        
	class_addmethod(c, (method)jucemsp_assist,       "assist",		A_CANT, 0L); 
	class_addmethod(c, (method)object_obex_dumpout,  "dumpout",		A_CANT,0);  
	class_addmethod(c, (method)object_obex_quickref, "quickref",	A_CANT, 0);
	
	// generate some attrs from the plugin params
	jucemsp_initattrs(c, juceAudioProcessor);
	
	// Setup our class to work with MSP
	class_dspinit(c);
	
	// Finalize our class
	class_register(CLASS_BOX, c);
	jucemsp_class = c;
	
	
	delete juceAudioProcessor; // delete the dummy
	return 0;
}

void jucemsp_initattrs(t_class *c, AudioProcessor* juceAudioProcessor)
{
	long attrflags = 0;
	t_object *attr;
	
	attr = attr_offset_new("name", 
						   _sym_symbol, ATTR_SET_OPAQUE_USER,
						   (method)0L, (method)0L, 
						   calcoffset(t_jucemsp, name)); 
	class_addattr(c, attr);
	
	for(int i = 0; i < juceAudioProcessor->getNumParameters(); i++) {		
		attr = attr_offset_new((char*)(const char*)
							   (juceAudioProcessor->getParameterName(i).replaceCharacter (' ', '_')
																	   .replaceCharacter ('.', '_')
																	   .retainCharacters (T(VALIDCHARS))), 
							   _sym_float32, attrflags,
							   (method)jucemsp_attr_get, (method)jucemsp_attr_set, 
							   calcoffset(t_jucemsp, params) + sizeof(float) * i); 
		class_addattr(c, attr);
	}
	
	
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
	
	float value = jlimit(0.f, 1.f, argv[1].a_w.w_float);
	
	x->juceAudioProcessor->setParameterNotifyingHost(index, value);
}

t_max_err jucemsp_attr_get(t_jucemsp *x, void *attr, long *ac, t_atom **av)
{	
	if (*ac && *av) {
		// memory passed in; use it 
	} else { 
		*ac = 1; // size of attr data 
		*av = (t_atom *)getbytes(sizeof(t_atom) * (*ac)); 
		if (!(*av)) { 
			*ac = 0; 
			return MAX_ERR_OUT_OF_MEM; 
		} 
	} 
	
	t_symbol *attrName = (t_symbol*)object_method(attr, _sym_getname); 
	
	for(int i = 0; i < x->juceAudioProcessor->getNumParameters(); i++) {
		
		t_symbol *paramName = gensym((char*)(const char*)
									 (x->juceAudioProcessor->getParameterName(i).replaceCharacter (' ', '_')
																				.replaceCharacter ('.', '_')
																				.retainCharacters (T(VALIDCHARS))));
		
		if(attrName == paramName) {
			atom_setfloat(*av, x->params[i]);
			return MAX_ERR_NONE;
		}
		
	}
	
	return MAX_ERR_GENERIC; 
}

t_max_err jucemsp_attr_set(t_jucemsp *x, void *attr, long ac, t_atom *av)
{	
	if (ac && av) { 
		t_symbol *attrName = (t_symbol*)object_method(attr, _sym_getname); 
		
		for(int i = 0; i < x->juceAudioProcessor->getNumParameters(); i++) {
			
			t_symbol *paramName = gensym((char*)(const char*)
										 (x->juceAudioProcessor->getParameterName(i).replaceCharacter (' ', '_')
																					.replaceCharacter ('.', '_')
																					.retainCharacters (T(VALIDCHARS))));
			
			if(attrName == paramName) {
				x->params[i] = jlimit(0.f, 1.f, atom_getfloat(av));
				x->juceAudioProcessor->setParameter(i,x->params[i]);
				
				return MAX_ERR_NONE;
			}
			
		}
		
	} 
	return MAX_ERR_GENERIC; 
}


void jucemsp_updateattr(t_jucemsp *x, int parameterIndex, float newValue)
{	
	x->params[parameterIndex] = newValue;
}


void jucemsp_assist(t_jucemsp *x, void *b, long inletOrOutlet, long inletOutletIndex, char *s)
{
	if (inletOrOutlet == ASSIST_INLET) {
		
		if(inletOutletIndex < x->juceAudioProcessor->getNumInputChannels()) {
			if(inletOutletIndex == 0)
				sprintf(s,"(signal) Input %s, and messages", 
						(char*)(const char*)x->juceAudioProcessor->getInputChannelName(inletOutletIndex));
			else
				sprintf(s,"(signal) Input %s", 
						(char*)(const char*)x->juceAudioProcessor->getInputChannelName(inletOutletIndex));
		} else {
			sprintf(s,"Message inlet %ld", inletOrOutlet+1);
		}
			
	} else {
		if(inletOutletIndex < x->juceAudioProcessor->getNumOutputChannels()) {
			sprintf(s,"(signal) Output %s", 
					(char*)(const char*)x->juceAudioProcessor->getOutputChannelName(inletOutletIndex));
		} else {
			sprintf(s,"Message outlet %ld", inletOrOutlet+1);
		}
	}
}


void *jucemsp_new(t_symbol *s, long argc, t_atom *argv)
{
	t_jucemsp *x;
	
	x = (t_jucemsp *)object_alloc(jucemsp_class);
	if(x) {
		object_obex_store((void *)x, _sym_dumpout, (t_object *)outlet_new(x,NULL));
		
		x->juceAudioProcessor = createPluginFilter();
		x->juceAudioProcessorListener = new JuceMSPListener(x);
		x->juceAudioProcessor->addListener(x->juceAudioProcessorListener);
		
		x->name = gensym((char*)(const char*)x->juceAudioProcessor->getName());
		
		int channelConfigIndex = 0;
		int attrArgsOffset = attr_args_offset(argc, argv);
		
		if(argc > 0 && attrArgsOffset > 0) {
			channelConfigIndex = jlimit(0, numChannelConfigs-1, (int)atom_getlong(argv));
		}
		
		x->juceAudioProcessor->setPlayConfigDetails(channelConfigs[channelConfigIndex][0], // num inputs
													channelConfigs[channelConfigIndex][1], // num outputs
													sys_getsr(), sys_getblksize());
		
		// inputs
		dsp_setup((t_pxobject *)x,  x->juceAudioProcessor->getNumInputChannels());
		
		// outputs
		for(int i = 0; i < x->juceAudioProcessor->getNumOutputChannels(); i++) {
			outlet_new((t_pxobject *)x, "signal");
		}
		
		attr_args_process(x,argc,argv);		// Handle attribute args
	}
	
	return(x);								// return pointer to the new instance 
}
	


void jucemsp_free(t_jucemsp *x)
{
	dsp_free((t_pxobject*)x);
	x->juceAudioProcessor->removeListener(x->juceAudioProcessorListener);
	delete x->juceAudioProcessorListener;
	delete x->juceAudioProcessor;
}



