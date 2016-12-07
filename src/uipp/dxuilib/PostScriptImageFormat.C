/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "PostScriptImageFormat.h"
#include "ImageNode.h"
#include "ImageFormatDialog.h"
#include "DXApplication.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include "../widgets/Number.h"
#include "WarningDialogManager.h"
#include "SymbolManager.h"

String PostScriptImageFormat::DefaultResources[] = {
    "*pageSizeLabel.labelString:	Page Dimensions:",
    "*marginSizeLabel.labelString:	Margin Width:",
    "*sizeLabel.labelString:		Image Dimensions:",
    "*pixelsLabel.labelString:		Input Image Size:",
    "*dpiLabel.labelString:		Output PPI:",
    "*layoutOM.labelString:		Orientation:",
    NUL(char*)
};

boolean PostScriptImageFormat::FmtConnectionWarning = FALSE;

//
// FIXME: What is a metric version of 8.5x11.0?
//
#define DEF_PAGE_SIZE 		"8.5x11.0"
#define DEF_PAGE_SIZE_CM 	"21.59x27.94"
#define RESET_PAGE this->isMetric()?DEF_PAGE_SIZE_CM:DEF_PAGE_SIZE

//
// How to print out the image dims in inches/cms
//
#define SIZE_FMT 		"%.1fx%.1f"
//#define SIZE_FMT 		"%.3lgx%.3lg"
#define PAGE_SIZE_FMT 		SIZE_FMT

#define ILLEGAL_VALUE 		"Ignoring illegal value"

PostScriptImageFormat::PostScriptImageFormat (const char *name, ImageFormatDialog *dialog) : 
    ImageFormat(name, dialog)
{
    this->size_text 		= NUL(Widget);
    this->page_size_text 	= NUL(Widget);
    this->chosen_layout 	= NUL(Widget);
    this->pixel_size_text	= NUL(Widget);
    this->dpi_number		= NUL(Widget);
    this->margin_width		= NUL(Widget);
    this->autoorient_button	= NUL(Widget);
    this->portrait_button	= NUL(Widget);
    this->landscape_button	= NUL(Widget);
    this->page_layout_om	= NUL(Widget);

    this->dirty = 0;
    this->pixel_size_val = NUL(char*);
    this->size_timer = 0;
    this->page_size_timer = 0;
    this->entered_dims_valid = FALSE;
    this->parsePageSize(RESET_PAGE);
}

PostScriptImageFormat::~PostScriptImageFormat()
{
    if (this->pixel_size_val) delete this->pixel_size_val;
    if (this->size_timer) XtRemoveTimeOut (this->size_timer);
}

Widget PostScriptImageFormat::createBody (Widget parent)
{
Arg args[9];

    this->initialize();

    Widget body = XtVaCreateManagedWidget (this->name,
	xmFormWidgetClass,	parent,
	XmNmappedWhenManaged,	False,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		2,
    NULL);

    //
    // W I D T H  X  H E I G H T       W I D T H  X  H E I G H T 
    // W I D T H  X  H E I G H T       W I D T H  X  H E I G H T 
    // W I D T H  X  H E I G H T       W I D T H  X  H E I G H T 
    //
    Widget lab = XtVaCreateManagedWidget ("sizeLabel",
	xmLabelWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopOffset,		7,
	XmNalignment,		XmALIGNMENT_END,
    NULL);
    this->size_text = XtVaCreateManagedWidget ("sizeText",
	xmTextWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		2,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		lab,
	XmNleftOffset,		2,
	XmNcolumns,		10,
    NULL);	
    XtAddCallback (this->size_text, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_ParseSizeCB, (XtPointer)this);
    XtAddCallback (this->size_text, XmNmodifyVerifyCallback, (XtCallbackProc)
	PostScriptImageFormat_ModifyCB, (XtPointer)this);
    XtAddEventHandler (this->size_text, LeaveWindowMask, False, 
	(XtEventHandler)PostScriptImageFormat_ParseSizeEH, (XtPointer)this);

    XtVaCreateManagedWidget ("pixelsLabel",
	xmLabelWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->size_text,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopOffset,		7,
	XmNalignment,		XmALIGNMENT_END,
    NULL);
    this->pixel_size_text = XtVaCreateManagedWidget ("pixelsText",
	xmTextWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->size_text,
	XmNtopOffset,		2,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->size_text,
	XmNleftOffset,		0,
	XmNcolumns,		10,
    NULL);	
    XtAddCallback (this->pixel_size_text, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_ParseSizeCB, (XtPointer)this);
    XtAddCallback (this->pixel_size_text, XmNmodifyVerifyCallback, (XtCallbackProc)
	PostScriptImageFormat_ModifyCB, (XtPointer)this);
    XtAddEventHandler (this->pixel_size_text, LeaveWindowMask, False, 
	(XtEventHandler)PostScriptImageFormat_ParseSizeEH, (XtPointer)this);

    //
    // P A G E   S I Z E       P A G E   S I Z E       P A G E   S I Z E  
    // P A G E   S I Z E       P A G E   S I Z E       P A G E   S I Z E  
    // P A G E   S I Z E       P A G E   S I Z E       P A G E   S I Z E  
    //
    lab = XtVaCreateManagedWidget ("pageSizeLabel",
	xmLabelWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->pixel_size_text,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopOffset,		7,
	XmNalignment,		XmALIGNMENT_END,
    NULL);
    char tbuf[64];
    if (this->isMetric())
	sprintf (tbuf, PAGE_SIZE_FMT, 
	    this->page_width*CM_PER_INCH, this->page_height*CM_PER_INCH);
    else
	sprintf (tbuf, PAGE_SIZE_FMT, this->page_width, this->page_height);
    this->page_size_text = XtVaCreateManagedWidget ("pageSizeText",
	xmTextWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->pixel_size_text,
	XmNtopOffset,		2,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->size_text,
	XmNleftOffset,		0,
	XmNcolumns,		10,
	XmNvalue,		tbuf,
    NULL);
    XtAddCallback (this->page_size_text, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_ParsePageSizeCB, (XtPointer)this);
    XtAddCallback (this->page_size_text, XmNmodifyVerifyCallback, (XtCallbackProc)
	PostScriptImageFormat_PageModifyCB, (XtPointer)this);


    //
    // P A G E    L A Y O U T           P A G E    L A Y O U T 
    // P A G E    L A Y O U T           P A G E    L A Y O U T 
    // P A G E    L A Y O U T           P A G E    L A Y O U T 
    //
    int n = 0;
    XtSetArg (args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
    Widget pulldown = XmCreatePulldownMenu(parent, "layoutPDM", args, n);
    this->autoorient_button = this->chosen_layout =
	XtVaCreateManagedWidget("automatic", xmPushButtonWidgetClass, pulldown, NULL);
    this->portrait_button = 
	XtVaCreateManagedWidget("portrait", xmPushButtonWidgetClass, pulldown, NULL);
    this->landscape_button = 
	XtVaCreateManagedWidget("landscape", xmPushButtonWidgetClass, pulldown, NULL);
    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset, 2); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, 2); n++;
    XtSetArg (args[n], XmNsubMenuId, pulldown); n++;
    this->page_layout_om = XmCreateOptionMenu (body, "layoutOM", args, n);
    XtManageChild (this->page_layout_om);
    XtAddCallback (this->autoorient_button, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_OrientCB, (XtPointer)this);
    XtAddCallback (this->landscape_button, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_OrientCB, (XtPointer)this);
    XtAddCallback (this->portrait_button, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_OrientCB, (XtPointer)this);


    //
    // D P I     D P I     D P I     D P I     D P I     D P I     D P I 
    // D P I     D P I     D P I     D P I     D P I     D P I     D P I 
    // D P I     D P I     D P I     D P I     D P I     D P I     D P I 
    //
    this->dpi = 300;
    int dots_per = this->dpi;
    if (this->isMetric())
	dots_per = (int)((double)this->dpi / (double)CM_PER_INCH);
    this->dpi_number = XtVaCreateManagedWidget ("dpi",
	xmNumberWidgetClass,    body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->page_layout_om,
	XmNtopOffset,		8,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		8,
	XmNdataType,            INTEGER,
	XmNiValue,		dots_per,
	XmNcharPlaces,		5,
	XmNeditable,		True,
	XmNrecomputeSize,	False,
	XmNiMinimum,		1,
    NULL);
    XtAddCallback (this->dpi_number, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_DpiCB, (XtPointer)this);
    XtVaCreateManagedWidget ("dpiLabel",
	xmLabelWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->page_layout_om,
	XmNtopOffset,		12,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->dpi_number,
	XmNrightOffset,		2,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	XmNwidth,		110,
    NULL);

    //
    // M A R G I N   S I Z E      M A R G I N   S I Z E      M A R G I N   S I Z E 
    // M A R G I N   S I Z E      M A R G I N   S I Z E      M A R G I N   S I Z E 
    // M A R G I N   S I Z E      M A R G I N   S I Z E      M A R G I N   S I Z E 
    //
