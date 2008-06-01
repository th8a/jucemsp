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

#define VALIDCHARS "ABCDEFGHIJKLMNOPQRTSUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789"

extern Component* createMaxBoxComponent();

typedef struct _jucebox
{
	t_box		ob;
	void		*obex;
	void		*qelem;	
	Rect 		rect;
	
	Component* juceEditorComp;
	class EditorComponentHolder* juceWindowComp;
	HIViewRef hiRoot;
	
	t_object*	dumpOut;
	
	float		params[256]; // must be able to do a deep count of child components to set this dynamically
	
} t_jucebox;

static t_class *jucebox_class;
void jucebox_setupattrs(t_class *c, t_symbol* prefixSym, Component* comp, int *paramIndex);
void *jucebox_new(t_symbol *s, short ac, t_atom *av);
t_max_err jucebox_attr_get(t_jucebox *x, void *attr, long *argc, t_atom **argv); 
t_max_err jucebox_attr_geteach(t_jucebox *x, Component* comp, t_symbol* attrName, t_symbol *prefixSym, int* paramIndex, long *ac, t_atom **av);
t_max_err jucebox_attr_set(t_jucebox *x, void *attr, long argc, t_atom *argv); 
t_max_err jucebox_attr_seteach(t_jucebox *x, Component* comp, t_symbol* attrName, t_symbol *prefixSym, int* paramIndex, long ac, t_atom *av);
void jucebox_sliderchanged(t_jucebox* x, Slider *slider);
t_max_err jucebox_sliderchangedeach(t_jucebox* x, Component* comp, Slider *changedSlider, t_symbol *prefixSym, int* paramIndex);
void jucebox_addjucecomponents(t_jucebox* x);
void jucebox_addjucecomponentseach(t_jucebox* x, Component *comp);
void *jucebox_menu(void *p, long x, long y, long font);
void jucebox_psave(t_jucebox *x, void *w);
void jucebox_free(t_jucebox* x);
void jucebox_deleteui(t_jucebox *x);
void jucebox_click(t_jucebox *x, MaxMSPPoint pt, short modifiers);
void jucebox_update(t_jucebox* x);
void jucebox_qfn(t_jucebox* x);


class EditorComponentHolder  :	public Component,
								public Timer,
								public SliderListener,
								public ComponentListener
{
public:
    EditorComponentHolder (Component* const editorComp_, t_jucebox* x)
		:	ref(x)
	{
		addAndMakeVisible (editorComp_);
        setOpaque (true);
        setVisible (true);
        //setBroughtToFrontOnMouseClick (true);
		
//#if ! JucePlugin_EditorRequiresKeyboardFocus
        setWantsKeyboardFocus (false);
//#endif
		
		editorComp = editorComp_;
		
		editorComp->addComponentListener(this);
		
		startTimer(20);
		
	}

	~EditorComponentHolder()
	{
		editorComp->removeComponentListener(this);
		stopTimer();
	}
	
	void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized)
	{
		//post("EditorComponentHolder::componentMovedOrResized");
		box_size(ref, editorComp->getWidth(), editorComp->getHeight());
		//setSize(editorComp->getWidth(), editorComp->getHeight());
	}
	
	void timerCallback()
    {
		if(box_ownerlocked((t_box*)ref)) {
			setVisible(true);
						
			calcAndSetBounds();
						
			getPeer()->toFront(false);
		
		}
		else {
			setVisible(false);
		}
	}
	
	void sliderValueChanged(Slider *slider)
	{
		//post("EditorComponentHolder::sliderValueChanged");
		
		jucebox_sliderchanged(ref, slider);
	}
	
	void calcAndSetBounds()
	{
		int windowX = ref->ob.b_patcher->p_wind->w_x1,
		windowY = ref->ob.b_patcher->p_wind->w_y1,
		windowR = ref->ob.b_patcher->p_wind->w_x2 - SBARWIDTH,
		windowB = ref->ob.b_patcher->p_wind->w_y2 - SBARWIDTH;
		
		int boxX = ref->ob.b_rect.left + windowX,
			boxY = ref->ob.b_rect.top + windowY,
			boxR = ref->ob.b_rect.right + windowX,
			boxB = ref->ob.b_rect.bottom + windowY;
		
		Rectangle windowRect(windowX, windowY, windowR-windowX, windowB-windowY);
		Rectangle boxRect(boxX, boxY, boxR-boxX, boxB-boxY);
				
		int offsetX = boxX >= windowX ? 0 : boxX-windowX;
		int offsetY = boxY >= windowY ? 0 : boxY-windowY;
				
		Rectangle sectRect = windowRect.getIntersection(boxRect);
		
		editorComp->setBounds(offsetX, offsetY, boxR-boxX, boxB-boxY);
		setBounds(sectRect);
		
		
	}
	
	void resized()
	{
		//if (getNumChildComponents() > 0)
		//	getChildComponent (0)->setBounds (0, 0, getWidth(), getHeight());
		
		// do the clipping by manipulating these two rects...
		
		
	}
	
		
