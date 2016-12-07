/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <Xm/DialogS.h>
#include <Xm/DrawP.h>

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#include "VCRControl.h"
#include "VCRControlP.h"
#include "backward.bm"
#include "forward.bm"
/* #include "frame.bm" */
#include "loop.bm"
#include "palim.bm"
#include "pause.bm"
#include "step.bm"
#include "stepb.bm"
#include "stepf.bm"
#include "stop.bm"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif



extern void _XmForegroundColorDefault();
extern void _XmBackgroundColorDefault();

static XtResource resources[] =
    {
        {
            XmNactionCallback,XmCActionCallback, XmRCallback, sizeof(XtPointer),
            XtOffset(XmVCRControlWidget, vcr_control.action_callback),
            XmRCallback, NULL
        },
        {
            XmNframeCallback, XmCFrameCallback, XmRCallback, sizeof(XtPointer),
            XtOffset(XmVCRControlWidget, vcr_control.frame_callback),
            XmRCallback, NULL
        },
        {
            XmNstart, XmCStart, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.start_value),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNnext, XmCNext, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.next_value),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNcurrent, XmCCurrent, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.current_value),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNcurrentVisible,XmCCurrentVisible,XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.current_visible),
            XmRImmediate, (XtPointer) True
        },
        {
            XmNstop, XmCStop, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.stop_value),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNminimum, XmCMinimum, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.min_value),
            XmRImmediate, (XtPointer) 1
        },
        {
            XmNmaximum, XmCMaximum, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.max_value),
            XmRImmediate, (XtPointer) 10
        },
        {
            XmNincrement, XmCIncrement, XmRShort, sizeof(short),
            XtOffset(XmVCRControlWidget, vcr_control.frame_increment),
            XmRImmediate, (XtPointer) 1
        },
        {
            XmNforwardButtonState, XmCForwardButtonState, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.button_is_in[VCR_FORWARD]),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNbackwardButtonState,XmCBackwardButtonState,XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.button_is_in[VCR_BACKWARD]),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNloopButtonState, XmCLoopButtonState, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.button_is_in[VCR_LOOP]),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNpalindromeButtonState, XmCPalindromeButtonState, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.button_is_in[VCR_PALINDROME]),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNstepButtonState, XmCStepButtonState, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.button_is_in[VCR_STEP]),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNcountButtonState, XmCCountButtonState, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.button_is_in[VCR_COUNT]),
            XmRImmediate, (XtPointer) 0
        },
        {
            XmNlimitColor, XmCLimitColor, XmRPixel, sizeof(Pixel),
            XtOffset(XmVCRControlWidget, vcr_control.limit_color),
            XmRCallProc, (XtPointer) _XmForegroundColorDefault
        },
        {
            XmNnextColor, XmCNextColor, XmRPixel, sizeof(Pixel),
            XtOffset(XmVCRControlWidget, vcr_control.next_color),
            XmRCallProc, (XtPointer) _XmForegroundColorDefault
        },
        {
            XmNcurrentColor, XmCCurrentColor, XmRPixel, sizeof(Pixel),
            XtOffset(XmVCRControlWidget, vcr_control.current_color),
            XmRCallProc, (XtPointer) _XmForegroundColorDefault
        },
        {
            XmNframeSensitive, XmCFrameSensitive, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.frame_sensitive),
            XmRImmediate, (XtPointer) TRUE
        },
        {
            XmNminSensitive, XmCMinSensitive, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.min_sensitive),
            XmRImmediate, (XtPointer) TRUE
        },
        {
            XmNmaxSensitive, XmCMaxSensitive, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.max_sensitive),
            XmRImmediate, (XtPointer) TRUE
        },
        {
            XmNincSensitive, XmCIncSensitive, XmRBoolean, sizeof(Boolean),
            XtOffset(XmVCRControlWidget, vcr_control.inc_sensitive),
            XmRImmediate, (XtPointer) TRUE
        },
    };


/*  Maintain actions support for basic manager functioning  */
static char defaultTranslations[] =
    "<EnterWindow>:  Enter() \n\
    <FocusIn>:      FocusIn()";

extern void _XmManagerEnter();
extern void _XmManagerFocusIn();

static XtActionsRec actionsList[] =
    {
        { "Enter",    (XtActionProc) _XmManagerEnter },
        { "FocusIn",  (XtActionProc) _XmManagerFocusIn },
    };


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

static void Initialize(XmVCRControlWidget request, XmVCRControlWidget new);
static Boolean SetValues(XmVCRControlWidget current,
                         XmVCRControlWidget request,
                         XmVCRControlWidget new);
static void Destroy(XmVCRControlWidget w);
static void MyRealize(XmVCRControlWidget w, XtValueMask *valueMask,
                      XSetWindowAttributes *attributes);
static void SimulateButtonPress(XmVCRControlWidget vcr, int button_num);

XmVCRControlClassRec xmVCRControlClassRec =
    {
        {			/* core_class fields      */
            (WidgetClass) &xmFormClassRec,		/* superclass         */
            "XmVCRControl",				/* class_name         */
            sizeof(XmVCRControlRec),			/* widget_size        */
            NULL,					/* class_initialize   */
            NULL,					/* class_part_init    */
            FALSE,					/* class_inited       */
            (XtInitProc) Initialize,			/* initialize         */
            NULL,					/* initialize_hook    */
            (XtRealizeProc)MyRealize,			/* realize            */
            actionsList,				/* actions	      */
            XtNumber(actionsList),			/* num_actions	      */
            resources,				/* resources          */
            XtNumber(resources),			/* num_resources      */
            NULLQUARK,				/* xrm_class          */
            TRUE,					/* compress_motion    */
            TRUE,					/* compress_exposure  */
            TRUE,					/* compress_enterlv   */
            FALSE,					/* visible_interest   */
            (XtWidgetProc) Destroy,			/* destroy            */
            (XtWidgetProc) XtInheritResize,			/* resize             */
            (XtExposeProc)_XtInherit,/*(XtExposeProc) Redisplay,*//* expose */
            (XtSetValuesFunc) SetValues,		/* set_values         */
            NULL,					/* set_values_hook    */
            (XtAlmostProc)_XtInherit,			/* set_values_almost  */
            NULL,					/* get_values_hook    */
            NULL,					/* accept_focus       */
            XtVersion,				/* version            */
            NULL,					/* callback_private   */
            defaultTranslations,			/* tm_table           */
            XtInheritQueryGeometry,			/* query_geometry     */
            NULL,             	                /* display_accelerator   */
            NULL,		                        /* extension             */
        },

        {		/* composite_class fields */
            XtInheritGeometryManager,			/* geometry _manager  */
            XtInheritChangeManaged,			/* change_managed     */
            (XtWidgetProc)_XtInherit,			/* insert_child       */
            XtInheritDeleteChild,			/* delete_child       */
            NULL,                                     /* extension          */
        },

        {		/* constraint_class fields */
            NULL,					/* resource list        */
            0,					/* num resources        */
            sizeof(XmFormConstraintRec),		/* constraint size      */
            NULL,					/* init proc            */
            NULL,					/* destroy proc         */
            NULL,					/* set values proc      */
            NULL,                                     /* extension            */
        },

        {		/* manager_class fields */
#if (XmVersion >= 1001)
            XtInheritTranslations,     		/* translations           */
#else
            (XtTranslations)_XtInherit,     		/* translations           */
#endif
            NULL,					/* get resources      	  */
            0,					/* num get_resources 	  */
            NULL,					/* get_cont_resources     */
            0,					/* num_get_cont_resources */
            (XmParentProcessProc)NULL,                /* parent_process         */
            NULL,					/* extension           */
        },

        {            /* bulletin board widget class */
            False,					/* install_accelarators */
            NULL,					/* install_accelarators */
            NULL,					/* install_accelarators */
            NULL,					/* extension           */
        },

        {            /* form widget class */
            NULL,					/* extension           */
        },

        {		/* vcr_control class - none */
            NULL,						/* mumble */
        }
    };

