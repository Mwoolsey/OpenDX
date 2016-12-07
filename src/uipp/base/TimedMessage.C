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
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "DXStrings.h"
#include "Application.h"
#include "TimedMessage.h"



TimedMessage::TimedMessage(const char *name,
			   Widget parent,
			   const char *message,
			   const char *title,
			   int         timeout) :
    TimedDialog(name, parent, timeout)
{
    this->message = DuplicateString(message);
    this->title = DuplicateString(title);
}

TimedMessage::~TimedMessage()
{
    if (this->message)
	delete[] this->message;
    if (this->title)
	delete[] this->title;
    if(theApplication->getLogoPixmap(FALSE) != XtUnspecifiedPixmap)
	theApplication->cleanupLogo();
}

Widget TimedMessage::createDialog(Widget parent)
{
    Widget dialog;
    Widget label;

    //
    // Create the information dialog.  Realize it due to a bug under SGI (and
    // maybe elsewhere).
    //
    dialog = XmCreateFormDialog(parent, this->name, NUL(ArgList), 0);
    XtRealizeWidget(dialog);

    //
    // Set the ok button to be the default button.
    //
    this->ok = 
	XtVaCreateManagedWidget(
		"OK", xmPushButtonWidgetClass, dialog,
		XmNshowAsDefault, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 5,
		XmNalignment, XmALIGNMENT_CENTER,
		NULL);

    //
    // Use the Logo as the button
    //
    if (theApplication->getLogoPixmap(FALSE) != XtUnspecifiedPixmap) {
	XtVaSetValues(this->ok, 
		XmNlabelPixmap, theApplication->getLogoPixmap(FALSE),
		XmNlabelType, XmPIXMAP,
		NULL);
    } else {
	XmString xmstr = XmStringCreate("OK", XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(this->ok, XmNlabelString,xmstr, NULL);
        XmStringFree(xmstr);
    }
	
    XmString messageString = 
	XmStringCreateLtoR(this->message, XmSTRING_DEFAULT_CHARSET);

    //
    // Create the label 
    //
    label = 
	XtVaCreateManagedWidget(
		"label", xmLabelWidgetClass, dialog,
		XmNalignment, XmALIGNMENT_CENTER,
		XmNlabelString, messageString,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 5, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->ok,
		XmNtopOffset, 5,
		NULL);

    XmString titleString = 
	XmStringCreate(this->title, XmSTRING_DEFAULT_CHARSET);

    XtVaSetValues(dialog,
	XmNdefaultButton, this->ok,
	XmNdialogTitle,   titleString,
	NULL);
    
    XmStringFree(messageString);
    XmStringFree(titleString);

    //
    // Return the created dialog.
    //
    Dimension b_width, l_width;
    XtVaGetValues(label, XmNwidth, &l_width, NULL);
    XtVaGetValues(this->ok, XmNwidth, &b_width, NULL);

    int offset;
    if(l_width > b_width)
    {
	offset = (l_width-b_width)>>1;
	XtVaSetValues(this->ok, 
	    XmNleftAttachment, XmATTACH_NONE, 
	    XmNx, offset+5, 
	    NULL);
	XtVaSetValues(label, 
	    XmNleftAttachment, XmATTACH_FORM, 
	    XmNleftOffset, 5,
	    XmNrightAttachment, XmATTACH_FORM, 
	    XmNrightOffset, 5,
	    NULL);
    }
    else
    {
        offset = (b_width-l_width)>>1;
	XtVaSetValues(label, 
	    XmNleftAttachment, XmATTACH_NONE, 
	    XmNx, offset, 
	    NULL);
	XtVaSetValues(this->ok, 
	    XmNleftAttachment, XmATTACH_FORM, 
	    XmNrightAttachment, XmATTACH_FORM, 
	    NULL);
    }
    
    return dialog;
}
