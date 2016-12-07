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
#include "OpenNetCommentDialog.h"

boolean OpenNetCommentDialog::ClassInitialized = FALSE;

String OpenNetCommentDialog::DefaultResources[] =
{
       "*dialogTitle:		Application Comment..." ,
       "*nameLabel.labelString:	Application Comment:" ,
        //
        // These two must match what is in SetNetworkCommentDialog
        //
        "*editorText.columns:           45",  
        "*editorText.rows:              16",

	NULL
};

OpenNetCommentDialog::OpenNetCommentDialog(Widget parent)
			: TextEditDialog("openNetComment", parent, TRUE)
{
    this->dialogModality = XmDIALOG_APPLICATION_MODAL;

    if (NOT OpenNetCommentDialog::ClassInitialized)
    {
        OpenNetCommentDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

OpenNetCommentDialog::~OpenNetCommentDialog()
{
}

//
// Install the default resources for this class.
//
void OpenNetCommentDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				OpenNetCommentDialog::DefaultResources);
    this->TextEditDialog::installDefaultResources( baseWidget);
}