WidgetClass xmVCRControlWidgetClass = (WidgetClass) &xmVCRControlClassRec;

/*  Define interval of showing momentary button as depressed  */

#define SHORT_WAIT 150
#define LONG_WAIT 500
#define MAX_VAL(a,b) ((a)>(b)?(a):(b))
#define MIN_VAL(a,b) ((a)<(b)?(a):(b))
#define superclass (&widgetClassRec)

static void SetLabel();
static void CreateVCRCounter();
static void VCRButtonResize();
static void _VCRButtonEvent();
static void VCRButtonEvent();
static void VCRButtonExpose();
static void VCRCounterExpose();
static void ReleaseVCRButton();
static void PushVCRButton();
static void ReleaseModeButtons();
static void ReplaceFrame();
void ChangeVCRStopValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                         Boolean to_frame_control, Boolean to_outside );
void ChangeVCRStartValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                          Boolean to_frame_control, Boolean to_outside );
void ChangeVCRNextValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                         Boolean to_frame_control, Boolean to_outside );
void ChangeVCRCurrentValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                            Boolean to_frame_control, Boolean to_outside );
void ChangeVCRIncrementValue( XmVCRControlWidget vcr, int increment,
                              Boolean to_vcr, Boolean to_frame_control,
                              Boolean to_outside );
static void PopdownCallback();
static void map_dialog();
static void FrameControlCallback();

/*  Subroutine:	Initialize
 *  Effect:	Create and initialize the component widgets
 */

static void Initialize( XmVCRControlWidget request, XmVCRControlWidget new )
{
    Arg wargs[20];

    if (request->core.width == 0)
        new->core.width = 240;
    if (request->core.height == 0)
        new->core.height = 60;

    new->vcr_control.last_frame_control_press = -1;  /* Net pressed yet */

    new->vcr_control.parent = XtParent(new);

    new->vcr_control.button_timeout = (XtIntervalId)NULL;

    /*
     * Check min and max relative to each other
     */
    if( request->vcr_control.max_value < request->vcr_control.min_value )
        new->vcr_control.max_value = new->vcr_control.min_value;

    /*
     * Check current and next relative to min.
     */
    if( request->vcr_control.next_value < request->vcr_control.min_value )
        new->vcr_control.next_value = new->vcr_control.min_value;
    if( request->vcr_control.current_value < request->vcr_control.min_value )
        new->vcr_control.current_value = new->vcr_control.min_value;

    /*
     * Check current and next relative to max.
     */
    if( request->vcr_control.next_value > request->vcr_control.max_value )
        new->vcr_control.next_value = new->vcr_control.max_value;
    if( request->vcr_control.current_value > request->vcr_control.max_value )
        new->vcr_control.current_value = new->vcr_control.max_value;

    new->vcr_control.step_being_held = FALSE;
    new->vcr_control.frame_control_is_up = FALSE;

    /* Init all non-resource buttons to the up state */
    new->vcr_control.button_is_in[VCR_STOP] = FALSE;
    new->vcr_control.button_is_in[VCR_PAUSE] = FALSE;
    new->vcr_control.button_is_in[VCR_FORWARD_STEP] = FALSE;
    new->vcr_control.button_is_in[VCR_BACKWARD_STEP] = FALSE;


    /*  Loop  */
    XtSetArg(wargs[0], XmNleftAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[1], XmNrightAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[2], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(wargs[3], XmNbottomAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[4], XmNleftPosition, 0);
    XtSetArg(wargs[5], XmNrightPosition, 25);
    XtSetArg(wargs[6], XmNtopPosition, 0);
    XtSetArg(wargs[7], XmNbottomPosition, 50);
    XtSetArg(wargs[8], XmNshadowType, XmSHADOW_OUT);
    new->vcr_control.frame[VCR_LOOP] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "loop", wargs, 9);
    SetLabel(new, VCR_LOOP, loop_bits, loop_width, loop_height);

    /*  Palindrome  */
    XtSetArg(wargs[4], XmNleftPosition, 25);
    XtSetArg(wargs[5], XmNrightPosition, 50);
    new->vcr_control.frame[VCR_PALINDROME] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "palindrome", wargs, 9);
    SetLabel(new, VCR_PALINDROME, palim_bits, palim_width, palim_height);

    /*  Step  */
    XtSetArg(wargs[4], XmNleftPosition, 50);
    XtSetArg(wargs[5], XmNrightPosition, 75);
    new->vcr_control.frame[VCR_STEP] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "step", wargs, 9);
    SetLabel(new, VCR_STEP, step_bits, step_width, step_height);

    /*  Counter  */
    XtSetArg(wargs[4], XmNleftPosition, 75);
    XtSetArg(wargs[5], XmNrightPosition, 100);
    new->vcr_control.frame[VCR_COUNT] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "frame", wargs, 9);
    CreateVCRCounter(new);

    /*  Backward  */
    XtSetArg(wargs[2], XmNtopAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[4], XmNleftPosition, 0);
    XtSetArg(wargs[5], XmNrightPosition, 25);
    XtSetArg(wargs[6], XmNtopPosition, 50);
    XtSetArg(wargs[7], XmNbottomPosition, 100);
    new->vcr_control.frame[VCR_BACKWARD] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "backward", wargs, 9);
    SetLabel(new, VCR_BACKWARD, backward_bits, backward_width, backward_height);

    /*  Forward  */
    XtSetArg(wargs[4], XmNleftPosition, 25);
    XtSetArg(wargs[5], XmNrightPosition, 50);
    new->vcr_control.frame[VCR_FORWARD] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "forward", wargs, 9);
    SetLabel(new, VCR_FORWARD, forward_bits, forward_width, forward_height);

    /*  Stop  */
    XtSetArg(wargs[4], XmNleftPosition, 50);
    XtSetArg(wargs[5], XmNrightPosition, 62);
    new->vcr_control.frame[VCR_STOP] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "stop", wargs, 9);
    SetLabel(new, VCR_STOP, stop_bits, stop_width, stop_height);

    /*  Pause  */
    XtSetArg(wargs[4], XmNleftPosition, 62);
    XtSetArg(wargs[5], XmNrightPosition, 100);
    new->vcr_control.frame[VCR_PAUSE] =
        (XmFrameWidget) XmCreateFrame((Widget)new, "pause", wargs, 9);
    SetLabel(new, VCR_PAUSE, pause_bits, pause_width, pause_height);
    XtManageChildren((Widget*)new->vcr_control.frame, 8);

    /*  Forward Step  */
    new->vcr_control.frame[VCR_FORWARD_STEP] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "stepf", wargs, 9);
    SetLabel(new, VCR_FORWARD_STEP, stepf_bits, stepf_width, stepf_height);

    /*  Backward Step  */
    new->vcr_control.frame[VCR_BACKWARD_STEP] =
        (XmFrameWidget)XmCreateFrame((Widget)new, "stepb", wargs, 9);
    SetLabel(new, VCR_BACKWARD_STEP, stepb_bits, stepb_width, stepb_height);

    XtSetArg(wargs[0], XmNwidth, 400);
    XtSetArg(wargs[1], XmNheight, 180);
    XtSetArg(wargs[2], XmNdeleteResponse, XmUNMAP);
    XtSetArg(wargs[3], XmNallowShellResize, True);
    XtSetArg(wargs[4], XmNminWidth, 380);
    XtSetArg(wargs[5], XmNminHeight, 160);

    new->vcr_control.shell = XmCreateDialogShell((Widget)new, "Frame Control",
                             wargs, 6);

    XtSetArg(wargs[2], XmNstart, new->vcr_control.start_value);
    XtSetArg(wargs[3], XmNnext, new->vcr_control.next_value);
    XtSetArg(wargs[4], XmNcurrent, new->vcr_control.current_value);
    XtSetArg(wargs[5], XmNstop, new->vcr_control.stop_value);
    XtSetArg(wargs[6], XmNminimum, new->vcr_control.min_value);
    XtSetArg(wargs[7], XmNincrement, new->vcr_control.frame_increment);
    XtSetArg(wargs[8], XmNmaximum, new->vcr_control.max_value);
    XtSetArg(wargs[9], XmNnextColor, new->vcr_control.next_color);
    XtSetArg(wargs[10], XmNcurrentColor, new->vcr_control.current_color);
    XtSetArg(wargs[11], XmNlimitColor, new->vcr_control.limit_color);
    XtSetArg(wargs[12], XmNcurrentVisible, new->vcr_control.current_visible);
    XtSetArg(wargs[13], XmNdefaultPosition, False);
    new->vcr_control.frame_control = (XmFrameControlWidget)XmCreateFrameControl(
                                         (Widget)new->vcr_control.shell, "frame_control", wargs, 14);


    XtAddCallback((Widget)new->vcr_control.frame_control, XmNvalueCallback,
                  (XtCallbackProc)FrameControlCallback, new);
    XtAddCallback((Widget)new->vcr_control.shell, XmNpopdownCallback,
                  (XtCallbackProc)PopdownCallback, new);
    XtAddCallback((Widget)new->vcr_control.frame_control, XmNmapCallback,
                  (XtCallbackProc)map_dialog, new);
}

