/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Xm.h>

#include "ImageFileDialog.h"
#include "Application.h"
#include "DXStrings.h"
#include "ErrorDialogManager.h"
#include "SaveImageDialog.h"

String ImageFileDialog::DefaultResources[] = {
    ".dialogStyle:	XmDIALOG_MODELESS",
    "*dialogTitle:	Save Image to File...",
    NUL(char*)
};

boolean ImageFileDialog::ClassInitialized = FALSE;

ImageFileDialog::ImageFileDialog(Widget parent, SaveImageDialog* dialog) :
                       FileDialog("imageFileSelector", parent)
{
    this->sid = dialog;
}

void ImageFileDialog::installDefaultResources(Widget  baseWidget)
{
    if (ImageFileDialog::ClassInitialized) return ;
    ImageFileDialog::ClassInitialized = TRUE;
    this->setDefaultResources(baseWidget, ImageFileDialog::DefaultResources);
    this->FileDialog::installDefaultResources( baseWidget);
}

void ImageFileDialog::okFileWork(const char *string)
{
    this->sid->setFilename(string, FALSE);
    this->sid->setCommandActivation();
}

void ImageFileDialog::cancelCallback (Dialog *)
{
    this->unmanage();
    this->sid->setCommandActivation();
}

Widget ImageFileDialog::createDialog (Widget parent)
{
    this->installDefaultResources (theApplication->getRootWidget());
    return this->FileDialog::createDialog(parent);
}
