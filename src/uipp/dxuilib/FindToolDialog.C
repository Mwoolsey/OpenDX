/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <X11/StringDefs.h>

#include <limits.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "DXApplication.h"
#include "FindToolDialog.h"
#include "Node.h"
#include "Network.h"
#include "ListIterator.h"
#include "EditorWindow.h"
#include "NodeDefinition.h"
#include "FindStack.h"
#include "WarningDialogManager.h"
#include "XmUtility.h"

boolean FindToolDialog::ClassInitialized = FALSE;

String  FindToolDialog::DefaultResources[] = {
    //
    //  NOTE: minWidth/Height doesn't work with the shell created by 
    //        Dialog:CreateMainForm().
    //
#if defined(sun4)
    ".minWidth:                         250",
#else
    ".minWidth:                         230",
#endif
    ".minHeight:                        210",
    "*list.visibleItemCount:            11",
    "*scrolledWindow.height:            220",
    "*dialogTitle:     			Find Tool...",
    "*selectionLabel.labelString:	Selection:",
    "*findButton.labelString:		Find",
    "*undoButton.labelString:		Undo",
    "*redoButton.labelString:		Redo",
    "*restoreButton.labelString:	Restore",
    "*closeButton.labelString:		Close",
    "*XmPushButton.defaultButtonShadowThickness: 0",
    "*XmPushButton.width:               60",
    NULL
};

