/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <limits.h>
#include <string.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <X11/X.h>

#include "lex.h"
#include "DXStrings.h"
#include "DXApplication.h"
#include "EditorWindow.h"
#include "ProcessGroupCreateDialog.h"
#include "ProcessGroupAssignDialog.h"
#include "ProcessGroupManager.h"
#include "ErrorDialogManager.h"
#include "DXPacketIF.h"
#include "XmUtility.h"

boolean ProcessGroupCreateDialog::ClassInitialized = FALSE;

String  ProcessGroupCreateDialog::DefaultResources[] = {
	"*dialogTitle:				Execution Groups...",
        "*scrolledWindow.width:                 265",
        "*scrolledWindow.height:                216",
	"*addButton.labelString:		Add To",
	"*createButton.labelString:		Create",
	"*deleteButton.labelString:		Delete",
	"*removeButton.labelString:		Remove From",
	"*removeButton.width:			100",
	"*showButton.labelString:		Show",
	"*closeButton.labelString:		Close",
	"*XmPushButton.width:			60",
	"*nameLabel.labelString:		Name:",
        NULL
};

ProcessGroupCreateDialog::ProcessGroupCreateDialog(EditorWindow *editor) 
		: Dialog("processGroupCreateDialog",editor->getRootWidget())
{
    this->editor = editor;

    if (NOT ProcessGroupCreateDialog::ClassInitialized)
    {
        ProcessGroupCreateDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

}

ProcessGroupCreateDialog::~ProcessGroupCreateDialog()
{

}

//
// Install the default resources for this class.
//
void ProcessGroupCreateDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ProcessGroupCreateDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

Widget ProcessGroupCreateDialog::createDialog(Widget parent)
{
    Arg   arg[5];
    int   n = 0;

    XtSetArg(arg[n], XmNautoUnmanage,        False);  n++;
    XtSetArg(arg[n], XmNnoResize,            False);  n++;
    XtSetArg(arg[n], XmNresizePolicy,        XmRESIZE_NONE);  n++;

    Widget mainForm = this->CreateMainForm(parent, this->name, arg, n);
 
    this->closebtn = XtVaCreateManagedWidget("closeButton",xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    this->showbtn = XtVaCreateManagedWidget("showButton",xmPushButtonWidgetClass, mainForm,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    Widget separator1 = XtVaCreateManagedWidget("separator",
        xmSeparatorGadgetClass, mainForm,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->closebtn,
        XmNbottomOffset,      7,
        NULL);

    this->createbtn = XtVaCreateManagedWidget("createButton",xmPushButtonWidgetClass, mainForm,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      10,
        NULL);

    this->addbtn = XtVaCreateManagedWidget("addButton",xmPushButtonWidgetClass, mainForm,
        XmNleftAttachment,    XmATTACH_WIDGET,
        XmNleftOffset,        10,
        XmNleftWidget,        this->createbtn,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      10,
        NULL);

    this->deletebtn = XtVaCreateManagedWidget("deleteButton",xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      10,
        NULL);

    this->removebtn = XtVaCreateManagedWidget("removeButton",xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,    XmATTACH_WIDGET,
        XmNrightOffset,        10,
        XmNrightWidget,       this->deletebtn,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      10,
        NULL);

    Widget separator = XtVaCreateManagedWidget("separator",xmSeparatorGadgetClass, mainForm,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->createbtn,
        XmNbottomOffset,      10,
        NULL);

    Widget namelabel = XtVaCreateManagedWidget("nameLabel",xmLabelWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator,
        XmNbottomOffset,      14,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        20,
        XmNalignment,         XmALIGNMENT_BEGINNING,
        NULL);

    this->text = XtVaCreateManagedWidget("text",xmTextWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator,
        XmNbottomOffset,      10,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_WIDGET,
        XmNleftOffset,        10,
        XmNrightOffset,       20,
        XmNleftWidget,        namelabel,
        XmNeditMode,          XmSINGLE_LINE_EDIT,
        XmNeditable,          True,
        NULL);

    Widget frame = XtVaCreateManagedWidget("frame",xmFrameWidgetClass, mainForm,
        XmNshadowThickness,   2,
        XmNtopAttachment,     XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNshadowType,        XmSHADOW_ETCHED_IN,
        XmNbottomWidget,      this->text,
        XmNtopOffset,         10,
        XmNbottomOffset,      10,
        XmNleftOffset,        20,
        XmNrightOffset,       20,
	XmNmarginWidth,	      3,
	XmNmarginHeight,      3,
        NULL);

    Widget sw = XtVaCreateManagedWidget("scrolledWindow",
                                xmScrolledWindowWidgetClass,
                                frame,
                                XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                                XmNshadowThickness,0,
                                XmNscrollBarDisplayPolicy, XmSTATIC,
                                NULL);

    this->list = XtVaCreateManagedWidget("list",
                                  xmListWidgetClass,
                                  sw,
                                  XmNlistMarginHeight,3,
                                  XmNlistMarginWidth, 3,
                                  XmNlistSpacing,     3,
                                  XmNshadowThickness, 1,
                                  XmNselectionPolicy, XmSINGLE_SELECT,
                                  XmNlistSizePolicy,  XmCONSTANT,
                                  NULL);

    XtVaSetValues(sw, XmNworkWindow, this->list, NULL);

    this->addCallbacks();

    return mainForm;
}

extern "C" void ProcessGroupCreateDialog_AddCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    char* name = XmTextGetString(dialog->text);

    if(IsBlankString(name))
    {
	ErrorMessage("No group name is given.");
	if(name) XtFree(name);    //   AJ
	return;
    }

    if(dialog->editor->getNodeSelectionCount() == 0)
    {
	ErrorMessage("No modules selected.");
	if(name) XtFree(name);    //   AJ
	return;
    }

    char *ptr = name + STRLEN(name) - 1;
    while(ptr > name AND IsWhiteSpace(ptr))
	ptr--;
    *++ptr = '\0';

#if WORKSPACE_PAGES
    if(NOT theDXApplication->getProcessGroupManager()->addGroupMember(name, 
#else
    if(NOT theDXApplication->PGManager->addGroupMember(name, 
#endif
					dialog->editor->getNetwork()))
	ErrorMessage("Group: %s does not exist.", name);

    XtFree(name);    //   AJ
}

extern "C" void ProcessGroupCreateDialog_RemoveCB(Widget    widget,
                                  XtPointer clientData,
                                  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    char* name = XmTextGetString(dialog->text);

    if(IsBlankString(name))
    {
        ErrorMessage("No group name is given.");
        if(name) XtFree(name);    //   AJ
        return;
    }

    if(dialog->editor->getNodeSelectionCount() == 0)
    {
        ErrorMessage("No modules selected.");
        if(name) XtFree(name);    //   AJ
        return;
    }

    char *ptr = name + STRLEN(name) - 1;
    while(ptr > name AND IsWhiteSpace(ptr))
	ptr--;
    *++ptr = '\0';

#if WORKSPACE_PAGES
    if(NOT theDXApplication->getProcessGroupManager()->removeGroupMember(name,
#else
    if(NOT theDXApplication->PGManager->removeGroupMember(name,
#endif
                                        dialog->editor->getNetwork()))
        ErrorMessage("Group: %s does not exist.", name);

    XtFree(name);    //   AJ
}


extern "C" void ProcessGroupCreateDialog_CreateCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    ProcessGroupAssignDialog *pgad = theDXApplication->processGroupAssignDialog;
    char* name = XmTextGetString(dialog->text);
    char* ptr;

    if(IsBlankString(name))
    {
	ErrorMessage("No group name is given.");
	if(name) XtFree(name);    //   AJ
	return;
    }

    if(dialog->editor->getNodeSelectionCount() == 0)
    {
	ErrorMessage("No modules selected.");
	if(name) XtFree(name);    //   AJ
	return;
    }

    ptr = name + STRLEN(name) - 1;
    while(ptr > name AND IsWhiteSpace(ptr))
	ptr--;
    *++ptr = '\0';

#if WORKSPACE_PAGES
    if(NOT theDXApplication->getProcessGroupManager()->createGroup(name, 
#else
    if(NOT theDXApplication->PGManager->createGroup(name, 
#endif
					dialog->editor->getNetwork()))
	ErrorMessage("Group: %s already exists.", name);

    else
    {
	dialog->makeList(-1);
	if(pgad AND pgad->isManaged())
	    pgad->refreshDisplay();
    }

    XtFree(name);    //   AJ
}

#ifdef PG_MODIFY_BUTTON
void ProcessGroupCreateDialog::ModifyCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    ASSERT(dialog->lastIndex);

    char* newname = XmTextGetString(dialog->text);
    char *name = (char*)pgm->getGroupName(dialog->lastIndex);

    if(IsBlankString(newname))
    {
	ErrorMessage("New name is not given.");
	if(newname) XtFree(newname);  //  AJ
	return;
    }

    if(NOT EqualString(newname, name) AND pgm->hasGroup(newname))
    {
	ErrorMessage("Group: %s already exists.", newname);
	XtFree(newname);  //  AJ
	return;
    }

    if(dialog->editor->getNodeSelectionCount() == 0)
    {
	ErrorMessage("No modules selected.");
	XtFree(newname);  //  AJ
	return;
    }

    char *hostname = DuplicateString(pgm->getGroupHost(name));
    pgm->removeGroup(name, dialog->editor->getNetwork());
    pgm->createGroup(newname, dialog->editor->getNetwork());
    pgm->assignHost(newname, hostname);

    if(hostname)
	pgm->addGroupAssignment(hostname, newname);

    dialog->makeList(-1);

    if(newname) XtFree(newname);  //  AJ
    if(hostname) delete hostname;
}
#endif

extern "C" void ProcessGroupCreateDialog_DeleteCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    char* name = XmTextGetString(dialog->text);
    ProcessGroupAssignDialog *pgad = theDXApplication->processGroupAssignDialog;

    if(IsBlankString(name))
    {
	ErrorMessage("No group name is given.");
	if(name) XtFree(name);   //  AJ
	return;
    }

    char *ptr = name + STRLEN(name) - 1;
    while(ptr > name AND IsWhiteSpace(ptr))
	ptr--;
    *++ptr = '\0';

#if WORKSPACE_PAGES
    if(NOT theDXApplication->getProcessGroupManager()->removeGroup(name, 
#else
    if(NOT theDXApplication->PGManager->removeGroup(name, 
#endif
					dialog->editor->getNetwork()))
	ErrorMessage("Group: %s does not exist.", name);

    else
    {
	dialog->makeList(dialog->lastIndex);
	if(pgad AND pgad->isManaged())
	    pgad->refreshDisplay();
    }

    XtFree(name);   //  AJ
}

extern "C" void ProcessGroupCreateDialog_ShowCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    char* name = XmTextGetString(dialog->text);

    if(IsBlankString(name))
    {
	ErrorMessage("No group name is given.");
	if(name) XtFree(name);   //  AJ
	return;
    }

    char *ptr = name + STRLEN(name) - 1;
    while(ptr > name AND IsWhiteSpace(ptr))
	ptr--;
    *++ptr = '\0';

#if WORKSPACE_PAGES
    if(NOT theDXApplication->getProcessGroupManager()->selectGroupNodes(name))
#else
    if(NOT theDXApplication->PGManager->selectGroupNodes(name))
#endif
	ErrorMessage("Group: %s does not exist.", name);
	
    XtFree(name);   //  AJ
}

extern "C" void ProcessGroupCreateDialog_CloseCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;

    dialog->unmanage();
}

extern "C" void ProcessGroupCreateDialog_TextVerifyCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)callData;
    int result = VerifyNameText(widget, cbs);

    switch (result)
    {
	case XmU_TEXT_DELETE:

    	    XmListDeselectAllItems(dialog->list);

	case XmU_TEXT_INVALID:

    	    return;
	    break;

 	default:    // valid string(see XmUtility.C for valid char set)

    	    dialog->setButtonsSensitive(True);
    	    if(cbs->event AND cbs->event->type == KeyPress)
    		XmListDeselectAllItems(dialog->list);
	
    }

}

extern "C" void ProcessGroupCreateDialog_TextChangeCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    char *name = XmTextGetString(dialog->text);

    if (IsBlankString(name))
    {
    	dialog->setButtonsSensitive(False);
	if(name) XtFree(name);   //  AJ
	return;
    }

    char *ptr = name;
    SkipWhiteSpace(ptr);

    if (ptr != name)
    	XmTextSetString(dialog->text, ptr);
    
    XtFree(name);   //  AJ
}

extern "C" void ProcessGroupCreateDialog_SelectCB(Widget    widget, 
				  XtPointer clientData,
				  XtPointer callData)
{
    ProcessGroupCreateDialog *dialog = (ProcessGroupCreateDialog*)clientData;
    XmListCallbackStruct* cs = (XmListCallbackStruct*)callData;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    int callIndex = cs->item_position;

    if(callIndex == dialog->lastIndex)
    {
        dialog->lastIndex = 0;
        XmTextSetString(dialog->text, NULL);
	dialog->setButtonsSensitive(False);
    }
    else
    {
        char *name = (char*)pgm->getGroupName(callIndex);
        XmTextSetString(dialog->text, name);
        XtVaSetValues(dialog->text, XmNcursorPosition, STRLEN(name),NULL);
        if(name)
	    dialog->setButtonsSensitive(True);

        dialog->lastIndex = callIndex;
    }

}

void ProcessGroupCreateDialog::makeList(int item)
{
    XmString *xmstrings;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif
    int      items = pgm->getGroupCount(); 
    int      i = -1;

    if (items != 0)
    {
        xmstrings = new XmString[items];
        for(i=0; i < items; i++)
        {
            xmstrings[i] =
                XmStringCreateSimple((char*)pgm->getGroupName(i+1));
        }
	XtSetSensitive(this->list, True);
    }
    else
    {
        items = 1;
        xmstrings = new XmString[1];
        xmstrings[0] = XmStringCreateSimple("(none)");
	XtSetSensitive(this->list, False);
    }

    XtVaSetValues(this->list,
        XmNitemCount,        items,
        XmNvisibleItemCount, items,
        XmNitems,            xmstrings,
        NULL);

    this->lastIndex = 0;

    if(i < 0 OR item == 0)
    {
        XmTextSetString(this->text, NULL);
	this->setButtonsSensitive(False);
        XmListDeselectAllItems(this->list);
    }
    else
    {
	this->setButtonsSensitive(True);

        if(item > 0)
             XmListSelectPos(this->list, item > items ? 0 : item, True);
        else
        {
             char *name = XmTextGetString(this->text);
             for(i=1; i<=items; i++)
                if(EqualString(name, (char*)pgm->getGroupName(i)))
                   break;
             XmListSelectPos(this->list, i, True);
             if(name) XtFree(name);   //  AJ
        }
    }

    for (i=0 ; i<items ; i++)
        XmStringFree(xmstrings[i]);

    delete xmstrings;

}

void ProcessGroupCreateDialog::manage()
{
    this->makeList(0);
    this->Dialog::manage();
}

void ProcessGroupCreateDialog::addCallbacks()
{
    XtAddCallback(this->createbtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupCreateDialog_CreateCB,
		  (XtPointer)this);

    XtAddCallback(this->deletebtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupCreateDialog_DeleteCB,
		  (XtPointer)this);

    XtAddCallback(this->removebtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupCreateDialog_RemoveCB,
		  (XtPointer)this);

    XtAddCallback(this->addbtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupCreateDialog_AddCB,
		  (XtPointer)this);

    XtAddCallback(this->showbtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupCreateDialog_ShowCB,
		  (XtPointer)this);

    XtAddCallback(this->closebtn,
		  XmNactivateCallback,
		  (XtCallbackProc)ProcessGroupCreateDialog_CloseCB,
		  (XtPointer)this);

    XtAddCallback(this->list,
                  XmNsingleSelectionCallback,
                  (XtCallbackProc)ProcessGroupCreateDialog_SelectCB,
                  (XtPointer)this);

    XtAddCallback(this->text,
                  XmNmodifyVerifyCallback,
                  (XtCallbackProc)ProcessGroupCreateDialog_TextVerifyCB,
                  (XtPointer)this);

    XtAddCallback(this->text,
                  XmNvalueChangedCallback,
                  (XtCallbackProc)ProcessGroupCreateDialog_TextChangeCB,
                  (XtPointer)this);
}

void ProcessGroupCreateDialog::setButtonsSensitive(Boolean setting)
{
    XtSetSensitive(this->createbtn, setting); 
    XtSetSensitive(this->removebtn, setting); 
    XtSetSensitive(this->deletebtn, setting);
    XtSetSensitive(this->addbtn,    setting);
    XtSetSensitive(this->showbtn,   setting); 
}

