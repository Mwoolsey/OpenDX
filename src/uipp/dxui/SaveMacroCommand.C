/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "SaveMacroCommand.h"
#include "DXApplication.h"
#include "DXWindow.h"
#include "MacroDefinition.h"
#include "QuestionDialogManager.h"
#include "Network.h"

SaveMacroCommand::SaveMacroCommand(const char*   name,
                 		CommandScope* scope,
                 		boolean       active,
				MacroDefinition *md)
		: NoUndoCommand(name, scope, active)
{
    this->md = md;
    this->next = NULL;
}

SaveMacroCommand::~SaveMacroCommand()
{
}

void SaveMacroCommand::SaveMacro(void *clientData)
{
    SaveMacroCommand *smc= (SaveMacroCommand*)clientData;

    smc->md->body->saveNetwork(smc->md->fileName);
    if(smc->next)
	smc->next->execute();
    else
    	delete smc->md;
}

void SaveMacroCommand::DiscardMacro(void *clientData)
{
    SaveMacroCommand *smc= (SaveMacroCommand*)clientData;

    if(smc->next)
	smc->next->execute();
    else
    	delete smc->md;
}

boolean SaveMacroCommand::doIt(CommandInterface *ci)
{
    char message[1024];
    sprintf(message, "Do you want to save macro %s as file: %s?", 
			this->md->body->getNameString(),
			this->md->fileName);

    theQuestionDialogManager->modalPost(
        theDXApplication->getAnchor()->getRootWidget(),
        message,
        "Save Confirmation",
        (void*)this,
        SaveMacro,
        DiscardMacro,
        NULL,
        "Yes",
        "No");

    return (TRUE);
}

void SaveMacroCommand::setNext(Command *next)
{
    this->next = next;
}