FindToolDialog::FindToolDialog(EditorWindow* editor) 
                       		: Dialog("findTool", editor->getRootWidget())
{
    this->editor = editor;
    this->redoStack = new FindStack;
    this->undoStack = new FindStack;
    this->warningDialog = new WarningDialogManager("FindToolWarning");
    this->maxInstance = INT_MAX;
    this->lastIndex = 0;
    this->moduleList = NULL;
    this->lastName[0] = '\0';


    if (NOT FindToolDialog::ClassInitialized)
    {
        FindToolDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

FindToolDialog::~FindToolDialog()
{
    ListIterator li;
    char *module;

    delete this->redoStack;
    delete this->undoStack;
    delete this->warningDialog;
    if(this->moduleList)
    {
	li.setList(*this->moduleList);
	while( (module = (char*)li.getNext()) )
	    delete module;
	this->moduleList->clear();
	delete this->moduleList;
    }
}

//
// Install the default resources for this class.
//
void FindToolDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, FindToolDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

Widget FindToolDialog::createDialog(Widget parent)
{
    Arg arg[10];
    int n = 0;
    Widget frame,separator, separator1;
    Widget selectionlabel;

    XtSetArg(arg[n], XmNautoUnmanage, False);		n++;
    XtSetArg(arg[n], XmNnoResize, False);		n++;
    XtSetArg(arg[n], XmNresizePolicy, XmRESIZE_NONE);	n++;

    Widget mainForm = this->CreateMainForm(parent, this->name, arg, n);

    this->closebtn = XtVaCreateManagedWidget("closeButton",
	xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      7,
     	NULL);

    this->restorebtn = XtVaCreateManagedWidget("restoreButton",
	xmPushButtonWidgetClass, mainForm,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      7,
	XmNsensitive,	      FALSE,
        NULL);

    separator1 = XtVaCreateManagedWidget("separator",
	xmSeparatorGadgetClass, mainForm,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->closebtn,
        XmNbottomOffset,      7,
        NULL);

    this->findbtn = XtVaCreateManagedWidget("findButton",
	xmPushButtonWidgetClass, mainForm,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
	XmNbottomWidget,      separator1,
#if (XmVersion > 1001) && defined (sun4) 
        XmNbottomOffset,      2,
	XmNheight,	      32,
#else
#if (XmVersion > 1001) && defined (aviion)
        XmNbottomOffset,      2,
#else
        XmNbottomOffset,      7,
#endif
#endif
#ifdef FIND_DEFAULT_BUTTON 
	XmNwidth,	      70,
     	XmNdefaultButtonShadowThickness, 1,
#endif
        NULL);

    this->redobtn = XtVaCreateManagedWidget("redoButton",
	xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      7,
	XmNsensitive,	      FALSE,
        NULL);

    this->undobtn = XtVaCreateManagedWidget("undoButton",
	xmPushButtonWidgetClass, mainForm,
        XmNrightAttachment,   XmATTACH_WIDGET,
	XmNrightWidget,	      this->redobtn,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator1,
        XmNbottomOffset,      7,
	XmNsensitive,	      FALSE,
        NULL);



    separator = XtVaCreateManagedWidget("separator",
	xmSeparatorGadgetClass, mainForm,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->redobtn,
        XmNbottomOffset,      7,
        NULL);

    selectionlabel = XtVaCreateManagedWidget("selectionLabel",
	xmLabelWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator, 
        XmNbottomOffset,      14,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
	XmNalignment,	      XmALIGNMENT_BEGINNING,
        NULL);

    this->text = XtVaCreateManagedWidget("text",
	xmTextWidgetClass, mainForm,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator, 
        XmNbottomOffset,      10,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_WIDGET,
        XmNleftOffset,        10,
        XmNrightOffset,       5,
        XmNleftWidget,        selectionlabel, 
	XmNeditMode,	      XmSINGLE_LINE_EDIT,
	XmNeditable,	      True,
        NULL);

    frame = XtVaCreateManagedWidget("frame",
	xmFrameWidgetClass, mainForm,
        XmNshadowThickness,   2,
        XmNtopAttachment,     XmATTACH_FORM,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNbottomAttachment,  XmATTACH_WIDGET,
	XmNshadowType,        XmSHADOW_ETCHED_IN,
        XmNbottomWidget,      this->text, 
        XmNbottomOffset,      10,
        XmNtopOffset,         10,
        XmNleftOffset,        10,
        XmNrightOffset,       10,
        XmNmarginWidth,       3,
        XmNmarginHeight,      3,
        NULL);

#ifdef FIND_DEFAULT_BUTTON 
    XtVaSetValues(mainForm, XmNdefaultButton, this->findbtn, NULL);
#endif

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

#if (XmVersion > 1001) // have to set the values here.
    XtVaSetValues(this->closebtn, XmNdefaultButtonShadowThickness, 0, NULL);
    XtVaSetValues(this->restorebtn, XmNdefaultButtonShadowThickness, 0, NULL);
    XtVaSetValues(this->undobtn, XmNdefaultButtonShadowThickness, 0, NULL);
    XtVaSetValues(this->redobtn, XmNdefaultButtonShadowThickness, 0, NULL);
#endif

    XtAddCallback(XtParent(mainForm),
		  XmNpopdownCallback,
		  (XtCallbackProc)FindToolDialog_PopdownCB,
		  (XtPointer)this);

    XtAddCallback(this->closebtn,
		      XmNactivateCallback,
		      (XtCallbackProc)FindToolDialog_CloseCB,
		      (XtPointer)this);

    XtAddCallback(this->restorebtn,
		      XmNactivateCallback,
		      (XtCallbackProc)FindToolDialog_RestoreCB,
		      (XtPointer)this);

    XtAddCallback(this->findbtn,
		      XmNactivateCallback,
		      (XtCallbackProc)FindToolDialog_FindCB,
		      (XtPointer)this);

    XtAddCallback(this->redobtn,
		      XmNactivateCallback,
		      (XtCallbackProc)FindToolDialog_RedoCB,
		      (XtPointer)this);

    XtAddCallback(this->undobtn,
		      XmNactivateCallback,
		      (XtCallbackProc)FindToolDialog_UndoCB,
		      (XtPointer)this);

    XtAddCallback(this->text,
		      XmNfocusCallback,
		      (XtCallbackProc)FindToolDialog_TextChangeCB,
		      (XtPointer)this);

    XtAddCallback(this->text,
                  XmNmodifyVerifyCallback,
                  (XtCallbackProc)FindToolDialog_TextVerifyCB,
                  (XtPointer)this);

    XtAddCallback(this->text,
                  XmNactivateCallback,
                  (XtCallbackProc)FindToolDialog_FindCB,
                  (XtPointer)this);

    XtAddCallback(this->list,
		      XmNsingleSelectionCallback,
		      (XtCallbackProc)FindToolDialog_SelectCB,
		      (XtPointer)this);

    XtAddCallback(this->list,
		      XmNdefaultActionCallback,
		      (XtCallbackProc)FindToolDialog_DefaultCB,
		      (XtPointer)this);

    return mainForm;
}

void FindToolDialog::changeList()
{
    int items;
    ListIterator li;
    char *module;
    XmString *xmstrings;
    int i;

    //
    // Remove the old list.
    //
    if(this->moduleList)
    {
	li.setList(*this->moduleList);
	while( (module = (char*)li.getNext()) )
	    delete module;
	this->moduleList->clear();
	delete this->moduleList;
	this->moduleList = NULL;
    }

    //
    // Create the new list.
    //
    this->moduleList = this->editor->makeModuleList();
    if(this->moduleList)
	items = this->moduleList->getSize();
    else 
	items = 0;

    if (items != 0)
    {
        xmstrings = new XmString[items];
        for(i=0; i < items; i++)
        {
            xmstrings[i] =
                XmStringCreateSimple((char*)this->moduleList->getElement(i+1));
        }
    }
    else
    {
        items = 1;
        xmstrings = new XmString[1];
        xmstrings[0] = XmStringCreateSimple("(none)");
    }

    XtVaSetValues(this->list,
        XmNitemCount,        items,
        XmNitems,            xmstrings,
        NULL);

    XmListDeselectAllItems(this->list);

    this->lastIndex = 0;

    for (i=0 ; i<items ; i++)
        XmStringFree(xmstrings[i]);

    delete xmstrings;

#ifdef DUMMY_FOR_LIST_WIDGET
    int  height;
#if (XmVersion > 1001)
    XtVaGetValues(this->list,
                  XmNheight, &height,
                  NULL);

    XtVaSetValues(this->list,
                  XmNheight, height+1,
                  NULL);

    XtVaSetValues(this->list,
                  XmNheight, height,
                  NULL);
#else
    XtVaGetValues(XtParent(this->list),
                  XmNheight, &height,
                  NULL);

    XtVaSetValues(XtParent(this->list),
                  XmNheight, height+1,
                  NULL);

    XtVaSetValues(XtParent(this->list),
                  XmNheight, height,
                  NULL);
#endif
#endif
}

extern "C" void FindToolDialog_SelectCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    XmListCallbackStruct* cs = (XmListCallbackStruct*)callData;
    FindToolDialog *dialog = (FindToolDialog*) clientData;

    if(cs->reason == XmCR_DEFAULT_ACTION)        // double click
	return;

    int callIndex = cs->item_position;

    if(callIndex == dialog->lastIndex)
    {
        dialog->lastIndex = 0;
        XmTextSetString(dialog->text, NULL);
    }
    else
    {
        char *name = (char*)dialog->moduleList->getElement(callIndex);
        XmTextSetString(dialog->text, name);

        dialog->lastIndex = callIndex;
    }


}

extern "C" void FindToolDialog_DefaultCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    XmListCallbackStruct* cs = (XmListCallbackStruct*)callData;
    FindToolDialog *dialog = (FindToolDialog*) clientData;

    dialog->lastIndex = cs->item_position;

    char *name = (char*)dialog->moduleList->getElement(dialog->lastIndex);
    XmTextSetString(dialog->text, name);
    XmListSelectPos(dialog->list, dialog->lastIndex, False);

    XtCallCallbacks(dialog->findbtn, XmNactivateCallback, (XtPointer)callData);
}

