/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _HelpOnContextCommand_h
#define _HelpOnContextCommand_h


#include "NoUndoCommand.h"


//
// Class name definition:
//
#define ClassHelpOnContextCommand	"HelpOnContextCommand"


//
// Referenced classes:
//
class MainWindow;


//
// HelpOnContextCommand class definition:
//				
class HelpOnContextCommand : public NoUndoCommand
{
  private:
    //
    // Private class data:
    //
   static boolean	HelpOnContextCommandClassInitialized;

  protected:
    //
    // Protected class data:
    //
    static Cursor	HelpCursor;	// help cursor for the component

    //
    // Protected member data:
    //
    MainWindow*		window;		// associated window

    //
    // Does nothing;
    //
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    HelpOnContextCommand(const char*   name,
			 CommandScope* scope,
			 boolean       active,
			 MainWindow*   window);

    //
    // Destructor:
    //
    ~HelpOnContextCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassHelpOnContextCommand;
    }
};


#endif // _HelpOnContextCommand_h
