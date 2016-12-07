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

#include <Xm/Xm.h>
#include <Xm/CutPaste.h>
#include <Xm/AtomMgr.h>
#include <Xm/DragC.h>
#include <Xm/DragDrop.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "DragSource.h" 
#include "Application.h" 
#include "WarningDialogManager.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#if defined(NEEDS_GETHOSTNAME_DECL)
extern "C" int gethostname(char *address, int address_len);
#endif

#if defined(DXD_WIN)
#include <winsock.h>
#endif


//
// DragSource handles (or cooperates with DropSite on) the following
//	1) the Motif end the dnd protocol
//	2) free-ing (or not) the memory used for the transfer
//	3) setting ICCCM atoms commonly used by dnd participants
//	4) the drag cursor
//
// 2) Deleting data depends on whether the transfer is within an executable
// or between executables.  We check on that here.
// 3) We're setting USER, HOST_NAME, PROCESS.  The book lists many other
// things we might set.  These atoms are used on the drop end.  Certain
// drops happen only within an executable.
// 4) The drag cursor is currently a monochrome bitmap.  It could be anything
// drawable in X.  Using Xpm would be a great way to make a prettier cursor.
//

boolean DragSource::DragSourceClassInitialized = FALSE;
XtTranslations DragSource::drag_tranlations = NULL;

void* DragSource::TransferData = 0;

static char dragTranslations[] = 
    "#override <Btn2Down>:	DragSource_StartDrag()\n\
	a<Btn1Down>:		DragSource_StartDrag()";

static XtActionsRec dragActions[] = 
    {
	{"DragSource_StartDrag", (XtActionProc)DragSource_StartDrag},
    };

static XContext dragcontext = 0;

DragSource::DragSource()
{
    this->drag_pixmap = 0;
    this->drag_context = NUL(Widget);
}

DragSource::~DragSource()
{
}

void DragSource::addSupportedType (int tag, const char *typeName, boolean preferred)
{
    ASSERT(typeName);
    ASSERT(typeName[0]);

    ASSERT(this->getDragDictionary());

    TransferStyle *ts = new TransferStyle(tag, typeName, preferred);
    ASSERT (ts);
    this->getDragDictionary()->addDefinition (typeName, (const void *)ts);
}


void DragSource::setDragWidget(Widget source_w)
{

    //
    // One time class initialization stuff
    //
    if(!DragSource::DragSourceClassInitialized)
    {

	//
	// So we can get "this" back from the widget
	//
	dragcontext = XUniqueContext();

	//
	// Parse the Button 2 translations
	//
	DragSource::drag_tranlations = 
	    XtParseTranslationTable(dragTranslations);

	//
	// Add the action routine
	//
	XtAppAddActions(XtWidgetToApplicationContext(source_w),
			dragActions,
			XtNumber(dragActions));

	DragSource::DragSourceClassInitialized = True;
    }

    //
    // So we can get "this" back from the widget
    //
    XSaveContext(XtDisplay(source_w), Window(source_w), 
	dragcontext, (const char *)this);

    XtOverrideTranslations(source_w, DragSource::drag_tranlations);
}
void DragSource::setDragIcon(Widget drag_icon)
{
    this->drag_pixmap = 1;
    this->drag_icon = drag_icon;
}
Widget DragSource::createDragIcon(int width, int height, char *bits, char *maskbits)
{
    Arg wargs[10];
    int n = 0;

    this->drag_pixmap = XCreateBitmapFromData(
		    theApplication->getDisplay(),
		    XtWindow(theApplication->getRootWidget()),
		    bits, width, height);

    this->drag_mask = XCreateBitmapFromData(
		    theApplication->getDisplay(),
		    XtWindow(theApplication->getRootWidget()),
		    maskbits, width, height);

    XtSetArg(wargs[n], XmNpixmap, this->drag_pixmap); n++;
    XtSetArg(wargs[n], XmNmask, this->drag_mask); n++;
    XtSetArg(wargs[n], XmNwidth, width); n++;
    XtSetArg(wargs[n], XmNheight, height); n++;
#ifndef DXD_WIN
    this->drag_icon = XmCreateDragIcon(theApplication->getRootWidget(), 
				"drag_icon", wargs, n);
#else
    /*  
	AJ  This is a bug in XmStatic.lib. 
	Exceed  is still working on it.
    */
    this->drag_icon = NULL; 
#endif
    return this->drag_icon;
}
extern "C" void DragSource_StartDrag(Widget w, XEvent* event,
	                  String* , Cardinal* )
{
XPointer	ptr;
DragSource	*ds;

    ASSERT(dragcontext);

    if (XFindContext(XtDisplay(w), Window(w), dragcontext, &ptr))  return ;
    ds = (DragSource *) ptr;
    
    ds->startDrag(w, event);
}

