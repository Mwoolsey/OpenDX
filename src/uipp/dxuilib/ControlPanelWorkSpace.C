/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#include <Xm/CutPaste.h>
#include <Xm/DragC.h>
#include <X11/cursorfont.h>
#include "ControlPanel.h"
#include "DXDropSite.h"
#include "ControlPanelWorkSpace.h" 
#include "DXApplication.h" 
#include "Node.h" 
#include "Network.h" 
#include "DecoratorStyle.h"
#include "DecoratorInfo.h"
#include "Decorator.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"

#include "../widgets/WorkspaceW.h"

// gethostname is needed by nodeTransferCB which is part of drag-n-drop
#if defined(NEEDS_GETHOSTNAME_DECL)
extern "C" int gethostname(char *address, int address_len);
#endif

#if 0
#if defined(DXD_WIN)
#include <winsock.h>
#endif
#endif

#define DXINTERACTORS "DXINTERACTORS"
#define DXINTERACTOR_NODES "DXINTERACTOR_NODES"
#define DXXOFFSET "DX_XOFFSET"
#define DXYOFFSET "DX_YOFFSET"


boolean ControlPanelWorkSpace::ControlPanelWorkSpaceClassInitialized = FALSE;
Dictionary* ControlPanelWorkSpace::DropTypeDictionary = new Dictionary;

String ControlPanelWorkSpace::DefaultResources[] =
{
     ".width:       		500",
     ".height:      		500",
     ".accentColor:      	#dddddddddddd",
     ".fractionBase:		1200",
     NUL(char*)
};

ControlPanelWorkSpace:: ControlPanelWorkSpace(const char *name, 
                          Widget parent, WorkSpaceInfo *info, 
                          ControlPanel *cp) : 
			WorkSpace(name,parent, info),
                     DXDropSite(TRUE)
  
{ 
    this->controlPanel = cp; 
}

//
// Done for a double click in the work space
//
void ControlPanelWorkSpace::doDefaultAction(Widget widget, XtPointer callData)
{
    this->controlPanel->doDefaultWorkSpaceAction();
}

void ControlPanelWorkSpace::doSelectionAction(Widget widget, XtPointer callData)
{
    this->controlPanel->setActivationOfEditingCommands();
}

//
// Done for a single click in the work space
//
void ControlPanelWorkSpace::doBackgroundAction(Widget w, XtPointer callData)
{
    this->controlPanel->placeSelectedInteractorCallback(w, callData);
}


void ControlPanelWorkSpace::initializeRootWidget(
		boolean fromFirstIntanceOfADerivedClass)
{
    if (NOT ControlPanelWorkSpace::ControlPanelWorkSpaceClassInitialized ||
        fromFirstIntanceOfADerivedClass) {

        ASSERT(theApplication);
        // Load our default resources first...
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ControlPanelWorkSpace::DefaultResources);

	// ...and force the parents default resources to be loaded.
        fromFirstIntanceOfADerivedClass = TRUE;

        ControlPanelWorkSpace::ControlPanelWorkSpaceClassInitialized = TRUE;

	this->addSupportedType (ControlPanelWorkSpace::Interactors, 
	    DXINTERACTORS, TRUE);
	this->addSupportedType (ControlPanelWorkSpace::Nodes,
	    DXINTERACTOR_NODES, FALSE);
#if defined(WANT_DND_FOR_TEXT)
	this->addSupportedType (ControlPanelWorkSpace::Text,
	    "COMPOUND_TEXT", FALSE);
#endif
    }
   
    // Now initialize the parent, thereby building the widget tree.
    this->WorkSpace::initializeRootWidget(fromFirstIntanceOfADerivedClass);

    if (!theDXApplication->appAllowsInteractorSelection()) {
	XtVaSetValues(this->getRootWidget(),
			XmNselectable, False,
			NULL);
    }

    if (!theDXApplication->appAllowsInteractorMovement()) {
	XtVaSetValues(this->getRootWidget(),
			XmNallowMovement, False,
			NULL);
    }

    // try to improve efficiency, although it might not be possible to
    // set developerStyle this early.
    if (!this->controlPanel->isDeveloperStyle()) {
	XtVaSetValues (this->getRootWidget(), XmNallowOverlap, True, NULL);
    }

    this->setDropWidget (this->getRootWidget(), XmDROP_SITE_COMPOSITE);
    this->dropPermission = TRUE;
}

