/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoCommand_h
#define _NoUndoCommand_h


#include "Command.h"


//
// NoUndoCommand class definition:
//				
class NoUndoCommand : public Command
{
  protected:
    //
    // No op for this class...
    //
    virtual boolean undoIt()
    {
	return FALSE;
    }

    //
    // Constructor:
    //   Protected to prevent direct instantiation.
    //
    NoUndoCommand(const char*   name,
		  CommandScope* scope,
		  boolean       active);

  public:
    //
    // Destructor:
    //
    ~NoUndoCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return "NoUndoCommand";
    }
};


#endif /*  _NoUndoCommand_h */
