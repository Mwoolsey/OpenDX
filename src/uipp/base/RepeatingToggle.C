/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>





#include "RepeatingToggle.h"
#include "Command.h"


RepeatingToggle::RepeatingToggle(Widget parent, char* name, 
    Command* command, boolean  state, const char *bubbleHelp):
	ToggleButtonInterface(parent, name, command, state, bubbleHelp)
{
}

void RepeatingToggle::activate()
{
    this->ToggleButtonInterface::activate();
    if (this->getState())
	this->command->execute();
}
