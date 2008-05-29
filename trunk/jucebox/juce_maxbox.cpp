#include "../../../juce_code/juce/juce.h"
#include "ext.h"
#include "ext_common.h"
#include "commonsyms.h"      // Common symbols used by the Max 4.5 API
#include "ext_obex.h"

// Point conflicts with the Juce::Point, perhaps use a namespace for this
struct MaxMSPPoint {
	short               v;
	short               h;
};
typedef struct MaxMSPPoint	MaxMSPPoint;
typedef MaxMSPPoint *		MaxMSPPointPtr;

#define RES_ID			10120
#define MAXWIDTH 		1024L	
#define MAXHEIGHT		512L		//		(defines maximum offscreen canvas allocation)
#define MINWIDTH 		20L			// minimum width and height
#define MINHEIGHT		2L			//		...
#define DEFWIDTH 		300			// default width and height
#define DEFHEIGHT		150			//		...

/*
 juce_mac_Windowing.cpp lines 2008-23 have to be:
 
 EventTypeSpec viewEvents[] =
 {
 { kEventClassHIObject, kEventHIObjectConstruct },
 { kEventClassHIObject, kEventHIObjectInitialize },
 { kEventClassHIObject, kEventHIObjectDestruct },
 { kEventClassControl, kEventControlInitialize },
 { kEventClassControl, kEventControlDraw },
	 //                { kEventClassControl, kEventControlBoundsChanged },
	 //                { kEventClassControl, kEventControlSetFocusPart },
 { kEventClassControl, kEventControlHitTest },
	 //                { kEventClassControl, kEventControlDragEnter },
	 //                { kEventClassControl, kEventControlDragLeave },
	 //                { kEventClassControl, kEventControlDragWithin },
	 //                { kEventClassControl, kEventControlDragReceive },
	 //                { kEventClassControl, kEventControlOwningWindowChanged }
 };
 
 */

class EditorComponentHolder  :	public Component,
								public Timer
{
public:
    EditorComponentHolder (Component* const editorComp_, t_box* x)
		:	ref(x),
			mouseIsInEditor(false),
			lastBoxX(0xffff),
			lastBoxY(0xffff),
			lastMouseIsDown(false)
	{
        addAndMakeVisible (editorComp_);
        setOpaque (true);
        setVisible (true);
        setBroughtToFrontOnMouseClick (true);
		
//#if ! JucePlugin_EditorRequiresKeyboardFocus
        setWantsKeyboardFocus (false);
//#endif
		
		editorComp = editorComp_;
		startTimer(250);
	}

	~EditorComponentHolder()
	{
		stopTimer();
	}
	
	void timerCallback()
    {
		//post("EditorComponentHolder::timerCallback %p", this);
		
		if (editorComp != 0)
        {
            int screenX, screenY;
            Desktop::getInstance().getMousePosition (screenX, screenY);
			
			//post("EditorComponentHolder::timerCallback screenmouse x=%d y=%d", screenX, screenY);
			
			int windowX = screenX - ref->b_patcher->p_wind->w_x1, 
				windowY = screenY - ref->b_patcher->p_wind->w_y1;
			
			//post("EditorComponentHolder::timerCallback windowmouse x=%d y=%d", windowX, windowY);
			
			int boxX = windowX - ref->b_rect.left,
				boxY = windowY - ref->b_rect.top;
			
			//post("EditorComponentHolder::timerCallback boxmouse x=%d y=%d", boxX, boxY);
			
			int boxW = ref->b_rect.right - ref->b_rect.left,
				boxH = ref->b_rect.bottom - ref->b_rect.top;
			
			const ModifierKeys mod = ModifierKeys::getCurrentModifiersRealtime();
			bool mouseIsDown = mod.isAnyMouseButtonDown();
			
			post("mouseIsDown = %d", mouseIsDown);
			
			if(mouseIsDown == false) {
				mouseDownX = boxX;
				mouseDownY = boxY;
				mouseDownTime = Time::getCurrentTime();
			}
			
			if(boxX >= 0 && boxX < boxW && boxY >= 0 && boxY < boxH) {
				//post("EditorComponentHolder MOUSE IN BOX");
				
				MouseEvent e(boxX, boxY,							// position
							 mod,									// mods
							 this,									// originator comp
							 Time::getCurrentTime(),				// event time
							 mouseDownX, mouseDownY,				// mouse down pos
							 mouseDownTime,							// mouse down time
							 mouseIsDown ? 1 : 0,					// numclicks
							 mouseIsDown && lastMouseIsDown);	    // mousewasdragged
				
				if(mouseIsInEditor == false) {
					mouseIsInEditor = true;
					editorComp->mouseEnter(e);
				}
				else if(boxX != lastBoxX || boxY != lastBoxY)
					editorComp->mouseMove(e);
				
				if(mouseIsDown == true && lastMouseIsDown == false)
					editorComp->mouseDown(e);
				else if(mouseIsDown == false && lastMouseIsDown == true)
					editorComp->mouseUp(e);
				
				
			}
			else if(mouseIsInEditor == true) {
				MouseEvent e(boxX, boxY,							// position
							 mod,									// mods
							 this,									// originator comp
							 Time::getCurrentTime(),				// event time
							 mouseDownX, mouseDownY,				// mouse down pos
							 mouseDownTime,							// mouse down time
							 mouseIsDown ? 1 : 0,					// numclicks, mousewasdragged
							 mouseIsDown && lastMouseIsDown);
				
				editorComp->mouseExit(e);
				mouseIsInEditor = false;
			}
			
			lastBoxX = boxX;
			lastBoxY = boxY;
			lastMouseIsDown = mouseIsDown;
		}
		
	}

	void resized()
	{
		if (getNumChildComponents() > 0)
			getChildComponent (0)->setBounds (0, 0, getWidth(), getHeight());
	}

	void paint (Graphics& g)
	{
		post("EditorComponentHolder::paint");
	}
	
	void mouseDown(const MouseEvent& e)
	{
		post("EditorComponentHolder::mousedown %d, %d", e.x, e.y);
	}
	
private:	
	t_box* ref;
	Component* editorComp;
	bool mouseIsInEditor;
	int lastBoxX, lastBoxY;
	bool lastMouseIsDown;
	int mouseDownX, mouseDownY;
	Time mouseDownTime;
};

