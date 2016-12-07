/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoJavaNetCommand.h"
#include "JavaNet.h"

NoUndoJavaNetCommand::NoUndoJavaNetCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       JavaNet	*net,
				       JavaNetCommandType comType ) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->network = net;
}


boolean NoUndoJavaNetCommand::doIt(CommandInterface *ci)
{
    JavaNet *net = this->network;

    ASSERT(net);

    switch (this->commandType) {
	case NoUndoJavaNetCommand::SaveBean:
	    net->saveBean();
	    break;
	case NoUndoJavaNetCommand::SaveApplet:
	    net->saveApplet();
	    break;
	case NoUndoJavaNetCommand::SaveWebPage:
            net->saveWebPage();
	    break;
	default:
	    ASSERT(0);
    }

    return TRUE;
}


