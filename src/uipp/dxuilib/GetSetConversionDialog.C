/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <X11/StringDefs.h>

#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/Frame.h>

#include "DXStrings.h"
#include "GetSetConversionDialog.h"
#include "DXApplication.h"
#include "MacroDefinition.h"
#include "Network.h"
#include "List.h"
#include "ListIterator.h"
#include "EditorWindow.h"
#include "ButtonInterface.h"
#include "Command.h"
#include "GlobalLocalNode.h"
#include "Node.h"
#include "Ark.h"

#define HELP            1
#define FIND_NEXT       2
#define FIND_FIRST      3
#define FIND_ALL        4
#define DONE            5

boolean GetSetConversionDialog::ClassInitialized = FALSE;

Cursor GetSetConversionDialog::WatchCursor = 0;

String  GetSetConversionDialog::DefaultResources[] = {
    "*dialogTitle:		  Convert Get/Set Modules",

    "*buttonForm.?.width:	  80",
    "*buttonForm.?.recomputeSize: False",

    "*globalOption.labelString:	  Global",
    "*globalOption.width:	  80",
    "*globalOption.recomputeSize: False",

    "*localOption.labelString:	  Local",
    "*localOption.width:	  80",
    "*localOption.recomputeSize:  False",

    "*gsHeader.labelString:	  Files containing Get/Set modules:",
    "*gsHeader.foreground:	  SteelBlue",
    NULL
};

//
// C O N S T R U C T O R    C O N S T R U C T O R    C O N S T R U C T O R    
// C O N S T R U C T O R    C O N S T R U C T O R    C O N S T R U C T O R    
//
GetSetConversionDialog::GetSetConversionDialog() :
    Dialog("getSetConversionDialog", theDXApplication->getRootWidget())
{
    if (NOT GetSetConversionDialog::ClassInitialized)
    {
        GetSetConversionDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

    this->referenced_macros = NULL;
    this->list = NULL;

    this->global_option = NULL;
    this->local_option = NULL;

    this->quiet_mode = FALSE;
}


//
// D E S T R U C T O R    D E S T R U C T O R    D E S T R U C T O R    
// D E S T R U C T O R    D E S T R U C T O R    D E S T R U C T O R    
//
GetSetConversionDialog::~GetSetConversionDialog()
{
    if (this->referenced_macros) 
	delete this->referenced_macros;

    if (this->global_option) delete this->global_option;
    if (this->local_option) delete this->local_option;
}


//
// L A Y O U T   T H E   D I A L O G       L A Y O U T   T H E   D I A L O G
// L A Y O U T   T H E   D I A L O G       L A Y O U T   T H E   D I A L O G
//
Widget GetSetConversionDialog::createDialog(Widget parent)
{
    int 	n;
    Arg		args[25];

    n = 0;
    XtSetArg(args[n], XmNautoUnmanage,  False); n++;
    Widget toplevelform = this->CreateMainForm(parent, this->name, args, n);

    Widget buttonform = XtVaCreateManagedWidget("buttonForm",
	xmFormWidgetClass, 	toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
	XmNbottomOffset,	8,
	XmNwidth,		350,
    NULL);
    this->layoutButtons (buttonform);

    Widget controlsform = XtVaCreateManagedWidget("controlsForm",
	xmFormWidgetClass, 	toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	buttonform,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
	XmNbottomOffset,	6,
    NULL);
    this->layoutControls(controlsform);

    Widget mainform = XtVaCreateManagedWidget("mainForm",
	xmFormWidgetClass, 	toplevelform,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment, 	XmATTACH_WIDGET, 
	XmNbottomWidget, 	controlsform,
	XmNtopOffset,		2,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
	XmNbottomOffset,	4,
    NULL);
    this->layoutChooser(mainform);

    XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	buttonform,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomOffset,	2,
    NULL);

    if (!GetSetConversionDialog::WatchCursor)
	GetSetConversionDialog::WatchCursor = 
	    XCreateFontCursor (XtDisplay(parent), XC_watch);

    return toplevelform; 
}


