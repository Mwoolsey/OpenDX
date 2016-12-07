/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Label.h>
#include <Xm/DragDrop.h>


#include "XmUtility.h"
#include "DXStrings.h"
#include "Decorator.h"
#include "WorkSpace.h"
#include "DecoratorStyle.h"
#include "DXApplication.h"
#include "../widgets/WorkspaceW.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include "WarningDialogManager.h"
#include "Application.h" //for theApplication->getDisplay()
#include "DecoratorInfo.h"
#include "ListIterator.h"

boolean Decorator::DecoratorClassInitialized = FALSE;

String Decorator::DefaultResources[] =
{
  ".allowVerticalResizing:	False",
  ".allowHorizontalResizing:	True",
  NUL(char*)
};

#define DXXOFFSET "DX_XOFFSET"
#define DXYOFFSET "DX_YOFFSET"


//
// The colors 
//
const char *Decorator::ColorNames[] = {
    "Red",      "Grey",         "Black",        "Blue",
    "Green",    "Yellow",       "Purple",       "Medium Blue",
    "Brown",    "Neon Green",
    DEFAULT_SETTING, NUL(const char *)
};
const char *Decorator::ColorValues[] = {
    "#b30000",  "#505050",      "#000",         "#0021ff",
    "#007000",  "#ff0",         "#a000ff",      "#1340b3",
    "#805009",  "#0f0",
    "Black", NUL(const char *)
};
 



//
// This defines the thickness of the white line drawn around a selected decorator
//
// HiLites is initialized to OFFSET and modified with the contents of
// dxui.decoratorHighlight if specified in .Xdefaults
#define OFFSET 1
int Decorator::HiLites = OFFSET + 1;

//
// Supply 2 constructors so that its convenient to create one of
// these things before there is a parent.  The parent can be supplied
// now, or later through the manage function.
//
Decorator::Decorator(void *wClass, const char *name, 
	boolean developerStyle) : WorkSpaceComponent(name, developerStyle),
	DXDragSource(PrintCPBuffer)
{
    this->workSpace = NUL(WorkSpace*);
    this->widgetClass = (WidgetClass)wClass;
    this->customPart = NUL(Widget);
    this->x = this->y = UIComponent::UnspecifiedPosition;
    this->width = this->height = UIComponent::UnspecifiedDimension;
    this->dndInfo = NUL(DecoratorInfo*);

    if (NOT Decorator::DecoratorClassInitialized)
    {
	Decorator::DecoratorClassInitialized = TRUE;

	Decorator::HiLites = OFFSET + 1;
#if NEED_TO_GO_OVERBOARD
	Display *d = theApplication->getDisplay();
	char *cp = XGetDefault (d, "dxui", "decoratorHighlight");
	// don't modify cp
	if ((cp) && (cp[0])) {
	    sscanf (cp, "%d", &HiLites);
	}
#endif
    }
}

Decorator::~Decorator()
{
    if (this->dndInfo)
	delete this->dndInfo;
}
 
 
void Decorator::manage(WorkSpace *workSpace)
{

    if (this->customPart) {
	this->WorkSpaceComponent::manage();
    } else {
	ASSERT(!this->workSpace);
	ASSERT(workSpace);
	this->workSpace = workSpace;
	this->createDecorator();
	this->WorkSpaceComponent::manage();
    }
}

void Decorator::uncreateDecorator()
{
    //
    // When we destroy the widget, this->root will automatically be set to
    // NUL(Widget) and WorkSpaceComponent::widgetDestroyed() will set root
    // to NUL(Widgdet) in every object in this->setResourceList.
    //
    XtDestroyWidget (this->getRootWidget());
    this->customPart = NUL(Widget);
    this->workSpace = NUL(WorkSpace*);
    this->selected = FALSE;
}

