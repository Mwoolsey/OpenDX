/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoChoiceCommand.h"
#include "TypeChoice.h"

//
// A little bit none C++, but in the interest of space...   Usually we create
// another NoUndoCommand class, but this time I'll use this class for commands
// in the subclasses of TypeChoice.
//
#include "GridChoice.h"
#include "SpreadSheetChoice.h"

NoUndoChoiceCommand::NoUndoChoiceCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       TypeChoice *choice,
				       ChoiceCommandType comType) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->choice = choice;
}


boolean NoUndoChoiceCommand::doIt(CommandInterface *)
{
    TypeChoice *choice = this->choice;
    GridChoice *grid;
    SpreadSheetChoice *ssc;
    boolean ret;

    ASSERT(choice);

    ret = TRUE;
    switch (this->commandType) {

    case NoUndoChoiceCommand::Browse:
	ret = choice->browse();
	break;

    case NoUndoChoiceCommand::SetChoice:
	ret = choice->setChoice();
	break;

    case NoUndoChoiceCommand::Visualize:
	ret = choice->visualize();
	break;

    case NoUndoChoiceCommand::VerifyData:
	ret = choice->verify();
	break;

    case NoUndoChoiceCommand::SetGridType:
	grid = (GridChoice*)choice;
	ret = grid->setGridType();
	break;

    case NoUndoChoiceCommand::Positions:
	grid = (GridChoice*)choice;
	ret = grid->usePositions();
	break;

    case NoUndoChoiceCommand::Prompter:
	ret = choice->prompter();
	break;

    case NoUndoChoiceCommand::SimplePrompter:
	ret = choice->simplePrompter();
	break;

    case NoUndoChoiceCommand::RestrictNames:
	ssc = (SpreadSheetChoice*)choice;
	ret = ssc->restrictNamesCB();
	break;

    case NoUndoChoiceCommand::SpecifyRows:
	ssc = (SpreadSheetChoice*)choice;
	ret = ssc->useRowsCB();
	break;

    case NoUndoChoiceCommand::UseDelimiter:
	ssc = (SpreadSheetChoice*)choice;
	ret = ssc->useDelimiterCB();
	break;

    case NoUndoChoiceCommand::NoOp:
	ret = TRUE;
	break;

    default:
	ASSERT(0);
    }

    return ret;
}
