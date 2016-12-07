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
#include "FileSelectorDialog.h"
#include "FileSelectorNode.h"
#include "FileSelectorInstance.h"
//#include "ErrorDialogManager.h"



boolean FileSelectorDialog::ClassInitialized = FALSE;

String FileSelectorDialog::DefaultResources[] =
{
        "*dialogTitle:     	File Selector...",
        "*dirMask:         	*.dx",
        "*cancelLabelString:    Close",
        ".background:		#b4b4b4b4b4b4",
        NULL
};


FileSelectorDialog::FileSelectorDialog(
			Widget parent, FileSelectorInstance *fsi) :
			ApplyFileDialog("fileSelectorDialog",parent)
{
    this->fsInstance = fsi;

    if (NOT FileSelectorDialog::ClassInitialized)
    {
        FileSelectorDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Load this class's default resources and do any other initializations.
//
//
// Install the default resources for this class.
//
void FileSelectorDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, FileSelectorDialog::DefaultResources);
    this->ApplyFileDialog::installDefaultResources( baseWidget);
}

//
// Call super class and then set the dialog title and the dirMask resource. 
//
Widget FileSelectorDialog::createDialog(Widget parent)
{

    FileSelectorInstance *fsi = this->fsInstance;
    Widget w = this->ApplyFileDialog::createDialog(parent);

    // Set the root  now for setDialogTitle();
    this->setDialogTitle(fsi->getInteractorLabel());

    const char *filter = fsi->getFileFilter();
    if (filter) {
	XmString string = XmStringCreateLtoR((char*)filter,
						XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(this->fsb,XmNdirMask,string, NULL);
	XmStringFree(string);
    }

    //
    // Add a callback to catch filter changes
    //
    XtAddCallback(this->fsb,
		XmNapplyCallback, (XtCallbackProc)FileSelectorDialog_FilterChangeCB, 
		(XtPointer)this);
		
    return w;
}
//
// Capture and save a filter change with the instance.
//
extern "C" void FileSelectorDialog_FilterChangeCB(Widget w, 
		XtPointer clientData, XtPointer callData)
{
    FileSelectorDialog *fsd = (FileSelectorDialog*)clientData;
    fsd->filterChangeCallback();
}
void FileSelectorDialog::filterChangeCallback()
{
    FileSelectorInstance *fsi = this->fsInstance;
    XmString string;
    char *dirMask;
    XtVaGetValues(this->fsb, XmNdirMask, &string, NULL);
    XmStringGetLtoR(string,XmSTRING_DEFAULT_CHARSET, &dirMask); 
    fsi->setFileFilter(dirMask);
    XtFree(dirMask);
}

//
// Set the output value of  the interactor to the given string.
//
void FileSelectorDialog::okFileWork(const char *string)
{
    InteractorInstance *ii = this->fsInstance;
    FileSelectorNode *fsn = (FileSelectorNode*)ii->getNode();
    fsn->setOutputValue(1,string);
}
