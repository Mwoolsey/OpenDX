/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <Xm/DragDrop.h>
#include <X11/Xatom.h>


#include "XmUtility.h"
#include "Interactor.h"
#include "InteractorInstance.h"
#include "../widgets/WorkspaceW.h"
#include "../widgets/XmDX.h"
#include "ControlPanel.h"
#include "ControlPanelWorkSpace.h"
#include "Network.h"
#include "DXStrings.h"
#include "DXApplication.h" 

#include "ntractor.bm"
#include "ntractormask.bm"

boolean Interactor::InteractorClassInitialized = FALSE;
Widget  Interactor::DragIcon;

#if defined(aviion) || defined(alphax)
// This constant exists in ToggleInteractor.C also, so if you make a change
// then dupe it there.
#define XMSTRING_COMPARE_IS_BROKEN
#endif

#define DXXOFFSET "DX_XOFFSET"
#define DXYOFFSET "DX_YOFFSET"

//
// This is the value that specifies the default thickness of the white
// border around the interactor (not the decorators).
// Use the constant as a default value for HiLites.
//
// If you change HiLites either thru a change to OFFSET or a change to
// dxui.interactorHighlight, you also affect the size of the interactor.
#define OFFSET 2
static int HiLites = OFFSET + 1;

String Interactor::DefaultResources[] =
{
    "*borderWidth:			0",
    ".shadowType:			SHADOW_OUT",
    ".resizePolicy:			XmRESIZE_ANY",
    "*interactive_form.resizePolicy:	XmRESIZE_ANY",
    ".shadowThickness:			1",
    ".marginWidth: 			1",
    ".marginHeight: 			1",
    ".interactor_label.marginWidth:	1",
    ".interactor_label.marginHeight:	0",
    ".interactor_label.marginLeft:	1",
    ".interactor_label.marginBottom:	1",
    ".interactor_label.marginTop:	1",
    ".interactor_label.marginRight:	1",
     NUL(char*)
};

Dictionary* Interactor::DragTypeDictionary = new Dictionary;

Interactor::Interactor(const char * name, InteractorInstance *ii) : 
   WorkSpaceComponent(name, FALSE), 
   DXDragSource(PrintCPBuffer)
{ 
    this->interactorInstance = ii; 
    this->label = this->customPart = NUL(Widget);
    this->currentLayout = ii->isVerticalLayout()?WorkSpaceComponent::VerticalUnset:
	WorkSpaceComponent::HorizontalUnset;
    this->drag_drop_wpid = 0;
}

Interactor::~Interactor() 
{ 
    if (this->drag_drop_wpid) XtRemoveWorkProc(this->drag_drop_wpid);
}
 
