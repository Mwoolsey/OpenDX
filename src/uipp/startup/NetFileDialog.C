/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "NetFileDialog.h"
#include "Application.h"
#include "StartupWindow.h"
#include <Xm/FileSB.h>

boolean NetFileDialog::ClassInitialized = FALSE;

String NetFileDialog::DefaultResources[] = {
    ".dialogTitle:		Net File Selection",
    ".dirMask:			*.net",
    "*helpLabelString:		New",
    NUL(char*)
};

NetFileDialog::NetFileDialog (Widget parent, StartupWindow *suw) :
    FileDialog ("netFileDialog", parent)
{
    this->suw = suw;

    if (!NetFileDialog::ClassInitialized) {
	this->installDefaultResources(theApplication->getRootWidget());
	NetFileDialog::ClassInitialized = TRUE;
    }
}

void NetFileDialog::installDefaultResources (Widget baseWidget)
{
    this->setDefaultResources (baseWidget, NetFileDialog::DefaultResources);
    this->FileDialog::installDefaultResources(baseWidget);
}

NetFileDialog::~NetFileDialog(){}


void NetFileDialog::okFileWork (const char *string)
{
    this->suw->startNetFile(string);
}

Widget NetFileDialog::createDialog(Widget parent)
{
    Widget dialog = this->FileDialog::createDialog(parent);

    this->showNewOption();

    return dialog;
}

void NetFileDialog::showNewOption(boolean show)
{
    //
    // Manage the help button, and rename it
    //
    Widget helpButton = XmFileSelectionBoxGetChild(this->fsb, XmDIALOG_HELP_BUTTON);
#if 0
    if (show)
	XtManageChild(helpButton);
    else
#endif
	XtUnmanageChild(helpButton);
}

void NetFileDialog::helpCallback (Dialog*)
{
    this->unmanage();
    this->okFileWork(NUL(char*));
}