void FindToolDialog::manage()
{
//    this->toolSelector->reset();
    this->changeList();
    this->restoreSens(False);
    this->redoSens(False);
    this->undoSens(False);
    this->lastInstance = 0;

    this->Dialog::manage();

    XmProcessTraversal(this->text, XmTRAVERSE_CURRENT);
    XmProcessTraversal(this->text, XmTRAVERSE_CURRENT);
}

extern "C" void FindToolDialog_RestoreCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{

    ASSERT(clientData);
    FindToolDialog *data = (FindToolDialog*) clientData;

    data->restoreSens(False);
    data->redoSens(False);
    data->undoSens(False);
    data->lastInstance = 0;
    (data->redoStack)->clear();
    (data->undoStack)->clear();

    data->editor->deselectAllNodes();
    data->editor->moveWorkspaceWindow(NULL);
    data->editor->resetOrigin();
}

extern "C" void FindToolDialog_RedoCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    char   name[500];
    char   label[500];
    int    instance;

    ASSERT(clientData);
    FindToolDialog *data = (FindToolDialog*) clientData;
    boolean stop = FALSE;
    int result;

    while (NOT stop)
    {
        (data->redoStack)->pop(name, &instance, label);
        if ((result = data->selectNode(name, instance, FALSE)) == 0)
        {
	    WarningMessage("Cannot find tool %s in the visual program.",
		name);
            stop = TRUE;
        }
        else
        {
            data->undoSens(True);
            (data->undoStack)->push(name,instance,label);
            if(result > 0)
                stop = TRUE;
        }

        if ((data->redoStack)->getSize() == 0)
        {
            stop = TRUE;
            data->redoSens(False);
        }
    }

    XmTextSetString(data->text, label);
}

extern "C" void FindToolDialog_UndoCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    char   name[50];
    char   label[50];
    int    instance;

    ASSERT(clientData);
    FindToolDialog *data = (FindToolDialog*) clientData;
    boolean stop = FALSE;
    int result;

    while (NOT stop)
    {
        (data->undoStack)->pop(name, &instance, label);
        if ((result = data->selectNode(name, instance, FALSE)) == 0)
        {
	    WarningMessage("Cannot find tool %s in the visual program.",
		name);
            stop = TRUE;
        }
        else
        {
            data->redoSens(True);
            (data->redoStack)->push(name,instance,label);
            if(result > 0)
                stop = TRUE;
        }

        if ((data->undoStack)->getSize() == 0)
        {
            stop = TRUE;
            data->undoSens(False);
        }
    }

    XmTextSetString(data->text, label);
}

