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
#include "SaveImageDialog.h"
#include "ImageNode.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "QuestionDialogManager.h"
#include "XmUtility.h"
#include "ImageFormatCommand.h"
#include "ToggleButtonInterface.h"
#include "ButtonInterface.h"
#include "ImageFileDialog.h"
#include "ImageFormat.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include "../widgets/Number.h"

boolean SaveImageDialog::ClassInitialized = FALSE;

String SaveImageDialog::DefaultResources[] =
{
    ".dialogTitle:				Save Image",
    "*saveCurrentOption.shadowThickness:     	0",
    "*saveContinuousOption.shadowThickness:    	0",
    "*saveCurrentOption.labelString:	     	Save Current",
    "*saveContinuousOption.labelString:		Continuous Saving",
    "*fileSelectOption.labelString:		Select File...",
    NULL
};


SaveImageDialog::SaveImageDialog(Widget parent,ImageNode *node, 
    CommandScope* commandScope) 
	: ImageFormatDialog("saveImageDialog", parent, node, commandScope)
{
    this->saveContinuousOption = NUL(ToggleButtonInterface*);
    this->saveCurrentOption = NUL(ToggleButtonInterface*);
    this->fileSelectOption = NUL(ButtonInterface*);
    this->sid_dirty = 0;
    this->fsb = NUL(ImageFileDialog*);
    this->file = NUL(char*);

    this->saveCurrentCmd = new ImageFormatCommand ("saveCmd", commandScope,
	TRUE, this, ImageFormatCommand::SaveCurrent);

    this->saveContinuousCmd = new ImageFormatCommand ("saveCmd", commandScope,
	FALSE, this, ImageFormatCommand::SaveContinuous);

    this->fileSelectCmd = new ImageFormatCommand ("selectFile", commandScope,
	FALSE, this, ImageFormatCommand::SelectFile);

    if (!SaveImageDialog::ClassInitialized) {
	SaveImageDialog::ClassInitialized = TRUE;
	this->installDefaultResources (theApplication->getRootWidget());
    }
}

void SaveImageDialog::installDefaultResources (Widget baseWidget)
{
    this->setDefaultResources (baseWidget, SaveImageDialog::DefaultResources);
    this->ImageFormatDialog::installDefaultResources (baseWidget);
}

SaveImageDialog::~SaveImageDialog()
{
    if (this->fileSelectOption) delete this->fileSelectOption;
    if (this->saveCurrentOption) delete this->saveCurrentOption;
    if (this->saveContinuousOption) delete this->saveContinuousOption;

    if (this->fileSelectCmd) delete this->fileSelectCmd;
    if (this->saveCurrentCmd) delete this->saveCurrentCmd;
    if (this->saveContinuousCmd) delete this->saveContinuousCmd;

    if (this->fsb) delete this->fsb;
    if (this->file) delete this->file;
}

Widget SaveImageDialog::createControls (Widget parent)
{
    Widget body = XtVaCreateManagedWidget ("saveImageBody",
	xmFormWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
    NULL);
    XmString xmstr = XmStringCreateLtoR ("Output file name:", "small_bold");
    XtVaCreateManagedWidget ("nameLabel",
	xmLabelWidgetClass,	body,
	XmNlabelString,		xmstr,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		4,
	XmNtopOffset,		0,
    NULL);
    XmStringFree (xmstr);
    this->file_name = XtVaCreateManagedWidget("fileName",
	xmTextWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		4,
	XmNtopOffset,		18,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		105,
    NULL);
    XtAddCallback (this->file_name, XmNmodifyVerifyCallback, (XtCallbackProc)
	SaveImageDialog_ModifyCB, (XtPointer)this);
    this->fileSelectOption = new ButtonInterface 
	(body, "fileSelectOption", this->fileSelectCmd);
    XtVaSetValues (this->fileSelectOption->getRootWidget(),
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		5,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->file_name,
	XmNtopOffset,		4,
    NULL);

    this->saveCurrentOption = new ToggleButtonInterface(body, "saveCurrentOption",
	this->saveCurrentCmd, FALSE);
    XtVaSetValues (this->saveCurrentOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->file_name,
	XmNtopOffset,		10,
    NULL);
    this->saveContinuousOption = new ToggleButtonInterface(body, "saveContinuousOption",
	this->saveContinuousCmd, FALSE);
    XtVaSetValues (this->saveContinuousOption->getRootWidget(),
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->file_name,
	XmNtopOffset,		10,
    NULL);

    return body;
}

