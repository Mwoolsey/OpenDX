/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoChooserCommand.h"
#include "GARChooserWindow.h"

NoUndoChooserCommand::NoUndoChooserCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       GARChooserWindow *chooser,
				       ChooserCommandType comType) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->chooser = chooser;
}


boolean NoUndoChooserCommand::doIt(CommandInterface *)
{
    GARChooserWindow *chooser = this->chooser;
    boolean ret;

    ASSERT(chooser);

    ret = TRUE;
    switch (this->commandType) {

    case NoUndoChooserCommand::ShowMessages:
	ret = chooser->showMessages();
	break;

    case NoUndoChooserCommand::SelectDataFile:
	ret = chooser->postDataFileSelector();
	break;

    case NoUndoChooserCommand::OpenPrompter:
	ret = chooser->openExistingPrompter();
	break;

    case NoUndoChooserCommand::NoOp:
	ret = TRUE;
	break;

    default:
	ASSERT(0);
    }

    return ret;
}
