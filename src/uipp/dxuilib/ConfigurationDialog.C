/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <sys/stat.h>

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/ScrolledW.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>

#include "ConfigurationDialog.h"
#include "ListIterator.h"
#include "CDBInput.h"
#include "CDBOutput.h"
#include "DescrDialog.h"

#include "Application.h"
#include "DXApplication.h"
#include "ErrorDialogManager.h"

#include "Parameter.h"
#include "Ark.h"
#include "Node.h"
#include "XmUtility.h"

//
// W.R.T. input parameters:
// I made significant changes in Oct/95.  Here's my understanding of how 
// it was broken, how it's supposed to work and the meanings of a few fields.  
// Bugs: 
//	c1tignor40: cdb trashes nodes' values which are set thru other ui dialogs
//	c1wright5:  click on the set value toggle repeatedly and the text varies
//	jkarol788, 
//	jkarol789:  cancel and restore buttons don't do anything
// N.B. These were filed against 7.x.  These bugs didn't exist in 7.0.2, but that
// doesn't mean you can refer to 7.0.2 code to see how it looked when it worked.
// 7.0.2 was broken also, but in different ways.
//
// Nutshell:
//	The restore and apply callbacks guarantee that upon completion, 
//	for each param
//	    input->initialValue == the value in the node 
//	restore resolves differences by moving values from input->initialValue into node.
//	apply resolves differences by moving values from the node into initialValue.
//	(basically...  Exception: if you type but don't hit <Enter> then handle that
//	a little differently.)
// 	The changeInput method is invoked by Node::ioParameterStatusChanged() and
//	is therefore extremely problematic.  If We touch a param, We can know that
//	the new value doesn't belong in initialValue, but if someone else touches
//	a param then We cannot know.  But if We don't have this knowledge, then
//	We won't know which params should be restored or applied.
//	Note that once a cdb is created, it maintains (for better or worse) its
//	collection of input objects all the time regardless of its isManaged() state.
// A few of things I did were minor, but the major change I made was to add logic
// to changeInput() to prevent some 
//
//
// input:	1 per param
// input->initialValue:	 equal to the value in the Node unless We've made a restorable
//	change.
// input->modified:	0 initially.  Set to 1 if the user makes a change.  Reset
//	in restore or apply.  This governs the action in changeInput.  If the user
//	has typed in new data then changeInput won't put new values into initialValue.
//	If the user hasn't touched a param using thru the cdb, then any update to that
// 	param goes into initialValue.  This is the change I made which makes the
//	restore,cancel,apply buttons work properly.
// input->valueChanged:  set to 1 if the value in the text widget has changed but
//	the value hasn't been used yet.  Set to 1 in a modifyVerifyCallback.
//

char* ConfigurationDialog::HelpText = 0;
boolean ConfigurationDialog::ClassInitialized = FALSE;
String ConfigurationDialog::DefaultResources[] =
{
    "*configNotationLabel.labelString:		Notation:",
    "*configNotationLabel.foreground:		SteelBlue",

    "*configOkButton.labelString:		OK",
    "*configApplyButton.labelString:		Apply",
    "*configExpandButton.labelString:		Expand",
    "*configCollapseButton.labelString:		Collapse",
    "*configDescriptionButton.labelString:	Description...",
    "*configSyntaxButton.labelString:		Help on Syntax",
    "*configRestoreButton.labelString:		Restore",
    "*configCancelButton.labelString:		Cancel",

    "*configInputTitle.labelString:		Inputs:",
    "*configInputTitle.foreground:		SteelBlue",
    "*configInputNameLabel.labelString:		Name",
    "*configInputNameLabel.foreground:		SteelBlue",
    "*configInputHideLabel.labelString:		Hide",
    "*configInputHideLabel.foreground:		SteelBlue",
    "*configInputTypeLabel.labelString:		Type",
    "*configInputTypeLabel.foreground:		SteelBlue",
    "*configInputSourceLabel.labelString:	Source",
    "*configInputSourceLabel.foreground:	SteelBlue",
    "*configInputValueLabel.labelString:	Value",
    "*configInputValueLabel.foreground:		SteelBlue",

    "*configInputNameLabel.leftOffset:		0",
    "*inputForm.configInputName.leftOffset:	0",
    "*configInputHideLabel.leftOffset:		130",
    "*inputForm.configInputHide.leftOffset:	140",
    "*configInputTypeLabel.leftOffset:		170",
    "*inputForm.configInputType.leftOffset:	170",
    "*configInputSourceLabel.leftOffset:	380",
    "*inputForm.configInputSource.leftOffset:	380",
    "*inputForm.configInputSource.rightOffset:	-555",
    "*configInputValueLabel.leftOffset:		560",
    "*inputForm.textPopup.leftOffset:		560",
#if !defined(LESSTIF_VERSION)
    "*inputForm.resizePolicy:			XmRESIZE_NONE",
#endif
    "*configOutputTitle.labelString:		Outputs:",
    "*configOutputTitle.foreground:		SteelBlue",
    "*configOutputNameLabel.labelString:	Name",
    "*configOutputNameLabel.foreground:		SteelBlue",
    "*configOutputTypeLabel.labelString:	Type",
    "*configOutputTypeLabel.foreground:		SteelBlue",
    "*configOutputDestinationLabel.labelString:	Destination",
    "*configOutputDestinationLabel.foreground:	SteelBlue",
    "*configOutputCacheLabel.labelString:	Cache",
    "*configOutputCacheLabel.foreground:	SteelBlue",
    "*cacheFull.labelString:			All Results",
    "*cacheLast.labelString:			Last Result",
    "*cacheOff.labelString:			No Results",

#if defined(aviion)
    "*configOutputSection.cacheOptionMenu.labelString:",
#endif

    NULL
};

#define MAX_INITIAL_BUTTONS 7
#define PIXELS_PER_LINE 30

// This is used to set XmNtopOffset for the togglebutton widgets.  It's significant
// because setting it to 4 makes the textPopup widget overlap and the cdb's 
// noticeably shorter.  Setting it to 7 makes the textPopup widgets not overlap
// and gives the appearance of a separator between each adjacent pair.
#define POPUP_OFFSET 5

ConfigurationDialog*
ConfigurationDialog::AllocateConfigurationDialog(Widget parent,
						 Node *node)
{
    return new ConfigurationDialog(parent, node);
}

boolean ConfigurationDialog::okCallback(Dialog *d)
{
    return this->applyCallback(d);
}
void ConfigurationDialog::helpCallback(Dialog *d)
{
    if (!this->descriptionDialog)
	this->descriptionDialog = new DescrDialog(this->getRootWidget(), 
						  this->node);
    this->descriptionDialog->post();
}
void ConfigurationDialog::cancelCallback(Dialog *d)
{
    this->unmanage();
    this->restoreCallback(d);
}
extern "C" void ConfigurationDialog_SyntaxCB(Widget widget,
                                             XtPointer clientdata,
                                             XtPointer)
{
     ConfigurationDialog *dialog = (ConfigurationDialog*)clientdata;
     InfoMessage(dialog->getHelpSyntaxString());
}
extern "C" void ConfigurationDialog_ApplyCB(Widget widget,
					XtPointer clientData,
					XtPointer)
{
    ConfigurationDialog *dialog = (ConfigurationDialog*)clientData;
    dialog->applyCallback(dialog);
}
    
boolean ConfigurationDialog::applyCallback(Dialog *)
{
    ListIterator li(this->inputList);
    int i;
    CDBInput *input;
    Node     *n = this->node;
    boolean anySet = FALSE;
    boolean retval = TRUE;

    for (i = 1; NULL != (input = (CDBInput*)li.getNext()); ++i)
    {
	if (input->valueChanged)
	{
	    if (this->widgetChanged(i, FALSE))
		anySet = TRUE;
	    else
		retval = FALSE;
	}

	const char *valueString;
	if (n->isInputDefaulting(i))
	{
	    valueString = n->getInputDefaultValueString(i);
	}
	else
	{
	    valueString = n->getInputSetValueString(i);
	}
	input->setInitialValue(valueString);
	input->initialValueIsDefault = n->isInputDefaulting(i);
	input->initialIsHidden = !n->isInputVisible(i);
	input->modified = 0;
    }

    if (anySet)
	n->sendValues(FALSE);

    const char *oldlabel = n->getLabelString();
    char *s = XmTextGetString(this->notation);
    if (!EqualString(oldlabel,s)) {
	if (n->setLabelString(s))  {
	    if (this->initialNotation)
		delete this->initialNotation;
	    this->initialNotation = DuplicateString(s);
	} else {
	    retval = FALSE;
	}
    }
    XtFree(s);

    return retval;
}
extern "C" void ConfigurationDialog_RestoreCB(Widget widget,
					XtPointer clientData,
					XtPointer)
{
    ConfigurationDialog *dialog = (ConfigurationDialog*)clientData;
    dialog->restoreCallback(dialog);
}
extern "C" void ConfigurationDialog_ExpandCB(Widget widget,
					XtPointer clientData,
					XtPointer)
{
    ConfigurationDialog *dialog = (ConfigurationDialog*)clientData;
    dialog->expandCallback(dialog);
}
extern "C" void ConfigurationDialog_CollapseCB(Widget widget,
					XtPointer clientData,
					XtPointer)
{
    ConfigurationDialog *dialog = (ConfigurationDialog*)clientData;
    dialog->collapseCallback(dialog);
}

