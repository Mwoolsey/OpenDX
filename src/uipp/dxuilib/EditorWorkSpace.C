/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/CutPaste.h>
#include <Xm/DragC.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <X11/cursorfont.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "EditorWorkSpace.h" 
#include "EditorWindow.h" 
#include "DXApplication.h" 
#include "Node.h" 
#include "StandIn.h"
#include "List.h"
#include "ListIterator.h"
#include "Network.h"
#include "WarningDialogManager.h"
#include "DXDropSite.h"
#include "ToolSelector.h"

#if  COMPOUND_TEXT_TRANSFER
#include "DecoratorStyle.h"
#include "DecoratorInfo.h"
#include "DictionaryIterator.h"
#endif

#include <../widgets/WorkspaceW.h>

Boolean EditorWorkSpace::EditorWorkSpaceClassInitialized = FALSE;

static XtTranslations pageTranslations = 0;
static String pageTable = "\
  <Btn1Down>:    select_w()\n\
  <Btn1Motion>:  move_w()\n\
  <Btn1Up>(2+):  select_w() release_w() select_w() release_w()\n\
  <Btn1Up>:      release_w()\n\
";

String EditorWorkSpace::DefaultResources[] =
{
    "*manhattanRoute:          True",
    "*lineThickness:       	1",
    "*accentColor:       	#dddddddddddd",
#if WORKSPACE_PAGES
    "*vpeCanvas.traversalOn:             True",
    "*vpeCanvas.marginWidth:             0",
    "*vpeCanvas.marginHeight:            0",
    "*vpeCanvas.allowResize:             True",
    "*vpeCanvas.sensitive:               True",
    "*vpeCanvas.allowMovement:           True",
    "*vpeCanvas.allowOverlap:  		 False",
    "*vpeCanvas.button1PressMode:        False",
    "*vpeCanvas.resizePolicy:            RESIZE_GROW",
    "*vpeCanvas.accentPolicy:            ACCENT_BACKGROUND",
    "*vpeCanvas.horizontalDrawGrid:      DRAW_HASH",
    "*vpeCanvas.verticalDrawGrid:        DRAW_HASH",
    "*vpeCanvas.horizontalAlignment:     ALIGNMENT_CENTER",
    "*vpeCanvas.verticalAlignment:       ALIGNMENT_CENTER",
    "*vpeCanvas.inclusionPolicy:         INCLUDE_ALL",
    "*vpeCanvas.outlineType:             OUTLINE_EACH",
    "*vpeCanvas.selectionPolicy:         EXTENDED_SELECT",
    "*vpeCanvas.sortPolicy:              ALIGNMENT_BEGINNING",
    "*vpeCanvas.allowVerticalResizing:   False",
    "*vpeCanvas.allowHorizontalResizing: False",
    "*vpeCanvas.lineInvisibility:	 True",
#endif
    NUL(char*)
};

#define DXMODULES "DXMODULES"
#define DXTOOLNAME "DXTOOLNAME"
#define FILE_NAME "FILE_NAME"
#define DXXOFFSET "DX_XOFFSET"
#define DXYOFFSET "DX_YOFFSET"

Dictionary* EditorWorkSpace::DropTypeDictionary = new Dictionary;

//
// Done for a double click in the work space
//
void EditorWorkSpace::doDefaultAction(Widget , XtPointer )
{
    //
    // First check for selected page icons.
    //
    this->editor->doSelectedNodesDefaultAction();
}

//
// Done for a single click in the work space
//
void EditorWorkSpace::doBackgroundAction(Widget, XtPointer callData)
{
    XmWorkspaceCallbackStruct *call = (XmWorkspaceCallbackStruct*) callData;

    this->editor->addCurrentNode(call->event->xbutton.x,call->event->xbutton.y, this);
}

EditorWorkSpace::EditorWorkSpace( const char* name, 
                     Widget parent, WorkSpaceInfo *info,
		     EditorWindow* editor) : 
		     WorkSpace(name, parent, info), 
		     DXDropSite()