//
// L A Y O U T   T H E    L I S T        L A Y O U T   T H E    L I S T 
// L A Y O U T   T H E    L I S T        L A Y O U T   T H E    L I S T 
//
void GetSetConversionDialog::layoutControls (Widget parent)
{
    //
    // A label showing the name of the active net and a count
    // of its unconverted Set/Get modules.
    //
    Widget frame = XtVaCreateManagedWidget ("net_name_frame",
	xmFrameWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopOffset,		2,
	XmNleftOffset,		80,
	XmNrightOffset,		4,
	XmNshadowThickness,	2,
	XmNmarginHeight,	3,
	XmNmarginWidth,		3,
    NULL);

    this->net_name = XtVaCreateManagedWidget("net_name",
	xmLabelWidgetClass,	frame,
	XmNalignment,		XmALIGNMENT_BEGINNING,
    NULL);

    XtVaCreateManagedWidget ("Selection:",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		frame,
	XmNtopWidget,		frame,
	XmNbottomWidget,	frame,
	XmNrightOffset,		4,
	XmNtopOffset,		0,
	XmNbottomOffset,	0,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass, parent,
	XmNtopWidget,		frame,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		6,
    NULL);


    this->net_name_label = 
	XtVaCreateManagedWidget ("network contains:\n   0 Get(s)\n    0 Set(s)",
	xmLabelWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		4,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftOffset,		10,
	XmNbottomOffset,	4,
	XmNalignment,		XmALIGNMENT_BEGINNING,
    NULL);

    //
    // Convert to {Global,Local} button
    //
    Command *cmd = EditorWindow::GetGlobalCmd();
    ASSERT(cmd);
    this->global_option = 
	new ButtonInterface (parent, "globalOption", cmd);
    XtVaSetValues (this->global_option->getRootWidget(),
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		28,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		10,
    NULL);
	

    cmd = EditorWindow::GetLocalCmd();
    ASSERT(cmd);
    this->local_option = 
	new ButtonInterface (parent, "localOption", cmd);
    XtVaSetValues (this->local_option->getRootWidget(),
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		28,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->global_option->getRootWidget(),
	XmNrightOffset,		10,
    NULL);

    XtVaCreateManagedWidget("Convert to:",
	xmLabelWidgetClass,	parent,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	this->local_option->getRootWidget(),
	XmNbottomOffset,	2,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->local_option->getRootWidget(),
	XmNleftOffset,		0,
    NULL);
}
	

