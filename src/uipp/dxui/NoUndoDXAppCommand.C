/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoDXAppCommand.h"
#include "DXApplication.h"
#include "MsgWin.h"
#include "Network.h"
#include "SequencerNode.h"
#include "ToggleButtonInterface.h"


NoUndoDXAppCommand::NoUndoDXAppCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       DXApplication *app,
				       DXAppCommandType comType ) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->application = app;
}


boolean NoUndoDXAppCommand::doIt(CommandInterface *ci)
{
    DXApplication *app = this->application;
    boolean ret;
    SequencerNode*  seq_node = NULL; // xlC wants this initialized 

    ASSERT(app);

    ret = TRUE;
    switch (this->commandType) {

    case NoUndoDXAppCommand::StartServer:
	ret = app->postStartServerDialog();
	break;

    case NoUndoDXAppCommand::ResetServer:
	ret = app->resetServer();
	break;

    case NoUndoDXAppCommand::ExecuteOnce:
	app->getExecCtl()->executeOnce();
	break;

    case NoUndoDXAppCommand::ExecuteOnChange:
	app->getExecCtl()->enableExecOnChange();
	break;

    case NoUndoDXAppCommand::EndExecution:
	app->getExecCtl()->terminateExecution();
	break;

    case NoUndoDXAppCommand::OpenNetwork:
	app->postOpenNetworkDialog();
	break;

    case NoUndoDXAppCommand::LoadMacro:
	app->postLoadMacroDialog();
	break;

    case NoUndoDXAppCommand::AssignProcessGroup:
	app->postProcessGroupAssignDialog();
	break;

#if USE_REMAP	// 6/14/93
    case NoUndoDXAppCommand::RemapInteractorOutputs:
	if (NOT app->network) {
	    ret = FALSE;
	} else {
	    Network *n = app->network;
	    boolean remap = n->isRemapInteractorOutputMode();
	    n->setRemapInteractorOutputMode(!remap);
	    this->notifyClients(ToggleButtonInterface::MsgToggleState);
	}
	break;
#endif

    case NoUndoDXAppCommand::OpenAllColormaps:
	if (NOT app->network) {
	    ret = FALSE;
	} else {
	    app->network->openColormap(TRUE);
	}
	break;

    case NoUndoDXAppCommand::OpenSequencer:
	ASSERT(app->network);
	seq_node = app->network->sequencer;
	if (NOT seq_node) {
	    ret = FALSE;
	} else {
	    seq_node->openDefaultWindow(app->getRootWidget());
	    ret = TRUE;
	}
	break;

    case NoUndoDXAppCommand::OpenMessageWindow:
	theDXApplication->getMessageWindow()->manage();
	ret = TRUE;
	break;

    case NoUndoDXAppCommand::ToggleInfoEnable:
	theDXApplication->enableInfo(!theDXApplication->isInfoEnabled());
	break;

    case NoUndoDXAppCommand::ToggleWarningEnable:
	theDXApplication->enableWarning(!theDXApplication->isWarningEnabled());
	break;

    case NoUndoDXAppCommand::ToggleErrorEnable:
	theDXApplication->enableError(!theDXApplication->isErrorEnabled());
	break;

    case NoUndoDXAppCommand::LoadUserMDF:
	theDXApplication->postLoadMDFDialog();
	ret = TRUE;
	break;

    case NoUndoDXAppCommand::HelpOnManual:
	theDXApplication->helpOn("OnManual");
	ret = TRUE;
	break;

    case NoUndoDXAppCommand::HelpOnHelp:
	theDXApplication->helpOn("OnHelp");
	ret = TRUE;
	break;

    default:
	ASSERT(0);
    }

    return ret;
}

//
// Conditionally call the super class method.  Currently, we don't
// activate if there is no connection to the server and we are activating
// ExecuteOnChange, ExecuteOnce or EndExecution command.
//
void NoUndoDXAppCommand::activate()
{
    DXApplication *app = this->application;
    boolean connected = app->getPacketIF() != NULL;


    ASSERT(app);

    boolean enable = TRUE;

    switch (this->commandType) {

    case NoUndoDXAppCommand::ExecuteOnce:
    case NoUndoDXAppCommand::ExecuteOnChange:
    case NoUndoDXAppCommand::EndExecution:
	if (!connected) enable = FALSE;
	break;

    }
    if (enable)
	this->NoUndoCommand::activate();
}