#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1, dx_l2, dx_l3, dx_l4; 
#endif
    double inc = 0.1;
    double value = 0.5;
    double min = 0.0;
    double max = MIN( ((this->page_width/2.0)-0.2), ((this->page_height/2.0)-0.2) );
    if (this->isMetric()) value*= CM_PER_INCH;
    this->margin_width = XtVaCreateManagedWidget ("marginWidth",
	xmNumberWidgetClass,    body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->dpi_number,
	XmNtopOffset,		3,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		8,
	XmNdataType,            DOUBLE,
	XmNdValueStep,          DoubleVal(inc, dx_l2),
	XmNdValue,		DoubleVal(value, dx_l1),
	XmNdecimalPlaces,       2,
	XmNcharPlaces,		5,
	XmNrecomputeSize,	False,
	XmNfixedNotation,       False,
	XmNeditable,		True,
	XmNdMinimum,		DoubleVal(min, dx_l3),
	XmNdMaximum,		DoubleVal(max, dx_l4),
    NULL);
    XtAddCallback (this->margin_width, XmNactivateCallback, (XtCallbackProc)
	PostScriptImageFormat_MarginCB, (XtPointer)this);
    XtVaCreateManagedWidget ("marginSizeLabel",
	xmLabelWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->dpi_number,
	XmNtopOffset,		7,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->dpi_number,
	XmNrightOffset,		2,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	XmNwidth,		110,
    NULL);

    this->setRootWidget(body);
    return body;
}

boolean PostScriptImageFormat::useLocalFormat()
{
    if (this->ImageFormat::useLocalFormat()) return TRUE;
    if (this->dirty & PostScriptImageFormat::DirtyDpi) return TRUE;
    return FALSE;
}

boolean PostScriptImageFormat::useLocalResolution()
{
ImageNode *node = this->dialog->getNode();
    boolean rescon = node->isRecordResolutionConnected();
    boolean rerender = this->dialog->isRerenderAllowed();

    if ((rescon) || (!rerender)) return FALSE;
    return ((this->dirty & PostScriptImageFormat::DirtyResolution)?TRUE:FALSE) ;
}

boolean PostScriptImageFormat::useLocalAspect()
{
ImageNode *node = this->dialog->getNode();
    boolean aspcon = node->isRecordAspectConnected();
    boolean rerender = this->dialog->isRerenderAllowed();

    if ((aspcon) || (!rerender)) return FALSE;
    return ((this->dirty & PostScriptImageFormat::DirtyAspect)?TRUE:FALSE) ;
}

