/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ConfirmedOpenCommand.h"
#include "GARApplication.h"
#include "GARMainWindow.h"

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>

ConfirmedOpenCommand::ConfirmedOpenCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
			 GARMainWindow *gmw,
			 GARApplication *app) :
    OptionalPreActionCommand(name, scope, active,
                             "Save Confirmation",
                             "Do you want to save the header file?")
{
    this->application = app;
    this->gmw = gmw;

    //
    // We create this later, because creation uses a virtual Application
    // method.  If this class gets instanced in the Application's constructor
    // (typical) then the wrong virtual method will get called.
    //
    this->command = NULL;

}

void   ConfirmedOpenCommand::doPreAction()
{
    char *fname = gmw->getFilename();

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

boolean ConfirmedOpenCommand::doIt(CommandInterface *ci)
{
    this->gmw->postOpenFileDialog();
    return TRUE;
}

boolean ConfirmedOpenCommand::needsConfirmation()
{
    return this->application->isDirty();
}

