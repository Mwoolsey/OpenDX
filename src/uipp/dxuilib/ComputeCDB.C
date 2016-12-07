/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ComputeCDB.h"


#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>

#include "CDBInput.h"

#include "ComputeNode.h"
#include "Ark.h"
#include "ListIterator.h"
#include "Application.h"
#include "ErrorDialogManager.h"

boolean ComputeCDB::ClassInitialized = FALSE;
String ComputeCDB::DefaultResources[] =
{
    "*computeInputTitle.labelString:		Inputs:",
    "*computeInputTitle.foreground:		SteelBlue",
    "*computeInputNameLabel.labelString:	Name",
    "*computeInputNameLabel.foreground:		SteelBlue",
    "*computeInputSourceLabel.labelString:	Source",
    "*computeInputSourceLabel.foreground:	SteelBlue",
    "*computeExpressionLabel.labelString:	Expression:",
    "*computeExpressionLabel.foreground:	SteelBlue",


    NULL
};

ConfigurationDialog*
ComputeCDB::AllocateConfigurationDialog(Widget parent,
					Node *node)
{
    return new ComputeCDB(parent, node);
}

boolean ComputeCDB::applyCallback(Dialog *)
{
    ListIterator li(this->inputList);
    int i;
    CDBInput *input;
    ComputeNode     *n = (ComputeNode*)this->node;
    boolean anySet = FALSE;

    char *expressionString = XmTextGetString(this->expression);

    for (i = 1; NULL != (input = (CDBInput*)li.getNext()); ++i)
    {
	if (input->nameWidget != NULL)
	{
	    char *valueString = XmTextGetString(input->nameWidget);
	    if (!EqualString(valueString, n->getName(i)))
	    {
		n->setName(valueString, i, FALSE);
		anySet = TRUE;
	    }
	    input->setInitialValue(valueString);
	    XtFree(valueString);
	}
    }

    if (!EqualString(n->getExpression(), expressionString)) {
	n->setExpression(expressionString);
	this->expression_modified = FALSE;
    } else if (anySet)
	n->sendValues(FALSE);
    if (this->initialExpression)
	delete this->initialExpression;
    this->initialExpression = DuplicateString(expressionString);
    XtFree(expressionString);

    char *s = XmTextGetString(this->notation);
    n->setLabelString(s);
    if (this->initialNotation)
	delete this->initialNotation;
    this->initialNotation = DuplicateString(s);
    XtFree(s);

    return TRUE;
}
void ComputeCDB::restoreCallback(Dialog *)
{
    ListIterator li(this->inputList);
    int i;
    CDBInput *input;
    ComputeNode     *n = (ComputeNode*)this->node;
    boolean anySet = FALSE;

    char *expressionString = XmTextGetString(this->expression);

    for (i = 1; NULL != (input = (CDBInput*)li.getNext()); ++i)
    {
	if (input->nameWidget != NULL)
	{
	    char *valueString = XmTextGetString(input->nameWidget);
	    if (!EqualString(valueString, input->initialValue))
	    {
		XmTextSetString(input->nameWidget, input->initialValue);
		n->setName(input->initialValue, i, FALSE);
		anySet = TRUE;
	    }
	    XtFree(valueString);
	}
    }
    char *valueString = this->initialExpression;
    if (!EqualString(valueString, expressionString))
    {
	this->setExpression(valueString);
	n->setExpression(valueString);
	this->expression_modified = FALSE;
    }
    else if (anySet)
	n->sendValues(FALSE);
    XtFree(expressionString);
}
extern "C" void 
ComputeCDB_ExpressionModifiedCB(Widget widget, XtPointer clientData, XtPointer)
{
    ComputeCDB *dialog = (ComputeCDB *)clientData;
    ASSERT(dialog);
    dialog->expression_modified = TRUE;
}

extern "C" void ComputeCDB_ExpressionChangedCB(Widget widget,
				   XtPointer clientData,
				   XtPointer)
{
    int i;
    ComputeCDB *dialog = (ComputeCDB *)clientData;
    ComputeNode *n = (ComputeNode*)dialog->node;

    char *expressionString = XmTextGetString(dialog->expression);

    for (i = 1; i < n->getInputCount(); ++i)
    {
	CDBInput *input = (CDBInput *)dialog->inputList.getElement(i);
	char *s = XmTextGetString(input->nameWidget);
	n->setName(s, i, FALSE);
	XtFree(s);
    }

    n->setExpression(expressionString, TRUE);
    XtFree(expressionString);
}
extern "C" void ComputeCDB_NameChangedCB(Widget widget,
			     XtPointer clientData,
			     XtPointer callData)
{
    ComputeCDB_ExpressionChangedCB(widget, clientData, callData);
}

