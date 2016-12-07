/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>

#include "OpenCommand.h"
#include "DXApplication.h"
#include "DXWindow.h"
#include "Network.h"

OpenCommand::OpenCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
			 DXApplication *app,
			 Widget	dialogParent) :
    OptionalPreActionCommand(name, scope, active,
                             "Save Confirmation",
                             "Do you want to save the program?",
			     dialogParent)
{
    this->application = app;
}

void   OpenCommand::doPreAction()
{
    DXApplication *app = this->application;
    Network *net = app->network;
    const char    *fname = net->getFileName();

    if(fname)
    {
	if(net->saveNetwork(fname))
	     this->doIt(NULL);
    }
    else {
	Widget parent = this->dialogParent;
	if (!parent)
	    parent = app->getAnchor()->getRootWidget();
    	net->postSaveAsDialog(parent, this);
    }

}

boolean OpenCommand::doIt(CommandInterface *ci)
{
   this->application->postOpenNetworkDialog();
   return TRUE;
}

boolean OpenCommand::needsConfirmation()
{
    return this->application->network->saveToFileRequired();
}

