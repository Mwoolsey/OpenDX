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
#include "LoadMDFDialog.h"
#include "MacroDefinition.h"


boolean LoadMDFDialog::ClassInitialized = FALSE;

String LoadMDFDialog::DefaultResources[] =
{
        "*dialogTitle:     Load Module Description...",
        "*.dirMask:         *.mdf",
        "*helpLabelString: Comments",
        NULL
};

LoadMDFDialog::LoadMDFDialog(Widget parent, DXApplication *ap) : 
                       FileDialog("loadMDFDialog", parent)
{
    this->hasCommentButton = FALSE;
    this->dxApp = ap;

    if (NOT LoadMDFDialog::ClassInitialized)
    {
        LoadMDFDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
LoadMDFDialog::LoadMDFDialog(char *name, Widget parent, DXApplication *ap) : 
                       FileDialog(name, parent)
{
    this->hasCommentButton = FALSE;
    this->dxApp = ap;
}

//
// Install the default resources for this class.
//
void LoadMDFDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, LoadMDFDialog::DefaultResources);
    this->FileDialog::installDefaultResources( baseWidget);
}

void LoadMDFDialog::helpCallback(Dialog* dialog)
{
    printf("help callback \n");
}

void LoadMDFDialog::okFileWork(const char *string)
{
    Dictionary d;
    this->dxApp->loadUDF(string, &d, TRUE);
}