//
// Change any global attributes and then ask the derived class to
// change the attributes of the interactive part.
//
void Interactor::handleInteractorStateChange(
			InteractorInstance *src_ii, boolean major_change)
{
int userw, userh, curw, curh;

    this->handleInteractivePartStateChange(src_ii, FALSE);

#if 0
   //
   // If the current label is different from the new one, then set it.
   // Otherwise don't since it may cause the Interactor 'flash' on the
   // control panel.
   // FIXME: should do the same filtering that this->setLabel() does.
   //
   XmString xms; 
   char *curr_label;
   const char *new_label = this->interactorInstance->getInteractorLabel();
   XtVaGetValues(this->label, XmNlabelString, &xms, NULL);
   // The charset must match what was used when the XmString was created.
   XmStringGetLtoR(xms, "bold", &curr_label); 
   if (!EqualString(curr_label,new_label))
       this->setLabel(new_label, FALSE);
   delete curr_label;
   XmStringFree(xms);
#else
   // setLabel does nothing if oldlabel == newlabel
   const char *new_label = this->interactorInstance->getInteractorLabel();
   this->setLabel(new_label, FALSE);
#endif

   //
   // Remanage the interactor if the subclass'
   // handleInteractivePartStateChange() method unmanaged it.
   //
   if (!this->isManaged())
	this->manage();

   //
   // Growing and shrinking is a good thing, but don't get smaller than
   // the most recent user specified dimensions.
   //
   if (this->isUserSizeSet()) {
	this->getUserDimensions (&userw, &userh);
	this->GetDimensions (this->getRootWidget(), &curh, &curw);
	if ((curh!=userh) || (curw!=userw)) {
	    Boolean avr = True, ahr = True;
	    curw = MAX(curw, userw); curh = MAX(curh, userh);
	    XtVaGetValues (this->getRootWidget(), XmNallowVerticalResizing, &avr,
		XmNallowHorizontalResizing, &ahr, NULL);
	    if (avr) this->setXYSize (UIComponent::UnspecifiedDimension, curh);
	    if (ahr) this->setXYSize (curw, UIComponent::UnspecifiedDimension);
	}
   } 
}

 
void Interactor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT Interactor::InteractorClassInitialized)
    {
        Interactor::InteractorClassInitialized = TRUE;
	Interactor::DragIcon = 
                this->createDragIcon(ntractor_width, ntractor_height,
                                     (char *)ntractor_bits,
                                     (char *)ntractormask_bits);

	HiLites = OFFSET + 1;
#if GOING_OVERBOARD
	Display *d = theApplication->getDisplay();
	char *cp = XGetDefault (d, "dxui", "interactorHighlight");
	// don't modify cp
	if ((cp) && (cp[0])) {
	    sscanf (cp, "%d", &HiLites);
	}
#endif

	if ((theDXApplication->appAllowsRWConfig())&&
	    (theDXApplication->appAllowsSavingNetFile()) &&
	    (theDXApplication->appAllowsSavingCfgFile()) &&
	    (theDXApplication->appAllowsPanelEdit())) {
	    this->addSupportedType (Interactor::Modules, DXINTERACTORS, TRUE);
	}
	if (theDXApplication->appAllowsPanelEdit())
	    this->addSupportedType (Interactor::Trash, DXTRASH, FALSE);
    }
}


void Interactor::setSelected (boolean state)
{
InteractorInstance *ii = this->interactorInstance;

    if (state) ii->setSelected();
    else ii->clrSelected();
    this->WorkSpaceComponent::setSelected(state);
}

//
// Set the displayed label of the interactor
//
void Interactor::setLabel(const char *labelString, boolean )
{
XmStringContext cxt;
char *text, *tag;
XmStringDirection dir;
Boolean sep;
unsigned char rp;
int i, linesNew, linesOld;
XmString oldXmStr = NULL;

    if(NOT this->label)
	return;

    linesNew = linesOld = 1;
    XtVaGetValues (this->label, XmNlabelString, &oldXmStr, NULL);
    ASSERT(oldXmStr);

    char *filtered = WorkSpaceComponent::FilterLabelString (labelString);
    for (i=0; filtered[i]!='\0'; i++) if (filtered[i]=='\n') linesNew++;

#if !defined(XMSTRING_COMPARE_IS_BROKEN)
    // For efficiency only
    XmString tmpXmStr = XmStringCreateLtoR (filtered, "canvas");
    if (XmStringCompare (tmpXmStr, oldXmStr)) {
	XmStringFree (tmpXmStr);
	if (filtered) delete filtered;
	XmStringFree(oldXmStr);
	return ;
    }
    XmStringFree (tmpXmStr);
#endif

    if (XmStringInitContext (&cxt, oldXmStr)) {
	while (XmStringGetNextSegment (cxt, &text, &tag, &dir, &sep)) {
	    if (sep) linesOld++;
	    XtFree(text);
	    if (tag) XtFree(tag);
	}
	XmStringFreeContext(cxt);
    }
    XmStringFree(oldXmStr);

    // toggle XmNresizePolicy so that the interactor doesn't snap back
    // to a reasonable size.  If the old string is taller/shorter than the new string 
    // then let it change.  

    if (linesNew == linesOld) {
	XtVaGetValues (this->getRootWidget(), XmNresizePolicy, &rp, NULL); 
	XtVaSetValues (this->getRootWidget(), XmNresizePolicy, XmRESIZE_GROW, NULL);
    } else {
	this->resetUserDimensions();
    }

    if ((!filtered)||(!filtered[0])||(!strlen(filtered))) {
        this->setBlankLabelLayout(TRUE);
    } else {
        this->setBlankLabelLayout(FALSE);
	this->WorkSpaceComponent::SetLabelResource(this->label, labelString);
    }
    delete filtered;

    if (linesNew == linesOld)
	XtVaSetValues (this->getRootWidget(), XmNresizePolicy, rp, NULL);
}

