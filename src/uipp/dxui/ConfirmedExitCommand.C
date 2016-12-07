/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>

#include "ConfirmedExitCommand.h"
#include "DXApplication.h"
#include "DXWindow.h"
#include "Network.h"
#include "QuitCommand.h"
#include "ListIterator.h"
#include "MacroDefinition.h"
#include "SaveMacroCommand.h"

ConfirmedExitCommand::ConfirmedExitCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
			 DXApplication *app) :
    OptionalPreActionCommand(name, scope, active,
                             "Save Confirmation",
                             "Do you want to save the macro(s)?")
{
    this->application = app;

}

//
// Pop up the confirmation dialog to ask the user to save the macro
// if it's modified.  Execute the quitCmd after all macros are done.
//
void   ConfirmedExitCommand::doPreAction()
{
    DXApplication *app = this->application;
    ListIterator  li(app->macroList);
    Network       *net;
    SaveMacroCommand *cmd = NULL, *first = NULL,*last = NULL;

    while ( (net = (Network*)li.getNext()) )
    	if (net->isMacro() AND net->saveToFileRequired())
	{
	    cmd = (SaveMacroCommand*)net->getDefinition()->getSaveCmd();
	    if (NOT first)
		first = cmd;
	    if (last)
		last->setNext(cmd);
	    last = cmd;
	}

    if (last)
    {
	last->setNext(this->application->quitCmd);
	first->execute();
    }
}

boolean ConfirmedExitCommand::doIt(CommandInterface *ci)
{
   this->application->quitCmd->execute(ci);

   return TRUE;
}

//
// Return TRUE if any macro is modified.
//
boolean ConfirmedExitCommand::needsConfirmation()
{
    DXApplication *app = this->application;
    ListIterator  li(app->macroList);
    Network       *net;

    while ( (net = (Network*)li.getNext()) )
    	if (net->isMacro() AND net->saveToFileRequired())
	    return TRUE;
     
    return FALSE;
}

