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

#include <limits.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "../base/DXStrings.h"
#include "SearchDialog.h"
#include "Browser.h"

boolean SearchDialog::ClassInitialized = FALSE;

String  SearchDialog::DefaultResources[] = {
	"*mainForm*closePB.labelString:		Close",
	"*mainForm*nextPB.labelString:		Find Next",
	"*mainForm*prevPB.labelString:		Find Previous",
	"*mainForm*textLabel.labelString:	Search for:",
        NULL
};


Widget SearchDialog::createDialog(Widget parent)
{
    Widget shell;

    shell = XmCreateDialogShell(parent, this->name, NULL, 0);
    XtVaSetValues(shell, 
		  XmNtitle, 	  "Browser Search...",
		  NULL);

    this->mainform = 
	XtVaCreateManagedWidget("mainForm",
		xmFormWidgetClass, 	shell,
		XmNautoUnmanage,        FALSE,
		XmNnoResize,		TRUE,
		XmNwidth,		400,
		XmNheight,		80,
		NULL);

    this->text_label = 
	XtVaCreateManagedWidget("textLabel",
		xmLabelWidgetClass, 	this->mainform,
		XmNleftAttachment,   	XmATTACH_FORM,
		XmNleftOffset,       	10,
		XmNtopAttachment,  	XmATTACH_FORM,
		XmNtopOffset,      	12,
		NULL);

    this->text= 
	XtVaCreateManagedWidget("text",
		xmTextWidgetClass, 	this->mainform,
		XmNleftAttachment,   	XmATTACH_WIDGET,
		XmNleftWidget,		this->text_label,
		XmNleftOffset,       	10,
		XmNrightAttachment,   	XmATTACH_FORM,
		XmNrightOffset,       	10,
		XmNtopAttachment,  	XmATTACH_FORM,
		XmNtopOffset,      	10,
		NULL);

    this->next_pb = 
	XtVaCreateManagedWidget("nextPB",
		xmPushButtonWidgetClass, this->mainform,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	5,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	31,
		XmNtopAttachment,  	XmATTACH_WIDGET,
		XmNtopWidget,		this->text,
		XmNtopOffset,      	10,
		XmNbottomAttachment,  	XmATTACH_FORM,
		XmNbottomOffset,      	10,
		NULL);

    this->prev_pb = XtVaCreateManagedWidget("prevPB",
		xmPushButtonWidgetClass, this->mainform,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	36,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	63,
		XmNbottomAttachment,  	XmATTACH_FORM,
		XmNbottomOffset,      	10,
		NULL);

    this->close_pb = 
	XtVaCreateManagedWidget("closePB",
		xmPushButtonWidgetClass, this->mainform,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	68,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	95,
		XmNbottomAttachment,  	XmATTACH_FORM,
		XmNbottomOffset,      	10,
		NULL);

    XtAddCallback(this->next_pb, 
		  XmNactivateCallback, 
		  (XtCallbackProc)SearchDialog_NextCB, 
		  (XtPointer)this);
    XtAddCallback(this->prev_pb, 
		  XmNactivateCallback, 
		  (XtCallbackProc)SearchDialog_PrevCB, 
		  (XtPointer)this);
    XtAddCallback(this->close_pb, 
		  XmNactivateCallback, 
		  (XtCallbackProc)SearchDialog_CloseCB, 
		  (XtPointer)this);
    XtAddCallback(this->text, 
		  XmNactivateCallback, 
		  (XtCallbackProc)SearchDialog_TextCB, 
		  (XtPointer)this);
    

    return shell;
}



SearchDialog::SearchDialog(Widget parent, Browser *browser) 
                       		: Dialog("browserSD", parent)
{
     
    this->browser = browser;
    this->last_search = FORWARD;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT SearchDialog::ClassInitialized)
    {
        SearchDialog::ClassInitialized = TRUE;
	this->installDefaultResources(parent);
    }
}

SearchDialog::~SearchDialog()
{

}

//
// Install the default resources for this class.
//
void SearchDialog::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, SearchDialog::DefaultResources);
    this->Dialog::installDefaultResources(baseWidget);
}

extern "C" void SearchDialog_NextCB(Widget, XtPointer client, XtPointer)
{
    SearchDialog *searchDialog = (SearchDialog*)client;

    char *text = XmTextGetString(searchDialog->text);
    if(STRLEN(text) == 0) return;
    searchDialog->browser->searchForward(text);
    searchDialog->last_search = SearchDialog::FORWARD;
}
extern "C" void SearchDialog_PrevCB(Widget, XtPointer client, XtPointer)
{
    SearchDialog *searchDialog = (SearchDialog*)client;

    char *text = XmTextGetString(searchDialog->text);
    if(STRLEN(text) == 0) return;
    searchDialog->browser->searchBackward(text);
    searchDialog->last_search = SearchDialog::BACKWARD;
}
extern "C" void SearchDialog_CloseCB(Widget, XtPointer client, XtPointer)
{
    SearchDialog *searchDialog = (SearchDialog*)client;

    searchDialog->unmanage();
}
extern "C" void SearchDialog_TextCB(Widget, XtPointer client, XtPointer)
{
    SearchDialog *searchDialog = (SearchDialog*)client;

    char *text = XmTextGetString(searchDialog->text);
    if(STRLEN(text) == 0) return;
    if(searchDialog->last_search == SearchDialog::FORWARD)
	searchDialog->browser->searchForward(text);
    else
	searchDialog->browser->searchBackward(text);
}

void SearchDialog::mapRaise()
{

        XMapRaised(XtDisplay(this->getRootWidget()), 
                   XtWindow(this->getRootWidget()));
	XtManageChild(this->mainform);

}
