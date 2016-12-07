/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#if defined(HAVE_IOSTREAM)
#include <iostream>
#elif defined(HAVE_IOSTREAM_H)
#include <iostream.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#include "DXApplication.h"

#include <Xm/BulletinB.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/DragDrop.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#ifndef XK_MISCELLANY
#define XK_MISCELLANY
#endif
#include <X11/keysym.h>

#include "../widgets/WorkspaceW.h"

#include "StandIn.h"
#include "Tab.h"
#include "Parameter.h"
#include "List.h"
#include "ListIterator.h"
#include "DXStrings.h"
#include "Node.h"
#include "Ark.h"
#include "ArkStandIn.h"
#include "Command.h"
#include "EditorWindow.h"
#include "Network.h"
#include "EditorWorkSpace.h"
#include "UIComponent.h" 
#include "ErrorDialogManager.h"
#include "TransmitterNode.h"
#include "ReceiverNode.h"
#include "DXDragSource.h"

#include "moduledrag.bm"
#include "moduledragmask.bm"

// gethostname is needed by selectedNodesCB which is part of drag-n-drop
#if defined(NEEDS_GETHOSTNAME_DECL) 
extern "C" int gethostname(char *address, int address_len);
#endif

#if 0
#if defined(DXD_WIN)
#include <winsock.h>
#endif
#endif

#ifdef DXD_XTOFFSET_HOSED
#undef XtOffset
#define XtOffset( p_type, field ) ((size_t)&((( p_type )0)->field))
#endif


#define DXXOFFSET "DX_XOFFSET"
#define DXYOFFSET "DX_YOFFSET"


String StandIn::DefaultResources[] =
{
        ".marginWidth:     1",
        NULL
};


typedef struct _NodeAttrs
{
    struct
    {
// This Pixel value now comes from DXApplication::getStandInBackground().
//        Pixel           background;     /* bboard background color      */
        Pixel           foreground;     /* bboard normal foreground     */
        Pixel           highlight_foreground;
    }                   bboard;

    struct
    {
        Pixel           background;
                                        /* i/o tag BG color             */
        Pixel           highlight_background;
                                        /* i/o highlighted tab BG color */
        Pixel           required_background;
                                        /* i/o required tag BG color    */
        Dimension       width;          /* i/o tag height               */
        Dimension       height;         /* i/o tag width                */

    }                   io;

    struct
    {
        Pixel           color;          /* line color                   */
        short           thickness;      /* line thickness               */

    }                   line;

} standInDefaultsStruct;

static standInDefaultsStruct standInDefaults;

static 
XtResource _SIColorResources[] = {
// This guy now lives in DXApplication. His value is shared with c.p.
//    {
//	"standInBackground",
//	"StandInBackground",
//	XmRPixel,
//	sizeof(Pixel),
//	XtOffset(standInDefaultsStruct*, bboard.background),
//	XmRString,
//	(XtPointer)"CadetBlue"
//    },
    {
	"standInForeground",
	"StandInForeground",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(standInDefaultsStruct*, bboard.foreground),
	XmRString,
	(XtPointer)"White"
    },
    {
	"standInHighlightForeground",
	"StandInHighlightForeground",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(standInDefaultsStruct*, bboard.highlight_foreground),
	XmRString,
	(XtPointer)"SpringGreen"
    },
    {
	"standInLineColor",
	"StandInLineColor",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(standInDefaultsStruct*, line.color),
	XmRString,
	(XtPointer)"Black"
    },
    {
	"standInIORequiredBackground",
	"StandInIORequiredBackground",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(standInDefaultsStruct*, io.required_background),
	XmRString,
	(XtPointer)"MediumTurquoise"
    },
    {
	"standInIOHighlightBackground",
	"StandInIOHighlightBackground",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(standInDefaultsStruct*, io.highlight_background),
	XmRString,
	(XtPointer)"SpringGreen"
    },
};

static
XtResource _SIResourceList[] =
{
    {
	"standInIOWidth",
	"StandInIOWidth",
	XmRDimension,
	sizeof(Dimension),
	XtOffset(standInDefaultsStruct*, io.width),
	XmRString,
	(XtPointer)"16"
    },
    {
	"standInIOHeight",
	"StandInIOHeight",
	XmRDimension,
	sizeof(Dimension),
	XtOffset(standInDefaultsStruct*, io.height),
	XmRString,
	(XtPointer)"10"
    },
};

Pixel standInGetDefaultForeground()
{
        return standInDefaults.bboard.foreground;
}

Boolean StandIn::ClassInitialized = FALSE;
Widget  StandIn::DragIcon = NULL;
Dictionary* StandIn::DragTypeDictionary = new Dictionary;

StandIn::StandIn(WorkSpace *w, Node *node):
                 UIComponent(node->getNameString()),
		 DXDragSource(PrintCut)
{
    this->workSpace = w;
    this->node = node;
    this->buttonWidget = NULL;
    this->requiredButtonWidth = 0;
    this->drag_drop_wpid = 0;
}



//
// Destructor
//
StandIn::~StandIn()
{
    Tab *tab;
    ListIterator iterator;

    iterator.setList(this->inputTabList);
    while ( (tab = (Tab*)iterator.getNext()) ) 
        delete tab;

    iterator.setList(this->outputTabList);
    while ( (tab = (Tab*)iterator.getNext()) ) 
        delete tab;

    this->node->standin = NULL;

    if (this->drag_drop_wpid)
	XtRemoveWorkProc (this->drag_drop_wpid);

    //
    // Remove it (visually) from the work space, 
    // because when opening a new network, we don't make it back to
    // the X main loop before we start reading in new stand-ins over
    // the old ones.
    //
    XtUnmapWidget(this->getRootWidget());
}

void StandIn::drawTab(int         paramIndex,
                      boolean     outputTab)
{
    XtWidgetGeometry geometry;
    int              x,x2;
    int              y;
    int              n;
    Arg              arg[10];
    Tab              *tab;
    Widget           line;
    Node*            node;
    int              inputTab;
    int              in = FALSE;


    inputTab = !outputTab;

    node = this->node;


    if ((node->isParameterVisible(paramIndex,inputTab) == FALSE) ||
        (inputTab && node->getInputCount() == 0))
	return;

    if (!node->isParameterViewable(paramIndex,inputTab))
	return;

    tab = this->getParameterTab(paramIndex,inputTab);
    y = tab->getTabY();
    boolean is_connected = node->isParameterConnected(paramIndex,inputTab);

    if  (is_connected OR (inputTab && !node->isInputDefaulting(paramIndex)))
        {
            /*
             * Move the tab in.
             */
            in = TRUE;
            int new_y = y + (outputTab ? - standInDefaults.io.height
                             : standInDefaults.io.height);
            tab->setBackground(standInDefaults.io.background);
	    tab->moveTabY(new_y, FALSE);
        }
        else
        {
            /*
             * Move the tab out.
             */
            in = FALSE;
            if (inputTab && node->isInputRequired(paramIndex))
		tab->setBackground(standInDefaults.io.required_background);
	    tab->moveTabY(y, FALSE);
        }

    /*
     * Now, make sure that the tab widget is on top of the name widget.
     */
    geometry.request_mode = CWStackMode;
    geometry.stack_mode   = Above;
    XtMakeGeometryRequest(tab->getRootWidget(), &geometry, NULL);

// FIXME
//    if (inputTab && !node->isInputDefaulting(paramIndex)) {
//       return;
//    }
//
    /*
     * Create/delete line stub as circumstance dictates.
     */

    line = tab->getLine();

    if (is_connected)
    {
        /*
         * If the line already exists, adjust it (in case this is a
         * tab/line adjustment).  Otherwise, create a new line.
         */
        /*
         * Calculate the (x, y) coordinates.
         */
        x2 = tab->getTabX(); 
        x = (x2 + standInDefaults.io.width / 2) -
            standInDefaults.line.thickness / 2;

        n = 0;
        XtSetArg(arg[n], XmNx, x); n++;
        XtSetArg(arg[n], XmNy, y); n++;
        if (line != NULL) {
            XtSetValues(line, arg, n);
        } else if (in) { 
             /*
             * Create line stub.
             */
             XtSetArg(arg[n], XmNheight,standInDefaults.io.height);n++;
             XtSetArg(arg[n], XmNwidth, standInDefaults.line.thickness); n++;
             XtSetArg(arg[n], XmNbackground,standInDefaults.line.color); n++;
             XtSetArg(arg[n], XmNforeground,standInDefaults.line.color); n++;
             line = XmCreateLabel(this->getRootWidget(), "line", arg, n);
	     tab->setLine(line);
             XtManageChild(line);
         }
    } else {
             /*
              * Delete the line widget, if it exists.
              */
            if (line != NULL) {
                XtDestroyWidget(line);
	        tab->setLine(NULL);
            }
    }
}

#if 0
static
Pixel getColor(Widget widget,
                char*  color)
{
    XrmValue from;
    XrmValue to;
    Pixel *pixel;


    from.size = sizeof(char*);
    from.addr = color;

    XtConvert(widget, XmRString, &from, XmRPixel, &to);

    if (to.addr) {
	pixel = (Pixel*)to.addr;
	return *pixel;
    } else
	return 0;
}
#endif

//
// Provide tabs with their width and height
//
int StandIn::getIOWidth()
{ 
    return standInDefaults.io.width;
}
int StandIn::getIOHeight()
{ 
    return standInDefaults.io.height;
}

//
// Static allocator found in theSIAllocatorDictionary
//
StandIn *StandIn::AllocateStandIn(WorkSpace *w, Node *node)
{
    StandIn *si = new StandIn(w,node);
    si->createStandIn();
    return si; 
}
	

//
// See ::createStandIn for more code related to initialization. Hmmmmm
//
void StandIn::initialize()
{
    if (!StandIn::ClassInitialized) {
	this->setDefaultResources(theApplication->getRootWidget(),
                              StandIn::DefaultResources);

	// DnD requires the ability to read and write .net and .cfg files.
	if ((theDXApplication->appAllowsSavingNetFile()) &&
	    (theDXApplication->appAllowsSavingCfgFile()) &&
	    (theDXApplication->appAllowsEditorAccess())) {
	    this->addSupportedType (StandIn::Modules, DXMODULES, TRUE);
	    this->addSupportedType (StandIn::Interactors, DXINTERACTOR_NODES,FALSE);
	}
	if (theDXApplication->appAllowsEditorAccess())
	    this->addSupportedType (StandIn::Trash, DXTRASH, FALSE);

	//
	// Delete the atoms set up when starting the drag
	//
	Display *d = XtDisplay(theDXApplication->getRootWidget());
	Atom xoff_atom = XInternAtom (d, DXXOFFSET, True);
	Atom yoff_atom = XInternAtom (d, DXYOFFSET, True);
	Screen *screen = XtScreen(theDXApplication->getRootWidget());
	Window root = RootWindowOfScreen(screen);
	if (xoff_atom != None) 
	    XDeleteProperty (d, root, xoff_atom);
	if (yoff_atom != None) 
	    XDeleteProperty (d, root, yoff_atom);
    }


    if (!StandIn::DragIcon) {
	StandIn::DragIcon = 
		this->createDragIcon(moduledrag_width, moduledrag_height,
				     (char *)moduledrag_bits,
				     (char *)moduledragmask_bits);
    }
    this->setDragIcon(StandIn::DragIcon);
}

Widget StandIn::getOutputParameterLine(int i)
{
    Tab *tab;
    tab = (Tab *) outputTabList.getElement(i);

    return tab->getLine();
}

Widget StandIn::getInputParameterLine(int i)
{
    Tab *tab;
    tab = (Tab *) inputTabList.getElement(i);

    return tab->getLine();
}

void StandIn::setInputParameterLine(int i, Widget w)
{
    Tab *tab;
    tab = (Tab *) inputTabList.getElement(i);

    tab->setLine(w);
}

