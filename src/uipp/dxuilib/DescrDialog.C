/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "Application.h"
#include "DescrDialog.h"
#include "Node.h"
#include "NodeDefinition.h"
#include "ParameterDefinition.h"

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>

boolean DescrDialog::ClassInitialized = FALSE;
String  DescrDialog::DefaultResources[] =
{
    "*dialogTitle:                Description of %s",
    "*descrLabel.labelString:     Module Description:",
    "*descrLabel.foreground:      SteelBlue",
    "*closeButton.labelString:    Close",
    "*closeButton.width:          70",
    "*inputTitle.labelString:     Inputs:",
    "*inputTitle.foreground :     SteelBlue",
    "*inputNameTitle.labelString: Name",
    "*inputNameTitle.width:       120",
    "*inputNameTitle.foreground:   SteelBlue",
    "*inputAttributeTitle.labelString:     Attributes",
    "*inputAttributeTitle.width:           100",
    "*inputAttributeTitle.foreground:      SteelBlue",
    "*inputDescriptionTitle.labelString:   Description",
    "*inputDescriptionTitle.foreground:    SteelBlue",
    "*outputTitle.labelString:      Outputs:",
    "*outputTitle.foreground:       SteelBlue",
    "*outputNameTitle.labelString:  Name",
    "*outputNameTitle.width:        120",
    "*outputNameTitle.foreground:   SteelBlue",
    "*outputDescriptionTitle.labelString:   Description",
    "*outputDescriptionTitle.foreground:    SteelBlue",
    NULL
};

