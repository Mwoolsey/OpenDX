/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "Application.h"
#include "SetNetworkCommentDialog.h"
#include "Network.h"

boolean SetNetworkCommentDialog::ClassInitialized = FALSE;

String SetNetworkCommentDialog::DefaultResources[] =
{
        "*dialogTitle:     		Application Comment...",
	"*nameLabel.labelString:	Application Comment:",
        // 
        // These two must match what is in OpenNetCommentDialog
        // 
        "*editorText.columns:           45",  
        "*editorText.rows:              16",
        NULL
};

SetNetworkCommentDialog::SetNetworkCommentDialog(const char *name,
				Widget parent, boolean readonly, Network *n) : 
			TextEditDialog(name,parent, readonly)
{
    this->network = n;
}
SetNetworkCommentDialog::SetNetworkCommentDialog(Widget parent,
				boolean readonly, Network *n) : 
			TextEditDialog("setNetworkComment",parent, readonly)
{
    this->network = n;

    if (NOT SetNetworkCommentDialog::ClassInitialized)
    {
        SetNetworkCommentDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetNetworkCommentDialog::~SetNetworkCommentDialog()
{
}

//
// Install the default resources for this class.
//
void SetNetworkCommentDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SetNetworkCommentDialog::DefaultResources);
    this->TextEditDialog::installDefaultResources( baseWidget);
}
//
// Get the the text that is to be edited. 
//
const char *SetNetworkCommentDialog::getText()
{
    return this->network->getNetworkComment();
}

//
// Save the given text. 
//
boolean SetNetworkCommentDialog::saveText(const char *s)
{
    this->network->setNetworkComment(s);
    return TRUE;
}