//
// ...when "this" is on the receiving end of a drag-n-drop
//
boolean ControlPanelWorkSpace::mergeNetElements 
	(Network *, List *tmppanels, int x, int y)
{
int x2use = x;
int y2use = y;

    if (!this->dropPermission) return FALSE;
    if (!XmWorkspaceLocationEmpty (this->getRootWidget(), x,y)) return FALSE;
    ASSERT (tmppanels);
    ASSERT (tmppanels->getSize() == 1);
    ControlPanel *panel = (ControlPanel *)tmppanels->getElement(1);
    ASSERT (panel);

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    int *xoff_value = NULL;
    int *yoff_value = NULL;
    //
    // Fetch the DX_{X,Y}OFFSET values and adjust x,y
    // according to where OFFSETS fit into the net.
    //
    Display *d = XtDisplay(this->getRootWidget());
    Screen *screen = XtScreen(this->getRootWidget());
    Window root = RootWindowOfScreen(screen);
    Atom xoff = XInternAtom (d, DXXOFFSET, True);
    Atom yoff = XInternAtom (d, DXYOFFSET, True);
    if ((xoff != None) && (yoff != None)) {
	XGetWindowProperty (d, root, xoff, 0, 1, False, XA_INTEGER, &actual_type,
	    &actual_format, &nitems, &bytes_after, (unsigned char **)&xoff_value);
	XGetWindowProperty (d, root, yoff, 0, 1, False, XA_INTEGER, &actual_type,
	    &actual_format, &nitems, &bytes_after, (unsigned char **)&yoff_value);

	if ((xoff_value) && (yoff_value)) {
	    x2use = x - *xoff_value;
	    y2use = y - *yoff_value;
	    x2use = (x2use<0?0:x2use);
	    y2use = (y2use<0?0:y2use);

	    XFree(xoff_value);
	    XFree(yoff_value);
	}
    }


    this->controlPanel->mergePanels (panel, x2use, y2use);
    this->controlPanel->getNetwork()->setCPSelectionOwner((ControlPanel *)0);
    return TRUE;
}

// FIXME
// "value" is Compound Text -- in this case should be a multi-byte string.
// For now assume that it's a null-terminated char *, which it will be
// in North America and western Europe.  (I understand the dx script sets LANG to C.)
//
#if !defined(WANT_DND_FOR_TEXT)
boolean ControlPanelWorkSpace::compoundTextTransfer 
	(char *, XtPointer , unsigned long , int , int ) { return FALSE; }
#else
boolean ControlPanelWorkSpace::compoundTextTransfer 
	(char *, XtPointer value, unsigned long len, int x, int y)
{
// The workspace is registered as a dropsite for COMPOUND_TEXT.  That happens
// when a DropSite object is constructed.  If this code is enabled, it will
// create LabelDecorator objects in the workspace with the text that is 
// dropped onto the workspace.  See the array up at the top if you want to use this.
Decorator *d = 0;
char *cp = (char *)value;
int i, j;

    if (!this->dropPermission) return False;

    int reallen = len;
    for (i=0; i<len; i++) if (cp[i] == '\n') reallen++;
    char *saveStr = new char[reallen+1]; 
    j = 0;
    for (i=0; i<len; i++) {
	if (cp[i] == '\n') {
	    saveStr[j++] = '\\';
	    saveStr[j++] = 'n';
	} else
	    saveStr[j++] = cp[i];
    }
    saveStr[j] = '\0';

    Dictionary *dict;
    dict = DecoratorStyle::GetDecoratorStyleDictionary("Label");
    ASSERT(dict);

    DictionaryIterator di(*dict);
    DecoratorStyle *ds;
    ds = (DecoratorStyle*)di.getNextDefinition();
    ASSERT(ds);
    ControlPanel *ctrlp = this->controlPanel;
    d = ds->createDecorator(ctrlp->isDeveloperStyle());
    ASSERT(d);
    d->setStyle (ds);

    DecoratorInfo *dnd = new DecoratorInfo (ctrlp->getNetwork(), (void*)ctrlp,
	(DragInfoFuncPtr)ControlPanelWorkSpace::SetOwner,
	(DragInfoFuncPtr)ControlPanelWorkSpace::DeleteSelections);
    d->setDecoratorInfo(dnd);
    ctrlp->getNetwork()->setFileDirty();

    d->setXYPosition(x,y);
    d->setLabel(saveStr);

    d->manage(this);
    ctrlp->addComponentToList ((void *)d);
    delete saveStr;
    return TRUE;
}
#endif


