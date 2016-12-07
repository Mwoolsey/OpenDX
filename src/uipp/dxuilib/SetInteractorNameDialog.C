/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXApplication.h"
#include "SetInteractorNameDialog.h"
#include "ControlPanel.h"


Boolean SetInteractorNameDialog::ClassInitialized = FALSE;
String SetInteractorNameDialog::DefaultResources[] = {
    "*dialogTitle: 			Set Interactor label...",
    "*nameLabel.labelString:		Interactor Label:",
    NULL
};

SetInteractorNameDialog::SetInteractorNameDialog( ControlPanel *panel)  : 
		SetNameDialog("setInteractorNameDialog", 
				panel->getRootWidget())
{
    this->panel = panel;

    if (NOT SetInteractorNameDialog::ClassInitialized)
    {
        SetInteractorNameDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetInteractorNameDialog::~SetInteractorNameDialog()
{
}

//
// Install the default resources for this class.
//
void SetInteractorNameDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SetInteractorNameDialog::DefaultResources);
    this->SetNameDialog::installDefaultResources( baseWidget);
}
const char *SetInteractorNameDialog::getText()
{
    return this->panel->getInteractorLabel();
}

boolean SetInteractorNameDialog::saveText(const char *s)
{
    this->panel->setInteractorLabel(s);
    return TRUE;
}

//
// The dialog must be modal because otherwise the user can select
// more interactors by double clicking in the vpe.  lloydt124
//
Widget SetInteractorNameDialog::createDialog(Widget parent)
{
    Widget dialog = this->SetNameDialog::createDialog(parent);
    XtVaSetValues (dialog, XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, NULL);
    return dialog;
}