#undef MIN_VAL
#undef MAX_VAL
static void MyRealize(XmVCRControlWidget w, XtValueMask *valueMask,
                      XSetWindowAttributes *attributes)
{

    /* Call the superclass Realize */
    (*superclass->core_class.realize)((Widget)w, valueMask, attributes);
    if (w->vcr_control.button_is_in[VCR_COUNT]) {
        w->vcr_control.button_is_in[VCR_COUNT] = FALSE;
        SimulateButtonPress(w, VCR_COUNT);
    }
    XtRealizeWidget((Widget)w->vcr_control.frame[VCR_BACKWARD]);
    XtRealizeWidget((Widget)w->vcr_control.button[VCR_BACKWARD]);
    if (w->vcr_control.button_is_in[VCR_BACKWARD]) {
        w->vcr_control.button_is_in[VCR_BACKWARD] = FALSE;
        SimulateButtonPress(w, VCR_BACKWARD);
    }
    XtRealizeWidget((Widget)w->vcr_control.frame[VCR_FORWARD]);
    XtRealizeWidget((Widget)w->vcr_control.button[VCR_FORWARD]);
    if (w->vcr_control.button_is_in[VCR_FORWARD]) {
        w->vcr_control.button_is_in[VCR_FORWARD] = FALSE;
        SimulateButtonPress(w, VCR_FORWARD);
    }
    XtRealizeWidget((Widget)w->vcr_control.frame[VCR_LOOP]);
    XtRealizeWidget((Widget)w->vcr_control.button[VCR_LOOP]);
    if (w->vcr_control.button_is_in[VCR_LOOP]) {
        w->vcr_control.button_is_in[VCR_LOOP] = FALSE;
        SimulateButtonPress(w, VCR_LOOP);
    }
    XtRealizeWidget((Widget)w->vcr_control.frame[VCR_STEP]);
    XtRealizeWidget((Widget)w->vcr_control.button[VCR_STEP]);
    if (w->vcr_control.button_is_in[VCR_STEP]) {
        w->vcr_control.button_is_in[VCR_STEP] = FALSE;
        SimulateButtonPress(w, VCR_STEP);
    }
    XtRealizeWidget((Widget)w->vcr_control.frame[VCR_PALINDROME]);
    XtRealizeWidget((Widget)w->vcr_control.button[VCR_PALINDROME]);
    if (w->vcr_control.button_is_in[VCR_PALINDROME]) {
        w->vcr_control.button_is_in[VCR_PALINDROME] = FALSE;
        SimulateButtonPress(w, VCR_PALINDROME);
    }
}
static void SimulateButtonPress(XmVCRControlWidget vcr, int button_num)
{
    Widget widget;
    XmDrawingAreaCallbackStruct call_data;

    call_data.event = (XEvent *)XtMalloc(sizeof(XEvent));

    call_data.event->type           = ButtonPress;
    call_data.event->xbutton.button = Button1;

    /*
     * Kludge to tell VCRButtonEvent not to callback application.
     */
    call_data.event->xbutton.x = -1;
    widget = (Widget)vcr->vcr_control.button[button_num];
    VCRButtonEvent(widget, vcr, &call_data);
    XtFree((char*)call_data.event);

}

/*  Subroutine: SetValues
 *  Effect:     Handles requests to change things from the application
*/

