/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "Command.h"
#include "ListIterator.h"
#include "Client.h"
#include "Application.h"
#include "CommandScope.h"
#include "DXStrings.h"


boolean Command::CommandClassInitialized = FALSE;

Symbol Command::MsgActivate     	= NUL(Symbol);
Symbol Command::MsgDeactivate   	= NUL(Symbol);
Symbol Command::MsgDisassociate 	= NUL(Symbol);
Symbol Command::MsgBeginExecuting 	= NUL(Symbol);
Symbol Command::MsgEndExecuting 	= NUL(Symbol);


Command::Command(const char*   name,
		 CommandScope* scope,
		 boolean       active)
{
    ASSERT(name);

    //
    // Perform class initialization, if necessary.
    //
    if (NOT Command::CommandClassInitialized)
    {
	ASSERT(theSymbolManager);

	Command::MsgActivate =
	    theSymbolManager->registerSymbol("Activate");
	Command::MsgDeactivate =
	    theSymbolManager->registerSymbol("Deactivate");
	Command::MsgDisassociate =
	    theSymbolManager->registerSymbol("Disassociate");
	Command::MsgBeginExecuting =
	    theSymbolManager->registerSymbol("BeginExecuting");
	Command::MsgEndExecuting =
	    theSymbolManager->registerSymbol("EndExecuting");

	Command::CommandClassInitialized = TRUE;
    }

    //
    // Initialize member data.
    //
    this->name    = DuplicateString(name);
    this->active  = active;
    this->hasUndo = TRUE;

    if (scope != NUL(CommandScope*))
    {
	this->scopeList.appendElement(scope);
    }
}


Command::~Command()
{
    Command *c;
    List toRemoveAuto;
    int autos = this->autoActivators.getSize();

    if (autos > 0) {
	//
	// Let all the commands that auto-de/activated this command know that
 	// this command is going away.  Slurp the list items out of the
	// autoActivators list because, so that we don't have to directly
	// iterate over the autoActivators list when calling removeAutoCmd()
 	// which will change the autoActivators list.
	//
	int i;	
	Command **cmds = new Command*[autos];
	ListIterator li(this->autoActivators);
	for (i=0 ; (c=(Command*)li.getNext()) ; i++)
	    cmds[i] = c; 

	for (i=0 ; i<autos ; i++)
	    cmds[i]->removeAutoCmd(this);

	delete[] cmds;
    }

    autos = this->activateCmds.getSize();
    if (autos > 0) {
	//
	// Let all the commands that we auto de/activate know that
 	// this command is going away.  See above for algorithm.
	//
	int i;	
	Command **cmds = new Command*[autos];
	ListIterator li(this->activateCmds);
	for (i=0 ; (c=(Command*)li.getNext()) ; i++)
	    cmds[i] = c; 

	for (i=0 ; i<autos ; i++)
	    this->removeAutoCmd(cmds[i]);

	delete[] cmds;
    }
    autos = this->deactivateCmds.getSize();
    if (autos > 0) {
	//
	// Let all the commands that we auto de/activate know that
 	// this command is going away.  See above for algorithm.
	//
	int i;	
	Command **cmds = new Command*[autos];
	ListIterator li(this->deactivateCmds);
	for (i=0 ; (c=(Command*)li.getNext()) ; i++)
	    cmds[i] = c; 

	for (i=0 ; i<autos ; i++)
	    this->removeAutoCmd(cmds[i]);

	delete[] cmds;
    }

    delete[] this->name;
}


boolean Command::registerClient(Client* DXinterface)
{
    boolean result;

    //
    // Add the interface to the interface list.
    //
    ASSERT(DXinterface);

    if ((result=this->Server::registerClient(DXinterface)))
    {
	//
	// Sync the interface with the current state of this command.
	//
	if (this->active)
	{
	    DXinterface->notify(Command::MsgActivate);
	}
	else
	{
	    DXinterface->notify(Command::MsgDeactivate);
	}
    }

    return result;
}


void Command::activate()
{
    //
    // 10/11/96 Reversed these 2 lines in order to make RepeatingToggle.  The
    // new class (subclasses from ToggleButtonInterface) and executes its associated
    // command whenever it's activated and getState()==TRUE.  In order to do that,
    // the command must be active first.
    //
    this->active = TRUE;
    this->notifyClients(Command::MsgActivate);
}


