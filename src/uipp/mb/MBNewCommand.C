/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "MBNewCommand.h"
#include "MBMainWindow.h"
#include "MBApplication.h"

MBNewCommand::MBNewCommand(const char      *name,
		       CommandScope    *scope,
		       boolean		active,
		       MBMainWindow   *mbmw,
		       Widget		dialogParent) :
    OptionalPreActionCommand(name, scope, active,
			     "Save Confirmation",
			     "Do you want to save the file?",
				dialogParent)
{
    this->mbmw = mbmw;
}

void   MBNewCommand::doPreAction()
{
    char    *fname = this->mbmw->getFilename();

    if(fname)
    {
        if(this->mbmw->saveMB(fname))
            this->doIt(NULL);
    }
    else 
    {
        this->mbmw->PostSaveAsDialog(this->mbmw, this);
    }

}

boolean MBNewCommand::doIt(CommandInterface *ci)
{
    this->mbmw->newMB(TRUE);
    return TRUE;
}


boolean MBNewCommand::needsConfirmation()
{
    return theMBApplication->isDirty();
}