Widget ComputeCDB::createInputs(Widget parent, Widget top)
{
    ComputeNode *n = (ComputeNode*)this->node;
    int numParam = n->getInputCount();
    if (numParam == 0)
	return NULL;

    Widget inputs = XtVaCreateManagedWidget(
	"computeInputSection",
	xmFormWidgetClass,
	parent,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       top,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , 5,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , 5,
	NULL);

    this->inputTitle = XtVaCreateManagedWidget(
	"computeInputTitle",
	xmLabelWidgetClass,
	inputs,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->expressionLabel = XtVaCreateManagedWidget(
	"computeExpressionLabel",
	xmLabelWidgetClass,
	inputs,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->inputTitle,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
	NULL);

    this->expression = XtVaCreateManagedWidget(
	"computeExpressionText",
	xmTextWidgetClass,
	inputs,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , expressionLabel,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
	NULL);
    this->initialExpression = DuplicateString(n->getExpression());
    //XmTextSetString(this->expression, this->initialExpression);
    this->setExpression(this->initialExpression);

    XtAddCallback(this->expression, XmNactivateCallback,
	(XtCallbackProc)ComputeCDB_ExpressionChangedCB, (XtPointer)this);
    XtAddCallback(this->expression, XmNmodifyVerifyCallback,
	(XtCallbackProc)ComputeCDB_ExpressionModifiedCB, (XtPointer)this);

    if (n->getInputCount() != 1)
    {
	int i;
	for (i = 2; i <= numParam; ++i)
	    this->newInput(i);
    }


    return inputs;
}

void ComputeCDB::setExpression(const char* s)
{
    XtRemoveCallback (this->expression, XmNmodifyVerifyCallback, (XtCallbackProc)
	ComputeCDB_ExpressionModifiedCB, (XtPointer)this);

    XmTextSetString (this->expression, (char*)s);

    XtAddCallback(this->expression, XmNmodifyVerifyCallback, (XtCallbackProc)
	ComputeCDB_ExpressionModifiedCB, (XtPointer)this);
}


ComputeCDB::ComputeCDB( Widget parent,
		       Node *node):
    ConfigurationDialog("computeConfigurationDialog", parent, node)
{
    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT ComputeCDB::ClassInitialized)
    {
	ASSERT(theApplication);
        ComputeCDB::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
    this->initialExpression = NULL;
    this->expression_modified = FALSE;
}

#if 00 // Not needed because there are any classes derived from this class 
ComputeCDB::ComputeCDB(char *name,
		       Widget parent,
		       Node *node) :
    ConfigurationDialog(name, parent, node, TRUE)
{
    this->initialExpression = NULL;
}
#endif // 00

ComputeCDB::~ComputeCDB()
{
    int i;
    for (i = 1; i <= this->inputList.getSize(); ++i)
    {
	CDBInput *input = (CDBInput*)this->inputList.getElement(i);
	delete input;
    }
    this->inputList.clear();
    if (this->initialExpression)
	delete this->initialExpression;
}

//
// Install the default resources for this class.
//
void ComputeCDB::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, ComputeCDB::DefaultResources);
    this->ConfigurationDialog::installDefaultResources(baseWidget);
}

void ComputeCDB::manage()
{
    this->ConfigurationDialog::manage();

    // Set the keyboard focus to the expression widget 
    XmProcessTraversal(this->expression,XmTRAVERSE_CURRENT);
}

Widget ComputeCDB::createDialog(Widget parent)
{
    Widget w = this->ConfigurationDialog::createDialog(parent);
    XtSetSensitive(this->expand, False);
    XtSetSensitive(this->collapse, False);

    return w;
}