void StandIn::setOutputParameterLine(int i, Widget w)
{
    Tab *tab;
    tab = (Tab *) outputTabList.getElement(i);

    tab->setLine(w);
}
   
Tab *StandIn::getInputParameterTab(int i)
{
    Tab *tab;
    tab = (Tab *) inputTabList.getElement(i);

    ASSERT(tab);
    return tab;
}

Tab *StandIn::getOutputParameterTab(int i)
{
    Tab *tab;
    tab = (Tab *) outputTabList.getElement(i);

    ASSERT(tab);
    return tab;
}

int StandIn::getInputParameterTabX(int i) 
{ 
    Tab *tab;
    tab = (Tab *) inputTabList.getElement(i);

    return tab->getTabX();
}

int StandIn::getInputParameterTabY(int i) 
{ 
    Tab *tab;
    tab = (Tab *) inputTabList.getElement(i);

    return tab->getTabY();
}

int StandIn::getOutputParameterTabX(int i)
{
    Tab *tab;
    tab = (Tab *) outputTabList.getElement(i);

    return tab->getTabX();
}

int StandIn::getOutputParameterTabY(int i)
{
    Tab *tab;
    tab = (Tab *) outputTabList.getElement(i);

    return tab->getTabY();
}



void StandIn::displayTabLabel(int index, boolean outputTab)
{
char             str1[64];
char             str2[64];
XmString         xmstr1;
XmString         xmstr2;
Dimension        width;
Dimension        height;
Dimension        str1_width;
Dimension        str1_height;
Dimension        str2_width;
Dimension        str2_height;
Parameter        *p;
EditorWorkSpace* workspace;
Position         y;
unsigned char    alignment;
Dimension	 shadow_thickness;
XRectangle	 xrect;
XmFontList       font_list;

    /*network = this->node->getNetwork();*/
    /*editor = network->getEditor();*/
    workspace =  (EditorWorkSpace*)this->workSpace;

    if(!outputTab)
    {
	p = this->node->getInputParameter(index);
	sprintf(str1, "%s", p->getNameString());
	sprintf(str2, "%s", this->node->getNameString());
    }
    else
    {
	p = this->node->getOutputParameter(index);
	sprintf(str1, "%s", this->node->getNameString());
	sprintf(str2, "%s", p->getNameString());
    }

#if 0
    if(!workspace->font_list)
    {
	XFontStruct *font_struct;
	font_struct = 
	    XLoadQueryFont(workspace->display, "-adobe-helvetica*bold-r*--12*");
	if (font_struct)
	{
	    workspace->font_list = XmFontListCreate(font_struct, "small");
	    XSetFont(XtDisplay(this->buttonWidget),
		     workspace->gc,
		     font_struct->fid);
	}
    }
#endif

    // Clear the PB label
    XtVaGetValues(this->buttonWidget,
		  XmNwidth, &width,
		  XmNheight, &height,
		  XmNshadowThickness, &shadow_thickness,
		  XmNfontList, &font_list,
		  NULL);
    XClearArea(XtDisplay(this->buttonWidget), XtWindow(this->buttonWidget),
		shadow_thickness, shadow_thickness, 
		width - 2*shadow_thickness, height - 2*shadow_thickness, False);

    xrect.x = shadow_thickness;
    xrect.y = shadow_thickness;
    xrect.width = width - 2*shadow_thickness;
    xrect.height = height - 2*shadow_thickness;

    xmstr1 = XmStringCreateLtoR(str1, "small_canvas");
    xmstr2 = XmStringCreateLtoR(str2, "small_canvas");

    XmStringExtent(font_list, xmstr1, &str1_width, &str1_height);
    XmStringExtent(font_list, xmstr2, &str2_width, &str2_height);

    y = height/2 - str1_height + 1;
    if(str1_width <= width)
	alignment = XmALIGNMENT_CENTER;
    else
	alignment = XmALIGNMENT_BEGINNING;

    XmStringDraw(XtDisplay(this->buttonWidget),
		 XtWindow(this->buttonWidget),
		 font_list,
		 xmstr1,
		 workspace->gc,
		 0, y, width,
		 alignment,
		 XmSTRING_DIRECTION_L_TO_R,
		 &xrect);

    y = height/2 - 1;
    if(str2_width <= width)
	alignment = XmALIGNMENT_CENTER;
    else
	alignment = XmALIGNMENT_BEGINNING;
    XmStringDraw(XtDisplay(this->buttonWidget),
		 XtWindow(this->buttonWidget),
		 font_list,
		 xmstr2,
		 workspace->gc,
		 0, y, width,
		 alignment,
		 XmSTRING_DIRECTION_L_TO_R,
		 &xrect);

    XmStringFree(xmstr1);
    XmStringFree(xmstr2);
}
void StandIn::clearTabLabel()
{
//    this->setButtonLabel();
XmString         xmstr;
XmFontList       font_list;
Dimension        width;
Dimension        height;
Dimension        str_width;
Dimension        str_height;
EditorWorkSpace* workspace;
Position         y;
Dimension	 shadow_thickness;
XRectangle	 xrect;

    /*network = this->node->getNetwork();*/
    /*editor = network->getEditor();*/
    workspace =  (EditorWorkSpace*)this->workSpace;

    // Clear the PB label
    XtVaGetValues(this->buttonWidget,
		  XmNwidth, &width,
		  XmNheight, &height,
		  XmNfontList, &font_list,
		  XmNshadowThickness, &shadow_thickness,
		  NULL);

    XClearArea(XtDisplay(this->buttonWidget), XtWindow(this->buttonWidget),
		shadow_thickness, shadow_thickness, 
		width - 2*shadow_thickness, height - 2*shadow_thickness, False);

    xrect.x = shadow_thickness;
    xrect.y = shadow_thickness;
    xrect.width = width - 2*shadow_thickness;
    xrect.height = height - 2*shadow_thickness;

    xmstr = this->createButtonLabel();

    XmStringExtent(font_list, xmstr, &str_width, &str_height);

    y = height/2 - str_height/2;

    XmStringDraw(XtDisplay(this->buttonWidget),
		 XtWindow(this->buttonWidget),
		 font_list,
		 xmstr,
		 workspace->gc,
		 0, y, width,
		 XmALIGNMENT_CENTER,
		 XmSTRING_DIRECTION_L_TO_R,
		 &xrect);
    XmStringFree(xmstr);
}

void StandIn::ToggleHotSpots(EditorWindow* editor,
                    Node*    destnode,
                    boolean  on)
{
    Node*  srcnode;
    ListIterator iterator;
    EditorWorkSpace* workspace;
    StandIn* standIn;
    int       i;
    int       icnt,ocnt;
    int       outsrc;
    Tab	      *tab;	


    standIn = destnode->getStandIn();
    workspace = (EditorWorkSpace*)standIn->workSpace;

    ASSERT(editor);

    if (on)
    {
        if (workspace->origin == ORG_SOURCE)
        {
            /*
             * The origin is a source node; therefore, the current node
             * being examined is a destination node, and we must find
             * the source node.
             */
            srcnode  = workspace->src.node;

            outsrc   = workspace->src.param;

            icnt = destnode->getInputCount();
    
            for (i=1 ; i<=icnt ; i++) {
                   if (destnode->isInputConnected(i) || 
                       !destnode->isInputDefaulting(i) ||
                       !destnode->isInputVisible(i))
                       continue;
        
                   if (srcnode->typeMatchOutputToInput(outsrc, destnode, i)) {
			tab = standIn->getInputParameterTab(i);
			tab->setBackground(standInDefaults.io.highlight_background);
                   }
            }
        }
        else
        {

            /*
             * The origin is a destination node; therefore, the current
             * node being examined is a source node, and we must find
             * the destination node.
             */
            srcnode  = workspace->dst.node;
            outsrc   = workspace->dst.param;


            ocnt = destnode->getOutputCount();

            for (i=1 ; i<=ocnt ; i++)
            {
                if (destnode->typeMatchOutputToInput(i, srcnode, outsrc)){
		    tab = standIn->getOutputParameterTab(i);
		    tab->setBackground(standInDefaults.io.highlight_background);
		}
            }
        }

    }
    else
    {

        if (workspace->origin == ORG_SOURCE) {
            icnt = destnode->getInputCount();

            for (i=1 ; i<=icnt ; i++) {
                if (destnode->isInputConnected(i) ||
                    !destnode->isInputDefaulting(i) ||
                    !destnode->isInputVisible(i))
                           continue;

                srcnode  = workspace->src.node;
                outsrc   = workspace->src.param;


                if (srcnode->typeMatchOutputToInput(outsrc, destnode, i)) {

		    tab = standIn->getInputParameterTab(i);
		    tab->setBackground(destnode->isInputRequired(i) ?
                                      standInDefaults.io.required_background :
                                      standInDefaults.io.background);
                 }
            }
        }
        else
        {

            srcnode  = workspace->dst.node;
            outsrc   = workspace->dst.param;

            ocnt = destnode->getOutputCount();

            for (i=1 ; i<=ocnt ; i++) {
                if (destnode->typeMatchOutputToInput(i, srcnode, outsrc)) {
		    tab = standIn->getOutputParameterTab(i);
		    tab->setBackground(standInDefaults.io.background);
		}
            }
        }
    }
}


extern "C" void StandIn_TrackArkEH(Widget widget,
                XtPointer clientData,
                XEvent* event,
		Boolean *)
{
    Node *node = (Node *) clientData;
    StandIn *standIn = node->getStandIn();
    ASSERT(standIn);
    standIn->trackArk(widget, event); 
} 