//
// Build an interactor widget tree.
//
void Decorator::createDecorator()
{
    int			 n;
    Arg			 args[20];

    ASSERT(this->workSpace);

    this->initialize();

    n = 0;
    if ((this->x)&&(this->x != UIComponent::UnspecifiedPosition)) 
	{ XtSetArg (args[n], XmNx, this->x); n++; }
    if ((this->y)&&(this->y != UIComponent::UnspecifiedPosition)) 
	{ XtSetArg (args[n], XmNy, this->y); n++; }
    if (this->resizeOnUpdate()==FALSE) {
	if ((this->width)&&(this->width != UIComponent::UnspecifiedDimension)) 
	    { XtSetArg (args[n], XmNwidth, this->width); n++; }
	if ((this->height)&&(this->height != UIComponent::UnspecifiedDimension)) 
	    { XtSetArg (args[n], XmNheight, this->height); n++; }
    }
    XtSetArg (args[n], XmNselected, this->selected); n++;
    XtSetArg (args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
    Widget form = XmCreateForm(this->workSpace->getRootWidget(), this->name, args, n);
    this->setRootWidget (form);

    this->installResizeHandler();
    XmWorkspaceAddCallback (form, XmNselectionCallback,
	 (XtCallbackProc)Component_SelectWorkSpaceComponentCB, (void *)this);

    if ((this->width) && (this->height) && (this->resizeOnUpdate()==FALSE) &&
	(this->width  != UIComponent::UnspecifiedDimension) && 
	(this->height != UIComponent::UnspecifiedDimension))  {
	this->setXYSize(this->width, this->height);
	XtVaSetValues (form, XmNresizePolicy, XmRESIZE_NONE, NULL);
    } else
	XtVaSetValues (form, XmNresizePolicy, XmRESIZE_ANY, NULL);

    n = 0;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    this->setArgs (args, &n);
    this->customPart = XtCreateManagedWidget ("decorator", this->widgetClass,form,args,n);
    this->passEvents (this->customPart, FALSE);

    //
    // this loop must be run before calling setAppearance because setAppearance
    // runs thru the list of resource settings.
    //
    if (this->setResourceList) {
	ListIterator it(*this->setResourceList);
	DynamicResource* dr;
	while ( (dr = (DynamicResource*)it.getNext()) ) {
	    // don't bother checking return value from setRootWidget because it will
	    // always return FALSE for resources belonging to xmLabelWidgetClass if
	    // there is no XmLabel widget already in its list.  Not a problem.
	    dr->setRootWidget(this->getRootWidget());
	}
    }

    this->setAppearance (this->developerStyle);
    this->completeDecorativePart();
    XtVaSetValues (form, XmNresizePolicy, XmRESIZE_ANY, NULL);
}


boolean Decorator::printComment (FILE *f)
{
int x,y,w,h;

    this->getXYPosition (&x,&y);
    this->getXYSize (&w,&h);
    if (fprintf (f, "    //\n    // decorator %s\tpos=(%d,%d) size=%dx%d style(%s)", 
	this->style->getKeyString(), x,y,w,h, this->style->getNameString()) < 0)
	return FALSE;
    return TRUE;
}

boolean Decorator::parseComment (const char *comment, const char *file, int line)
{
int items_parsed;
char decoStyle[128];
char stylename[128];
int x,y,w,h;
boolean parsed = FALSE;

    this->x = UIComponent::UnspecifiedPosition;
    this->y = UIComponent::UnspecifiedPosition; 
    this->width = UIComponent::UnspecifiedDimension; 
    this->height = UIComponent::UnspecifiedDimension; 

    items_parsed = 
	sscanf (comment, " decorator %[^\t]\tpos=(%d,%d) size=%dx%d style(%[^)])",
	    stylename, &x,&y,&w,&h, decoStyle);
    if (items_parsed == 6) parsed = TRUE;

    if (!parsed)
	items_parsed = sscanf (comment, " decorator %[^\t]\tpos=(%d,%d) size=%dx%d",
	    stylename, &x,&y,&w,&h);
    if (items_parsed == 5) parsed = TRUE;

    //
    // For 3 weeks I made dxui print comments in the following format.  Now
    // I want to be able to parse them so that nets made during that time still
    // work.  Next month, erase this.  I made this change to decorator comments
    // so that the new comment format would be recognizable to 3.1.2.
    //
#ifndef ERASE_ME
    if (!parsed)
	items_parsed = 
	    sscanf (comment, " decorator %[^\t]\tstyle(%[^)]) pos=(%d,%d) size=%dx%d",
		stylename, decoStyle, &x,&y,&w,&h);
    if (items_parsed == 6) parsed = TRUE;
#endif

    if (!parsed) {
	WarningMessage ("Unrecognized Decorator Comment (file %s, line %d)... continuing",
		file, line);
	return TRUE;
    }

    if (strcmp (stylename, this->style->getKeyString()) != 0) {
	WarningMessage ("Unrecognized Decorator Comment (file %s, line %d)... continuing",
		file, line);
	return TRUE;
    }

    this->x = x; this->y = y;
    this->width = w; this->height = h;

    return TRUE;
}

//
// Determine if this Component is of the given class.
//
boolean Decorator::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassDecorator);
    if (s == classname) return TRUE;
    return this->WorkSpaceComponent::isA(classname);
}

void Decorator::openDefaultWindow()
{
    WarningMessage("Decorators do not have 'Set Attributes...' dialogs");
}

