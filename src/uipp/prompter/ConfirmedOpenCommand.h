/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ConfirmedOpenCommand_h
#define _ConfirmedOpenCommand_h


#include "../base/OptionalPreActionCommand.h"

#include <Xm/Xm.h>

class   GARApplication;
class   GARMainWindow;
class   OpenFileDialog;
class   Command;

//
// Class name definition:
//
#define ClassConfirmedOpenCommand	"ConfirmedOpenCommand"


//
// ConfirmedOpenCommand class definition:
//				
class ConfirmedOpenCommand : public OptionalPreActionCommand 
{
  private:

    Command     *command;
    GARApplication *application;
    GARMainWindow *gmw;

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
    ConfirmedOpenCommand(const char*   name,
                        CommandScope* scope,
                        boolean       active,
			GARMainWindow *gmw,
			GARApplication *app);

    //
    // Destructor:
    //
    ~ConfirmedOpenCommand(){}


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassConfirmedOpenCommand;
    }
};


#endif // _ConfirmedOpenCommand_h
