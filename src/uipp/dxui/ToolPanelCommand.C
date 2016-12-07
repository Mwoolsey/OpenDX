/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "ToolPanelCommand.h"
#include "ListIterator.h"
#include "DXApplication.h"
#include "EditorWindow.h"
#include "ToggleButtonInterface.h"
#include "DXStrings.h"


ToolPanelCommand::ToolPanelCommand(const char*     name,
				   CommandScope*   scope,
				   boolean         active,
				   EditorWindow* editor) :
	Command(name, scope, active)
{
    ASSERT(editor);

    //
    // Save the associated editor.
    //
    this->editor = editor;
}


boolean ToolPanelCommand::doIt(CommandInterface *ci)
{
    //
    // Toggle the tool panel and toggle button interface states.
    //
    this->editor->toggleToolPanel();
    this->notifyClients(ToggleButtonInterface::MsgToggleState);
    return TRUE;
}



boolean ToolPanelCommand::undoIt()
{
    this->doIt(NULL);
    return TRUE;
}