//
// At the end of restoreCallback(), the set of initial values for each param
// must be equal to what the node is holding.  Inside restoreCallback, these
// 2 sets are compared for every param and where they're different, the initial
// values stored locally are sent to the node.
//
void ConfigurationDialog::restoreCallback(Dialog *clientData)
{
    ListIterator li(this->inputList);
    int i;
    CDBInput *input;
    Node *n = this->node;
    boolean anySet = FALSE;
    boolean callbacksWereEnabled;

    for (i = 1; NULL != (input = (CDBInput*)li.getNext()); ++i)
    {
	const char *defstr = n->getInputDefaultValueString(i);
	const char *inpstr = n->getInputValueString(i);
	boolean isdefing = n->isInputDefaulting(i);

  	if (!n->isInputViewable(i) || n->isInputConnected(i))
	    continue;

	callbacksWereEnabled = input->valueTextPopup->disableCallbacks();

	if (input->initialValueIsDefault)
	{
	    if (!n->isInputDefaulting(i))
	    {
		anySet = TRUE;
		n->useDefaultInputValue(i, FALSE);
	    }
	    else
		input->valueTextPopup->setText(defstr);
	}
	else
	{
	    if (!EqualString(input->initialValue, inpstr)) {
		if (EqualString(input->initialValue, defstr)) {
		    n->useDefaultInputValue(i);
		    input->setInitialValue (defstr);
		    input->valueTextPopup->setText(defstr);
		} else {
		    n->setInputValue(i,input->initialValue,DXType::UndefinedType,FALSE);
		    input->valueTextPopup->setText(input->initialValue);
		}
		anySet = TRUE;
	    } else {
		if (isdefing) {
		    n->setInputValue(i,input->initialValue,DXType::UndefinedType,FALSE);
		    anySet = TRUE;
		}
		input->valueTextPopup->setText(input->initialValue);
	    }
	}
	if (!n->isInputConnected(i) &&
	    (n->isInputVisible(i) != !input->initialIsHidden))
	    n->setInputVisibility(i, !input->initialIsHidden);

	if (callbacksWereEnabled)
	    input->valueTextPopup->enableCallbacks();

	input->valueChanged = FALSE;
	input->modified = 0;
    }

    if (anySet)
	n->sendValues(FALSE);

    char *s = XmTextGetString(this->notation);
    if (!EqualString(s,this->initialNotation)) {
	XmTextSetString(this->notation, this->initialNotation);
	n->setLabelString(this->initialNotation);
    }
    XtFree(s);
}
void ConfigurationDialog::ActivateInputValueCB(TextPopup *tp,
						const char *value, 
						void *clientData)
{

    int i;
    ConfigurationDialog *dialog = (ConfigurationDialog *)clientData;
    for (i = 1; i <= dialog->node->getInputCount(); ++i)
    {
	CDBInput *input = (CDBInput *)dialog->inputList.getElement(i);
	if (input->valueTextPopup == tp)
	{
	    input->modified = 1;
	    dialog->widgetChanged(i);
	    break;
	}
    }
}
void ConfigurationDialog::ValueChangedInputValueCB( TextPopup *tp,
                                                const char *value,
                                                void *clientData)

{
    int i;
    ConfigurationDialog *dialog = (ConfigurationDialog *)clientData;
    for (i = 1; i <= dialog->node->getInputCount(); ++i)
    {
	CDBInput *input = (CDBInput *)dialog->inputList.getElement(i);
	if (input->valueTextPopup == tp)
	{
	    input->modified = 1;
	    input->valueChanged = TRUE;
	    break;
	}
    }
}
extern "C" void ConfigurationDialog_ActivateOutputCacheCB(Widget widget,
					      XtPointer clientData,
					      XtPointer)
{
    int i;
    ConfigurationDialog *dialog = (ConfigurationDialog *)clientData;
    for (i = 1; i <= dialog->node->getOutputCount(); ++i)
    {
	CDBOutput *output = (CDBOutput *)dialog->outputList.getElement(i);
	if (output->fullButton == widget)
	{
	    dialog->node->setOutputCacheability(i, OutputFullyCached);
	    break;
	}
	else if (output->lastButton == widget)
	{
	    dialog->node->setOutputCacheability(i, OutputCacheOnce);
	    break;
	}
	else if (output->offButton == widget)
	{
	    dialog->node->setOutputCacheability(i, OutputNotCached);
	    break;
	}
    }
}

//
extern "C" void ConfigurationDialog_ValueChangedInputNameCB(Widget widget,
						XtPointer clientData,
						XtPointer)
{
    int i;
    ConfigurationDialog *dialog = (ConfigurationDialog *)clientData;
    Node *n = dialog->node;
    char pname[128];

    for (i = 1; i <= dialog->node->getInputCount(); ++i)
    {
	CDBInput *input = (CDBInput *)dialog->inputList.getElement(i);
	if (input->nameWidget == widget)
	{
	    input->modified = 1;
	    Boolean set;
	    XtVaGetValues(widget, XmNset, &set, NULL);
	    if (set && dialog->node->isInputDefaulting(i))
	    {
		//
		// If the user has changed the value widget, get the current
		// string and save it in the parameter.  Otherwise, 
		// get the value from the parameter and put it in the 
		// widget.
		const char *defstr = dialog->node->getInputDefaultValueString(i);
		const char *setstr = dialog->node->getInputSetValueString(i);
		boolean isset = (
		    (!EqualString (defstr, setstr)) && 
		    (!EqualString (setstr, "NULL"))
		);
		if (input->valueChanged)
		{
		    dialog->widgetChanged(i);
		}
		else if (isset)
		{
		    input->valueTextPopup->setText(setstr);
		    dialog->node->useAssignedInputValue(i);
		}
		else 
		{
		    n->setInputValue(i, "NULL");
		}
		input->valueChanged = FALSE;
	    }
	    else if (!set && !n->isInputDefaulting(i))
	    {
		if (input->valueChanged)
		{
		    char *s = input->valueTextPopup->getText(); 

		    if (!EqualString(s, n->getInputDefaultValueString(i)) &&
		        *s != '\0')
		    {
			if (n->setInputValue(i, s, DXType::UndefinedType, FALSE)				== DXType::UndefinedType)
			{
			    ErrorMessage(
				"String `%s' is not a valid value for "
				    "%s parameter '%s'",
				s, n->getNameString(),
				n->getInputNameString(i,pname));
			}
		    }
		    XtFree(s);
		}
		dialog->node->useDefaultInputValue(i);
		input->valueTextPopup->setText(
		    dialog->node->getInputDefaultValueString(i));
		input->valueChanged = FALSE;
	    }
	    break;
	}
    }
}
//
extern "C" void ConfigurationDialog_ValueChangedInputHideCB(Widget widget,
						XtPointer clientData,
						XtPointer)
{
    int i;
    ConfigurationDialog *dialog = (ConfigurationDialog *)clientData;
    for (i = 1; i <= dialog->node->getInputCount(); ++i)
    {
	CDBInput *input = (CDBInput *)dialog->inputList.getElement(i);
	if (input->hideWidget == widget)
	{
	    input->modified = 1;
	    Boolean set;
	    XtVaGetValues(widget, XmNset, &set, NULL);
	    dialog->node->setInputVisibility(i, (boolean)!set);
	    if (set)
		XtSetSensitive(dialog->collapse, True);
	    break;
	}
    }
}

void ConfigurationDialog::expandCallback(Dialog *clientData)
{
    this->remanageInputs(TRUE);
}
void ConfigurationDialog::collapseCallback(Dialog *clientData)
{
    this->remanageInputs(FALSE);
}

extern "C" void ConfigurationDialog_ResizeCB(Widget widget,
					XtPointer clientData,
					XtPointer)
{
ConfigurationDialog *dialog = (ConfigurationDialog*)clientData;
    ASSERT(dialog);
    dialog->resizeCallback();
}

//
// The contents of a scrolled window widget will not change size by itself
// nor will it have a size imposed on it by the scrolled window.  If the 
// user resizes the window, then code must query for new dimensions and then
// force a new size on the contents of the scrolled window.
//
void ConfigurationDialog::resizeCallback()
{
Dimension dw;

    if ((!this->getRootWidget()) || (!this->scrolledInputForm)) return ;
    XtVaGetValues (this->getRootWidget(), XmNwidth, &dw, NULL);
    if (dw<100) return ;

    Pixel ts,bs,bg;
    Dimension sh,fh;
    XtVaGetValues (this->inputScroller, 
	XmNtopShadowColor,	&ts,
	XmNbottomShadowColor,	&bs,
	XmNbackground,		&bg,
	XmNheight,		&sh,
    NULL);

    XtVaGetValues (this->scrolledInputForm, XmNheight, &fh, NULL);
    if (fh > sh) {
	XtVaSetValues (XtParent(this->inputScroller), 
	    XmNtopShadowColor,		ts,
	    XmNbottomShadowColor,	bs,
	NULL);
	XtVaSetValues (this->scrolledInputForm, XmNwidth, dw-35, NULL);
    } else {
	XtVaSetValues (XtParent(this->inputScroller), 
	    XmNtopShadowColor,		bg,
	    XmNbottomShadowColor,	bg,
	NULL);
	XtVaSetValues (this->scrolledInputForm, XmNwidth, dw-20, NULL);
    }
}

