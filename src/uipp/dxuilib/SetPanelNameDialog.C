/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "Xm/Form.h"
#include "Xm/Label.h"
#include "Xm/PushB.h"
#include "Xm/Separator.h"
#include "Xm/Text.h"

#include "DXStrings.h"
#include "SetPanelNameDialog.h"

#include "Application.h"
#include "ControlPanel.h"
#include "Network.h"
#include "ErrorDialogManager.h"

boolean SetPanelNameDialog::ClassInitialized = FALSE;
String SetPanelNameDialog::DefaultResources[] =
{
    "*dialogTitle:               	Change Control Panel Name...", 
    "*nameLabel.labelString:            Control Panel Name:",
    NULL
};


SetPanelNameDialog::SetPanelNameDialog(Widget parent, ControlPanel *cp) :
    SetNameDialog("setPanelNameDialog", parent)
{
    this->controlPanel = cp;
  
    if (NOT SetPanelNameDialog::ClassInitialized)
    {
        SetPanelNameDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetPanelNameDialog::~SetPanelNameDialog()
{
}

//
// Install the default resources for this class.
//
void SetPanelNameDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SetPanelNameDialog::DefaultResources);
    this->SetNameDialog::installDefaultResources( baseWidget);
}

const char *SetPanelNameDialog::getText()
{
    return this->controlPanel->getPanelNameString();
}

boolean SetPanelNameDialog::saveText(const char *s)
{
    if (IsBlankString(s)) {
	ModalErrorMessage(this->getRootWidget(),
		"The name string cannot be blank.");
        return FALSE;
    } else {
	ControlPanel *panel = this->controlPanel;
    	panel->setPanelName(s);
    	panel->getNetwork()->setFileDirty();
    }

    return TRUE;
}

