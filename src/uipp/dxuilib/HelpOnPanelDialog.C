/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXStrings.h"
#include "Application.h"
#include "HelpOnPanelDialog.h"
#include "ControlPanel.h"

boolean HelpOnPanelDialog::ClassInitialized = FALSE;

String HelpOnPanelDialog::DefaultResources[] =
{
        "*dialogTitle:     		Help on Control Panel...",
        NULL
};

HelpOnPanelDialog::HelpOnPanelDialog(Widget parent, ControlPanel *cp) : 
				SetPanelCommentDialog("helpOnPanel",
				parent, TRUE, cp)
{
    if (NOT HelpOnPanelDialog::ClassInitialized)
    {
        HelpOnPanelDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

HelpOnPanelDialog::~HelpOnPanelDialog()
{
}
char *HelpOnPanelDialog::getDialogTitle()
{
    const char *title = this->controlPanel->getWindowTitle();
    char *dialogTitle = NULL;

    if (title) {
        int len = STRLEN(title) + STRLEN(" Help On Control Panel...") + 16;
        dialogTitle = new char[len];
        sprintf(dialogTitle,"Help On %s...",title);
    }
    return dialogTitle;
}


//
// Install the default resources for this class.
//
void HelpOnPanelDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, HelpOnPanelDialog::DefaultResources);
    this->SetPanelCommentDialog::installDefaultResources( baseWidget);
}
