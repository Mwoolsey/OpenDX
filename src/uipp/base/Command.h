/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _Command_h
#define _Command_h


#include "Server.h"
#include "List.h"


//
// Class name definition:
//
#define ClassCommand	"Command"


//
// Referenced classes:
//
class CommandScope;
class CommandInterface;

//
// Command class definition:
//				
class Command : public Server
{
  private:
    //
    // Private class data:
    //
    static boolean CommandClassInitialized; // Command class initialized?

    //
    // Private member data:
    //
    char*		name;		// name of the command
    boolean		active;		// is the command active?
    List		activateCmds;	// Commands to activate on success
    List		deactivateCmds;	// Commands to activate on success
    List		autoActivators;	// Commands that automatically 
					// (de)activate this command.
    void		addActivator(Command*);
    void		removeActivator(Command*);

  protected:
    //
    // Protected member data:
    //
    boolean		hasUndo;	// TRUE if this Command supports undo
    List		scopeList;	// list of command scopes

    //
    // To be defined by subclasses:
    // This function will be called by execute() function to perform
    // the actual work done by the command.
    // As per the definition of execute(), if available, the CommandInterface 
    // that initiated the execution is provided, otherwise ci will be NULL.
    // NULL.  So, if this method uses ci, it may also need to be prepared to 
    // handle the case where ci==NULL. 
    //
    virtual boolean doIt(CommandInterface *ci) = 0;

    //
    // To be defined by subclasses:
    //   This function will be called by undo() function to perform
    //   the actual undo work.
    //
    virtual boolean undoIt() = 0;

    //
    // Constructor:
    //   Protected to prevent direct instantiation.
    //
    Command(const char*   name,
	    CommandScope* scope,
	    boolean       active);

  public:
    //
    // Notification messages:
    //
    static Symbol MsgActivate;
    static Symbol MsgDeactivate;
    static Symbol MsgDisassociate;
    static Symbol MsgBeginExecuting;
    static Symbol MsgEndExecuting;

    //
    // Destructor:
    //
    ~Command();

    //
    // Implementation of registerClient() for this class:
    //   Sends activation/deactivation message to client interfaces
    //   to sync it with the current state of the Command object.
    //
    virtual boolean registerClient(Client* client);

    //
    // Intiates the action or actions supported by this Command object.
    // This function provides the public interface through which
    // any command is executed.  It notifies all clients of the application
    // when the command begins and ends.  If available, then the 
    // CommandInterface that initiated the execution is provided, otherwise
    // ci is passed as NULL. 
    //
    virtual boolean execute(CommandInterface *ci = NULL);

    //
    // Causes the action initiated by the execute() member function to be
    // reversed. This funciton provides the public interface through 
    // which any command is be reversed.
    //
    boolean undo();

    //
    // Enables the command and any user interface component associated
    // with this Command object.  Command objects can be executed only
    // if they are enabled.
    //
    virtual void activate();

    //
    // Disables the command and any user interface component associated
    // with this Command object.
    //
    virtual void deactivate(const char *reason = NULL);

    //
    // Registers a new scope to this command.
    //   Returns TRUE if successful; FALSE, otherwise.
    //
    virtual boolean registerScope(CommandScope* scope);

    //
    // Unregisters a scope from this command.
    //   Returns TRUE if successful; FALSE, otherwise.
    //
    virtual boolean unregisterScope(CommandScope* scope);

    //
    // Returns active status.
    //
    boolean isActive()
    {
	return this->active;
    }

    //
    // Returns undo capability status.
    //
    boolean canUndo()
    {
	return this->hasUndo;
    }

    //
    // Functions to append a command to either the activation or deactivation
    // list.
    //
    void autoActivate(Command *c);
    void autoDeactivate(Command *c);
    void removeAutoCmd(Command *c);

    //
    // Returns the command name.
    //
    const char* getName()
    {
	return this->name;
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCommand;
    }
};


#endif // _Command_h