{

    this->editor = editor;
    this->members_initialized = FALSE;
    this->included_in_ps = TRUE;
    this->recorded_positions = FALSE;
    this->record_positions = theDXApplication->getAutoScrollInitialValue();
}


EditorWorkSpace::~EditorWorkSpace()
{
}

void EditorWorkSpace::initializeRootWidget(
		boolean fromFirstIntanceOfADerivedClass)
{
    XGCValues        values;
    int              n;
    Arg              arg[50];
    Pixel            background;
    Pixel            white;

    if (NOT EditorWorkSpace::EditorWorkSpaceClassInitialized ||
	fromFirstIntanceOfADerivedClass) {

        ASSERT(theApplication);
	// Load our default resources first....
        this->setDefaultResources(theApplication->getRootWidget(),
                                  EditorWorkSpace::DefaultResources);

	// ...and force the parents default resources to be loaded.
	fromFirstIntanceOfADerivedClass = TRUE;

        EditorWorkSpace::EditorWorkSpaceClassInitialized = TRUE;

	if (!pageTranslations) {
	    pageTranslations = XtParseTranslationTable(pageTable);
	}

	this->addSupportedType (EditorWorkSpace::Modules, DXMODULES, TRUE);
	this->addSupportedType (EditorWorkSpace::ToolName, DXTOOLNAME, FALSE);
#if COSE_ACTUALLY_WORKS 
	// This should be the type used by the file manager in COSE.   The only
	// example of COSE I have is aix 4.1.  It understands this type, but the
	// drop happens the only data I get is the path name of my home directory.
	this->addSupportedType (EditorWorkSpace::File, "FILE", FALSE);
#endif
#if  COMPOUND_TEXT_TRANSFER
	this->addSupportedType (EditorWorkSpace::Text, "COMPOUND_TEXT", FALSE);
#endif
    }
    
    // Now initialize the parent, thereby building the widget tree.
    this->WorkSpace::initializeRootWidget(fromFirstIntanceOfADerivedClass);

    n = 0;
    XtSetArg(arg[n], XmNbackground,    &background);              n++;
    XtGetValues(this->getRootWidget(), arg, n);

    white = WhitePixelOfScreen(XtScreen(this->getRootWidget()));

    values.background     = background;
    values.function       = GXxor;
    values.subwindow_mode = IncludeInferiors;
    values.foreground     = white ^ values.background;

    this->gc_xor = XtGetGC(this->getRootWidget(),
                           GCForeground | 
                           GCBackground | 
                           GCFunction | 
                           GCSubwindowMode,
                           &values);

    values.background     = background;
    values.foreground     = white;
    this->gc= XtGetGC(this->getRootWidget(),
                      GCForeground | 
                      GCBackground,
                      &values);

    this->src.node  = NULL;
    this->src.param = 0;
    this->hot_spot = NULL;
    this->labeled_tab = NULL;
    this->font_list = NULL;
    this->tracker = NULL;
    this->dst.node  = 0;   // FIXME

    this->remove_arcs = False;


    this->cursor[UP_CURSOR] = XCreateFontCursor(this->display, 
						XC_sb_up_arrow);

    this->cursor[DOWN_CURSOR] = XCreateFontCursor(this->display, 
						XC_sb_down_arrow);

}
void EditorWorkSpace::setRootWidget(Widget w, boolean standardDestroy)
{
    //
    // Call the superclass method
    //
    this->WorkSpace::setRootWidget(w, standardDestroy);
}



boolean EditorWorkSpace::mergeNetElements (Network *tmpnet, List *tmppanels, int x, int y)
{
boolean retval;
int x2use = x;
int y2use = y;

    if (XmWorkspaceLocationEmpty(this->getRootWidget(), x,y)) {
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

	tmpnet->setTopLeftPos(x2use, y2use);
	retval = this->editor->getNetwork()->mergeNetworks(tmpnet, tmppanels, TRUE);
    } else
	retval = False;

    return retval;
}