Widget ConfigurationDialog::createInputs(Widget parent, Widget)
{
    Node *n = this->node;
    int numParam = n->getInputCount();
    int j = 0;
    Arg args[15];


    Widget inputs = this->inputForm = XtVaCreateManagedWidget(
	"configInputSection",
	xmFormWidgetClass,
	parent,
	NULL);

    this->inputTitle = XtVaCreateWidget(
	"configInputTitle",
	xmLabelWidgetClass,
	inputs,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    Widget swp_form = XtVaCreateWidget ( "scroller_parent",	
	xmFrameWidgetClass,	inputs,
	XmNtopAttachment, 	XmATTACH_WIDGET,
	XmNtopWidget, 		this->inputTitle,
	XmNleftAttachment, 	XmATTACH_FORM,
	XmNrightAttachment, 	XmATTACH_FORM,
	XmNbottomAttachment, 	XmATTACH_FORM,
	XmNtopOffset, 		10,
	XmNshadowThickness, 	2,
    NULL);

    j = 0;
#if 0
    XtSetArg (args[j], XmNtopAttachment, XmATTACH_WIDGET); j++;
    XtSetArg (args[j], XmNtopWidget, this->inputTitle); j++;
    XtSetArg (args[j], XmNleftAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNrightAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNbottomAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNtopOffset, 10); j++;
#endif
    XtSetArg (args[j], XmNshadowThickness, 0); j++;
    XtSetArg (args[j], XmNscrollingPolicy, XmAUTOMATIC); j++;
    XtSetArg (args[j], XmNspacing, 0); j++;
    XtSetArg (args[j], XmNleftOffset, 0); j++;
    XtSetArg (args[j], XmNrightOffset, 0); j++;
    XtSetArg (args[j], XmNwidth, 790); j++;
    this->inputScroller = XmCreateScrolledWindow (swp_form, "inputScroller", args, j);
    XtManageChild(this->inputScroller);

    this->scrolledInputForm = XtVaCreateManagedWidget (
	"inputForm",
	xmFormWidgetClass,
	this->inputScroller,
	NULL
    );
    XtVaSetValues (this->inputScroller, 
	XmNworkWindow, this->scrolledInputForm, 
    NULL);

    Widget hsb;
    XtVaGetValues (this->inputScroller, XmNhorizontalScrollBar, &hsb, NULL);
    if (hsb) XtUnmanageChild (hsb);

    int i;
    for (i = 1; i <= numParam; ++i) 
	this->newInput(i);

    //
    // Set the height of the scrolled window.
    //
    int visCnt = 0;
    for (i=1; i<=numParam; i++) {
	if (!n->isInputViewable(i)) continue;
	if (!n->isInputVisible(i)) continue;
	visCnt++;
    }
    if (visCnt > MAX_INITIAL_BUTTONS) visCnt = MAX_INITIAL_BUTTONS;
    if (visCnt <= 0) visCnt = 1;
    XtVaSetValues (this->inputScroller, XmNheight, 5+(visCnt*PIXELS_PER_LINE), NULL);

    j = 0;
    XtSetArg (args[j], XmNtopAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNleftAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNrightAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNbottomAttachment, XmATTACH_FORM); j++;
    XtSetArg (args[j], XmNmappedWhenManaged, False); j++;
    Widget drawa = XmCreateDrawingArea (inputs, "junk", args, j);
    XtManageChild (drawa);
    XtAddCallback (drawa, XmNresizeCallback, 
	(XtCallbackProc)ConfigurationDialog_ResizeCB, (XtPointer)this);

    return inputs;
}

Widget ConfigurationDialog::createOutputs(Widget parent, Widget )
{
    Node *n = this->node;
    int numParam = n->getOutputCount();
    if (numParam == 0)
	return NULL;

    Widget outputs = XtVaCreateManagedWidget(
	"configOutputSection",
	xmFormWidgetClass,
	parent,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , 5,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , 5,
	NULL);

    this->outputTitle = XtVaCreateManagedWidget(
	"configOutputTitle",
	xmLabelWidgetClass,
	outputs,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->outputNameLabel = XtVaCreateManagedWidget(
	"configOutputNameLabel",
	xmLabelWidgetClass,
	outputs,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->outputTitle,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNwidth           , 130,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->outputTypeLabel = XtVaCreateManagedWidget(
	"configOutputTypeLabel",
	xmLabelWidgetClass,
	outputs,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->outputTitle,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget      , this->outputNameLabel,
        XmNalignment       , XmALIGNMENT_BEGINNING,
        XmNwidth           , 180,
	NULL);

    this->outputDestLabel = XtVaCreateManagedWidget(
	"configOutputDestinationLabel",
	xmLabelWidgetClass,
	outputs,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->outputTitle,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget      , this->outputTypeLabel,
        XmNalignment       , XmALIGNMENT_BEGINNING,
        XmNwidth           , 200,
	NULL);

    this->outputCacheLabel = XtVaCreateManagedWidget(
	"configOutputCacheLabel",
	xmLabelWidgetClass,
	outputs,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->outputTitle,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget      , this->outputDestLabel,
        XmNwidth           , 120,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    int i;
    for (i = 1; i <= numParam; ++i)
	this->newOutput(i);

    return outputs;
}



// top == this->notation
Widget ConfigurationDialog::createBody(Widget parent)
{
    this->typeWidth = 160;
    this->outputForm = this->createOutputs(parent, NULL);
    if (this->outputForm) {
	XtVaSetValues (this->outputForm, 
	    XmNbottomAttachment,   XmATTACH_WIDGET,
	    XmNbottomWidget,       this->buttonSeparator,
	    XmNbottomOffset,       4,
	NULL);
    }

    this->inputForm = this->createInputs(parent, this->notation);
    if (this->inputForm) {
	XtVaSetValues (this->inputForm,
	    XmNtopAttachment,   XmATTACH_WIDGET,
	    XmNtopWidget,       this->notation,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget,     (this->outputForm? this->outputForm: this->buttonSeparator),
	    XmNbottomOffset	   , 4,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNleftOffset      , 5,
	    XmNrightAttachment , XmATTACH_FORM,
	    XmNrightOffset     , 5,
	NULL);
    }
    return this->outputForm == NULL? this->inputForm: this->outputForm;
}

Widget ConfigurationDialog::createDialog(Widget parent)
{
    Arg arg[10];
    int  n = 0;

    XtSetArg(arg[n], XmNminWidth, 770); n++;
    XtSetArg(arg[n], XmNwidth, 800); n++;
    XtSetArg(arg[n], XmNminHeight, 95); n++;
    XtSetArg(arg[n], XmNallowShellResize, True); n++;

//    Widget dialog = XmCreateFormDialog(parent, this->name, arg, n);
    Widget dialog = this->CreateMainForm(parent, this->name, arg, n);
    XtSetValues (XtParent(dialog), arg, n);

    XtVaSetValues(XtParent(dialog),
	XmNtitle, this->node->getNameString(),
	NULL);

    Widget label = XtVaCreateManagedWidget(
	"configNotationLabel",
	xmLabelWidgetClass,
	dialog,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNtopOffset,       10,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNleftOffset,      5,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    this->notation = XtVaCreateManagedWidget(
	"configNotationText",
	xmTextWidgetClass,
	dialog,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNtopOffset,       10,
	XmNleftAttachment,  XmATTACH_WIDGET,
	XmNleftWidget,      label,
	XmNleftOffset,      10,
	XmNrightAttachment, XmATTACH_FORM,
	XmNrightOffset,     5,
	XmNeditMode,        XmSINGLE_LINE_EDIT,
	NULL);
    XmTextSetString(this->notation, (char*)this->node->getLabelString());


    Widget buttonForm = XtVaCreateManagedWidget(
	"configButtonForm",
	xmFormWidgetClass,
	dialog,
	XmNbottomAttachment    , XmATTACH_FORM,
	XmNbottomOffset        , 10,
	XmNleftAttachment   , XmATTACH_FORM,
	XmNleftOffset       , 5,
	XmNrightAttachment  , XmATTACH_FORM,
	XmNrightOffset      , 5,
	NULL);

    this->buttonSeparator = XtVaCreateManagedWidget(
	"configButtonSeparator",
	xmSeparatorWidgetClass,
	dialog,
	XmNbottomAttachment,    XmATTACH_WIDGET,
	XmNbottomWidget,        buttonForm,
	XmNbottomOffset,     6,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNleftOffset,       0,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNrightOffset,      0,
	NULL);

    this->ok = XtVaCreateManagedWidget(
	"configOkButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNleftAttachment , XmATTACH_FORM,
	XmNwidth          , 70,
	NULL);

    this->apply = XtVaCreateManagedWidget(
	"configApplyButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNleftAttachment , XmATTACH_WIDGET,
	XmNleftWidget     , this->ok,
	XmNleftOffset     , 10,
	XmNwidth          , 70,
	NULL);

    this->expand = XtVaCreateManagedWidget(
	"configExpandButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNleftAttachment , XmATTACH_WIDGET,
	XmNleftWidget     , this->apply,
	XmNleftOffset     , 10,
	XmNwidth          , 70,
	NULL);
    XtSetSensitive(this->expand, False);

    this->collapse = XtVaCreateManagedWidget(
	"configCollapseButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNleftAttachment , XmATTACH_WIDGET,
	XmNleftWidget     , this->expand,
	XmNleftOffset     , 10,
	XmNwidth          , 70,
	NULL);
    XtSetSensitive(this->collapse, False);

    this->help = XtVaCreateManagedWidget(
	"configDescriptionButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNleftAttachment , XmATTACH_WIDGET,
	XmNleftWidget     , this->collapse,
	XmNleftOffset     , 10,
	XmNwidth          , 110,
	NULL);

    this->syntax = XtVaCreateManagedWidget(
	"configSyntaxButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNleftAttachment , XmATTACH_WIDGET,
	XmNleftWidget     , this->help,
	XmNleftOffset     , 10,
	XmNwidth          , 110,
	NULL);


    this->cancel = XtVaCreateManagedWidget(
	"configCancelButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNwidth          , 70,
	NULL);

    this->restore = XtVaCreateManagedWidget(
	"configRestoreButton",
	xmPushButtonWidgetClass,
	buttonForm,
	XmNtopAttachment  , XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_WIDGET,
	XmNrightWidget    , this->cancel,
	XmNrightOffset    , 10,
	XmNwidth          , 70,
	NULL);

    //
    // A Container for inputs and outputs.
    // Must be done last becuase it requires this->buttonSeparator
    //
    this->createBody(dialog);

    return dialog;
}


//
//  This constructor is for derived classes (not instances of this class).
//
ConfigurationDialog::ConfigurationDialog(const char *name, 
					Widget parent, Node *node):
    			Dialog(name, parent)
{
    this->initInstanceData(node);
}

//
//  This constructor is for instances of this class (not derived classes).
//
ConfigurationDialog::ConfigurationDialog(Widget parent, Node *node):
    			Dialog("configurationDialog", parent)
{
    this->initInstanceData(node);

    if (NOT ConfigurationDialog::ClassInitialized)
    {
        ConfigurationDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
//
// Initialize instance data for the constructors
//
void ConfigurationDialog::initInstanceData(Node *node)
{
    this->node = node;
    this->notation = NULL;
    this->apply = NULL;
    this->restore = NULL;
    this->expand = NULL;
    this->collapse = NULL;

    this->descriptionDialog = NULL;

    this->inputNameLabel = NULL;
    this->inputTypeLabel = NULL;
    this->inputSourceLabel = NULL;
    this->inputValueLabel = NULL;
    this->outputForm = NULL;
    this->inputForm = NULL;
    this->inputScroller = NULL;
    this->scrolledInputForm = NULL;

    this->outputNameLabel = NULL;
    this->outputTypeLabel = NULL;
    this->outputDestLabel = NULL;

    this->initialNotation = DuplicateString(this->node->getLabelString());
}

ConfigurationDialog::~ConfigurationDialog()
{
    int i;
    for (i = 1; i <= this->inputList.getSize(); ++i)
    {
	CDBInput *input = (CDBInput*)this->inputList.getElement(i);
	delete input;
    }
    this->inputList.clear();
    for (i = 1; i <= this->outputList.getSize(); ++i)
    {
	CDBOutput *output = (CDBOutput*)this->outputList.getElement(i);
	delete output;
    }
    this->outputList.clear();

    if (this->initialNotation)
	delete this->initialNotation;
    if (this->descriptionDialog)
	delete this->descriptionDialog;
}

void ConfigurationDialog::post()
{
    ListIterator li(this->inputList);
    int i;
    CDBInput *input;

    for (i = 1; NULL != (input = (CDBInput*)li.getNext()); ++i)
    {
        input->valueChanged = FALSE;
    }

    boolean firstTime = this->getRootWidget() == NULL;

    this->Dialog::post();

    if (firstTime)
    {
	if (this->apply)
	    XtAddCallback(this->apply, XmNactivateCallback,
		(XtCallbackProc)ConfigurationDialog_ApplyCB, (XtPointer)this);
	if (this->syntax)
	    XtAddCallback(this->syntax, XmNactivateCallback,
		(XtCallbackProc)ConfigurationDialog_SyntaxCB, (XtPointer)this);
	if (this->restore)
	    XtAddCallback(this->restore, XmNactivateCallback,
		(XtCallbackProc)ConfigurationDialog_RestoreCB, (XtPointer)this);
	if (this->expand)
	    XtAddCallback(this->expand, XmNactivateCallback,
		(XtCallbackProc)ConfigurationDialog_ExpandCB, (XtPointer)this);
	if (this->collapse)
	    XtAddCallback(this->collapse, XmNactivateCallback,
		(XtCallbackProc)ConfigurationDialog_CollapseCB, (XtPointer)this);
    }
}

void ConfigurationDialog::changeInput(int i)
{
boolean callbacksWereEnabled;
Node *n = this->node;


    CDBInput *input = (CDBInput*) this->inputList.getElement(i);

    if ((!input->modified) && (!n->isInputConnected(i))) {
	const char *valueString;
	if (n->isInputDefaulting(i))
	{
	    valueString = n->getInputDefaultValueString(i);
	}
	else
	{
	    valueString = n->getInputSetValueString(i);
	}
	input->setInitialValue(valueString);
	input->initialValueIsDefault = n->isInputDefaulting(i);
	input->initialIsHidden = !n->isInputVisible(i);
    }

    if (input->nameWidget != NULL)
    {
	char pname[128];
	boolean connected  = n->isInputConnected(i);
	boolean defaulting = n->isInputDefaulting(i);
	boolean visible = n->isInputVisible(i);
	int  nin = n->getInputCount();

	// Temporarily disable callbacks.
	callbacksWereEnabled = input->valueTextPopup->disableCallbacks();
	XtRemoveCallback(input->nameWidget, XmNvalueChangedCallback,
	    (XtCallbackProc)ConfigurationDialog_ValueChangedInputNameCB, (XtPointer)this);
	XtRemoveCallback(input->hideWidget, XmNvalueChangedCallback,
	    (XtCallbackProc)ConfigurationDialog_ValueChangedInputHideCB, (XtPointer)this);


	//if (this->getRootWidget())
	    //XtVaSetValues(this->getRootWidget(),
		//XmNresizePolicy, XmRESIZE_NONE,
		//NULL);

	XmString name = XmStringCreate((char *)n->getInputNameString(i,pname),
				       XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(input->nameWidget,
	    XmNset, connected || !defaulting,
	    XmNlabelString, name,
	    XmNsensitive, !connected,
	    NULL);
	XmStringFree(name);

	XtVaSetValues(input->hideWidget,
	    XmNset, (Boolean)!visible,
	    NULL);
	XtSetSensitive(input->hideWidget, (Boolean)!n->isInputConnected(i));
	if (!visible)
	    XtSetSensitive(this->collapse, True);

	const char * const*typeString = n->getInputTypeStrings(i);
	XmString typeList;
	if (typeString[0] != NULL)
	{
	    const char * const*ts = typeString;
	    int len = STRLEN(*ts) + 1;
	    ++ts;
	    while (*ts != NULL)
	    {
		len += STRLEN(*ts) + 2;
		++ts;
	    }
	    ts = typeString;
	    char *s = new char[len];
	    strcpy(s, *ts);
	    ++ts;
	    while (*ts != NULL)
	    {
		strcat(s, ", ");
		strcat(s, *ts);
		++ts;
	    }
	    typeList = XmStringCreate(s, XmSTRING_DEFAULT_CHARSET);
	    delete s;
	}
	else
	    typeList = XmStringCreate("", XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(input->typeWidget, XmNlabelString, typeList, NULL);
	XmStringFree(typeList);
	
	const char *sourceString;
	if (n->isInputConnected(i))
	{
	    List *arcs = (List *)n->getInputArks(i);
	    Ark *a = (Ark*)arcs->getElement(1);
	    int paramNum;
	    Node *source = a->getSourceNode(paramNum);
	    sourceString = source->getNameString();
	    XmString s = XmStringCreate((char *)sourceString,
					XmSTRING_DEFAULT_CHARSET);
	    XtVaSetValues(input->connectedToWidget,
		XmNlabelString,	s,
		NULL);
	    XmStringFree(s);

	    input->valueTextPopup->setText(
			    source->getOutputValueString(paramNum));
	}
	else {
	    XmString s = XmStringCreate("", XmSTRING_DEFAULT_CHARSET);
	    XtVaSetValues(input->connectedToWidget,
		XmNlabelString,	s,
		NULL);
	    XmStringFree(s);

	    const char *valueString;
	    if (n->isInputDefaulting(i))
	    {
		valueString = n->getInputDefaultValueString(i);
	    }
	    else
	    {
		valueString = n->getInputSetValueString(i);
	    }
	    char *oldValueString = input->valueTextPopup->getText();
	    if  (!EqualString(oldValueString, valueString)) {
		input->valueTextPopup->setText(valueString);
		input->valueChanged = FALSE;
	    }
	    XtFree(oldValueString);
	}
	Boolean sens = !n->isInputConnected(i);
 	if (sens)
	    input->valueTextPopup->activate();
	else
	    input->valueTextPopup->deactivate();

	//if (this->getRootWidget())
	    //XtVaSetValues(this->getRootWidget(),
		//XmNresizePolicy, XmRESIZE_ANY,
		//NULL);

	if (!XtIsManaged(input->nameWidget))
	{
	    Widget top = NULL;
	    if (!XtIsManaged(this->inputNameLabel))
		this->remanageInputs(FALSE);
	    else
	    {
		Widget managed[10];
		int nmanaged = 0;
		int j;
		for (j = 1; j < i; ++j)
		{
		    CDBInput *input2 = (CDBInput*)this->inputList.getElement(j);
		    if (input2 != NULL && input2->nameWidget != NULL && 
			XtIsManaged(input2->nameWidget))
		    {
			top = input2->nameWidget;
		    }
		}
		managed[nmanaged++] = input->nameWidget;
		if (top) {
		    XtVaSetValues(input->nameWidget, 
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, top, 
		    NULL);
		} else {
		    XtVaSetValues(input->nameWidget,XmNtopAttachment,XmATTACH_FORM,NULL);
		}

		managed[nmanaged++] = input->hideWidget;
		managed[nmanaged++] = input->typeWidget;
		managed[nmanaged++] = input->connectedToWidget;

		if (n->isInputVisible(i))
		    top = input->nameWidget;
		Widget next = NULL;
		for (j = i+1; j <= nin; ++j)
		{
		    CDBInput *input2 = (CDBInput*)this->inputList.getElement(j);
		    if (input2 != NULL && input2->nameWidget != NULL &&
			XtIsManaged(input2->nameWidget))
		    {
			next = input2->nameWidget;
			break;
		    }
		}
		if (n->isInputVisible(i)) {
		    if (!XtIsManaged(input->valueTextPopup->getRootWidget())) {
			Dimension ifh;
			//
			// Grow the window
			//
			XtVaGetValues (this->scrolledInputForm, XmNheight, &ifh, NULL);
			XtVaSetValues (this->scrolledInputForm, 
			    XmNheight, ifh+PIXELS_PER_LINE, NULL);

			XtVaGetValues (this->inputForm, XmNheight, &ifh, NULL);
			if (ifh < (5+(MAX_INITIAL_BUTTONS * PIXELS_PER_LINE))) {
			    XtVaSetValues (this->inputScroller, 
				XmNheight, ifh+PIXELS_PER_LINE,
			    NULL);
			} 
			this->resizeCallback();
		    }
		    input->valueTextPopup->manage();
		    XtManageChildren(managed, nmanaged);
		    if (next) XtVaSetValues(next, XmNtopWidget, input->nameWidget, NULL);
		    XtSetSensitive(this->collapse, True);
		    if (!XtIsManaged(XtParent(this->inputScroller)))
			XtManageChild (XtParent(this->inputScroller));

		    
		} else {
		    if (XtIsManaged(input->valueTextPopup->getRootWidget())) {
			Dimension ifh;
			//
			// Shrink the window
			//
			XtVaGetValues (this->scrolledInputForm, XmNheight, &ifh, NULL);
			if (ifh > PIXELS_PER_LINE)
			    XtVaSetValues (this->scrolledInputForm, 
				XmNheight, ifh-PIXELS_PER_LINE, 
			    NULL);
		    }
		    input->valueTextPopup->unmanage();
		    XtUnmanageChildren(managed, nmanaged);
		    XtSetSensitive(this->expand, True);
		}
	    }
	}

	if (callbacksWereEnabled)
	    input->valueTextPopup->enableCallbacks();
	XtAddCallback(input->nameWidget, XmNvalueChangedCallback,
	    (XtCallbackProc)ConfigurationDialog_ValueChangedInputNameCB, (XtPointer)this);
	XtAddCallback(input->hideWidget, XmNvalueChangedCallback,
	    (XtCallbackProc)ConfigurationDialog_ValueChangedInputHideCB, (XtPointer)this);

    }

}
void ConfigurationDialog::changeOutput(int i)
{
    Node *n = this->node;
    CDBOutput *output = (CDBOutput*)this->outputList.getElement(i);

    if (output->nameWidget != NULL)
    {
	char pname[128];
	//if (this->getRootWidget())
	    //XtVaSetValues(this->getRootWidget(),
		//XmNresizePolicy, XmRESIZE_NONE,
		//NULL);

	XmString name = XmStringCreate((char *)n->getOutputNameString(i,pname),
				       XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(output->nameWidget,
	    XmNlabelString, name,
	    NULL);
	XmStringFree(name);

	const char * const*typeString = n->getOutputTypeStrings(i);
	XmString typeList;
	if (typeString[0] != NULL)
	{
	    const char * const*ts = typeString;
	    int len = STRLEN(*ts) + 1;
	    ++ts;
	    while (*ts != NULL)
	    {
		len += STRLEN(*ts) + 2;
		++ts;
	    }
	    ts = typeString;
	    char *s = new char[len];
	    strcpy(s, *ts);
	    ++ts;
	    while (*ts != NULL)
	    {
		strcat(s, ", ");
		strcat(s, *ts);
		++ts;
	    }
	    typeList = XmStringCreate(s, XmSTRING_DEFAULT_CHARSET);
	    delete s;
	}
	else
	    typeList = XmStringCreate("", XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(output->typeWidget, XmNlabelString, typeList, NULL);
	XmStringFree(typeList);

	XmString s;
	if (n->isOutputConnected(i))
	{
	    char *destString;
	    List *arcs = (List *)n->getOutputArks(i);
	    ListIterator li(*arcs);
	    Ark *a = (Ark*)li.getNext();
	    int dummy;
	    Node *dest = a->getDestinationNode(dummy);
	    int len = STRLEN(dest->getNameString()) + 1;
	    while ((a = (Ark*)li.getNext()) != NULL)
	    {
		dest = a->getDestinationNode(dummy);
		len += STRLEN(dest->getNameString()) + 2;
	    }
	    destString = new char[len];
	    li.setList(*arcs);
	    a = (Ark*)li.getNext();
	    dest = a->getDestinationNode(dummy);
	    strcpy(destString, dest->getNameString());
	    while ((a = (Ark*)li.getNext()) != NULL)
	    {
		dest = a->getDestinationNode(dummy);
		strcat(destString, ", ");
		strcat(destString, dest->getNameString());
	    }
	    s = XmStringCreate(destString, XmSTRING_DEFAULT_CHARSET);
	    delete destString;
	}
	else {
	    s = XmStringCreate("", XmSTRING_DEFAULT_CHARSET);
	}
	XtVaSetValues(output->connectedToWidget,
	    XmNlabelString,	s,
	    NULL);
	XmStringFree(s);

	switch (n->getOutputCacheability(i)) {
	case OutputFullyCached:
	    XtVaSetValues(output->cacheWidget,
		XmNmenuHistory, output->fullButton, NULL);
	    break;
	case OutputCacheOnce:
	    XtVaSetValues(output->cacheWidget,
		XmNmenuHistory, output->lastButton, NULL);
	    break;
	case OutputNotCached:
	    XtVaSetValues(output->cacheWidget,
		XmNmenuHistory, output->offButton, NULL);
	    break;
	}

	XtSetSensitive(output->cacheWidget, 
			n->isOutputCacheabilityWriteable(i));

	//if (this->getRootWidget())
	    //XtVaSetValues(this->getRootWidget(),
		//XmNresizePolicy, XmRESIZE_ANY,
		//NULL);
    }
}
void ConfigurationDialog::newInput(int i)
{
    Arg args[10];
    int m;
    Node *n = this->node;
    CDBInput *input = (CDBInput*)this->inputList.getElement(i);
    if (input == NULL)
    {
	input = new CDBInput;
	this->inputList.insertElement(input, i);
    }

    if (!n->isInputViewable(i))
	return;

    Widget prevNameWidget = NULL;
    Widget prevTextPopup = NULL;
    int j;
    for (j = i - 1; j > 0; --j)
    {
	CDBInput *prevInput = (CDBInput*)this->inputList.getElement(j);
	if (prevInput != NULL && prevInput->nameWidget != NULL &&
	    !prevInput->lineIsHidden)
	{
	    prevNameWidget = prevInput->nameWidget;
	    prevTextPopup = prevInput->valueTextPopup->getRootWidget();
	    break;
	}
    }

    if (prevNameWidget == NULL)
    {
	if (this->inputNameLabel == NULL)
	{
	    XtManageChild(this->inputTitle);

	    this->inputNameLabel = XtVaCreateManagedWidget(
		"configInputNameLabel",
		xmLabelWidgetClass,
		this->inputForm,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNleftAttachment  , XmATTACH_FORM,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		XmNallowResize,     False,
		NULL);

	    XtVaSetValues (XtParent(this->inputScroller), XmNtopWidget, this->inputNameLabel, NULL);

	    this->inputHideLabel = XtVaCreateManagedWidget(
		"configInputHideLabel",
		xmLabelWidgetClass,
		this->inputForm,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNleftAttachment  , XmATTACH_FORM,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		XmNallowResize,     False,
		NULL);

	    this->inputTypeLabel = XtVaCreateManagedWidget(
		"configInputTypeLabel",
		xmLabelWidgetClass,
		this->inputForm,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNleftAttachment  , XmATTACH_FORM,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		NULL);

	    this->inputSourceLabel = XtVaCreateManagedWidget(
		"configInputSourceLabel",
		xmLabelWidgetClass,
		this->inputForm,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNleftAttachment  , XmATTACH_FORM,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		NULL);

	    this->inputValueLabel = XtVaCreateManagedWidget(
		"configInputValueLabel",
		xmLabelWidgetClass,
		this->inputForm,
		XmNtopAttachment   , XmATTACH_WIDGET,
		XmNtopWidget       , this->inputTitle,
		XmNleftAttachment  , XmATTACH_FORM,
		XmNrightAttachment , XmATTACH_FORM,
		XmNalignment       , XmALIGNMENT_BEGINNING,
		NULL);
	}
    }

    Widget name;
    if (!prevNameWidget) {
	name = XtVaCreateWidget(
	    "configInputName", 
	    xmToggleButtonWidgetClass,
	    this->scrolledInputForm,
	    XmNtopAttachment,   XmATTACH_FORM,
	    NULL);
    } else {
	name = XtVaCreateWidget(
	    "configInputName", 
	    xmToggleButtonWidgetClass,
	    this->scrolledInputForm,
	    XmNtopAttachment,   XmATTACH_WIDGET,
	    XmNtopWidget,       prevNameWidget,
	    NULL);
    }

    XtVaSetValues (name,
	XmNuserData,        this,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	XmNset,       	    (Boolean)(n->isInputConnected(i) ||
				     !n->isInputDefaulting(i)),
	XmNallowResize,     False,
	XmNshadowThickness, 0,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNtopOffset,       POPUP_OFFSET,
	NULL);

    input->nameWidget = name;
    XtVaGetValues(name,
	XmNtopShadowColor,    &this->standardTopShadowColor,
	XmNbottomShadowColor, &this->standardBottomShadowColor,
	NULL);
	
    XmString null = XmStringCreate("", XmSTRING_DEFAULT_CHARSET);
    Widget hide = XtVaCreateWidget(
	"configInputHide", 
	xmToggleButtonWidgetClass,
	this->scrolledInputForm,
	XmNtopAttachment,   XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,       input->nameWidget,
	XmNtopOffset,       0,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNuserData,        this,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	XmNset,       	    (Boolean)(!n->isInputVisible(i)),
	XmNallowResize,     False,
	XmNlabelString,     null,
	XmNshadowThickness, 0,
	NULL);
    input->hideWidget = hide;
    input->initialIsHidden = !n->isInputVisible(i);
    XmStringFree(null);

    Widget type = XtVaCreateWidget(
	"configInputType", 
	xmLabelWidgetClass,
	this->scrolledInputForm,
	XmNtopAttachment,   XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,       input->nameWidget,
	XmNtopOffset,       0,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    input->typeWidget = type;

    Widget source = XtVaCreateWidget(
	"configInputSource",
	xmLabelWidgetClass,
	this->scrolledInputForm,
	XmNtopAttachment,   XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,       input->nameWidget,
	XmNtopOffset,       0,
	XmNbottomAttachment,   XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,       input->nameWidget,
	XmNbottomOffset,       0,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_OPPOSITE_FORM,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    input->connectedToWidget = source;

    const char *const *options = n->getInputValueOptions(i);
    int option_count = 0;
    if (options) {
	while (options[option_count]) 
	    option_count++;
    }
    input->valueTextPopup->createTextPopup(this->scrolledInputForm,(const char**)options, 
			option_count,
		    ConfigurationDialog::ActivateInputValueCB, 
		    ConfigurationDialog::ValueChangedInputValueCB, 
		    (void*)this);

    m = 0;
    XtSetArg (args[m], XmNtopAttachment,   XmATTACH_OPPOSITE_WIDGET); m++;
    XtSetArg (args[m], XmNtopWidget,       input->nameWidget); m++;
    XtSetArg (args[m], XmNtopOffset,       0); m++; 
    XtSetArg (args[m], XmNleftAttachment,  XmATTACH_FORM); m++;
    XtSetArg (args[m], XmNleftOffset,  560); m++;
    XtSetArg (args[m], XmNrightAttachment, XmATTACH_FORM); m++;
    if (prevTextPopup) {
	Dimension ph;
	XtVaGetValues (prevTextPopup, XmNheight, &ph, NULL);
	XtSetArg (args[m], XmNheight, ph); m++;
    }
    XtSetValues(input->valueTextPopup->getRootWidget(), args, m);

    const char *valueString;
    if (n->isInputDefaulting(i))
    {
	valueString = n->getInputDefaultValueString(i);
    }
    else
    {
	valueString = n->getInputSetValueString(i);
    }
    input->setInitialValue(valueString);
    input->initialValueIsDefault = n->isInputDefaulting(i);
    input->initialIsHidden = !n->isInputVisible(i);

    input->valueTextPopup->enableCallbacks();

    XtAddCallback(input->nameWidget, XmNvalueChangedCallback,
	(XtCallbackProc)ConfigurationDialog_ValueChangedInputNameCB, (XtPointer)this);
    XtAddCallback(input->hideWidget, XmNvalueChangedCallback,
	(XtCallbackProc)ConfigurationDialog_ValueChangedInputHideCB, (XtPointer)this);

    input->modified = 0;
    this->changeInput(i);

    Dimension width;
    XtVaGetValues(input->typeWidget, XmNwidth, &width, NULL);
    if (width > this->typeWidth)
	this->typeWidth = width;

    input->lineIsHidden = !n->isInputVisible(i);
    if (!input->lineIsHidden)
    {
	XtManageChild(input->nameWidget);
	XtManageChild(input->hideWidget);
	XtManageChild(input->typeWidget);
	XtManageChild(input->connectedToWidget);
	input->valueTextPopup->manage();
	for (j = i + 1; j <= this->inputList.getSize(); ++j)
	{
	    CDBInput *nextInput = (CDBInput*)this->inputList.getElement(j);
	    if (nextInput != NULL && nextInput->nameWidget != NULL &&
		!nextInput->lineIsHidden)
	    {
		XtVaSetValues(nextInput->nameWidget,
		    XmNtopWidget, input->nameWidget,
		    XmNtopAttachment, XmATTACH_WIDGET,
		    NULL);
		break;
	    }
	}
    }
    else
    {
	XtUnmanageChild(input->nameWidget);
	XtUnmanageChild(input->hideWidget);
	XtUnmanageChild(input->typeWidget);
	XtUnmanageChild(input->connectedToWidget);
	input->valueTextPopup->unmanage();
	XtSetSensitive(this->expand, True);
    }

    //
    // Adjust the size of the scrolled Window.
    //
    int numParam = this->inputList.getSize();
    int visInputCount = 0;
    for (i=1; i<=numParam; i++) {
	input = (CDBInput*)this->inputList.getElement(i);
	if (!input->nameWidget) continue;
	if (!XtIsManaged(input->nameWidget)) continue;
	visInputCount++;
    }
    if (visInputCount) {
	XtVaSetValues (this->scrolledInputForm, 
	    XmNheight, 5+(PIXELS_PER_LINE*visInputCount), 
	NULL);
	if (!XtIsManaged(XtParent(this->inputScroller)))
	    XtManageChild (XtParent(this->inputScroller));


        XSync (XtDisplay(this->scrolledInputForm), False);

        Dimension newHeight, oldHeight;
        int vis = visInputCount;
        if (vis > MAX_INITIAL_BUTTONS) vis = MAX_INITIAL_BUTTONS;
        if (vis <= 0) vis = visInputCount = 1;
        newHeight = 5+(vis*PIXELS_PER_LINE);
        XtVaGetValues (this->inputScroller, XmNheight, &oldHeight, NULL);
        if ((visInputCount <= MAX_INITIAL_BUTTONS) || (newHeight > oldHeight)) {
            XtVaSetValues (this->inputScroller,
                XmNheight, newHeight,
            NULL);
        } else {
            this->resizeCallback();
        }
    }
}
void ConfigurationDialog::newOutput(int i)
{
    Node *n = this->node;
    CDBOutput *output = (CDBOutput*)this->outputList.getElement(i);
    if (output == NULL)
    {
	output = new CDBOutput;
	this->outputList.insertElement(output, i);
    }

    if (!n->isOutputViewable(i))
	return;

    Widget prevNameWidget = NULL;
    int j;
    for (j = i - 1; j > 0; --j)
    {
	CDBOutput *prevOutput = (CDBOutput*)this->outputList.getElement(j);
	if (prevOutput != NULL && prevOutput->nameWidget != NULL)
	{
	    prevNameWidget = prevOutput->nameWidget;
	    break;
	}
    }
    if (prevNameWidget == NULL)
    {
	prevNameWidget = this->outputNameLabel;
    }

    Widget outputs = XtParent(prevNameWidget);

    Widget name = XtVaCreateManagedWidget(
	"Name Goes Here", 
	xmLabelWidgetClass,
	outputs,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       prevNameWidget,
	XmNtopOffset,       3,
	XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,      this->outputNameLabel,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,     this->outputNameLabel,
	XmNuserData,        this,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    output->nameWidget = name;

    Widget type = XtVaCreateManagedWidget(
	"Type Goes Here", 
	xmLabelWidgetClass,
	outputs,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       prevNameWidget,
	XmNtopOffset,       3,
	XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,      this->outputTypeLabel,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,     this->outputTypeLabel,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    output->typeWidget = type;

    Widget dest = XtVaCreateManagedWidget(
	"destination", 
	xmLabelWidgetClass,
	outputs,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       prevNameWidget,
	XmNtopOffset,       3,
	XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,      this->outputDestLabel,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,     this->outputDestLabel,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    output->connectedToWidget = dest;

    output->cachePulldown =  XmCreatePulldownMenu(outputs,
	"cachePulldown", NULL, 0);
    output->fullButton = XtVaCreateManagedWidget("cacheFull",
	xmPushButtonWidgetClass, output->cachePulldown, NULL);
    output->lastButton = XtVaCreateManagedWidget("cacheLast",
	xmPushButtonWidgetClass, output->cachePulldown, NULL);
    output->offButton = XtVaCreateManagedWidget("cacheOff",
	xmPushButtonWidgetClass, output->cachePulldown, NULL);
    XtAddCallback(output->fullButton, XmNactivateCallback, 
		    (XtCallbackProc)ConfigurationDialog_ActivateOutputCacheCB, 
		    (XtPointer)this);
    XtAddCallback(output->lastButton, XmNactivateCallback, 
		    (XtCallbackProc)ConfigurationDialog_ActivateOutputCacheCB, 
		    (XtPointer)this);
    XtAddCallback(output->offButton, XmNactivateCallback, 
		    (XtCallbackProc)ConfigurationDialog_ActivateOutputCacheCB, 
		    (XtPointer)this);
    
    output->cacheWidget = XtVaCreateManagedWidget(
	"cacheOptionMenu", xmRowColumnWidgetClass, outputs,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       prevNameWidget,
	XmNtopOffset,       3,
	XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,      this->outputCacheLabel,
	XmNleftOffset,      -10,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,     this->outputCacheLabel,
	XmNrowColumnType,   XmMENU_OPTION,
	XmNsubMenuId,       output->cachePulldown,
	NULL);

    this->changeOutput(i);

    Dimension width;
    XtVaGetValues(output->typeWidget, XmNwidth, &width, NULL);
    if (width > this->typeWidth)
	this->typeWidth = width;

    for (j = i + 1; j <= this->outputList.getSize(); ++j)
    {
	CDBOutput *nextOutput = (CDBOutput*)this->outputList.getElement(j);
	if (nextOutput != NULL && nextOutput->nameWidget != NULL)
	{
	    XtVaSetValues(nextOutput->nameWidget,
		XmNtopWidget, output->nameWidget,
		NULL);
	    XtVaSetValues(nextOutput->typeWidget,
		XmNtopWidget, output->nameWidget,
		NULL);
	    XtVaSetValues(nextOutput->connectedToWidget,
		XmNtopWidget, output->nameWidget,
		NULL);
	    XtVaSetValues(nextOutput->cacheWidget,
		XmNtopWidget, output->nameWidget,
		NULL);
	    break;
	}
    }

#if defined(ibm6000)
    XtVaSetValues (outputs, XmNheight, 0, NULL);
#endif
}

void ConfigurationDialog::deleteInput(int i)
{
    CDBInput *input = (CDBInput*)this->inputList.getElement(i);
    if (input == NULL)
	return;

    if (input->nameWidget != NULL)
    {
	input->valueTextPopup->disableCallbacks();
	XtRemoveCallback(input->nameWidget, XmNvalueChangedCallback,
	    (XtCallbackProc)ConfigurationDialog_ValueChangedInputNameCB, (XtPointer)this);
	XtDestroyWidget(input->nameWidget);
	XtDestroyWidget(input->typeWidget);
	XtDestroyWidget(input->connectedToWidget);
	XtDestroyWidget(input->valueTextPopup->getRootWidget());
	XtDestroyWidget(input->hideWidget);
    }

    CDBInput *prevVisible;
    int j;
    for (j = i-1; (prevVisible = (CDBInput*)this->inputList.getElement(j)); --j)
	if (!prevVisible->lineIsHidden)
	    break;
    Widget prevWidget;
    if (prevVisible)
	prevWidget = prevVisible->nameWidget;
    else
	prevWidget = NULL;

    CDBInput *nextVisible;
    for (j = i+1; (nextVisible = (CDBInput*)this->inputList.getElement(j)); ++j)
	if (!nextVisible->lineIsHidden)
	    break;
    if (nextVisible)
	if (prevWidget)
	    XtVaSetValues(nextVisible->nameWidget, XmNtopWidget, prevWidget, NULL);
	else
	    XtVaSetValues(nextVisible->nameWidget, XmNtopAttachment, XmATTACH_FORM, NULL);

    this->inputList.deleteElement(i);
    delete input;

    //
    // Adjust the size of the scrolled Window.
    //
    int numParam = this->inputList.getSize();
    int visInputCount = 0;
    for (i=1; i<=numParam; i++) {
	input = (CDBInput*)this->inputList.getElement(i);
	if (!input->nameWidget) continue;
	if (!XtIsManaged(input->nameWidget)) continue;
	visInputCount++;
    }
    if (visInputCount) {
	XtVaSetValues (this->scrolledInputForm, 
	    XmNheight, 5+(PIXELS_PER_LINE*visInputCount), NULL);
	XSync (XtDisplay(this->scrolledInputForm), False);

	Dimension newHeight, oldHeight;
	int vis = visInputCount;
	if (vis > MAX_INITIAL_BUTTONS) vis = MAX_INITIAL_BUTTONS;
	if (vis <= 0) vis = visInputCount = 1;
	newHeight = 5+(vis*PIXELS_PER_LINE);
	XtVaGetValues (this->inputScroller, XmNheight, &oldHeight, NULL);
	if ((visInputCount <= MAX_INITIAL_BUTTONS) || (newHeight > oldHeight)) {
	    XtVaSetValues (this->inputScroller, 
		XmNheight, newHeight,
	    NULL);
	} else {
	    this->resizeCallback();
	}
    }
}
void ConfigurationDialog::deleteOutput(int i)
{
    CDBOutput *output = (CDBOutput*)this->outputList.getElement(i);
    if (output == NULL)
	return;

    Widget prevNameWidget=NULL;
    int j;
    for (j = i - 1; j > 0; --j)
    {
	CDBOutput *prevOutput = (CDBOutput*)this->outputList.getElement(j);
	if (prevOutput != NULL && prevOutput->nameWidget != NULL)
	{
	    prevNameWidget = prevOutput->nameWidget;
	    break;
	}
    }
    if (prevNameWidget == NULL)
    {
	prevNameWidget = this->outputNameLabel;
    }


    for (j = i + 1; j <= this->outputList.getSize(); ++j)
    {
	CDBOutput *nextOutput = (CDBOutput*)this->outputList.getElement(j);
	if (nextOutput != NULL && nextOutput->nameWidget != NULL)
	{
	    XtVaSetValues(nextOutput->nameWidget,
		XmNtopWidget, prevNameWidget,
		NULL);
	    XtVaSetValues(nextOutput->typeWidget,
		XmNtopWidget, prevNameWidget,
		NULL);
	    XtVaSetValues(nextOutput->connectedToWidget,
		XmNtopWidget, prevNameWidget,
		NULL);
	    XtVaSetValues(nextOutput->cacheWidget,
		XmNtopWidget, prevNameWidget,
		NULL);
	    break;
	}
    }
    if (output->nameWidget != NULL)
    {
	XtDestroyWidget(output->nameWidget);
	XtDestroyWidget(output->typeWidget);
	XtDestroyWidget(output->connectedToWidget);
	XtDestroyWidget(output->cacheWidget);
	XtDestroyWidget(output->cachePulldown);
    }
    this->outputList.deleteElement(i);
    delete output;
}

boolean ConfigurationDialog::widgetChanged(int i, boolean send)
{
    CDBInput *input = (CDBInput*)this->inputList.getElement(i);
    const char *s = input->valueTextPopup->getText();
    boolean retval = TRUE;
    char pname[128];

    if (*s == '\0')
    {
	this->node->useDefaultInputValue(i, send);
	input->valueTextPopup->setText(
			this->node->getInputDefaultValueString(i));
    }
    else if (EqualString(s, this->node->getInputDefaultValueString(i)))
    {
	if ((s) && (s[0]))
	    this->node->setInputValue(i, s, DXType::UndefinedType, send);
	else
	    this->node->setInputValue(i, "NULL", DXType::UndefinedType, send);
    }
    else if (this->node->setInputValue(i, s, DXType::UndefinedType, send) ==
	     DXType::UndefinedType)
    {
	ErrorMessage("String `%s' is not a valid value for %s parameter '%s'",
	    s, this->node->getNameString(), 
		this->node->getInputNameString(i,pname));
	retval = FALSE;
    }
    XtFree((char *)s);

    input->valueChanged = !retval;
    return retval;
}

void ConfigurationDialog::changeLabel()
{
    const char *l = this->node->getLabelString();
    XmTextSetString(this->notation, (char *)l);
}

void ConfigurationDialog::remanageInputs(boolean force)
{
    int count = this->node->getInputCount();
    int visible = 0;	// Are any input lines visible?
    int invisible = 0;	// Are any visible input lines invisible inputs?
    int noline    = 0;	// Are any invisible intputs' lines invisible?
    int i;
    Widget lastName = NULL;
    Widget *managed = new Widget[(count + 1) * 7];
    int     nmanaged = 0;
    Widget *unmanaged = new Widget[(count + 1) * 7];
    int     nunmanaged = 0;

    //
    // Make sure that all titles are managed.
    managed[nmanaged++] = this->inputTitle;
    managed[nmanaged++] = this->inputNameLabel;
    managed[nmanaged++] = this->inputHideLabel;
    managed[nmanaged++] = this->inputTypeLabel;
    managed[nmanaged++] = this->inputSourceLabel;
    managed[nmanaged++] = this->inputValueLabel;
    XtManageChildren(managed, nmanaged); nmanaged = 0;
    managed[nmanaged++] = XtParent(this->inputScroller);
    XtManageChildren(managed, nmanaged); nmanaged = 0;
    for (i = 1; i <= count; ++i)
    {
	if (!this->node->isInputViewable(i))
	    continue;
	CDBInput *input = (CDBInput*)this->inputList.getElement(i);
	if (force || this->node->isInputVisible(i))
	{
	    ++visible;
	    if (!this->node->isInputVisible(i))
		invisible++;
	    ASSERT(input->nameWidget);
	    if (!lastName)
		XtVaSetValues(input->nameWidget, 
		    XmNtopAttachment, XmATTACH_FORM, 
		NULL);
	    else
		XtVaSetValues(input->nameWidget, 
		    XmNtopAttachment, XmATTACH_WIDGET, 
		    XmNtopWidget, lastName, 
		NULL);
	    managed[nmanaged++] = input->nameWidget;
	    managed[nmanaged++] = input->hideWidget;
	    managed[nmanaged++] = input->typeWidget;
	    managed[nmanaged++] = input->connectedToWidget;
	    input->valueTextPopup->manage();
	    lastName = input->nameWidget;
	    input->lineIsHidden=FALSE;
	}
	else
	{
	    ++noline;
	    unmanaged[nunmanaged++] = input->nameWidget;
	    unmanaged[nunmanaged++] = input->hideWidget;
	    unmanaged[nunmanaged++] = input->typeWidget;
	    unmanaged[nunmanaged++] = input->connectedToWidget;
	    input->valueTextPopup->unmanage();
	    input->lineIsHidden=TRUE;
	}
    }
    XtManageChildren(managed, nmanaged); nmanaged = 0;
    XtUnmanageChildren(unmanaged, nunmanaged); nunmanaged = 0;

    if (count > 0 && visible == 0)
    {
	unmanaged[nunmanaged++] = this->inputTitle;
	unmanaged[nunmanaged++] = this->inputNameLabel;
	unmanaged[nunmanaged++] = this->inputHideLabel;
	unmanaged[nunmanaged++] = this->inputTypeLabel;
	unmanaged[nunmanaged++] = this->inputSourceLabel;
	unmanaged[nunmanaged++] = this->inputValueLabel;
	XtUnmanageChildren(unmanaged, nunmanaged); nunmanaged = 0;
	unmanaged[nunmanaged++] = XtParent(this->inputScroller);
	XtUnmanageChildren(unmanaged, nunmanaged); nunmanaged = 0;
    }

    //
    // too-small-dialog workaround for displaying on Sun4.
    //
    if (visible == 0 AND this->node->getOutputCount() == 0)
        XtVaSetValues(this->getRootWidget(), XmNwidth, 770, NULL);

    delete managed;
    delete unmanaged;

    //
    // If we just expanded, gray out expand.
    // if none of the visible lines are invisible parameters, gray out collpase.
    //
    XtSetSensitive(this->expand, (Boolean)noline != 0);
    XtSetSensitive(this->collapse, (Boolean)(invisible != 0));

    //
    // Set the height of the scrolled window.
    //
    XtVaSetValues (this->scrolledInputForm, XmNheight, 5+(PIXELS_PER_LINE*visible), NULL);

    Dimension oldHeight, newHeight;
    int vis = visible > MAX_INITIAL_BUTTONS ? 
		MAX_INITIAL_BUTTONS : visible < 1 ? 1 : visible;
	
    newHeight = 5+(vis*PIXELS_PER_LINE);
    XtVaGetValues (this->inputScroller, XmNheight, &oldHeight, NULL);

    Dimension ifh; 
    int d = ((int)newHeight) - oldHeight;

    XtVaGetValues(this->inputForm, XmNheight, &ifh, NULL);
    XtVaSetValues(this->inputForm, XmNheight, ifh+d, NULL);
    XtVaSetValues(this->inputScroller, XmNheight, newHeight, NULL);

    Widget cw;
    Dimension fWidth, isWidth, cwWidth;

    XtVaGetValues(this->inputScroller,
	    XmNwidth, &isWidth,
	    XmNclipWindow, &cw,
	    NULL);

    XtVaGetValues(cw, XmNwidth, &cwWidth, NULL);
    XtVaGetValues(this->scrolledInputForm, XmNwidth, &fWidth, NULL);

    d = (fWidth - cwWidth) + 10;
    if (d > 0)
    {
	Dimension iWidth;
	XtVaGetValues(this->inputForm, XmNwidth, &iWidth, NULL);
	XtVaSetValues(this->inputForm, XmNwidth, iWidth+d, NULL);
	XtVaGetValues(this->inputForm, XmNwidth, &iWidth, NULL);
	XtVaSetValues(cw, XmNwidth, cwWidth+d, NULL);
	XtVaGetValues(cw, XmNwidth, &cwWidth, NULL);
	XtVaSetValues(this->inputScroller, XmNwidth, isWidth+d, NULL);
	XtVaGetValues(this->inputScroller, XmNwidth, &isWidth, NULL);
    }
}

void
ConfigurationDialog::unmanage()
{
    if (this->descriptionDialog)
	this->descriptionDialog->unmanage();
    this->Dialog::unmanage();
}
void
ConfigurationDialog::manage()
{
    this->restoreCallback(this);
    this->Dialog::manage();
}
//
// Install the default resources for this class.
//
void ConfigurationDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,ConfigurationDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
const char *ConfigurationDialog::getHelpSyntaxString()
{
    if (ConfigurationDialog::HelpText == 0) {
	char *nosup = "No Syntax Help Available";
	const char *dxroot = theDXApplication->getUIRoot();
	if (!dxroot) {
	    ConfigurationDialog::HelpText = new char[1+strlen(nosup)];
	    strcpy (ConfigurationDialog::HelpText, nosup);
	    return ConfigurationDialog::HelpText;
	}

	char supfile[1024];
	sprintf(supfile,"%s/ui/syntax.txt",dxroot);
	FILE *fp;
	int helpsize;

	struct STATSTRUCT buf;

	if (STATFUNC(supfile, &buf) != 0) {
	    ConfigurationDialog::HelpText = new char[1+strlen(nosup)];
	    strcpy (ConfigurationDialog::HelpText, nosup);
	    return ConfigurationDialog::HelpText;
	}
	helpsize = buf.st_size;

	if (!(fp = fopen(supfile,"r")))
	    return nosup;

	char *helpstr = new char[helpsize + 30];
	if (!helpstr) return nosup;
	fread(helpstr,1,helpsize,fp);
	fclose(fp);
	helpstr[helpsize] = '\0'; // Null ternimate string
	ConfigurationDialog::HelpText = helpstr;
    }
    return ConfigurationDialog::HelpText;
}

