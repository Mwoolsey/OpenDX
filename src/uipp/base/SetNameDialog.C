/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "Xm/Form.h"
#include "Xm/Label.h"
#include "Xm/PushB.h"
#include "Xm/Separator.h"
#include "Xm/Text.h"

#include "DXStrings.h"
#include "SetNameDialog.h"

#include "Application.h"
//#include "Network.h"
//#include "ErrorDialogManager.h"

//
// Since this is an abstract class, THIS class never installs these
// resources.  It is up to the derived classes to install them.
//
String SetNameDialog::DefaultResources[] =
{
    "*nameLabel.labelString:            Name:",
    "*nameLabel.foreground:             SteelBlue",
    "*name.width:			230",
    NULL
};


SetNameDialog::SetNameDialog(const char *name, Widget parent, 
			boolean has_apply ) :
    				TextEditDialog((char*)name, parent, FALSE)
{
    this->hasApply = has_apply;
}

SetNameDialog::~SetNameDialog()
{
}

#if 00 
//
// If this were not an abstract class, then these initialization should
// be done in the public constructor (that does not take a name).
//
boolean SetNameDialog::ClassInitialized = FALSE;
SetNameDialog::SetNameDialog(Widget parent, boolean has_apply ) :
  		TextEditDialog((char*)"setNameDialog", parent, FALSE)
{
    if (NOT SetNameDialog::ClassInitialized)
    {
        SetNameDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
#endif

void SetNameDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetNameDialog::DefaultResources);
    this->TextEditDialog::installDefaultResources( baseWidget);
}

Widget SetNameDialog::createDialog(Widget parent)
{
    Arg arg[10];
    int n = 0;

    XtSetArg(arg[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    XtSetArg(arg[n], XmNautoUnmanage, False); n++;

//    Widget dialog = XmCreateFormDialog(parent, this->name, arg, n);
    Widget dialog = this->CreateMainForm(parent, this->name, arg, n);

    Widget nameLabel = XtVaCreateManagedWidget(
	"nameLabel", xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
#if 0
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
#else
        XmNleftOffset      , 10,
#endif
	NULL);

    this->editorText = XtVaCreateManagedWidget(
	"name", xmTextWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget	   , nameLabel,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset	   , 10,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
	NULL);

    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->editorText,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 2,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 2,
	NULL);

    this->ok = XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 10,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNbottomOffset    , 5,
#ifdef sun4
        XmNheight,           35,
        XmNwidth,            80,
#endif
        XmNshowAsDefault,    2,
	NULL);

     if (this->hasApply) {
         Widget apply = XtVaCreateManagedWidget("applyButton",
                    xmPushButtonWidgetClass, dialog,
        	    XmNtopAttachment,     XmATTACH_WIDGET,
		    XmNtopWidget, 	  separator,
		    XmNtopOffset, 	  17,
                    XmNleftAttachment,    XmATTACH_WIDGET,
                    XmNleftWidget,        this->ok,
                    XmNleftOffset,        10,
#if 00
                    XmNbottomAttachment,  XmATTACH_FORM,
                    XmNbottomOffset,      5,
#endif
                    NULL);

        XtAddCallback(apply,XmNactivateCallback,
                       (XtCallbackProc)TextEditDialog_ApplyCB, (XtPointer)this);
    } 


    this->cancel = XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 17,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
//        XmNbottomAttachment, XmATTACH_FORM,
//        XmNbottomOffset    , 5,
	NULL);

    XtAddCallback(this->editorText,
                  XmNactivateCallback,
                  (XtCallbackProc)SetNameDialog_TextCB,
                  (XtPointer)this);
#if (XmVersion < 1002)
    XtAddEventHandler(dialog,
                  EnterWindowMask,
                  False,
                  (XtEventHandler)SetNameDialog_FocusEH,
                  (XtPointer)this);
#endif

//    XtVaSetValues(dialog, XmNdefaultButton, this->ok, NULL);

    return dialog;
}

void SetNameDialog::manage()
{
    this->TextEditDialog::manage();
    XmProcessTraversal(this->editorText, XmTRAVERSE_CURRENT);
    XmProcessTraversal(this->editorText, XmTRAVERSE_CURRENT);
}

extern "C" void SetNameDialog_FocusEH(Widget widget,
                             	 XtPointer clientData,
                             	 XEvent* event,
				 Boolean*)
{
    SetNameDialog* d = (SetNameDialog*)clientData;
 
    XSetInputFocus(XtDisplay(d->editorText),
                   XtWindow(d->editorText),
                   RevertToPointerRoot,
                   CurrentTime);
}

extern "C" void SetNameDialog_TextCB(Widget widget,
                  		 XtPointer clientData,
                                 XtPointer)
{
    SetNameDialog* d = (SetNameDialog*)clientData;

    XtCallCallbacks(d->ok, XmNactivateCallback, NULL);
}

