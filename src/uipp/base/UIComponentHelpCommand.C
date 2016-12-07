/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <X11/cursorfont.h>
#include <Xm/Xm.h>

#include "UIComponentHelpCommand.h"
#include "UIComponent.h"


UIComponentHelpCommand::UIComponentHelpCommand(const char*   name,
					   CommandScope* scope,
					   boolean       active,
					   UIComponent*   component) :
	NoUndoCommand(name, scope, active)
{

    ASSERT(component);

    this->component = component;
}


boolean UIComponentHelpCommand::doIt(CommandInterface *ci)
{

    this->component->componentHelp();
    return TRUE;
}