void PostScriptImageFormat::setCommandActivation()
{
int x,y,height;
double hd, aspect;
char size_val[64];
ImageNode *node = this->dialog->getNode();

    if (this->dialog->isResetting()) return ;

    //
    // Flags which will tell use what to do...
    //
    boolean rescon = node->isRecordResolutionConnected();
    boolean resset = rescon || node->isRecordResolutionSet();
    boolean aspcon = node->isRecordAspectConnected();
    boolean aspset = aspcon || node->isRecordAspectSet();
    boolean dirty_res = (this->dirty & PostScriptImageFormat::DirtyResolution);
    boolean dirty_asp = (this->dirty & PostScriptImageFormat::DirtyAspect);
    boolean rerender = this->dialog->isRerenderAllowed();
    boolean fmtcon = node->isRecordFormatConnected();

    //
    // What are the node's values?
    //
    boolean resfetched = FALSE;
    boolean aspfetched = FALSE;
    if (rescon) {
	node->getRecordResolution(x,y);
	resfetched = TRUE;
    } 
    if (aspcon) {
	node->getRecordAspect(aspect);
	aspfetched = TRUE;
    } 
    if ((rerender) && (!resfetched) && (resset)) {
	node->getRecordResolution(x,y);
	resfetched = TRUE;
    }
    if ((rerender) && (!aspfetched) && (aspset)) {
	node->getRecordAspect(aspect);
	aspfetched = TRUE;
    }
    if (!resfetched) 
	node->getResolution(x,y);
    if (!aspfetched)
	node->getAspect(aspect);

    //
    // Do we use the node's value or our own?
    //
    boolean use_nodes_resolution = (rescon || !rerender || (resset && !dirty_res));
    boolean use_nodes_aspect = (aspcon || !rerender || (aspset && !dirty_asp));

    //
    // If we know we're using the node's value, then disable the text widget so
    // that the user sees she can't enter her own value.
    //
    if (((rescon) && (aspcon)) || (!rerender)) {
	this->setTextSensitive (this->pixel_size_text, FALSE);
    } else {
	this->setTextSensitive (this->pixel_size_text, TRUE);
    }
    XtSetSensitive (this->dpi_number, (fmtcon == FALSE));

    //
    // If recordFormat is connected (which implies dpi is fixed), and allow-rerendering
    // is off, then it's not possible to alter the output image dimensions.
    //
    if ((fmtcon == FALSE) || (this->dialog->isRerenderAllowed() == TRUE)) {
	this->setTextSensitive (this->size_text, TRUE);
    } else {
	this->setTextSensitive (this->size_text, FALSE);
    }

    //
    // Based on what we already know, lets put a value into the text widget.
    //
    if ((use_nodes_resolution) && (use_nodes_aspect)) {
	this->aspect = aspect;
	this->width = x;
	hd = (double)this->width * this->aspect;
	this->dirty&= ~PostScriptImageFormat::DirtyResolution;
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;


    //
    // Only use the node's resolution value, use a stored 
    // aspect to compute the height.
    //
    } else if (use_nodes_resolution) {
	this->width = x;
	hd = (double)this->width * this->aspect;
	this->dirty&= ~PostScriptImageFormat::DirtyResolution;


    //
    // Use the node's apsect value.  Use a stored resolution value to
    // compute the height.
    //
    } else if (use_nodes_aspect) {
	this->aspect = aspect;
	hd = (double)this->width * this->aspect;
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;


    //
    // Use only the values store here.
    //
    } else {
	hd = (double)this->width * this->aspect;
    }
    height = (int)hd;
    if ((hd - height) >= 0.5) height++;

    //
    // Set page layout
    //
    double mrgn;
    XtVaGetValues (this->margin_width, XmNdValue, &mrgn, NULL);
    double mrgn2 = 2.0 * mrgn;

    Orientation* portrait = NUL(Orientation*);;
    Orientation* landscape = NUL(Orientation*);;
    Orientation* chosen_orientation = NUL(Orientation*);

    if ((this->entered_dims_valid == FALSE) ||
	((this->dirty & PostScriptImageFormat::DirtyImageSize) == 0)) {
	portrait = new Orientation 
	    (this->width, height, this->page_width, this->page_height, mrgn2);
	landscape = new Orientation 
	    (height, this->width, this->page_width, this->page_height,mrgn2);
    } else {
	portrait = new RestrictedOrientation (
	    this->width, height, this->page_width, this->page_height, mrgn2,
	    this->entered_printout_width, this->entered_printout_height, FALSE
	    );
	landscape = new RestrictedOrientation (
	    this->width, height, this->page_height, this->page_width, mrgn2,
	    this->entered_printout_width, this->entered_printout_height, FALSE
	    );
    }

    if (this->chosen_layout == this->autoorient_button) {
	if (landscape->misfit <= portrait->misfit)
	    chosen_orientation = landscape;
	else
	    chosen_orientation = portrait;
    } else if (this->chosen_layout == this->portrait_button) {
	chosen_orientation = portrait;
    } else {
	ASSERT (this->chosen_layout == this->landscape_button);
	chosen_orientation = landscape;
    }
    ASSERT(chosen_orientation);

    //
    // Autocompute printer dpi
    //
    if ((fmtcon == FALSE) && ((this->dirty & PostScriptImageFormat::DirtyDpi) == 0)) {
	this->dpi = chosen_orientation->dpi;
	XtVaSetValues (this->dpi_number,
	    XmNiValue, this->dpi,
	NULL);
    } else if (fmtcon == TRUE) {
	const char* value = NUL(char*); 
	node->getRecordFormat(value);
	char *matchstr = "dpi=";
	const char *cp = strstr (value, matchstr);
	if (cp) {
	    int dpi;
	    cp+= strlen(matchstr);
	    int items_parsed = sscanf (cp, "%d", &dpi);
	    if (items_parsed == 1) {
		this->dpi = dpi;
		if (this->isMetric()) {
		    double d = dpi;
		    d/= CM_PER_INCH;
		    dpi = (int)d;
		    if ((d-dpi) > 0.0) dpi++;
		}
		XtVaSetValues (this->dpi_number,
		    XmNiValue, dpi,
		NULL);
	    }
	}
    }

    if (landscape) delete landscape;
    landscape = NUL(Orientation*);
    if (portrait) delete portrait;
    portrait = NUL(Orientation*);
    chosen_orientation = NUL(Orientation*);

    XtSetSensitive (this->page_layout_om, (fmtcon == False));


    //
    // Round the float value for height and put the result into
    // the text widget.
    //
    sprintf (size_val, "%dx%d", this->width, height);
    if (this->pixel_size_val) delete this->pixel_size_val;
    this->pixel_size_val = DuplicateString(size_val);

    double width_inches, height_inches;
    width_inches = this->width / (double)this->dpi;
    height_inches = (double)height / (double)this->dpi;

    if (this->size_text) {
	if (this->size_timer)
	    XtRemoveTimeOut (this->size_timer);
	this->size_timer = 0;

	int old_dirt = this->dirty;
	this->setSizeTextString (this->pixel_size_text, this->pixel_size_val);
	if (!this->setVerifiedSizeTextString (width_inches, height_inches)) {
	    if (old_dirt != this->dirty)
		this->setCommandActivation();
	}
    }
}