//
// {width,height}Delta between Workspace widget and dialog box.
// If one widget changes size, then recalculate them.
//
void ControlPanelWorkSpace::resize()
{
Boolean aa;
Dimension w,h; 
Widget ws, diag;

    ws = this->getRootWidget();
    XtVaGetValues (ws, XmNautoArrange, &aa, NULL);
    if (!aa) { this->WorkSpace::resize(); }
    else {
	// This action is required because WorkSpace::resize doesn't do nice
	// things for the contents of a scrolled window when the scroll bars
	// are unmanaged.  The size of the Workspace widget shrinks but only
	// by a few pixels at a time.
	diag = ws;
	while (!XtIsShell(diag)) diag = XtParent(diag);
	XtVaGetValues(diag, XmNwidth, &w, XmNheight, &h, NULL);
	this->setWidthHeight (w-this->widthDelta, h-this->heightDelta);
    }
}

// D R A G  - N -  D R O P   C A L L B A C K S
// D R A G  - N -  D R O P   C A L L B A C K S
// D R A G  - N -  D R O P   C A L L B A C K S
// D R A G  - N -  D R O P   C A L L B A C K S



boolean ControlPanelWorkSpace::decodeDropType (int tag,
	char *type, XtPointer value, unsigned long len, int x, int y)
{
boolean retVal;
char name[MAXHOSTNAMELEN + 10];

    if (!this->dropPermission) return FALSE;
    if (!theDXApplication->appAllowsEditorAccess()) return FALSE;

    switch (tag) {
	case ControlPanelWorkSpace::Interactors:
	    retVal = this->transfer (type, value, len, x,y);
	    break;


// executing ControlPanel::addSelectedInteractorsOption() would be nice
// but it is a special modal function and this op must proceed all at once
// with no intermediate steps.  There is no real data transfer here.
// There is just a header which contains hostname:pid because this is not
// a dnd operation which can exist between executables.  It only happens
// within 1 running copy of dx.
	case ControlPanelWorkSpace::Nodes:
	    int l;
	    char *cp;
	    // Check the header for machine type and pid against our own.
	    if (gethostname (name, MAXHOSTNAMELEN) == -1) {
		retVal = FALSE;
		break;
	    }

	    l = strlen(name);
	    sprintf (&name[l], ":%d", getpid());

	    cp = (char *)value;
	    if (strcmp(cp, name)) {
		retVal = FALSE;
		break;
	    }

	    retVal = this->controlPanel->dndPlacement (x,y);
	    break;


	case ControlPanelWorkSpace::Text:
	    retVal = this->compoundTextTransfer (type, value, len, x,y);
	    break;


	default:
	    retVal = FALSE;
	    break;
    }
    return retVal;
}


#if defined(WANT_DND_FOR_TEXT)
void ControlPanelWorkSpace::SetOwner(void *b)
{
ControlPanel *cp = (ControlPanel*)b;
Network *netw = cp->getNetwork();
    netw->setCPSelectionOwner(cp);
}
 
void ControlPanelWorkSpace::DeleteSelections(void *b)
{
ControlPanel *cp = (ControlPanel*)b;
 
    cp->deleteSelectedInteractors();
}

#endif

void ControlPanelWorkSpace::dropsPermitted(boolean onoff)
{ 
    this->dropPermission = onoff; 
    XtVaSetValues (this->getRootWidget(), 
	XmNdropSiteActivity, (onoff? XmDROP_SITE_ACTIVE: XmDROP_SITE_INACTIVE), 
    NULL);
}


void ControlPanelWorkSpace::getSelectedBoundingBox 
    (int *minx, int *miny, int *maxx, int *maxy)
{
    this->controlPanel->getMinSelectedXY(minx, miny);
}
