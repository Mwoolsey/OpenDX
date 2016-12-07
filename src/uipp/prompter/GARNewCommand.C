/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "GARNewCommand.h"
#include "GARMainWindow.h"
#include "GARApplication.h"

GARNewCommand::GARNewCommand(const char      *name,
		       CommandScope    *scope,
		       boolean		active,
		       GARMainWindow   *gmw,
		       Widget		dialogParent) :
    OptionalPreActionCommand(name, scope, active,
			     "Save Confirmation",
			     "Do you want to save the header?",
				dialogParent)
{
    this->gmw = gmw;
}

void   GARNewCommand::doPreAction()
{
    char    *fname = this->gmw->getFilename();

    if(fname)
    {
        if(this->gmw->saveGAR(fname))
            this->doIt(NULL);
    }
    else 
    {
        this->gmw->PostSaveAsDialog(this->gmw, this);
    }

}

boolean GARNewCommand::doIt(CommandInterface *)
{
    this->gmw->newGAR();
    return TRUE;
}


boolean GARNewCommand::needsConfirmation()
{
    return theGARApplication->isDirty();
}
