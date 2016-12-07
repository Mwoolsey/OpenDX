/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ConfirmedQCommand_h
#define _ConfirmedQCommand_h


#include "OptionalPreActionCommand.h"

#include <Xm/Xm.h>

class   MBApplication;
class   MBMainWindow;
class   Command;

//
// Class name definition:
//
#define ClassConfirmedQCommand	"ConfirmedQCommand"


//
// ConfirmedQCommand class definition:
//				
class ConfirmedQCommand : public OptionalPreActionCommand 
{
  private:

    Command     *command;
    MBApplication *application;
    MBMainWindow *mbmw;

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
    ConfirmedQCommand(const char*   name,
                        CommandScope* scope,
                        boolean       active,
			MBMainWindow  *mbmw,
			MBApplication *app);

    //
    // Destructor:
    //
    ~ConfirmedQCommand(){}


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassConfirmedQCommand;
    }
};


#endif // _ConfirmedQCommand_h
