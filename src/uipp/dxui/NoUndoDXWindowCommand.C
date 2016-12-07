/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include "NoUndoDXWindowCommand.h"
#include "ToggleButtonInterface.h"
#include "DXWindow.h"

NoUndoDXWindowCommand::NoUndoDXWindowCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       DXWindow  *window,
				       DXWindowCommandType comType ) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->dxWindow = window;
}


boolean NoUndoDXWindowCommand::doIt(CommandInterface *ci)
{
    DXWindow *window = this->dxWindow;

    ASSERT(window);

    switch (this->commandType) {
	case NoUndoDXWindowCommand::ToggleWindowStartup:
	    window->toggleWindowStartup();
	    if (window->isStartup())
		this->notifyClients(ToggleButtonInterface::MsgToggleOn);
	    else
		this->notifyClients(ToggleButtonInterface::MsgToggleOff);
	    break;
	default:
	    ASSERT(0);
    }

    return TRUE;
}


