/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ConfirmedExitCommand_h
#define _ConfirmedExitCommand_h


#include "OptionalPreActionCommand.h"


class   DXApplication;
class   Command;

//
// Class name definition:
//
#define ClassConfirmedExitCommand	"ConfirmedExitCommand"


//
// ConfirmedExitCommand class definition:
//				
class ConfirmedExitCommand : public OptionalPreActionCommand 
{
  private:

    DXApplication *application;

  protected:
    //
    // Implements the command:

    virtual boolean needsConfirmation();
    virtual void    doPreAction();

  public:
    //
    // Constructor:
    //
    ConfirmedExitCommand(const char*   name,
                        CommandScope* scope,
                        boolean       active,
			DXApplication *app);

    //
    // Destructor:
    //
    ~ConfirmedExitCommand(){}

    boolean doIt(CommandInterface*);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassConfirmedExitCommand;
    }
};


#endif // _ConfirmedExitCommand_h
