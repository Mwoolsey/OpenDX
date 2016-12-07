/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "DeleteNodeCommand.h"
#include "Application.h"
#include "EditorWindow.h"


DeleteNodeCommand::DeleteNodeCommand(const char*   name,
			             CommandScope* scope, 
				     boolean active,
                                     EditorWindow *editor) 
                      : NoUndoCommand(name, scope, active)
{
    ASSERT(editor);
    this->editor = editor;
}


boolean DeleteNodeCommand::doIt(CommandInterface *ci)
{
    EditorWindow *editor = this->editor;

    editor->removeSelectedNodes();

    return TRUE;
}