void PostScriptImageFormat::changeUnits()
{
#if 0
char tbuf[128];

    //
    // Reset page size
    //
    if (this->isMetric()) {
	sprintf (tbuf, PAGE_SIZE_FMT, this->page_width * CM_PER_INCH,
	    this->page_height * CM_PER_INCH);
    } else {
	sprintf (tbuf, PAGE_SIZE_FMT, this->page_width, this->page_height);
    }

    //
    // Reset margin width,height
    //

    //
    // Reset image size
    //
#endif
}

void PostScriptImageFormat::applyValues ()
{
    this->dirty&= ~PostScriptImageFormat::DirtyResolution;
    this->dirty&= ~PostScriptImageFormat::DirtyAspect;
    this->dirty&= ~PostScriptImageFormat::DirtyOrient;
    this->dirty&= ~PostScriptImageFormat::DirtyPageSize;
    this->dirty&= ~PostScriptImageFormat::DirtyImageSize;
}

void PostScriptImageFormat::parseRecordFormat(const char* value)
{
    ImageNode *node = this->dialog->getNode();
    boolean fmtcon = node->isRecordFormatConnected();
    boolean issue_ubd_warning = FALSE;

    if ((fmtcon) || ((this->dirty & PostScriptImageFormat::DirtyDpi) == 0)) {
	char *matchstr = "dpi=";
	const char *cp = strstr (value, matchstr);
	if (cp) {
	    int dpi;
	    cp+= strlen(matchstr);
	    int items_parsed = sscanf (cp, "%d", &dpi);
	    if (items_parsed == 1) {
		this->dpi = dpi;
		if (fmtcon) this->dirty|= PostScriptImageFormat::DirtyDpi;
		if (this->isMetric()) {
		    double d = dpi;
		    d/= CM_PER_INCH;
		    dpi = (int)d;
		    if ((d-dpi) > 0.0) dpi++;
		}
		XtVaSetValues (this->dpi_number,
		    XmNiValue, dpi,
		NULL);
	    } else if (fmtcon) {
		issue_ubd_warning = TRUE;
	    }
	} else if (fmtcon) {
	    issue_ubd_warning = TRUE;
	}
    }

    if ((fmtcon) || ((this->dirty & PostScriptImageFormat::DirtyOrient) == 0)) {
	char *matchstr = "orient=";
	const char *cp = strstr (value, matchstr);
	boolean correctly_parsed = FALSE;
	Widget new_choice = NUL(Widget);
	if (cp) {
	    char ori[128];
	    int items_parsed = sscanf (cp, "orient=%[^ ]", ori);
	    if (items_parsed == 1) {
		int len = strlen(ori);
		if ((len) && (ori[len-1] == '"'))
		    ori[len-1] = '\0';
		if (EqualString(ori, "landscape")) {
		    correctly_parsed = TRUE;
		    new_choice = this->landscape_button;
		} else if (EqualString(ori, "portrait")) {
		    correctly_parsed = TRUE;
		    new_choice = this->portrait_button;
		} 
	    } 
	}
	if ((!correctly_parsed) && (fmtcon)) {
	    new_choice = this->autoorient_button;
	    issue_ubd_warning = TRUE;
	}
	if ((new_choice) && (new_choice != this->chosen_layout) && 
	    ((this->chosen_layout != this->autoorient_button)||(fmtcon))) {
	    this->chosen_layout = new_choice;
	    XtVaSetValues(this->page_layout_om, XmNmenuHistory, this->chosen_layout,NULL);
	    this->dirty|= PostScriptImageFormat::DirtyOrient;
	}
    }

    if ((this->dirty & PostScriptImageFormat::DirtyPageSize) == 0) {
	boolean refreshed = FALSE;
	char *matchstr = "page=";
	const char *cp = strstr (value, matchstr);
	if (cp) {
	    char psize[64];
	    cp+= strlen(matchstr);
	    int items_parsed = sscanf (cp, "%s", psize);
	    if (items_parsed == 1) {
		this->parsePageSize(psize);
		this->dirty|= PostScriptImageFormat::DirtyPageSize;
		refreshed = TRUE;
	    }
	}
	if (!refreshed) {
	    this->parsePageSize(RESET_PAGE);
	}
    }

    if ((issue_ubd_warning) && (PostScriptImageFormat::FmtConnectionWarning == FALSE)) {
	PostScriptImageFormat::FmtConnectionWarning = True;
	char* cp = NUL(char*);
	if (theDXApplication->appAllowsEditorAccess() == FALSE)
	    cp = 
		"The value for the \'recordFormat\' input parameter of your\n"
		"image tool is specified by an incomming connection, and is\n"
		"missing a dpi or orientation value.  Check your interactor\n"
		"settings.  This warning will not appear again.";
	else
	    cp = 
		"The value for the \'recordFormat\' input parameter of your image\n"
		"tool is specified by an incomming connection, and is missing a\n"
		"dpi or orientation value.   Disconnect the paramter or modify\n"
		"your interactor values in order to obtain dependable results\n"
		"from the SaveImage dialog. This warning will not appear again.";
	WarningMessage (cp);
    }
}

