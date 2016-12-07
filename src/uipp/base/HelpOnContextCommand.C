/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <X11/cursorfont.h>
#include <Xm/Xm.h>

#include "HelpOnContextCommand.h"
#include "Application.h"
#include "MainWindow.h"


boolean HelpOnContextCommand::HelpOnContextCommandClassInitialized = FALSE;
Cursor  HelpOnContextCommand::HelpCursor = NUL(Cursor);


HelpOnContextCommand::HelpOnContextCommand(const char*   name,
					   CommandScope* scope,
					   boolean       active,
					   MainWindow*   window) :
	NoUndoCommand(name, scope, active)
{
    ASSERT(window);

    if (NOT HelpOnContextCommand::HelpOnContextCommandClassInitialized)
    {
	ASSERT(theApplication);
	HelpOnContextCommand::HelpCursor =
	    XCreateFontCursor(theApplication->getDisplay(), XC_question_arrow);

	HelpOnContextCommand::HelpOnContextCommandClassInitialized = TRUE;
    }

    this->window = window;
}


boolean HelpOnContextCommand::doIt(CommandInterface *ci)
{
    Widget                       widget;
    MainWindowHelpCallbackStruct callData;

    widget =
	XmTrackingLocate
	    (this->window->getMainWindow(), 
	     HelpOnContextCommand::HelpCursor,
	     False);

    XSync(theApplication->getDisplay(), False);

    while (widget)
    {
	if (XtHasCallbacks(widget, XmNhelpCallback) == XtCallbackHasSome)
	{
	    callData.reason = DxCR_HELP;
	    callData.event  = NULL;
	    callData.widget = widget;
	    XtCallCallbacks
		(widget,
		 XmNhelpCallback,
		 (XtPointer)&callData);
	    break;
	}
	else
	{
	    widget = XtParent(widget);
	}
    }

    return TRUE;
}


