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
#include <X11/Xutil.h>

#include "Application.h" 
#include "DropSite.h" 
#include "TransferStyle.h"
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

boolean DropSite::DropSiteClassInitialized = FALSE;

static XContext dropcontext = 0;
int DropSite::drop_x = 0;
int DropSite::drop_y = 0;

//
// This is the new+improved constructor.  Following your birth as a subclass
// of DropSite, you may call this->addSupportedType in order to let the world
// know that you can accept data of another type. Note that at the present time,
// you must invoke addSupportedType for all your types, before invoking setDropWidget
// because setDropWidget takes all your types and does the broadcast.
//
DropSite::DropSite ()
{
}

//
// Add a "style" to the dictionary.  The dictionary is a set of Atom,
// conversion proc pairs.  Every drop site supplies a list of Atoms he
// understands and a pointer to the proc which can convert data of the
// type described by that Atom.
//
// I need the dictionary to use Atom as a key rather than the name of the
// Atom as the key.  The Atom is an int and makes searching and inserting
// fast and easy, but I don't know if there is support in Dictionary for
// any key value type other than String.
//
// the "preferred" argument is True if typeName is the preferred type for this
// object.  The preferred type will be used in the event that there are several
// possible matching transfer types between sender and receiver.
void DropSite::addSupportedType 
	(int tag, const char *typeName, boolean preferred)
{
    ASSERT (typeName);
    ASSERT (typeName[0]);

    ASSERT (this->getDropDictionary());

    TransferStyle *ts = new TransferStyle(tag, typeName, preferred);
    ASSERT(ts);
    this->getDropDictionary()->addDefinition (typeName, (const void *)ts);
}

DropSite::~DropSite()
{
}

void DropSite::setDropWidget(Widget drop_w, unsigned char type, 
    Pixmap animation, Pixmap animation_mask)
{
int	n;
Arg	wargs[20];
#define MAX_IMPORTS 20
Atom    imports[MAX_IMPORTS];
int	num_imports;

    if(!DropSite::DropSiteClassInitialized)
    {
	dropcontext = XUniqueContext();
	DropSite::DropSiteClassInitialized = True;
    }

    // What's this about?  We're using X{Save,Find}Context to get the "this"
    // pointer inside an XtCallbackProc.  The key value for the lookup is drop_w.
    XSaveContext(theApplication->getDisplay(), (Window)drop_w, 
				dropcontext, (const char *)this);

    // Create an array of Atoms.  Each entry identifies a data type we understand.
    // The array is passed to Motif - lets the world know who we are.  Loop over
    // the dictionary entries to create this array.
    DictionaryIterator di(*this->getDropDictionary());
    TransferStyle *ts;
    num_imports = 0;
    while ((ts=(TransferStyle*)di.getNextDefinition())) {
	imports[num_imports] = ts->getAtom();
	if ((++num_imports) == MAX_IMPORTS) break;
    }

    n = 0;
    XtSetArg(wargs[n], XmNimportTargets, imports); n++;
    XtSetArg(wargs[n], XmNnumImportTargets, num_imports); n++;
    XtSetArg(wargs[n], XmNdropSiteOperations, XmDROP_MOVE|XmDROP_COPY); n++;
    XtSetArg(wargs[n], XmNdropProc, DropSite_HandleDrop); n++;
    XtSetArg(wargs[n], XmNdropSiteType, type); n++;

    if (animation != XmUNSPECIFIED_PIXMAP) {
	XtSetArg (wargs[n], XmNanimationPixmap, animation); n++;
	XtSetArg (wargs[n], XmNanimationMask, animation_mask); n++;
	XtSetArg (wargs[n], XmNanimationStyle, XmDRAG_UNDER_PIXMAP); n++;

	//
	// Bug: I was unable to use real pixmaps (with depth == 8 and with colors).
	// When I attempted I got Xlib warnings on stdout when motif tried to use
	// the pixmap.  I was only able to make this work when the pixmap was
	// made with XCreateBitmapFromData() (a 1 plane pixmap).  I tried
	// XCreatePixmapFromBitmapData() (as is normally the case) with and without
	// setting XmNanimationPixmapDepth.
	//
	//int depth;
	//XtVaGetValues (drop_w, XmNdepth, &depth, NULL);
	//XtSetArg (wargs[n], XmNanimationPixmapDepth, depth); n++;
    }

    XmDropSiteRegister((Widget)drop_w, wargs, n);
    this->drop_w = drop_w;
}

typedef struct {
   void *style;
   void *obj;
} StyleObjPair;