void PostScriptImageFormat::restore()
{
    this->dirty = 0;
    this->fifo_bits.clear();

    if (this->size_timer) XtRemoveTimeOut (this->size_timer);
    if (this->page_size_timer) XtRemoveTimeOut (this->page_size_timer);
    this->size_timer = 0;
    this->page_size_timer = 0;
    this->entered_dims_valid = FALSE;
    this->parsePageSize(RESET_PAGE);

    char tbuf[64];
    if (this->isMetric())
	sprintf (tbuf, PAGE_SIZE_FMT, 
	    this->page_width * CM_PER_INCH, this->page_height * CM_PER_INCH);
    else
	sprintf (tbuf, PAGE_SIZE_FMT, this->page_width, this->page_height);
    this->setTextString (this->page_size_text, tbuf, (void*)
	PostScriptImageFormat_PageModifyCB);

    this->chosen_layout = this->autoorient_button;
    XtVaSetValues (this->page_layout_om, XmNmenuHistory, this->chosen_layout, NULL);

#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1; 
#endif
    double value = 0.5;
    if (this->isMetric()) value*= CM_PER_INCH;
    XtVaSetValues (this->margin_width, 
	XmNdValue, DoubleVal(value, dx_l1),
    NULL);
}

void PostScriptImageFormat::parseImageSize(const char *str)
{
    if ((!str) || (!str[0])) return ;

    char *dimstr = DuplicateString(str);

    boolean width_parsed = FALSE;
    boolean height_parsed = FALSE;
    double dwidth = 0.0;
    double dheight = 0.0;

    char *cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	*cp = '\0';
    }
    if ((!cp) || (cp != dimstr)) {
	int items_parsed = sscanf (dimstr, "%lg", &dwidth);
	if ((items_parsed == 1) && (dwidth != 0.0))
	    width_parsed = TRUE;
    }
    delete dimstr;


    dimstr = DuplicateString(str);
    cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	cp++;
	if (cp[0]) {
	    int items_parsed = sscanf (cp, "%lg", &dheight);
	    if ((items_parsed == 1) && (dheight != 0.0))
		height_parsed = TRUE;
	}
    }
    delete dimstr;

    if (((width_parsed) && (dwidth <= 0.0)) ||
	((height_parsed) && (dheight <= 0.0))) {
	WarningMessage (ILLEGAL_VALUE);
	return ;
    }

    int width = (int)dwidth;
    int height = (int)dheight;

    if ((width_parsed) && (height_parsed)) {
	double wd = width;
	double hd = height;
	this->width = width;
	this->aspect = hd / wd;
	this->dirty|= PostScriptImageFormat::DirtyResolution;
	this->dirty|= PostScriptImageFormat::DirtyAspect;


    } else if (width_parsed) {
	ImageNode *node = this->dialog->getNode();
	this->width = width;
	node->getAspect(this->aspect);
	this->dirty|= PostScriptImageFormat::DirtyResolution;
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;

    } else if (height_parsed) {
	ImageNode *node = this->dialog->getNode();
        node->getAspect(this->aspect);
	double hd = height;
	double wd = hd / this->aspect;
	this->width = (int)wd;
	this->dirty|= PostScriptImageFormat::DirtyResolution;
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;

    } else {
	// The user can't type very well.
	int y;
	ImageNode *node = this->dialog->getNode();
	node->getResolution (this->width,y);
	node->getAspect(this->aspect);
	this->dirty&= ~PostScriptImageFormat::DirtyResolution;
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;
    }

    if ((this->dialog->isRerenderAllowed()) && (this->entered_dims_valid)) {
	double hd = (double)this->width * this->aspect;
	int height = (int)hd;
	if ((hd - height) >= 0.5) height++;
	Orientation portrait(this->width, height, 
	    this->entered_printout_width, this->entered_printout_height, 0.0, FALSE);
	Orientation landscape(height, this->width, 
	    this->entered_printout_width, this->entered_printout_height, 0.0, FALSE);
	Orientation *chosen_orientation = NUL(Orientation*);
	if (this->chosen_layout == this->autoorient_button) {
	    if (landscape.misfit <= portrait.misfit)
		chosen_orientation = &landscape;
	    else
		chosen_orientation = &portrait;
	} else if (this->chosen_layout == this->portrait_button) {
	    chosen_orientation = &portrait;
	} else {
	    ASSERT (this->chosen_layout == this->landscape_button);
	    chosen_orientation = &landscape;
	}
	ASSERT(chosen_orientation);
	this->dpi = chosen_orientation->dpi;
	XtVaSetValues (this->dpi_number,
	    XmNiValue, this->dpi,
	NULL);
    } 
}

