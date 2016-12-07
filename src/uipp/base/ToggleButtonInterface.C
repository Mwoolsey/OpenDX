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
#include <Xm/ToggleB.h>

#include "ToggleButtonInterface.h"


boolean ToggleButtonInterface::ToggleButtonInterfaceClassInitialized = FALSE;

Symbol ToggleButtonInterface::MsgToggleOn    = NUL(Symbol);
Symbol ToggleButtonInterface::MsgToggleOff   = NUL(Symbol);
Symbol ToggleButtonInterface::MsgToggleState = NUL(Symbol);


ToggleButtonInterface::ToggleButtonInterface(Widget   parent,
					     char*    name,
					     Command* command,
					     boolean  state,
					     const char *bubbleHelp):
	CommandInterface(name, command)
{
    ASSERT(parent);

    //
    // Perform class initialization, if necessary.
    //
    if (NOT ToggleButtonInterface::ToggleButtonInterfaceClassInitialized)
    {
	ASSERT(theSymbolManager);

	ToggleButtonInterface::MsgToggleOn =
	    theSymbolManager->registerSymbol("ToggleOn");
	ToggleButtonInterface::MsgToggleOff =
	    theSymbolManager->registerSymbol("ToggleOff");
	ToggleButtonInterface::MsgToggleState =
	    theSymbolManager->registerSymbol("ToggleState");

	ToggleButtonInterface::ToggleButtonInterfaceClassInitialized = TRUE;
    }

    //
    // Remember the current state.
    //
    this->state = state;

    //
    // Create the button widget.
    //
    this->setRootWidget(
	XtVaCreateManagedWidget
	    (this->name,
	     xmToggleButtonWidgetClass,
	     parent,
	     XmNset,            this->state,
	     XmNvisibleWhenOff, TRUE,
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
	 XmNvalueChangedCallback,
	 (XtCallbackProc)ToggleButtonInterface_ExecuteCommandCB,
	 (XtPointer)this);

     if (bubbleHelp) {
	 this->setBubbleHelp (bubbleHelp, this->getRootWidget());
     }

}


void ToggleButtonInterface::notify(const Symbol message, 
	const void *data, const char *msg)
{
    if (message == ToggleButtonInterface::MsgToggleOn)
    {
	this->setState(TRUE);
    }
    else if (message == ToggleButtonInterface::MsgToggleOff)
    {
	this->setState(FALSE);
    }
    else if (message == ToggleButtonInterface::MsgToggleState)
    {
	this->toggleState();
    }
    else
    {
	this->CommandInterface::notify(message,data,msg);
    }
}


void ToggleButtonInterface::toggleState()
{
    this->state = NOT this->state;
    if (this->getRootWidget())
    {
	XtVaSetValues(this->getRootWidget(), XmNset, this->state, NULL);
    }
}


void ToggleButtonInterface::setState(const boolean state, const boolean notify)
{
    this->state = state;

    if (notify && this->getRootWidget())
    {
	XtVaSetValues(this->getRootWidget(), XmNset, this->state, NULL);
    }
}
boolean ToggleButtonInterface::getState()
{
Boolean set;

    if (this->getRootWidget())
    {
	XtVaGetValues(this->getRootWidget(), XmNset, &set, NULL);
    }

    return (boolean)set;
}

extern "C" void ToggleButtonInterface_ExecuteCommandCB(Widget widget,
					      XtPointer clientData,
					      XtPointer )
{
    ASSERT(clientData);
    ToggleButtonInterface* DXinterface = (ToggleButtonInterface*)clientData;

    DXinterface->tbExecuteCommand();
}
void ToggleButtonInterface::tbExecuteCommand()
{
    this->executeCommand();
}