void SaveImageDialog::restoreCallback()
{
    this->sid_dirty = 0;
    if (!this->node->isRecordFileConnected()) 
	this->node->unsetRecordFile (FALSE);
    if (!this->node->isRecordEnableConnected()) 
	this->node->unsetRecordEnable(FALSE);
    this->saveCurrentOption->setState(FALSE, TRUE);
    this->saveContinuousOption->setState(FALSE, TRUE);
    this->ImageFormatDialog::restoreCallback();
}

boolean SaveImageDialog::okCallback(Dialog *dialog)
{
    boolean should_send = FALSE;
    this->ImageFormatDialog::okCallback(dialog);

    const char *fname = this->getOutputFile();
    //
    // Check for file existence and query the user for overwriting if necessary.
    // If we're on unix, then don't check for the file unless it starts with '/
    // because the exec is writing the file and it's cwd will probably not be
    // the same as ours.
    //
    if ((!fname) || (!fname[0]) || (IsBlankString(fname))) {
	WarningMessage ("A file name is required.");
	return FALSE;
    }

    int response = QuestionDialogManager::OK;
    char abspath = '/';
    char *full_filename = NUL(char*);
#if !defined(DXD_WIN)
    if (fname[0] == '"') abspath = fname[1];
    else abspath = fname[0];
#endif
    if ((abspath == '/') && (this->saveCurrentOption->getState())) {
	const char *ext = this->choice->fileExtension();
	full_filename = new char[strlen(fname) + strlen(ext) + 1];
	if (fname[0] == '"') strcpy (full_filename, &fname[1]);
	else strcpy (full_filename, fname);
	int quote_spot = strlen(full_filename) - 1;
	if (full_filename[quote_spot] == '"')  full_filename[quote_spot] = '\0';
	if (!strstr (full_filename, ext))
	    strcat (full_filename, ext);

	struct STATSTRUCT buffer;
	if (STATFUNC(full_filename, &buffer) == 0) {
	    Widget parent = XtParent(this->getRootWidget());
	    char *title = "Save Confirmation";
	    if (this->choice->supportsAppend()) {
		response = theQuestionDialogManager->userQuery(
		    parent, "Append to existing file?", title,
		    "Append", "Overwrite", "Cancel", 3
		);
	    } else {
		response = theQuestionDialogManager->userQuery(
		    parent, "Overwrite existing file?", title,
		    "Overwrite", "Cancel", NULL, 2
		);
	    }
	}
    }

#   define SAVE_CAN_PROCEED 1
#   define CANCEL_SAVE  2
#   define ERASE_THEN_PROCEED 3

    int action = 0;
    switch (response) {
	case QuestionDialogManager::OK:
	    // mod-writeimage will erase the file if it doesn't support appending.
	    // mod-writeimage will append to the file if it does support appending.
	    action = SAVE_CAN_PROCEED; 
	    break;
	case QuestionDialogManager::Cancel:
	    if (this->choice->supportsAppend())
		action = ERASE_THEN_PROCEED; 
	    else
		action = CANCEL_SAVE;
	    break;
	case QuestionDialogManager::Help:
	    if (this->choice->supportsAppend())
		action = CANCEL_SAVE;
	    else
		ASSERT(0);
	    break;
    }

    ASSERT(action);
    switch (action) {
	case CANCEL_SAVE:
	    return FALSE;
	    break;
	case ERASE_THEN_PROCEED:
	    ASSERT ((full_filename) && (full_filename[0]));
	    this->choice->eraseOutputFile(full_filename);
	case SAVE_CAN_PROCEED:
	    break;
    }
    if (full_filename) delete full_filename;

    if (!this->node->isRecordFileConnected()) {
	if (this->sid_dirty & SaveImageDialog::DirtyFilename) {
	    this->node->setRecordFile (fname, FALSE);
	    should_send = TRUE;
	}
    }
    this->sid_dirty&= ~SaveImageDialog::DirtyFilename;


    if (!this->node->isRecordEnableConnected()) {
	if (this->sid_dirty & SaveImageDialog::DirtyContinuous) {
	    if (this->saveContinuousOption->getState()) 
		this->node->setRecordEnable(TRUE, FALSE);
	    else 
		this->node->setRecordEnable(FALSE, FALSE);
	    should_send = TRUE;
	}
    }
    this->sid_dirty&= ~SaveImageDialog::DirtyContinuous;

    if (this->saveCurrentOption->getState()) {
	this->currentImage();
	this->saveCurrentOption->setState (FALSE, TRUE);
    }
    this->sid_dirty&= ~SaveImageDialog::DirtyCurrent;

    if (should_send)
	this->getNode()->sendValues(TRUE);

    return FALSE;
}