void PostScriptImageFormat::parsePictureSize(const char *str)
{
    if ((!str) || (!str[0])) {
	this->entered_dims_valid = FALSE;
	return ;
    }

    char *dimstr = DuplicateString(str);

    boolean width_parsed = FALSE;
    boolean height_parsed = FALSE;
    double dwidth = 0.0;
    double dheight = 0.0;

    char *cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	*cp = '\0';
    }
    if ((!cp) || (cp != dimstr)) {
	int items_parsed = sscanf (dimstr, "%lg", &dwidth);
	if ((items_parsed == 1) && (dwidth != 0.0))
	    width_parsed = TRUE;
    }
    delete dimstr;


    dimstr = DuplicateString(str);
    cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	cp++;
	if (cp[0]) {
	    int items_parsed = sscanf (cp, "%lg", &dheight);
	    if ((items_parsed == 1) && (dheight != 0.0))
		height_parsed = TRUE;
	}
    }
    delete dimstr;

    if (((width_parsed) && (dwidth <= 0.0)) ||
	((height_parsed) && (dheight <= 0.0))) {
	WarningMessage (ILLEGAL_VALUE);
	return ;
    }

    if ((!width_parsed) && (!height_parsed)) return ;

    //
    // Scale the numbers in case we're metric
    //
    if (this->isMetric()) {
	if (width_parsed) dwidth/= CM_PER_INCH;
	if (height_parsed) dheight/= CM_PER_INCH;
    }


    //
    // If no rerender then just calculate a new dpi value
    // If rerender, then also allow a new aspect ratio
    // Never calculate a width.
    //
    if (this->dialog->isRerenderAllowed()) {
	if ((width_parsed) && (height_parsed)) {
	    this->aspect = dheight / dwidth;
	    this->dirty|= PostScriptImageFormat::DirtyAspect;
	}
    } else {
	if ((width_parsed) && (height_parsed)) {
	    char msg[256];
	    sprintf (msg, 
		"These values (%s) would change the\n%s\n%s", str,
		"aspect ratio of the image.  You must push the",
		"\'Allow Rerendering\' toggle, first.");
	    WarningMessage (msg);
	}
    }

    ImageNode *node = this->dialog->getNode();
    if (node->isRecordFormatConnected() == FALSE) {
	//
	// change pixel image size to match dpi and output image size
	//
	if (this->dialog->isRerenderAllowed()) {
	    ASSERT (height_parsed || width_parsed);
	    double new_resolution;
	    if (width_parsed) new_resolution = dwidth * this->dpi;
	    else new_resolution = (dheight/this->aspect) * this->dpi;
	    this->width = (int)new_resolution;
	    if ((new_resolution - this->width) > 0.5) this->width++;
	    this->dirty|= PostScriptImageFormat::DirtyResolution;
	}
    } else {
	//
	// If recordFormat is connected, then we can't change dpi, so we'll
	// need to recalculate width and/or height.  But, we'll wait until
	// we've marked the user's entries valid or invalid.
	//
    }

    //
    // In case the user typed in a size for the printout, hang on to it and
    // mark the numbers valid so that a subsequent pixel entry can solve for
    // a new dpi value.
    //
    if (width_parsed) {
	this->entered_printout_width = dwidth;
	this->entered_dims_valid = TRUE;
    } else {
	ASSERT(height_parsed);
	this->entered_printout_width = dheight / this->aspect;
    }
    if (height_parsed) {
	this->entered_printout_height = dheight;
	this->entered_dims_valid = TRUE;
    } else {
	ASSERT(width_parsed);
	this->entered_printout_height = dwidth * this->aspect;
    }


    if ((node->isRecordFormatConnected() == TRUE) && (width_parsed)) {
	this->dirty|= PostScriptImageFormat::DirtyResolution;
	this->width = (int)(dwidth * this->dpi);
    }

    if (this->entered_dims_valid)
	this->dirty|= PostScriptImageFormat::DirtyImageSize;
}

void PostScriptImageFormat::parsePageSize(const char *str)
{
    if ((!str) || (!str[0])) return ;

    char *dimstr = DuplicateString(str);

    boolean width_parsed = FALSE;
    boolean height_parsed = FALSE;
    double width, height;

    char *cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	*cp = '\0';
    }
    if ((!cp) || (cp != dimstr)) {
	int items_parsed = sscanf (dimstr, "%lg", &width);
	if (items_parsed == 1) width_parsed = TRUE;
    }
    delete dimstr;


    dimstr = DuplicateString(str);
    cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	cp++;
	if (cp[0]) {
	    int items_parsed = sscanf (cp, "%lg", &height);
	    if (items_parsed == 1) height_parsed = TRUE;
	}
    }
    delete dimstr;

    if ((width_parsed) && (height_parsed)) {
	this->page_width = width / (this->isMetric() ? CM_PER_INCH: 1.0);
	this->page_height = height / (this->isMetric() ? CM_PER_INCH: 1.0);

    } else if (width_parsed) {
	this->page_width = width / (this->isMetric() ? CM_PER_INCH: 1.0);

    } else if (height_parsed) {
	this->page_height = height / (this->isMetric() ? CM_PER_INCH: 1.0);

    } else {
	// The user can't type very well.
	this->page_width = 8.5;
	this->page_height = 11.0;
    }

    //
    // Return the values to the text widget.
    //
    if (this->page_size_text) {
	char tbuf[64];
	if (this->isMetric())
	    sprintf (tbuf, PAGE_SIZE_FMT, 
		this->page_width * CM_PER_INCH, this->page_height * CM_PER_INCH);
	else
	    sprintf (tbuf, PAGE_SIZE_FMT, this->page_width, this->page_height);

	this->setTextString (this->page_size_text, tbuf, (void*)
	    PostScriptImageFormat_PageModifyCB);
    }

    //
    // Set max margin size.
    //
    if (this->margin_width) {
	double min_dim = MIN(this->page_width,this->page_height);
	double max_mrgn = (min_dim/2.0) - 0.2;
	double mrgn;
	max_mrgn = MAX(0.0, max_mrgn);
#ifdef PASSDOUBLEVALUE
	XtArgVal    dx_l1, dx_l2; 
#endif
	XtVaGetValues (this->margin_width,
	    XmNdValue, &mrgn,
	NULL);

	if (mrgn > max_mrgn) {
	    XtVaSetValues (this->margin_width, 
		XmNdValue, DoubleVal(max_mrgn, dx_l2),
		XmNdMaximum, DoubleVal(max_mrgn, dx_l1),
	    NULL);
	} else {
	    XtVaSetValues (this->margin_width, 
		XmNdMaximum, DoubleVal(max_mrgn, dx_l1),
	    NULL);
	}

    }
}

const char* PostScriptImageFormat::getRecordFormat()
{
static char formstr[512];
char tbuf[128];
int totlen = 0;
 
    const char *cp = this->paramString();
    strcpy (&formstr[totlen], cp);
    totlen = strlen (cp);

    if (this->dialog->dirtyGamma()) {
	formstr[totlen++] = ' '; formstr[totlen] = '\0';
	sprintf (tbuf, "gamma=%.3g", this->dialog->getGamma());
	strcpy (&formstr[totlen], tbuf);
	totlen+= strlen(tbuf);
    }

    if ((1) || (this->dirty & PostScriptImageFormat::DirtyDpi)) {
	formstr[totlen++] = ' '; formstr[totlen] = '\0';
	sprintf (tbuf, "dpi=%d", this->dpi);
	strcpy (&formstr[totlen], tbuf);
	totlen+= strlen(tbuf);
    }

    if (1) {
	formstr[totlen++] = ' '; formstr[totlen] = '\0';
	if (this->chosen_layout == this->landscape_button)
	    strcpy(tbuf, "orient=landscape");
	else if (this->chosen_layout == this->portrait_button)
	    strcpy(tbuf, "orient=portrait");
	else if (this->aspect >= 1.0)
	    strcpy(tbuf, "orient=portrait");
	else
	    strcpy(tbuf, "orient=landscape");
	strcpy (&formstr[totlen], tbuf);
	totlen+= strlen(tbuf);
    }

    if (this->dirty & PostScriptImageFormat::DirtyPageSize) {
	formstr[totlen++] = ' '; formstr[totlen] = '\0';
	char *cp = XmTextGetString (this->page_size_text);
	if (cp) {
	    sprintf (tbuf, "page=%s", cp);
	    strcpy (&formstr[totlen], tbuf);
	    totlen+= strlen(tbuf);
	    XtFree(cp);
	}
    }

    if (this->dialog->getDelayedColors()) {
	formstr[totlen++] = ' '; formstr[totlen] = '\0';
	strcpy (tbuf, "delayed=1");
	strcpy (&formstr[totlen], tbuf);
	totlen+= strlen(tbuf);
    }
	  
    return formstr;
}