static Boolean SetValues(XmVCRControlWidget current,
                         XmVCRControlWidget request,
                         XmVCRControlWidget new)
{
    int i;
    Arg wargs[12];

    if (new->vcr_control.min_value > new->vcr_control.max_value) {
        new->vcr_control.min_value = new->vcr_control.max_value;
    };

    if (new->vcr_control.max_value < new->vcr_control.min_value) {
        new->vcr_control.max_value = new->vcr_control.min_value;
    };

    if (new->vcr_control.start_value > new->vcr_control.stop_value) {
        new->vcr_control.start_value = new->vcr_control.stop_value;
    };

    if (new->vcr_control.stop_value < new->vcr_control.start_value) {
        new->vcr_control.stop_value = new->vcr_control.start_value;
    };

    if (new->vcr_control.next_value < new->vcr_control.start_value) {
        new->vcr_control.next_value = new->vcr_control.start_value;
    };
#ifdef OBSOLETE

    if (new->vcr_control.current_value < new->vcr_control.start_value) {
        new->vcr_control.current_value = new->vcr_control.start_value;
    };
#endif

    if (new->vcr_control.next_value > (new->vcr_control.stop_value)) {
        new->vcr_control.next_value = new->vcr_control.stop_value;
    };
#ifdef OBSOLETE

    if (new->vcr_control.current_value > (new->vcr_control.stop_value)) {
        new->vcr_control.current_value = new->vcr_control.stop_value;
    };
#endif

    if (new->vcr_control.frame_increment > (new->vcr_control.stop_value -
                                            new->vcr_control.start_value)) {
        new->vcr_control.frame_increment = new->vcr_control.stop_value -
                                           new->vcr_control.start_value;
    };

    for(i = 0; i < VCR_STOP; i++)	/* To first non-resource button */
    {
        if (current->vcr_control.button_is_in[i] !=
                request->vcr_control.button_is_in[i])
        {
            new->vcr_control.button_is_in[i] =
                current->vcr_control.button_is_in[i];
            SimulateButtonPress(new, i);
            XFlush(XtDisplay(new));
        }
    }
    if (current->vcr_control.min_value !=
            new->vcr_control.min_value) {
        XtSetArg(wargs[0], XmNminimum, new->vcr_control.min_value);
        XtSetValues((Widget)new->vcr_control.frame_control, wargs, 1);
        if (new->vcr_control.min_value > new->vcr_control.next_value)
            ChangeVCRNextValue(new, new->vcr_control.min_value,
                               TRUE, FALSE, TRUE);
    }
    if (current->vcr_control.max_value !=
            new->vcr_control.max_value) {
        XtSetArg(wargs[0], XmNmaximum, new->vcr_control.max_value);
        XtSetValues((Widget)new->vcr_control.frame_control, wargs, 1);
        if (new->vcr_control.max_value < new->vcr_control.next_value)
            ChangeVCRNextValue(new, new->vcr_control.max_value,
                               TRUE, FALSE, TRUE);
    }
    if (current->vcr_control.start_value !=
            request->vcr_control.start_value) {
        ChangeVCRStartValue(new, request->vcr_control.start_value,
                            TRUE, TRUE, FALSE);
    }
    if (current->vcr_control.stop_value !=
            request->vcr_control.stop_value) {
        ChangeVCRStopValue(new, request->vcr_control.stop_value,
                           TRUE, TRUE, FALSE);
    }
    if (current->vcr_control.next_value !=
            request->vcr_control.next_value) {
        ChangeVCRNextValue(new, new->vcr_control.next_value,
                           TRUE, TRUE, FALSE);
    }
    if (current->vcr_control.current_value !=
            request->vcr_control.current_value) {
        ChangeVCRCurrentValue(new, new->vcr_control.current_value,
                              TRUE, TRUE, FALSE);
    }
    if (current->vcr_control.frame_increment !=
            request->vcr_control.frame_increment) {
        ChangeVCRIncrementValue(new, new->vcr_control.frame_increment,
                                TRUE, TRUE, FALSE);
    }
    if (current->vcr_control.next_color !=
            new->vcr_control.next_color) {
        XtSetArg(wargs[0],XmNnextColor, new->vcr_control.next_color);
        XtSetValues((Widget)new->vcr_control.frame_control, wargs, 1);
    }
    if (current->vcr_control.current_color !=
            new->vcr_control.current_color) {
        XtSetArg(wargs[0],XmNcurrentColor, new->vcr_control.current_color);
        XtSetValues((Widget)new->vcr_control.frame_control, wargs, 1);
    }
    if (current->vcr_control.limit_color !=
            new->vcr_control.limit_color) {
        XtSetArg(wargs[0], XmNlimitColor, new->vcr_control.limit_color);
        XtSetValues((Widget)new->vcr_control.frame_control, wargs, 1);
    }
    if (current->vcr_control.frame_sensitive !=
            new->vcr_control.frame_sensitive) {
        XtSetSensitive((Widget)new->vcr_control.frame_control,
                       new->vcr_control.frame_sensitive);
    }
    if (current->vcr_control.min_sensitive !=
            new->vcr_control.min_sensitive) {
        XtVaSetValues((Widget)new->vcr_control.frame_control,
                      XmNminSensitive, new->vcr_control.min_sensitive, NULL);
    }
    if (current->vcr_control.max_sensitive !=
            new->vcr_control.max_sensitive) {
        XtVaSetValues((Widget)new->vcr_control.frame_control,
                      XmNmaxSensitive, new->vcr_control.max_sensitive, NULL);
    }
    if (current->vcr_control.inc_sensitive !=
            new->vcr_control.inc_sensitive) {
        XtVaSetValues((Widget)new->vcr_control.frame_control,
                      XmNincSensitive, new->vcr_control.inc_sensitive, NULL);
    }
    if (current->vcr_control.current_visible !=
            new->vcr_control.current_visible) {
        ChangeVCRCurrentValue(new, new->vcr_control.current_value,
                              TRUE, TRUE, FALSE);
    }

    return FALSE;
}

/*  Subroutine: Destroy
 *  Effect:     Frees resources before widget is destroyed
*/

static void Destroy(XmVCRControlWidget w)
{
    /* Free all the callbacks */
}

/*  Subroutine:	SetLabel
 *  Purpose:	Prepare the interactive part of a button and its iconic image
 *  Note:	The interaction is through a DrawingArea while the 3d frame
 *		is currently a Frame parent (only because the DrawingArea
 *		doesn't want to draw its own shadow).  Since we are already
 *		handling some of the Frame's shadow drawing, we may be able
 *		to do the same for a DrawingArea without frame).
 */

static void SetLabel( XmVCRControlWidget vcr,
                      int i, char* bits, int width, int height )
{
    Arg wargs[2];
    unsigned long fg, bg;
    Display *display;

    vcr->vcr_control.button[i] =
        (XmDrawingAreaWidget) XmCreateDrawingArea((Widget)vcr->vcr_control.frame[i],
                "label", wargs, 0);
    display = XtDisplay(vcr->vcr_control.button[i]);
    fg = vcr->vcr_control.button[i]->manager.foreground;
    bg = vcr->vcr_control.button[i]->core.background_pixel;
    vcr->vcr_control.button_image[i].width = width;
    vcr->vcr_control.button_image[i].height = height;
    vcr->vcr_control.button_image[i].gc = NULL;
    vcr->vcr_control.button_image[i].pixid =
        XCreatePixmapFromBitmapData(display,
                                    XRootWindowOfScreen(XtScreen(vcr)), bits, width, height,
                                    fg,bg,DefaultDepth(display,XScreenNumberOfScreen(XtScreen(vcr))));
    vcr->vcr_control.button[i]->manager.user_data =
        (void *)(&vcr->vcr_control.button_image[i]);

    XtAddCallback((Widget)vcr->vcr_control.button[i], XmNexposeCallback,
                  VCRButtonExpose, vcr);
    XtAddCallback((Widget)vcr->vcr_control.button[i], XmNresizeCallback,
                  VCRButtonResize, vcr);
    XtAddCallback((Widget)vcr->vcr_control.button[i], XmNinputCallback,
                  VCRButtonEvent, vcr);
    XtManageChild((Widget)vcr->vcr_control.button[i]);
}

/*  Subroutine:	CreateVCRCounter
 *  Purpose:	Prepare the frame number display and the button which brings
 *		up the frame_control.  This button is different from the others.
 */
