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

#include "Application.h"
#include "ColormapEditor.h"
#include "OpenColormapDialog.h"
#include "ErrorDialogManager.h"


boolean OpenColormapDialog::ClassInitialized = FALSE;

String OpenColormapDialog::DefaultResources[] =
{
        "*dialogTitle:     Open Colormap...",
        "*dirMask:         *.cm",
        NULL
};



void OpenColormapDialog::okFileWork(const char *string)
{
    boolean r;
    if (this->opening)
	r = this->editor->cmOpenFile((char*)string);
    else
	r = this->editor->cmSaveFile((char*)string);

    if (!r) {
	ErrorMessage("Cannot open file: %s.\n", string);
    }
}

OpenColormapDialog::OpenColormapDialog( Widget parent, 
                                       ColormapEditor* editor,
                                       int opening) : 
                       FileDialog("openColormapDialog", parent)
{

    this->editor = editor;
    this->opening = opening;

    //XMapRaised(XtDisplay(this->getRootWidget()),
    //           XtWindow(this->getRootWidget()));
 
    if (NOT OpenColormapDialog::ClassInitialized)
    {
        OpenColormapDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

}

//
// Install the default resources for this class.
//
void OpenColormapDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, OpenColormapDialog::DefaultResources);
    this->FileDialog::installDefaultResources( baseWidget);
}
void OpenColormapDialog::setMode(boolean opening)
{
    this->opening = opening;
}
//
// Set the correct title 'Open/Close Colormap...' 
//
void OpenColormapDialog::manage()
{
    XmString   xmstring;

    if (opening)
	xmstring = XmStringCreateSimple("Open Colormap...");
    else
	xmstring = XmStringCreateSimple("Save Colormap...");

    XtVaSetValues(this->fsb,
		  XmNdialogTitle, xmstring,
		  NULL);
    XmStringFree(xmstring);

    this->FileDialog::manage();
}