void StandIn::trackArk(Widget widget, XEvent *event)
{
    Network*      network;
    Node*         node;
    Node*         nodePtr;
    StandIn*      standInPtr;
    StandIn*      curNodeStandInPtr = NUL(StandIn*);
    EditorWindow* editor;
    EditorWorkSpace*    workspace;
    ListIterator  iterator;
    Widget        hot_spot;
    Widget        bboard;
    Window        window;
    int           x,y;
    Widget	  labeled_tab;

    node = this->node;
    network = node->getNetwork();
    editor = network->getEditor();
    workspace =  (EditorWorkSpace*)this->workSpace;

    if (workspace->first)
    {
        workspace->first = FALSE;
    }
    else
    {
        /*
         * Remove the previous line.
         */
        XDrawLine(workspace->display,
                  XtWindow(workspace->getRootWidget()),
                  workspace->gc_xor,
                  workspace->first_x + workspace->start_x,
                  workspace->first_y + workspace->start_y,
                  workspace->last_x  + workspace->start_x,
                  workspace->last_y  + workspace->start_y);
    }

    /*
     * Determine if we are in a node boundary.
     */
    XTranslateCoordinates(workspace->display,
                          XtWindow(widget),
                          XtWindow(workspace->getRootWidget()),
                          event->xbutton.x,
                          event->xbutton.y,
                          &x,
                          &y,
                          &window);

    if (window == None)
    {
        if (workspace->hot_spot)
        {
            ToggleHotSpots
                (editor, workspace->hot_spot_node , FALSE);
                 workspace->hot_spot = NUL(Widget);
        }
	if(workspace->labeled_tab)
	{
	    workspace->labeled_si->clearTabLabel();
	    workspace->labeled_tab = NULL;
	}
    }
    else
    {
        /*
         * Convert windows to widgets.
         */
        hot_spot = XtWindowToWidget(workspace->display, window);

        FOR_EACH_NETWORK_NODE(network, nodePtr, iterator) 
	{
            standInPtr = nodePtr->getStandIn();
#if WORKSPACE_PAGES
	    if (standInPtr == NUL(StandIn*)) continue;
#endif
            bboard = standInPtr->getRootWidget();

	    if(hot_spot == bboard) curNodeStandInPtr = standInPtr;

	    /*
	     * Turn off the previous hot spot.
	     */
	    if ((bboard == workspace->hot_spot) AND
		(bboard != hot_spot)            AND
		workspace->hot_spot)
	    {
		ToggleHotSpots
		    (editor, nodePtr, FALSE);
		workspace->hot_spot = NUL(Widget);
	    }

	    /*
	     * Turn on the new hot spot.
	     */
            if ((bboard  == hot_spot) AND
                (bboard  != workspace->source_spot) AND
		(NOT workspace->hot_spot))

            {
		workspace->hot_spot      = hot_spot;
		workspace->hot_spot_node = nodePtr;

		ToggleHotSpots
		    (editor, nodePtr, TRUE);
            }
        }
	/*
	 * Determine if we are in a tab boundary.
	 */
	XTranslateCoordinates(workspace->display,
			      XtWindow(widget),
			      window,
			      event->xbutton.x,
			      event->xbutton.y,
			      &x,
			      &y,
			      &window);
	labeled_tab = XtWindowToWidget(workspace->display, window);

	if(labeled_tab AND 
	   (labeled_tab != workspace->labeled_tab)) 
	{
	    workspace->labeled_si->clearTabLabel();
	    workspace->labeled_tab = NULL;
	}
	if ((labeled_tab) && (curNodeStandInPtr))
	{
	    Tab *tab;
	    int i;
	    boolean found;

	    found = FALSE;
	    int count = curNodeStandInPtr->node->getInputCount();
	    for (i = 1; i <= count ; i++ ) 
	    {
		tab = (Tab *) curNodeStandInPtr->inputTabList.getElement(i);
		if(labeled_tab == tab->getRootWidget())
		{
		    if(workspace->labeled_tab != labeled_tab)
		    {
			workspace->labeled_tab = labeled_tab;
			workspace->labeled_si = curNodeStandInPtr;
			curNodeStandInPtr->displayTabLabel(i, FALSE);
		    }
		    found = TRUE;
		    break;
		}
	    }
	    if(!found)
	    {
	        count = curNodeStandInPtr->node->getOutputCount();
		for (i = 1; i <= count; i++)
		{
		    tab = (Tab *)curNodeStandInPtr->outputTabList.getElement(i);
		    if(labeled_tab == tab->getRootWidget())
		    {
			if(workspace->labeled_tab != labeled_tab)
			{
			    workspace->labeled_tab = labeled_tab;
			    workspace->labeled_si = curNodeStandInPtr;
			    curNodeStandInPtr->displayTabLabel(i, TRUE);
			}
			found = TRUE;
			break;
		    }
		}
	    }
	    if(!found && workspace->labeled_tab)
	    {
		curNodeStandInPtr->clearTabLabel();
		workspace->labeled_tab = NULL;
	    }
	}
	else if ((workspace->labeled_tab) && (curNodeStandInPtr))
	{
	    curNodeStandInPtr->clearTabLabel();
	    workspace->labeled_tab = NULL;
	}
    }

    /*
     * Draw the new line.
     */
    workspace->last_x = event->xmotion.x_root;
    workspace->last_y = event->xmotion.y_root;

    XDrawLine(workspace->display,
              XtWindow(workspace->getRootWidget()),
              workspace->gc_xor,
              workspace->first_x + workspace->start_x,
              workspace->first_y + workspace->start_y,
              workspace->last_x  + workspace->start_x,
              workspace->last_y  + workspace->start_y);
}


extern "C" void StandIn_ArmInputCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata)
{
    Node *node = (Node *) clientData;
    StandIn *standIn = node->getStandIn();
    ASSERT(standIn);
    standIn->armInput(widget, cdata); 
} 

void StandIn::armInput(Widget widget, XtPointer cdata)
{
    Node*         node;
    Node*         node2;
    StandIn*      standIn2;
    EditorWorkSpace*    workspace;
    ListIterator  iterator;
    XEvent*       event;
    Ark*          a;
    List*         arcs;
    int           paramInd;
    Tab*          tab;
    int           n;
    int           i;
    int           j;
    Position      nx;
    Position      ny;
    Position      px;
    Position      py;
    int           cursor;
    Arg           arg[4];
    XmAnyCallbackStruct* callData = (XmAnyCallbackStruct*) cdata;


    ASSERT(widget);

    node = this->node; 
    /*network = node->getNetwork();*/
    /*editor = network->getEditor();*/
    workspace =  (EditorWorkSpace*)this->workSpace;


    event   = callData->event;

    if (event->type != ButtonPress)
    {
        return;
    }

    /*
     * Get pointers to the node and the param of the destination
     * end of the arc and remember it.
     */


    iterator.setList(this->inputTabList);
    for (i = 1; (tab = (Tab*)iterator.getNext()) ; i++) {
        if (tab->getRootWidget() == widget) {
            break;
        }
    }

    workspace->src.node  = NULL;
    workspace->src.param = -1;

    // Display the parameter name
    workspace->labeled_tab = widget;
    workspace->labeled_si = this;
    this->displayTabLabel(i, FALSE);

    /*
     * If this input parameter is "pressed in" (in use),
     * then no need to go further.
     */
    if (!node->isInputDefaulting(i) &&
	!node->isInputConnected(i))
    {
        workspace->dst.node  = NULL;
        workspace->dst.param = -1;
        return;
    }

    /*
     * Get the current locations of the node and param.
     */
    Widget standInRoot = this->getRootWidget();
    n = 0;
    XtSetArg(arg[n], XmNx, &nx); n++;
    XtSetArg(arg[n], XmNy, &ny); n++;
    XtGetValues(standInRoot, arg, n);

    n = 0;
    XtSetArg(arg[n], XmNx, &px); n++;
    XtSetArg(arg[n], XmNy, &py); n++;
    XtGetValues(widget, arg, n);

    /*
     * Set up tracking params.
     */
    workspace->tracker     = (XtEventHandler)StandIn_TrackArkEH;
    workspace->io_tab      = widget;
    workspace->source_spot = standInRoot;
    workspace->first       = TRUE;

    /*
     * Get current node and param location.
     */
    workspace->first_x =
        (event->xbutton.x_root - event->xbutton.x) 
        + standInDefaults.io.width / 2;

    workspace->first_y =
        (event->xbutton.y_root - event->xbutton.y);

    workspace->start_x =
        (nx + px + event->xbutton.x) - event->xbutton.x_root;
    workspace->start_y =
        (ny + py + event->xbutton.y) - event->xbutton.y_root;

    if (node->isInputConnected(i))
    {
	  // Remember the dst node and param so that we can set the param's
	  // defaulting status if the user winds up deleting an arc.
	  workspace->dst.node  = node;
	  workspace->dst.param = i; 

        /*
         * If the input tab is already connected, we need to anchor the
         * rubber band line from the output node....
         * So, find the other end, and remember it.
         */


          arcs = (List *) node->getInputArks(i);

          for (j = 1; (a = (Ark*) arcs->getElement(j)) != NULL; j++) {
            a->getDestinationNode(paramInd);
            if (paramInd == i) {
              break;
            }
          }

          workspace->remove_arcs = True;
          workspace->arc = a;

          node2 = a->getSourceNode(paramInd);

          standIn2 = node2->getStandIn();

          ASSERT(node2);

          workspace->src.node  = node2;
          workspace->src.param = paramInd;

        /*
         * Get the pointers to the source node and param.
         */

        /*
         * Start rubber band from source node.
         */
        workspace->first_x -= nx + px;
        workspace->first_y -= ny + py;

        /*
         * Get (x, y) of the bulletinboard and output tab widget.
         */
        n = 0;
        XtSetArg(arg[n], XmNx, &nx); n++;
        XtSetArg(arg[n], XmNy, &ny); n++;
        XtGetValues(standIn2->getRootWidget(), arg, n);

        n = 0;
        XtSetArg(arg[n], XmNx, &px); n++;
        XtSetArg(arg[n], XmNy, &py); n++;
        XtGetValues(standIn2->getOutputParameterTab(paramInd)->getRootWidget(), 		arg, n);

        workspace->first_x += nx + px;
        workspace->first_y += ny + py + standInDefaults.io.height;

        workspace->origin = ORG_SOURCE;

        cursor = DOWN_CURSOR;


        /*
         * Reset the source widget to be the source node.
         */
        workspace->source_spot = standIn2->getRootWidget();
    }
    else
    {
        /*
         * If the input tab is not currently connected,
         * then there is no source (output) tab to disconnect.
         */

        workspace->dst.node  = node;
        workspace->dst.param = i; 

        workspace->origin = ORG_DESTINATION;

        cursor = UP_CURSOR;
    }

    /*
     * Prepare for tracking.
     */
    XGrabPointer(workspace->display,
                 XtWindow(widget),
                 FALSE,
                 ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
                 GrabModeAsync,
                 GrabModeAsync,
                 XtWindow(workspace->getRootWidget()),
                 workspace->cursor[cursor],
                 CurrentTime);
    /*
     * Draw the initial line.
     */
    workspace->last_x = event->xmotion.x_root;
    workspace->last_y = event->xmotion.y_root;

    XDrawLine(workspace->display,
              XtWindow(workspace->getRootWidget()),
              workspace->gc_xor,
              workspace->first_x + workspace->start_x,
              workspace->first_y + workspace->start_y,
              workspace->last_x  + workspace->start_x,
              workspace->last_y  + workspace->start_y);

    workspace->first = FALSE;

    /*
     * Add cursor tracking handler.
     */
    XtAddEventHandler(widget,
                      ButtonMotionMask,
                      FALSE,
                      workspace->tracker,
                      (XtPointer)node);
}


extern "C" void StandIn_ArmOutputCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata)
{
    Node *node = (Node *) clientData;
    StandIn *standIn = node->getStandIn();
    ASSERT(standIn);
    standIn->armOutput(widget, cdata); 
} 

void StandIn::armOutput(Widget widget, XtPointer cdata)
{
    Node*    node;
    EditorWorkSpace*    workspace; 
    XEvent*     event;
    int         n;
    int         i;
    Position    nx;
    Position    ny;
    Position    px;
    Position    py;
    int         cursor;
    Arg         arg[4];
    XmAnyCallbackStruct* callData = (XmAnyCallbackStruct*) cdata;
    Tab         *tab;

    ASSERT(widget);

    node = this->node; 
    /*network = node->getNetwork();*/
    /*editor = network->getEditor();*/
    workspace =  (EditorWorkSpace*)this->workSpace;


    event   = callData->event;

    if (event->type != ButtonPress)
    {
        return;
    }

    /*
     * Get pointers to the node and the param.
     */

    ListIterator iterator(this->outputTabList);
    for (i = 1; (tab = (Tab*)iterator.getNext()) ; i++) {
        if (tab->getRootWidget() == widget) {
            break;
        }
    }
   
    // Display the parameter name
    workspace->labeled_tab = widget;
    workspace->labeled_si = this;
    this->displayTabLabel(i, TRUE);

    /*
     * Remember the source (output) end of the arc (no destination yet).
     */
    workspace->src.node  = node;
    workspace->src.param = i;

    workspace->dst.node  = NULL;
    workspace->dst.param = -1;


    /*
     * Get the current locations of the node and param.
     */
    Widget standInRoot = this->getRootWidget();
    n = 0;
    XtSetArg(arg[n], XmNx, &nx); n++;
    XtSetArg(arg[n], XmNy, &ny); n++;
    XtGetValues(standInRoot, arg, n);

    n = 0;
    XtSetArg(arg[n], XmNx, &px); n++;
    XtSetArg(arg[n], XmNy, &py); n++;
    XtGetValues(tab->getRootWidget(), arg, n);

    /*
     * Set up tracking params.
     */
    workspace->tracker     = (XtEventHandler)StandIn_TrackArkEH;
    workspace->io_tab      = widget;
    workspace->source_spot = standInRoot;
    workspace->first       = TRUE;

    workspace->origin = ORG_SOURCE;

    cursor = DOWN_CURSOR;

    workspace->first_x =
        (event->xbutton.x_root - event->xbutton.x) + 
         standInDefaults.io.width / 2;
    workspace->first_y =
        (event->xbutton.y_root - event->xbutton.y) + standInDefaults.io.height;

    workspace->start_x =
        (nx + px + event->xbutton.x) - event->xbutton.x_root;
    workspace->start_y =
        (ny + py + event->xbutton.y) - event->xbutton.y_root;

    /*
     * Prepare for tracking.
     */
    XGrabPointer(workspace->display,
                 XtWindow(widget),
                 FALSE,
                 ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
                 GrabModeAsync,
                 GrabModeAsync,
                 XtWindow(workspace->getRootWidget()),
                 workspace->cursor[cursor],
                 CurrentTime);
    /*
     * Add cursor tracking handler.
     */
    XtAddEventHandler(widget,
                      ButtonMotionMask,
                      FALSE,
                      workspace->tracker,
                      (XtPointer)node);
}