boolean PostScriptImageFormat::setVerifiedSizeTextString (double width, double height)
{
    char tbuf[64];

    double dh = this->width * this->aspect;
    int ih = (int)dh;
    if ((dh-ih) >= 0.5) ih++;

    double mrgn;
    XtVaGetValues (this->margin_width, XmNdValue, &mrgn, NULL);
    double mrgn2 = 2.0 * mrgn;

    Orientation  portrait(this->width, ih, this->page_width, this->page_height, mrgn2);
    Orientation landscape(ih, this->width, this->page_width, this->page_height, mrgn2);
    Orientation *chosen_orientation = NUL(Orientation*);

    if (this->chosen_layout == this->autoorient_button) {
	if (landscape.misfit <= portrait.misfit)
	    chosen_orientation = &landscape;
	else
	    chosen_orientation = &portrait;
    } else if (this->chosen_layout == this->portrait_button) {
	chosen_orientation = &portrait;
    } else {
	ASSERT (this->chosen_layout == this->landscape_button);
	chosen_orientation = &landscape;
    }
    ASSERT(chosen_orientation);

    boolean size_valid = TRUE;
    if (chosen_orientation == &landscape) {
	if ((width + mrgn) > this->page_height) size_valid = FALSE;
	else if ((height + mrgn) > this->page_width) size_valid = FALSE;
    } else {
	ASSERT (chosen_orientation == &portrait);
	if ((height + mrgn) > this->page_height) size_valid = FALSE;
	else if ((width + mrgn) > this->page_width) size_valid = FALSE;
    }

    if (size_valid) {
	if (this->isMetric()) {
	    sprintf (tbuf, SIZE_FMT, width * CM_PER_INCH, height * CM_PER_INCH);
	} else {
	    sprintf (tbuf, SIZE_FMT, width, height);
	}
	this->setSizeTextString (this->size_text, tbuf);
    } else {
	this->dirty&= ~PostScriptImageFormat::DirtyDpi;
	this->dirty&= ~PostScriptImageFormat::DirtyResolution;
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;
	this->dirty&= ~PostScriptImageFormat::DirtyOrient;
	this->dirty&= ~PostScriptImageFormat::DirtyImageSize;
	WarningMessage ("Your image size specification exceeded page boundaries");
    }
    return size_valid;
}

void PostScriptImageFormat::shareSettings (ImageFormat *imgfmt)
{
    if (imgfmt->useLocalResolution()) {
	this->width = imgfmt->getRecordResolution();
	this->dirty|= PostScriptImageFormat::DirtyResolution;
    } else {
	this->dirty&= ~PostScriptImageFormat::DirtyResolution;
    }
 
    if (imgfmt->useLocalAspect()) {
	this->aspect = imgfmt->getRecordAspect();
	this->dirty|= PostScriptImageFormat::DirtyAspect;
    } else {
	this->dirty&= ~PostScriptImageFormat::DirtyAspect;
    }
 
    if (imgfmt->useLocalFormat()) {
	const char *cp = imgfmt->getRecordFormat();
	this->parseRecordFormat(cp);
	if (imgfmt->isA(theSymbolManager->registerSymbol(ClassPostScriptImageFormat)))
	    this->dirty|= PostScriptImageFormat::DirtyDpi;
    } else if (imgfmt->isA(theSymbolManager->registerSymbol(ClassPostScriptImageFormat))){
	this->dirty&= ~PostScriptImageFormat::DirtyDpi;
	if (this->dirty & PostScriptImageFormat::DirtyPageSize) {
	    this->dirty&= ~PostScriptImageFormat::DirtyPageSize;
	    this->parsePageSize(RESET_PAGE);
	}
    }
}

void PostScriptImageFormat::setSizeTextString (Widget w, char *value)
{
    this->setTextString (w, value, (void*)PostScriptImageFormat_ModifyCB);
}
void PostScriptImageFormat::setTextString (Widget w, char *value, void* modifyCB)
{
    if (w) {
	XtRemoveCallback (w, XmNmodifyVerifyCallback, (XtCallbackProc)
	    modifyCB, (XtPointer)this);
	XmTextPosition tpos = XmTextGetInsertionPosition (w);
	XmTextSetString (w, value);
	XmTextSetInsertionPosition (w, tpos);
	XtAddCallback (w, XmNmodifyVerifyCallback, (XtCallbackProc)
	    modifyCB, (XtPointer)this);
    }
}

boolean PostScriptImageFormat::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassPostScriptImageFormat);
    if (s == classname)
	return TRUE;
    else
	return this->ImageFormat::isA(classname);
}

extern "C" {

void PostScriptImageFormat_MarginCB (Widget , XtPointer clientData, XtPointer)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    ImageNode* node = pif->dialog->getNode();
    boolean fmtcon = node->isRecordFormatConnected();
    if (fmtcon == FALSE)
	pif->dirty&= ~PostScriptImageFormat::DirtyDpi;
    pif->setCommandActivation();
}

