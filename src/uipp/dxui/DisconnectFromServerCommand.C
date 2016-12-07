/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "DisconnectFromServerCommand.h"
#include "DXApplication.h"


DisconnectFromServerCommand::DisconnectFromServerCommand(const char*   name,
                CommandScope* scope,
                boolean       active):
     ConfirmedCommand(name, scope, active,
		      "Disconnect from Server...",
		      "Do you really want to disconnect from the server?")
{
}

DisconnectFromServerCommand::~DisconnectFromServerCommand()
{
}

boolean DisconnectFromServerCommand::doIt(CommandInterface *ci)
{
    return theDXApplication->disconnectFromServer();
}
