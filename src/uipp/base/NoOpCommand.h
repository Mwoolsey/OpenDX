/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoOpCommand_h
#define _NoOpCommand_h


#include "Command.h"


//
// Class name definition:
//
#define ClassNoOpCommand	"NoOpCommand"


//
// NoOpCommand class definition:
//				
class NoOpCommand : public Command
{
  protected:
    //
    // Does nothing;
    //
    virtual boolean doIt(CommandInterface *ci);

    //
    // Undoes nothing;
    //
    virtual boolean undoIt();

  public:
    //
    // Constructor:
    //
    NoOpCommand(const char*   name,
		CommandScope* scope,
		boolean       active);

    //
    // Destructor:
    //
    ~NoOpCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoOpCommand;
    }
};


#endif // _NoOpCommand_h
