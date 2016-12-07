/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "NoOpCommand.h"


NoOpCommand::NoOpCommand(const char*   name,
			 CommandScope* scope,
			 boolean       active) : Command(name, scope, active)
{
    //
    // No op.
    //
}

boolean NoOpCommand::doIt(CommandInterface *ci)
{
    return TRUE;
}

boolean NoOpCommand::undoIt()
{
    return TRUE;
}