void StandIn::deleteArk(Ark *a)
{
    EditorWindow* editor = this->node->getNetwork()->getEditor();
    if (editor) editor->notifyArk(a,FALSE);
    delete a;
}

extern "C" void StandIn_DisarmTabCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata)
{
    XUngrabPointer(XtDisplay(widget), CurrentTime);
    XFlush(XtDisplay(widget));
    Node *node = (Node *) clientData;
    StandIn *standIn = node->getStandIn();
    ASSERT(standIn);
    standIn->disarmTab(widget, cdata); 
} 

void StandIn::disarmTab(Widget widget, XtPointer cdata)
{
    XEvent*        event;
    Network*       network;
    Node*          node;
    Node*          nodePtr = NULL;
    EditorWorkSpace*     workspace;
    EditorWindow*  editor;
    StandIn	   *standIn = NULL;
    int            icnt;
    ListIterator   iterator;
    Node*          node2;
    Node*          to_node = NULL;
    int            to_param = -1;
    Ark*           newArk;
    int            param2;
    int         i;
    int         x;
    int         y;
    int         ocnt;
    Widget      n_widget;
    Widget      t_widget = (Widget)None;
    Window      n_window;
    Window      t_window;
    XmAnyCallbackStruct* callData = (XmAnyCallbackStruct*) cdata;
    static char *type_error = 
	"Output parameter type and input parameter type do not match.";
    static char *cycle_error = "Connections cannot be cyclic.";
    static char *connect_error = "The input is already connected or set.";

    ASSERT(widget);
    ASSERT(callData);

    node = this->node; 
    network = node->getNetwork();
    editor = network->getEditor();
    workspace =  (EditorWorkSpace*)this->workSpace;

    event   = callData->event;

    /*
     * Release the pointer.
     */
    XUngrabPointer(workspace->display, CurrentTime);

    /*
     * If there is a tracking routine installed, remove it.
     * Otherwise, no need to process further (since one of
     * the arming routines didn't need to track the cursor).
     */
    if (workspace->tracker)
    {
        XtRemoveEventHandler(workspace->io_tab,
                             ButtonMotionMask,
                             FALSE,
                             workspace->tracker,
                             (XtPointer)node);
        workspace->tracker = NUL(XtEventHandler);
    }
    else
    {
	// Clear the parameter name if required
	if(workspace->labeled_tab)
	{
	    workspace->labeled_si->clearTabLabel();
	    workspace->labeled_tab = NULL;
	}
        return;
    }

    /*
     * Determine where the cursor stopped.
     */
    Window src_w = XtWindow(widget);
    Window dst_w = XtWindow(workspace->getRootWidget());
    XTranslateCoordinates(workspace->display,
                          src_w, //XtWindow(widget),
                          dst_w, //XtWindow(workspace->getRootWidget()),
                          event->xbutton.x,
                          event->xbutton.y,
                          &x,
                          &y,
                          &n_window);

    /*
     * If the release point is in the node....
     */
    if (n_window != None)
    {

        /*
         * Determine which node bboard widget the cursor is in.
         */
        XTranslateCoordinates(workspace->display,
                              XtWindow(workspace->getRootWidget()),
                              n_window,
                              x,
                              y,
                              &x,
                              &y,
                              &t_window);

        n_widget = XtWindowToWidget(workspace->display, n_window);

        FOR_EACH_NETWORK_NODE(network, nodePtr, iterator) 
        {
            standIn = nodePtr->getStandIn();
#if WORKSPACE_PAGES
	    if (!standIn) continue;
#endif
            if (standIn->getRootWidget() == n_widget) {
		if (standIn->workSpace != workspace) {
		    ErrorMessage ("Connections cannot cross page boundaries");
		    /*
		     * Remove the last line.
		     */
		    XDrawLine(workspace->display,
			      XtWindow(workspace->getRootWidget()),
			      workspace->gc_xor,
			      workspace->first_x + workspace->start_x,
			      workspace->first_y + workspace->start_y,
			      workspace->last_x + workspace->start_x,
			      workspace->last_y + workspace->start_y);
		    return ;
		}
                break;
            }
        }

	if (!nodePtr) {
	    /*
	     * Remove the last line.
	     */
	    XDrawLine(workspace->display,
		      XtWindow(workspace->getRootWidget()),
		      workspace->gc_xor,
		      workspace->first_x + workspace->start_x,
		      workspace->first_y + workspace->start_y,
		      workspace->last_x + workspace->start_x,
		      workspace->last_y + workspace->start_y);
	    return ;
	}

        /*
         * Determine which child widget of the node bulletinboard widget
         * the cursor is in, if any.
         */
        if (t_window == None)
        {
            t_widget = NUL(Widget);
        }
        else
        {
            t_widget = XtWindowToWidget(workspace->display, t_window);
        }

        /*
         * If the pointer is in the node button widget or just on the
         * bulletin widget, calculate the left-most legal connection,
         * if any.
         */
        if (t_widget == standIn->buttonWidget OR t_widget == NUL(Widget))
        {
            if (workspace->origin == ORG_SOURCE)
            {
                node2  = workspace->src.node;
                param2 = workspace->src.param;

                icnt = nodePtr->getInputCount();
                for(i=1; i<=icnt; i++) {
                    if (nodePtr->isInputConnected(i) ||
                        !nodePtr->isInputDefaulting(i) ||
                        !nodePtr->isInputVisible(i))
                        continue;
                    //
                    // Make sure the input types match 
                    //
                    if (node2->typeMatchOutputToInput(param2, 
                                                      nodePtr, 
                                                      i))
                        {
                            to_node = nodePtr;
                            to_param = i;
                            break;
                        }
                    }
            }
            else
            {
                node2  = workspace->dst.node;
                param2 = workspace->dst.param;

                ocnt = nodePtr->getOutputCount();
                for(i=1; i<=ocnt; i++) {
                    if (!nodePtr->isOutputVisible(i))
                        continue;
                    if (nodePtr->typeMatchOutputToInput(i, node2, param2))
                        {
                            to_node = nodePtr;
                            to_param = i;
                            break;
                        }
                }
            }
        }

        /*
         * Else if the cursor is in a input tab widget, determine which
         * input parameter it corresponds to.
         */
        else if (workspace->origin == ORG_SOURCE)
        {
            node2  = workspace->src.node;
            param2 = workspace->src.param;

            icnt = nodePtr->getInputCount();
            for(i=1; i<=icnt; i++) {
                if (standIn->getInputParameterTab(i)->getRootWidget()==t_widget)
                {
		    if(!nodePtr->isInputDefaulting(i)){
		    	if(widget == t_widget) break;
			ErrorMessage(connect_error);
			break;
		    }
                    if (node2->typeMatchOutputToInput(param2,
                                                      nodePtr,
                                                      i)) {
                        to_node = nodePtr;
                        to_param = i;
                    } else { 
			ErrorMessage(type_error);
                    }
		    break;
                }
            }
        /*
         * Else the cursor has to be in an output tab widget; determine
         * which output parameter it corresponds to.
         */
        } else {
                node2  = workspace->dst.node;
                param2 = workspace->dst.param;

                ocnt = nodePtr->getOutputCount();

                for(i=1; i<=ocnt; i++) {
                    if (standIn->getOutputParameterTab(i)->getRootWidget() == t_widget)
                    {
                        if (nodePtr->typeMatchOutputToInput(i, node2, param2)) 
                        {
                            to_node = nodePtr;
                            to_param = i;
                        } else {
			    ErrorMessage(type_error);
                        }
                        break;
                    }

                }
          }
     }


    /*
     * If cursor moved...
     */
    if (NOT workspace->first)
    {
        /*
         * Remove the last line.
         */
        XDrawLine(workspace->display,
                  XtWindow(workspace->getRootWidget()),
                  workspace->gc_xor,
                  workspace->first_x + workspace->start_x,
                  workspace->first_y + workspace->start_y,
                  workspace->last_x + workspace->start_x,
                  workspace->last_y + workspace->start_y);


	// Clear the parameter name if required
	if(workspace->labeled_tab)
	{
	    workspace->labeled_si->clearTabLabel();
	    workspace->labeled_tab = NULL;
	}

        /*
         * Remove hot spots.
         */
        if (workspace->hot_spot && nodePtr != NULL)
        {
            ToggleHotSpots(editor, nodePtr, FALSE);
            workspace->hot_spot = NUL(Widget);
        }

        /*
         * Delete the old arc, if applicable.
         */
	if(workspace->io_tab == t_widget) workspace->remove_arcs = FALSE;
        /*notified = FALSE;*/
        if (workspace->remove_arcs) {
	      if (!standIn) {
		  int dummy;
		  Node* src = workspace->arc->getSourceNode(dummy);
		  standIn = src->getStandIn();
	      }
              standIn->deleteArk(workspace->arc);
              workspace->remove_arcs = False;

	      // Reset the Defaulting status of the dst param

	      node2  = workspace->dst.node;
	      param2 = workspace->dst.param;
	      node2->setIODefaultingStatus(param2, TRUE, TRUE, FALSE);
        }

        /*
         * Cause the originating parameter tab to be redrawn.
         */
        if (workspace->origin == ORG_SOURCE)
        {
            node2  = workspace->src.node;
            param2 = workspace->src.param;
        }
        else
        {
            node2  = workspace->dst.node;
            param2 = workspace->dst.param;
        }
        XtUnmanageChild(widget);
        XtManageChild(widget);



//
// Was any matching parameter found ? If not just return.
//
        if (to_node == NULL)
           return;

        /*
         * Now add the new arc, if applicable.
         */

        if (workspace->origin == ORG_SOURCE) {
            // 
            // Make sure adding this arc won't add a cycle
            //
            if (!network->checkForCycle(node2, to_node)) {
                newArk = new Ark(node2, 
                        param2,  
                        to_node, 
                        to_param);
                editor->notifyArk(newArk);
            } else  {
		/* don't complain if you drop the arc back on the
		 * originating node.  this often happens when
		 * disconnecting existing arcs.
		 */
		if (node2 != to_node)
		    ErrorMessage(cycle_error);
            }
        } else {
            if (!network->checkForCycle(to_node, workspace->dst.node)) {
                newArk = new Ark(to_node, 
                        to_param, 
                        workspace->dst.node, 
                        workspace->dst.param);
                editor->notifyArk(newArk);
            } else  {
		/* see comment above about existing arcs */
		if (to_node != workspace->dst.node)
		    ErrorMessage(cycle_error);
            }

        }
    }

    // Clear the parameter name if required
    if(workspace->labeled_tab)
    {
	workspace->labeled_si->clearTabLabel();
	workspace->labeled_tab = NULL;
    }

    /*
     * Reinitialize the node descriptors.
     */
    workspace->origin = ORG_NONE;

    workspace->src.node  = NULL;
    workspace->src.param = 0;
    workspace->dst.node  = NULL;
    workspace->dst.param = 0;

    workspace->first       = TRUE;
    workspace->io_tab      = NUL(Widget);
    workspace->source_spot = NUL(Widget);
}




