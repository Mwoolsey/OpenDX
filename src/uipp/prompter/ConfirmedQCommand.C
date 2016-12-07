/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ConfirmedQCommand.h"
#include "GARApplication.h"
#include "GARMainWindow.h"
#include "../base/QuitCommand.h"

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>

ConfirmedQCommand::ConfirmedQCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active) :
    OptionalPreActionCommand(name, scope, active,
                             "Save Confirmation",
                             "Do you want to save the header file?")
{

    //
    // We create this later, because creation uses a virtual Application
    // method.  If this class gets instanced in the Application's constructor
    // (typical) then the wrong virtual method will get called.
    //
    this->command = NULL;

}

void   ConfirmedQCommand::doPreAction()
{
    char *fname = NUL(char*);
    GARMainWindow *gmw = theGARApplication->getMainWindow();
    if (gmw) fname = gmw->getFilename();

    if (fname) {
       if (gmw->saveGAR(fname))
            this->doIt(NULL);
    } else if (gmw) {
        gmw->PostSaveAsDialog(gmw, this);
    }
}

boolean ConfirmedQCommand::doIt(CommandInterface *ci)
{
    if (!this->command) {
	//
	// We can use a static buffer since there is only one application.
	//
        static char *dialogQuestion = NULL;
        if (!dialogQuestion) {
            dialogQuestion = new char[256];
            sprintf(dialogQuestion,"Do you really want to quit %s?",
                                theApplication->getInformalName());
        }

        CommandScope *scope = (CommandScope*)this->scopeList.getElement(1);

	this->command = 
	    new QuitCommand
		(theGARApplication,
		 "quit",
		 scope,
		 TRUE,
		 "Quit",
		 dialogQuestion);
    }

    this->command->execute(ci);
    return TRUE;
}

boolean ConfirmedQCommand::needsConfirmation()
{
    return theGARApplication->isDirty();
}

