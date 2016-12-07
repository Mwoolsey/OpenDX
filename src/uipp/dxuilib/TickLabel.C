/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>

#include "TickLabel.h"
#include "../widgets/Number.h"
#include "DXStrings.h"
#include "DXApplication.h"

String TickLabel::DefaultResources[] = {
    "*tlNumber.topOffset:		6",
    "*tlNumber.bottomOffset:		5",
    "*tlNumber.leftOffset:		6",
    "*tlLabel.topOffset:		3",
    "*tlLabel.leftOffset:		4",
    "*tlLabel.rightOffset:		6",
    "*tlLabel.bottomOffset:		3",
    NULL
};
#define MAX_TICK_LABEL_LEN 64

boolean TickLabel::ClassInitialized = FALSE;

TickLabel::TickLabel (double dval, const char *str, int pos, 
TickSelectCB tscb, void *clientData) :
	UIComponent("tickLabel")
{
    this->dval = dval;
    this->str = DuplicateString(str);
    this->pos = pos;

    this->dirty = 0; 
    this->number = NUL(Widget);
    this->text = NUL(Widget);
    this->form = NUL(Widget);

    this->selected = FALSE;
    this->tscb = tscb;
    this->clientData = clientData;
    this->ignoreVerifyCallback = FALSE;
}

void TickLabel::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT TickLabel::ClassInitialized) {
	this->setDefaultResources(theApplication->getRootWidget(),
	    TickLabel::DefaultResources);
	TickLabel::ClassInitialized = TRUE;
    }
}

TickLabel::~TickLabel()
{
    if (this->str) delete this->str;
}

void
TickLabel::destroyLine()
{
    this->getText();

    //
    // It should not be necessary to setRootWidget to NULL, but if you don't,
    // you have to wait for libXt to process the destroyCallback so that UIComponent
    // can do it for you.  I want it to be NULL right away.
    //
    Widget w = this->getRootWidget();
    XtDestroyWidget(w);

    this->number = NUL(Widget);
    this->form = NUL(Widget);
    this->text = NUL(Widget);
}

void
TickLabel::createLine (Widget parent)
{
    if (this->number) return ;

    this->initialize();

    Widget form = this->form = XtVaCreateManagedWidget(this->name,
	xmFormWidgetClass,	parent,
	XmNshadowThickness,	1,
	XmNshadowType,		XmSHADOW_IN,
	XmNpositionIndex,	(this->pos - 1),
    NULL);

    XtAddEventHandler (form, ButtonPressMask, False,
	(XtEventHandler)TickLabel_SelectEH, (XtPointer)this);

#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1, dx_l4;
#endif
    double inc = 0.1;
    this->number = XtVaCreateManagedWidget("tlNumber",
	xmNumberWidgetClass,	form,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNdataType, 		DOUBLE,
	XmNdecimalPlaces, 	3,
	XmNcharPlaces,		9,
	XmNrecomputeSize,	False,
	XmNfixedNotation,	False,
	XmNdValue, 		DoubleVal(this->dval, dx_l4),
	XmNdValueStep,	 	DoubleVal(inc, dx_l1),
    NULL);
    XtAddCallback (this->number, XmNactivateCallback,
	(XtCallbackProc)TickLabel_NumberCB, (XtPointer)this);
    XtAddCallback (this->number, XmNactivateCallback,
	(XtCallbackProc)TickLabel_NumberArmCB, (XtPointer)this);

    this->text = XtVaCreateManagedWidget ("tlLabel",
	xmTextFieldWidgetClass,	form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->number,
	XmNmaxLength,		MAX_TICK_LABEL_LEN,
    NULL);
    XtAddEventHandler (this->text, ButtonPressMask, False,
	(XtEventHandler)TickLabel_SelectEH, (XtPointer)this);

    this->ignoreVerifyCallback = TRUE;
    if (this->str) XmTextFieldSetString (this->text, this->str);
    else XmTextFieldSetString (this->text, "");
    this->ignoreVerifyCallback = FALSE;
    XtAddCallback (this->text, XmNmodifyVerifyCallback,
	(XtCallbackProc)TickLabel_TextModifyCB, (XtPointer)this);

    this->setRootWidget(form);
    this->selected = TRUE;
    this->setSelected (FALSE, FALSE);
}

