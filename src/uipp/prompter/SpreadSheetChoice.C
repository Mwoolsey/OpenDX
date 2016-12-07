/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "SpreadSheetChoice.h"
#include "GARApplication.h"
#include "NoUndoChoiceCommand.h"
#include "ToggleButtonInterface.h"
#include "GARChooserWindow.h"
#include "DXStrings.h"
#include "XmUtility.h"
#include "DXValue.h"
#include "DXType.h"
#include "WarningDialogManager.h"
#include "ButtonInterface.h"


#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>

#include "../widgets/Number.h"


boolean SpreadSheetChoice::ClassInitialized = FALSE;

String SpreadSheetChoice::DefaultResources[] = {
    "*conform*XmToggleButton.shadowThickness:	0",
    "*conform*tableOption.labelString:		Table",
    "*conform*matrixOption.labelString:		Matrix",
    "*conform.interpret.labelString:		Data organization:",
    "*conform.restrictOption.labelString:	Import only these columns:",
    "*conform.specifyRowsOption.labelString:	Import only these rows:",
    "*conform.useDelimiterOption.labelString:	Column delimiter:",
    "*conform.accelerators: #augment\n"
    "<Key>Return:    BulletinBoardReturn()",

    NUL(char*),
};


SpreadSheetChoice::SpreadSheetChoice (GARChooserWindow *gcw, Symbol sym) :
    NonimportableChoice ("spreadSheet", TRUE, TRUE, TRUE, FALSE, gcw, sym) 
{
    this->tableOption = NUL(ToggleButtonInterface*);
    this->matrixOption = NUL(ToggleButtonInterface*);
#if USES_DXLINK
    this->restrictOption = NUL(ToggleButtonInterface*);
    this->specifyRowsOption = NUL(ToggleButtonInterface*);
#endif
    this->useDelimiterOption = NUL(ToggleButtonInterface*);

#if USES_DXLINK
    this->row_numbers = NUL(Widget);
    this->starting_row = NUL(Widget);
    this->ending_row = NUL(Widget);
    this->delta_row = NUL(Widget);
#endif
    this->delimiter = NUL(Widget);

    this->restrictCmd =
	new NoUndoChoiceCommand ("restrict", this->getCommandScope(),
	    TRUE, this, NoUndoChoiceCommand::RestrictNames);

    this->useRowsCmd =
	new NoUndoChoiceCommand ("specifyRows", this->getCommandScope(),
	    TRUE, this, NoUndoChoiceCommand::SpecifyRows);

    this->delimiterCmd =
	new NoUndoChoiceCommand ("useDelimiter", this->getCommandScope(),
	    TRUE, this, NoUndoChoiceCommand::UseDelimiter);

    this->names_value = new DXValue;
    this->delimiter_value = new DXValue;
}

SpreadSheetChoice::~SpreadSheetChoice()
{
    if (this->restrictCmd) delete this->restrictCmd;
    if (this->useRowsCmd) delete this->useRowsCmd;
    if (this->delimiterCmd) delete this->delimiterCmd;

    delete this->names_value;
    delete this->delimiter_value;
}


void SpreadSheetChoice::initialize() 
{
    if (SpreadSheetChoice::ClassInitialized) return ;
    SpreadSheetChoice::ClassInitialized = TRUE;

    this->setDefaultResources
	(theApplication->getRootWidget(), TypeChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), NonimportableChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), SpreadSheetChoice::DefaultResources);
}