void
GetSetConversionDialog::layoutChooser(Widget parent)
{
Arg args[10];

    //
    // A list containing filenames of referenced macros.
    //
    XtVaCreateManagedWidget ("gsHeader",
	xmLabelWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		22,
	XmNtopOffset,		6,
    NULL);
    Widget sw_frame = XtVaCreateManagedWidget ("sw_frame",
	xmFrameWidgetClass,	parent,
    	XmNtopAttachment, XmATTACH_FORM,
    	XmNleftAttachment, XmATTACH_FORM,
    	XmNrightAttachment, XmATTACH_FORM,
    	XmNbottomAttachment, XmATTACH_FORM,
    	XmNtopOffset, 21,
    	XmNleftOffset, 4,
    	XmNrightOffset, 4,
    	XmNbottomOffset, 0,
	XmNmarginWidth,   3,
	XmNmarginHeight,   3,
	XmNshadowThickness, 2,
    NULL);


    int n = 0;
    XtSetArg (args[n], XmNvisibleItemCount, 10); n++;
    XtSetArg (args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
    this->list = XmCreateScrolledList (sw_frame, "chooser", args, n);
    XtManageChild (this->list);
    XtAddCallback (this->list, XmNsingleSelectionCallback,
	(XtCallbackProc)GetSetConversionDialog_SelectCB, (XtPointer)this);
    XtAddCallback (this->list, XmNdefaultActionCallback,
	(XtCallbackProc)GetSetConversionDialog_SelectCB, (XtPointer)this);
}

char *
GetSetConversionDialog::GetFileName(Network *net)
{
    if (!net) return DuplicateString("");

    const char *fn = net->getFileName();

    if ((!fn) || (!fn[0])) {
	if (net->isMacro()) return DuplicateString ("<unnamed macro>");
	else return DuplicateString ("<unnamed main program>");
    }
    return DuplicateString (fn);
}

void
GetSetConversionDialog::setActiveEditor(EditorWindow *ew)
{
    if (!this->list) return ;

    if (!ew) {
	XmListDeselectAllItems (this->list);
	this->unhinge();
	return ;
    }

    Network *net = ew->getNetwork();
    ASSERT(net);
    char *cp = GetSetConversionDialog::GetFileName(net);

    if ((!cp) || (!cp[0])) {
	if (cp) delete cp;
	return ;
    }

    XmString xmstr = XmStringCreateLtoR (cp, "bold");
    delete cp;
    if (!XmListItemExists(this->list, xmstr))
	this->update();
    XmListSelectItem (this->list, xmstr, True);
    XmStringFree(xmstr);
}

EditorWindow *
GetSetConversionDialog::getActiveEditor ()
{
    if (!this->getRootWidget())
	return NULL;

    int cnt, *posList;
    Boolean memory_allocated;
    EditorWindow *e = NULL;

    memory_allocated = XmListGetSelectedPos (list, &posList, &cnt);

    // 
    //  Fetch the text of the selected item and pop open the editor window.
    //
    int i;
    for (i=0; i<cnt; i++) {
	if ((posList[i] == 1) && (this->list_includes_main)) {
	    e = theDXApplication->network->getEditor();
	    continue;
	}
	int list_position; 
	if (this->list_includes_main) list_position = posList[i]-1;
	else list_position = posList[i];
	ASSERT (this->referenced_macros->getSize() >= list_position);
	MacroDefinition *md = (MacroDefinition*) 
	    this->referenced_macros->getElement(list_position);
	ASSERT(md);
	Network *net = (Network*)md->getNetwork();
	ASSERT(net);
	e = net->getEditor();
	if (!e) {
	    e = theDXApplication->newNetworkEditor(net);
	}
    }
    if (memory_allocated) XtFree ((char*)posList);

    return e;
}

//
// reset the transientFor resource to something reasonable for the application.
// This controls the dialog's behavior wrt which window I'll ride on top of.
//
void
GetSetConversionDialog::unhinge(boolean select_next_editor)
{
    Widget dshell = this->getRootWidget();
    while ((dshell) && (XtClass(dshell) != xmDialogShellWidgetClass))
	dshell = XtParent(dshell);
    if (!dshell) return ;

    EditorWindow *e = NULL;

    int cnt = this->referenced_macros->getSize() + (this->list_includes_main?1:0);
    boolean found_managed_editor = FALSE;

    int posList;
    for (posList=1; posList<=cnt; posList++) {
	Network *net;
	if ((posList == 1) && (this->list_includes_main)) {
	    net = theDXApplication->network;
	} else {
	    int list_position; 
	    if (this->list_includes_main) list_position = posList-1;
	    else list_position = posList;
	    ASSERT (this->referenced_macros->getSize() >= list_position);
	    MacroDefinition *md = (MacroDefinition*) 
		this->referenced_macros->getElement(list_position);
	    ASSERT(md);
	    net = (Network*)md->getNetwork();
	}
	ASSERT(net);
	e = net->getEditor();
	if ((e) && (e->isManaged())) {
	    found_managed_editor = TRUE;
	    break;
	}
    }

    if ((found_managed_editor) && (select_next_editor)) {
	this->setActiveEditor(e);
    } else {
	DXWindow *new_hinge;
	if (found_managed_editor) new_hinge = (DXWindow*)e;
	else new_hinge = (DXWindow*)theDXApplication->getAnchor();
	XtVaSetValues (dshell, 
	    XmNtransientFor, new_hinge->getRootWidget(),
	    XmNwindowGroup, XtWindow(new_hinge->getRootWidget()),
	NULL);
	XSync (XtDisplay(dshell), False);
    }
}

void
GetSetConversionDialog::post()
{
    this->unhinge();
    boolean ism = this->isManaged();
    if (this->list) XmListDeselectAllItems (this->list);
    this->Dialog::post();
    if (!ism) this->update();
}

void
GetSetConversionDialog::update()
{
    if (this->quiet_mode) return ;

    if ((!this->getRootWidget()) || (!this->isManaged())) 
	return ;

    //
    // Hang onto selections because they might still be around after the 
    // list is refreshed.
    //
    int cnt = 0;
    XmStringTable xmstrs = NULL;
    XtVaGetValues (this->list, 
	XmNselectedItems, &xmstrs, 
	XmNselectedItemCount, &cnt,
    NULL);
    XmString selected_xmstr = NULL; 
    if (cnt) {
	ASSERT ((xmstrs)&&(xmstrs[0]));
	selected_xmstr = XmStringCopy (xmstrs[0]);
    }

    XmListDeleteAllItems (this->list);
    if (this->referenced_macros) {
	delete this->referenced_macros;
	this->referenced_macros = NULL;
    }

    List *maclist = new List;
    List *gets = NULL; List *sets = NULL;
    if (theDXApplication->network) {
	Widget root = this->getRootWidget();
	XDefineCursor (XtDisplay(root), XtWindow(root), 
	    GetSetConversionDialog::WatchCursor);
	XSync (XtDisplay(root), False);

	GetSetConversionDialog::GetSetPlacements 
	    (theDXApplication->network, maclist, &gets, &sets);

	XUndefineCursor (XtDisplay(root), XtWindow(root));
    } else
	return ;
    this->referenced_macros = new List;


    if ((gets) || (sets)) {
	char *cp = GetSetConversionDialog::GetFileName(theDXApplication->network);
	XmString xmstr = XmStringCreateLtoR (cp, "bold");
	delete cp;
	XmListAddItem (this->list, xmstr, 1);
	XmStringFree(xmstr);
	this->list_includes_main = TRUE;
	if (gets) delete gets;
	if (sets) delete sets;
	sets = gets = NULL;
    } else
	this->list_includes_main = FALSE;

    ListIterator it(*maclist);
    MacroDefinition *md;
    while ( (md = (MacroDefinition*)it.getNext()) ) {
	Network *net = md->getNetwork();
	ASSERT (net);
	this->referenced_macros->appendElement((const void*)md);
	char *cp = GetSetConversionDialog::GetFileName (net);
	XmString xmstr = XmStringCreateLtoR (cp, "bold");
	delete cp;
	XmListAddItemUnselected (this->list, xmstr, 0);
	XmStringFree(xmstr);
    }
    delete maclist;

    if ((selected_xmstr) && (XmListItemExists(this->list, selected_xmstr))) 
	XmListSelectItem (this->list, selected_xmstr, True);
    else {
	this->updateNetName (NULL);
	this->unhinge(TRUE);
    }
    if (selected_xmstr)
	XmStringFree(selected_xmstr);
}


void
GetSetConversionDialog::updateNetName(Network *net)
{
    //
    // Print the name of the net up top.
    //
    char *cp = GetSetConversionDialog::GetFileName(net);
    if ((!cp) || (!cp[0])) {
	if (cp) delete cp;
	cp = DuplicateString ("(no network selected)");
    }

    //
    // If the new name is shorter than the old one, then turn off recomputeSize
    // This disables the icky display when the dialog keeps resizing itself
    //
    XmFontList xmfl;
    Boolean oldRecomp;
    Dimension old_width;
    XtVaGetValues (this->net_name, 
	XmNwidth, &old_width,
	XmNfontList, &xmfl, 
	XmNrecomputeSize, &oldRecomp,
    NULL);

    XmString xmstr = XmStringCreateLtoR (cp, "bold");
    delete cp;

    Boolean recomp = True;
    Dimension neww = XmStringWidth (xmfl, xmstr);
    recomp = ((neww + 8) > old_width);
    if (recomp != oldRecomp) {
	XtVaSetValues (this->net_name, XmNrecomputeSize, recomp, NULL);
    }

    XtVaSetValues  (this->net_name, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);

    //
    // Print the count of Get/Set modules
    //
    List *gets = NULL;
    List *sets = NULL;
    if (net) {
	gets =  net->makeNamedNodeList ("Get");
	sets =  net->makeNamedNodeList ("Set");
    }

    int gcnt = (gets?gets->getSize():0);
    int scnt = (sets?sets->getSize():0);
    char msg[128];
    if ((net) && (net->isMacro()))
	sprintf (msg, "  macro contains:\n%4d Get(s)\n%4d Set(s)", gcnt, scnt);
    else
	sprintf (msg, "network contains:\n%4d Get(s)\n%4d Set(s)", gcnt, scnt);
    xmstr = XmStringCreateLtoR (msg, "bold");

    XtVaSetValues (this->net_name_label, 
	XmNlabelString, xmstr, 
    NULL);
    XtVaSetValues (this->net_name_label, 
	XmNrecomputeSize, False, 
    NULL);
    XmStringFree(xmstr);

    if (gets) delete gets;
    if (sets) delete sets;

    if ((gcnt == 0) && (scnt == 0)) {
	if (net) this->update();
	this->setFindButton (TRUE, TRUE);
    } 
}

void
GetSetConversionDialog::GetSetPlacements 
	(Network *topnet, List * maclist, List **gets, List **sets)
{
    ASSERT(topnet);
    topnet->getReferencedMacros(maclist, NULL);
    ListIterator it(*maclist);
    MacroDefinition *md;
    while ( (md = (MacroDefinition*)it.getNext()) ) {
        Network *net = md->getNetwork();
        ASSERT(net);
        List *mgets =  net->makeNamedNodeList ("Get");
        if (!mgets) {
            List *msets = net->makeNamedNodeList ("Set");
            if (!msets) {
                maclist->removeElement((const void*)md);
            } else
                delete msets;
        } else
            delete mgets;
    }

    *gets = theDXApplication->network->makeNamedNodeList("Get");
    *sets = theDXApplication->network->makeNamedNodeList("Set");
}

void
GetSetConversionDialog::selectAllNodes()
{
EditorWindow *e = this->getActiveEditor();
GlobalLocalNode *gln;

    if (!e) return ;

    Network *net = e->getNetwork();

    List *gets = NULL; 
    List *sets = NULL; 
    if (net) {
	gets = net->makeNamedNodeList("Get");
	sets = net->makeNamedNodeList("Set");
    }
    ListIterator it;

    if ((!gets) && (!sets)) {
	if (net) this->update();
    }

    if ((gets) || (sets))
	e->deselectAllNodes();

    if (gets) {
	it.setList(*gets);
	while ( (gln = (GlobalLocalNode*)it.getNext()) )
	    e->selectNode("Get", gln->getInstanceNumber(), TRUE);
	delete gets;
	gets = NULL;
    }
    if (sets) {
	it.setList(*sets);
	while ( (gln = (GlobalLocalNode*)it.getNext()) )
	    e->selectNode("Set", gln->getInstanceNumber(), TRUE);
	delete sets;
	sets = NULL;
    }
}

void
GetSetConversionDialog::setCommandActivation()
{
EditorWindow *e = this->getActiveEditor();
Network *net = NULL;

    if (e) net = e->getNetwork();

    List *gets = NULL; 
    List *sets = NULL; 
    if (net) {
	gets = net->makeNamedNodeList("Get");
	sets = net->makeNamedNodeList("Set");
    }
    int gcnt = 0;
    int scnt = 0;
    if (gets) gcnt = gets->getSize();
    if (sets) scnt = sets->getSize();

    if ((gcnt == 0) && (scnt == 0)) {
	if (net) this->update();
	else this->updateNetName(NULL);
    } else {
	this->updateNetName(net);
	this->setFindButton (FALSE, FALSE);
    }

    if (gets) delete gets;
    if (sets) delete sets;
}

void
GetSetConversionDialog::selectNextNode()
{
int search_instance;
boolean get_found = FALSE;
boolean set_found = FALSE;
EditorWindow *e = this->getActiveEditor();
Network *net = e->getNetwork();
GlobalLocalNode *gln, *found_node;
ListIterator it;

    if (!net) {
	this->update();
	return ;
    }

    //
    // Search for a Get node with the next higher instance number.
    //
    List *gets = net->makeNamedNodeList("Get");
    if (gets) {
	it.setList(*gets);
	search_instance = 9999999;
	while ( (gln = (GlobalLocalNode*)it.getNext()) ) {
	    int instance = gln->getInstanceNumber();
	    if (instance < this->next_get_instance) continue;
	    else if (instance == this->next_get_instance) {
		get_found = TRUE;
		found_node = gln;
		break;
	    } else if (instance < search_instance) {
		get_found = TRUE;
		found_node = gln;
		search_instance = instance;
	    }
	}
	if (get_found) this->next_get_instance = found_node->getInstanceNumber() + 1;
    }

    //
    // If no get was found, then search for a Get node with the 
    // the next higher instance number.
    //
    List *sets = NULL;
    if (!get_found) {
	sets = net->makeNamedNodeList("Set");
	if (sets) {
	    search_instance = 9999999;
	    it.setList(*sets);
	    while ( (gln = (GlobalLocalNode*)it.getNext()) ) {
		int instance = gln->getInstanceNumber();
		if (instance < this->next_set_instance) continue;
		else if (instance == this->next_set_instance) {
		    set_found = TRUE;
		    found_node = gln;
		    break;
		} else if (instance < search_instance) {
		    set_found = TRUE;
		    found_node = gln;
		    search_instance = instance;
		}
	    }
	    if (set_found) this->next_set_instance = found_node->getInstanceNumber() + 1;
	}
    }

    //
    // If neither Get nor Set is found, then grey out the button.
    // Otherwise set the StandIn hiliting.
    // Update the dialog.
    //
    e->deselectAllNodes();
    if ((!set_found) && (!get_found)) {
	this->setFindButton (TRUE, TRUE);
	this->update();
    } else {
	char *s = DuplicateString (found_node->getNameString());
	e->selectNode(s, found_node->getInstanceNumber(), TRUE);
	delete s;
	this->selectPartner (found_node, e);
	this->setFindButton (FALSE, FALSE);
    }

    if (gets) delete gets;
    if (sets) delete sets;
}

void
GetSetConversionDialog::selectPartner (GlobalLocalNode *node, EditorWindow *ew)
{
const char *node_name = node->getNameString();
int dummy=0;

    if (EqualString(node_name, "Get")) {
	if (node->isOutputConnected(2)) {
	    List *arcs = (List *)node->getOutputArks(2);
	    ListIterator li(*arcs);
	    Ark *a;
	    while ( (a = (Ark*)li.getNext()) ) {
		Node *dest = a->getDestinationNode(dummy);
		const char *dest_name = dest->getNameString();
		if (EqualString(dest_name, "Set")) {
		    char *tmp = DuplicateString (dest_name);
		    ew->selectNode (tmp, dest->getInstanceNumber(), TRUE);
		    delete tmp;
		}
	    }
	}
    } else if (EqualString(node_name, "Set")) {
	if (node->isInputConnected(2)) {
	    List *arcs = (List *)node->getInputArks(2);
	    Ark *a = (Ark*)arcs->getElement(1);
	    Node *src = a->getSourceNode(dummy);
	    const char *src_name = src->getNameString();
	    if (EqualString(src_name, "Get")) {
		char *tmp = DuplicateString (src_name);
		ew->selectNode (tmp, src->getInstanceNumber(), TRUE);
		delete tmp;
	    }
	}
    }
}

void
GetSetConversionDialog::setFindButton (boolean first, boolean done)
{
char *cp;

    if (first) cp = "Find First";
    else cp = "Find Next";

    XmString xmstr = XmStringCreateLtoR (cp, "bold");
    XtVaSetValues (this->find_next_btn, 
	XmNuserData,  (first?FIND_FIRST: FIND_NEXT),
	XmNlabelString, xmstr, 
	XmNsensitive, !done,
    NULL);
    XmStringFree(xmstr);

    XtVaSetValues (this->find_all_btn,
	XmNsensitive, !done,
    NULL);
}


//
// L A Y O U T   T H E    B U T T O N S       L A Y O U T   T H E    B U T T O N S 
// L A Y O U T   T H E    B U T T O N S       L A Y O U T   T H E    B U T T O N S 
//
void GetSetConversionDialog::layoutButtons (Widget parent)
{
    this->find_next_btn = XtVaCreateManagedWidget("Find",
	xmPushButtonWidgetClass, parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		10,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
	XmNsensitive,		False,
	XmNuserData,		FIND_NEXT,
        NULL);

    this->find_all_btn = XtVaCreateManagedWidget("Find All",
	xmPushButtonWidgetClass, parent,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->find_next_btn,
	XmNleftOffset,		10,
	XmNsensitive,		False,
	XmNuserData,		FIND_ALL,
        NULL);

    Widget button = XtVaCreateManagedWidget("Close",
	xmPushButtonWidgetClass, parent,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
        XmNrightAttachment,	XmATTACH_FORM,
        XmNrightOffset,		10,
	XmNuserData,		DONE,
        NULL);

    XtAddCallback(this->find_next_btn, 
		  XmNactivateCallback, 
		  (XtCallbackProc)GetSetConversionDialog_PushbuttonCB, 
		  (XtPointer)this);

    XtAddCallback(this->find_all_btn, 
		  XmNactivateCallback, 
		  (XtCallbackProc)GetSetConversionDialog_PushbuttonCB, 
		  (XtPointer)this);

    XtAddCallback(button, 
		  XmNactivateCallback, 
		  (XtCallbackProc)GetSetConversionDialog_PushbuttonCB, 
		  (XtPointer)this);

}

extern "C" void GetSetConversionDialog_SelectCB(Widget    ,
				  XtPointer clientData,
				  XtPointer cbs)
{
    ASSERT(clientData);
    GetSetConversionDialog *dialog = (GetSetConversionDialog*) clientData;
    XmListCallbackStruct *lcs = (XmListCallbackStruct*)cbs;
    Widget root = dialog->getRootWidget();
    Widget dshell = root;
    while ((dshell) && (XtClass(dshell) != xmDialogShellWidgetClass))
	dshell = XtParent(dshell);

    if (lcs->selected_item_count == 0) {
	dialog->setFindButton (TRUE, TRUE);
	Command *cmd = EditorWindow::GetGlobalCmd();
	if (cmd) cmd->deactivate();
	cmd = EditorWindow::GetLocalCmd();
	if (cmd) cmd->deactivate();
	dialog->unhinge();
	return ;
    }

    //
    // Manage the associated vpe 
    //
    XDefineCursor (XtDisplay(root), XtWindow(root), 
	GetSetConversionDialog::WatchCursor);
    XSync (XtDisplay(root), False);
    dialog->quiet_mode = TRUE;
    EditorWindow *e = dialog->getActiveEditor();
    ASSERT(e);
    if ((!e->isManaged()) || (lcs->reason == XmCR_DEFAULT_ACTION))
	e->manage(); 

    if (dshell) 
	XtVaSetValues (dshell, 
	    XmNtransientFor, e->getRootWidget(), 
	NULL);

    dialog->quiet_mode = FALSE;

    dialog->setFindButton (TRUE, FALSE);
    dialog->updateNetName (e->getNetwork());
    XUndefineCursor (XtDisplay(root), XtWindow(root));
}


extern "C" void GetSetConversionDialog_PushbuttonCB(Widget    widget,
				  XtPointer clientData,
				  XtPointer )
{
    ASSERT(clientData);
    GetSetConversionDialog *dialog = (GetSetConversionDialog*) clientData;
    XtPointer data;
    XtVaGetValues(widget, XmNuserData, &data, NULL);

    switch((int)(long)data)
    {
      case HELP:
	break;
      case FIND_ALL:
	dialog->selectAllNodes();
	break;
      case FIND_NEXT:
	dialog->selectNextNode();
	break;
      case FIND_FIRST:
	dialog->next_get_instance = 0;
	dialog->next_set_instance = 0;
	dialog->selectNextNode();
	break;
      case DONE:
	dialog->unmanage();
	break;
      default:
	ASSERT(FALSE);
	break;
    }
}




//
// Install the default resources for this class.
//
void GetSetConversionDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, GetSetConversionDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}


//
// If the editor window is represented in the list, then update the list because
// its title is changing.
//
void GetSetConversionDialog::notifyTitleChange (EditorWindow *ew, const char *)
{
    Network *net = ew->getNetwork();
    List *gets = (net?net->makeNamedNodeList("Get"):NULL);
    List *sets = (net?net->makeNamedNodeList("Set"):NULL);
    int gcnt = (gets?gets->getSize():0);
    int scnt = (sets?sets->getSize():0);
    if (gets) delete gets;
    if (sets) delete sets;
    if ((!gcnt) && (!scnt)) return ;
    this->update();
}