void PostScriptImageFormat_OrientCB (Widget w, XtPointer clientData, XtPointer)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);

    ImageNode* node = pif->dialog->getNode();
    boolean fmtcon = node->isRecordFormatConnected();

    //
    // Mark dpi as not dirty so that it can be set automatically.
    //
    if (w != pif->chosen_layout) {
	//if (pif->dialog->isRerenderAllowed())
	if (fmtcon == FALSE)
	    pif->dirty&= ~PostScriptImageFormat::DirtyDpi;
	if (w == pif->autoorient_button) 
	    pif->dirty&= ~PostScriptImageFormat::DirtyOrient;
	else
	    pif->dirty|= PostScriptImageFormat::DirtyOrient;

	pif->chosen_layout = w;
	pif->setCommandActivation();
    }
}

void PostScriptImageFormat_DpiCB (Widget , XtPointer clientData, XtPointer)
{
int dots_per;

    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    pif->dirty|= PostScriptImageFormat::DirtyDpi;
    pif->fifo_bits.push(PostScriptImageFormat::DirtyDpi, &pif->dirty);
    XtVaGetValues (pif->dpi_number, XmNiValue, &dots_per, NULL);

    if (pif->isMetric()) {
	double d = dots_per * CM_PER_INCH;
	pif->dpi = (int)d;
	if ((d-pif->dpi) > 0.0) pif->dpi++;
    } else {
	pif->dpi = dots_per;
    }

    boolean need_reset = TRUE;
    if (pif->dialog->isRerenderAllowed()) {
	//
	// if rerender is allowed then just reset image dimensions ...
	//
	if ((pif->entered_dims_valid == FALSE) || 
	    ((pif->dirty & PostScriptImageFormat::DirtyImageSize) == 0)) {
	    need_reset = FALSE;
	    double width = (double)pif->width / (double)pif->dpi;
	    double height = width * pif->aspect;
	    if (pif->isMetric()) {
		width*= CM_PER_INCH;
		height*= CM_PER_INCH;
	    } 

	    if (!pif->setVerifiedSizeTextString (width, height)) 
		need_reset = TRUE;
	
	//
	// ...else change the input image size in pixels.
	//
	} else {
	    double new_resolution = pif->entered_printout_width * pif->dpi;
	    pif->width = (int)new_resolution;
	    if ((new_resolution - pif->width) > 0.5) pif->width++;
	    pif->dirty&= ~PostScriptImageFormat::DirtyResolution;
	}
    } else {
	pif->entered_dims_valid = FALSE;
    }
    if (need_reset) pif->setCommandActivation();
}

void PostScriptImageFormat_ModifyCB (Widget w, XtPointer clientData, XtPointer)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    if (pif->size_timer)
	XtRemoveTimeOut (pif->size_timer);
    pif->size_timer = 0;

    if (w == pif->pixel_size_text)
	pif->size_timer = XtAppAddTimeOut (theApplication->getApplicationContext(),
	    2000, (XtTimerCallbackProc)PostScriptImageFormat_PixelSizeTO, (XtPointer)pif);
    else
	pif->size_timer = XtAppAddTimeOut (theApplication->getApplicationContext(),
	    2000, (XtTimerCallbackProc)PostScriptImageFormat_SizeTO, (XtPointer)pif);
}

void PostScriptImageFormat_ParseSizeEH (Widget w, XtPointer clientData, XEvent*, Boolean*)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    if (!pif->size_timer) return;
    PostScriptImageFormat_ParseSizeCB(w, clientData, (XtPointer)NULL);
}

void PostScriptImageFormat_ParseSizeCB (Widget w, XtPointer clientData, XtPointer)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    int dirt_history;
    if (pif->size_timer)
	XtRemoveTimeOut (pif->size_timer);
    pif->size_timer = 0;
    char *str = XmTextGetString(w);

    if (w == pif->pixel_size_text) 
	dirt_history = PostScriptImageFormat::DirtyResolution;
    else 
	dirt_history = PostScriptImageFormat::DirtyImageSize;
    pif->fifo_bits.push(dirt_history, &pif->dirty);

    if (w == pif->pixel_size_text) 
	pif->parseImageSize (str);
    else 
	pif->parsePictureSize (str);
    pif->setCommandActivation();
    if (str) XtFree(str);
}

void PostScriptImageFormat_SizeTO (XtPointer clientData, XtIntervalId* )
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    //char *str = XmTextGetString(pif->size_text);
    pif->size_timer = 0;
    //pif->parsePictureSize (str);
    //pif->setCommandActivation();
    //if (str) XtFree(str);
}

void PostScriptImageFormat_PixelSizeTO (XtPointer clientData, XtIntervalId* )
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    //char *str = XmTextGetString(pif->pixel_size_text);
    pif->size_timer = 0;
    //pif->parseImageSize (str);
    //pif->setCommandActivation();
    //if (str) XtFree(str);
}

void PostScriptImageFormat_PageModifyCB (Widget , XtPointer clientData, XtPointer)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    if (pif->page_size_timer)
	XtRemoveTimeOut (pif->page_size_timer);
    pif->page_size_timer = 0;
    pif->page_size_timer = XtAppAddTimeOut (theApplication->getApplicationContext(),
	2000, (XtTimerCallbackProc)PostScriptImageFormat_PageSizeTO, (XtPointer)pif);
    pif->dirty|= PostScriptImageFormat::DirtyPageSize;
    pif->dirty&= ~PostScriptImageFormat::DirtyDpi;
}

void PostScriptImageFormat_ParsePageSizeCB (Widget w, XtPointer clientData, XtPointer)
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    if (pif->page_size_timer)
	XtRemoveTimeOut (pif->page_size_timer);
    pif->page_size_timer = 0;
    char *str = XmTextGetString(w);
    pif->parsePageSize (str);
    pif->setCommandActivation();
    if (str) XtFree(str);
}

void PostScriptImageFormat_PageSizeTO (XtPointer clientData, XtIntervalId* )
{
    PostScriptImageFormat* pif = (PostScriptImageFormat*)clientData;
    ASSERT(pif);
    char *str = XmTextGetString(pif->page_size_text);
    pif->page_size_timer = 0;
    pif->parsePageSize (str);
    pif->setCommandActivation();
    if (str) XtFree(str);
}

} // end extern C