Widget SpreadSheetChoice::createBody (Widget parent, Widget top)
{
    Widget conform = XtVaCreateManagedWidget ("conform",
	xmFormWidgetClass,	parent,
	XmNtopAttachment,	(top?XmATTACH_WIDGET:XmATTACH_FORM),
	XmNtopWidget,		top,
	XmNtopOffset,		(top?0:2),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
    NULL);

    //
    // Table/Matrix
    //
    Widget label = XtVaCreateManagedWidget ("interpret",
	xmLabelWidgetClass,	conform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		20,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		8,
    NULL);

    Widget radioBox = XmCreateRadioBox (conform, "ssheetRadio", NULL, 0);
    XtManageChild (radioBox);
    this->tableOption = 
	new ToggleButtonInterface (radioBox, "tableOption", this->gcw->getNullCmd(),
	    TRUE, "Each column will be a separate component of a field with 1-d positions.");
    this->matrixOption = 
	new ToggleButtonInterface (radioBox, "matrixOption", this->gcw->getNullCmd(),
	    FALSE, "Import as a row x column (2-d) grid with a single data component.");
    XtVaSetValues (radioBox,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		label,
	XmNleftOffset,		8,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		4,
	XmNorientation,		XmHORIZONTAL,
    NULL);

#if USES_DXLINK
    //
    // Column names to import
    //
    this->restrictOption =
	new ToggleButtonInterface (conform, "restrictOption", this->restrictCmd,
	    FALSE, "Each field name must be quoted.");
    XtVaSetValues (this->restrictOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		20,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		radioBox,
	XmNtopOffset,		20,
    NULL);
    this->restrict_names = XtVaCreateManagedWidget ("restrictNames",
	xmTextFieldWidgetClass,	conform,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		radioBox,
	XmNtopOffset,		18,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->restrictOption->getRootWidget(),
	XmNleftOffset,		6,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		8,
    NULL);
    this->restrictCmd->execute();
    XtAddCallback (this->restrict_names, XmNactivateCallback, 
	(XtCallbackProc)SpreadSheetChoice_ParseCB, (XtPointer)this);


    //
    // Row numbers to import
    //
    this->specifyRowsOption =
	new ToggleButtonInterface (conform, "specifyRowsOption", this->useRowsCmd,
	    FALSE, "Pushed in means you must specify start, end, and delta values");
    XtVaSetValues (this->specifyRowsOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		20,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->restrictOption->getRootWidget(),
	XmNtopOffset,		30,
    NULL);
    Widget row_numbers = XtVaCreateManagedWidget ("rowNumbers",
	xmRowColumnWidgetClass,	conform,
	XmNorientation,		XmHORIZONTAL,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->restrict_names,
	XmNleftOffset,		0,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->restrictOption->getRootWidget(),
	XmNtopOffset,		28,
	XmNspacing,		30,
    NULL);
    this->starting_row = XtVaCreateManagedWidget ("startingRow",
	xmNumberWidgetClass,	row_numbers,
	XmNcharPlaces,		4,
	XmNiMinimum,		1,
	XmNiMaximum,		9999,
	XmNeditable,		True,
	XmNiValue,		1,
    NULL);
    XtAddCallback (this->starting_row, XmNactivateCallback, (XtCallbackProc)
	SpreadSheetChoice_NumberCB, (XtPointer)this);
    this->ending_row = XtVaCreateManagedWidget ("endingRow",
	xmNumberWidgetClass,	row_numbers,
	XmNcharPlaces,		4,
	XmNiMinimum,		1,
	XmNiMaximum,		9999,
	XmNeditable,		True,
	XmNiValue,		2,
    NULL);
    XtAddCallback (this->ending_row, XmNactivateCallback, (XtCallbackProc)
	SpreadSheetChoice_NumberCB, (XtPointer)this);
    this->delta_row = XtVaCreateManagedWidget ("deltaRow",
	xmNumberWidgetClass,	row_numbers,
	XmNcharPlaces,		4,
	XmNiMinimum,		1,
	XmNiMaximum,		9999,
	XmNeditable,		True,
	XmNiValue,		1,
    NULL);
    XtAddCallback (this->delta_row, XmNactivateCallback, (XtCallbackProc)
	SpreadSheetChoice_NumberCB, (XtPointer)this);
    this->useRowsCmd->execute();

    Widget start_lab = XtVaCreateManagedWidget ("start:",
	xmLabelWidgetClass,	conform,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		row_numbers,
	XmNleftOffset,		0,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	row_numbers,
	XmNbottomOffset,	-4,
    NULL);

    Widget delta_lab = XtVaCreateManagedWidget ("delta:",
	xmLabelWidgetClass,	conform,
	XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		row_numbers,
	XmNrightOffset,		6,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	row_numbers,
	XmNbottomOffset,	-4,
    NULL);

    XtVaCreateManagedWidget ("end:",
	xmLabelWidgetClass,	conform,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		start_lab,
	XmNleftOffset,		0,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		delta_lab,
	XmNrightOffset,		0,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	row_numbers,
	XmNbottomOffset,	-4,
    NULL);
#endif

    //
    // Column delimiter
    //
    this->useDelimiterOption =
	new ToggleButtonInterface (conform, "useDelimiterOption", this->delimiterCmd,
	    FALSE, "The character(s) separating columns of data");
    XtVaSetValues (this->useDelimiterOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		20,
	XmNtopAttachment,	XmATTACH_WIDGET,
#if USES_DXLINK
	XmNtopWidget,		this->specifyRowsOption->getRootWidget(),
#else
	XmNtopWidget,		radioBox,
#endif
	XmNtopOffset,		26,
    NULL);
    this->delimiter = XtVaCreateManagedWidget ("delimiter",
	xmTextFieldWidgetClass,	conform,
	XmNtopAttachment,	XmATTACH_WIDGET,
#if USES_DXLINK
	XmNtopWidget,		this->specifyRowsOption->getRootWidget(),
#else
	XmNtopWidget,		radioBox,
#endif
	XmNtopOffset,		18,
#if USES_DXLINK
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->restrict_names,
	XmNleftOffset,		0,
#else
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->useDelimiterOption->getRootWidget(),
	XmNleftOffset,		6,
#endif
	XmNcolumns,		10,
    NULL);
    this->delimiterCmd->execute();
    XtAddCallback (this->delimiter, XmNactivateCallback, 
	(XtCallbackProc)SpreadSheetChoice_ParseCB, (XtPointer)this);



    return conform;
}