void Decorator::setXYPosition (int x, int y)
{
    this->x = x; this->y = y;
    if (this->customPart) this->WorkSpaceComponent::setXYPosition (x,y);
}

void Decorator::setXYSize (int w, int h)
{
    this->width = w; this->height = h;
    if (this->customPart) this->WorkSpaceComponent::setXYSize (w,h);
}

void Decorator::setAppearance (boolean developerStyle)
{
    // Let WorkSpaceComponent provide the visuals
    this->WorkSpaceComponent::setAppearance (FALSE);

    // Then go back and adjust internal values
    this->developerStyle = developerStyle;
    Widget root = this->getRootWidget();
    if (!root) return ;
    XtVaSetValues (root, XmNselectable, developerStyle, NULL);
}

void Decorator::getXYSize (int *w, int *h)
{
    if (this->getRootWidget())
	this->WorkSpaceComponent::getXYSize (w,h);
    else {
	*w = this->width;
	*h = this->height;
    }
}

void Decorator::getXYPosition (int *x, int *y)
{
    if (this->getRootWidget())
	this->WorkSpaceComponent::getXYPosition (x,y);
    else {
	*x = (int)this->x;
	*y = (int)this->y;
    }
}

//
// if the operation was a move and not a copy, then delete
// the original interactors.
//
void Decorator::dropFinish (long operation, int tag, unsigned char status)
{
    // If the operation was a copy and the type was Trash,
    // then treat it would be better to treat it like a move
    // so that the user isn't required to use the Shift key.

    if (status) {
	if ((operation == XmDROP_MOVE) || (tag == Decorator::Trash)) {
	    DecoratorInfo *dnd = this->dndInfo;
	    if (dnd) dnd->dlete(dnd->ownerObj);
	}
    }
}

//
// The header (which will not end up being part of the files) says
// "hostname:pid, net length = %d, cfg length = %d\n"
//
boolean Decorator::createNetFiles(Network *netw, FILE *netf, char *cfgfile)
{
    if (this->dndInfo) {
	DecoratorInfo *dnd = this->dndInfo;
	dnd->dragPrep (dnd->ownerObj);
    }
    return (boolean)this->DXDragSource::createNetFiles (netw, netf, cfgfile);
}

void Decorator::setSelected(boolean state)
{
    this->WorkSpaceComponent::setSelected(state);
    if ((this->dndInfo) && (this->dndInfo->select)) {
	DecoratorInfo *dnd = this->dndInfo;
	dnd->select (dnd->ownerObj);
    }
}
 