void ComputeCDB::changeInput(int i)
{
    ComputeNode *n = (ComputeNode*)this->node;
    if (i == 1)
    {
	if (!this->expression_modified) {
	    const char *s = n->getExpression();
	    this->setExpression(s);
	}
    }
    else
    {
	CDBInput *input = (CDBInput*) this->inputList.getElement(i - 1);
	if (input->nameWidget != NULL)
	{
	    XmTextSetString(input->nameWidget, (char *)n->getName(i - 1));
	    
	    const char *sourceString;
	    if (n->isInputConnected(i))
	    {
		List *arcs = (List *)n->getInputArks(i);
		Ark *a = (Ark*)arcs->getElement(1);
		int paramNum;
		ComputeNode *source = (ComputeNode*)a->getSourceNode(paramNum);
		sourceString = source->getNameString();
		XmString s = XmStringCreate((char *)sourceString,
					    XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(input->connectedToWidget,
		    XmNlabelString,	s,
		    NULL);
		XmStringFree(s);
	    }
	    else {
		XmString s = XmStringCreate("", XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(input->connectedToWidget,
		    XmNlabelString,	s,
		    NULL);
		XmStringFree(s);
	    }
	}
    }
}
void ComputeCDB::newInput(int i)
{
    ComputeNode *n = (ComputeNode*)this->node;
    CDBInput *input = (CDBInput*)this->inputList.getElement(i - 1);
    if (input == NULL)
    {
	input = new CDBInput;
	this->inputList.insertElement(input, i - 1);
    }

    if (!n->isInputViewable(i))
	return;

    const char *pname = n->getName(i - 1);

    Widget inputs = XtParent(this->expressionLabel);
    Widget prevNameWidget = NULL;
    int j;
    for (j = i - 2; j > 0; --j)
    {
	CDBInput *prevInput = (CDBInput*)this->inputList.getElement(j);
	if (prevInput != NULL && prevInput->nameWidget != NULL)
	{
	    prevNameWidget = prevInput->nameWidget;
	    break;
	}
    }
    if (prevNameWidget == NULL)
    {
	if (this->inputNameLabel == NULL)
	{
	    this->inputSourceLabel = XtVaCreateManagedWidget(
		"computeInputSourceLabel",
		xmLabelWidgetClass,
		inputs,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNrightAttachment , XmATTACH_FORM,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		XmNwidth           , 250,
		NULL);

	    this->inputNameLabel = XtVaCreateManagedWidget(
		"computeInputNameLabel",
		xmLabelWidgetClass,
		inputs,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNleftAttachment  , XmATTACH_FORM,
		XmNleftOffset      , 5,
		XmNrightAttachment , XmATTACH_WIDGET,
		XmNrightWidget     , this->inputSourceLabel,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		NULL);
	}

	prevNameWidget = this->inputNameLabel;
    }


    Widget name = XtVaCreateManagedWidget(
	pname, 
	xmTextWidgetClass,
	inputs,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       prevNameWidget,
	XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,      this->inputNameLabel,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,     this->inputNameLabel,
	XmNvalue,           pname,
	XmNeditMode,        XmSINGLE_LINE_EDIT,
	NULL);
    input->nameWidget = name;
    input->setInitialValue(n->getName(i-1));

    Widget source = XtVaCreateManagedWidget(
	"source",
	xmLabelWidgetClass,
	inputs,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       prevNameWidget,
	XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,      this->inputSourceLabel,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,     this->inputSourceLabel,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    input->connectedToWidget = source;

    this->changeInput(i);

    XtAddCallback(name, XmNactivateCallback,
	(XtCallbackProc)ComputeCDB_NameChangedCB, (XtPointer)this);

    CDBInput *nextInput = input;
    for (j = i + 1; j <= this->inputList.getSize(); ++j)
    {
	nextInput = (CDBInput*)this->inputList.getElement(j);
	if (nextInput != NULL && nextInput->nameWidget != NULL)
	{
	    XtVaSetValues(nextInput->nameWidget,
		XmNtopWidget, input->nameWidget,
		NULL);
	    XtVaSetValues(nextInput->connectedToWidget,
		XmNtopWidget, input->nameWidget,
		NULL);
	    break;
	}
    }
    XtVaSetValues(this->expressionLabel,
	XmNtopWidget, nextInput->nameWidget,
	NULL);
}

void ComputeCDB::deleteInput(int i)
{
    // Set i to be the index in the inputList corresponding with the input.
    --i;
    CDBInput *input = (CDBInput*)this->inputList.getElement(i);
    if (input == NULL)
	return;

    Widget prevNameWidget = NULL;
    int j;
    for (j = i - 1; j > 0; --j)
    {
	CDBInput *prevInput = (CDBInput*)this->inputList.getElement(j);
	if (prevInput != NULL && prevInput->nameWidget != NULL)
	{
	    prevNameWidget = prevInput->nameWidget;
	    break;
	}
    }
    if (prevNameWidget == NULL)
    {
	prevNameWidget = this->inputNameLabel;
    }


    for (j = i + 1; j <= this->inputList.getSize(); ++j)
    {
	CDBInput *nextInput = (CDBInput*)this->inputList.getElement(j);
	if (nextInput != NULL && nextInput->nameWidget != NULL)
	{
	    XtVaSetValues(nextInput->nameWidget,
		XmNtopWidget, prevNameWidget,
		NULL);
	    XtVaSetValues(nextInput->connectedToWidget,
		XmNtopWidget, prevNameWidget,
		NULL);
	    break;
	}
    }
    if (input->nameWidget != NULL)
    {
	XtDestroyWidget(input->nameWidget);
	XtDestroyWidget(input->connectedToWidget);
    }
    this->inputList.deleteElement(i);
    delete input;

    CDBInput *lastInput = (CDBInput*)
	this->inputList.getElement(this->inputList.getSize());
    if (lastInput != NULL)
	prevNameWidget = lastInput->nameWidget;
    else
	prevNameWidget = this->inputNameLabel;

    XtVaSetValues(this->expressionLabel,
	XmNtopWidget, prevNameWidget,
	NULL);
}

boolean ComputeCDB::widgetChanged(int i, boolean send)
{
    if (i < 2)
	return TRUE;
    CDBInput *input = (CDBInput*)this->inputList.getElement(i - 1);
    const char *s = XmTextGetString(input->nameWidget);
    ComputeNode *n = (ComputeNode*)this->node;

    n->setName(s, i-1, send);

    XtFree((char *)s);

    return TRUE;
}

void   ComputeCDB::remanageInputs(boolean)
{
}