void DragSource::startDrag(Widget w, XEvent* event)
{
Arg	wargs[20];
int	n;
int     num_exports;
#define MAX_EXPORTS 20
Atom    exports[MAX_EXPORTS];
int     response = this->decideToDrag(event);

    if(response != DragSource::Proceed)
    {
	if (response == DragSource::Abort) 
	    XBell(XtDisplay(w), 100);
	return;
    }


    // Create an array of Atoms.  Each entry identifies a data type we understand.
    // The array is passed to Motif - lets the world know who we are.  Loop over
    // the dictionary entries to create this array.
    DictionaryIterator di(*this->getDragDictionary());
    num_exports = 0;
    TransferStyle *ts;
    while ( (ts = (TransferStyle*)di.getNextDefinition()) ) {
        exports[num_exports] = ts->getAtom();
        if ((++num_exports) == MAX_EXPORTS) break;
    }

    n = 0;
    XtSetArg(wargs[n], XmNexportTargets, exports); n++;
    XtSetArg(wargs[n], XmNnumExportTargets, num_exports); n++;
    if(event->xkey.state & ShiftMask)
    {
	XtSetArg(wargs[n], XmNdragOperations, XmDROP_MOVE); n++;
	this->operation = XmDROP_MOVE;
    }
    else
    {
	XtSetArg(wargs[n], XmNdragOperations, XmDROP_COPY); n++;
	this->operation = XmDROP_COPY;
    }
    XtSetArg(wargs[n], XmNconvertProc, DragSource_ConvertProc); n++;
    XtSetArg(wargs[n], XmNincremental, False); n++;
    XtSetArg(wargs[n], XmNblendModel, XmBLEND_JUST_SOURCE); n++;

    // Store "this" in clientData now.  It also goes into callData in the
    // the callback to DragSource_DropFinishCB.  It's replaced in clientData
    // before DragSource_DropFinishCB, inside DragSource_ConvertProc to hold
    // a TransferStyle*.
    XtSetArg(wargs[n], XmNclientData, this); n++;

    if(this->drag_pixmap != 0)
    {
	Pixel bg = WhitePixelOfScreen(XtScreen(w));
	Pixel fg = BlackPixelOfScreen(XtScreen(w));
	XtSetArg(wargs[n], XmNcursorBackground, bg); n++;
	XtSetArg(wargs[n], XmNcursorForeground, fg); n++;
	XtSetArg(wargs[n], XmNsourceCursorIcon, this->drag_icon); n++;
    }

    this->drag_context = XmDragStart(w, event, wargs, n);
    if (!this->drag_context) return ;
    XtAddCallback(this->drag_context, XmNdropFinishCallback, 
		    DragSource_DropFinishCB, this);
    if (this->intra_toplevel) {
	this->inside_own_shell = TRUE;
	this->top_level_window = 0;
	XtAddCallback(this->drag_context, XmNtopLevelLeaveCallback, 
			DragSource_TopLevelLeaveCB, this);
	XtAddCallback(this->drag_context, XmNtopLevelEnterCallback, 
			DragSource_TopLevelEnterCB, this);
    }

    //
    // Set special ICCCM Atom values so that reciever knows who
    // we are and can help us to know what to do with the data
    // DELETE == do we delete this the memory?
    // PROCESS == our pid
    // HOST_NAME == hostname of machine we're running on
    // USER == current user name
    //
    Atom PROCESS_ATOM, HOST_NAME_ATOM, USER_ATOM;
    Display *d = XtDisplay(this->drag_context);
    char hostname[MAXHOSTNAMELEN];

    // This is a strang way to get memory for an int, but if I do just declare
    // pid as an int, then texture complains about an unaligned access. I guess
    // something inside XChangeProperty expects more alignment than an int provides.
    double pid_space;
    int *pid = (int*)&pid_space;
    *pid = (int)getpid();

    /*DELETE_ATOM = XmInternAtom(d, "DELETE", False);*/
    PROCESS_ATOM = XmInternAtom(d, "PROCESS", False);
    HOST_NAME_ATOM = XmInternAtom(d, "HOST_NAME", False);
    USER_ATOM = XmInternAtom(d, "USER", False);

    Screen *screen = XtScreen(w);
    Window root = RootWindowOfScreen(screen);
    XChangeProperty (d, root, PROCESS_ATOM, XA_INTEGER, 32,
	PropModeReplace, (unsigned char *)pid, 1);
    gethostname(hostname, sizeof(hostname));
    XChangeProperty (d, root, HOST_NAME_ATOM, XA_STRING, 8, 
	PropModeReplace, (unsigned char *)hostname, strlen(hostname));

    char *login = GETLOGIN;

    if ((login)&&(login[0]))
	XChangeProperty (d, root, USER_ATOM, XA_STRING, 8, 
	    PropModeReplace, (unsigned char*)login, strlen(login));
}