void 
Interactor::openDefaultWindow() {this->interactorInstance->openSetAttrDialog();}

//
// Build an interactor widget tree.
//
void Interactor::createInteractor()
{
    int	                 n,x,y;
    Arg	                 wargs[30];
    const char		 *labelString;
    Widget		 parent;
    InteractorInstance   *ii = this->interactorInstance;

    if (NOT Interactor::InteractorClassInitialized) this->Interactor::initialize();

    this->workSpace = ii->getControlPanel()->getWorkSpace();
    parent = this->workSpace->getRootWidget();
    ASSERT(parent);

    //
    // Create the outermost frame.
    //
    ii->getXYPosition(&x,&y); 

    n = 0;
    XtSetArg(wargs[n], XmNx, x); n++;
    XtSetArg(wargs[n], XmNy, y); n++;
    XtSetArg(wargs[n], XmNuserData, (XtPointer)ii); n++;
    Widget standInRoot = XmCreateForm(parent, this->name, wargs, n);
    this->setRootWidget(standInRoot);
    this->setDragWidget(standInRoot);
    this->setDragIcon(Interactor::DragIcon);
    this->developerStyle = this->getControlPanel()->isDeveloperStyle();


    //
    // Restore width,height.  By checking allow{Vertical,Horizontal}Resizing
    // we're ignoring dimensions which aren't user configurable.  That way
    // we can read in .cfg files from other versions of dx without making
    // crazy looking interactors.  We always get the space we need.
    //
    int	width = 0;
    int height = 0;
    ii->getXYSize(&width,&height); 
    Boolean avr = True, ahr = True;
    XtVaGetValues (this->getRootWidget(), 
	XmNallowVerticalResizing, &avr,
	XmNallowHorizontalResizing, &ahr, 
    NULL);
    if ((!avr)||(!height)) height = UIComponent::UnspecifiedDimension;
    if ((!ahr)||(!width)) width = UIComponent::UnspecifiedDimension;
    if ((avr) || (ahr))
	this->setXYSize (width, height);

    n = 0;
    if (this->currentLayout & WorkSpaceComponent::Vertical) {
	XtSetArg (wargs[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg (wargs[n], XmNrightOffset, HiLites); n++;
	XtSetArg (wargs[n], XmNbottomAttachment, XmATTACH_NONE); n++;
    } else {
	XtSetArg (wargs[n], XmNrightAttachment, XmATTACH_NONE); n++;
	XtSetArg (wargs[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (wargs[n], XmNbottomOffset, HiLites); n++;
    }
    XtSetArg(wargs[n], XmNtopOffset, HiLites); n++;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset, HiLites); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNalignment, XmALIGNMENT_CENTER); n++;
    XtSetArg(wargs[n], XmNuserData, this); n++;
    this->label = XmCreateLabel(standInRoot, "interactor_label", wargs, n);
    XtManageChild (this->label);
    this->passEvents (this->label, TRUE);

    const char *new_label = this->interactorInstance->getInteractorLabel();
    if ((new_label) && (new_label[0]))
	this->WorkSpaceComponent::SetLabelResource(this->label, new_label);

    if (this->currentLayout & WorkSpaceComponent::Vertical) {
	XtSetArg (wargs[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg (wargs[n], XmNtopWidget, this->label); n++;
	XtSetArg (wargs[n], XmNleftOffset, HiLites); n++;
    } else {
	XtSetArg (wargs[n], XmNtopOffset, HiLites); n++;
	XtSetArg (wargs[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (wargs[n], XmNleftWidget, this->label); n++;
    }
    XtSetArg (wargs[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (wargs[n], XmNbottomOffset, HiLites); n++;
    XtSetArg (wargs[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (wargs[n], XmNrightOffset, HiLites); n++;
    XtSetArg (wargs[n], XmNuserData, this); n++;
    this->customPart = XmCreateForm(standInRoot, "interactive_form", wargs, n);
    XtManageChild(this->customPart);
    this->createInteractivePart(this->customPart);

    this->passEvents (this->customPart, FALSE);

    // 
    // Set the label after creating the interactive part in case the
    // sub-class wants to override setLabel().
    //
    labelString = ii->getInteractorLabel();
    ASSERT(labelString);
    this->setLabel(labelString, FALSE);

    // Set to ! because must be sure the function does something.
    this->currentLayout|= WorkSpaceComponent::NotSet;
    this->layoutInteractor();

    //
    // Register selection callback.
    //
    XmWorkspaceAddCallback (this->getRootWidget(), XmNselectionCallback,
	 (XtCallbackProc)Component_SelectWorkSpaceComponentCB, (void *)this);

    //
    // Make sure the interactive part contains the correct initial value.
    //
    this->updateDisplayedInteractorValue();

    this->setAppearance (this->developerStyle);

    this->installResizeHandler();

    //
    // Perform any actions that need to be done after the parent is managed.
    //
    this->completeInteractivePart();

    if ((avr)||(ahr)) 
	this->setUserDimensions (width, height);
}

//
// Pass button and motion events through to the parent of w.
//
void Interactor::passEvents(Widget w, boolean dnd)
{
    this->WorkSpaceComponent::passEvents (w, dnd);
    if (dnd) this->setDragWidget(w);
}


//
// O R I E N T A T I O N   O R I E N T A T I O N   O R I E N T A T I O N   
// O R I E N T A T I O N   O R I E N T A T I O N   O R I E N T A T I O N   
// O R I E N T A T I O N   O R I E N T A T I O N   O R I E N T A T I O N   
// O R I E N T A T I O N   O R I E N T A T I O N   O R I E N T A T I O N   
//
// It's no longer necessary to do arithmetic inside the Interactor.
// The original XmBulletinBoard implementation is replaced with an XmForm.
// So things lay themselves out and stay laid out.  The only quirky part is
// is the XmLabel widget which must be forced to the proper size the first
// time it appears on the screen.  When changing the layout simply change
// form attachments.
//
void Interactor::setBlankLabelLayout(boolean blank_label)
{
    if ((this->currentLayout & WorkSpaceComponent::NotSet) == 0) {
	if ((blank_label)&&
	    ((this->currentLayout & WorkSpaceComponent::BlankLabel)!=0)) return ;
	if ((!blank_label)&&
	    ((this->currentLayout & WorkSpaceComponent::BlankLabel)==0)) return ;
    } else {
	if (blank_label) this->currentLayout|= WorkSpaceComponent::BlankLabel;
	else this->currentLayout&= ~WorkSpaceComponent::BlankLabel;
	return ;
    }

    InteractorInstance *ii = this->interactorInstance;
    if (blank_label) {
	ii->setVerticalLayout(TRUE);
	this->currentLayout|= WorkSpaceComponent::BlankLabel;
	this->layoutInteractor();
    } else {
	this->currentLayout&= ~WorkSpaceComponent::BlankLabel;
	this->currentLayout|= WorkSpaceComponent::NotSet;
	this->layoutInteractor();
    }
}
void Interactor::setVerticalLayout(boolean vertical)
{
    if ((this->currentLayout & WorkSpaceComponent::NotSet) == 0) {
	if ((vertical)&&(this->currentLayout & WorkSpaceComponent::Vertical)) return ;
	if ((!vertical)&&(this->currentLayout & WorkSpaceComponent::Horizontal)) return ;
    }

    this->WorkSpaceComponent::setVerticalLayout(vertical);
    this->layoutInteractor();
}

void Interactor::layoutInteractor()
{
    this->resetUserDimensions();

    if (this->currentLayout & WorkSpaceComponent::BlankLabel)
        this->layoutInteractorWithNoLabel();
    else if (this->currentLayout & WorkSpaceComponent::Vertical) 
        this->layoutInteractorVertically();
    else if (this->currentLayout & WorkSpaceComponent::Horizontal)
        this->layoutInteractorHorizontally();
    else
	ASSERT(0);
    this->currentLayout&= ~WorkSpaceComponent::NotSet;
}


//
// WRT layoutInteractorHorizontally() and Vertically()...
// the order in which you make calls to set form constraints is important.
// If you change the order to can wind up with either error message to stderr,
// or a mess inside an interactor.  Be especially careful with margins.
//

void Interactor::layoutInteractorHorizontally()
{
boolean label_managed = XtIsManaged(this->label);

    if (!label_managed)  XtManageChild (this->label);

    //
    // disconnect
    //
    XtVaSetValues (this->customPart, 
	XmNtopAttachment, XmATTACH_NONE,
    NULL);
    XtVaSetValues (this->label, 
	XmNrightAttachment, XmATTACH_NONE, 
    NULL);

    //
    // reconnect
    //
    XtVaSetValues (this->customPart, 
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNleftWidget, this->label,
	XmNtopOffset, HiLites, 
	XmNleftOffset, 0, 
    NULL);
    XtVaSetValues (this->label, 
	XmNbottomAttachment, XmATTACH_FORM, 
	XmNbottomOffset, HiLites, 
	XmNleftOffset, HiLites, 
	XmNtopOffset, HiLites,
    NULL);
}

void Interactor::layoutInteractorVertically()
{
boolean label_managed = XtIsManaged(this->label);

    if (!label_managed)  {
	XtManageChild (this->label);
    }

    //
    // disconnect
    //
    XtVaSetValues (this->label, 
	XmNbottomAttachment, XmATTACH_NONE, 
	XmNbottomOffset, 0, 
    NULL);
    XtVaSetValues (this->customPart, 
	XmNleftAttachment, XmATTACH_NONE, 
	XmNleftOffset, HiLites, 
	XmNtopAttachment, XmATTACH_NONE,
    NULL);

    //
    // reconnect
    //
    XtVaSetValues (this->label, 
	XmNrightAttachment, XmATTACH_FORM, 
	XmNrightOffset, HiLites, 
    NULL);
    XtVaSetValues (this->customPart, 
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, this->label,
	XmNtopOffset, 0, 
	XmNleftAttachment, XmATTACH_FORM,
    NULL);
}

void Interactor::layoutInteractorWithNoLabel()
{
    //
    // disconnect
    //
    XtVaSetValues (this->customPart,
	XmNtopAttachment, XmATTACH_NONE,
    NULL);
    if (XtIsManaged(this->label)) {
	XtUnmanageChild (this->label);
	XtVaSetValues (this->getRootWidget(), XmNheight, 0, NULL);
    }

    //
    // reconnect
    //
    XtVaSetValues (this->customPart,
	XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, HiLites,
	XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, HiLites,
    NULL);
}

// D R A G - N - D R O P --- R E L A T E D   S T U F F --- D R A G - N - D R O P   
// D R A G - N - D R O P --- R E L A T E D   S T U F F --- D R A G - N - D R O P   
// D R A G - N - D R O P --- R E L A T E D   S T U F F --- D R A G - N - D R O P   
// D R A G - N - D R O P --- R E L A T E D   S T U F F --- D R A G - N - D R O P   
//
// ...requires only that it be selected
//
int Interactor::decideToDrag(XEvent *xev)
{
    InteractorInstance *ii = this->interactorInstance;
    if (!this->developerStyle) return DragSource::Abort;
    if (!ii->selected) return DragSource::Inactive;

    Display *d = XtDisplay(this->getRootWidget());
    Atom xoff = XInternAtom (d, DXXOFFSET, False);
    Atom yoff = XInternAtom (d, DXYOFFSET, False);
    Screen *screen = XtScreen(this->getRootWidget());
    Window root = RootWindowOfScreen(screen);
    int x = 0;
    int y = 0;
    ii->getXYPosition (&x, &y);
    int minx = x;
    int miny = y;
    int junk;

    this->workSpace->getSelectedBoundingBox (&minx, &miny, &junk, &junk);

    int xoffset = x - minx;
    int yoffset = y - miny;

    if (xev->type == ButtonPress) {
	xoffset+= xev->xbutton.x;
	yoffset+= xev->xbutton.y;
    }

    XChangeProperty (d, root, xoff, XA_INTEGER, 32, PropModeReplace, 
	(unsigned char*)&xoffset, 1);
    XChangeProperty (d, root, yoff, XA_INTEGER, 32, PropModeReplace, 
	(unsigned char*)&yoffset, 1);

    return DragSource::Proceed;
}

//
// The header (which will not end up being part of the files) says
// "hostname:pid, net length = %d, cfg length = %d\n"
//
boolean Interactor::createNetFiles(Network *netw, FILE *netf, char *cfgfile)
{
   netw->setCPSelectionOwner(this->getControlPanel());
   return (boolean)this->DXDragSource::createNetFiles (netw, netf, cfgfile);
}

//
// if the operation was a move and not a copy, then delete
// the original interactors.
//
void Interactor::dropFinish (long operation, int tag, unsigned char status)
{
    //
    // Delete the atoms set up when starting the drag
    //
    Display *d = XtDisplay(this->getRootWidget());
    Atom xoff_atom = XInternAtom (d, DXXOFFSET, True);
    Atom yoff_atom = XInternAtom (d, DXYOFFSET, True);
    Screen *screen = XtScreen(this->getRootWidget());
    Window root = RootWindowOfScreen(screen);
    if (xoff_atom != None)
	XDeleteProperty (d, root, xoff_atom);
    if (yoff_atom != None)
	XDeleteProperty (d, root, yoff_atom);


    // If the operation was a copy and the type was Trash,
    // then treat it would be better to treat it like a move
    // so that the user isn't required to use the Shift key.

    if (status) {
	if ((operation == XmDROP_MOVE) || (tag == Interactor::Trash)) {
	    //this->getControlPanel()->deleteSelectedInteractors();
	    XtAppContext apcxt = theApplication->getApplicationContext();
	    this->drag_drop_wpid = XtAppAddWorkProc (apcxt, Interactor_DragDropWP,
		    (XtPointer)this);
	}
    }
}

boolean Interactor::decodeDragType (int tag,
	char * a, XtPointer *value, unsigned long *length, long operation)
{
boolean retVal;

    switch (tag) {
	case Interactor::Modules:
	    // this-convert() comes from DXDragSource and can't be overridden.
	    retVal = this->convert (this->getNetwork(), a, value, length, operation);
	    break;

	// Don't do anything...  Just say OK.  this->dropFinish does the delete
	case Interactor::Trash:
	    retVal = TRUE;

	    // dummy pointer
	    *value = (XtPointer)malloc(4);
	    *length = 4;
	    break;

	default:
	    retVal = FALSE;
	    break;
    }
    return retVal;
}


// U T I L S   U T I L S   U T I L S   U T I L S   U T I L S   U T I L S
// U T I L S   U T I L S   U T I L S   U T I L S   U T I L S   U T I L S
// U T I L S   U T I L S   U T I L S   U T I L S   U T I L S   U T I L S
// U T I L S   U T I L S   U T I L S   U T I L S   U T I L S   U T I L S
// U T I L S   U T I L S   U T I L S   U T I L S   U T I L S   U T I L S
// U T I L S   U T I L S   U T I L S   U T I L S   U T I L S   U T I L S
boolean Interactor::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassInteractor);
    if (s == classname) return TRUE;
    return this->WorkSpaceComponent::isA(classname);
}

//
// Indicate that the interactor is selected. 
//
void Interactor::indicateSelect(boolean selected)
{
    if (this->getRootWidget()) {
	XtVaSetValues(this->getRootWidget(),
        	XmNselected, (selected ? True : False),
		NULL);
    }
}

const char *Interactor::getComponentHelpTopic()
{
    static char topic[100];
    sprintf(topic, "%sInteractor",
		this->getNode()->getNameString());
    return topic;
}

void *GetUserData(Widget widget)
{
    XtPointer data;
    ASSERT(widget);
    XtVaGetValues(widget, XmNuserData, &data, NULL);
    return (data);
}

Network *Interactor::getNetwork()
{ 
    ASSERT(this->interactorInstance);
    return this->interactorInstance->getNetwork();
}
Node *Interactor::getNode()
{ 
    ASSERT(this->interactorInstance);
    return this->interactorInstance->getNode();
}
ControlPanel *Interactor::getControlPanel()
{
    ASSERT(this->interactorInstance);
    return this->interactorInstance->getControlPanel();
}

extern "C" Boolean Interactor_DragDropWP(XtPointer clientData)
{
    Interactor* ntr = (Interactor*)clientData;
    ntr->drag_drop_wpid = 0;
    ntr->getControlPanel()->deleteSelectedInteractors();
    return TRUE;
}
