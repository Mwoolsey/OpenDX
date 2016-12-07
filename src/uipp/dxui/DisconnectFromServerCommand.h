/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _DisconnectFromServerCommand_h
#define _DisconnectFromServerCommand_h


#include "ConfirmedCommand.h"


//
// Class name definition:
//
#define ClassDisconnectFromServerCommand	"DisconnectFromServerCommand"


//
// DisconnectFromServerCommand class definition:
//				
class DisconnectFromServerCommand : public ConfirmedCommand
{
  protected:
    //
    // Implements the command:
    //   Queries the user for confirmation of the command.
    //   If affirmatively confirmed, the command terminates
    //   the application program.
    //
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    DisconnectFromServerCommand(const char*   name,
		CommandScope* scope,
		boolean       active);

    //
    // Destructor:
    //

    ~DisconnectFromServerCommand();
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDisconnectFromServerCommand;
    }
};


#endif // _DisconnectFromServerCommand_h
