/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <limits.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/SeparatoG.h>
#include <Xm/ScrolledW.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>

#include "DXStrings.h"
#include "ListIterator.h"
#include "ControlPanel.h"
#include "ControlPanelGroupDialog.h"
#include "PanelGroupManager.h"
#include "Network.h"
#include "DXApplication.h"
#include "ErrorDialogManager.h"
#include "XmUtility.h"

ControlPanelGroupDialog* theCPGroupDialog = NULL; 

boolean ControlPanelGroupDialog::ClassInitialized = FALSE;

String  ControlPanelGroupDialog::DefaultResources[] = {
    "*XmToggleButton.indicatorSize:     15",
    "*dialogTitle:			Control Panel Group...", 
    "*text.width:                       280",
    "*scrolledWindow.height:            170",
    "*addButton.labelString: 		Add",
    "*deleteButton.labelString: 	Delete",
    "*changeButton.labelString: 	Modify",
    "*closeButton.labelString: 		Close",
    "*nameLabel.labelString: 		Group Name: ",
    "*groupLabel.labelString: 		Groups: ",
    "*panelLabel.labelString: 		Panels: ",
        NULL
};

ControlPanelGroupDialog::ControlPanelGroupDialog(Widget parent)
                                : Dialog("panelGroupDialog", parent)
{
    if (NOT ControlPanelGroupDialog::ClassInitialized)
    {
        ControlPanelGroupDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
ControlPanelGroupDialog::~ControlPanelGroupDialog()
{
}


//
// Install the default resources for this class.
//
void ControlPanelGroupDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ControlPanelGroupDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}


Widget ControlPanelGroupDialog::createDialog(Widget parent)
{
    Widget frame1, frame2, sw1, sw2, separator1;
    Widget nameLabel, groupLabel;
    Arg    arg[5];
    int    n = 0;

    XtSetArg(arg[n], XmNautoUnmanage,        False);  n++;
    XtSetArg(arg[n], XmNresizePolicy,        XmRESIZE_NONE);  n++;

    Widget mainform = this->CreateMainForm(parent, this->name, arg, n);

    this->addbtn = XtVaCreateManagedWidget("addButton",
	xmPushButtonWidgetClass,mainform,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
	XmNwidth,	      60,
        NULL);

    this->deletebtn = XtVaCreateManagedWidget("deleteButton",
	xmPushButtonWidgetClass,mainform,
        XmNleftAttachment,    XmATTACH_WIDGET,
	XmNleftWidget,	      this->addbtn,
        XmNleftOffset,        10,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
	XmNwidth,	      60,
        NULL);

    this->changebtn = XtVaCreateManagedWidget("changeButton",
	xmPushButtonWidgetClass,mainform,
        XmNleftAttachment,    XmATTACH_WIDGET,
	XmNleftWidget,	      this->deletebtn,
        XmNleftOffset,        10,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
	XmNwidth,	      60,
	XmNsensitive, 	      False,
        NULL);

    this->cancel = XtVaCreateManagedWidget("closeButton",
	xmPushButtonWidgetClass,mainform,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
	XmNwidth,	      60,
        NULL);

    separator1 = XtVaCreateManagedWidget("separator",
	xmSeparatorGadgetClass, mainform,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->addbtn,
        XmNbottomOffset,      10,
        NULL);
   
    nameLabel = XtVaCreateManagedWidget("nameLabel", 
	xmLabelWidgetClass,mainform, 
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      14,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);
	
    this->text = XtVaCreateManagedWidget("text", 
	xmTextWidgetClass, mainform,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      10,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_WIDGET,
        XmNleftOffset,        10,
        XmNrightOffset,       5,
        XmNleftWidget,        nameLabel,
        XmNeditMode,          XmSINGLE_LINE_EDIT,
        XmNeditable,          True,
        NULL);

    groupLabel = XtVaCreateManagedWidget("groupLabel", 
	xmLabelWidgetClass,mainform, 
        XmNtopAttachment,     XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNtopOffset,         5,
        XmNleftOffset,        5,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);
	
    XtVaCreateManagedWidget("panelLabel", 
	xmLabelWidgetClass,mainform, 
        XmNtopAttachment,     XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_POSITION,
	XmNleftPosition,      50,
        XmNtopOffset,         5,
        XmNleftOffset,        5,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);
	
    frame1 = XtVaCreateManagedWidget("frame",
	xmFrameWidgetClass, mainform,
        XmNshadowThickness,   1,
        XmNtopAttachment,     XmATTACH_WIDGET,
	XmNtopWidget,	      groupLabel,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNrightAttachment,   XmATTACH_POSITION,
	XmNrightPosition,     50,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->text,
        XmNshadowType,        XmSHADOW_IN,
        XmNleftOffset,    5,
        XmNrightOffset,   3,
        XmNtopOffset,     5,
        XmNbottomOffset,  10,
        NULL);

    frame2 = XtVaCreateManagedWidget("frame",
	xmFrameWidgetClass, mainform,
        XmNshadowThickness,   1,
        XmNtopAttachment,     XmATTACH_WIDGET,
	XmNtopWidget,	      groupLabel,
        XmNleftAttachment,    XmATTACH_POSITION,
	XmNleftPosition,      50,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->text,
        XmNshadowType,        XmSHADOW_IN,
        XmNleftOffset,    3,
        XmNrightOffset,   5,
        XmNtopOffset,     5,
        XmNbottomOffset,  10,
        NULL);

    sw1 = XtVaCreateManagedWidget("scrolledWindow",
				xmScrolledWindowWidgetClass,
				frame1,
				XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		      		XmNshadowThickness,0,
				XmNscrollBarDisplayPolicy, XmSTATIC,
				NULL);

    this->list = XtVaCreateManagedWidget("list",
				  xmListWidgetClass,
				  sw1,
		  		  XmNlistMarginHeight,3,
		  		  XmNlistMarginWidth, 3,
		  		  XmNlistSpacing,     3,
                  		  XmNshadowThickness, 0,
				  XmNselectionPolicy, XmSINGLE_SELECT,
				  XmNlistSizePolicy,  XmCONSTANT,
                                  NULL);

    XtVaSetValues(sw1, XmNworkWindow, this->list, NULL);

    sw2 = XtVaCreateManagedWidget("scrolledWindow",
				xmScrolledWindowWidgetClass,
				frame2,
				XmNscrollingPolicy, XmAUTOMATIC,
		      		XmNshadowThickness,0,
				NULL);
    this->sform = XtVaCreateManagedWidget("sform",
				  xmFormWidgetClass,
				  sw2,
                                  NULL);

    XtVaSetValues(sw2, XmNworkWindow, this->sform, NULL);

    XtAddCallback(this->list,
		  XmNsingleSelectionCallback,
		  (XtCallbackProc)ControlPanelGroupDialog_SelectCB,
		  (XtPointer)this);

    XtAddCallback(this->addbtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ControlPanelGroupDialog_AddCB,
		  (XtPointer)this);

    XtAddCallback(this->deletebtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ControlPanelGroupDialog_DeleteCB,
		  (XtPointer)this);

    XtAddCallback(this->changebtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ControlPanelGroupDialog_ChangeCB,
		  (XtPointer)this);

    XtAddCallback(this->text,
		  XmNmodifyVerifyCallback,
		  (XtCallbackProc)ControlPanelGroupDialog_TextVerifyCB,
		  (XtPointer)this);

    return mainform;
}


void ControlPanelGroupDialog::makeGroupList(int item)
{
    XmString *xmstrings;
    int      items = this->panelManager->getGroupCount();
    int      i = -1;

    if (items != 0)
    {
        xmstrings = new XmString[items];
	for(i=0; i < items; i++)
	{
            xmstrings[i] = 
		XmStringCreateSimple((char*)this->panelManager->getPanelGroup(i+1, NULL));
        }
    }
    else
    {
        items = 1;
        xmstrings = new XmString[1];
        xmstrings[0] = XmStringCreateSimple("(none)");
    }

    XtVaSetValues(this->list, 
    	XmNitemCount,        items,
    	XmNvisibleItemCount, items,
    	XmNitems,            xmstrings,
    	NULL);

    this->lastIndex = 0;

    if(i < 0 OR item == 0)
    {
	XmTextSetString(this->text, NULL);
	XtVaSetValues(this->changebtn, XmNsensitive, False, NULL);
	XtVaSetValues(this->deletebtn, XmNsensitive, False, NULL);
    	XmListDeselectAllItems(this->list);
	this->setToggles();
    }
    else
    {
	if(item > 0)
	     XmListSelectPos(this->list, item > items ? 0 : item, True);
	else
	{
	     char *name = XmTextGetString(this->text);
	     for(i=1; i<=items; i++)
		if(EqualString(name, 
			       (char*)this->panelManager->getPanelGroup(i,NULL)))
		   break;
	     XmListSelectPos(this->list, i, True);
	     XtFree(name);   //	AJ
	}
    }

    for (i=0 ; i<items ; i++)
        XmStringFree(xmstrings[i]);

    delete xmstrings;
}

void ControlPanelGroupDialog::makeToggles()
{
     int i;
     Boolean set = FALSE, state = FALSE;
     Widget widget, lastwidget = NULL; // NULL stops scary cc warnings
     ControlPanel *cp;
     List    glist;
     char    *pname;
     Network      *network = this->panelManager->getNetwork();

     ListIterator  li(this->toggleList[0]);
     int size = network->getPanelCount();

     XtUnmanageChild(this->sform);

     if(this->toggleList[0].getSize() > 0)
     {
        while( (widget = (Widget)li.getNext()) )
             XtDestroyWidget(widget);

	li.setList(this->toggleList[1]);
        while( (widget = (Widget)li.getNext()) )
             XtDestroyWidget(widget);

        this->toggleList[0].clear();
        this->toggleList[1].clear();
     }

     if(size == 0)
     {
        this->unmanage();
        return;
     }

     set = this->lastIndex 
	   AND this->panelManager->getGroupCount()
     	   AND this->panelManager->getPanelGroup(this->lastIndex, &glist);

     for(i = 0; i < size; i++)
     {
        cp = network->getPanelByIndex(i+1);
	/*inst = cp->getInstanceNumber();*/
	if(set)
	     state = glist.isMember((void*)cp->getInstanceNumber());
	else
	     state = FALSE;

        pname = (char*)cp->getPanelNameString();
        if(IsBlankString(pname))
            pname = "Control Panel";

        widget = XmCreateToggleButton(this->sform,
                                     pname, 
                                     NULL,
                                     0);
        XtVaSetValues(widget,
		      XmNset,		 state,
		      XmNindicatorType,	 XmN_OF_MANY,
		      XmNalignment,	 XmALIGNMENT_BEGINNING,
		      XmNshadowThickness,0,
		      XmNleftAttachment, XmATTACH_FORM,
		      XmNrightAttachment,XmATTACH_POSITION,
		      XmNrightPosition,  70,
		      XmNtopOffset,      2,
		      XmNleftOffset,     2,
                      NULL);
	if(i == 0)
            XtVaSetValues(widget, XmNtopAttachment,XmATTACH_FORM,NULL);
	else
            XtVaSetValues(widget,
		          XmNtopAttachment,XmATTACH_WIDGET,
		          XmNtopWidget,    lastwidget,
                          NULL);

        this->toggleList[0].appendElement((void*)widget);
        XtManageChild(widget);

	state = cp->isManaged();
        widget = XmCreateToggleButton(this->sform,
                                     "...",
                                     NULL,
                                     0);
        XtVaSetValues(widget,
		      XmNuserData,      cp,
		      XmNset,		state,
		      XmNindicatorOn,	False,
		      XmNfillOnSelect,	False,
		      XmNrightAttachment,XmATTACH_FORM,
		      XmNleftAttachment,XmATTACH_POSITION,
		      XmNleftPosition,  70,
		      XmNindicatorSize, 1,
		      XmNtopOffset,     2,
		      XmNleftOffset,    5,
                      NULL);
	if(i == 0)
            XtVaSetValues(widget, XmNtopAttachment,XmATTACH_FORM,NULL);
	else
            XtVaSetValues(widget,
		          XmNtopAttachment,XmATTACH_WIDGET,
		          XmNtopWidget,    lastwidget,
                          NULL);

        XtAddCallback(widget,
                      XmNvalueChangedCallback,
                      (XtCallbackProc)ControlPanelGroupDialog_OpenPanelCB,
                      (XtPointer)this);

        this->toggleList[1].appendElement((void*)widget);
        XtManageChild(widget);

	lastwidget = widget;
     }

     XtManageChild(this->sform);
}

void ControlPanelGroupDialog::setToggles()
{
     Widget   	widget;
     List	plist;
     int	i, size = this->toggleList[0].getSize(); 
     boolean	state=false, set = this->lastIndex;
     ControlPanel *cp;

     if(set)
     	this->panelManager->getPanelGroup(this->lastIndex, &plist);

     for(i = 1; i <= size; i++)
     {
        widget = (Widget)this->toggleList[1].getElement(i);
        XtVaGetValues(widget, XmNuserData, &cp, NULL);
        XtVaSetValues(widget, XmNset, cp->isManaged(), NULL);
        if(set)
            state = plist.isMember((void*)cp->getInstanceNumber());
        widget = (Widget)this->toggleList[0].getElement(i);
        XtVaSetValues(widget, XmNset, set AND state, NULL);
     }

}

extern "C" void ControlPanelGroupDialog_DeleteCB(Widget widget,
                                           XtPointer clientData,
                                           XtPointer callData)
{
    const char *name;
    ControlPanelGroupDialog* pgd = (ControlPanelGroupDialog*)clientData;

    if(pgd->lastIndex == 0)
	ModalErrorMessage(pgd->getRootWidget(), "No group name is chosen.");
    else if(pgd->panelManager->getGroupCount() == 0)
    	ModalErrorMessage(pgd->getRootWidget(), "No group exists.");
    else
    {
	name = pgd->panelManager->getPanelGroup(pgd->lastIndex, NULL);
	pgd->panelManager->removePanelGroup(name);
    	pgd->makeGroupList(pgd->lastIndex);
    }

    //
    // Let the application know that the panels have changed (generally,
    // this means notifying the DXWindows which may have panelAccessDialogs
    // open).
    //
    theDXApplication->notifyPanelChanged();
}

extern "C" void ControlPanelGroupDialog_ChangeCB(Widget widget,
                                           XtPointer clientData,
                                           XtPointer callData)
{
    ControlPanelGroupDialog* pgd = (ControlPanelGroupDialog*)clientData;
    ASSERT(pgd->lastIndex);

    ControlPanelGroupDialog_AddCB(widget, clientData, callData);
}

extern "C" void ControlPanelGroupDialog_AddCB(Widget widget,
                                           XtPointer clientData,
                                           XtPointer callData)
{
    ControlPanelGroupDialog* pgd = (ControlPanelGroupDialog*)clientData;
    char* oldname = NULL;
    char* name = XmTextGetString(pgd->text); 
    Boolean setting;

    if(widget == pgd->changebtn)
    {
    	oldname = (char*)pgd->panelManager->getPanelGroup(pgd->lastIndex, NULL);
	if(EqualString(oldname, name))
	{
    	    pgd->panelManager->removePanelGroup(name);
	    oldname = NULL;
        }
    }	

    if(IsBlankString(name))
    {
	ModalErrorMessage(pgd->getRootWidget(), "No name is given.");
	goto error;
    }
    else
    if(NOT pgd->panelManager->createPanelGroup(name))
    {
	ModalErrorMessage(pgd->getRootWidget(), "%s already exists.", name);
	goto error;
    }
    else
    {
	int     i, size = pgd->toggleList[0].getSize();	
	Widget  widget;
	ControlPanel *cp;	

	for(i = 1; i <= size; i++)	
	{
	     widget = (Widget)pgd->toggleList[0].getElement(i);
	     XtVaGetValues(widget, XmNset, &setting, NULL);
	     if(setting)
	     {
	     	widget = (Widget)pgd->toggleList[1].getElement(i);
	     	XtVaGetValues(widget, XmNuserData, &cp, NULL);
		pgd->panelManager->addGroupMember(name,cp->getInstanceNumber()); 
	     }
	}

	if(oldname)	
    	    pgd->panelManager->removePanelGroup(oldname);
	pgd->makeGroupList(-1);
    }


    //
    // Let the application know that the panels have changed (generally,
    // this means notifying the DXWindows which may have panelAccessDialogs
    // open).
    //
    theDXApplication->notifyPanelChanged();

error:
    if(name)  XtFree(name);   //	AJ
}

extern "C" void ControlPanelGroupDialog_SelectCB(Widget widget,
                                           XtPointer clientData,
                                           XtPointer callData)
{
    ControlPanelGroupDialog* pgd = (ControlPanelGroupDialog*)clientData;
    XmListCallbackStruct* cs = (XmListCallbackStruct*)callData;

    int callIndex = cs->item_position;

    if(callIndex == pgd->lastIndex)
    {
	pgd->lastIndex = 0;
	XmTextSetString(pgd->text, NULL);
	XtVaSetValues(pgd->changebtn, XmNsensitive, False, NULL);
	XtVaSetValues(pgd->deletebtn, XmNsensitive, False, NULL);
	pgd->setToggles();
    }
    else
    {
	char *name = (char*)pgd->panelManager->getPanelGroup(callIndex, NULL); 
	XmTextSetString(pgd->text, name);
	if(name)
        {
	    XtVaSetValues(pgd->changebtn, XmNsensitive, True, NULL);
	    XtVaSetValues(pgd->deletebtn, XmNsensitive, True, NULL);
        }

	pgd->lastIndex = callIndex;
	pgd->setToggles();
    }
}


extern "C" void ControlPanelGroupDialog_OpenPanelCB(Widget widget, 
					   XtPointer clientData, 
					   XtPointer callData)
{
    Boolean  set;
    ControlPanelGroupDialog *pgd = (ControlPanelGroupDialog*)clientData;
    ControlPanel *cp;
    int  cpCount2, cpCount1 = pgd->panelManager->getNetwork()->getPanelCount();

    XtVaGetValues(widget, 
		  XmNset, &set,
		  XmNuserData, &cp, 
		  NULL);

    set ? cp->manage() : cp->unmanage(); 

    cpCount2 = pgd->panelManager->getNetwork()->getPanelCount(); 

    if(cpCount2 == 0)
         pgd->unmanage();
    else if (cpCount2 < cpCount1)
    	 pgd->makeToggles();
}

extern "C" void ControlPanelGroupDialog_TextVerifyCB(Widget    widget,
                                  	   XtPointer clientData,
                                  	   XtPointer callData)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)callData;

    VerifyNameText(widget, cbs);
}


void ControlPanelGroupDialog::manage()
{
    this->makeGroupList();
    this->makeToggles();
    XmTextSetString(this->text,NULL);
    this->Dialog::manage();
}

void ControlPanelGroupDialog::setData(PanelGroupManager* pgm)
{
    this->panelManager = pgm;
}

void SetControlPanelGroup(PanelGroupManager* pgm)
{
    if(NOT theCPGroupDialog)
	theCPGroupDialog =
	      	new ControlPanelGroupDialog(theDXApplication->getRootWidget());
    theCPGroupDialog->setData(pgm);
    theCPGroupDialog->post();
}

