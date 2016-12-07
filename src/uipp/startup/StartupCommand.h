/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _StartupCommand_h
#define _StartupCommand_h



#include <Xm/Xm.h>

#include "../base/NoUndoCommand.h"

class  StartupWindow;
typedef int StartupCommandType;

//
// StartupCommand class definition:
//				

#define ClassStartupCommand  "StartupCommand"

class StartupCommand : public NoUndoCommand 
{
  private:

    static  String	DefaultResources[];
    StartupWindow* 	startup_window;
    int			option;
    StartupCommandType  commandType;

  protected:
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    StartupCommand(const char*,
		CommandScope*,
		boolean active,
                StartupWindow*,
		int option);

    // These are the various operations that the StartupCommand can
    // implement on behalf of a StartupApplication.
    //
    enum {
	StartImage	= 3,
	StartPrompter	= 4,
	StartEditor	= 5,
	EmptyVPE	= 6,
	StartTutorial	= 7,
	StartDemo	= 8,
	Quit		= 9,
	Help		= 10
    };

    //
    // Destructor:
    //
    ~StartupCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassStartupCommand; }
};


#endif // _StartupCommand_h
