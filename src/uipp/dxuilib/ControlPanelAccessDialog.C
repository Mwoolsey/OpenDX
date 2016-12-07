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
#include <Xm/SeparatoG.h>
#include <Xm/ScrolledW.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>

#include "DXStrings.h"
#include "ListIterator.h"
#include "ControlPanel.h"
#include "ControlPanelAccessDialog.h"
#include "PanelAccessManager.h"
#include "PanelGroupManager.h"
#include "Network.h"
#include "DXApplication.h"

ControlPanelAccessDialog* theCPAccessDialog = NULL; 

boolean ControlPanelAccessDialog::ClassInitialized = FALSE;

String  ControlPanelAccessDialog::DefaultResources[] = {
	"*XmToggleButton.indicatorSize: 15",
	".title:			Control Panel Access...", 
	"*okButton.labelString:      	OK",
	"*cancelButton.labelString:  	Cancel",
        NULL
};

ControlPanelAccessDialog::ControlPanelAccessDialog(Widget parent,
						   PanelAccessManager* pam)
                    	: Dialog("panelAccessManager", parent)
{
    ASSERT(pam);
    this->separator = NULL;
    this->panelManager = pam;

    if (NOT ControlPanelAccessDialog::ClassInitialized)
    {
        ControlPanelAccessDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

ControlPanelAccessDialog::~ControlPanelAccessDialog()
{
}

//
// Install the default resources for this class.
//
void ControlPanelAccessDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ControlPanelAccessDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

Widget ControlPanelAccessDialog::createDialog(Widget parent)
{
    Arg    arg[5];
    int    n = 0;
    Widget frame, sw, separator;

    XtSetArg(arg[n], XmNwidth,  260);  n++;
    XtSetArg(arg[n], XmNheight, 200);  n++;

    Widget mainform = this->CreateMainForm(parent, this->name, arg, n);

    this->ok = XtVaCreateManagedWidget("okButton",
	xmPushButtonWidgetClass,mainform,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
	XmNwidth,	      60,
        NULL);

    this->cancel = XtVaCreateManagedWidget("cancelButton",
	xmPushButtonWidgetClass,mainform,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
	XmNwidth,	      60,
        NULL);

    separator = XtVaCreateManagedWidget("separator",
	xmSeparatorGadgetClass, mainform,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->ok,
        XmNbottomOffset,      10,
        NULL);
   

    frame = XtVaCreateManagedWidget("frame",xmFrameWidgetClass, mainform,
        XmNshadowThickness,   2,
        XmNtopAttachment,     XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator,
        XmNshadowType,        XmSHADOW_ETCHED_IN,
        XmNleftOffset,    5,
        XmNrightOffset,   5,
        XmNtopOffset,     10,
        XmNbottomOffset,  10,
	XmNmarginWidth,   3,
	XmNmarginHeight,  3,
        NULL);

    sw = XtVaCreateManagedWidget("scrolledWindow",
				xmScrolledWindowWidgetClass,
				frame,
				XmNscrollingPolicy, XmAUTOMATIC,
				XmNvisualPolicy,    XmVARIABLE,
		      		XmNshadowThickness, 1,
				NULL);

    this->sform = XtVaCreateManagedWidget("sform",
				  xmFormWidgetClass,
				  sw,
                                  NULL);

    XtVaSetValues(sw, XmNworkWindow, this->sform, NULL);

    return mainform;
}

void ControlPanelAccessDialog::makeToggles()
{
     int i, inst;
     Boolean set;
     Widget widget, lastwidget = NULL; // NULL stops scary cc warnings 
     ControlPanel *cp;
     Network      *network = this->panelManager->getNetwork();
     PanelGroupManager * pgm = network->getPanelGroupManager();
     char   *gname, *pname;

     XtUnmanageChild(this->sform);

     ListIterator  li(this->toggleList[0]);
     int size = network->getPanelCount();

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

     if(this->separator)
     {
	XtDestroyWidget(this->separator);
	this->separator = NULL;
     }

     if(this->toggleList[2].getSize() > 0)
     {
	li.setList(this->toggleList[2]);
        while( (widget = (Widget)li.getNext()) )
             XtDestroyWidget(widget);

        this->toggleList[2].clear();
     }

     if(size == 0)
        return;

     for(i = 0; i < size; i++)
     {
        cp = network->getPanelByIndex(i+1);
	inst = cp->getInstanceNumber();
	set  = this->panelManager->isAccessiblePanel(inst);

        pname = (char*)cp->getPanelNameString();
        if(IsBlankString(pname))
            pname = "Control Panel";

        widget = XmCreateToggleButton(this->sform,
                                     pname, 
                                     NULL,
                                     0);
        XtVaSetValues(widget,
		      XmNset,		 set,
		      XmNindicatorType,	 XmN_OF_MANY,
		      XmNalignment,	 XmALIGNMENT_BEGINNING,
		      XmNshadowThickness,0,
		      XmNleftAttachment, XmATTACH_FORM,
		      XmNrightAttachment,XmATTACH_POSITION,
		      XmNrightPosition,  80,
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

	if(this->panelManager->getControlPanel() == cp)
            XtVaSetValues(widget, 
			  XmNsensitive, False,
			  XmNset,	False, 
			  NULL);

        this->toggleList[0].appendElement((void*)widget);
        XtManageChild(widget);

	set = cp->isManaged();
        widget = XmCreateToggleButton(this->sform,
                                     "...",
                                     NULL,
                                     0);
        XtVaSetValues(widget,
		      XmNuserData,      cp,
		      XmNset,		set,
		      XmNindicatorOn,	False,
    		      XmNfillOnSelect,  False,
		      XmNrightAttachment,XmATTACH_FORM,
		      XmNleftAttachment,XmATTACH_POSITION,
		      XmNleftPosition,  80,
		      XmNtopOffset,     2,
		      XmNleftOffset,    5,
		      XmNindicatorSize, 1,
		      XmNspacing,	0,
                      NULL);
	if(i == 0)
            XtVaSetValues(widget, XmNtopAttachment,XmATTACH_FORM,NULL);
	else
            XtVaSetValues(widget,
		          XmNtopAttachment,XmATTACH_WIDGET,
		          XmNtopWidget,    lastwidget,
                          NULL);

	if(this->panelManager->getControlPanel() == cp)
            XtVaSetValues(widget, XmNsensitive, False, NULL);

        XtAddCallback(widget,
                      XmNvalueChangedCallback,
                      (XtCallbackProc)ControlPanelAccessDialog_OpenPanelCB,
                      (XtPointer)this);

        this->toggleList[1].appendElement((void*)widget);
        XtManageChild(widget);

	lastwidget = widget;
     }

    size = pgm->getGroupCount();

    if(size == 0)
    {
     	XtManageChild(this->sform);
	return;
    }

    this->separator = XtVaCreateManagedWidget("separator",
	xmSeparatorGadgetClass, this->sform,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNtopAttachment,     XmATTACH_WIDGET,
        XmNtopWidget,         lastwidget,
        XmNtopOffset,         5,
        NULL);
   
     for(i = 1; i <= size; i++)
     {
	gname = (char*)pgm->getPanelGroup(i, NULL);
	set  = this->panelManager->isAccessibleGroup(gname);

        widget = XmCreateToggleButton(this->sform, gname, NULL, 0);
        XtVaSetValues(widget,
                      XmNset,            set,
                      XmNindicatorType,  XmN_OF_MANY,
                      XmNalignment,      XmALIGNMENT_BEGINNING,
                      XmNshadowThickness,0,
                      XmNleftAttachment, XmATTACH_FORM,
                      XmNtopAttachment,  XmATTACH_WIDGET,
                      XmNtopOffset,      2,
                      XmNleftOffset,     2,
                      NULL);
        if(i == 1)
            XtVaSetValues(widget, 
			  XmNtopWidget,    this->separator,
                          XmNtopOffset,    5,
			  NULL);
        else
            XtVaSetValues(widget,
                          XmNtopWidget,    lastwidget,
                          NULL);

        this->toggleList[2].appendElement((void*)widget);
        XtManageChild(widget);

	lastwidget = widget;
    }

     XtManageChild(this->sform);
}

boolean ControlPanelAccessDialog::okCallback(Dialog *dialog)
{
    Boolean set;
    int i, inst, size = this->toggleList[0].getSize();
    Widget widget;
    Network *network = this->panelManager->getNetwork();
    PanelGroupManager* pgm = network->getPanelGroupManager();

    network->setFileDirty();

    this->panelManager->allowAllPanelAccess();
    this->panelManager->allowAllGroupAccess();
 
    for(i = 1; i <= size; i++)
    {
	widget = (Widget)this->toggleList[0].getElement(i);
	XtVaGetValues(widget, XmNset, &set, NULL);

	if(NOT set)
	{
	    inst = network->getPanelByIndex(i)->getInstanceNumber();
	    this->panelManager->addInaccessiblePanel(inst);
	}	
    }

    size = this->toggleList[2].getSize();

    for(i = 1; i <= size; i++)
    {
	widget = (Widget)this->toggleList[2].getElement(i);
	XtVaGetValues(widget, XmNset, &set, NULL);

	if(NOT set)
	{
    	     const char *gname = pgm->getPanelGroup(i,NULL);
	     this->panelManager->addInaccessibleGroup(gname);
	}	
    }
    return TRUE;
}


extern "C" void ControlPanelAccessDialog_OpenPanelCB(Widget widget, 
					   XtPointer clientData, 
					   XtPointer callData)
{
    Boolean  set;
    ControlPanelAccessDialog *pad = (ControlPanelAccessDialog*)clientData;
    ControlPanel *cp;
    int  cpCount2, cpCount1 = pad->panelManager->getNetwork()->getPanelCount();

    XtVaGetValues(widget, 
		  XmNset, &set,
		  XmNuserData, &cp, 
		  NULL);

    set ? cp->manage() : cp->unmanage(); 

    cpCount2 = pad->panelManager->getNetwork()->getPanelCount();

    if (cpCount2 == 0)
         pad->unmanage();
    else if (cpCount2 < cpCount1)
         pad->makeToggles();
}

void ControlPanelAccessDialog::manage()
{
    this->makeToggles();
    this->Dialog::manage();
}

void ControlPanelAccessDialog::update()
{
    if(this->isManaged())
	this->makeToggles();
}


