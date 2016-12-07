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
#include "SetImageNameDialog.h"

#include "Application.h"
#include "DisplayNode.h"
#include "ImageWindow.h"
#include "Network.h"
#include "ErrorDialogManager.h"

boolean SetImageNameDialog::ClassInitialized = FALSE;
String SetImageNameDialog::DefaultResources[] =
{
    "*dialogTitle:               	Change Image Name...", 
    "*nameLabel.labelString:            Image Name:", 
    NULL
};


SetImageNameDialog::SetImageNameDialog(ImageWindow *iw) :
    SetNameDialog("setImageNameDialog", iw->getRootWidget())
{
    this->imageWindow = iw;

    if (NOT SetImageNameDialog::ClassInitialized)
    {
        SetImageNameDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetImageNameDialog::~SetImageNameDialog()
{
}

//
// Install the default resources for this class.
//
void SetImageNameDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetImageNameDialog::DefaultResources);
    this->SetNameDialog::installDefaultResources( baseWidget);
}

const char *SetImageNameDialog::getText()
{
    ImageWindow *iw = this->imageWindow;
    DisplayNode *n = (DisplayNode*)iw->getAssociatedNode();
    return n->getTitle();
}

//
// Permit empty strings.  This should set the title param so that it's defaulting
// and do resetWindowTitle on the ImageWindow.
//
boolean SetImageNameDialog::saveText(const char *s)
{
#if 0
    if (IsBlankString(s)) {
	ModalErrorMessage(this->getRootWidget(),
		"The name string cannot be blank.");
        return FALSE;
    } else {
#endif
	ImageWindow *iw = this->imageWindow;
	DisplayNode *n = (DisplayNode*)iw->getAssociatedNode();
    	n->setTitle(s);
    	n->getNetwork()->setFileDirty();
#if 0
    }
#endif
    return TRUE;
}