void Command::deactivate(const char *reason)
{
    //
    // 10/11/96 Reversed these 2 lines in order to match Command::activate().
    //
    this->active = FALSE;
    this->notifyClients(Command::MsgDeactivate, NULL, reason);
}


boolean Command::registerScope(CommandScope* scope)
{
    ASSERT(scope);

    if (this->scopeList.isMember(scope))
    {
	return FALSE;
    }
    else
    {
	return this->scopeList.appendElement(scope);
    }
}


boolean Command::unregisterScope(CommandScope* scope)
{
    int position;

    ASSERT(scope);

    if ((position=this->scopeList.getPosition(scope)))
    {
	return this->scopeList.deleteElement(position);
    }
    else
    {
	return FALSE;
    }
}


boolean Command::execute(CommandInterface *ci)
{
    ListIterator  scopeListIterator(this->scopeList);
    CommandScope* scope;
    boolean       result;

    //
    // If this command is not active, do not execute.
    //
    if (NOT this->active)
    {
	return FALSE;
    }

    //
    // Turn busy cursor on.
    //
    ASSERT(theApplication);
    theApplication->setBusyCursor(TRUE);


    //
    // Execute the command (in the subclasses).
    //
    if (this->doIt(ci))
    {
	//
	// Set the "Undo" command status for each scope.
	//
	while ((scope=(CommandScope*)scopeListIterator.getNext()))
	{
	    //
	    // If the "Undo" command exists within the application...
	    //
	    if (scope->getUndoCommand())
	    {
		//
		// If the command is reversible, activate the "Undo" command;
		// otherwise, deactivate it.
		//
		if (this->hasUndo)
		{
		    scope->setLastCommand(this);
		    scope->getUndoCommand()->activate();
		}
		else
		{
		    scope->setLastCommand(NUL(Command*));
		    scope->getUndoCommand()->deactivate();
		}
	    }
	}

	ListIterator list;
	Command      *c;
	for (list.setList(activateCmds); (c=(Command *)list.getNext()); )
	    c->activate();
	for (list.setList(deactivateCmds); (c=(Command *)list.getNext()); )
	    c->deactivate();

	result = TRUE;
    }
    else
    {
	result = FALSE;
    }

    //
    // Turn the busy cursor off.
    //
    theApplication->setBusyCursor(FALSE);

    //
    // Return execution result.
    //
    return result;
}


boolean Command::undo()
{
    ListIterator  scopeListIterator(this->scopeList);
    CommandScope* scope;
    Command*      command;
    boolean       result;

    //
    // Undo the command (in the subclasses).
    //
    if (this->undoIt())
    {
	while ((scope=(CommandScope*)scopeListIterator.getNext()))
	{
	    if ((command=scope->getUndoCommand()))
	    {
		command->deactivate();
	    }
	}

	result = TRUE;
    }
    else
    {
	result = FALSE;
    }

    //
    // Return execution result.
    //
    return result;
}


void Command::autoActivate(Command *c)
{
    int position;

    this->activateCmds.insertElement((const void *)c, 1);
    if ((position = this->deactivateCmds.getPosition((const void*)c)) != 0)
	this->deactivateCmds.deleteElement(position);

    c->addActivator(this);
}
void Command::autoDeactivate(Command *c)
{
    int position;

    this->deactivateCmds.insertElement((const void *)c, 1);
    if ((position = this->activateCmds.getPosition((const void*)c)) != 0)
	this->activateCmds.deleteElement(position);

    c->addActivator(this);
}
void Command::removeAutoCmd(Command *c)
{
    int position;

    if ((position = this->activateCmds.getPosition((const void*)c)) != 0)
	this->activateCmds.deleteElement(position);

    if ((position = this->deactivateCmds.getPosition((const void*)c)) != 0)
	this->deactivateCmds.deleteElement(position);

    c->removeActivator(this);
}

void Command::addActivator(Command *c)
{
    this->autoActivators.appendElement((const void*)c);
}
void Command::removeActivator(Command *c)
{
    this->autoActivators.removeElement((const void*)c);
}
