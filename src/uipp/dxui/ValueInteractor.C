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

#include "Application.h"
#include "ValueNode.h"
#include "ValueInteractor.h"
#include "InteractorInstance.h"
#include "SetAttrDialog.h"
#include "ScalarNode.h"		// For XmNcolumns below
#include "ScalarListNode.h"	// For XmNcolumns below
#include "ErrorDialogManager.h"

boolean ValueInteractor::ClassInitialized = FALSE;

String ValueInteractor::DefaultResources[] =
{
    ".allowHorizontalResizing:          True",
    "*pinLeftRight:			True",
    "*wwLeftOffset:			0",
    "*wwRightOffset:			0",
#if (XmVersion < 1001)
    "*textEditor.columns:     		50",
#else
    "*textEditor.columns:     		12",
#endif
     NUL(char*)
};

ValueInteractor::ValueInteractor(const char * name, InteractorInstance *ii) : 
					Interactor(name,ii) 
{ 
    this->textEditor = NULL;
}

ValueInteractor::~ValueInteractor()
{
}
 
void ValueInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT ValueInteractor::ClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ValueInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        ValueInteractor::ClassInitialized = TRUE;
    }
}

//
// Allocate the interactor class and widget tree. 
//
Interactor *ValueInteractor::AllocateInteractor(const char *name,
						InteractorInstance *ii)
{

    ValueInteractor *i = new ValueInteractor(name,ii);
    return (Interactor*)i;
}

//
// Build the text editing widget (without callbacks)
//
Widget ValueInteractor::createTextEditor(Widget parent)
{
    Widget widget;
    int    n;
    Arg    wargs[10];

    n = 0;
    Node *node = this->interactorInstance->getNode();
    if (node->isA(ClassScalarNode) && !node->isA(ClassScalarListNode)) {
	ScalarNode *snode = (ScalarNode*)node;
	if (!snode->isVectorType()) {
#if (XmVersion < 1001)
	    XtSetArg(wargs[n], XmNcolumns,    20); n++;
#else
	    XtSetArg(wargs[n], XmNcolumns,    5); n++;
#endif
	}
    }
    XtSetArg(wargs[n], XmNresizeWidth, False);              n++;
    XtSetArg(wargs[n], XmNeditMode,    XmSINGLE_LINE_EDIT); n++;
    XtSetArg(wargs[n], XmNleftAttachment,   XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightAttachment,  XmATTACH_FORM); n++;

    widget = XmCreateText(parent, "textEditor", wargs, n);


    XtManageChild(widget);
    this->textEditor = widget;

    return widget;
}
    
Widget ValueInteractor::createInteractivePart(Widget parent)
{
    Widget widget = this->createTextEditor(parent);

    XtAddCallback(widget, XmNactivateCallback, 
			(XtCallbackProc)ValueInteractor_ValueChangeCB, (XtPointer)this);

    return widget;
}
extern "C" void ValueInteractor_ValueChangeCB(Widget w, XtPointer clientData,
                                XtPointer callData)
{
    ValueInteractor *vi = (ValueInteractor*)clientData;
    vi->valueChangeCallback(w, callData);
}
//
// Handle a change in the text 
//
void ValueInteractor::valueChangeCallback(Widget w, XtPointer cb)
{
    InteractorInstance *ii = (InteractorInstance*)this->interactorInstance;
    Node *vnode = ii->getNode();
    char *reason;

    char *string = this->getDisplayedText(); 
    if (!ii->setAndVerifyOutput(1,string, DXType::UndefinedType,TRUE,&reason)) {
	if (!reason) {
	    ErrorMessage("'%s' is not a valid value for a %s interactor.",
			string, vnode->getNameString());
	} else {
	    ErrorMessage(reason);
	    delete reason;
	}
	this->updateDisplayedInteractorValue();
    }
    delete string;
}
//
// Update the text value with the give string. 
//
void ValueInteractor::installNewText(const char *text)
{
    XtVaSetValues(this->textEditor, XmNvalue, text, NULL);
}
//
// Get the text that is currently displayed in the text window. 
// The returned string must be freed by the caller.
//
char *ValueInteractor::getDisplayedText()
{
    char *p = NULL;
    char *string = XmTextGetString(this->textEditor);  
    if (string) {
	p = DuplicateString(string);
	XtFree(string);
    }
    return p;
}

//
// Update the text value from the instance's notion of the current text.
//
void ValueInteractor::updateDisplayedInteractorValue()
{
    InteractorInstance *ii = (InteractorInstance*)this->interactorInstance;
    Node *vnode = ii->getNode();
    const char *v = vnode->getOutputValueString(1);

    this->installNewText(v);
}
#if 0
//
// Get the current text value. For String and Value nodes, this is saved
// globally with the node, but for String/ValueListNodes, this is stored
// with the instance and is probably the currenlty selected list item.
//
const char *ValueInteractor::getCurrentTextValue()
{
    Node *n = this->getNode();
    return n->getOutputValueString(1);
}
#endif
//
// ValueInteractors do not have attributes, but we must always update
// the displayed value.
//
void ValueInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    this->updateDisplayedInteractorValue();
}

//
// Nothing to complete, but must be defined.
//
void ValueInteractor::completeInteractivePart()
{
    return; 
}

