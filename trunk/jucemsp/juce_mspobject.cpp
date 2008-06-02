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
	t_wind*		window;
	bool		windowIsVisible;
	
	AudioProcessor*					juceAudioProcessor;
	Component*						juceWindowComp;
	AudioProcessorEditor*			juceEditorComp;
	class JuceMSPListener*			juceListener;
	
	float* inputChannels[32]; // need to make this dynamic
	float* outputChannels[32]; // need to make this dynamic
	
	AudioSampleBuffer* bufferSpace;
	
	MidiBuffer* midiEvents;
	
	float params[1];
	//.. extra space will be alloacted for the number of params
} t_jucemsp;

#pragma mark -
#pragma mark t_jucemsp function prototypes 

void jucemsp_setupattrs(t_class *c, AudioProcessor* juceAudioProcessor);
t_int *jucemsp_perform(t_int *w);
void jucemsp_midievent(t_jucemsp *x, int byte1, int byte2, int byte3);
void jucemsp_list(t_jucemsp *x, t_symbol* s, short argc, t_atom* argv);
void jucemsp_initparameterattrs(t_jucemsp *x);
void jucemsp_updateattr(t_jucemsp *x, int parameterIndex, float newValue);
t_max_err jucemsp_attr_get(t_jucemsp *x, void *attr, long *argc, t_atom **argv); 
t_max_err jucemsp_attr_set(t_jucemsp *x, void *attr, long argc, t_atom *argv); 
void jucemsp_dsp(t_jucemsp *x, t_signal **sp, short *count);
void jucemsp_dblclick(t_jucemsp *x);
void jucemsp_update(t_jucemsp *x);
void jucemsp_invis(t_jucemsp *x); 
void jucemsp_assist(t_jucemsp *x, void *b, long m, long a, char *s);
void *jucemsp_new(t_symbol *s, long argc, t_atom *argv);
void jucemsp_deleteui(t_jucemsp *x);
void jucemsp_free(t_jucemsp *x);

#pragma mark -
#pragma mark Juce helper classes

class JuceMSPListener :		public AudioProcessorListener,
							public ComponentListener
{
public:
	JuceMSPListener(t_jucemsp* x) 
	:	AudioProcessorListener(),
		juceMsp(x),
		recursive(false)
	{
	}
	void audioProcessorParameterChanged (AudioProcessor *processor, int parameterIndex, float newValue)
	{
		jucemsp_updateattr(juceMsp, parameterIndex, newValue);
	}
	
	void audioProcessorChanged (AudioProcessor *processor) { }
	
	void componentMovedOrResized (Component& component,
                                  bool wasMoved,
                                  bool wasResized)
    {
        if (! recursive)
        {
            recursive = true;
			
            if (juceMsp->juceEditorComp != 0 && wasResized)
            {
                const int w = jmax (32, juceMsp->juceEditorComp->getWidth());
                const int h = jmax (32, juceMsp->juceEditorComp->getHeight());
				
				syswindow_size(wind_syswind(juceMsp->window), w, h, true);
				
                if (juceMsp->juceWindowComp->getWidth() != w
					|| juceMsp->juceWindowComp->getHeight() != h)
                {
                    juceMsp->juceWindowComp->setSize (w, h);
                }
				
                juceMsp->juceEditorComp->repaint();
            }
			
            recursive = false;
        }
    }
	
private:
	t_jucemsp* juceMsp; 
	bool recursive;
};

class EditorComponentHolder  : public Component
{
public:
    EditorComponentHolder (Component* const editorComp)
    {
        addAndMakeVisible (editorComp);
        setOpaque (true);
        setVisible (true);
        setBroughtToFrontOnMouseClick (true);
		
//#if ! JucePlugin_EditorRequiresKeyboardFocus
//        setWantsKeyboardFocus (false);
//#endif
    }
	
    ~EditorComponentHolder()
    {
    }
	
    void resized()
    {
        if (getNumChildComponents() > 0)
            getChildComponent (0)->setBounds (0, 0, getWidth(), getHeight());
    }
	
