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


#include "../base/OptionalPreActionCommand.h"

#include <Xm/Xm.h>

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
                        boolean       active);

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
