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
#include "DXApplication.h"
#include "QuestionDialogManager.h"
#include "PrintImageDialog.h"
#include "ImageNode.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "XmUtility.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include "../widgets/Number.h"

boolean PrintImageDialog::ClassInitialized = FALSE;

#define DEFAULT_COMMAND "lpr -P"

String PrintImageDialog::DefaultResources[] =
{
    "*onceButton.labelString:	     Save Current",
    "*toggleButton.labelString:	     Continuous Saving",
    ".dialogTitle:		     Print Image",
    NULL
};


PrintImageDialog::PrintImageDialog(Widget parent,ImageNode *node, 
    CommandScope* commandScope) 
	: ImageFormatDialog("printImageDialog", parent, node, commandScope)
{

    this->command_str = DuplicateString (DEFAULT_COMMAND);
    this->command = NUL(Widget);

    if (!PrintImageDialog::ClassInitialized) {
	PrintImageDialog::ClassInitialized = TRUE;
	this->installDefaultResources (theApplication->getRootWidget());
    }
}

void PrintImageDialog::installDefaultResources (Widget baseWidget)
{
    this->setDefaultResources (baseWidget, PrintImageDialog::DefaultResources);
    this->ImageFormatDialog::installDefaultResources (baseWidget);
}

PrintImageDialog::~PrintImageDialog()
{
    if (this->command_str) delete this->command_str;
}

Widget PrintImageDialog::createControls (Widget parent)
{
    Widget body = XtVaCreateManagedWidget ("saveImageBody",
	xmFormWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
    NULL);
    XmString xmstr = XmStringCreateLtoR ("Print command:", "small_bold");
    XtVaCreateManagedWidget ("nameLabel",
	xmLabelWidgetClass,     body,
	XmNlabelString,         xmstr,
	XmNtopAttachment,       XmATTACH_FORM,
	XmNleftAttachment,      XmATTACH_FORM,
	XmNleftOffset,          4,
	XmNtopOffset,           0,
    NULL);
    XmStringFree (xmstr);
    this->command = XtVaCreateManagedWidget("command",
	xmTextWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopOffset,		18,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		10,
    NULL);
    if (this->command_str) 
	XmTextSetString (this->command, this->command_str);

    return body;
}


boolean PrintImageDialog::okCallback(Dialog *dialog)
{
    this->ImageFormatDialog::okCallback (dialog);
    this->currentImage();
    return FALSE;
}



//
// Control greying out of options due to data-driving.  Certain toggle button
// values imply certain others so set them too.  e.g. push rerender implies
// both size toggle pushed and resolution toggle pushed.
//
void PrintImageDialog::setCommandActivation()
{
    this->ImageFormatDialog::setCommandActivation();
}

const char* PrintImageDialog::getOutputFile()
{
static char cmd[512];

    if (this->command) {
	if (this->command_str) delete this->command_str;
	char *cp = XmTextGetString (this->command);
	this->command_str = DuplicateString(cp);
	XtFree(cp);
    }

    sprintf(cmd, "!%s", this->command_str);
    return cmd;
}

void PrintImageDialog::restoreCallback()
{
    if (this->command_str) delete this->command_str;
    this->command_str = DuplicateString(DEFAULT_COMMAND);
    if (this->command)
	XmTextSetString (this->command, this->command_str);
    this->ImageFormatDialog::restoreCallback();
}