static void CreateVCRCounter( XmVCRControlWidget vcr )
{
    Arg                  wargs[16];
    XmString             text;
    char                 string[100];
    Dimension            width;

    vcr->vcr_control.button[VCR_COUNT] = (XmDrawingAreaWidget)
                                         XmCreateDrawingArea((Widget)vcr->vcr_control.frame[VCR_COUNT],
                                                             "label", wargs, 0);
    XtManageChild((Widget)vcr->vcr_control.button[VCR_COUNT]);
    XtAddCallback((Widget)vcr->vcr_control.button[VCR_COUNT], XmNexposeCallback,
                  VCRCounterExpose, vcr);
    XtAddCallback((Widget)vcr->vcr_control.button[VCR_COUNT], XmNinputCallback,
                  VCRButtonEvent, vcr);

    /* Create the label */
    XtSetArg(wargs[0], XmNx, 10);
    XtSetArg(wargs[1], XmNy, 15);
    /* Leave room for 5 digits */
    sprintf(string, "55555");
    text = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[2], XmNlabelString, text);
    XtSetArg(wargs[3], XmNrecomputeSize, False);
    vcr->vcr_control.label = (XmLabelWidget)XmCreateLabel(
                                 (Widget)vcr->vcr_control.button[VCR_COUNT],
                                 "label", wargs, 4);
    XtManageChild((Widget)vcr->vcr_control.label);
    XmStringFree(text);

    /*
     * Now give the label the real string to display
     */
    if (vcr->vcr_control.current_visible) {
        sprintf(string, "%5d", vcr->vcr_control.current_value);
    } else {
        sprintf(string, "     ");
    }
    text = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[0], XmNlabelString, text);
    XtSetValues((Widget)vcr->vcr_control.label, wargs, 1);
    XmStringFree(text);

    XtSetArg(wargs[0], XmNwidth, &width);
    XtGetValues((Widget)vcr->vcr_control.label, wargs, 1);

    text = XmStringCreate("...", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[0], XmNx, 20 + width);
    XtSetArg(wargs[1], XmNy, 15);
    XtSetArg(wargs[2], XmNlabelString, text);
    vcr->vcr_control.dot_dot_dot =
        (XmLabelWidget)XmCreateLabel((Widget)vcr->vcr_control.button[VCR_COUNT],
                                     "label", wargs,3);
    XmStringFree(text);
    XtManageChild((Widget)vcr->vcr_control.dot_dot_dot);

    XtAddEventHandler((Widget)vcr->vcr_control.label, ButtonPressMask,
                      False, _VCRButtonEvent, vcr);
    XtAddEventHandler((Widget)vcr->vcr_control.dot_dot_dot, ButtonPressMask,
                      False, _VCRButtonEvent, vcr);
}


/*  Subroutine:	VCRButtonResize
 *  Purpose:	Clear the old image and treat the resize as a new exposure
 */
static void VCRButtonResize( XmDrawingAreaWidget w, XmVCRControlWidget vcr,
                             XmDrawingAreaCallbackStruct* cb )
{
    if( XtIsRealized((Widget)w) ) {
        XClearWindow(XtDisplay(w), XtWindow(w));
        VCRButtonExpose(w, vcr, cb);
    }
}

/*  Subroutine:	VCRButtonExpose
 *  Purpose:	Draw the iconic image in the button (it is usually attached
 *		as user_data, but there may be exceptions).
 */
static void VCRButtonExpose( XmDrawingAreaWidget w, XmVCRControlWidget vcr,
                             XmDrawingAreaCallbackStruct* cb )
{
    int x, y;
    ButtonImage *image_data = (ButtonImage *)(w->manager.user_data);
    Display *display;
    Window window;

    if(   (image_data->background != w->core.background_pixel)
            && image_data->gc ) {
        XtReleaseGC((Widget)w, image_data->gc);
        image_data->gc = NULL;
    }
    if( image_data->gc == NULL ) {
        XGCValues	values;
        XtGCMask	valueMask;
        valueMask = GCForeground | GCBackground | GCFunction;
        values.foreground = w->manager.foreground;
        values.background = w->core.background_pixel;
        values.function = GXcopy;
        image_data->gc = XtGetGC((Widget)w, valueMask, &values);
        image_data->background = w->core.background_pixel;
    }
    x = (w->core.width - image_data->width) / 2;
    y = (w->core.height - image_data->height) / 2;
    display = XtDisplay(w);
    if (XtIsRealized((Widget)w)) {
        window = XtWindow(w);
        XCopyArea(display, image_data->pixid, window, image_data->gc, 0, 0,
                  image_data->width, image_data->height, x, y);
    }
}


/*  Subroutine:	VCRCounterExpose
 *  Purpose:	Center the contents of the counter button
 */
static void VCRCounterExpose( XmDrawingAreaWidget w, XmVCRControlWidget vcr,
                              XmDrawingAreaCallbackStruct* cb )
{
    int x1, x2, y;
    x1 = vcr->vcr_control.button[VCR_COUNT]->core.width
         - (vcr->vcr_control.label->core.width * 2);
    if( x1 > 16 )
        x1 /= 4;
    else if( x1 > 4 )
        x1 = 4;
    else
        x1 = 0;
    y = (vcr->vcr_control.button[VCR_COUNT]->core.height
         - vcr->vcr_control.label->core.height) / 2;
    if( (x1 != vcr->vcr_control.label->core.x) ||
            (y != vcr->vcr_control.label->core.y) ) {
        RectObj g = (RectObj) vcr->vcr_control.label;
        XmDropSiteStartUpdate((Widget)vcr->vcr_control.label);
        XtMoveWidget((Widget)g, x1, y);
    }
    x1 += vcr->vcr_control.label->core.width;
    x2 = x1 + ((vcr->vcr_control.button[VCR_COUNT]->core.width -
                (x1 + vcr->vcr_control.dot_dot_dot->core.width)) / 2);
    y = (vcr->vcr_control.button[VCR_COUNT]->core.height
         - vcr->vcr_control.dot_dot_dot->core.height) / 2;
    if(   (x2 != vcr->vcr_control.dot_dot_dot->core.x)
            || (y != vcr->vcr_control.dot_dot_dot->core.y) ) {
        /*  Move the Gadget (XtMoveWidget doesn't move Gadgets in R3)  */
        RectObj g = (RectObj) vcr->vcr_control.dot_dot_dot;
        XmDropSiteStartUpdate((Widget)vcr->vcr_control.dot_dot_dot);
        XtMoveWidget((Widget)g, x2, y);
    }
}

/*  Subroutine:	_VCRButtonEvent
 *  Purpose: In XtR4/Motif1.1, the button events did not get to the drawing
 *           area widget, so we will dispatch them on our own...	
 */
static void _VCRButtonEvent( Widget w, XmVCRControlWidget vcr,
                             XEvent *event, Boolean *continue_to_dispatch )
{
    XmDrawingAreaCallbackStruct call_data;

    call_data.event = event;
    VCRButtonEvent(vcr->vcr_control.button[VCR_COUNT], vcr, &call_data);
}
/*  Subroutine:	VCRButtonEvent
 *  Purpose:	Field button events from the Drawing Areas to make them behave
 *		like buttons (including all interactions among the buttons).
 */