//
// Control greying out of options due to data-driving.  Certain toggle button
// values imply certain others so set them too.  e.g. push rerender implies
// both size toggle pushed and resolution toggle pushed.
//
void SaveImageDialog::setCommandActivation()
{
    //
    // Record enable / continuous saving
    //
    int recenab;
    this->node->getRecordEnable(recenab);
    boolean enab = (boolean)recenab;
    if (this->node->isRecordEnableConnected()) {
	this->saveContinuousCmd->deactivate();
	this->saveContinuousOption->setState (enab, TRUE);
	this->sid_dirty&= ~SaveImageDialog::DirtyContinuous;
    } else {
	this->saveContinuousCmd->activate();
	if ((this->sid_dirty & SaveImageDialog::DirtyContinuous) == 0) 
	    this->saveContinuousOption->setState (enab, TRUE);
    }


    //
    // Record file
    //
    const char *value = NUL(char*);
    this->node->getRecordFile (value);
    if (this->node->isRecordFileConnected()) {
	this->setFilename(value);
	this->sid_dirty&= ~SaveImageDialog::DirtyFilename;
	this->fileSelectCmd->deactivate();
	if (this->fsb) this->fsb->unmanage();
    } else {
	int mask = SaveImageDialog::DirtyFilename;
	int dirt = (this->sid_dirty&mask);
	if (dirt == 0)
	    this->setFilename(value);
	if (this->fsb) {
	    if (this->fsb->isManaged())
		this->fileSelectCmd->deactivate();
	    else
		this->fileSelectCmd->activate();
	} else {
	    this->fileSelectCmd->activate();
	}
    }
    XtSetSensitive (this->file_name, this->fileSelectCmd->isActive());
    //this->setTextSensitive (this->file_name, this->fileSelectCmd->isActive());

    this->ImageFormatDialog::setCommandActivation();
}


void SaveImageDialog::setFilename(const char* filename, boolean skip_callbacks)
{
    if (this->file) delete this->file;
    this->file = DuplicateString(filename);

    if (!this->file_name) return ;
    if (skip_callbacks) {
	XtRemoveCallback (this->file_name, XmNmodifyVerifyCallback,
	    (XtCallbackProc)SaveImageDialog_ModifyCB, (XtPointer)this);
    }
    if (!this->file) 
	XmTextSetString (this->file_name, "");
    else
	XmTextSetString (this->file_name, this->file);
    if (skip_callbacks) {
	XtAddCallback (this->file_name, XmNmodifyVerifyCallback,
	    (XtCallbackProc)SaveImageDialog_ModifyCB, (XtPointer)this);
    }
}

const char *SaveImageDialog::getOutputFile()
{
    if (this->file_name) {
	if (this->file) delete this->file;
	char *cp = XmTextGetString(this->file_name);
#ifdef DXD_WIN
	for(int i=0; i<strlen(cp); i++)
		if(cp[i] == '\\') cp[i] = '/';
#endif
	this->file = DuplicateString(cp);
	XtFree(cp);
    }
    return this->file;
}

boolean SaveImageDialog::postFileSelectionDialog()
{
    if (!this->fsb)
	this->fsb = new ImageFileDialog (this->getRootWidget(), this);
    this->fsb->post();
    XSync (XtDisplay(this->getRootWidget()), False);
    this->setCommandActivation();
    return TRUE;
}

boolean SaveImageDialog::dirtyCurrent()
{
    this->setCommandActivation();
    return TRUE;
}

extern "C" {

void SaveImageDialog_ModifyCB (Widget , XtPointer clientData, XtPointer)
{
    SaveImageDialog* sid = (SaveImageDialog*)clientData;
    ASSERT(sid);
    sid->sid_dirty|= SaveImageDialog::DirtyFilename;
}

} // end extern C
