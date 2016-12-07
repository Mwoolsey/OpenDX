/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "NoUndoCommand.h"


NoUndoCommand::NoUndoCommand(const char*   name,
			     CommandScope* scope,
			     boolean       active): Command(name, scope, active)
{
    this->hasUndo = FALSE;
}