// 
// was _uinSelectNodeCB
//
extern "C" void StandIn_SelectNodeCB(Widget,
                XtPointer clientData,
                XtPointer cdata)
{
    XmWorkspaceChildCallbackStruct* cb =(XmWorkspaceChildCallbackStruct*)cdata;
    StandIn* standin= (StandIn*) clientData;

    standin->setSelected(cb->status);
    
}

void StandIn::handleSelectionChange(boolean select)
{
    Node *node = this->node;
    ASSERT(node);
    EditorWindow *editor = node->network->editor;
    ASSERT(editor);

    editor->handleNodeStatusChange(node,
	select? Node::NodeSelected: Node::NodeDeselected);
}

void StandIn::setButtonLabel()
{
    XmString string = this->createButtonLabel();
    Widget button = this->buttonWidget;
#if 0
    XtVaSetValues(button,
	XmNlabelString, string,
	NULL);
#else
    Arg arg[1];
    XtSetArg(arg[0], XmNlabelString, string);
    XtSetValues(button,arg,1);
#endif

    XmFontList fonts;
#if 0
    XtVaGetValues(button, XmNfontList, &fonts, NULL);
#else
    XtSetArg(arg[0], XmNfontList, &fonts);
    XtGetValues(button,arg,1);
#endif

    Dimension buttonHeight;
    Dimension buttonWidth;
    XmStringExtent(fonts, string, &buttonWidth, &buttonHeight);

    XmStringFree(string);

    this->requiredButtonWidth = buttonWidth + 6;

    int width;
    this->setMinimumWidth(width);
    this->adjustParameterLocations(width);
}

XmString StandIn::createButtonLabel()
{
    XmString xms;
    const char *label = this->getButtonLabel();
    const char *font = this->getButtonLabelFont();

    if (theDXApplication->showInstanceNumbers()) {
        char buffer[128];
	sprintf(buffer,"%s:%d",label,this->node->getInstanceNumber());
	label = buffer;
    } 
    xms = XmStringCreateLtoR((char*)label, (char*)font);
    return xms;
}

const char* StandIn::getButtonLabel()
{
   return this->node->getNameString(); 
}
const char* StandIn::getButtonLabelFont()
{
   return "canvas";
}

int StandIn::getVisibleOutputCount()
{
    int i;
    int vo = 0;

    for (i=1; i <= node->getOutputCount(); i++)
      if (node->isOutputVisible(i))
        vo++;

    return vo;
}

int StandIn::getVisibleInputCount()
{
    int i;
    int vi = 0;

    for (i=1; i <= node->getInputCount(); i++)
      if (node->isInputVisible(i) && node->isInputViewable(i))
        vi++;

    return vi;
}

void StandIn::adjustParameterLocations(int width)
{
    int             vi, vo, i, x, j;
    int             icnt, ocnt;
    List            *arcs;
    Ark             *a;
    ArkStandIn      *asi;
    Network         *network;
    EditorWindow    *editor;
    Tab             *tab;


    vi = getVisibleInputCount();
    icnt = node->getInputCount();
    ocnt = node->getOutputCount();

    j = 0;  // use j to keep track of the visible parameters 


    for (i = 1; i <= icnt; i++) {
        if ( (node->isInputVisible(i) == FALSE) ||
	     (node->isInputViewable(i) == FALSE) )
           continue;

        tab = (Tab *) inputTabList.getElement(i);
	if (tab == NULL)
	    continue;

        j++;

        x = ((2 * (j-1) + 1) * (width)) /
            (2 * vi) - (standInDefaults.io.width / 2);

	tab->moveTabX(x, TRUE);

        x =  (x + standInDefaults.io.width / 2) -
              standInDefaults.line.thickness / 2;
        if (tab->getLine() != NULL) {
            arcs = (List *) this->node->getInputArks(i);
            if (arcs->getSize() != 0) {
                a = (Ark *) arcs->getElement(1);
                asi = a->getArkStandIn();
		if(asi) delete asi;
                network = node->getNetwork();
                editor = network->getEditor();
                addArk(editor, a);
            }
        }
    }
    

    vo = getVisibleOutputCount();

    j = 0;

    for (i = 1; i <= ocnt; i++) {
        if (node->isOutputVisible(i) == FALSE)
           continue;

        tab = (Tab *) outputTabList.getElement(i);
	if (tab == NULL)
	    continue;

        j++;

        x = ((2 * (j-1) + 1) * (width)) /
          (2 * vo) - (standInDefaults.io.width / 2);
        /*n = 0;*/

	//
	// Move it and show it
	//
	tab->moveTabX(x, TRUE);
	tab->manage();

        if (tab->getLine() != NULL) {
            arcs = (List *) this->node->getOutputArks(i); 

	    int k;
            for (k = 1; k <= arcs->getSize(); k++) {
                a = (Ark *) arcs->getElement(k);
                asi = a->getArkStandIn();
                if (asi) delete asi;
                network = node->getNetwork();
                editor = network->getEditor();
                addArk(editor, a);
            }
        }
    }
}

Tab *StandIn::createOutputTab(Widget, int ndx, int width)
{
    int      i;
    Position x;
    int      visible_outputs;
    Tab      *tab = new Tab(this);
    int	     vindex;

    visible_outputs = getVisibleOutputCount();

    vindex = 0;
    for(i = 1; i <= ndx; i++)
	if(node->isOutputVisible(i)) vindex++ ;

    if(visible_outputs > 0)
	x = ((2 * (vindex-1) + 1) * (width)) /
	      (2 * visible_outputs) - (standInDefaults.io.width / 2);
    else
	x = 0;


    tab->createTab(this->getRootWidget());
    XtAddEventHandler (tab->getRootWidget(), KeyPressMask, False,
	(XtEventHandler)StandIn_TabKeyNavEH, (XtPointer)NULL);

    /*
     * Set background color.
     */

    tab->setBackground(standInDefaults.io.background);

    Position y = this->buttonHeight + standInDefaults.io.height;
    tab->moveTabXY(x, y, TRUE);

    Widget tabRoot = tab->getRootWidget();
    XtAddCallback(tabRoot,
                      XmNarmCallback,
                      (XtCallbackProc)StandIn_ArmOutputCB,
                      (XtPointer)this->node);

    XtAddCallback(tabRoot,
                      XmNdisarmCallback,
                      (XtCallbackProc)StandIn_DisarmTabCB,
                      (XtPointer)this->node);

#if OLD_LESSTIF == 1
    char *str = "\
    	<Btn1Down>: Arm() \n\
    	<Btn1Up>: Disarm()";
    XtVaSetValues(tabRoot, XmNtranslations,  XtParseTranslationTable(str), NULL);
#endif

    return tab;
}

Tab *StandIn::createInputTab(Widget, int ndx, int width)
{
    int      i;
    int      visible_inputs;
    Position x;
    Tab      *tab = new Tab(this);
    int	     vindex;

    visible_inputs = getVisibleInputCount();

    vindex = 0;
    for(i = 1; i <= ndx; i++)
	if(node->isInputVisible(i)) vindex++ ;

    if(visible_inputs > 0)
	x = ((2 * (vindex-1) + 1) * (width)) /
	      (2 * visible_inputs) - (standInDefaults.io.width / 2);
    else
	x = 0;


    tab->createTab(this->getRootWidget());
    XtAddEventHandler (tab->getRootWidget(), KeyPressMask, False,
	(XtEventHandler)StandIn_TabKeyNavEH, (XtPointer)NULL);

    /*
     * Set background color.
     */

    if (node->isInputRequired(ndx)) {
        tab->setBackground(standInDefaults.io.required_background);
    } else {
        tab->setBackground(standInDefaults.io.background);
    }

    tab->moveTabXY(x, 0, TRUE);

    XtAddCallback(tab->getRootWidget(),
                      XmNarmCallback,
                      (XtCallbackProc)StandIn_ArmInputCB,
                      (XtPointer)this->node);

    XtAddCallback(tab->getRootWidget(),
                      XmNdisarmCallback,
                      (XtCallbackProc)StandIn_DisarmTabCB,
                      (XtPointer)this->node);
#if OLD_LESSTIF == 1
    char *str = "\
    	<Btn1Down>: Arm() \n\
    	<Btn1Up>: Disarm()";
    XtVaSetValues(tab->getRootWidget(), XmNtranslations,  XtParseTranslationTable(str), NULL);
#endif

    return tab;
}