class EditorComponent : public Component
{
public:
	EditorComponent()
	{
		addAndMakeVisible(slider = new Slider("Slider"));
	}
	
	~EditorComponent()
	{
		deleteAllChildren();
	}
	
	void resized()
    {
		slider->setBounds(20,20,200,24);
	}
	
	void paint (Graphics& g)
    {
		post("EditorComponent::paint");
		g.fillAll (Colour::greyLevel (0.9f));
    }
	
	void mouseEnter(const MouseEvent& e)
	{
		post("EditorComponent::mouseEnter %d, %d", e.x, e.y);
	}
	
	void mouseExit(const MouseEvent& e)
	{
		post("EditorComponent::mouseExit %d, %d", e.x, e.y);
	}
	
	void mouseDown(const MouseEvent& e)
	{
		post("EditorComponent::mouseDown %d, %d", e.x, e.y);
	}
	
	void mouseUp(const MouseEvent& e)
	{
		post("EditorComponent::mouseUp %d, %d", e.x, e.y);
	}
	
	void mouseMove(const MouseEvent& e)
	{
		post("EditorComponent::mouseMove %d, %d", e.x, e.y);
	}
	
private:
	Slider* slider;
	ResizableCornerComponent* resizer;
};

typedef struct _jucebox
{
	t_box		ob;
	void		*obex;
	void		*qelem;	
	Rect 		rect;
	
	EditorComponent* juceEditorComp;
	EditorComponentHolder* juceWindowComp;
	HIViewRef hiRoot;
	
} t_jucebox;

static t_class *jucebox_class;
void *jucebox_new(t_symbol *s, short ac, t_atom *av);
void jucebox_addjucecomponents(t_jucebox* x);
void jucebox_back(t_jucebox* x);
void jucebox_forward(t_jucebox* x);
void *jucebox_menu(void *p, long x, long y, long font);
void jucebox_psave(t_jucebox *x, void *w);
void jucebox_free(t_jucebox* x);
void jucebox_deleteui(t_jucebox *x);
void jucebox_click(t_jucebox *x, MaxMSPPoint pt, short modifiers);
void jucebox_update(t_jucebox* x);
void jucebox_qfn(t_jucebox* x);


