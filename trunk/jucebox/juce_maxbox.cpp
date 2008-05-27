#include "../../../juce_code/juce/juce.h"
#include "ext.h"
#include "ext_common.h"
#include "ext_obex.h"

#define RES_ID			10120
#define MAXWIDTH 		1024L	
#define MAXHEIGHT		512L		//		(defines maximum offscreen canvas allocation)
#define MINWIDTH 		20L			// minimum width and height
#define MINHEIGHT		2L			//		...
#define DEFWIDTH 		300			// default width and height
#define DEFHEIGHT		150			//		...

class EditorComponentHolder  : public Component
{
public:
    EditorComponentHolder (Component* const editorComp, t_box* x)
	: ref(x)
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
		post("EditorComponentHolder::paint");
//		
//		patcher_setport(ref->b_patcher);
//		ClipRect(&ref->b_rect);
	}
	
	void mouseDown(const MouseEvent& e)
	{
		post("EditorComponentHolder::mousedown %d, %d", e.x, e.y);
	}
	
private:	
	t_box* ref;
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
		
//		getParentComponent()->getPeer()->grabFocus();
//		getParentComponent()->getPeer()->handleFocusLoss();
		
    }
	
	void mouseEnter(const MouseEvent& e)
	{
		post("EditorComponent::mouseEnter %d, %d", e.x, e.y);
	}
	
	void mouseExit(const MouseEvent& e)
	{
		post("EditorComponent::mouseExit %d, %d", e.x, e.y);
		
//		getParentComponent()->getPeer()->grabFocus();
//		getParentComponent()->getPeer()->handleFocusLoss();
		
	}
	
	void mouseDown(const MouseEvent& e)
	{
		post("EditorComponent::mouseDown %d, %d", e.x, e.y);
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
	
} t_jucebox;

static t_class *jucebox_class;
void *jucebox_new(t_symbol *s, short ac, t_atom *av);
void *jucebox_menu(void *p, long x, long y, long font);
void jucebox_free(t_jucebox* x);
void jucebox_update(t_jucebox* x);
void jucebox_qfn(t_jucebox* x);

int main()
{	
	initialiseJuce_GUI();
	
	long attrflags = 0;
	t_class *c;
	t_object *attr;
		
	c = class_new("juce_maxbox",(method)jucebox_new, (method)jucebox_free, (short)sizeof(t_jucebox), (method)jucebox_menu, A_GIMME, 0);
	class_obexoffset_set(c, calcoffset(t_jucebox, obex));
	
	class_addmethod(c, (method)jucebox_update, 	"update", A_CANT, 0L);
		
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
		
		flags = F_DRAWFIRSTIN | F_NODRAWBOX | F_GROWBOTH | F_DRAWINLAST | F_SAVVY;
		
		// now actually initialize the t_box structure
		box_new((t_box *)x, (t_patcher *)patcher, flags, x_coord, y_coord, x_coord + width, y_coord + height);
		
		// Reassign the box's firstin field to point to our new object
		x->ob.b_firstin = (void *)x;
		
		// Cache rect for comparisons when the user decides to re-size the object
		x->rect = x->ob.b_rect;
		
		// Create our queue element for defering calls to the draw function
		x->qelem = qelem_new(x, (method)jucebox_qfn);
		
		x->juceEditorComp = new EditorComponent(); //new Slider (T("gain"));
        x->juceEditorComp->setBounds(0, 0, width, height);
        
        const int w = x->juceEditorComp->getWidth();
        const int h = x->juceEditorComp->getHeight();
        
        x->juceEditorComp->setOpaque (true);
        x->juceEditorComp->setVisible (true);
        
		
		
        x->juceWindowComp = new EditorComponentHolder(x->juceEditorComp, (t_box*)x);
        x->juceWindowComp->setBounds(x_coord, y_coord + WINDOWTITLEBARHEIGHT, w, h);
		
//		x->juceWindowComp->setBounds(x_coord + x->ob.b_patcher->p_wind->w_x1, 
//									 y_coord + x->ob.b_patcher->p_wind->w_y1, 
//									 w, h);
		
		//x->juceWindowComp->setBounds(0, 0, w, h);
        
        // Mac only here...!
        HIViewRef hiRoot = HIViewGetRoot((WindowRef)wind_syswind(x->ob.b_patcher->p_wind));
		post("HIViewRef hiRoot=%p", hiRoot);
		
		HIViewRef parentView = 0;
		//HIViewFindByID (hiRoot, kHIViewWindowContentID, &parentView);
		
		if (parentView == 0) {
			post("parentView=0");
			parentView = hiRoot;
		}        
        
		x->juceWindowComp->setInterceptsMouseClicks(true, true);
        x->juceWindowComp->addToDesktop(0, (void*)parentView);
		//x->juceWindowComp->addToDesktop(0, 0);	
		
		// Finish it up...
		box_ready((t_box *)x);
	}
	else
		error("juce_maxbox - could not create instance");
	
	return(x);
}

void *jucebox_menu(void *p, long x, long y, long font)
{
	t_atom argv[5];		// reduced to 5 on 24 nov 2004
	
	SETOBJ(argv, (t_object *)p);				// patcher
	SETLONG(argv+1, x);							// x coord
	SETLONG(argv+2, y);							// y coord
	SETLONG(argv+3, DEFWIDTH);					// width
	SETLONG(argv+4, DEFHEIGHT);					// height
	
	return jucebox_new(gensym("juce_maxbox"), 5, argv);
}

void jucebox_free(t_jucebox* x)
{
	qelem_free(x->qelem);				// delete our qelem
	box_free((t_box *)x);				// free the ui box
}

void jucebox_update(t_jucebox* x)
{
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
	
	x->juceWindowComp->setBounds(x->ob.b_rect.left, x->ob.b_rect.top + WINDOWTITLEBARHEIGHT, width_new, height_new);
	
//	x->juceWindowComp->setBounds(x->ob.b_rect.left + x->ob.b_patcher->p_wind->w_x1, 
//								 x->ob.b_rect.top + x->ob.b_patcher->p_wind->w_y1, 
//								 width_new, height_new);
	
	x->juceWindowComp->repaint();
//	x->juceWindowComp->toFront(false);
	
    patcher_restoreport(gp); 
	
	
	
}

void jucebox_qfn(t_jucebox* x)
{
	
}



