/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>

#include "DXStrings.h"
#include "MoveNodesDialog.h"
#include "PageSelector.h"
#include "TextSelector.h"
#include "EditorWorkSpace.h"
#include "EditorWindow.h"
#include "Command.h"
#include "CommandScope.h"
#include "ButtonInterface.h"

#include "DXApplication.h"
#include "ErrorDialogManager.h"

#include "DictionaryIterator.h"
#include "XmUtility.h"

boolean MoveNodesDialog::ClassInitialized = FALSE;
String MoveNodesDialog::DefaultResources[] =
{
    "*nameLabel.labelString:            Page Name:",
    "*nameLabel.foreground:             SteelBlue",
    "*textSelector.maxLength:	   	12",
    "*textSelector.columns:	   	12",
    "*okButton.labelString:             OK",
    "*okButton.width:                   60",
    "*applyButton.labelString:          Apply",
    "*applyButton.width:                60",
    "*cancelButton.labelString:         Cancel",
    "*cancelButton.width:               60",
    "*accelerators:           		#augment\n"
    "<Key>Return:                   	BulletinBoardReturn()",
    NULL
};


MoveNodesDialog::MoveNodesDialog(Widget parent, PageSelector* psel):
    Dialog("setPageNameDialog", parent)
{
    this->selector = psel;
    this->selector_menu = NUL(TextSelector*);
    this->page_name = NUL(char*);
    this->stop_updates = FALSE;
    this->apply_cmd = NUL(Command*);
    this->ok_option = NUL(ButtonInterface*);
    this->apply_option = NUL(ButtonInterface*);
    this->scope = new CommandScope;
    if (MoveNodesDialog::ClassInitialized == FALSE) {
        MoveNodesDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

    this->apply_cmd = new MoveNodesCommand("move", this->scope, TRUE, this);
}

//
// Install the default resources for this class.
//
void MoveNodesDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, MoveNodesDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

MoveNodesDialog::~MoveNodesDialog()
{
    if (this->page_name) delete this->page_name;
    if (this->apply_cmd) delete this->apply_cmd;
}


Widget MoveNodesDialog::createDialog(Widget parent)
{
    Arg arg[10];
    int n = 0;
    XtSetArg(arg[n], XmNautoUnmanage, False); n++;
    XtSetArg(arg[n], XmNminWidth, 340); n++;
    XtSetArg(arg[n], XmNminHeight, 90); n++;
    XtSetArg(arg[n], XmNwidth, 340); n++;
    Widget dialog = this->CreateMainForm(parent, this->name, arg, n);

    XtVaSetValues(XtParent(dialog), XmNtitle, "Move Nodes to Page...", NULL);

    Widget w = XtVaCreateManagedWidget("nameLabel", 
	xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
    NULL);

    this->selector_menu = new TextSelector; 
    this->selector_menu->createTextSelector(dialog, (XtCallbackProc)
	MoveNodesDialog_ModifyNameCB, (XtPointer)this);
    this->selector_menu->manage();
    Widget selm = this->selector_menu->getRootWidget();
    XtVaSetValues(selm,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 7,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
    NULL);

    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , selm,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 0,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 0,
    NULL);

    this->ok_option = new ButtonInterface(dialog, "okButton", this->apply_cmd);
    this->ok = this->ok_option->getRootWidget();
    XtVaSetValues(this->ok,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 10,
	NULL);

    this->apply_option = new ButtonInterface(dialog, "applyButton", this->apply_cmd);
    this->apply = this->apply_option->getRootWidget();
    XtVaSetValues(this->apply,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftOffset      , 10,
        XmNleftWidget      , this->ok,
	NULL);

    EditorWindow* editor = this->selector->getEditor();
    Command* mn = editor->getMoveSelectedCmd();
    mn->registerClient(this->ok_option);
    mn->registerClient(this->apply_option);

    this->cancel = XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
	NULL);

    return dialog;
}

//
// Must call this prior to manage()
//
void MoveNodesDialog::setWorkSpace(const char* page_name)
{
    if (this->page_name) delete this->page_name;
    this->page_name = DuplicateString(page_name);
    this->update();
}

void MoveNodesDialog::update()
{

    if (this->stop_updates) return ;
    if ((this->getRootWidget()) && (this->isManaged())) {
	//
	// Initialize the name
	//
	int size = this->selector->getSize();
	ASSERT(size > 0);
	String *names = new String[size];
	int i = 1;
	while (i <= size) {
	    const char* name = this->selector->getStringKey(i);
	    if ((!name) || (!name[0])) name = "Untitled";
	    names[i-1] = DuplicateString(name);
	    i++;
	}
	this->selector_menu->setItems(names, size);
	this->selector_menu->setSelectedItem(1);

	for (i=0; i<size; i++)
	    delete names[i];
    }
}

void MoveNodesDialog::manage()
{
    //
    // Manage first, then update so that update can check managed state to avoid
    // unnecessary work.
    //
    this->Dialog::manage();
    this->update();
}


boolean MoveNodesDialog::okCallback(Dialog *)
{
    return this->applyCallback(this);
}

boolean MoveNodesDialog::applyCallback(Dialog *)
{

    char name[64];
    this->selector_menu->getSelectedItem(name);
    //
    // If attempting to move to the current page, then just quit
    //
    if (EqualString(name, this->page_name))
	return TRUE;

    this->stop_updates = TRUE;
    //
    // Set the new name
    //
    boolean retVal = TRUE;
    EditorWorkSpace* ews = NUL(EditorWorkSpace*);
    if ((name) && (retVal))
	ews = (EditorWorkSpace*)this->selector->findDefinition(name);
    if (ews == NUL(EditorWorkSpace*)) {
	char msg[128];
	if (name)
	    sprintf (msg, "Page %s doesn't exist.", name);
	else
	    sprintf (msg, "Page doesn't exist.");
	ErrorMessage (msg);
	retVal = FALSE;
    }
    EditorWindow* editor = this->selector->getEditor();
    if ((retVal) && (editor->areSelectedNodesPagifiable(TRUE) == FALSE))
	retVal = FALSE;

    if ((retVal) && (editor->pagifySelectedNodes(ews) == FALSE)) {
	char msg[128];
	sprintf (msg, "Unable to move selected items to %s", name);
	ErrorMessage (msg);
	retVal = FALSE;
    }

    this->stop_updates = FALSE;
    if (retVal) 
	this->update();
    return retVal;
}


extern "C" {

void MoveNodesDialog_ModifyNameCB (Widget , XtPointer clientData, XtPointer)
{
}

} // end extern C


MoveNodesCommand::MoveNodesCommand(const char *name, CommandScope *scope, 
    boolean active, MoveNodesDialog *mnd): NoUndoCommand(name, scope, active)
{
    this->moveNodes = mnd;
}

boolean MoveNodesCommand::doIt(CommandInterface *ci)
{
    this->moveNodes->applyCallback(this->moveNodes);
    return TRUE;
}

