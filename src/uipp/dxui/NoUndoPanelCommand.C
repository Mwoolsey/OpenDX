/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoPanelCommand.h"
#include "ControlPanel.h"
#include "Network.h"

NoUndoPanelCommand::NoUndoPanelCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       ControlPanel  *cp,
				       PanelCommandType comType ) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->controlPanel = cp;
}


boolean NoUndoPanelCommand::doIt(CommandInterface *ci)
{
    ControlPanel *cp = this->controlPanel;

    ASSERT(cp);

    switch (this->commandType) {
	case NoUndoPanelCommand::AddInteractors:
	    cp->initiateInteractorPlacement();
	    break;
	case NoUndoPanelCommand::ShowInteractors:
	    cp->showSelectedInteractors();
	    break;
	case NoUndoPanelCommand::ShowStandIns:
	    cp->showSelectedStandIns();
	    break;
	case NoUndoPanelCommand::DeleteInteractors:
	    cp->deleteSelectedInteractors();
	    break;
	case NoUndoPanelCommand::SetInteractorAttributes:
	    cp->openSelectedSetAttrDialog();
	    break;
	case NoUndoPanelCommand::SetInteractorLabel:
	    cp->setSelectedInteractorLabel();
	    break;
	case NoUndoPanelCommand::SetHorizontalLayout:
	    cp->setVerticalLayout(FALSE);
	    break;
	case NoUndoPanelCommand::SetVerticalLayout:
	    cp->setVerticalLayout(TRUE);
	    break;
	case NoUndoPanelCommand::SetPanelComment:
	    cp->editPanelComment();
	    break;
	case NoUndoPanelCommand::SetPanelName:
	    cp->editPanelName();
	    break;
	case NoUndoPanelCommand::SetPanelAccess:
	    cp->postPanelAccessDialog(cp->panelAccessManager); 
	    break;
	case NoUndoPanelCommand::SetPanelGrid:
	    cp->setPanelGrid();
	    break;
	case NoUndoPanelCommand::TogglePanelStyle:
	    cp->setPanelStyle(FALSE);
	    break;
#if 0
	case NoUndoPanelCommand::TogglePanelStartup:
	    cp->togglePanelStartup();
	    break;
#endif
#if 000
	case NoUndoPanelCommand::OpenFile:
	    cp->postOpenCFGDialog();	
	    break;
	case NoUndoPanelCommand::SaveFile:
	    cp->postSaveCFGDialog();	
	    break;
#endif
	case NoUndoPanelCommand::HelpOnPanel:
	    cp->postHelpOnPanel();	
	    break;
	case NoUndoPanelCommand::HitDetection:
	    cp->toggleHitDetection();	
	    break;
	default:
	    ASSERT(0);
    }

    return TRUE;
}


