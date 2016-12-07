/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>


#include "Application.h"
#include "TextPopup.h"

boolean TextPopup::ClassInitialized = FALSE;
String TextPopup::DefaultResources[] =
{
        ".popupButton.labelString: ...",
        ".popupButton.recomputeSize: True",
        NULL
};


void TextPopup::initInstanceData()
{
    this->popupMenu = NULL;
    this->popupButton = NULL;
    this->textField = NULL;
    this->setTextCallback = NULL;
    this->changeTextCallback = NULL;
    this->callbackData = NULL;
    this->callbacksEnabled = FALSE;

}
TextPopup::TextPopup() : UIComponent("textPopup")
{
   this->initInstanceData();
}
TextPopup::TextPopup(const char *name) : UIComponent(name)
{
   this->initInstanceData();
}
TextPopup::~TextPopup()
{
}
void TextPopup::initialize()
{
    if (!TextPopup::ClassInitialized) {
	TextPopup::ClassInitialized = TRUE;
        this->setDefaultResources(theApplication->getRootWidget(),
                                  TextPopup::DefaultResources);

    }
}


//
// The event handler needs to work with a timer proc so that the pane
// can be either spring loaded or sticky.  It's only sticky if its popped up
// from a ButtonRelease event.  But you must be able to popup from a ButtonPress
// event so that the person isn't required to click extra times.
//
extern "C" void TextPopup_ButtonEH(Widget w, XtPointer client,
                                        XEvent *e, Boolean *ctd)
{
    TextPopup *dialog = (TextPopup *)client;
    XmMenuPosition(dialog->popupMenu , (XButtonPressedEvent *)e);

    if (e->type == ButtonRelease) XtManageChild(dialog->popupMenu);
    else XtAppAddTimeOut (theApplication->getApplicationContext(), 100,
	(XtTimerCallbackProc)XtManageChild, (XtPointer)dialog->popupMenu);
}
extern "C" void TextPopup_ItemSelectCB(Widget    widget,
                            XtPointer clientData,
                            XtPointer callData)
{
    ASSERT(clientData);
    TextPopup *dialog = (TextPopup*) clientData;
    dialog->itemSelectCallback(XtName(widget));
}
extern "C" void TextPopup_ActivateTextCB(Widget    widget,
                            XtPointer clientData,
                            XtPointer callData)
{
    ASSERT(clientData);
    TextPopup *dialog = (TextPopup*) clientData;
    ASSERT(dialog->callbacksEnabled);
    char *v = XmTextGetString(widget);
    dialog->activateTextCallback(v, dialog->callbackData);
    XtFree(v);
}
extern "C" void TextPopup_ValueChangedTextCB(Widget    widget,
                            XtPointer clientData,
                            XtPointer callData)
{
    ASSERT(clientData);
    TextPopup *dialog = (TextPopup*) clientData;
    ASSERT(dialog->callbacksEnabled);
    char *v = XmTextGetString(widget);
    dialog->valueChangedTextCallback(v, dialog->callbackData);
    XtFree(v);
}
Widget TextPopup::createTextPopup(Widget parent, const char **items, int nitems,
				SetTextCallback stc,
				ChangeTextCallback ctc,
				void *callData)
{
    ASSERT(!this->getRootWidget());
    this->initialize();

    this->setTextCallback = stc;
    this->changeTextCallback = ctc;
    this->callbackData = callData;

    Widget form = XtVaCreateWidget(this->name,xmFormWidgetClass, parent, NULL);
    this->setRootWidget(form);


    this->textField = XtVaCreateManagedWidget("textField",
        xmTextWidgetClass,form,
	XmNleftAttachment, 	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
        NULL);
	


    Widget button = XtVaCreateManagedWidget("popupButton",
        xmPushButtonWidgetClass,form,
#if 00
        XmNleftAttachment,      XmATTACH_WIDGET,
        XmNleftWidget,          this->textField,
	XmNleftOffset,		10,
#else
        XmNrightAttachment,     XmATTACH_FORM,
	XmNrightOffset,		0,
#endif
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		3,		// Center on text widget
        NULL);
    this->popupButton = button;

    XtUninstallTranslations(button);

#if 11
    XtVaSetValues(this->textField, 
        XmNrightAttachment,      XmATTACH_WIDGET,
        XmNrightWidget,          button,
        XmNrightOffset,          10,
	NULL);
#endif
    //
    // Get rid of any existing tranlstions on the label
    //
    if (nitems > 0) {
	//
	// Disable motif's attempts to pop the menu pane up for us because
	// if we disable the button later on, them motif will do a grab and
	// not pop up the pane, which hoses your session.  Perhaps there is
	// a better way to do this disabling?
	//
	Arg args[2];
	XtSetArg (args[0], XmNmenuPost, "Shift<Btn4Down>");
	this->popupMenu = XmCreatePopupMenu(form, "popupMenu", args, 1);
	int i;
	for(i = 0; i < nitems; i++)
	{
	    XmString xmstring = XmStringCreateSimple((char*)items[i]);

	    Widget wid = XtVaCreateManagedWidget
		   (items[i],
		    xmPushButtonWidgetClass,this->popupMenu,
		    XmNrecomputeSize,       True,
		    XmNlabelString,         xmstring,
		    NULL);
	    XmStringFree(xmstring);

	    XtAddCallback(wid,
		  XmNactivateCallback,
		  (XtCallbackProc)TextPopup_ItemSelectCB,
		  (XtPointer)this);
	}

	XtAddEventHandler(button, ButtonPressMask|ButtonReleaseMask, False,
	    (XtEventHandler)TextPopup_ButtonEH, (XtPointer)this);
    } else {
	XtSetSensitive(button,False);
    }

    this->enableCallbacks();
    this->manage();
    return this->getRootWidget();
}

//
// De/Install callbacks for the text widget. 
//
boolean TextPopup::enableCallbacks(boolean enable)
{
    if (this->callbacksEnabled == enable) return this->callbacksEnabled;

    this->callbacksEnabled = enable;
    ASSERT(this->getRootWidget() && this->textField);

    if (!enable) {
	XtRemoveCallback(this->textField, XmNactivateCallback,
		(XtCallbackProc)TextPopup_ActivateTextCB, 
		(XtPointer)this);
	XtRemoveCallback(this->textField, XmNvalueChangedCallback,
		(XtCallbackProc)TextPopup_ValueChangedTextCB, 
		(XtPointer)this);

    } else {
	XtAddCallback(this->textField, XmNactivateCallback,
		    (XtCallbackProc)TextPopup_ActivateTextCB, 
		    (XtPointer)this);
	XtAddCallback(this->textField, XmNvalueChangedCallback,
		    (XtCallbackProc)TextPopup_ValueChangedTextCB, 
		    (XtPointer)this);
    }
    return !enable;
}

char *TextPopup::getText()
{
    return XmTextGetString(this->textField);
}
void TextPopup::setText(const char *value, boolean doActivate)
{
    XmTextSetString(this->textField, (char*)value);
    if (doActivate)
	XtCallCallbacks(this->textField,XmNactivateCallback, this);
}
void TextPopup::itemSelectCallback(const char *value)
{
    this->setText(value, TRUE);
}
void TextPopup::activateTextCallback(const char *value, void *callData)
{
    if (this->setTextCallback)
	this->setTextCallback(this, value, callData);
}
void TextPopup::valueChangedTextCallback(const char *value, void *callData)
{
    if (this->changeTextCallback)
	this->changeTextCallback(this, value, callData);
}