Widget DescrDialog::createDialog(Widget parent)
{
    int i;
    Arg a[1];
    XtSetArg(a[0], XmNresizePolicy, XmRESIZE_GROW);
//    Widget dialog = XmCreateFormDialog(parent, this->name, a, 1);
    Widget dialog = this->CreateMainForm(parent, this->name, a, 1);

    NodeDefinition *def = this->node->getDefinition();

    XmString titleString;
    char *title;
    XtVaGetValues(dialog, XmNdialogTitle, &titleString, NULL);
    XmStringGetLtoR(titleString, XmSTRING_DEFAULT_CHARSET, &title);
    char buffer[100];
    sprintf(buffer, title, this->node->getNameString());
    XtFree(title);
    titleString = XmStringCreate(buffer, XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues(dialog, XmNdialogTitle, titleString, NULL);
    XmStringFree(titleString);

    Widget descrLabel = XtVaCreateManagedWidget(
	"descrLabel",
	xmLabelWidgetClass,
	dialog,
        XmNtopAttachment    , XmATTACH_FORM,
        XmNtopOffset        , 10,
        XmNleftAttachment   , XmATTACH_FORM,
        XmNleftOffset       , 5,
        XmNalignment        , XmALIGNMENT_BEGINNING,
	NULL);

    const char *desc = this->node->getDescription();
    if (desc)
    {
	XmString descriptionString = XmStringCreate((char *)desc,
				       XmSTRING_DEFAULT_CHARSET);
	Widget description = XtVaCreateManagedWidget(
	    "description",
	    xmLabelWidgetClass,
	    dialog,
	    XmNtopAttachment    , XmATTACH_FORM,
	    XmNtopOffset        , 10,
	    XmNleftAttachment   , XmATTACH_WIDGET,
	    XmNleftWidget       , descrLabel,
	    XmNleftOffset       , 10,
	    XmNrightAttachment  , XmATTACH_FORM,
	    XmNrightOffset      , 10,
	    XmNalignment        , XmALIGNMENT_BEGINNING,
	    XmNlabelString      , descriptionString,
	    NULL);
	XmStringFree(descriptionString);
    }

    Widget buttonTop = descrLabel;

    int viewableCount = 0;
    int inputCount = def->getInputCount();
    for (i = 1; i <= inputCount; ++i)
	if (def->getInputDefinition(i)->isViewable())
	    ++viewableCount;
    if (viewableCount > 0)
    {
	Widget inputForm = XtVaCreateManagedWidget(
	    "inputForm",
	    xmFormWidgetClass,
	    dialog,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , descrLabel,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNleftOffset      , 5,
	    XmNrightAttachment , XmATTACH_FORM,
	    XmNrightOffset     , 5,
	    NULL);
	buttonTop = inputForm;

	Widget inputTitle = XtVaCreateManagedWidget(
	    "inputTitle",
	    xmLabelWidgetClass,
	    inputForm,
	    XmNtopAttachment   , XmATTACH_FORM,
	    XmNtopOffset       , 10,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);

	Widget inputNameTitle = XtVaCreateManagedWidget(
	    "inputNameTitle",
	    xmLabelWidgetClass,
	    inputForm,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , inputTitle,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);

	Widget inputAttributeTitle = XtVaCreateManagedWidget(
	    "inputAttributeTitle",
	    xmLabelWidgetClass,
	    inputForm,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , inputTitle,
	    XmNrightAttachment , XmATTACH_FORM,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);

	Widget inputDescriptionTitle = XtVaCreateManagedWidget(
	    "inputDescriptionTitle",
	    xmLabelWidgetClass,
	    inputForm,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , inputTitle,
	    XmNleftAttachment  , XmATTACH_WIDGET,
	    XmNleftWidget      , inputNameTitle,
	    XmNrightAttachment , XmATTACH_WIDGET,
	    XmNrightWidget     , inputAttributeTitle,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);
	Widget nameTop = inputNameTitle;
	Widget attrTop = inputAttributeTitle;
	Widget descTop = inputDescriptionTitle;
	for (i = 1; i <= inputCount; ++i)
	{
	    ParameterDefinition *pd = def->getInputDefinition(i);
	    if (pd->isViewable())
	    {
		Widget name = XtVaCreateManagedWidget(
		    pd->getNameString(),
		    xmLabelWidgetClass,
		    inputForm,
		    XmNtopAttachment,   XmATTACH_WIDGET,
		    XmNtopWidget,       nameTop,
		    XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
		    XmNleftWidget,      inputNameTitle,
		    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNrightWidget,     inputNameTitle,
		    XmNalignment,       XmALIGNMENT_BEGINNING,
		    NULL);

		Widget attr = XtVaCreateManagedWidget(
		    (pd->isRequired()? "Required":
			(i > def->getInputCount() - def->getInputRepeatCount()?
			    "Repeatable": "-")),
		    xmLabelWidgetClass,
		    inputForm,
		    XmNtopAttachment,   XmATTACH_WIDGET,
		    XmNtopWidget,       attrTop,
		    XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
		    XmNleftWidget,      inputAttributeTitle,
		    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNrightWidget,     inputAttributeTitle,
		    XmNalignment,       XmALIGNMENT_BEGINNING,
		    NULL);

		Widget desc = XtVaCreateManagedWidget(
		    pd->getDescription(),
		    xmLabelWidgetClass,
		    inputForm,
		    XmNtopAttachment,   XmATTACH_WIDGET,
		    XmNtopWidget,       descTop,
		    XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
		    XmNleftWidget,      inputDescriptionTitle,
		    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNrightWidget,     inputDescriptionTitle,
		    XmNalignment,       XmALIGNMENT_BEGINNING,
		    NULL);
		Dimension descWidth;
		Dimension titleWidth;
		XtVaGetValues(desc, XmNwidth, &descWidth, NULL);
		XtVaGetValues(inputDescriptionTitle, XmNwidth, &titleWidth,
			      NULL);
		if (titleWidth < descWidth)
		    XtVaSetValues(inputDescriptionTitle, XmNwidth, descWidth,
				  NULL);
		nameTop = name;
		descTop = desc;
		attrTop = attr;
	    }
	}
    }

    viewableCount = 0;
    int outputCount = def->getOutputCount();
    for (i = 1; i <= outputCount; ++i)
	if (def->getOutputDefinition(i)->isViewable())
	    ++viewableCount;
    if (viewableCount > 0)
    {
	Widget outputForm = XtVaCreateManagedWidget(
	    "outputForm",
	    xmFormWidgetClass,
	    dialog,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , buttonTop,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNleftOffset      , 5,
	    XmNrightAttachment , XmATTACH_FORM,
	    XmNrightOffset     , 5,
	    NULL);
	buttonTop = outputForm;

	Widget outputTitle = XtVaCreateManagedWidget(
	    "outputTitle",
	    xmLabelWidgetClass,
	    outputForm,
	    XmNtopAttachment   , XmATTACH_FORM,
	    XmNtopOffset       , 10,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);

	Widget outputNameTitle = XtVaCreateManagedWidget(
	    "outputNameTitle",
	    xmLabelWidgetClass,
	    outputForm,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , outputTitle,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);

	Widget outputDescriptionTitle = XtVaCreateManagedWidget(
	    "outputDescriptionTitle",
	    xmLabelWidgetClass,
	    outputForm,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , outputTitle,
	    XmNleftAttachment  , XmATTACH_WIDGET,
	    XmNleftWidget      , outputNameTitle,
	    XmNrightAttachment , XmATTACH_FORM,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);
	Widget nameTop = outputNameTitle;
	Widget descTop = outputDescriptionTitle;
	for (i = 1; i <= outputCount; ++i)
	{
	    ParameterDefinition *pd = def->getOutputDefinition(i);
	    if (pd->isViewable())
	    {
		Widget name = XtVaCreateManagedWidget(
		    pd->getNameString(),
		    xmLabelWidgetClass,
		    outputForm,
		    XmNtopAttachment,   XmATTACH_WIDGET,
		    XmNtopWidget,       nameTop,
		    XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
		    XmNleftWidget,      outputNameTitle,
		    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNrightWidget,     outputNameTitle,
		    XmNalignment,       XmALIGNMENT_BEGINNING,
		    NULL);


		Widget desc = XtVaCreateManagedWidget(
		    pd->getDescription(),
		    xmLabelWidgetClass,
		    outputForm,
		    XmNtopAttachment,   XmATTACH_WIDGET,
		    XmNtopWidget,       descTop,
		    XmNleftAttachment,  XmATTACH_OPPOSITE_WIDGET,
		    XmNleftWidget,      outputDescriptionTitle,
		    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNrightWidget,     outputDescriptionTitle,
		    XmNalignment,       XmALIGNMENT_BEGINNING,
		    NULL);

		Dimension descWidth;
		Dimension titleWidth;
		XtVaGetValues(desc, XmNwidth, &descWidth, NULL);
		XtVaGetValues(outputDescriptionTitle, XmNwidth, &titleWidth,
			      NULL);
		if (titleWidth < descWidth)
		    XtVaSetValues(outputDescriptionTitle, XmNwidth, descWidth,
				  NULL);

		nameTop = name;
		descTop = desc;
	    }
	}
    }

    Widget sep = XtVaCreateManagedWidget(
	"separator",
	xmSeparatorWidgetClass,
	dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , buttonTop,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 2,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 2,
	NULL);

    this->cancel = XtVaCreateManagedWidget(
        "closeButton",
        xmPushButtonWidgetClass,
        dialog,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , sep,
	XmNtopOffset       , 10,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , 10,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNbottomOffset    , 10,
	NULL);

    return dialog;
}

DescrDialog::DescrDialog(Widget parent, Node *n) :
    Dialog("descrDialog", parent)
{

    if (NOT DescrDialog::ClassInitialized)
    {
        DescrDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
    this->node = n;
}

DescrDialog::~DescrDialog()
{
}
//
// Install the default resources for this class.
//
void DescrDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, DescrDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