    void paint (Graphics& g)
    {
		//post("EditorComponentHolder::paint");
    }
	
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
	
	initialiseJuce_GUI();
	
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
	class_addmethod(c, (method)jucemsp_midievent,	 "midievent",	A_LONG, A_LONG, A_LONG, 0);
	class_addmethod(c, (method)jucemsp_list,         "list",		A_GIMME, 0L);
	class_addmethod(c, (method)jucemsp_dblclick,	 "dblclick",	A_CANT, 0); 
	class_addmethod(c, (method)jucemsp_update,		 "update",		A_CANT, 0);
	class_addmethod(c, (method)jucemsp_invis,		 "invis",		A_CANT, 0); 
	class_addmethod(c, (method)jucemsp_assist,       "assist",		A_CANT, 0L); 
	class_addmethod(c, (method)object_obex_dumpout,  "dumpout",		A_CANT,0);  
	class_addmethod(c, (method)object_obex_quickref, "quickref",	A_CANT, 0);
	
	// generate some attrs from the plugin params
	jucemsp_setupattrs(c, juceAudioProcessor);
	
	// Setup our class to work with MSP
	class_dspinit(c);
	
	// Finalize our class
	class_register(CLASS_BOX, c);
	jucemsp_class = c;
	
	
	delete juceAudioProcessor; // delete the dummy
	return 0;
}

void jucemsp_setupattrs(t_class *c, AudioProcessor* juceAudioProcessor)
{
	long attrflags = 0;
	t_object *attr;
	
	// plugin name attribute
	attr = attr_offset_new("pluginname", 
						   _sym_symbol, ATTR_SET_OPAQUE_USER,
						   (method)0L, (method)0L, 
						   calcoffset(t_jucemsp, name)); 
	class_addattr(c, attr);
	
	
	// plugin parameter attributes
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
		x->bufferSpace->copyFrom(i, 0, x->inputChannels[i], numSamples);
	}
	
	x->juceAudioProcessor->processBlock(*x->bufferSpace, *x->midiEvents);

//	 midioutput...
//		if (! midiEvents.isEmpty())
//		{
//	#if JucePlugin_ProducesMidiOutput
//			const JUCE_NAMESPACE::uint8* midiEventData;
//			int midiEventSize, midiEventPosition;
//			MidiBuffer::Iterator i (midiEvents);
//			
//			while (i.getNextEvent (midiEventData, midiEventSize, midiEventPosition))
//			{
//				jassert (((unsigned int) midiEventPosition) < (unsigned int) numSamples);
//				
//				
//				
//				//xxx
//			}
//	#else
	
	x->midiEvents->clear();
	
	AudioSampleBuffer outputBuffer(x->outputChannels, x->juceAudioProcessor->getNumOutputChannels(), numSamples);
	
	// need to iterate through output channels
	for(int i = 0; i < x->juceAudioProcessor->getNumOutputChannels(); i++) {
		outputBuffer.copyFrom(i, 0, *x->bufferSpace, i, 0, numSamples);
	}
	
    return (w+3);
}