boolean EditorWorkSpace::decodeDropType (int tag,
	char *type, XtPointer value, unsigned long len, int x, int y)
{
boolean retVal;
ToolSelector *ts;
char *category, *toolname;
NodeDefinition *nd;

    if (!theDXApplication->appAllowsEditorAccess())  return FALSE;

    switch (tag) {
	case EditorWorkSpace::Modules:
	    retVal = this->transfer (type, value, len, x, y);
	    break;

	// A name was supplied by a ToolSelector somewhere.  Might have been
	// this copy of dx, might not have been.  
	// For ::ToolName we receive something like this: Rendering::Image.  Separate
	// the two names, then do a lookup operation.  This dx must know about both the
	// category and the tool.  Use ToolSelector dictionary functions.
	case EditorWorkSpace::ToolName:
	    ts = this->editor->getToolSelector();
	    category = new char[len];
	    toolname = new char[len];
	    sscanf ((char *)value, "%[^:]:%s", category, toolname);
	    nd = ts->definitionOf (category, toolname);
	    if (nd) {
		this->editor->addNode(nd, x,y, this);
		retVal = TRUE;
	    } else {
		retVal = FALSE;
	    }

	    delete category;
	    delete toolname;
	   
	    break;

	// Dropping text onto the vpe
	case EditorWorkSpace::Text:
	    retVal = this->compoundTextTransfer (type, value, len, x, y);
	    break;


	//
	// Drop type supplied by COSE
	// If the network isn't dirty then read in new .net.
	//
	case EditorWorkSpace::File:
	    if (this->editor->getNetwork()->isFileDirty())
		retVal = FALSE;
	    else
		retVal = 
		    theDXApplication->openFile ((const char *)value, NUL(const char *));
	    break;


	default:
	    retVal = FALSE;
	    break;
    }
    return retVal;
}

boolean EditorWorkSpace::compoundTextTransfer
	(char *, XtPointer value, unsigned long len, int x, int y)
{
#if  COMPOUND_TEXT_TRANSFER
// The workspace is registered as a dropsite for COMPOUND_TEXT.  That happens
// when a DropSite object is constructed.  If this code is enabled, it will
// create LabelDecorator objects in the workspace with the text that is
// dropped onto the workspace.  See the calls to addSupportedType();
Decorator *d = 0;
char *cp = (char *)value;
int i, j;

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

    Dictionary *dict = DecoratorStyle::GetDecoratorStyleDictionary("Annotate");
    ASSERT(dict);

    DictionaryIterator di(*dict);
    DecoratorStyle *ds;
    while (ds = (DecoratorStyle*)di.getNextDefinition()) {
	if (EqualString(ds->getNameString(), "Marker")) break;
    }
    if (!ds) {
	di.setList(*dict);
	ds = (DecoratorStyle*)di.getNextDefinition();
    }
    ASSERT(ds);
    d = ds->createDecorator(TRUE);
    ASSERT(d);
    d->setStyle (ds);

    DecoratorInfo *dnd = new DecoratorInfo (this->editor->getNetwork(), 
	(void*)this->editor,
	(DragInfoFuncPtr)EditorWorkSpace::SetOwner,
	(DragInfoFuncPtr)EditorWorkSpace::DeleteSelections,
	(DragInfoFuncPtr)EditorWorkSpace::Select);
    d->setDecoratorInfo(dnd);
    this->editor->getNetwork()->setFileDirty();

    d->setXYPosition(x,y);
    d->setLabel(saveStr);

    d->manage(this);
    this->editor->getNetwork()->addDecoratorToList ((void*)d);
    delete saveStr;
#endif
    return True;
}


void EditorWorkSpace::SetOwner(void *b)
{
EditorWindow *ew = (EditorWindow*)b;
Network *netw = ew->getNetwork();
    netw->setCPSelectionOwner(NUL(ControlPanel*));
}
 