int main()
{	
	initialiseJuce_GUI();

	long attrflags = 0;
	t_class *c;
	t_object *attr;
	
	common_symbols_init();
		
	c = class_new("juce_maxbox",(method)jucebox_new, (method)jucebox_free, (short)sizeof(t_jucebox), (method)jucebox_menu, A_GIMME, 0);
	class_obexoffset_set(c, calcoffset(t_jucebox, obex));
	
	class_addmethod(c, (method)jucebox_psave,		"psave",		A_CANT, 0L);
	class_addmethod(c, (method)jucebox_click,		"click",		A_CANT, 0L);
	class_addmethod(c, (method)jucebox_update,		"update",		A_CANT, 0L);
	class_addmethod(c, (method)jucebox_back,		"back",			0L);
	class_addmethod(c, (method)jucebox_forward,		"forward",		0L);
		
	class_register(CLASS_BOX, c);
	jucebox_class = c;	
	return 0;
}

#define WINDOWTITLEBARHEIGHT 22

void *jucebox_new(t_symbol *s, short argc, t_atom *argv)
{
	t_jucebox *x;
	void *patcher;
	long x_coord, y_coord, width, height;
	short flags;
	
	x = (t_jucebox*)object_alloc(jucebox_class);
	
	if(x) {		
		patcher = argv[0].a_w.w_obj;					// patcher
		x_coord = argv[1].a_w.w_long;					// x coord
		y_coord = argv[2].a_w.w_long;					// y coord
		width = argv[3].a_w.w_long;						// width
		height = argv[4].a_w.w_long;					// height

		width = CLIP(width, MINWIDTH, MAXWIDTH); 		// constrain to min and max size
		height = CLIP(height, MINHEIGHT, MAXHEIGHT);	// constrain to min and max size

		flags = F_DRAWFIRSTIN | F_NODRAWBOX | F_GROWBOTH | F_DRAWINLAST | F_TRANSPARENT;// |	F_SAVVY ;

		// now actually initialize the t_box structure
		box_new((t_box *)x, (t_patcher *)patcher, flags, x_coord, y_coord, x_coord + width, y_coord + height);

		// Reassign the box's firstin field to point to our new object
		x->ob.b_firstin = (void *)x;
		
		object_obex_store((void *)x, _sym_dumpout, (t_object *)outlet_new(x,NULL));
		
		// Cache rect for comparisons when the user decides to re-size the object
		x->rect = x->ob.b_rect;

		// Create our queue element for defering calls to the draw function
		x->qelem = qelem_new(x, (method)jucebox_qfn);
		
		x->juceWindowComp = 0L;
		x->juceEditorComp = 0L;
		
		// Finish it up...
		box_ready((t_box *)x);
	}
	else
		error("juce_maxbox - could not create instance");
	
	return(x);
}

void jucebox_addjucecomponents(t_jucebox* x)
{
	long	x_coord = x->ob.b_rect.left, 
			y_coord = x->ob.b_rect.top, 
			width   = x->ob.b_rect.right -  x->ob.b_rect.left, 
			height  = x->ob.b_rect.bottom - x->ob.b_rect.top;
	
	x->juceEditorComp = new EditorComponent();
	x->juceEditorComp->setBounds(0, 0, width, height);
	
	const int w = x->juceEditorComp->getWidth();
	const int h = x->juceEditorComp->getHeight();
	
	x->juceEditorComp->setOpaque (true);
	x->juceEditorComp->setVisible (true);
	
	x->juceWindowComp = new EditorComponentHolder(x->juceEditorComp, (t_box*)x);
	x->juceWindowComp->setBounds(x_coord, y_coord + WINDOWTITLEBARHEIGHT, w, h);
	
	// Mac only here...
	HIViewRef hiRoot = HIViewGetRoot((WindowRef)wind_syswind(x->ob.b_patcher->p_wind));
		
	x->juceWindowComp->addToDesktop(ComponentPeer::windowRepaintedExplictly, (void*)hiRoot);	
	
	//HIViewRef bottomView = HIViewGetLastSubview(hiRoot);
	//HIViewRef topView = HIViewGetFirstSubview(hiRoot);

	//HIViewSetZOrder((HIViewRef)x->juceWindowComp->getPeer()->getNativeView(), kHIViewZOrderBelow, topView);
	
	x->hiRoot = hiRoot;
}