void jucemsp_dsp(t_jucemsp *x, t_signal **sp, short *count)
{
	dsp_add(jucemsp_perform, 2, x, sp[0]->s_n);
		
	// setup our intermediate buffers
	x->bufferSpace->setSize(jmax(x->juceAudioProcessor->getNumInputChannels(), 
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
		
	x->midiEvents->clear();
	x->juceAudioProcessor->prepareToPlay(sp[0]->s_sr, sp[0]->s_n);
}
	
void jucemsp_midievent(t_jucemsp *x, int byte1, int byte2, int byte3)
{
#if JucePlugin_WantsMidiInput

	JUCE_NAMESPACE::uint8 data [4];
	data[0] = byte1;
	data[1] = byte2;
	data[2] = byte3;
	data[3] = 0;
		
	x->midiEvents->addEvent (data, 3, 0); 
	
#endif
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

void jucemsp_initparameterattrs(t_jucemsp *x)
{
	for(int i = 0; i < x->juceAudioProcessor->getNumParameters(); i++) {	
		x->params[i] = x->juceAudioProcessor->getParameter(i);
	}
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

#define WINDOWTITLEBARHEIGHT 22

void jucemsp_dblclick(t_jucemsp *x) 
{ 
	//post("jucemsp_dblclick");
	
	if(!x->windowIsVisible) {
		
		wind_vis(x->window);
		syswindow_hide(wind_syswind(x->window));
		
		x->juceEditorComp = x->juceAudioProcessor->createEditorIfNeeded();
		//x->juceEditorComp->setBounds(50, 50, 300, 100); don't do this, the editor should already be set
		
		const int w = x->juceEditorComp->getWidth();
		const int h = x->juceEditorComp->getHeight();
		
		x->juceEditorComp->setOpaque (true);
		x->juceEditorComp->setVisible (true);
		
		x->juceWindowComp = new EditorComponentHolder(x->juceEditorComp);
		x->juceWindowComp->setBounds(0, WINDOWTITLEBARHEIGHT, w, h);
		
		syswindow_size(wind_syswind(x->window), w, h, true);	
		
		int mx, my;
		Desktop::getMousePosition(mx,my);
		
		syswindow_move(wind_syswind(x->window), mx, my + WINDOWTITLEBARHEIGHT, false);
		
		// Mac only here...!
		HIViewRef hiRoot = HIViewGetRoot((WindowRef)wind_syswind(x->window));
		x->juceWindowComp->addToDesktop(0, (void*)hiRoot);
		
		x->windowIsVisible = true;
		syswindow_show(wind_syswind(x->window));
		
		x->juceEditorComp->addComponentListener(x->juceListener); // harmless to do this more than once...
	}
	
	syswindow_move(wind_syswind(x->window), x->window->w_x1, x->window->w_y1, true);

}

void jucemsp_update(t_jucemsp *x)
{
	//post("jucemsp_update");
	
	x->juceWindowComp->repaint();
}

void jucemsp_invis(t_jucemsp *x)
{
	//post("jucemsp_invis");
	
	x->windowIsVisible = false;
	delete x->juceWindowComp; // do this?
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
		
		x->bufferSpace = new AudioSampleBuffer(4, 64);
		
		x->juceAudioProcessor = createPluginFilter();
		x->juceListener	= new JuceMSPListener(x);
		x->juceAudioProcessor->addListener(x->juceListener);
		
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
		
		x->midiEvents = new MidiBuffer();
		//x->midiEvents->clear();
		
		jucemsp_initparameterattrs(x);
		
		x->juceEditorComp = 0L;
		x->juceWindowComp = 0L;
		x->window = wind_new (x, 50, 50, 150, 150, WCLOSE | WCOLOR); 
		wind_settitle(x->window, x->name->s_name, 0);
		
		x->windowIsVisible = false;
	}
	
	return(x);								// return pointer to the new instance 
}
	

void jucemsp_deleteui(t_jucemsp *x)
{
	PopupMenu::dismissAllActiveMenus();
	
	// there's some kind of component currently modal, but the host
	// is trying to delete our plugin..
//	jassert (Component::getCurrentlyModalComponent() == 0);
	
	if (x->juceEditorComp != 0) {
		x->juceEditorComp->removeComponentListener(x->juceListener);
		x->juceAudioProcessor->editorBeingDeleted (x->juceEditorComp);
	}
	
	deleteAndZero (x->juceEditorComp);
	deleteAndZero (x->juceWindowComp);
}

void jucemsp_free(t_jucemsp *x)
{
	dsp_free((t_pxobject*)x);
	
	wind_close(x->window);
	
	x->juceAudioProcessor->removeListener(x->juceListener);
	
	delete x->bufferSpace;
	delete x->midiEvents;
	delete x->juceListener;
	
	jucemsp_deleteui(x);
	
	wind_free((t_object*)x->window);
		
	delete x->juceAudioProcessor;
}