extern "C" void DropSite_HandleDrop(Widget w, XtPointer, XtPointer call_data)
{
XmDropProcCallback	DropData;
int			n;
Cardinal		i, num_exports;
Atom			*exportTargets;
XmDropTransferEntryRec	transferEntries[1];
XPointer		ptr;
Arg			wargs[20];
Boolean			favoriteIsFound;

    //
    // Retreive the "this" pointer
    //
    XFindContext(theApplication->getDisplay(), (Window)w, dropcontext, &ptr);
    DropSite *ds = (DropSite *)ptr;

    DropData = (XmDropProcCallback)call_data;

    DropSite::drop_x = DropData->x;
    DropSite::drop_y = DropData->y;

    XtVaGetValues(DropData->dragContext, 
	XmNnumExportTargets, &num_exports, 
	XmNexportTargets, &exportTargets, 
	NULL);

#if DIAGNOSING_PROBLEMS_IN_DND
    for(i = 0; i < num_exports; i++)
    {
	char *cp = XGetAtomName (XtDisplay(w), exportTargets[i]);
	printf ("name of export atom %#x -- %s\n", exportTargets[i], (cp?cp:"<NULL>"));
	if (cp) free(cp);
    }
#endif

    // n-squared Search:
    // Iterate over the dictionary entries to find a type which matches something
    // in the incoming list.  The incoming list describes the types the supplier
    // is capable of.  Use either our favorite type, or - in the event the set of
    // matches excludes our favorite type - any match.
    DictionaryIterator di(*ds->getDropDictionary());
    TransferStyle *ts;
    favoriteIsFound = FALSE;
    TransferStyle *matchFound = NUL(TransferStyle*);
    while ((ts=(TransferStyle*)di.getNextDefinition())) {
	for (i=0; i<num_exports; i++) {
	    if (exportTargets[i] == ts->getAtom()) {
		if (ts->isPreferred()) {
		    favoriteIsFound = TRUE;
		} 
		matchFound = ts;
	    }
	    if (favoriteIsFound) break;
	}
	if (favoriteIsFound) break;
    }

    
    n = 0;
    if ((DropData->dropAction != XmDROP) || 
	((DropData->operation != XmDROP_MOVE) && (DropData->operation != XmDROP_COPY)) ||
	(!matchFound))
    {
	XtSetArg(wargs[n], XmNtransferStatus, XmTRANSFER_FAILURE); n++;
	XtSetArg(wargs[n], XmNnumDropTransfers, 0); n++;
    }
    else
    {
	static StyleObjPair sop;
	sop.style = matchFound;
	sop.obj = ds;
	transferEntries[0].target = matchFound->getAtom();
	transferEntries[0].client_data = (XtPointer)&sop;

	XtSetArg(wargs[n], XmNdropTransfers, transferEntries); n++;
	XtSetArg(wargs[n], XmNnumDropTransfers, XtNumber(transferEntries)); n++;
	XtSetArg(wargs[n], XmNtransferProc, DropSite_TransferProc); n++;
    }
    XmDropTransferStart(DropData->dragContext, wargs, n);
}
extern "C" void 
DropSite_TransferProc(Widget w, XtPointer client_data, Atom *, Atom *type,
    XtPointer value, unsigned long *length, int)
{

    // I thought it was not possible to be called with value==nil
    // but it happens if the drag source set it to nil even though
    // it returned False.  ???
    if (!value) {
	XtVaSetValues(w, XmNtransferStatus, XmTRANSFER_FAILURE, NULL);
	return ;
    }


    StyleObjPair* sop = (StyleObjPair*)client_data;
    ASSERT(sop);
    TransferStyle *ts = (TransferStyle*)sop->style;
    DropSite *ds = (DropSite*)sop->obj;
    ASSERT(ts);
    ASSERT(ds);

    char *cp = XGetAtomName(theApplication->getDisplay(), *type);
    if (!ds->decodeDropType (ts->getTag(), cp, value, *length, DropSite::drop_x, DropSite::drop_y))
	XtVaSetValues(w, XmNtransferStatus, XmTRANSFER_FAILURE, NULL);
    if (cp) XFree(cp);

    //
    // Special ICCCM Atoms.  Share these values with the drag source.  
    // DELETE: if intra-executable then set to true so he knows to delete his memory
    // HOST_NAME: hostname of machine of drag source
    // PROCESS: pid of drag source
    //
    Atom DELETE_ATOM, PROCESS_ATOM, HOST_NAME_ATOM;
    Display *d = theApplication->getDisplay();
    Screen *screen = XtScreen(theApplication->getRootWidget());
    Window root = RootWindowOfScreen(screen);
    char hostname[MAXHOSTNAMELEN], *src_host;
    char tbuf[8];

    DELETE_ATOM = XmInternAtom(d, "DELETE", False);
    PROCESS_ATOM = XmInternAtom(d, "PROCESS", False);
    HOST_NAME_ATOM = XmInternAtom(d, "HOST_NAME", False);

    int *process_id;
    unsigned long remain, nitems;
    Atom actual_type;
    int actual_format;
    XGetWindowProperty (d, root, PROCESS_ATOM, 0, 1, False, XA_INTEGER,
	&actual_type, &actual_format, &nitems, &remain, (unsigned char **)&process_id);

    gethostname(hostname, sizeof(hostname));
    XGetWindowProperty (d, root, HOST_NAME_ATOM, 0, sizeof(src_host)>>2, False, XA_STRING,
	&actual_type, &actual_format, &nitems, &remain, (unsigned char **)&src_host);
	
    if ((src_host) && (!strcmp(src_host, hostname)) && (*process_id == getpid())) {
	strcpy (tbuf, "FALSE");
    } else {
	strcpy (tbuf, "TRUE");
    }

    if (src_host) XFree(src_host);
    if (process_id) XFree(process_id);
    XChangeProperty (d, root, DELETE_ATOM, XA_STRING, 8,
	PropModeReplace, (const unsigned char *)tbuf, strlen(tbuf));
}

//
// DropSite::transfer used to be pure virtual, but now we create drop sites
// that do nothing but reject drops (StandIns).  They don't need to create
// a tranfer func.
//
boolean DropSite::transfer(char *, XtPointer, unsigned long, int, int)
{
    return FALSE;
}

#ifdef Comment
void IncTransferProc(Widget w, XtPointer client_data, Atom *selection, 
		    Atom *type, XtPointer value, unsigned long length, int *f)
{
Atom DXMODULE;
XmWorkspaceDragDropCallbackStruct call_data;
Widget ww;

    DXMODULE = XmInternAtom(XtDisplay(w), "DXMODULE", False);

    ww = client_data;

    call_data.data = value;
    call_data.amount = length;
    if(*type == DXMODULE)
	XtCallCallbacks((Widget) ww, XmNdropCallback, &call_data);
}
#endif