void jucebox_back(t_jucebox* x)
{
	post("jucebox_back");
	
	HIViewRef nextView = HIViewGetNextView(x->hiRoot);
	HIViewSetZOrder((HIViewRef)x->juceWindowComp->getPeer()->getNativeView(), kHIViewZOrderBelow, nextView);
	
}


void jucebox_forward(t_jucebox* x)
{
	post("jucebox_forward");
	
	HIViewRef prevView = HIViewGetPreviousView(x->hiRoot);
	HIViewSetZOrder((HIViewRef)x->juceWindowComp->getPeer()->getNativeView(), kHIViewZOrderAbove, prevView);
	
}

void *jucebox_menu(void *p, long x, long y, long font)
{
	t_atom argv[5];
	
	SETOBJ(argv, (t_object *)p);				// patcher
	SETLONG(argv+1, x);							// x coord
	SETLONG(argv+2, y);							// y coord
	SETLONG(argv+3, DEFWIDTH);					// width
	SETLONG(argv+4, DEFHEIGHT);					// height
	
	return jucebox_new(gensym("juce_maxbox"), 5, argv);
}

void jucebox_psave(t_jucebox *x, void *w)
{
	Rect r = x->ob.b_rect;
	t_atom argv[16];
	short inc = 0;
	
	SETSYM(argv,gensym("#P"));
	if (x->ob.b_hidden) {	// i.e. if it's hidden when the patcher is locked
		SETSYM(argv+1,gensym("hidden"));
		inc = 1;
	}
	SETSYM(argv+1+inc, gensym("user"));
	SETSYM(argv+2+inc, ob_sym(x));
	SETLONG(argv+3+inc, r.left);			// x
	SETLONG(argv+4+inc, r.top);				// y
	SETLONG(argv+5+inc, r.right - r.left);	// width
	SETLONG(argv+6+inc, r.bottom - r.top);	// height
	
	binbuf_insert(w, 0L, 7+inc, argv);
}

void jucebox_free(t_jucebox* x)
{
	jucebox_deleteui(x);
	
	qelem_free(x->qelem);				// delete our qelem
	box_free((t_box *)x);				// free the ui box
}

void jucebox_deleteui(t_jucebox *x)
{
	PopupMenu::dismissAllActiveMenus();
	
	// there's some kind of component currently modal, but the host
	// is trying to delete our plugin..
	//	jassert (Component::getCurrentlyModalComponent() == 0);
		
	if(x->juceEditorComp) deleteAndZero (x->juceEditorComp);
	if(x->juceWindowComp) deleteAndZero (x->juceWindowComp);
}

void jucebox_click(t_jucebox *x, MaxMSPPoint pt, short modifiers)
{
	post("jucebox_click x=%d y=%d", pt.h, pt.v);
}

void jucebox_update(t_jucebox* x)
{
	box_invalnow(&x->ob);
	
	post("jucebox_update");
	
	short width_old = x->rect.right - x->rect.left;
	short height_old = x->rect.bottom - x->rect.top;
	short width_new = x->ob.b_rect.right - x->ob.b_rect.left;
	short height_new = x->ob.b_rect.bottom - x->ob.b_rect.top;
	
	if(height_new != height_old || width_new != width_old) {
		width_new = CLIP(width_new, MINWIDTH, MAXWIDTH); // constrain to min and max size
		height_new = CLIP(height_new, MINHEIGHT, MAXHEIGHT);
		box_size(&x->ob, width_new, height_new);	// this function actually resizes out t_box structure
		x->rect = x->ob.b_rect;
	}
	
	GrafPtr	gp = patcher_setport(x->ob.b_patcher);
	
	if(!x->juceWindowComp) jucebox_addjucecomponents(x);
	
	x->juceWindowComp->setBounds(0xffff, 0xffff, width_new, height_new);
	x->juceWindowComp->setBounds(x->ob.b_rect.left, x->ob.b_rect.top + WINDOWTITLEBARHEIGHT, width_new, height_new);
//	x->juceWindowComp->resized();
//	x->juceWindowComp->repaint();
	x->juceWindowComp->repaint();
	x->juceWindowComp->getPeer()->repaint(0, 0, 0xffff, 0xffff);
	x->juceWindowComp->getPeer()->performAnyPendingRepaintsNow();
	
    patcher_restoreport(gp); 
	
}

void jucebox_qfn(t_jucebox* x)
{
	
}