private:	
	t_jucebox* ref;
	Component* editorComp;
};




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
	
	class_addmethod(c, (method)object_obex_dumpout,  "dumpout",		A_CANT,	0);  
	class_addmethod(c, (method)object_obex_quickref, "quickref",	A_CANT, 0);
		
	Component* tempComp = createMaxBoxComponent(); //new EditorComponent();
	int paramIndex = 0;
	jucebox_setupattrs(c, gensym(""), tempComp, &paramIndex);
	delete tempComp;
	
	class_register(CLASS_BOX, c);
	jucebox_class = c;	
	return 0;
}

void jucebox_setupattrs(t_class *c, t_symbol* prefixSym, Component* comp, int *paramIndex)
{
	long attrflags = 0;
	t_object *attr;
	
	String prefix(prefixSym->s_name);
	if(prefix.isNotEmpty())
		prefix << "_";
	
	// plugin parameter attributes
	for(int i = 0; i < comp->getNumChildComponents(); i++) {
		(*paramIndex)++;
		
		Component* child = comp->getChildComponent(i);
				
		String childAttrName(prefix + child->getName().replaceCharacter (' ', '_')
													  .replaceCharacter ('.', '_')
													  .retainCharacters (T(VALIDCHARS)));
		
		//post("procssing attr: %s", (char*)(const char*)childAttrName);
		
		Slider * slider = dynamic_cast <Slider*> (child);
		
		if(slider != 0) {
			attr = attr_offset_new((char*)(const char*)childAttrName, 
								   _sym_float32, attrflags,
								   (method)jucebox_attr_get, (method)jucebox_attr_set, 
								   calcoffset(t_jucebox, params) + sizeof(float) * (*paramIndex)); 
			class_addattr(c, attr);
			
			post("procssing attr: %s index=%d", (char*)(const char*)childAttrName, *paramIndex);
		}
		
		GroupComponent * group = dynamic_cast <GroupComponent*> (child);
		
		if(group != 0 && group->getNumChildComponents() > 0) {
			jucebox_setupattrs(c, gensym((char*)(const char*)childAttrName), group, paramIndex);
		}
	}
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

		flags = F_DRAWFIRSTIN | F_NODRAWBOX | F_GROWBOTH | F_DRAWINLAST | F_SAVVY ;

		// now actually initialize the t_box structure
		box_new((t_box *)x, (t_patcher *)patcher, flags, x_coord, y_coord, x_coord + width, y_coord + height);

		// Reassign the box's firstin field to point to our new object
		x->ob.b_firstin = (void *)x;
		
		// create attr dumpout
		object_obex_store((void *)x, 
						  _sym_dumpout, 
						  x->dumpOut = (t_object *)outlet_new(x,NULL));
		
		// Cache rect for comparisons when the user decides to re-size the object
		x->rect = x->ob.b_rect;

		// Create our queue element for defering calls to the draw function
		x->qelem = qelem_new(x, (method)jucebox_qfn);
		
		x->juceWindowComp = 0L;
		x->juceEditorComp = 0L;
		
		jucebox_addjucecomponents(x);
		
		// Finish it up...
		box_ready((t_box *)x);
	}
	else
		error("juce_maxbox - could not create instance");
	
	return(x);
}

t_max_err jucebox_attr_get(t_jucebox *x, void *attr, long *ac, t_atom **av)
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
	
	int paramIndex = 0;
	
	return jucebox_attr_geteach(x, x->juceEditorComp, attrName, gensym(""), &paramIndex, ac, av);
}

