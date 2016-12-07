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
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>

#include "Application.h"
#include "FileSelectorNode.h"
#include "FileSelectorInteractor.h"
#include "FileSelectorDialog.h"
#include "InteractorInstance.h"
#include "FileSelectorInstance.h"
#include "ErrorDialogManager.h"

#define TEXT_COLUMNS	16

boolean FileSelectorInteractor::ClassInitialized = FALSE;

String FileSelectorInteractor::DefaultResources[] =
{
    ".allowHorizontalResizing:                  True",
     "*fsdButton.labelString:			...",
     "*fsdButton.bottomOffset:			2",
     "*fsdButton.topOffset:			2",
     "*fsdButton.rightOffset:			3",
     "*textEditor.bottomOffset:			2",
     "*textEditor.topOffset:			2",
     "*textEditor.leftOffset:			3",
     "*pinLeftRight:				True",
     NUL(char*)
};

FileSelectorInteractor::FileSelectorInteractor(const char * name, 
				InteractorInstance *ii) : 
				ValueInteractor(name, ii) 
{ 
    this->fileSelectorDialog = NULL; 
    this->fsdButton = NULL; 
}

FileSelectorInteractor::~FileSelectorInteractor()
{
    if (this->fileSelectorDialog)
	delete this->fileSelectorDialog;
}
//
// Allocate the interactor. 
//
Interactor *FileSelectorInteractor::AllocateInteractor(const char *name,
					InteractorInstance *ii)
{
    FileSelectorInteractor *i = new FileSelectorInteractor(name,ii);
    return (Interactor*)i;
}

void FileSelectorInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT FileSelectorInteractor::ClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  FileSelectorInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ValueInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        FileSelectorInteractor::ClassInitialized = TRUE;
    }
}
//
// Put the super class's interactive part in a form with a button that
// opens the file selection box.
//
Widget FileSelectorInteractor::createInteractivePart(Widget form)
{
    // FIXME: install modifyVerify callback to ensure that file exists.
    Widget text = this->ValueInteractor::createInteractivePart(form);
    XtVaSetValues(text, XmNleftAttachment, XmATTACH_FORM,
    			XmNtopAttachment, XmATTACH_FORM,
    			XmNbottomAttachment, XmATTACH_FORM,
#if (XmVersion < 1001)
			XmNcolumns,               4*TEXT_COLUMNS,
#else
			XmNcolumns,               TEXT_COLUMNS,
#endif
                        NULL);

    this->fsdButton = XtVaCreateManagedWidget("fsdButton",
                        xmPushButtonWidgetClass,     form,
    			XmNrightAttachment, XmATTACH_FORM,
    			XmNtopAttachment, XmATTACH_FORM,
    			XmNbottomAttachment, XmATTACH_FORM,
                        NULL);

    XtVaSetValues(text, XmNrightAttachment, XmATTACH_WIDGET,
			XmNrightWidget, this->fsdButton,
			XmNrightOffset, 10,
    			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

    XtAddCallback(this->fsdButton,
			XmNactivateCallback, 
			(XtCallbackProc)FileSelectorInteractor_ButtonCB,
			(XtPointer)this);

    return form; 

}
//
// Calls the virtual method this->buttonCallback() when the 
// button is pressed.
//
extern "C" void FileSelectorInteractor_ButtonCB(Widget w, 
				XtPointer clientData,
				XtPointer callData)
{
      FileSelectorInteractor *fsi = (FileSelectorInteractor*) clientData;
      fsi->buttonCallback(w,callData);
}
//
// Opens/closes the interactor's file selector dialog.
//
void FileSelectorInteractor::buttonCallback(Widget w, XtPointer callData)
{
    InteractorInstance *ii = this->interactorInstance;
    boolean first = FALSE;
    if (!this->fileSelectorDialog) {
	first = TRUE;
	this->fileSelectorDialog = new FileSelectorDialog(
					this->getRootWidget(),
					(FileSelectorInstance*)ii);
    }

    FileSelectorDialog *fsd = this->fileSelectorDialog;
    fsd->post();

    if (first) {
	const char *labelString = ii->getInteractorLabel(); 
	if (labelString) {
	    char *s = this->FilterLabelString(labelString);
	    fsd->setDialogTitle(s);
	    delete s;
	}
    }
}

void FileSelectorInteractor::completeInteractivePart()
{
    this->ValueInteractor::completeInteractivePart();

    //
    // Make the fsdButton the same height as the text widget
    //
    Dimension height;
    XtVaGetValues(this->textEditor, XmNheight, &height, NULL);
    XtVaSetValues(this->fsdButton, 
	XmNheight,		height, 
	XmNrecomputeSize,	False, 
	NULL);
}
//
// Update the FileSelectorInteractor's title to match that of
// this interactor. 
//
void FileSelectorInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    this->updateDisplayedInteractorValue();
}
//
// Update the text value from the instance's notion of the current text.
// This is the same as ValueInteractor except that we strip of the
// double quotes and right justify the text.
//
void FileSelectorInteractor::updateDisplayedInteractorValue()
{
    InteractorInstance *ii = (InteractorInstance*)this->interactorInstance;
    Node *vnode = ii->getNode();
    const char *v = vnode->getOutputValueString(1);
    int	len;

#ifdef RM_QUOTES
    char *p = strchr(v,'"');
    if (p && strchr(p+1,'"')) {
	p = DuplicateString(p+1);	
	char *c = strchr(p,'"');
	ASSERT(c);
	*c = '\0';
    	this->installNewText(p);
        len = STRLEN(p);
	delete p;
    } else {
    	this->installNewText(v);
        len = STRLEN(v);
    } 
#else
    this->ValueInteractor::updateDisplayedInteractorValue();
    len = STRLEN(v);
#endif

    //
    // Right justify the text 
    //
    XmTextShowPosition(this->textEditor,len);
    XmTextSetInsertionPosition(this->textEditor,len);
}
//
// After calling the super class method, set the label on the 
// FileSelectorDialog if we have one. 
//
void FileSelectorInteractor::setLabel(const char *labelString, 
					boolean re_layout)
{
    this->ValueInteractor::setLabel(labelString,re_layout);
    if (this->fileSelectorDialog)  {
	char *s = this->FilterLabelString(labelString);
	this->fileSelectorDialog->setDialogTitle(s);
	delete s;
    }
}


