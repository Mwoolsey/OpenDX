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
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>

#include "ConfirmedQuitCommand.h"
#include "DXApplication.h"
#include "DXWindow.h"
#include "Network.h"
#include "QuitCommand.h"

ConfirmedQuitCommand::ConfirmedQuitCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
			 DXApplication *app) :
    OptionalPreActionCommand(name, scope, active,
                             "Save Confirmation",
                             "Do you want to save the program?")
{
    this->application = app;

    //
    // We create this later, because creation uses a virtual Application
    // method.  If this class gets instanced in the Application's constructor
    // (typical) then the wrong virtual method will get called. 
    //
    this->command = NULL;	
}
ConfirmedQuitCommand::~ConfirmedQuitCommand()
{
    if (this->command)
	delete this->command;

}

void   ConfirmedQuitCommand::doPreAction()
{
    Network *net = theDXApplication->network;
    const char    *fname = net->getFileName();

    if(fname)
    {
        if(net->saveNetwork(fname))
            this->doIt(NULL);
    }
    else
        net->postSaveAsDialog(theDXApplication->getAnchor()->getRootWidget(), 
					this);
}

boolean ConfirmedQuitCommand::doIt(CommandInterface *ci)
{
    // 
    // If the application doesn't allow confirmation, then don't do it.
    // (Yes, another, possibly better, way to do this would be to have
    //  DXApplication allocate an (non-existent yet) UnconfirmedQuitCommand 
    // instead of a ConfirmedQuitCommand)
    // 
    if (!theDXApplication->appAllowsConfirmedQuit()) {
	theDXApplication->shutdownApplication();
	return TRUE;
    }


    // 
    // We can use a static buffer since there is only one application.
    // 
    if (!this->command) { 

	static char *dialogQuestion = NULL;
	if (!dialogQuestion) {
	    dialogQuestion = new char[256];
	    sprintf(dialogQuestion,"Do you really want to quit %s?",
				theApplication->getInformalName());
	}

	CommandScope *scope = (CommandScope*)this->scopeList.getElement(1);
	this->command = 
	    new QuitCommand
		(this->application,
		 "quit",
		 scope,
		 TRUE,
		 "Quit",
		 dialogQuestion);
    }

    this->command->execute(ci);

    return TRUE;
}

boolean ConfirmedQuitCommand::needsConfirmation()
{
    DXApplication *ap = this->application;

    return ap->network->saveToFileRequired();
}