boolean
SpreadSheetChoice::restrictNamesCB()
{
#if USES_DXLINK
    if (!this->restrictOption->getState()) {
	XtVaSetValues (this->restrict_names, 
	    RES_CONVERT(XmNforeground, "gray30"),
	    XmNeditable,  False,
	    XmNvalue,	  "(Use All Columns)",
	NULL);
    } else {
	char *cp = DuplicateString(this->names_value->getValueString());
	ASSERT(cp);
	if (EqualString(cp, "NULL")) cp = "";
	XtVaSetValues (this->restrict_names, 
	    RES_CONVERT(XmNforeground, "black"),
	    XmNeditable,  True,
	    XmNvalue, cp,
	NULL);
    }
#endif
    return TRUE;
}
boolean
SpreadSheetChoice::useRowsCB()
{
#if USES_DXLINK
    if (!this->specifyRowsOption->getState()) {
	XtSetSensitive (this->starting_row, False);
	XtSetSensitive (this->ending_row, False);
	XtSetSensitive (this->delta_row, False);
    } else {
	XtSetSensitive (this->starting_row, True);
	XtSetSensitive (this->ending_row, True);
	XtSetSensitive (this->delta_row, True);
    }
#endif
    return TRUE;
}

boolean
SpreadSheetChoice::useDelimiterCB()
{
    if (!this->useDelimiterOption->getState()) {
	XtVaSetValues (this->delimiter, 
	    RES_CONVERT(XmNforeground, "gray30"),
	    XmNeditable,  False,
	    XmNvalue,	  "\" \" ",
	NULL);
    } else {
	char *cp = DuplicateString(this->delimiter_value->getValueString());
	ASSERT(cp);
	if (EqualString(cp, "NULL")) cp = "";
	XtVaSetValues (this->delimiter,
	    RES_CONVERT(XmNforeground, "black"),
	    XmNeditable,  True,
	    XmNvalue, cp,
	NULL);
    }
    return TRUE;
}




