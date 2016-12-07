/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <Xm/Xm.h>
#include <Xm/PushB.h>

#include "ButtonInterface.h"


ButtonInterface::ButtonInterface(Widget   parent,
				 char*    name,
				 Command* command,
				 const char *bubbleHelp):
	CommandInterface(name, command)
{
    ASSERT(parent);

    //
    // Create the button widget.
    //
    this->setRootWidget(
	XtVaCreateManagedWidget
	    (this->name,
	     xmPushButtonWidgetClass,
	     parent,
	     NULL));


    //
    // If, for what ever reason, this interface is not active at this
    // point, deactivate it (BY CONVENTION).
    //
    if (NOT this->active)
    {
	this->deactivate();
    }

    //
    // Register the ExecuteCommandCallback() to be called when the
    // button is "pressed."
    //
    XtAddCallback
	(this->getRootWidget(),
	 XmNactivateCallback,
	 (XtCallbackProc)CommandInterface_ExecuteCommandCB, 
	 (XtPointer)this);

    if (bubbleHelp) {
	this->setBubbleHelp (bubbleHelp, this->getRootWidget());
    }
}

void ButtonInterface::setLabel(const char* string)
{
    Arg	     arg[1];
    XmString name;

    name = XmStringCreateSimple((char*)string);
    XtSetArg(arg[0],XmNlabelString, name);
    XtSetValues(this->getRootWidget(), arg, 1);

    XmStringFree(name);

}