extern "C" void FindToolDialog_TextChangeCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    FindToolDialog *dialog = (FindToolDialog*) clientData;

    XmListDeselectAllItems(dialog->list);
    dialog->lastIndex = 0;
}

extern "C" void FindToolDialog_TextVerifyCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)callData;

    VerifyNameText(widget, cbs);
}

extern "C" void FindToolDialog_CloseCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{

    ASSERT(clientData);
    FindToolDialog *data = (FindToolDialog*) clientData;
    data->unmanage();
}

extern "C" void FindToolDialog_PopdownCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    ASSERT(clientData);

    FindToolDialog *data = (FindToolDialog*) clientData;

    (theDXApplication->network->getEditor())->resetOrigin();
    data->redoStack->clear();
    data->undoStack->clear();
}

extern "C" void FindToolDialog_FindCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    char*  	name;
    int         instance;
    int         nextInstance = INT_MAX;
    int		startPos = 1;
    Node	*node, *theNode = NULL;

    ASSERT(clientData);
    ASSERT(callData);

    FindToolDialog *data = (FindToolDialog*) clientData;

    name = XmTextGetString(data->text);
    if (IsBlankString(name))
    {
	WarningMessage("No tool is chosen.");
	if(name) XtFree(name);   //	AJ
	return;
    }

    if (strcmp(name, data->lastName) != 0)
    {
	strcpy(data->lastName, name);
	data->lastInstance = 0;
	data->lastPos = 1;
    }

    while ( (node=(data->editor->getNetwork())->findNode(name,&startPos)) )
    {
	instance = node->getInstanceNumber();
	if (instance > data->lastInstance AND instance < nextInstance)
	{
	    nextInstance = instance;
	    theNode = node;
        }
    }

    if (nextInstance == INT_MAX)
	while((theNode = data->editor->getNetwork()->
			findNode(name,&data->lastPos,TRUE))
	      AND EqualString(theNode->getNameString(),
			      theNode->getLabelString()));

    if(NOT theNode)
    {
	if (data->lastInstance == 0)
	    WarningMessage("Cannot find tool %s in the visual program.",
		name);
	else
	{	
	    data->lastInstance = 0;
	    WarningMessage("Last instance of tool %s found.", name); 
	};

	data->lastPos = 1;
    }
    else
    {
	//
	// In order to implement restore, we need to record the name
	// of the vpe page that was selected prior to our doing
	// anything.  This used to happen as a side effect of respositioning
	// the vpe window however that was broken when vpe pages were added.
	//
	if (data->undoStack->getSize() == 0)
	    data->editor->checkPointForFindDialog(data);

    	data->selectNode((char*)theNode->getNameString(), 
			 theNode->getInstanceNumber());
    	data->lastInstance = nextInstance;
	data->undoStack->push((char*)theNode->getNameString(), 
				theNode->getInstanceNumber(), 
				name);
        data->redoStack->clear();
    	data->restoreSens(True);
    	data->undoSens(True);
    	data->redoSens(False);
    }

    if (name) 
	XtFree(name);
}

int FindToolDialog::selectNode(char* name, int instance, boolean newNode)
{
    int result = 1;
    List *nlist;
    Node *node;

    if(NOT newNode AND this->editor->getNodeSelectionCount() == 1)
    {
	nlist = this->editor->makeSelectedNodeList();
	ASSERT(nlist);
    	node = (Node*)nlist->getElement(1);
        if(EqualString(node->getNameString(), name)
       	   AND node->getInstanceNumber() == instance)
	      result = -1;	
    	
	nlist->clear();
	delete nlist;
    }

    this->editor->deselectAllNodes();

    if(result < 0) return result;

    if(this->editor->selectNode(name,instance,True))
	result = 1;
    else
	result = 0;

    return result;
}


void FindToolDialog::restoreSens(boolean sensitive)
{
    if (XtIsSensitive(this->restorebtn) != sensitive) {
	XtSetSensitive(this->restorebtn,sensitive);
    }
}

void FindToolDialog::redoSens(boolean sensitive)
{
     if (XtIsSensitive(this->redobtn) != sensitive)
     	XtSetSensitive(this->redobtn,sensitive);
}

void FindToolDialog::undoSens(boolean sensitive)
{
     if (XtIsSensitive(this->undobtn) != sensitive)
     	XtSetSensitive(this->undobtn,sensitive);
}