const char *
TickLabel::getText()
{
    if ((this->text) && (this->dirty & DIRTY_TICK_TEXT)) {
	if (this->str) delete this->str;
	char *cp = XmTextFieldGetString (this->text);
	this->str = DuplicateString(cp);
	XtFree(cp);
    }
    return this->str;
}

void 
TickLabel::setText (const char *str) 
{
    if (this->str) delete this->str;
    this->str = DuplicateString(str);
    this->ignoreVerifyCallback = TRUE;
    if (this->text) {
	if (this->str)
	    XmTextFieldSetString (this->text, this->str);
	else
	    XmTextFieldSetString (this->text, "");
    }
    this->setText();
    this->ignoreVerifyCallback = FALSE;
}

void
TickLabel::setNumber (double dval)
{
#ifdef PASSDOUBLEVALUE
    XtArgVal    dxl;
#endif

    this->dval = dval;
    if (this->number) 
	XtVaSetValues (this->number, XmNdValue, DoubleVal(this->dval, dxl), NULL);
    this->setNumber();
}

void
TickLabel::setSelected (boolean set, boolean callCallback)
{
Pixel bg,ts,bs,fg;
boolean changed = (set != this->selected);

    this->selected = set;
    if (!this->form) return ;
    XtVaGetValues (this->text, 
	XmNbackground, 		&bg,
	XmNforeground, 		&fg,
	XmNtopShadowColor, 	&ts,
	XmNbottomShadowColor,	&bs,
    NULL);

    if (this->selected) {
	XtVaSetValues (this->form,
	    XmNtopShadowColor,	  fg,
	    XmNbottomShadowColor, fg,
	NULL);
    } else {
	XtVaSetValues (this->form,
	    XmNtopShadowColor,	  bg,
	    XmNbottomShadowColor, bg,
	NULL);
    }

    if ((callCallback) && (changed))  this->tscb(this, this->clientData);
}


extern "C" {
void TickLabel_SelectEH(Widget , XtPointer clientData,
                                        XEvent *, Boolean *)
{
    TickLabel *tlab = (TickLabel *)clientData;
    ASSERT(tlab);
    tlab->setSelected(TRUE, TRUE);
}


void TickLabel_DeleteOneCB(Widget , XtPointer clientData, XtPointer)
{
    TickLabel *tlab = (TickLabel *)clientData;
    ASSERT(tlab);
}
void TickLabel_AppendOneCB(Widget , XtPointer clientData, XtPointer)
{
    TickLabel *tlab = (TickLabel *)clientData;
    ASSERT(tlab);
}

void TickLabel_NumberArmCB(Widget , XtPointer clientData, XtPointer)
{
    TickLabel *tlab = (TickLabel *)clientData;
    ASSERT(tlab);
    tlab->setSelected(!tlab->selected, TRUE);
}

void TickLabel_NumberCB(Widget , XtPointer clientData, XtPointer)
{
    TickLabel *tlab = (TickLabel *)clientData;
    ASSERT(tlab);

    XtVaGetValues (tlab->number, XmNdValue, &tlab->dval, NULL);
    tlab->dirty|= DIRTY_TICK_NUMBER;
}

void TickLabel_TextModifyCB(Widget , XtPointer clientData, XtPointer cbs)
{
    TickLabel *tlab = (TickLabel *)clientData;
    ASSERT(tlab);
    XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct*)cbs;

    if ((tlab->str) && (tlab->ignoreVerifyCallback == FALSE)) {
	delete tlab->str;
 	tlab->str = NULL;
    }
    
    tlab->dirty|= DIRTY_TICK_TEXT;
    tvcs->doit = True;
}

} // extern "C"
