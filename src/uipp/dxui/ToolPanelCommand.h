/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ToolPanelCommand_h
#define _ToolPanelCommand_h


#include "Command.h"


//
// Class name definition:
//
#define ClassToolPanelCommand	"ToolPanelCommand"


//
// Referenced classes:
//
class ToggleButtonInterface;
class EditorWindow;


//
// ToolPanelCommand class definition:
//				
class ToolPanelCommand : public Command
{
  protected:
    //
    // Protected member data:
    //
    EditorWindow* editor;
    boolean       setting;

    //
    // Implements the command:
    //   Toggles the tool panel to be visible/invisible.
    //
    virtual boolean doIt(CommandInterface *ci);


    //
    // Undoes the command.
    //
    virtual boolean undoIt();


  public:
    //
    // Constructor:
    //
    ToolPanelCommand(const char*     name,
		     CommandScope*   scope,
		     boolean         active,
		     EditorWindow* editor);

    //
    // Destructor:
    //
    ~ToolPanelCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassToolPanelCommand;
    }
};


#endif //  _ToolPanelCommand_h