void StandIn::createStandIn()
{
    Network              *network;
    long                 mask;
    ListIterator         iterator;
    Tab                  *tab;
    int                  n;
    Arg                  arg[100];
    int                  i;
    int                  icnt,ocnt;
    Dimension            height;
    Dimension            total_width;
    int                  width;
    Position             y;
    XmString       	 string;
    WorkSpace		*workspace;

    ASSERT(this->node);
    this->initialize();
    network = this->node->getNetwork();
    /*editor = network->getEditor();*/
    workspace = (EditorWorkSpace*)this->workSpace;
    Widget parent = workspace->getRootWidget();

    /***
     *** Create the node bulletin board.
     ***/

    n = 0;
    XtSetArg(arg[n], XmNx,            node->getVpeX());    n++;
    XtSetArg(arg[n], XmNy,            node->getVpeY());    n++;
    XtSetArg(arg[n], XmNmarginHeight, 0);                  n++;
    XtSetArg(arg[n], XmNmarginWidth,  0);                  n++;
    XtSetArg(arg[n], XmNdialogStyle,  XmDIALOG_WORK_AREA); n++;
    XtSetArg(arg[n], XmNresizePolicy, XmRESIZE_NONE);      n++;
    XtSetArg(arg[n], XmNuserData,     network);   n++;

    Widget standInRoot = XtCreateWidget
            (this->name,
             xmBulletinBoardWidgetClass,
             parent,
             arg,
             n);

    this->setRootWidget(standInRoot);
    this->setDragWidget(standInRoot);

    //
    // Provide Button2 selectability.  Prefer doing it via translation inside
    // Workspace widget however DragSource already applies a Button2 translation
    // which takes precedence.  Would have to either override that, or else
    // provide a method for supplying the DragSource's preferred translation.
    // This is fast, easy, but not flexible.
    //
    XtAddEventHandler (standInRoot, ButtonPressMask, False,
        (XtEventHandler)StandIn_Button2PressEH, (XtPointer)NULL);
    XtAddEventHandler (standInRoot, ButtonReleaseMask, False,
        (XtEventHandler)StandIn_Button2ReleaseEH, (XtPointer)NULL);

    //
    // Do this here, because we can do getResources() until we have 
    // a root widget. 
    //
    if (!StandIn::ClassInitialized) {

        standInDefaults.io.background=theDXApplication->getStandInBackground();
	this->getResources((XtPointer)&standInDefaults,
			_SIColorResources, XtNumber(_SIColorResources));
    	this->getResources((XtPointer)&standInDefaults,
		       _SIResourceList,
		       XtNumber(_SIResourceList));
	StandIn::ClassInitialized = TRUE;
    }

    XtVaGetValues(parent, XmNlineThickness, &(standInDefaults.line.thickness), NULL);

    standInDefaults.line.thickness = MAX(standInDefaults.line.thickness, 1);

    //
    // Mark node as unselected 
    //

    /*
     * Register selection callback.
     */
    this->selected = FALSE;   
    XmWorkspaceAddCallback
        (standInRoot,
        XmNselectionCallback,
        (XtCallbackProc)StandIn_SelectNodeCB,           
        (XtPointer)this);  

    /*
     * Determine the border width of the bulletinboard widget
     */

    string = this->createButtonLabel();
    
    /*
     * Create name button.
     * armColor added to the list to accompany background because Motif does 
     * adds a white border when the color map fills up if armColor isn't set.
     */
    n = 0;
    XtSetArg(arg[n], XmNy, standInDefaults.io.height); 		         n++;
    XtSetArg(arg[n], XmNborderWidth, 0);                      		 n++;
    XtSetArg(arg[n], XmNx,           0);                                 n++;
    XtSetArg(arg[n], XmNforeground,  standInDefaults.bboard.foreground); n++;
    XtSetArg(arg[n], XmNbackground,  standInDefaults.io.background); n++;
    XtSetArg(arg[n], XmNarmColor,    standInDefaults.io.background); n++;
    XtSetArg(arg[n], XmNlabelString, string);                            n++;

    Widget button = this->buttonWidget =
        XtCreateManagedWidget("standInButton",
                               xmPushButtonWidgetClass, 
                               standInRoot, arg, n);

    XmStringFree(string);

    /*
     * Get created width.
     */
    n = 0;
    XtSetArg(arg[n], XmNheight, &height);      n++;
    XtSetArg(arg[n], XmNwidth, &this->requiredButtonWidth);      n++;
    XtGetValues(button, arg, n);


    this->setMinimumWidth(width);

    total_width = width;
    height += 2 * standInDefaults.io.height;

    this->buttonHeight = height;

    n = 0;
    XtSetArg(arg[n], XmNheight,        this->buttonHeight); n++;
    XtSetArg(arg[n], XmNrecomputeSize, FALSE);  n++;
    XtSetValues(button, arg, n);


    /***
     *** Create input param buttons.
     ***/

    string = XmStringCreateLtoR("", "canvas");

    y  = 0;
#if 0
    n0 = 0;
    XtSetArg(arg[n0], XmNlabelString, string);                       n0++;
    XtSetArg(arg[n0], XmNwidth,       standInDefaults.io.width);     n0++;
    XtSetArg(arg[n0], XmNheight,      standInDefaults.io.height);    n0++;
    XtSetArg(arg[n0], XmNborderWidth, 0);                 n0++;
    XtSetArg(arg[n0], XmNfillOnArm,   FALSE);                        n0++;
    XtSetArg(arg[n0], XmNy,           y);                            n0++;
#endif


    icnt = node->getInputCount();


    /*visible_inputs = getVisibleInputCount();*/
    /*visible_outputs = getVisibleOutputCount();*/


    for(i=1; i<=icnt; i++)
    {

        tab = new Tab(this);
        if (node->isInputViewable(i)) {
            tab->createTab(standInRoot,node->isInputVisible(i));
	    XtAddEventHandler (tab->getRootWidget(), KeyPressMask, False,
		(XtEventHandler)StandIn_TabKeyNavEH, (XtPointer)NULL);
	}
	tab->moveTabY(0, FALSE);
	tab->setBackground(standInDefaults.io.background);
	inputTabList.appendElement((void *) tab);

	Widget tabRoot = tab->getRootWidget();
	if(tabRoot)
	{
	    XtAddCallback(tabRoot,
			      XmNarmCallback,
			      (XtCallbackProc)StandIn_ArmInputCB,
			      (XtPointer)this->node);

	    XtAddCallback(tabRoot,
			      XmNdisarmCallback,
			      (XtCallbackProc)StandIn_DisarmTabCB,
			      (XtPointer)this->node);

#if OLD_LESSTIF == 1
	    char *str = "\
		<Btn1Down>: Arm() \n\
		<Btn1Up>: Disarm()";
	    XtVaSetValues(tabRoot, XmNtranslations,  XtParseTranslationTable(str), NULL);
#endif
	}
    }

    /***
     *** Create output param buttons.
     ***/

    y = height + standInDefaults.io.height;

    ocnt = node->getOutputCount();


    for (i=1; i<=ocnt; i++) {
        tab = new Tab(this);
        if (node->isOutputViewable(i)) {
            tab->createTab(standInRoot,node->isOutputVisible(i));
	    XtAddEventHandler (tab->getRootWidget(), KeyPressMask, False,
		(XtEventHandler)StandIn_TabKeyNavEH, (XtPointer)NULL);
	}

	tab->moveTabY(y, TRUE);
	tab->setBackground(standInDefaults.io.background);
	outputTabList.appendElement((void *) tab);

	Widget tabRoot = tab->getRootWidget();
	if(tabRoot)
	{
	    XtAddCallback(tabRoot,
			      XmNarmCallback,
			      (XtCallbackProc)StandIn_ArmOutputCB,
			      (XtPointer)this->node);

	    XtAddCallback(tabRoot,
			      XmNdisarmCallback,
			      (XtCallbackProc)StandIn_DisarmTabCB,
			      (XtPointer)this->node);

#if OLD_LESSTIF == 1
	    char *str = "\
		<Btn1Down>: Arm() \n\
		<Btn1Up>: Disarm()";
	    XtVaSetValues(tabRoot, XmNtranslations,  XtParseTranslationTable(str), NULL);
#endif
	}
    }
    this->setMinimumWidth(width);
    this->adjustParameterLocations(width);
    XmStringFree(string);

    /***
     *** Center and display the widgets.
     ***/

    /*
     * Get and save widget size and location.
     */
    height += 2 * (standInDefaults.io.height);
    this->setXYSize(total_width, height);

    /*
     * Display it.
     */
    this->manage();

    mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | 
	KeyPressMask | KeyReleaseMask;
    StandIn::ModifyButtonWindowAttributes (button, mask);
#if XmVersion >= 1001
// FIXME
//    XtAddEventHandler(this->getRootWidget(), ButtonPressMask,
//                      FALSE, _uinHelpEventHandler, (XtPointer)NULL);
#endif

}

//
// Was part of ::createStandIn().  ...is now broken out so that it
// can be used on the io tabs as well as the main part of the standin.
// We want to pass thru kbd events on io tabs so that keyboard events
// will work in the vpe.
//
void StandIn::ModifyButtonWindowAttributes (Widget button, int mask)
{
    /*
     * Make sure button and motion events get passed through.
     */
    Window button_window = XtWindow(button);
    if (!button_window) {
	printf ("%s[%d] no window to modify\n", __FILE__,__LINE__);
	return ;
    }
    Display *dsp = XtDisplay(button);
    XWindowAttributes    getatt;

    //
    // Arrange to call XGetWindowAttributes only once.  It's too 
    // expensive an unnecessary to do every time we create a StandIn.
    //
    static boolean getatt_initialized = FALSE;
    static int your_event_mask = 0;
    static int do_not_propagate_mask = 0;

    if (!getatt_initialized) {
	XGetWindowAttributes(dsp, button_window, &getatt);
	//getatt_initialized = TRUE;
	your_event_mask = getatt.your_event_mask;
	do_not_propagate_mask = getatt.do_not_propagate_mask;
    }

    XSetWindowAttributes setatt;
    setatt.event_mask            = your_event_mask & ~mask;
    setatt.do_not_propagate_mask = do_not_propagate_mask & ~mask;

    XChangeWindowAttributes(dsp,
                            button_window,
                            CWEventMask | CWDontPropagate,
                            &setatt);

}

void StandIn::drawArks(Node* )
{
}

void StandIn::addArk(EditorWindow* editor, Ark *a)
{
    EditorWorkSpace*      workspace;
    StandIn*        src_standIn;
    StandIn*        dst_standIn;
    ArkStandIn*     asi;
    Node*           src_node;
    Node*           dst_node;
    int             paramInd, arcIndex;
    XmWorkspaceLine l;

    src_node = a->getSourceNode(paramInd);
    dst_node = a->getDestinationNode(arcIndex);
    src_standIn = src_node->getStandIn();
    dst_standIn = dst_node->getStandIn();

    workspace = (EditorWorkSpace*)this->workSpace;

    //
    // Create the StandIn if neccessary.  This should probably be done in
    // getStandIn().
    //
    if(!src_standIn)
    {
	src_node->newStandIn(workspace);
        src_standIn = src_node->getStandIn();
    }
    if(!dst_standIn)
    {
	dst_node->newStandIn(workspace);
        dst_standIn = dst_node->getStandIn();
    }

    if (src_node->isParameterVisible(paramInd, FALSE) &&
	src_node->isParameterViewable(paramInd,FALSE) &&
	dst_node->isParameterVisible(arcIndex, TRUE)  &&
	dst_node->isParameterViewable(arcIndex,TRUE))
    {
	
	l = XmCreateWorkspaceLine((XmWorkspaceWidget)workspace->getRootWidget(),
		standInDefaults.line.color,
		src_standIn->getRootWidget(),
		src_standIn->getOutputParameterTabX(paramInd)
			     + standInDefaults.io.width /2,
		src_standIn->getOutputParameterTabY(paramInd)
			     + standInDefaults.io.height,
		dst_standIn->getRootWidget(),
		dst_standIn->getInputParameterTabX(arcIndex)
			     + standInDefaults.io.width / 2,
		dst_standIn->getInputParameterTabY(arcIndex));

	asi = new ArkStandIn((XmWorkspaceWidget) workspace->getRootWidget(), l);
	a->setArkStandIn(asi);
	src_standIn->drawTab(paramInd, TRUE);
	dst_standIn->drawTab(arcIndex, FALSE);
    }
}



//
// Caution:  We're going to refuse to set our own selected state, if we see that
// the Workspace we inhabit is unmanaged.  This is on behalf of WorkSpage pages.
// The motivation is primarily the EditorWindow menu bar option - 
// Edit/Select Unconnected.  This option produces a traversal of the network selecting
// potentially many standins.  The standins might occupy pages which aren't at the 
// foreground.   This will need to be revisited if pages can be managed inside a 
// separated unmanaged window.  The point is to prevent operating of selected nodes
// you don't know are selected.
//
void StandIn::setSelected(boolean s)
{
    WorkSpace* ws = this->getWorkSpace();
    if (ws->isManaged() == FALSE) return ;

    boolean old_s = this->selected;

    this->selected = s;

    if (old_s != s)  {
        this->indicateSelection(s);
	this->handleSelectionChange(s);
    }

}
void StandIn::indicateSelection(boolean select)
{
    Arg     arg[1];

    if (!this->getRootWidget())
	return;

    XtSetArg(arg[0], XmNselected, select);
    XtSetValues(this->getRootWidget(), arg, 1);
}


void StandIn::ioStatusChange(int index, boolean outputTab,
				NodeParameterStatusChange status) 
{
    if (status == Node::ParameterSetValueChanged)
	return; 

    setVisibility(index, outputTab);
    drawTab(index,outputTab);
}

boolean StandIn::setMinimumWidth(int &width)
{
    int     min_width, visible_inputs, visible_outputs;
    int     curr_width, curr_height;
    Arg     arg[3];

    
   /*
    * Calculate the minimum width needed for i/o labels.
    */

    visible_inputs = this->getVisibleInputCount();
    visible_outputs = this->getVisibleOutputCount();
    min_width = MAX(visible_inputs, visible_outputs);

    min_width = (standInDefaults.io.width +
                 standInDefaults.io.width / 2 ) * min_width;

    //
    // Don't let the button width get smaller than what is 
    // needed for the text.
    //

    width = MAX(min_width, this->requiredButtonWidth);

    this->getXYSize(&curr_width, &curr_height);

    if (curr_width  == width) 
        return(FALSE);  // no change necessary
    
    this->setXYSize(width, curr_height);

    XtSetArg(arg[0], XmNwidth, width);
    XtSetValues(this->buttonWidget, arg, 1);

    return(TRUE);
}

//
// Add a tab at the given index.
// This generally called by Node::addRepeats(), so we don't need to 
// notify the node or set the network dirty (node does that).
//

