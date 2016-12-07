/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _QuitCommand_h
#define _QuitCommand_h


#include "ConfirmedCommand.h"


//
// Class name definition:
//
#define ClassQuitCommand	"QuitCommand"


class Application;


//
// QuitCommand class definition:
//				
class QuitCommand : public ConfirmedCommand
{
  private:
    Application *application;
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
    QuitCommand(Application*  app,
		const char*   name,
		CommandScope* scope,
		boolean       active,
		char*         dialogTitle,
		char*         dialogQuestion);

    //
    // Destructor:
    //
    ~QuitCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassQuitCommand;
    }
};


#endif // _QuitCommand_h