void EditorWorkSpace::DeleteSelections(void *b)
{
EditorWindow *ew = (EditorWindow*)b;
Command *cmd = ew->getDeleteNodeCmd();
 
    cmd->execute();
}

void EditorWorkSpace::Select(void *b)
{
EditorWindow *ew = (EditorWindow*)b;
    ew->handleDecoratorStatusChange();
}


void EditorWorkSpace::getSelectedBoundingBox (int *minx, int *miny, int *maxx, int *maxy)
{
List *nodes = this->editor->makeSelectedNodeList();
List *decors = this->editor->makeSelectedDecoratorList();

    this->editor->getNodesBBox (minx, miny, maxx, maxy, nodes, decors);
    if (nodes) delete nodes;
    if (decors) delete decors;
}


#define MOTIF_12_DRAG_N_DROP_HAS_BUGS 1
#if MOTIF_12_DRAG_N_DROP_HAS_BUGS
//
// virtual versions of {un}manage are supplied here because overlapping drop
// sites don't seem to work properly.  I have tried XmDropSiteConfigureStackingOrder,
// and changing XmNdropSiteActivity to XmDROP_SITE_{IN}ACTIVE, but so far the only
// thing I have found which kicks off the drop callbacks in the proper widget is
// unregistering pages which aren't in use.  When you unmanage a page, Motif's
// drop code still thinks there is a drop site there.
//
void EditorWorkSpace::manage()
{
    if (this->isManaged()) return ;
    this->setDropWidget(this->getRootWidget(), XmDROP_SITE_COMPOSITE);
    this->WorkSpace::manage();
}

void EditorWorkSpace::unmanage()
{
    this->WorkSpace::unmanage();
    XmDropSiteUnregister (this->getRootWidget());
}
#else
 must put the call the setDropWidget back in EditorWorkSpace::setRootWidget()
#endif

void EditorWorkSpace::restorePosition ()
{
    int x,y;
    this->getRecordedScrollPos (&x, &y);
    this->editor->moveWorkspaceWindow (x,y,False);
}

//
// ww is the workspace widget.  The widget that moved is stored
// in calldata.  That has to be translated into a EditorWorkSpaceComponent
// object. The x,y values in the callback struct haven't been applied yet.
//
// Ignore the callback if it originated from a KeyEvent because we don't
// need to undo the movement of canvas objects that started from arrow keys.
//
void EditorWorkSpace::doPosChangeAction (Widget ww, XtPointer callData)
{
    static Time previous_time_stamp = 0;
    this->WorkSpace::doPosChangeAction(ww,callData);
    XmWorkspacePositionChangeCallbackStruct* cbs = 
	(XmWorkspacePositionChangeCallbackStruct*)callData;
    Widget child = cbs->child;
    XtPointer udata;
    XtVaGetValues (child, XmNuserData, &udata, NULL);
    UIComponent* uic = (UIComponent*)udata;
    boolean mouse_type = FALSE;
    boolean same_event = FALSE;
    if (cbs->event) {
	switch (cbs->event->xany.type) {
	    case ButtonPress:
	    case ButtonRelease:
		same_event = (cbs->event->xbutton.time == previous_time_stamp);
		previous_time_stamp = cbs->event->xbutton.time;
		mouse_type = TRUE;
		break;
	    case MotionNotify:
		same_event = (cbs->event->xmotion.time == previous_time_stamp);
		previous_time_stamp = cbs->event->xmotion.time;
		mouse_type = TRUE;
		break;
	    case KeyPress:
	    case KeyRelease:
		same_event = (cbs->event->xkey.time == previous_time_stamp);
		previous_time_stamp = cbs->event->xkey.time;
		break;
	    default:
		break;
	}
    }
    this->editor->saveLocationForUndo(uic, mouse_type, same_event);
}