void StandIn::addInput(int index) 
{
    int      width;
    Tab      *tab;


    ASSERT(index > 0);

    this->setMinimumWidth(width);
    tab = createInputTab(this->getRootWidget(), index, width);
    if ((this->node->isInputVisible(index) == FALSE) ||
        (this->node->isInputViewable(index) == FALSE))
	tab->unmanage();


    inputTabList.insertElement(tab, index);

    this->adjustParameterLocations(width);
}

void StandIn::removeLastInput() 
{
    Tab *tab;
    int width;

    int index = this->inputTabList.getSize();
    if (index == 0)
	return;
    tab = (Tab  *) this->inputTabList.getElement(index);
    this->inputTabList.deleteElement(index);
    delete tab;

    this->setMinimumWidth(width);
    this->adjustParameterLocations(width);
}

void StandIn::setVisibility(int index, boolean outputTab)
{
    int width;
    Tab  *tab;
     
    if (outputTab) 
        tab = (Tab  *) outputTabList.getElement(index);
    else 
        tab = (Tab  *) inputTabList.getElement(index);

    if (tab->getRootWidget() == NULL)
        return;    // this parameter has no standIn representation

    //
    // If our visibility is currently correct...
    //
    if (node->isParameterVisible(index,!outputTab) == tab->isManaged()) return;

    if (node->isParameterVisible(index,!outputTab) == FALSE) {
        if (tab->isManaged())
            tab->unmanage();
    } else {
        tab->manage();
    }

    this->setMinimumWidth(width);
    this->adjustParameterLocations(width);
}

void StandIn::addOutput(int index)
{
    ASSERT(index > 0);

    int      width;
    Tab      *tab;

    this->setMinimumWidth(width);
    tab = this->createOutputTab(this->getRootWidget(), index, width);

    this->outputTabList.insertElement(tab, index);

    this->adjustParameterLocations(width);
}

void StandIn::removeLastOutput() 
{
    Tab *tab;
    int width;


    int index = this->outputTabList.getSize();
    if (index == 0)
	return;

    tab = (Tab  *) this->outputTabList.getElement(index);
    this->outputTabList.deleteElement(index);
    delete tab;

    this->setMinimumWidth(width);
    this->adjustParameterLocations(width);
}

void StandIn::setLabelColor(Pixel color)
{
    Arg     arg[1];
    XtSetArg(arg[0], XmNforeground, color);
    XtSetValues(this->buttonWidget,arg,1);
}
Pixel StandIn::getLabelColor()
{
    Pixel color;
#if 0
    XtVaGetValues(this->buttonWidget,
	 XmNforeground, &color, NULL);
#else
    Arg     arg[1];
    XtSetArg(arg[0], XmNforeground, &color);
    XtGetValues(this->buttonWidget,arg,1);
#endif

    return color;
}

//
// Print a representation of the stand in on a PostScript device.  We
// assume that the device is in the same coordinate space as the parent
// of this uicomponent (i.e. we send the current geometry to the 
// PostScript output file).  We do not print the ArkStandIns, but do
// print the Tabs.
//
boolean StandIn::PrintPostScriptSetup(FILE *f)
{

    if (fprintf(f,"/Box { %% Usage : r g b x y width height\n"
			"\t2 dict begin\n"
			"\t/height 2 1 roll def\n"
			"\t/width  2 1 roll def\n"
			"\t%% First, draw a filled box\n"
			"\tmoveto		%% move to lower left\n"
			"\t0 height rlineto 	%% move to upper left\n"
			"\twidth 0  rlineto   	%% Move to upper right\n"
			"\t0 height neg rlineto %% move to lower right\n"
			"\tclosepath\n"
			"\tgsave\n"
			"\tsetrgbcolor fill 	%% set the given gray & fill\n"
			"\tgrestore		%% restore original state\n"
			"\t%% Second, draw an outline for the box\n"
			"\t1 setlinewidth stroke\n"
			"\tend			%% end of dictionary\n"
			"} bind def \n") <= 0)
	return FALSE;

    return TRUE;
}

//
// Get the position of a widget relative to one of its ancestor widgets.
//
static void GetPositionRelativeTo(Widget relative_to, Widget of, int *x, int *y)
{
    Widget parent = XtParent(of);

    if (parent == relative_to)  {
	*x = *y = 0;
    } else {
	int parent_x, parent_y;
	Position of_x, of_y;

	GetPositionRelativeTo(relative_to, parent, &parent_x, &parent_y);

	XtVaGetValues(of,
	    XmNx, &of_x,
	    XmNy, &of_y,
	    NULL);

	*x = of_x + parent_x;
	*y = of_y + parent_y;
    }

}
const char *StandIn::getPostScriptLabelFont()
{
    return "Helvetica-Bold";
}
void PixelToRGB(Widget w, Pixel p, float *r, float *g, float *b)
{
    Colormap cmap = 0;
    XColor xcolor;
    Display *d = XtDisplay(w);
    xcolor.pixel = p;
    XtVaGetValues (w, XmNcolormap, &cmap, NULL); //XmNcolormap comes from Core
    ASSERT(cmap);
    XQueryColor(d,cmap,&xcolor);

    *r = xcolor.red   / 65535.0;
    *g = xcolor.green / 65535.0;
    *b = xcolor.blue  / 65535.0;
}
boolean StandIn::printPostScriptPage(FILE *f, boolean label_parameters)
{
    int tab_xpos, tab_ypos, button_xpos, button_ypos, xpos, ypos, xsize, ysize;
    int i, standin_xpos, standin_ypos, standin_ysize, standin_xsize;;
    ListIterator iterator;
    Tab *t;
    // Network *network = this->node->getNetwork();
    // EditorWindow *editor = network->getEditor();
    Widget workspace = this->workSpace->getRootWidget();
    float red,green,blue;
    Pixel pixel;

    //
    // Print the root widget box 
    //

    Position x,y;
    Dimension w,h;
    
    this->getGeometry(&xpos, &ypos, &xsize, &ysize);
    standin_ysize = ysize;
    standin_xsize = xsize;
    standin_xpos = xpos;
    standin_ypos = ypos;

    XtVaGetValues(this->buttonWidget,
        XmNx, &x,
        XmNy, &y,
        XmNwidth, &w,
        XmNheight, &h,
	NULL);
    button_xpos = xpos + x; button_ypos = ypos + y; xsize = w; ysize = h;
    int font_scale_height = ysize;

    XtVaGetValues(this->buttonWidget, XmNbackground, &pixel, NULL);
    PixelToRGB(this->buttonWidget,pixel, &red, &green, &blue);
    if (fprintf(f,"%f %f %f %d %d %d %d Box\n", 
			red,green,blue,
			button_xpos,button_ypos, xsize, ysize) < 0)
	return FALSE;
    float font_scale = ysize/3;
    const char *label = this->getButtonLabel();
    char *esc_label;
    if (strchr(label,'(') || strchr(label,')')) {
	//
	// If the label has left/right parens, then we must escape them.
	//
	esc_label = new char[2 * STRLEN(label) + 1];
	const char *src = label;
	char c, *dest = esc_label;
	for (src = label, dest = esc_label ; (c = *src) ; src++, dest++) {
	    if ((c == ')' || c == '(')) {
		*dest = '\\';
		dest++;
	    }
	    *dest = c;
	}
	*dest = '\0';
    } else {
	esc_label = (char*)label;
    }
  
    if (fprintf(f,"/%s findfont\n"
		  "[ %f 0 0 -%f 0 0 ] makefont setfont\n"
		  "(%s) stringwidth\n"
		  "pop 3 %d mul 5 div %d add %% y position\n"
		  "2 1 roll\n"
		  "neg %d add 2 div %d add %% x position\n"
		  "2 1 roll\n"
		  "moveto (%s) show\n",
		   this->getPostScriptLabelFont(),
		   font_scale, font_scale,
		   esc_label,
		   ysize, button_ypos,
		   xsize, button_xpos,
		   esc_label) <=0)
	return FALSE;
		

    if (esc_label != label)
	delete esc_label;

    boolean param_font_set = FALSE;
    font_scale = ysize/7;
    //
    // Print the input tabs 
    //
    iterator.setList(this->inputTabList);
    i = 0;
    while ( (t = (Tab*)iterator.getNext()) ) {
	i++;
	if (t->isManaged()) {
	    Node *tnode = this->node;
	    t->getXYSize(&xsize, &ysize);
	    GetPositionRelativeTo(workspace,t->getRootWidget(),
							&tab_xpos,&tab_ypos);
	    
	    XtVaGetValues(t->getRootWidget(), XmNbackground, &pixel, NULL);
	    PixelToRGB(t->getRootWidget(),pixel, &red, &green, &blue);
	    if (fprintf(f,"%f %f %f %d %d %d %d Box\n", 
			red,green,blue,
			xpos + tab_xpos,ypos + tab_ypos, xsize, ysize) < 0)
		return FALSE;

	    if (tnode->isInputConnected(i)) {
		int x =  (int) (xpos + tab_xpos + xsize/2.0);
		int y =  ypos + tab_ypos - ysize;
		if (fprintf(f,"%d %d moveto %d %d lineto stroke\n", 
			    x, y, x, y+ysize) < 0)
		    return FALSE;
		
	    } else if (label_parameters && !tnode->isInputDefaulting(i)) {
		int x =  (int) (xpos + tab_xpos + .50*xsize);
		int y =  (int) (ypos + tab_ypos - .20*ysize);
		// Input is set 
		if (!param_font_set) {
		    param_font_set = TRUE;
		    if (fprintf(f,"/Helvetica findfont\n"
		      "[ %f 0 0 -%f 0 0 ] makefont setfont\n",
		       font_scale, font_scale) <= 0)
			return FALSE;
		} 
		char buf[1024];
		if (tnode->isInputDefaulting(i)) {
		    strcpy(buf, tnode->getInputNameString(i));
		} else {
		    int ii, jj;
		    int cutoff;
		    char* doublequote;
		    char dup_val[64];
		    char escaped_val[128];
		    const char *val = tnode->getInputValueString(i); 
		    //trim the string if too long, don't count escape for escaped chars
		    if (STRLEN(val) > 32) {
			if(STRLEN(val) > 58 ) {
				strncpy(dup_val,val,58);
				dup_val[57] = '\0';
			} else {
				strcpy(dup_val,val);
			}
			cutoff=0;
			for (ii=0; ii<STRLEN(dup_val); ii++) {
				if(dup_val[ii]!='\\') {
					++cutoff;
					// don't have escape character be last!
					if((cutoff>31)&&(ii>0)&&(dup_val[ii-1]!='\\')) dup_val[ii]='\0';
				}
			}
			strcat(dup_val,"...");
			// restore balanced quotes for postscript
			// ---turns out it isn't a requirement, but it is aesthetic
			ii=0;
			doublequote=dup_val;
			while((doublequote=strchr(doublequote,'\"'))!=NULL) {
				++doublequote;
				++ii;
			}
			if(ii&1) strcat(dup_val,"\"");
		    } else {
			strcpy(dup_val,val);
		    }	
		    jj=0;
		    for (ii=0; ii<=STRLEN(dup_val); ++ii) {
			//some characters require escaping, feel free to expand the list
		 	if(ii<STRLEN(dup_val) && strchr("()",dup_val[ii])) escaped_val[jj++]='\\';
			escaped_val[jj++]=dup_val[ii];
			}
		    sprintf(buf,"%s = %s", 
				tnode->getInputNameString(i),escaped_val);
	 	}
		
		if (fprintf(f,"gsave %d %d translate -45 rotate "
		      	"0 0 moveto\n"
			"(%s) show grestore\n",
			x, y, buf) <=0)
			return FALSE;
		
	    }
	}
    }
    //
    // Print the output tabs 
    //
    iterator.setList(this->outputTabList) ;
    i = 0;
    while ( (t = (Tab*)iterator.getNext()) ) {
	i++;
	if (t->isManaged()) {
	    t->getXYSize(&xsize, &ysize);
	    GetPositionRelativeTo(workspace,t->getRootWidget(),
							&tab_xpos,&tab_ypos);
	    XtVaGetValues(t->getRootWidget(), XmNbackground, &pixel, NULL);
	    PixelToRGB(t->getRootWidget(),pixel, &red, &green, &blue);
	    if (fprintf(f,"%f %f %f %d %d %d %d Box\n", 
			red,green,blue,
			xpos + tab_xpos,ypos + tab_ypos, xsize, ysize) < 0)
		return FALSE;
	    if (this->node->isOutputConnected(i)) {
		int x =  (int) (xpos + tab_xpos + xsize/2.0);
		int y =  ypos + tab_ypos + ysize;
		if (fprintf(f,"%d %d moveto %d %d lineto stroke\n", 
			    x, y, x, y+ysize) < 0)
		    return FALSE;
		
	    }
	}
    }

    //
    // We'll try to print 1 extra piece of text if our node supplies one.
    // This is really on behalf of ComputeNode since he has nowhere to show
    // the compute expr since the expr input tab is not viewable.
    // Inside a standin there might be some extra room.  There is the area
    // below a standin which is blank if all output tabs are connected.  Then
    // there is the area below the label which is blank if all output tabs
    // are unconnected.  To figure out where to display extra text, loop over
    // outputs to find if either situation prevails.  If so you're in luck.
    //
    // Oridinarily we would handle a task like this inside a subclass of
    // StandIn and make new subclass of StandIn on behalf of ComputeNode.
    // I'm not doing that in this case because printPostScriptPage isn't 
    // virtual.  Having PostScript generation code be virtual might not be
    // so cool because it's complicated and no one knows how to work on it,
    // unlike the code that writes other types of files.
    //
    const char* extra_text = NUL(char*);
    if (label_parameters) 
	extra_text = this->node->getExtraPSText();
    if (extra_text) {

	boolean below_box = TRUE;
	boolean inside_box = TRUE;
	int i = 1;
	iterator.setList(this->outputTabList) ;
	Tab* t;
	while ( (t = (Tab*)iterator.getNext()) ) {
	    if (!t->isManaged()) continue;
	    boolean iscon = this->node->isOutputConnected(i);
	    if (iscon) inside_box = FALSE;
	    else below_box = FALSE;
	    i++;
	}

	if (this->outputTabList.getSize() == 1) {
	    if (this->node->isOutputConnected(1))
		below_box = TRUE;
	    else
		below_box = FALSE;
	    inside_box = !below_box;
	}

	if ((inside_box) || (below_box)) {
	    //
	    // If the label has left/right parens, then we must escape them.
	    //
	    if (strchr(extra_text,'(') || strchr(extra_text,')')) {
		esc_label = new char[2 * STRLEN(extra_text) + 6];
		const char *src = extra_text;
		char c, *dest = esc_label;
		for (src = extra_text, dest = esc_label ; (c = *src) ; src++, dest++) {
		    if ((c == ')' || c == '(')) {
			*dest = '\\';
			dest++;
		    }
		    *dest = c;
		}
		*dest = '\0';
	    } else {
		esc_label = new char[STRLEN(extra_text) + 6];
		strcpy (esc_label, extra_text);
	    }

	    //
	    // for every tab more than 3 add some bytes to max len.
	    //
	    int max_len = (below_box?26:20);
	    int mgd_i_tab_cnt = 0;
	    int mgd_o_tab_cnt = 0;
	    iterator.setList(this->inputTabList) ;
	    while ( (t = (Tab*)iterator.getNext()) ) if (t->isManaged()) mgd_i_tab_cnt++;
	    iterator.setList(this->outputTabList) ;
	    while ( (t = (Tab*)iterator.getNext()) ) if (t->isManaged()) mgd_o_tab_cnt++;

	    int tab_cnt = MAX(mgd_i_tab_cnt, mgd_o_tab_cnt);
	    int diff = tab_cnt - 2;
	    max_len+= (diff>0?diff*8:0);
	    if (strlen(esc_label) >= max_len) 
		strcpy (&esc_label[max_len], "...");

	    int box_relative (below_box?-2:-14);
	    
	    font_scale = font_scale_height/7;
	    if (!param_font_set) {
		param_font_set = TRUE;
		if (fprintf(f,"/Helvetica findfont\n"
		  "[ %f 0 0 -%f 0 0 ] makefont setfont\n",
		   font_scale, font_scale) <= 0)
		    return FALSE;
	    } 
	    if (fprintf(f,"(%s) stringwidth pop\n"
			  "0.5 neg mul 0.5 %d mul add %d add  %% x position\n"
			  "%d\n"
			  "moveto (%s) show\n",
			   esc_label,
			   standin_xsize, standin_xpos,
			   standin_ysize + standin_ypos + box_relative,
			   esc_label) <=0)
		return FALSE;
		    
	    delete esc_label;
	}
    }

    return TRUE;
}

