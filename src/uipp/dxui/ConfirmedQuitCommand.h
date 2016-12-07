/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ConfirmedQuitCommand_h
#define _ConfirmedQuitCommand_h


#include "OptionalPreActionCommand.h"

#include <Xm/Xm.h>

class   DXApplication;
class   Command;

//
// Class name definition:
//
#define ClassConfirmedQuitCommand	"ConfirmedQuitCommand"


//
// ConfirmedQuitCommand class definition:
//				
class ConfirmedQuitCommand : public OptionalPreActionCommand 
{
  private:

    Command     *command;
    DXApplication *application;

  protected:
    //
    // Implements the command:

    virtual boolean needsConfirmation();
    virtual void    doPreAction();
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    ConfirmedQuitCommand(const char*   name,
                        CommandScope* scope,
                        boolean       active,
			DXApplication *app);

    //
    // Destructor:
    //
    ~ConfirmedQuitCommand();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassConfirmedQuitCommand;
    }
};


#endif // _ConfirmedQuitCommand_h