boolean SpreadSheetChoice::verify(const char *)
{
    if (!this->connect(TypeChoice::ImageMode)) return FALSE;
    DXLConnection* conn = this->gcw->getConnection();

    char net_file[512];
    const char *net_dir = theGARApplication->getResourcesNetDir();
    sprintf (net_file, "%s/ui/testssheet.net", net_dir);
    DXLLoadVisualProgram (conn, net_file);

    if (this->tableOption->getState()) {
	DXLSetValue (conn, "format", "1-d");
    } else {
	DXLSetValue (conn, "format", "2-d");
    }

    const char* dfname = this->gcw->getDataFilename();
    DXLSetString (conn, "filename", dfname);

    if (this->useDelimiterOption->getState()) {
	if (!this->parseValue (this->delimiter)) 
	    return FALSE ;
	const char *cp = this->delimiter_value->getValueString();
	if (cp) DXLSetValue (conn, "delimiter", cp);
    }

#if USES_DXLINK
    if (this->restrictOption->getState()) {
	if (!this->parseValue(this->restrict_names))
	    return FALSE ;
	const char *cp = this->names_value->getValueString();
	if (cp) DXLSetValue (conn, "variable", cp);
    }

    if (this->specifyRowsOption->getState()) {
	int start, end, delta;

	XtVaGetValues (this->starting_row, XmNiValue, &start, NULL);
	XtVaGetValues (this->ending_row, XmNiValue, &end, NULL);
	XtVaGetValues (this->delta_row, XmNiValue, &delta, NULL);

	char tbuf[16];
	sprintf (tbuf, "%d", start);
	DXLSetValue (conn, "record_start", tbuf);
	sprintf (tbuf, "%d", end);
	DXLSetValue (conn, "record_end", tbuf);
	sprintf (tbuf, "%d", delta);
	DXLSetValue (conn, "record_delta", tbuf);
    }
#endif

    DXLExecuteOnce (conn);

    return TRUE;
}


extern "C" 
void SpreadSheetChoice_ParseCB (Widget w, XtPointer cData, XtPointer)
{
    //
    // These 2 lines are protection because the text widgets aren't
    // UIComponents and so aren't subject to a deactivate();
    Boolean edit;
    XtVaGetValues (w, XmNeditable, &edit, NULL);
    if (!edit) return ;
    SpreadSheetChoice *ssc = (SpreadSheetChoice*)cData;
    ASSERT(ssc);
    ssc->parseValue(w);
}

boolean SpreadSheetChoice::parseValue(Widget w)
{
    ASSERT(XtClass(w) == xmTextFieldWidgetClass);

    char *cp = XmTextFieldGetString(w);
    if (w == this->delimiter) {
	if (!this->delimiter_value->setValue (cp, DXType::StringType)) {
	    char *new_value = DXValue::CoerceValue (cp, DXType::StringType);
	    XtFree(cp);
	    if (!new_value) {
		WarningMessage ("Invalid string entered for column delimiter.");
		return FALSE;
	    }

	    this->delimiter_value->setValue (new_value, DXType::StringType);
	    if (EqualString(new_value, "NULL")) {
		XmTextFieldSetString (w, "");
	    } else {
		XmTextFieldSetString (w, new_value);
	    }
	    delete new_value;
	} 

#if USES_DXLINK
    } else if (w == this->restrict_names) {
	if (!this->names_value->setValue (cp, DXType::StringListType)) {
	    char *new_value = DXValue::CoerceValue (cp, DXType::StringListType);
	    XtFree(cp);
	    if (!new_value) {
		WarningMessage ("Invalid string entered for column names.");
		return FALSE;
	    }

	    this->names_value->setValue (new_value, DXType::StringListType);
	    if (EqualString(new_value, "NULL")) {
		XmTextFieldSetString (w, "");
	    } else {
		XmTextFieldSetString (w, new_value);
	    }
	    delete new_value;
	}
#endif
    } else
	ASSERT(0);

    return TRUE;
}