void StandIn::dropFinish(long operation, int tag, unsigned char status)
{
    Network*         network;
    EditorWindow*    editor;

    network = this->node->getNetwork();
    editor = network->getEditor();

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
    // then it would be better to treat it like a move
    // so that the user isn't required to use the Shift key.
    // When dragging from vpe to c.p. disallow cutting with the shift key.
    if (status) {
	if (tag == StandIn::Interactors) {
	} else if ((operation == XmDROP_MOVE) || (tag == StandIn::Trash)) {
	    //editor->getDeleteNodeCmd()->execute();
	    XtAppContext apcxt = theApplication->getApplicationContext();
	    this->drag_drop_wpid = XtAppAddWorkProc (apcxt,
		    StandIn_DragDropWP, (XtPointer)this);
	}
    }
}


boolean StandIn::decodeDragType (int tag,
	char * a, XtPointer *value, unsigned long *length, long operation)
{
boolean retVal;
char *hostname;
int len;

    switch (tag) {
	case StandIn::Modules:
	    // this-convert() comes from DXDragSource and can't be overridden.
	    retVal = this->convert (this->node->getNetwork(), a,value, length, operation);
	    break;


	// The only data needed is hostname:pid.  That string is used to verify that
	// this operation is taking place in intra-executable fashion and not between
	// executables.  This is necessary because placing an interactor in a panel
	// depends on what is in the net.
	case StandIn::Interactors:
	    hostname = new char[MAXHOSTNAMELEN + 16];
	    if (gethostname (hostname, MAXHOSTNAMELEN) == -1) {
		retVal = FALSE;
		break;
	    }
	    len = strlen(hostname);
	    sprintf (&hostname[len], ":%d", getpid());

	    *value = hostname;
	    *length = strlen(hostname);
	    retVal = TRUE;
	    break;

	// this->dropFinish will take care of the delete.  No data is required.
	case StandIn::Trash:
	    retVal = TRUE;

	    // dummy pointer. This memory shouldn't be referenced anywhere.
	    *value = (XtPointer)malloc(4);
	    *length = 4;
	    break;

	default:
	    retVal = FALSE;
	    break;
    }
    return retVal;
}


// Is it ok to start a drag?  This depends only on the selected state of
// the node.  
int StandIn::decideToDrag(XEvent *xev) 
{ 
    if (!this->selected) return DragSource::Abort;

    // Node *node = this->node; 
    // Network *network = node->getNetwork();
    ListIterator it;

    Display *d = XtDisplay(this->getRootWidget());
    Atom xoff_atom = XInternAtom (d, DXXOFFSET, False);
    Atom yoff_atom = XInternAtom (d, DXYOFFSET, False);
    Screen *screen = XtScreen(this->getRootWidget());
    Window root = RootWindowOfScreen(screen);
    int x;
    int y;
    this->getXYPosition (&x, &y);
    int minx = x;
    int miny = y;
    int junk;

    this->workSpace->getSelectedBoundingBox (&minx, &miny, &junk, &junk);
#if defined(alphax)
    // unaligned access error otherwise
    long int xoffset = x - minx;
    long int yoffset = y - miny;
#else
    int xoffset = x - minx;
    int yoffset = y - miny;
#endif

    if (xev->type == ButtonPress) {
	xoffset+= xev->xbutton.x;
	yoffset+= xev->xbutton.y;
    }

    XChangeProperty (d, root, xoff_atom, XA_INTEGER, 32, PropModeReplace, 
	(unsigned char*)&xoffset, 1);
    XChangeProperty (d, root, yoff_atom, XA_INTEGER, 32, PropModeReplace, 
	(unsigned char*)&yoffset, 1);

    return DragSource::Proceed;
}

extern "C"  {
//
// Provide Button2 selectability on standins so that an attempt to drag something
// unselected might select the thing instead of beeping.
//
void StandIn_Button2ReleaseEH
(Widget w, XtPointer , XEvent *xev, Boolean *keep_going)
{
    *keep_going = True;
    if (xev->xbutton.button != 2)
        return ;
 
    if (xev->xbutton.state & ShiftMask)
        return ;
    XtCallActionProc (w, "release_w", xev, NULL, 0);
}
void StandIn_Button2PressEH
(Widget w, XtPointer , XEvent *xev, Boolean *keep_going)
{
    *keep_going = True;
    if (xev->xbutton.button != 2)
        return ;
 
    if (xev->xbutton.state & ShiftMask)
        return ;
    XtCallActionProc (w, "select_w", xev, NULL, 0);
}

//
// Pass keyboard events thru to the workspace.  This ought to be handled
// either with translations or with the same mechanism used for standInRoot.
//
void StandIn_TabKeyNavEH
(Widget w, XtPointer , XEvent *xev, Boolean *keep_going)
{
    *keep_going = TRUE;
    boolean is_help_key = FALSE;
    XKeyEvent* xke = (XKeyEvent*)xev;
    KeySym lookedup = XLookupKeysym (xke, 0);

    // 
    // problem here is that we don't know for sure that F1 is help.
    // What we really want is to compare to osfHelp, not XK_F1.
    //
    if (lookedup == XK_F1) is_help_key = TRUE;

    // the meaning of False here, is that we have no need for <Key> events
    // in the io tabs.  They have always been hooked up and working but
    // for no purpose.  You could hit the space bar on a tab and watch
    // it wiggle, but so what.
    
    if (!is_help_key) {
	*keep_going = False;
	XtCallActionProc (w, "child_nav", xev, NULL, 0);
    }
}

//
// Used during drag-n-drop operations that result in deletion of nodes.
// The Command passed in is the EditorWindow's delete-selected-nodes
// command.
//
Boolean StandIn_DragDropWP (XtPointer clientData)
{
    StandIn* si = (StandIn*)clientData;
    Network*         network;
    EditorWindow*    editor;

    network = si->node->getNetwork();
    editor = network->getEditor();
    si->drag_drop_wpid = 0;
    editor->getDeleteNodeCmd()->execute();
    return TRUE;
}
} // extern C

//
// This function can be called to notify a standin that its label
// has changed.  By default, this standin just calls setButtonLabel()
// to reset the label.
void StandIn::notifyLabelChange()
{
    this->setButtonLabel();
}

//
// In addition to superclass work, we'll need to create new workspace
// lines since the start/end points of
// In the case of StandIn, using the superclass doesn't do any good
// since the widget is geometry-managed by the containing widget.  Setting
// XmN{x,y} has no effect.
//
void StandIn::setXYPosition (int x, int y)
{
    this->UIComponent::setXYPosition (x,y);
    if (this->isManaged())
	XmWorkspaceSetLocation (this->getRootWidget(), x, y);
}

// 
// Store the this pointer in the widget's XmNuserData so that we
// can retrieve the Object in a callback in EditorWorkSpace.C
// 
void StandIn::setRootWidget(Widget root, boolean standardDestroy)
{
    this->UIComponent::setRootWidget(root, standardDestroy);
    this->setLocalData(this);
}

