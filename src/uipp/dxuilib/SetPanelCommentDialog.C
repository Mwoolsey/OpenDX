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
#include "SetPanelCommentDialog.h"
#include "ControlPanel.h"

boolean SetPanelCommentDialog::ClassInitialized = FALSE;

String SetPanelCommentDialog::DefaultResources[] =
{
        "*dialogTitle:     		Control Panel Comment...",
	"*nameLabel.labelString:	Panel Comment:",
        NULL
};

SetPanelCommentDialog::SetPanelCommentDialog(
				const char *name, Widget parent,
				boolean readonly, ControlPanel *cp) : 
				TextEditDialog(name, parent, readonly)
{
    // Any initializations here, need to go in the other constructor also.
    this->controlPanel = cp;
}

SetPanelCommentDialog::SetPanelCommentDialog(Widget parent,
				boolean readonly, ControlPanel *cp) : 
				TextEditDialog("setPanelComment",
				parent, readonly)
{
    // Any initializations here, need to go in the other constructor also.
    this->controlPanel = cp;

    if (NOT SetPanelCommentDialog::ClassInitialized)
    {
        SetPanelCommentDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetPanelCommentDialog::~SetPanelCommentDialog()
{
}

char *SetPanelCommentDialog::getDialogTitle()
{
    const char *title = this->controlPanel->getWindowTitle();
    char *dialogTitle = NULL;

    if (title) {
        int len = STRLEN(title) + STRLEN(" Comment ...") + 16;
        dialogTitle = new char[len];
	sprintf(dialogTitle,"%s Comment...",title);
    }
    return dialogTitle;
}
//
// Install the default resources for this class.
//
void SetPanelCommentDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SetPanelCommentDialog::DefaultResources);
    this->TextEditDialog::installDefaultResources( baseWidget);
}
//
// Get the the text that is to be edited. 
//
const char *SetPanelCommentDialog::getText()
{
    return this->controlPanel->getPanelComment();
}

//
// Save the given text. 
//
boolean SetPanelCommentDialog::saveText(const char *s)
{
    this->controlPanel->setPanelComment(s);
    return TRUE;
}