//
// set DXLInputs:
//	filename
//	delimiter
//	variable
//	record_start
//	record_end
//	record_delta
//
boolean SpreadSheetChoice::visualize()
{
    if (!this->connect(TypeChoice::ImageMode)) return FALSE;
    DXLConnection* conn = this->gcw->getConnection();

    char net_file[512];
    const char *net_dir = theGARApplication->getResourcesNetDir();
    if (this->tableOption->getState()) {
	sprintf (net_file, "%s/ui/ImportSpreadsheetTable.net", net_dir);
    } else {
	sprintf (net_file, "%s/ui/ImportSpreadsheetMatrix.net", net_dir);
    }
    char* args[20];
    int n = 0;


    const char* dfname = this->gcw->getDataFilename();

    args[n] = "_filename_"; n++;
    args[n] = DuplicateString(dfname); n++;


    args[n] = "_delimiter_"; n++;
    if (this->useDelimiterOption->getState()) {
	if (!this->parseValue (this->delimiter)) 
	    return FALSE ;
	const char *cp = this->delimiter_value->getValueString();
	if (cp) {
	    char* tmp = DuplicateString(cp);
	    int len = strlen(tmp);
	    tmp[len-1] = '\0';
	    args[n] = &tmp[1]; n++;
	} else {
	    args[n] = " "; n++;
	}
    } else {
	args[n] = " "; n++;
    }

#if USES_DXLINK
    if (this->restrictOption->getState()) {
	if (!this->parseValue(this->restrict_names))
	    return FALSE ;
	const char *cp = this->names_value->getValueString();
	if (cp) {
	    args[n] = "_variable"; n++;
	    args[n] = DuplicateString(cp); n++;

	}
    }

    if (this->specifyRowsOption->getState()) {
	int start, end, delta;

	XtVaGetValues (this->starting_row, XmNiValue, &start, NULL);
	XtVaGetValues (this->ending_row, XmNiValue, &end, NULL);
	XtVaGetValues (this->delta_row, XmNiValue, &delta, NULL);

	char tbuf[16];
	sprintf (tbuf, "%d", start);
	args[n] = "_record_start_"; n++;
	args[n] = DuplicateString(tbuf); n++;

	sprintf (tbuf, "%d", end);
	args[n] = "_record_end_"; n++;
	args[n] = DuplicateString(tbuf); n++;

	sprintf (tbuf, "%d", delta);
	args[n] = "_record_delta_"; n++;
	args[n] = DuplicateString(tbuf); n++;
    }
#endif
    this->gcw->loadAndSet (conn, net_file, args, n);
    uiDXLOpenAllImages(conn);

    this->hideList();
    DXLExecuteOnce (conn);

    return TRUE;
}


extern "C"
void SpreadSheetChoice_NumberCB (Widget number, XtPointer clientData, XtPointer)
{
#if USES_DXLINK
    SpreadSheetChoice* ssc = (SpreadSheetChoice*)clientData;
    ASSERT(ssc);
    int starting, ending, delta, tmp;

    XtVaGetValues (ssc->starting_row, XmNiValue, &starting, NULL);
    XtVaGetValues (ssc->ending_row, XmNiValue, &ending, NULL);
    XtVaGetValues (ssc->delta_row, XmNiValue, &delta, NULL);

    //
    // Ensure that ending >= starting + delta
    //
    if ((number == ssc->starting_row) || (number == ssc->delta_row)) {
	tmp = starting + delta;
	if (ending < tmp) {
	    if (ending < starting) {
		XtVaSetValues (ssc->ending_row, 
		    XmNiValue, starting,
		NULL);
	    } else {
		XtVaSetValues (ssc->ending_row, 
		    XmNiValue, tmp,
		NULL);
	    }
	}

    //
    // Ensure that delta <= ending - starting
    //
    } else if (number == ssc->ending_row) {
	tmp = ending - starting;
	tmp = MAX(tmp,1);
	if (ending < starting) {
	    XtVaSetValues (ssc->starting_row,
		XmNiValue, ending,
	    NULL);
	} 

    } else {
	ASSERT(0);
    }
#endif
}
