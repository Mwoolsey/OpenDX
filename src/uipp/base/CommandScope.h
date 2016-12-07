/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _CommandScope_h
#define _CommandScope_h


#include "Base.h"
#include "Command.h"


//
// Class name definition:
//
#define ClassCommandScope	"CommandScope"


//
// Referenced classes:
//
class Command;
class UndoCommand;


//
// CommandScope class definition:
//				
class CommandScope : public Base
{
    friend class Command;
    friend class UndoCommand;

  protected:
    //
    // Protected member data:
    //
    Command* lastCommand;	// the last undoable command executed
    Command* undoCommand;	// the local undo command

    //
    // This routine is intended to be called by Command objects only.
    //
    virtual void setLastCommand(Command* command)
    {
	this->lastCommand = command;
    }

    //
    // This routine is intended to be called by UndoCommand objects only.
    //
    virtual void setUndoCommand(Command* command)
    {
	this->undoCommand = command;
    }

    //
    // Access routines:
    //   To be accessed by friend classes only.
    //
    Command* getLastCommand()
    {
	return this->lastCommand;
    }
    Command* getUndoCommand()
    {
	return this->undoCommand;
    }

  public:
    //
    // Constructor:
    //
    CommandScope()
    {
	this->lastCommand = NUL(Command*);
	this->undoCommand = NUL(Command*);
    }

    //
    // Destructor:
    //
    ~CommandScope(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCommandScope;
    }
};


#endif // _CommandScope_h