t_max_err jucebox_attr_geteach(t_jucebox *x, Component* comp, t_symbol* attrName, t_symbol *prefixSym, int* paramIndex, long *ac, t_atom **av)
{
	String prefix(prefixSym->s_name);
	if(prefix.isNotEmpty())
		prefix << "_";
	
	for(int i = 0; i < comp->getNumChildComponents(); i++) {
		(*paramIndex)++;
		
		Component* child = comp->getChildComponent(i);
		
		String childAttrName(prefix + child->getName().replaceCharacter (' ', '_')
							 .replaceCharacter ('.', '_')
							 .retainCharacters (T(VALIDCHARS)));
		
		t_symbol *paramName = gensym((char*)(const char*)childAttrName);
		
		post("jucebox_attr_geteach attrName=%s paramName=%s", attrName->s_name, paramName->s_name);
		
		if(attrName == paramName) {
			
			post("getting attr: %s index=%d", (char*)(const char*)childAttrName, *paramIndex);
			
			atom_setfloat(*av, x->params[*paramIndex]);
			return MAX_ERR_NONE;
		}
		
		GroupComponent * group = dynamic_cast <GroupComponent*> (child);
		
		if(group != 0 && group->getNumChildComponents() > 0) {
			t_max_err err = jucebox_attr_geteach(x, group, 
												 attrName,
												 gensym((char*)(const char*)childAttrName), 
												 paramIndex, ac, av);
			
			if(err == MAX_ERR_NONE)
				return MAX_ERR_NONE; // we found it
		}
	}
	
	return MAX_ERR_GENERIC; // we didn't find it
}


t_max_err jucebox_attr_set(t_jucebox *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) { 
		t_symbol *attrName = (t_symbol*)object_method(attr, _sym_getname); 
		
		int paramIndex = 0;
		
		return jucebox_attr_seteach(x, x->juceEditorComp, attrName, gensym(""), &paramIndex, ac, av);
	} 
	
	return MAX_ERR_GENERIC; 
}

t_max_err jucebox_attr_seteach(t_jucebox *x, Component* comp, t_symbol* attrName, t_symbol *prefixSym, int* paramIndex, long ac, t_atom *av)
{
	String prefix(prefixSym->s_name);
	if(prefix.isNotEmpty())
		prefix << "_";
	
	for(int i = 0; i < comp->getNumChildComponents(); i++) {
		(*paramIndex)++;
		
		Component* child = comp->getChildComponent(i);
		
		String childAttrName(prefix + child->getName().replaceCharacter (' ', '_')
							 .replaceCharacter ('.', '_')
							 .retainCharacters (T(VALIDCHARS)));
		
		t_symbol *paramName = gensym((char*)(const char*)childAttrName);
		
		Slider* slider = dynamic_cast <Slider*> (child);
		
		if(slider != 0) {
			if(attrName == paramName) {
				post("setting attr: %s index=%d", (char*)(const char*)childAttrName, *paramIndex);
				
				x->params[*paramIndex] = atom_getfloat(av);			
				
				slider->setValue(x->params[*paramIndex], false);
				
				if(slider->getValue() != x->params[*paramIndex])
					x->params[*paramIndex] = slider->getValue();
								
				return MAX_ERR_NONE;
			}
		}
		
		GroupComponent * group = dynamic_cast <GroupComponent*> (child);
		
		if(group != 0 && group->getNumChildComponents() > 0) {
			t_max_err err = jucebox_attr_seteach(x, group, attrName, paramName, paramIndex, ac, av);
			
			if(err == MAX_ERR_NONE)
				return MAX_ERR_NONE;  // we found it
		}
	}
	
	return MAX_ERR_GENERIC; // we didn't find it
}

void jucebox_sliderchanged(t_jucebox* x, Slider *slider)
{
	int paramIndex = 0;
	jucebox_sliderchangedeach(x, x->juceEditorComp, slider, gensym(""), &paramIndex);
}

