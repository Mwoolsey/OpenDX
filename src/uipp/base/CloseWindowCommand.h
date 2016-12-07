/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _CloseWindowCommand_h
#define _CloseWindowCommand_h


#include "NoUndoCommand.h"


//
// Class name definition:
//
#define ClassCloseWindowCommand	"CloseWindowCommand"


//
// Referenced classes:
//
class MainWindow;


//
// CloseWindowCommand class definition:
//				
class CloseWindowCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    MainWindow* window;

    //
    // Does nothing;
    //
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    CloseWindowCommand(const char*   name,
		       CommandScope* scope,
		       boolean       active,
		       MainWindow*   window);

    //
    // Destructor:
    //
    ~CloseWindowCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCloseWindowCommand;
    }
};


#endif // _CloseWindowCommand_h
