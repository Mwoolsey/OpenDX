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
#include "SetBGColorDialog.h"
#include "ImageWindow.h"
#include "Application.h"

boolean SetBGColorDialog::ClassInitialized = FALSE;
String  SetBGColorDialog::DefaultResources[] =
{
    "*dialogTitle:                      Change Background Color...",
    "*nameLabel.labelString:            Background Color:",
    NULL
};
SetBGColorDialog::SetBGColorDialog(ImageWindow *image) :
    SetNameDialog("setBGColorDialog", image->getRootWidget())
{
    this->imageWindow = image;

    if (NOT SetBGColorDialog::ClassInitialized)
    {
        SetBGColorDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetBGColorDialog::~SetBGColorDialog()
{
}


//
// Install the default resources for this class.
//
void SetBGColorDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetBGColorDialog::DefaultResources);
    this->SetNameDialog::installDefaultResources( baseWidget);
}
boolean SetBGColorDialog::saveText(const char *s)
{
    if (s && !IsBlankString(s))
	this->imageWindow->setBackgroundColor(s);
    else
	this->imageWindow->setBackgroundColor(NULL);

    return TRUE;
}

const char *SetBGColorDialog::getText()
{
    return this->imageWindow->getBackgroundColor();
}