extern "C" Boolean DragSource_ConvertProc(Widget w, Atom *, 
					  Atom *target, Atom *type_return, 
					  XtPointer *value_return, 
					  unsigned long *length_return, 
					  int *format_return)
{
DragSource *ds;
Boolean ret;


    ret = True;
    XtVaGetValues(w, XmNclientData, &ds, NULL);
    ASSERT(ds);

    //
    // Call the convert routine, passing in the target, and getting back
    // the value and length
    //
    char *cp = XGetAtomName(theApplication->getDisplay(), *target);

    // Look up the data type for the transfer in the dictionary based on
    // the transfer atom.
    DictionaryIterator di(*ds->getDragDictionary());
    TransferStyle *ts;
    while ( (ts = (TransferStyle*)di.getNextDefinition()) ) {
	if (ts->getAtom() == *target) break;
    }
    //
    // It's not possible for ts==NULL at this point because this callback can
    // be invoked only if there is some atom which DragSource and DropSite have
    // in common.  However it did occur on ship day, that the irix desktop will
    // call this callback when there is no shared atom. so...
    //
    if (!ts) {
	if (cp) XFree(cp);
	return False;
    }
    XtVaSetValues (w, XmNclientData, ts, NULL);


    if (!ds->decodeDragType(ts->getTag(),cp,value_return,length_return,ds->operation))
	{
	    *value_return = NULL;
	    DragSource::TransferData = 0;
	    *length_return = 0;
	    ret = False;
	} else {
	    *type_return = *target;
	    DragSource::TransferData = (void *)*value_return;
	    *format_return = 8;
	}
    if (cp) XFree(cp);

    //
    // FIXME!!!
    // 
    if (*length_return > 63000)
    {
	*value_return = NULL;
	*length_return = 0;
	WarningMessage("Too much info to drag and drop.  Try transferring in pieces");
	ret = False;
    }

    return ret;
}


extern "C" void DragSource_DropFinishCB(Widget w, XtPointer cdata, XtPointer call_data)
{
DragSource *ds = (DragSource *)cdata;
XmDropFinishCallbackStruct *dfcbs = (XmDropFinishCallbackStruct *)call_data;
TransferStyle *ts;

    XtVaGetValues(w, XmNclientData, &ts, NULL);
    ASSERT(ts);
    ds->drag_context = NUL(Widget);

    ds->dropFinish(ds->operation, ts->getTag(), dfcbs->completionStatus);

    //
    // The ds object might not exist at this point so don't reference it anymore.
    //
    Atom DELETE_ATOM; 
    Display *d = XtDisplay(w);
    Window root = DefaultRootWindow(d);
    Atom actual_type;
    int actual_format;
    unsigned char *data;
    unsigned long remain, nitems;
    DELETE_ATOM = XmInternAtom(d, "DELETE", False);
    XGetWindowProperty (d, root, DELETE_ATOM, 0, 4, False, XA_STRING,
	&actual_type, &actual_format, &nitems, &remain, &data);

    if ((data) && (!strcmp((const char *)data, "TRUE"))) {
	if (DragSource::TransferData) 
#if !defined(sun4)
	    free (DragSource::TransferData);
#else
	    free ((char *)DragSource::TransferData);
#endif
	DragSource::TransferData = 0;
	XFree(data);
    }
}

void DragSource::dropFinish(long, int, unsigned char)
{
    // Do nothing, by default
}

extern "C" void DragSource_TopLevelLeaveCB(Widget w, XtPointer cdata, XtPointer call)
{
    DragSource* ds = (DragSource*)cdata;
    XmTopLevelLeaveCallbackStruct* tllcbs = (XmTopLevelLeaveCallbackStruct*)call;
    ASSERT(ds);
    if (ds->top_level_window == 0) 
	ds->top_level_window = tllcbs->window;
}

extern "C" void DragSource_TopLevelEnterCB(Widget w, XtPointer cdata, XtPointer call)
{
    DragSource* ds = (DragSource*)cdata;
    XmTopLevelEnterCallbackStruct* tlecbs = (XmTopLevelEnterCallbackStruct*)call;
    ASSERT(ds);
    if (ds->top_level_window)
	ds->inside_own_shell = (ds->top_level_window == tlecbs->window);
}

boolean DragSource::isIntraShell()
{
    ASSERT(this->intra_toplevel);
    ASSERT(this->drag_context);
    return this->inside_own_shell;
}

#ifdef Comment

extern "C" Boolean IncConvertProc(Widget	w,
				  Atom		*selection, 
				  Atom		*target, 
				  Atom		*type_return, 
				  XtPointer	*value_return, 
				  unsigned long *length_return, 
				  int		*format_return,
				  unsigned long *max_len, 
				  XtPointer	client_data, 
				  XtRequestId	*request_id)
{

    Widget ww;
    Atom DXMODULE;

    DXMODULE = XmInternAtom(XtDisplay(w), "DXMODULE", False);

    XtVaGetValues(w, XmNclientData, &ww, NULL);

    call_data.amount = *max_len;
    XtCallCallbacks((Widget) ww, XmNdragConvertCallback, &call_data);

    *target = DXMODULE;
    *type_return = XmInternAtom(XtDisplay(w), "XA_STRING", False);
    *value_return = call_data.data;
    *length_return = call_data.amount;
    *format_return = 8;
    return call_data.done;
}
#endif