int
Decorator::decideToDrag (XEvent *xev)
{
    if (!this->selected) return DragSource::Inactive;


    Display *d = XtDisplay(this->getRootWidget());
    Atom xoff = XInternAtom (d, DXXOFFSET, False);
    Atom yoff = XInternAtom (d, DXYOFFSET, False);
    Screen *screen = XtScreen(this->getRootWidget());
    Window root = RootWindowOfScreen(screen);
    int x = 0;
    int y = 0;
    this->getXYPosition (&x, &y);
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

Network* Decorator::getNetwork()
{
DecoratorInfo *dnd = this->dndInfo;

    if (!dnd) return NUL(Network*);
    return dnd->net;
}

void Decorator::setDecoratorInfo(DecoratorInfo *oi)
{ 
    if (this->dndInfo) delete this->dndInfo; 
    this->dndInfo = oi; 
}

boolean Decorator::parseResourceComment (const char *comment, const char *f, int l)
{
DynamicResource *dr;
 
    // If there is no this->customPart available now, then the rootWidget will be
    // set into the DynamicResource in ::completeDecorativePart(). If it is available
    // then you must supply it now because ::completeDecorativePart() wouldn't be called.
    if (this->customPart)
	dr = new DynamicResource(NUL(char*), this->getRootWidget());
    else
	dr = new DynamicResource(NUL(char*), NUL(Widget));
 
    // If the resource comment was successfully parsed, then add the
    // DynamicResource to our list.  Since there are no widgets at this point,
    // some method down the road will have to do dr->setRootWidget for the
    // DynamicResource list to take effect.
    if (dr->parseComment (comment, f, l)) {
	if (!this->setResourceList) {
	    this->setResourceList = new List;
	}
	this->setResourceList->appendElement((void *)dr);
	return TRUE;
    } 
    delete dr;
    return FALSE;
}

//
// An value argument of NUL or "" means delete the resource setting.
//
void
Decorator::setResource (const char *res, const char *val)
{
ListIterator it;
DynamicResource *dr=NUL(DynamicResource*);
 
    if (!this->setResourceList) {
	if ((!val)||(!val[0])) return ;
	this->setResourceList = new List;
    }
 
    it.setList (*this->setResourceList);
    while ( (dr = (DynamicResource *)it.getNext()) ) {
	if (!strcmp (dr->getResourceName(), res)) {
	    break;
	}
    }
 
    boolean applyOK=TRUE;
    if (!dr) {
	if ((!val)||(!val[0])) return ;
	dr = new DynamicResource (res, this->getRootWidget());
	applyOK = dr->addWidgetToNameList (this->customPart);
	this->setResourceList->appendElement((void *)dr);
    }
    if ((val)&&(val[0])) {
	if (applyOK)
	    applyOK = dr->setData (val);
    } else {
	applyOK = FALSE;
    }
 
    if (!applyOK) {
	this->setResourceList->removeElement((void*)dr);
	delete dr;
    }
}
 

int Decorator::CountLines (const char* str)
{
    if ((!str) || (!str[0])) return 0;
    if ((str[0] == '\\') && (str[1] == '0')) return 0;
    int i;
    int lines = 1;
    for (i = 0; str[i] != '\0'; i++)
	if ((str[i] == 'n') && (i>0) && (str[i-1] == '\\'))
	    lines++;
    return lines;
}

const char* Decorator::FetchLine (const char* str, int line)
{
    static char lbuf[256];
    lbuf[0] = '\0';
    ASSERT(str);
    int len = strlen(str);
    char* head = (char*)&str[0];
    char* tail = (char*)&str[len];
    int current_line = 1;
    char* cp = head;
    int next_slot = 0;
    while ((cp != tail) && (current_line <= line)) {
	if (*cp == '\\') {
	    cp++;
	    if (cp != tail) {
		if (*cp == 'n')
		    current_line++;
		cp++;
	    }
	    continue;
	}
	if (current_line == line) {
	    if (*cp == '"') 
		lbuf[next_slot++] = '\\';
	    lbuf[next_slot++] = *cp;
	    lbuf[next_slot] = '\0';
	}
	cp++;
    }
    return lbuf;
}


boolean Decorator::printJavaResources (FILE* jf, const char* indent, const char* var)
{
    //
    // Make an attempt at reflecting resource settings in java
    //
    if (this->setResourceList) {
	ListIterator it(*this->setResourceList);
	DynamicResource* dr;
	while ( (dr = (DynamicResource*)it.getNext()) ) {
	    if ((EqualString(dr->getResourceName(), XmNforeground)) ||
		(EqualString(dr->getResourceName(), XmNbackground))) {
		boolean ok_to_set = FALSE;
		const char* clr = dr->getStringRepresentation();
		if ((!clr) || (!clr[0])) continue;
		clr++;
		int bytes_per_color = strlen(clr) / 3;
		char red[3], blue[3], green[3];
		int redh, blueh, greenh;
		red[2] = '\0';    
		green[2] = '\0';
		blue[2] = '\0';  
		switch (bytes_per_color) {
		    case 1:
			red[0] = red[1] = clr[0]; 
			green[0] = green[1] = clr[1]; 
			blue[0] = blue[1] = clr[2]; 
			ok_to_set = TRUE;
			break;
		    case 2:
			red[0] = clr[0]; red[1] = clr[1];
			green[0] = clr[2];  green[1] = clr[3];
			blue[0] = clr[4]; blue[1] = clr[5];
			ok_to_set = TRUE;
			break;
		    case 3:
			red[0] = clr[0]; red[1] = clr[1];
			green[0] = clr[3];  green[1] = clr[4];
			blue[0] = clr[6]; blue[1] = clr[7];
			ok_to_set = TRUE;
			break;
		    case 4:
			red[0] = clr[0]; red[1] = clr[1];
			green[0] = clr[4];  green[1] = clr[5];
			blue[0] = clr[8]; blue[1] = clr[9];
			ok_to_set = TRUE;
			break;
		    default:
			break;
		}
		if (ok_to_set) {
		    if ((sscanf (red, "%x", &redh) == 1) &&
			(sscanf (green, "%x", &greenh) == 1) &&
			(sscanf (blue, "%x", &blueh) == 1))
			if (EqualString(dr->getResourceName(), XmNforeground))
			    fprintf (jf, "%s%s.setForeground(new Color(%d, %d, %d));\n",
				indent, var, redh, greenh, blueh);
			else
			    fprintf (jf, "%s%s.setBackground(new Color(%d, %d, %d));\n",
				indent, var, redh, greenh, blueh);
		}
	    }
	}
    }
    return TRUE;
}