static void VCRButtonEvent( XmDrawingAreaWidget w, XmVCRControlWidget vcr,
                            XmDrawingAreaCallbackStruct* call_data )
{
    VCRCallbackStruct data;

    if( call_data->event->type == ButtonPress &&
            call_data->event->xbutton.button == Button1 )
    {
        if ( w != vcr->vcr_control.button[VCR_COUNT] )
            vcr->vcr_control.last_frame_control_press = -1;

        if( w == vcr->vcr_control.button[VCR_FORWARD] ) {
            if( !vcr->vcr_control.button_is_in[VCR_FORWARD] ) {
                ReleaseModeButtons(vcr);
                PushVCRButton(vcr, VCR_FORWARD);

                if (call_data->event->xbutton.x != -1) {
                    data.action = VCR_FORWARD;
                    data.on     = TRUE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
                if(   (vcr->vcr_control.button_is_in[VCR_LOOP] == FALSE)
                        && (vcr->vcr_control.button_is_in[VCR_PALINDROME] == FALSE)
                        && (  (vcr->vcr_control.next_value +
                               vcr->vcr_control.frame_increment)
                              > vcr->vcr_control.stop_value) ) {
                    /*  Do not queue up additional step is step mode */
                    if( vcr->vcr_control.button_is_in[VCR_STEP] )
                        return;
                }
            }
            /*  If in, pressing releases (releases pause as well) exc. step  */
            else if( vcr->vcr_control.button_is_in[VCR_STEP] == FALSE ) {
                ReleaseVCRButton(vcr, VCR_FORWARD);


                if (call_data->event->xbutton.x != -1) {
                    data.action = VCR_PAUSE;
                    data.on     = TRUE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
            }
        } else if( w == vcr->vcr_control.button[VCR_BACKWARD] ) {
            /*  If out, unset forward, stop, pause  */
            if( !vcr->vcr_control.button_is_in[VCR_BACKWARD] ) {
                ReleaseModeButtons(vcr);
                PushVCRButton(vcr, VCR_BACKWARD);
                if (call_data->event->xbutton.x != -1) {
                    data.action = VCR_BACKWARD;
                    data.on     = TRUE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
                if(   (vcr->vcr_control.button_is_in[VCR_LOOP] == FALSE)
                        && (vcr->vcr_control.button_is_in[VCR_PALINDROME] == FALSE)
                        && (  (vcr->vcr_control.next_value -
                               vcr->vcr_control.frame_increment)
                              < vcr->vcr_control.start_value) ) {
                    /*  Do not queue up additional step is step mode */
                    if( vcr->vcr_control.button_is_in[VCR_STEP] )
                        return;
                }
            }
            /*  If in, pressing releases (releases pause as well) exc. step  */
            else if( vcr->vcr_control.button_is_in[VCR_STEP] == FALSE ) {
                ReleaseVCRButton(vcr, VCR_BACKWARD);

                if (call_data->event->xbutton.x != -1) {
                    data.action = VCR_PAUSE;
                    data.on     = TRUE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
            }
        } else if( w == vcr->vcr_control.button[VCR_PALINDROME] ) {
            if( vcr->vcr_control.button_is_in[VCR_PALINDROME] ) {
                ReleaseVCRButton(vcr, VCR_PALINDROME);
            } else {
                PushVCRButton(vcr, VCR_PALINDROME);
            }
            if (call_data->event->xbutton.x != -1) {
                data.action = VCR_PALINDROME;
                data.on     = vcr->vcr_control.button_is_in[VCR_PALINDROME];
                XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
            }
        } else if( w == vcr->vcr_control.button[VCR_STEP] ) {
            if( vcr->vcr_control.button_is_in[VCR_STEP] ) {
                /*  Call application first, then change the button state  */
                data.action = VCR_STEP;
                if (call_data->event->xbutton.x != -1) {
                    data.on     = FALSE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
                ReleaseVCRButton(vcr, VCR_STEP);
                /*  Switch the button icons  */
                vcr->vcr_control.button[VCR_FORWARD]->manager.user_data =
                    (void *)(&vcr->vcr_control.button_image[VCR_FORWARD]);
                vcr->vcr_control.button[VCR_BACKWARD]->manager.user_data =
                    (void *)(&vcr->vcr_control.button_image[VCR_BACKWARD]);
                /*  Clear old icons before drawing new ones  */
                XClearWindow(XtDisplay(vcr),
                             XtWindow(vcr->vcr_control.button[VCR_FORWARD]));
                XClearWindow(XtDisplay(vcr),
                             XtWindow(vcr->vcr_control.button[VCR_BACKWARD]));
            } else {
                /*  Call application first, then change the button state  */
                if (call_data->event->xbutton.x != -1) {
                    data.action = VCR_STEP;
                    data.on     = TRUE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
                PushVCRButton(vcr, VCR_STEP);
                if(   vcr->vcr_control.button_is_in[VCR_FORWARD]
                        || vcr->vcr_control.button_is_in[VCR_BACKWARD] ) {
                    /*  Invoking step during playing pauses action  */
                    if( vcr->vcr_control.button_is_in[VCR_FORWARD] )
                        ReleaseVCRButton(vcr, VCR_FORWARD);
                    else
                        ReleaseVCRButton(vcr, VCR_BACKWARD);
                }
                /*  Switch the button icons  */
                vcr->vcr_control.button[VCR_FORWARD]->manager.user_data =
                    (void *)(&vcr->vcr_control.button_image[VCR_FORWARD_STEP]);
                vcr->vcr_control.button[VCR_BACKWARD]->manager.user_data =
                    (void *)(&vcr->vcr_control.button_image[VCR_BACKWARD_STEP]);
                /*  No need to clear since the step icon is bigger  */
            }
            /*  Redraw the Button icon  */
            VCRButtonExpose(vcr->vcr_control.button[VCR_FORWARD], vcr, NULL);
            VCRButtonExpose(vcr->vcr_control.button[VCR_BACKWARD], vcr, NULL);
        } else if( w == vcr->vcr_control.button[VCR_LOOP] ) {
            if( vcr->vcr_control.button_is_in[VCR_LOOP] )
                ReleaseVCRButton(vcr, VCR_LOOP);
            else
                PushVCRButton(vcr, VCR_LOOP);
            if (call_data->event->xbutton.x != -1) {
                data.action = VCR_LOOP;
                data.on     = vcr->vcr_control.button_is_in[VCR_LOOP];
                XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
            }
        } else if( w == vcr->vcr_control.button[VCR_STOP] ) {
            if( !vcr->vcr_control.button_is_in[VCR_STOP] ) {
                ReleaseModeButtons(vcr);
                if( vcr->vcr_control.button_is_in[VCR_PAUSE] )
                    ReleaseVCRButton(vcr, VCR_PAUSE);
                PushVCRButton(vcr, VCR_STOP);
                if (call_data->event->xbutton.x != -1) {
                    data.action = VCR_STOP;
                    data.on     = TRUE;
                    XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
                }
            }
        } else if( w == vcr->vcr_control.button[VCR_PAUSE] ) {
            /*  This only happens if pause is a toggle  */
            if( vcr->vcr_control.button_is_in[VCR_PAUSE] )
                ReleaseVCRButton(vcr, VCR_PAUSE);
            else {
                ReleaseModeButtons(vcr);
                PushVCRButton(vcr, VCR_PAUSE);
            }
            if (call_data->event->xbutton.x != -1) {
                data.action = VCR_PAUSE;
                data.on     = TRUE;
                XtCallCallbacks((Widget)vcr, XmNactionCallback, &data);
            }
        } else if( w == vcr->vcr_control.button[VCR_COUNT] ) {
            /*
             * Ignore the second click of a double click on this button.
             */
            if((vcr->vcr_control.last_frame_control_press != -1) &&
                    (abs(vcr->vcr_control.last_frame_control_press -
                         call_data->event->xbutton.time) < 500)) {
                return;
            }

            vcr->vcr_control.last_frame_control_press =
                call_data->event->xbutton.time;
            if( vcr->vcr_control.button_is_in[VCR_COUNT] ) {
                ReleaseVCRButton(vcr, VCR_COUNT);
                if( vcr->vcr_control.frame_control_is_up ) {
                    XtUnmanageChild((Widget)vcr->vcr_control.frame_control);
                }
                vcr->vcr_control.frame_control_is_up = False;
            } else {

                XtManageChild((Widget)vcr->vcr_control.frame_control);

                PushVCRButton(vcr, VCR_COUNT);
                vcr->vcr_control.frame_control_is_up = True;
                ChangeVCRCurrentValue(vcr, vcr->vcr_control.current_value,
                                      TRUE, TRUE, FALSE);
                ChangeVCRNextValue(vcr, vcr->vcr_control.next_value,
                                   TRUE, TRUE, FALSE);
            }
        }
    } else if( call_data->event->type == ButtonRelease ) {
        /*  Release buttons which are press-only (momentary)  */
        if( vcr->vcr_control.button_is_in[VCR_PAUSE] )
            ReleaseVCRButton(vcr, VCR_PAUSE);
        else if( vcr->vcr_control.button_is_in[VCR_STOP] )
            ReleaseVCRButton(vcr, VCR_STOP);
        else if(   vcr->vcr_control.button_is_in[VCR_STEP]
                   && (vcr->vcr_control.button_timeout == 0) ) {
            /*  If in step mode and time has expired, release play button  */
            if( vcr->vcr_control.button_is_in[VCR_FORWARD] )
                ReleaseVCRButton(vcr, VCR_FORWARD);
            else if( vcr->vcr_control.button_is_in[VCR_BACKWARD] )
                ReleaseVCRButton(vcr, VCR_BACKWARD);
        }
        vcr->vcr_control.step_being_held = FALSE;
    }
}

/*  Subroutine:	ReleaseVCRButton
 *  Purpose:	Simulate effect of releasing button which had been pushed in
 */
static void ReleaseVCRButton( XmVCRControlWidget vcr, int button )
{
    Arg wargs[2];

    XtSetArg(wargs[0], XmNshadowType, XmSHADOW_OUT);
    XtSetValues((Widget)vcr->vcr_control.frame[button], wargs, 1);
    vcr->vcr_control.button_is_in[button] = FALSE;
    /*  Since the Frame's SetValues doesn't ask to redraw, we must do it!  */
    ReplaceFrame(vcr->vcr_control.frame[button]);
}

/*  Subroutine:	PushVCRButton
 *  Purpose:	Simulate effect of pushing in a button
 */
static void PushVCRButton( XmVCRControlWidget vcr, int button )
{
    Arg wargs[2];

    XtSetArg(wargs[0], XmNshadowType, XmSHADOW_IN);
    XtSetValues((Widget)vcr->vcr_control.frame[button], wargs, 1);
    vcr->vcr_control.button_is_in[button] = TRUE;
    /*  Since the Frame's SetValues doesn't ask to redraw, we must do it!  */
    if (XtIsRealized((Widget)vcr->vcr_control.frame[button])) {
        ReplaceFrame(vcr->vcr_control.frame[button]);
        /*  Send it right-away to maximize flicker, before it gets released  */

    }
}


/*  Subroutine:	ReleaseModeButtons
 *  Purpose:	Release any pushed button of the ganged radiobutton set
 */
static void ReleaseModeButtons( XmVCRControlWidget vcr )
{
    if( vcr->vcr_control.button_is_in[VCR_FORWARD] )
        ReleaseVCRButton(vcr, VCR_FORWARD);
    else if( vcr->vcr_control.button_is_in[VCR_BACKWARD] )
        ReleaseVCRButton(vcr, VCR_BACKWARD);
    else if( vcr->vcr_control.button_is_in[VCR_STOP] )
        ReleaseVCRButton(vcr, VCR_STOP);
    else if( vcr->vcr_control.button_is_in[VCR_PAUSE] )
        ReleaseVCRButton(vcr, VCR_PAUSE);
}

/*  Subroutine:	ReplaceFrame
 *  Purpose:	Redraw widget shadow.  Calls internal Xm call when stupid
 *		widget doesn't but should (i.e. when shadow of Frame is
 *		changed by SetValues).
 *  Note: %%	This uses Xm internals in violation of object oriented
 *		practice.  It may need adjustment with rev changes in Xm.
 */
static void ReplaceFrame( XmFrameWidget frame )
{
#if (OLD_LESSTIF != 1)

    short highlight_thickness = 0;

    if( XtIsRealized((Widget)frame) )
#if    (XmVersion < 2000)

        _XmDrawShadowType((Widget)frame, frame->frame.shadow_type,
                          frame->core.width, frame->core.height,
                          frame->manager.shadow_thickness,
                          highlight_thickness, frame->manager.top_shadow_GC,
                          frame->manager.bottom_shadow_GC);
#else    /* XmVersion >= 2000 */

        XmeDrawShadows(XtDisplay((Widget)frame), XtWindow((Widget)frame),
                       frame->manager.top_shadow_GC,
                       frame->manager.bottom_shadow_GC,
                       highlight_thickness, highlight_thickness,
                       frame->core.width, frame->core.height,
                       frame->manager.shadow_thickness,
                       frame->frame.shadow_type);
#endif    /* XmVersion */
#endif /* OLD_LESSTIF */
}



/*  Subroutine:	ChangeVCRNextValue
 *  Purpose:	Change the next frame value everywhere in the VCR control
 */
void ChangeVCRNextValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                         Boolean to_frame_control, Boolean to_outside )
{
    VCRCallbackStruct data;
    Arg wargs[2];

    vcr->vcr_control.next_value = value;
    /*  An idea to try, but may not be so great  */
    /*  Use the change of number to trigger the button release  */
    if( vcr->vcr_control.button_is_in[VCR_STEP] &&
            (vcr->vcr_control.step_being_held == FALSE) ) {
        if( vcr->vcr_control.button_timeout ) {
            XtRemoveTimeOut(vcr->vcr_control.button_timeout);
            vcr->vcr_control.button_timeout = 0;
        }
        if( vcr->vcr_control.button_is_in[VCR_FORWARD] )
            ReleaseVCRButton(vcr, VCR_FORWARD);
        if( vcr->vcr_control.button_is_in[VCR_BACKWARD] )
            ReleaseVCRButton(vcr, VCR_BACKWARD);
    }

    if( to_frame_control && vcr->vcr_control.frame_control_is_up) {
        XtSetArg(wargs[0], XmNnext, value);
        XtSetValues((Widget)vcr->vcr_control.frame_control, wargs, 1);
    }

    if( to_outside ) {

        data.detent = value;
        data.which  = XmCR_NEXT;
        XtCallCallbacks((Widget)vcr, XmNframeCallback, &data);
    }
}


/*  Subroutine:	ChangeVCRCurrentValue
 *  Purpose:	Change the current frame value everywhere in the VCR control
 */
void ChangeVCRCurrentValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                            Boolean to_frame_control, Boolean to_outside )
{
    VCRCallbackStruct data;
    Arg wargs[2];
    char string[100];
    XmString text;

    vcr->vcr_control.current_value = value;

    /*Update the display */
    if (vcr->vcr_control.current_visible) {
        sprintf(string, "%4d", vcr->vcr_control.current_value);
    } else {
        sprintf(string, " ");
    }

    text = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[0], XmNlabelString, text);
    XtSetValues( (Widget)vcr->vcr_control.label, wargs, 1);
    XmStringFree(text);

    if( to_frame_control && vcr->vcr_control.frame_control_is_up) {
        XtSetArg(wargs[0], XmNcurrent, value);
        XtSetArg(wargs[1], XmNcurrentVisible, vcr->vcr_control.current_visible);
        XtSetValues((Widget)vcr->vcr_control.frame_control, wargs, 2);
    }

    if( to_outside ) {

        data.detent = value;
        data.which  = XmCR_CURRENT;
        XtCallCallbacks((Widget)vcr, XmNframeCallback, &data);
    }
}

/*  Function: FrameControlCallback
 *  Purpose:	Handle changes due to the frame_control
 */
static void FrameControlCallback( Widget w, XmVCRControlWidget vcr,
                                  FrameControlCallbackStruct *call_data )
{
    VCRCallbackStruct data;

    if (call_data->reason == XmCR_START) {
        ChangeVCRStartValue(vcr, call_data->value, TRUE, FALSE, TRUE);
    }
    if (call_data->reason == XmCR_STOP) {
        ChangeVCRStopValue(vcr, call_data->value, TRUE, FALSE, TRUE);
    }
    if (call_data->reason == XmCR_NEXT) {
        ChangeVCRNextValue(vcr, call_data->value, TRUE, FALSE, TRUE);
    }
    if (call_data->reason == XmCR_INCREMENT) {
        ChangeVCRIncrementValue(vcr, call_data->value, TRUE, FALSE, TRUE);
    }
    if (call_data->reason == XmCR_MIN) {
        data.value = call_data->value;
        data.which  = XmCR_MIN;
        XtCallCallbacks((Widget)vcr, XmNframeCallback, &data);
    }
    if (call_data->reason == XmCR_MAX) {
        data.value = call_data->value;
        data.which  = XmCR_MAX;
        XtCallCallbacks((Widget)vcr, XmNframeCallback,
                        &data);
    }
}

/*  Function: PopdownCallback
 *  Purpose:	Assure appropriate state of interface if something else pops
 *		down the frame guide
 */
static void PopdownCallback( Widget w, XmVCRControlWidget vcr,
                             XmAnyCallbackStruct* call_data )
{
    if( vcr->vcr_control.button_is_in[VCR_COUNT] )
        ReleaseVCRButton(vcr, VCR_COUNT);
}

/*  Function: map_dialog
 *  Purpose:	Make sure the frame control doesn't map over the sequencer.
 */
static void map_dialog (Widget dialog, XmVCRControlWidget vcr,
                        XmAnyCallbackStruct* call_data)
{
    Arg wargs[12];
    Display *display;
    Dimension width, height;
    int dest_x, dest_y;
    int dispHeight = 0, dispWidth = 0, wmHeight = 0, borderWidth = 0;
    int screen = 0;
    Window child, rootW, parentW, *childrenW, vcrW;
    unsigned int numChildren;
    XWindowAttributes xwa;

    /*
       Get the location of the VCR control and then 
       determine if the frame control should go above or 
       below. Once determined, put it there.
     */

    display = XtDisplay(vcr);
    XtSetArg(wargs[0], XmNheight, &height);
	XtSetArg(wargs[1], XmNwidth, &width);
    XtGetValues((Widget)vcr, wargs, 2);
    screen = XScreenNumberOfScreen(XtScreen(vcr));
    dispHeight = DisplayHeight(display, screen);
	dispWidth = DisplayWidth(display, screen);

    vcrW = XtWindow(vcr);

    XTranslateCoordinates( display, vcrW,
                           XRootWindowOfScreen(XtScreen(vcr)),
                           0, 0, &dest_x, &dest_y, &child);

    /*
       Not very elegant, but it does the job. In order
       to get the window manager decorations sizes, it
       is required to get the window attributes of the
       parent's, parent's, parent of the shell. 
    */

    XQueryTree(display, vcrW, &rootW, &parentW,
               &childrenW, &numChildren);
    if(childrenW)
        XFree((char *)childrenW);

    XQueryTree(display, parentW, &rootW, &parentW,
               &childrenW, &numChildren);
    if(childrenW)
        XFree((char *)childrenW);

    XQueryTree(display, parentW, &rootW, &parentW,
               &childrenW, &numChildren);
    if(childrenW)
        XFree((char *)childrenW);

    XGetWindowAttributes(display, parentW, &xwa);
    wmHeight = dest_y - xwa.y;
    borderWidth = dest_x - xwa.x;

    if(dest_y + height + 180 + wmHeight + 2  + borderWidth * 2 > dispHeight) {
        XtGetValues((Widget)vcr->vcr_control.frame_control, wargs, 1);
        dest_y = dest_y - 2 * wmHeight - borderWidth - height - 2;
    } else
        dest_y = dest_y + height + borderWidth + 2;

	if(dest_x + 400 + borderWidth * 2 > dispWidth) {
		dest_x = dispWidth - 2*borderWidth - 400 - 2;
	} else
	    dest_x -= borderWidth;

    XtVaSetValues(dialog,
                  XmNx, dest_x,
                  XmNy, dest_y,
                  NULL);

}

/*  Subroutine:	ChangeVCRStartValue
 *  Purpose:	Change the start frame value everywhere in the VCR control
 *  Note:	Values are in external format (min based or +min from internal)
 */

void ChangeVCRStartValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                          Boolean to_frame_control, Boolean to_outside )
{
    VCRCallbackStruct data;
    Arg wargs[2];

    vcr->vcr_control.start_value = value;
    if( to_frame_control) {
        XtSetArg(wargs[0], XmNstart, value);
        XtSetValues((Widget)vcr->vcr_control.frame_control, wargs, 1);
    }
    if( to_outside ) {
        data.detent = value;
        data.which  = XmCR_START;
        XtCallCallbacks((Widget)vcr, XmNframeCallback, &data);
    }
}


/*  Subroutine:	ChangeVCRStopValue
 *  Purpose:	Change the stop frame value everywhere in the VCR control
 *  Note:	Values are in external format (min based or +min from internal)
 */
void ChangeVCRStopValue( XmVCRControlWidget vcr, int value, Boolean to_vcr,
                         Boolean to_frame_control, Boolean to_outside )
{
    VCRCallbackStruct data;
    Arg wargs[2];

    vcr->vcr_control.stop_value = value;
    if( to_frame_control) {
        XtSetArg(wargs[0], XmNstop, value);
        XtSetValues((Widget)vcr->vcr_control.frame_control, wargs, 1);
    }
    if( to_outside ) {
        data.detent = value;
        data.which  = XmCR_STOP;
        XtCallCallbacks((Widget)vcr, XmNframeCallback, &data);
    }
}


/*  Subroutine:	ChangeVCRIncrementValue
 *  Purpose:	Change the frame increment value for the sequence controller
 */
void ChangeVCRIncrementValue( XmVCRControlWidget vcr, int increment,
                              Boolean to_vcr, Boolean to_frame_control,
                              Boolean to_outside )
{
    VCRCallbackStruct data;
    Arg wargs[2];

    vcr->vcr_control.frame_increment = increment;
    if( to_frame_control) {
        XtSetArg(wargs[0], XmNincrement, increment);
        XtSetValues((Widget)vcr->vcr_control.frame_control, wargs, 1);
    }
    if( to_outside ) {/*&& vcr->vcr_control.value_callback )*/
        data.detent = increment;
        data.which  = XmCR_INCREMENT;
        XtCallCallbacks((Widget)vcr, XmNframeCallback, &data);
    }
}

/*   Subroutine: XmCreateVCRControl
 *   Purpose: This function creates and returns a VCRControl widget.
*/

Widget XmCreateVCRControl( Widget parent, char *name, ArgList args,
                           Cardinal num_args)
{
    return (XtCreateWidget(name, xmVCRControlWidgetClass, parent, args,
                           num_args));

}
