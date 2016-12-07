/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "MainWindow.h"
#include "CloseWindowCommand.h"


CloseWindowCommand::CloseWindowCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       MainWindow*   window):
	NoUndoCommand(name, scope, active)
{
    ASSERT(window);

    //
    // Save the associated window.
    //
    this->window = window;
}


boolean CloseWindowCommand::doIt(CommandInterface *ci)
{
    this->window->closeWindow();

    return TRUE;
}