t_max_err jucebox_sliderchangedeach(t_jucebox* x, Component* comp, Slider *changedSlider, t_symbol *prefixSym, int* paramIndex)
{
	String prefix(prefixSym->s_name);
	if(prefix.isNotEmpty())
		prefix << "_";
	
	for(int i = 0; i < comp->getNumChildComponents(); i++) {
		(*paramIndex)++;
		
		Component* child = comp->getChildComponent(i);
		
		String childAttrName(prefix + child->getName().replaceCharacter (' ', '_')
							 .replaceCharacter ('.', '_')
							 .retainCharacters (T(VALIDCHARS)));
		
		t_symbol *paramName = gensym((char*)(const char*)childAttrName);
		
		Slider* slider = dynamic_cast <Slider*> (child);
		
		if(changedSlider == slider) {
			post("jucebox_sliderchanged %s index=%d", paramName->s_name, *paramIndex);
			
			x->params[*paramIndex] = slider->getValue();
			
			//object_attr_getdump(x, paramName, 0, 0L); weird I get "der" from the outlet!
			
			t_atom atom;
			atom_setfloat(&atom, x->params[*paramIndex]);
			outlet_anything(x->dumpOut, paramName, 1, &atom);
			
			return MAX_ERR_NONE;
		}
		
		GroupComponent * group = dynamic_cast <GroupComponent*> (child);
		
		post("jucebox_sliderchangedeach group=%p", group);
		
		if(group != 0 && group->getNumChildComponents() > 0) {
			
			post("jucebox_sliderchangedeach in group");
			
			t_max_err err = jucebox_sliderchangedeach(x, group, changedSlider, paramName, paramIndex);
			
			if(err == MAX_ERR_NONE)
				return MAX_ERR_NONE;  // we found it
		}
		
	}
	
	return MAX_ERR_GENERIC; // we didn't find it
}

void jucebox_addjucecomponents(t_jucebox* x)
{
	long	x_coord = x->ob.b_rect.left, 
			y_coord = x->ob.b_rect.top, 
			width   = x->ob.b_rect.right -  x->ob.b_rect.left, 
			height  = x->ob.b_rect.bottom - x->ob.b_rect.top;
	
	x->juceEditorComp = createMaxBoxComponent(); // new EditorComponent();
	x->juceEditorComp->setBounds(0, 0, width, height);
	
	const int w = x->juceEditorComp->getWidth();
	const int h = x->juceEditorComp->getHeight();
	
	x->juceEditorComp->setOpaque (true);
	x->juceEditorComp->setVisible (true);
	
	x->juceWindowComp = new EditorComponentHolder(x->juceEditorComp, x);			
	x->juceWindowComp->calcAndSetBounds();
	
	// Mac only here...
	HIViewRef hiRoot = HIViewGetRoot((WindowRef)wind_syswind(x->ob.b_patcher->p_wind));
			
	x->juceWindowComp->addToDesktop(0);//, (void*)hiRoot);

	x->hiRoot = hiRoot;
		
	jucebox_addjucecomponentseach(x, x->juceEditorComp);
}

void jucebox_addjucecomponentseach(t_jucebox* x, Component *comp)
{
	for(int i = 0; i < comp->getNumChildComponents(); i++) {
		Component * child = comp->getChildComponent(i);
		
		Slider* slider = dynamic_cast <Slider*> (child);
		
		if(slider != 0)
			slider->addListener(x->juceWindowComp);
		
		GroupComponent * group = dynamic_cast <GroupComponent*> (child);
				
		if(group != 0 && group->getNumChildComponents() > 0) {
			jucebox_addjucecomponentseach(x, group);
		}
	}
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
	//post("jucebox_click x=%d y=%d", pt.h, pt.v);
}

void jucebox_update(t_jucebox* x)
{
	box_invalnow(&x->ob);
	
	//post("jucebox_update");
	
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
		
	if(box_ownerlocked((t_box*)x)) {
		if(!x->juceWindowComp) jucebox_addjucecomponents(x);
		
//		x->juceWindowComp->setBounds(x->ob.b_rect.left + x->ob.b_patcher->p_wind->w_x1, 
//									 x->ob.b_rect.top + x->ob.b_patcher->p_wind->w_y1, 
//									 width_new, height_new);
		
		x->juceWindowComp->calcAndSetBounds();
	}
	else {
		GrafPtr	gp = patcher_setport(x->ob.b_patcher);
		// could display some dummy text in unlocked mode
			
			RGBColor ggryrgb = {32767, 32767, 32767};
			RGBColor oldBackColor;
			
			GetBackColor(&oldBackColor);
			RGBBackColor(&ggryrgb);
			
			EraseRect(&x->ob.b_rect);
			
			MoveTo(x->ob.b_rect.left + 4, x->ob.b_rect.bottom - 4); 
			TextFont(0); 
			TextSize(12); 
			
			String text;
			
			text << "juce_box";
			
			DrawText((char*)(const char*)text, 0, text.length());
			
			RGBBackColor(&oldBackColor);
			
		patcher_restoreport(gp); 
	}
	
}

void jucebox_qfn(t_jucebox* x)
{
	
}



