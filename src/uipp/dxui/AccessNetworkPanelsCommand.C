/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "AccessNetworkPanelsCommand.h"
#include "Network.h"
#include "CommandInterface.h"

AccessNetworkPanelsCommand::
    AccessNetworkPanelsCommand(const char*   name,
				CommandScope* scope,
				boolean       active,
				Network*      network,
				AccessPanelType	how) :
	NoUndoCommand(name, scope, active)
{
    ASSERT(network);
    //
    // Save the associated network.
    //
    this->network 	= network;
    this->accessType	= how;
}


boolean AccessNetworkPanelsCommand::doIt(CommandInterface *ci)
{
    Network *network = this->network;
   

    switch (this->accessType) {
	case AccessNetworkPanelsCommand::OpenAllPanels:
	    network->openControlPanel(0);
	    break;
	case AccessNetworkPanelsCommand::CloseAllPanels:
	    network->closeControlPanel(0);
	    break;
	case AccessNetworkPanelsCommand::OpenPanelByInstance:
	    if (ci)
		network->openControlPanel((int)(long)ci->getLocalData());
	    break;
	case AccessNetworkPanelsCommand::OpenPanelGroupByIndex:
	    if (ci)
		network->openControlPanelGroup((int)(long)ci->getLocalData());
	    break;
    }

    //
    // Return the result.
    //
    return TRUE;
}


