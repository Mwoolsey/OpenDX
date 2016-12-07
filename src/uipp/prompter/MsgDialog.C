/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include "../base/Application.h"
#include "MsgDialog.h"
#include "ListIterator.h"

boolean MsgDialog::ClassInitialized = FALSE;

String MsgDialog::DefaultResources[] =
{
    "*dialogTitle:     		Messages...",
    "*nameLabel.labelString:	Header Msg:",
    NULL
};

MsgDialog::MsgDialog(Widget parent, TypeChoice *choice) : Dialog("msgDialog",parent)
{
    this->test_output = NUL(Widget);
    this->choice = choice;
    this->item_list = NUL(List*);

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT MsgDialog::ClassInitialized)
    {
	ASSERT(theApplication);
        MsgDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

MsgDialog::~MsgDialog()
{
    if (this->item_list) {
	ListIterator it(*this->item_list);
	XmString xmstr;
	while (xmstr = (XmString)it.getNext())
	    XmStringFree(xmstr);
	delete this->item_list;
    }
}

//
// Install the default resources for this class.
//
void MsgDialog::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, MsgDialog::DefaultResources);
    this->Dialog::installDefaultResources(baseWidget);
}



//
// L A Y O U T   T H E   D I A L O G       L A Y O U T   T H E   D I A L O G
// L A Y O U T   T H E   D I A L O G       L A Y O U T   T H E   D I A L O G
//
Widget MsgDialog::createDialog(Widget parent)
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

    this->ok = XtVaCreateManagedWidget("Clear",
	xmPushButtonWidgetClass, buttonform,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
        XmNleftAttachment,	XmATTACH_FORM,
        XmNleftOffset,		10,
	XmNwidth,		70,
    NULL);

    this->cancel = XtVaCreateManagedWidget("Close",
	xmPushButtonWidgetClass, buttonform,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
        XmNrightAttachment,	XmATTACH_FORM,
        XmNrightOffset,		10,
	XmNwidth,		70,
    NULL);

    Widget mainform = XtVaCreateManagedWidget("mainForm",
	xmFormWidgetClass, 	toplevelform,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment, 	XmATTACH_WIDGET, 
	XmNbottomWidget, 	buttonform,
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

    return toplevelform; 
}


	

void MsgDialog::layoutChooser(Widget parent)
{
Arg args[10];

    //
    // A list containing filenames of referenced macros.
    //
    Widget sw_frame = XtVaCreateManagedWidget ("sw_frame",
	xmFrameWidgetClass,	parent,
    	XmNtopAttachment, XmATTACH_FORM,
    	XmNleftAttachment, XmATTACH_FORM,
    	XmNrightAttachment, XmATTACH_FORM,
    	XmNbottomAttachment, XmATTACH_FORM,
    	XmNtopOffset, 4,
    	XmNleftOffset, 4,
    	XmNrightOffset, 4,
    	XmNbottomOffset, 6,
	XmNmarginWidth,   3,
	XmNmarginHeight,   3,
	XmNshadowThickness, 2,
    NULL);


    int n = 0;
    XtSetArg (args[n], XmNheight, 200); n++;
    XtSetArg (args[n], XmNwidth, 500); n++;
    XtSetArg (args[n], XmNlistSizePolicy, XmCONSTANT); n++;
    this->test_output = XmCreateScrolledList (sw_frame, "chooser", args, n);
    XtManageChild (this->test_output);

    if (this->item_list) {
	ListIterator it(*this->item_list);
	XmString xmstr;
	while (xmstr = (XmString)it.getNext()) {
	    this->addItem (xmstr, 0);
	    XmStringFree(xmstr);
	}
	delete this->item_list;
	this->item_list = NUL(List*);
    }

    //
    // The list wants a new size when add items using multiple fonts.  This
    // setting makes clear and append operations leave the size alone.
    //
    XtVaSetValues (parent, XmNresizePolicy, XmRESIZE_NONE, NULL);
}

//
// Attached to the Clear button
//
boolean MsgDialog::okCallback(Dialog *)
{
    if (!this->test_output) return TRUE;

    XmListDeleteAllItems (this->test_output);

    //
    // returning FALSE means don't close the window.
    //
    return FALSE;
}

void MsgDialog::addItem (XmString xmstr, int pos)
{
    if (this->test_output) {
	XmListAddItemUnselected (this->test_output, xmstr, pos);
	XmListSetBottomPos(this->test_output, 0);
	return ;
    }

    //
    // These items will be added to the list when the list is created.
    //
    if (!this->item_list) this->item_list = new List;
    XmString newstr = XmStringCopy(xmstr);
    this->item_list->appendElement((void*)newstr);
}

int MsgDialog::getMessageCount()
{
    if (!this->test_output)
	if (this->item_list) return this->item_list->getSize();
	else return 0;

    int count;
    XtVaGetValues (this->test_output, XmNitemCount, &count, NULL);
    return count;
}
