/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <stdlib.h>
#include <limits.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Label.h>

#include "DXStrings.h"
#include "DXApplication.h"
#include "EditorWindow.h"
#include "ProcessGroupAssignDialog.h"
#include "ProcessGroupManager.h"
#include "ProcessGroupOptionsDialog.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "List.h"
#include "ListIterator.h"
#include "Dictionary.h"
#include "DXPacketIF.h"
#include "XmUtility.h"

boolean ProcessGroupAssignDialog::ClassInitialized = FALSE;

String  ProcessGroupAssignDialog::DefaultResources[] = {
	"*dialogTitle:			Execution Group Assignment...",
        "*text.width:                   170",
        "*scrolledWindow.height:        230",
        "*form.width:                   330",
	"*closeButton.labelString:	Close",
	"*applyButton.labelString:	Apply",
	"*XmPushButton.width:		80",
	"*groupLabel.labelString:	Group Name:",
	"*hostLabel.labelString:	Host Name:",
	"*highlightThickness:		0",
        NULL
};

ProcessGroupAssignDialog::ProcessGroupAssignDialog( DXApplication *app) 
		: Dialog("processGroupAssignDialog",app->getRootWidget())
{
    this->app = app;
    this->dirty = FALSE;
    this->i_caused_it = FALSE;

    if (NOT ProcessGroupAssignDialog::ClassInitialized)
    {
        ProcessGroupAssignDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void ProcessGroupAssignDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ProcessGroupAssignDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

ProcessGroupAssignDialog::~ProcessGroupAssignDialog()
{
    if(this->pgod) delete this->pgod;
}

Widget ProcessGroupAssignDialog::createDialog(Widget parent)
{
    Widget form;
    Arg    arg[5];
    int    n = 0;

    XtSetArg(arg[n], XmNautoUnmanage,        False);  n++;
    XtSetArg(arg[n], XmNnoResize,            False);  n++;
 
    Widget mainForm = this->CreateMainForm(parent, this->name, arg, n);

    this->ok = XtVaCreateManagedWidget("applyButton",xmPushButtonWidgetClass, 
	mainForm,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       10,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    this->cancel = XtVaCreateManagedWidget("closeButton",xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       10,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    Widget separator = XtVaCreateManagedWidget("separator",xmSeparatorGadgetClass, mainForm,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->cancel,
        XmNbottomOffset,      10,
        NULL);

    this->hostlabel = XtVaCreateManagedWidget("hostLabel",xmLabelWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator,
        XmNbottomOffset,      14,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        10,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);

    this->option_pb = XtVaCreateManagedWidget("Options...",
	xmPushButtonWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_OPPOSITE_WIDGET,
        XmNbottomWidget,      this->hostlabel,
        XmNbottomOffset,      -4,
        XmNtopAttachment,     XmATTACH_OPPOSITE_WIDGET,
        XmNtopWidget,         this->hostlabel,
        XmNtopOffset,         -4,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNleftOffset,        10,
        XmNrightOffset,       10,
        NULL);

    this->hosttext = XtVaCreateManagedWidget("text",xmTextWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator,
        XmNbottomOffset,      10,
        XmNrightAttachment,   XmATTACH_WIDGET,
	XmNrightWidget,       this->option_pb,
	XmNrightOffset,	      5,
        XmNleftAttachment,    XmATTACH_WIDGET,
        XmNleftOffset,        5,
        XmNleftWidget,        this->hostlabel,
        XmNeditMode,          XmSINGLE_LINE_EDIT,
        XmNeditable,          True,
        NULL);
	
    Widget groupLabel = XtVaCreateManagedWidget("groupLabel",
	xmLabelWidgetClass, mainForm,
        XmNtopAttachment,     XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNtopOffset,         10,
        XmNleftOffset,        20,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);
 
    XtVaCreateManagedWidget("hostLabel",
	xmLabelWidgetClass, mainForm,
        XmNtopAttachment,     XmATTACH_FORM,
        XmNtopOffset,         10,
        XmNleftAttachment,    XmATTACH_POSITION,
        XmNleftPosition,      50,
        XmNleftOffset,        -5,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);
 
    Widget frame = XtVaCreateManagedWidget("frame",xmFrameWidgetClass, mainForm,
        XmNshadowThickness,   2,
        XmNtopAttachment,     XmATTACH_WIDGET,
	XmNtopWidget,	      groupLabel,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNshadowType,        XmSHADOW_ETCHED_IN,
        XmNbottomWidget,      this->hosttext,
        XmNtopOffset,         10,
        XmNbottomOffset,      10,
        XmNleftOffset,        10,
        XmNrightOffset,       10,
	XmNmarginWidth,	      3,
	XmNmarginHeight,      3,
        NULL);

    Widget sw = XtVaCreateManagedWidget("scrolledWindow",
                                xmScrolledWindowWidgetClass,
                                frame,
                                XmNscrollingPolicy, XmAUTOMATIC,
                                XmNshadowThickness, 1,
                                NULL);

    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, sw, 
	NULL);

    this->grouplist = XtVaCreateManagedWidget("GroupList", 
	xmListWidgetClass,   form,
        XmNlistMarginHeight, 3,
        XmNlistMarginWidth,  0,
        XmNlistSpacing,      3,
        XmNshadowThickness,  0,
        XmNselectionPolicy,  XmSINGLE_SELECT,
        XmNlistSizePolicy,   XmCONSTANT,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNleftOffset,	     3,
	XmNrightAttachment,  XmATTACH_POSITION,
	XmNrightPosition,    50,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    this->hostlist = XtVaCreateManagedWidget("HostList", 
	xmListWidgetClass,   form,
        XmNlistMarginHeight, 3,
        XmNlistMarginWidth,  0,
        XmNlistSpacing,      3,
        XmNshadowThickness,  0,
        XmNselectionPolicy,  XmSINGLE_SELECT,
        XmNlistSizePolicy,   XmCONSTANT,
	XmNleftAttachment,   XmATTACH_POSITION,
	XmNleftPosition,     50,	     
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    XtVaSetValues(sw, XmNworkWindow, form, NULL);

    XtAddCallback(XtParent(mainForm),
                  XmNpopdownCallback,
                  (XtCallbackProc)ProcessGroupAssignDialog_PopdownCB,
                  (XtPointer)this);

    this->addCallbacks();

    this->pgod = 
	new ProcessGroupOptionsDialog(this);

    return mainForm;
}

extern "C" void ProcessGroupAssignDialog_PopdownCB(Widget    widget,
                                        XtPointer clientData,
                                        XtPointer callData)
{
    ProcessGroupAssignDialog *dialog = (ProcessGroupAssignDialog*)clientData;

    dialog->unmanage();
}

extern "C" void ProcessGroupAssignDialog_OptionCB(Widget    widget, 
				        XtPointer clientData,
				        XtPointer callData)
{
    ProcessGroupAssignDialog *dialog = (ProcessGroupAssignDialog*)clientData;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    char *name = XmTextGetString(dialog->hosttext);  // AJ 
    const char *args = pgm->getArgument(name);
    dialog->pgod->post();
    dialog->pgod->setArgs(args);

    if(name) XtFree(name);    //  AJ
}
extern "C" void ProcessGroupAssignDialog_TextCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupAssignDialog *dialog = (ProcessGroupAssignDialog*)clientData;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    if(dialog->lastIndex == 0)
    {
        ModalErrorMessage(dialog->getRootWidget(), "No group is chosen.");
        return;
    }

    char* name = XmTextGetString(dialog->hosttext);

    if(IsBlankString(name)
      OR EqualString(name, "DEFAULT") 
      OR EqualString(name, "default")
      OR EqualString(name, "localhost"))
    {
    	pgm->assignHost(dialog->lastIndex, NULL);
    	XtSetSensitive(dialog->option_pb, False);
    }
    else
    {
    	pgm->assignHost(dialog->lastIndex, name);
    	XtSetSensitive(dialog->option_pb, True);
    }

    if(!dialog->i_caused_it)
	dialog->makeList(dialog->lastIndex);
    dialog->dirty = TRUE;

    if(name)  XtFree(name);  //	AJ

}

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern "C" void ProcessGroupAssignDialog_ListButtonDownEH(Widget    widget, 
					      XtPointer clientData,
					      XEvent    *event,
					      Boolean   *ctd)
{
    ProcessGroupAssignDialog *dialog = (ProcessGroupAssignDialog*)clientData;
    int item;
    int tip;
    int vic;
    Dimension h;
    
    XtVaGetValues(widget,
		XmNtopItemPosition,	&tip,
		XmNvisibleItemCount,	&vic,
		XmNheight,		&h,
		NULL);
    item = tip + event->xbutton.y/(h/vic);
    if(item == dialog->lastIndex)
	XmListDeselectAllItems((widget == dialog->grouplist) ?
				dialog->hostlist : dialog->grouplist);
    else
	XmListSelectPos((widget == dialog->grouplist) ? 
			 dialog->hostlist : dialog->grouplist, 
			 item, False);
}

//
// Note: This function gets called after SelectListCB is called when the 
//       lists are double-clicked. 
//    
extern "C" void ProcessGroupAssignDialog_DefaultActionCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupAssignDialog *dialog = (ProcessGroupAssignDialog*)clientData;

    if(dialog->lastIndex == 0)
	XmListDeselectAllItems((widget == dialog->grouplist) ?
				dialog->hostlist : dialog->grouplist);
    else
	XmListSelectPos((widget == dialog->grouplist) ? 
			 dialog->hostlist : dialog->grouplist, 
			 dialog->lastIndex, False);
}

extern "C" void ProcessGroupAssignDialog_SelectListCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupAssignDialog *dialog = (ProcessGroupAssignDialog*)clientData;
    XmListCallbackStruct* cs = (XmListCallbackStruct*)callData;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    int callIndex = cs->item_position;

    dialog->i_caused_it = TRUE;
    if(callIndex == dialog->lastIndex)
    {
        XmListDeselectAllItems(dialog->grouplist);
        XmListDeselectAllItems(dialog->hostlist);
        dialog->lastIndex = 0;
        XmTextSetString(dialog->hosttext, "");
	SetTextSensitive(dialog->hosttext, False, 
			 theDXApplication->getInsensitiveColor());
	XtSetSensitive(dialog->hostlabel, False);
	XtSetSensitive(dialog->option_pb, False);
    }
    else
    {
        const char *name = pgm->getGroupNewHost(callIndex);
        if(!name)
             name = pgm->getGroupHost(callIndex);
        if(!name)
             name = "localhost";
 
        XmTextSetString(dialog->hosttext, (char*)name);
        XtVaSetValues(dialog->hosttext, XmNcursorPosition, STRLEN(name),NULL);
	SetTextSensitive(dialog->hosttext, True,
			 theDXApplication->getForeground());
	XtSetSensitive(dialog->hostlabel, True);

	XtSetSensitive(dialog->option_pb, !EqualString(name, "localhost"));

        dialog->lastIndex = callIndex;

	//
	// Just in case.
	//
	//XmListSelectPos( dialog->grouplist, callIndex, False);
	//XmListSelectPos( dialog->hostlist,  callIndex, False);

    }
    dialog->i_caused_it = FALSE;
}

void ProcessGroupAssignDialog::makeList(int item)
{
    XmString *gstrings;
    XmString *hstrings;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif
    int      items = pgm->getGroupCount(); 
    int      i = -1;
    const char    *host;

    if (items != 0)
    {
        gstrings = new XmString[items];
        hstrings = new XmString[items];
        for(i=0; i < items; i++)
        {
	    host = pgm->getGroupNewHost(i+1);
	    if(!host)
		host = pgm->getGroupHost(i+1);

	    if(!host)
	    {
		host = "localhost";
	    }
            gstrings[i] = XmStringCreateSimple((char*)pgm->getGroupName(i+1));
            hstrings[i] = XmStringCreateSimple((char*)host);
        }
    }
    else
    {
        items = 1;
        gstrings = new XmString[1];
        gstrings[0] = XmStringCreateSimple("(none)");

        hstrings = new XmString[1];
        hstrings[0] = XmStringCreateSimple("(none)");
    }

    XtVaSetValues(this->grouplist,
        XmNvisibleItemCount, items,
        NULL);
    XmListDeleteAllItems(this->grouplist);
    XmListAddItems(this->grouplist, gstrings, items, 0);

    XtVaSetValues(this->hostlist,
        XmNvisibleItemCount, items,
        NULL);
    XmListDeleteAllItems(this->hostlist);
    XmListAddItems(this->hostlist, hstrings, items, 0);

    XtSetSensitive(this->grouplist, items != 0);
    XtSetSensitive(this->hostlist, items != 0);

    this->lastIndex = item;

    if(i < 0 OR item == 0)
    {
        XmListDeselectAllItems(this->grouplist);
        XmListDeselectAllItems(this->hostlist);
        XmTextSetString(this->hosttext, "");
	SetTextSensitive(this->hosttext, False,
			 theDXApplication->getInsensitiveColor());
        XtSetSensitive(this->hostlabel, False);
	XtSetSensitive(this->option_pb, False);
    }
    else
    {
        XmListSelectPos(this->grouplist, item > items ? 0 : item, False);
        XmListSelectPos(this->hostlist,  item > items ? 0 : item, False);
    }

    for (i=0 ; i<items ; i++)
    {
        XmStringFree(gstrings[i]);
        XmStringFree(hstrings[i]);
    }

    delete gstrings;
    delete hstrings;
}

boolean ProcessGroupAssignDialog::okCallback(Dialog* dialog)
{
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    if(NOT this->dirty)
	return FALSE;
	
//    DXPacketIF *p = this->app->getPacketIF();   
// Do we need this warning ?
//    if (NOT p)
//	WarningMessage("No connection to the server.");

    pgm->updateHosts();
    pgm->updateAssignment();

    this->dirty = FALSE;

    return FALSE;
}

void ProcessGroupAssignDialog::manage()
{
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#endif
    if(NOT this->isManaged())
    {
#if WORKSPACE_PAGES
	pgm->clearNewHosts();
#else
        this->app->PGManager->clearNewHosts();
#endif
    	this->makeList(0);
    }
    this->Dialog::manage();
}


void ProcessGroupAssignDialog::addCallbacks()
{
    XtAddCallback(this->hosttext,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupAssignDialog_TextCB,
		  (XtPointer)this);

    XtAddCallback(this->grouplist,
                  XmNsingleSelectionCallback,
                  (XtCallbackProc)ProcessGroupAssignDialog_SelectListCB,
                  (XtPointer)this);

    XtAddCallback(this->hostlist,
                  XmNsingleSelectionCallback,
                  (XtCallbackProc)ProcessGroupAssignDialog_SelectListCB,
                  (XtPointer)this);

    XtAddCallback(this->grouplist,
                  XmNdefaultActionCallback,
                  (XtCallbackProc)ProcessGroupAssignDialog_DefaultActionCB,
                  (XtPointer)this);

    XtAddCallback(this->hostlist,
                  XmNdefaultActionCallback,
                  (XtCallbackProc)ProcessGroupAssignDialog_DefaultActionCB,
                  (XtPointer)this);

    XtAddCallback(this->option_pb,
                  XmNactivateCallback,
                  (XtCallbackProc)ProcessGroupAssignDialog_OptionCB,
                  (XtPointer)this);

    XtAddEventHandler(this->grouplist,
		      ButtonPressMask,
		      False,
		      (XtEventHandler)ProcessGroupAssignDialog_ListButtonDownEH,
		      (XtPointer)this);

    XtAddEventHandler(this->hostlist,
		      ButtonPressMask,
		      False,
		      (XtEventHandler)ProcessGroupAssignDialog_ListButtonDownEH,
		      (XtPointer)this);

}

