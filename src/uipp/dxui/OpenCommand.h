/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _OpenCommand_h
#define _OpenCommand_h


#include "OptionalPreActionCommand.h"

#include <Xm/Xm.h>

class	Network;
class   DXApplication;

//
// Class name definition:
//
#define ClassOpenCommand	"OpenCommand"


//
// OpenCommand class definition:
//				
class OpenCommand : public OptionalPreActionCommand 
{
  private:

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
    OpenCommand(const char*   name,
                        CommandScope* scope,
                        boolean       active,
			DXApplication *app,
			Widget dialogParent = NULL);

    //
    // Destructor:
    //
    ~OpenCommand(){}


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassOpenCommand;
    }
};


#endif // _OpenCommand_h
