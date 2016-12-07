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
#include <Xm/MainW.h>

#include "CommandInterface.h"
#include "Command.h"
#include "Application.h"


extern "C" void CommandInterface_ExecuteCommandCB(Widget,
					      XtPointer clientData,
					      XtPointer)
{
    ASSERT(clientData);
    CommandInterface* DXinterface = (CommandInterface*)clientData;

    DXinterface->executeCommand();
}



CommandInterface::CommandInterface(char*    name,
				   Command* command): UIComponent(name)
{
    ASSERT(command);

    this->command = command;

    if (command != NUL(Command*))
    {
	command->registerClient(this);
    }
}


CommandInterface::~CommandInterface()
{
    command->unregisterClient(this);
}


inline void CommandInterface::setCommand(Command* command)
{
    this->command = command;
}


void CommandInterface::notify(const Symbol message, const void *data, const char *msg)
{
    if (message == Command::MsgActivate)
    {
	this->activate();
    }
    else if (message == Command::MsgDeactivate)
    {
	this->deactivate(msg);
    }
    else if (message == Command::MsgDisassociate)
    {
	this->command = NUL(Command*);
    }
}


Widget CommandInterface::getDialogParent()
{
    Widget w = this->getRootWidget();

    while (w && !XmIsMainWindow(w))
	w = XtParent(w);

    if (!w)
	w = theApplication->getRootWidget();

    return w;
}

void CommandInterface::executeCommand()
{
    if (this->command != NUL(Command*))
    {
	theApplication->startCommandInterfaceExecution();
	this->command->execute(this);
	theApplication->endCommandInterfaceExecution();
    }
}
