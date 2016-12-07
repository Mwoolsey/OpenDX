/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dxconfig.h>
#include "../base/defines.h"



#include <stdio.h>
#include <math.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/BulletinBP.h>
#include <Xm/FormP.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/FileSB.h>
#include <Xm/ToggleB.h>
#include <Xm/XmP.h>
#include <Xm/Xm.h>
#include "Number.h"
#include "FFloat.h"
#include <float.h>
#include "ColorMapEditor.h"
#include "ControlField.h"
#include "ColorMapEditorP.h"
#include "FieldCursor.h"
#include "Grid.h"
#ifdef SPLINE
#include "cubic_spline.h"
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define WIDTH 80
#define HEIGHT 300

#define RGB_L 1
#define COL_L 14
#define IND_L 20
#define HUE_L 36
#define GREEN_L 40
#define BLUE_L 45
#define SAT_L 52
#define VAL_L 68
#define OP_L 84
#define VALUE_L 20
#define TOP_L 5
#define BOTTOM_L 92

#define superclass (&widgetClassRec)
#define MIN_WIDTH 550
#define MIN_HEIGHT 320
/* convenient magic numbers */

/*  Declare defaults that can be assigned through pointers  */
#define DEF_MIN 3
#define DEF_MAX 100

static double DefaultMinDbl = (double)DEF_MIN;
static double DefaultMaxDbl = (double)DEF_MAX;

/* External Functions */
void AddControlPoint(ControlMap* map, double level, double value,
                     Boolean above);   /* From ControlPoint.c */
void RemoveSelectedControlPoints(XmColorMapEditorWidget cmew); /* From ControlPoint.c */
void PrintAllControlPoints(XmColorMapEditorWidget cmew); /* From ControlPoint.c */
void SetColorCoords( XColor *cells, ControlField* field,
                            short* load, short load_limit ); /* From ColorRGB.c */
void DrawColorBar( ColorBar* bar, XColor* undithered_cells ); /* From ColorBar.c */
void SetRGBLines( XColor* cells, ControlField* field,
                  ControlLine* h, ControlLine* s, ControlLine* v ); /* From ColorRGB.c */


/*  Class and internal functions  */

static void    ClassInitialize();
static void    Initialize();
static void    Resize();
static void    Realize();
static void    Destroy();
static Boolean SetValues();
static void    exposeCME(XmColorMapEditorWidget, XExposeEvent *, Region);
void update_callback(ControlField *field);

static void FieldControlCB(Widget w, XtPointer client,
	 XmAnyCallbackStruct* call_data);
static void MinNumCallback( XmNumberWidget nw, XmColorMapEditorWidget cmew,
	 XmDoubleCallbackStruct* call_data);
static void MaxNumCallback( XmNumberWidget nw, XmColorMapEditorWidget cmew,
	 XmDoubleCallbackStruct* call_data);
static void CreateColors( Widget toplevel, Pixel *black,
			  Pixel *red, Pixel *green, Pixel *blue );
static Widget CreateFormLabel( Widget form, char *title, int attach_left,
			       int attach_right, Boolean attach_top );
extern void LoadColormapFromFile(char *filename, XmColorMapEditorWidget cmew,
		Boolean init);
extern void LoadHardCodeDefaults(XmColorMapEditorWidget new, Boolean init);
extern void CallActivateCallback(XmColorMapEditorWidget cmew, int reason);
void RecalculateControlPointLevels(XmColorMapEditorWidget cmew);

static XtResource resources[] = 
{
   {
      XmNminEditable, XmCEditable, XmRBoolean, sizeof(Boolean),
      XtOffset(XmColorMapEditorWidget, color_map_editor.min_editable),
      XmRImmediate, (XtPointer) True
   },
   {
      XmNmaxEditable, XmCEditable, XmRBoolean, sizeof(Boolean),
      XtOffset(XmColorMapEditorWidget, color_map_editor.max_editable),
      XmRImmediate, (XtPointer) True
   },
   {
      XmNvalue, XmCValue, XmRDouble, sizeof(double),
      XtOffset (XmColorMapEditorWidget, color_map_editor.value_current), 
      XmRDouble, (XtPointer)  &DefaultMinDbl
   },
   {
      XmNvalueMinimum, XmCValueMinimum, XmRDouble, sizeof(double),
      XtOffset (XmColorMapEditorWidget, color_map_editor.value_minimum), 
      XmRDouble, (XtPointer)  &DefaultMinDbl
   },
   {
      XmNvalueMaximum, XmCValueMaximum, XmRDouble, sizeof(double),
      XtOffset (XmColorMapEditorWidget, color_map_editor.value_maximum), 
      XmRDouble, (XtPointer) &DefaultMaxDbl
   },
   {
      XmNdefaultColormap, XmCDefaultColormap, XmRString, sizeof(String),
      XtOffset(XmColorMapEditorWidget, color_map_editor.default_colormap ),
      XmRString, (XtPointer) NULL
   },
   {
      XmNactivateCallback, XmCActivateCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmColorMapEditorWidget, color_map_editor.g.activate_callback),
      XmRCallback, NULL
   },
   {
      XmNconstrainVertical, XmCConstrainVertical, XmRBoolean, sizeof(Boolean),
      XtOffset(XmColorMapEditorWidget, color_map_editor.constrain_vert),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNconstrainHorizontal,XmCConstrainHorizontal,XmRBoolean,sizeof(Boolean),
      XtOffset(XmColorMapEditorWidget, color_map_editor.constrain_hor),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNtriggerCallback, XmCTriggerCallback, XmRBoolean, sizeof(Boolean),
      XtOffset(XmColorMapEditorWidget, color_map_editor.trigger_callback),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNdisplayOpacity, XmCDisplayOpacity, XmRBoolean, sizeof(Boolean),
      XtOffset(XmColorMapEditorWidget, color_map_editor.display_opacity),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNbackgroundStyle, XmCBackgroundStyle, XmRInt, sizeof(int),
      XtOffset(XmColorMapEditorWidget, 
		color_map_editor.color_bar_background_style),
      XmRImmediate, (XtPointer) 0
   },
   {
      XmNdrawMode, XmCDrawMode, XmRInt, sizeof(int),
      XtOffset(XmColorMapEditorWidget, 
		color_map_editor.draw_mode),
      XmRImmediate, (XtPointer) CME_GRID
   },
   {
      XmNprintCP, XmCPrintCP, XmRInt, sizeof(int),
      XtOffset(XmColorMapEditorWidget, 
		color_map_editor.print_points),
      XmRImmediate, (XtPointer) XmPRINT_ALL
   },
};

XmColorMapEditorClassRec xmColorMapEditorClassRec = {
  {
/* core_class fields */	
    /* superclass	  */	(WidgetClass) &xmFormClassRec,
    /* class_name	  */	"XmColorMapEditor",
    /* widget_size	  */	sizeof(XmColorMapEditorRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_initiali*/	NULL,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */	NULL,
    /* realize		  */	Realize,
    /* actions		  */    NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	Destroy,
    /* resize		  */	Resize,
    /* expose		  */	(XtExposeProc)exposeCME,
    /* set_values	  */	SetValues,
    /* set_values_hook	  */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus	  */	NULL,
    /* version		  */	XtVersion,
    /* callback_private   */	NULL,
    /* tm_table		  */	XtInheritTranslations,
    /* query_geometry     */    XtInheritQueryGeometry,
    /* display accel	  */	NULL,
    /* extension	  */	NULL,
   },

   {						/* composite_class fields */
      XtInheritGeometryManager,                 /* geometry_manager       */
      XtInheritChangeManaged,                   /* change_managed         */
      (XtWidgetProc)_XtInherit,                 /* insert_child           */
      XtInheritDeleteChild,                     /* delete_child           */
      NULL,             			/* extension		  */
   },

   {						/* constraint_class fields */
      NULL,                                     /* constraint resource     */
      0,                 		        /* number of constraints   */
      sizeof(XmFormConstraintRec),              /* size of constraint      */
      NULL,                                     /* initialization          */
      NULL,                                     /* destroy proc            */
      NULL,                                     /* set_values proc         */
      NULL,                                     /* extension               */
   },

   {						/* manager_class fields   */
#if (XmVersion >= 1001)
      XtInheritTranslations,     		/* translations           */
#else
      (XtTranslations)_XtInherit,     		/* translations           */
#endif
      NULL,                                     /* get resources          */
      0,                                        /* num get_resources      */
      NULL,                                     /* get_cont_resources     */
      0,                                        /* num_get_cont_resources */
      (XmParentProcessProc)NULL,		/* parent_process 	  */
      NULL,             			/* extension 		  */
   },

   {				/* bulletin board class fields   */
	False,			/* always_install_accelerators */
	NULL,			/* geo_matrix_create */
	NULL,			/* focus_moved_proc */
	NULL,			/* extension */
   },

   {				/* form class fields   */
	NULL,             	/* extension         */
   },

   {				/* color map editor class fields */
	NULL,             	/* Control Point Cut Buffer */
	0,
	NULL,
	NULL,
   }
};

WidgetClass xmColorMapEditorWidgetClass = 
            (WidgetClass) &xmColorMapEditorClassRec;

XmColorMapEditorClassRec *xmCMEWidgetClass = &xmColorMapEditorClassRec;
	
static void Initialize (XmColorMapEditorWidget request, 
			XmColorMapEditorWidget new)
{
Pixel black, red, green, blue;
Arg wargs[12];
int n;
#if (XmVersion < 1001)
short shadow_thickness;
#else
Dimension shadow_thickness;
#endif
unsigned long 	valuemask;
XGCValues  	values;
XmFontList 	font_list;

#if (XmVersion >= 1001)
XmFontContext   context;
XmStringCharSet charset;
#endif
    
    /*
     * Set this to null, so that one can call XmColorMapDrawHistogram()
     * before the colormap editor is created.
     */
    new->color_map_editor.grid_w = NULL;

    new->core.width = MAX(MIN_WIDTH, new->core.width);
    new->core.height = MAX(MIN_HEIGHT, new->core.height);

    CreateColors(new->core.parent, &black, &red, &green, &blue);

    /*  Create labels  */
    new->color_map_editor.label[0] = 
		CreateFormLabel((Widget)new, "RGB", RGB_L, COL_L, True); 
    new->color_map_editor.label[1] = 
		CreateFormLabel((Widget)new, "max", COL_L, IND_L, True); 
    new->color_map_editor.label[12] = 
		CreateFormLabel((Widget)new, "min", COL_L, IND_L, False);
    new->color_map_editor.label[10] = 
		CreateFormLabel((Widget)new, "min", RGB_L, -1, False);
    new->color_map_editor.label[11] = 
		CreateFormLabel((Widget)new, "max", -1, COL_L, False);
    new->color_map_editor.label[13] = 
		CreateFormLabel((Widget)new, "r", HUE_L, -1, False);
    new->color_map_editor.label[14] = 
		CreateFormLabel((Widget)new, "g", GREEN_L, -1, False);
    new->color_map_editor.label[15] = 
		CreateFormLabel((Widget)new, "b", BLUE_L, -1, False);
    new->color_map_editor.label[16] = 
		CreateFormLabel((Widget)new, "r", -1, SAT_L, False);
    new->color_map_editor.label[17] = 
		CreateFormLabel((Widget)new, "min", SAT_L, -1, False);
    new->color_map_editor.label[18] = 
		CreateFormLabel((Widget)new, "max", -1, VAL_L, False);
    new->color_map_editor.label[19] = 
		CreateFormLabel((Widget)new, "min", VAL_L, -1, False);
    new->color_map_editor.label[20] = 
		CreateFormLabel((Widget)new, "max", -1, OP_L, False);
    new->color_map_editor.label[6] = 
		CreateFormLabel((Widget)new, "min", OP_L, -1, False);
    new->color_map_editor.label[7] = 
		CreateFormLabel((Widget)new, "max", -1, 99, False);

    /*  Create graph fields - ControlFields are the drawing area widgets and
        the data structure to describe the area and it's state */

    XtSetArg(wargs[0], XmNtopAttachment, XmATTACH_WIDGET);
    XtSetArg(wargs[1], XmNbottomAttachment, XmATTACH_WIDGET);
    XtSetArg(wargs[2], XmNleftAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[3], XmNrightAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[4], XmNtopWidget, new->color_map_editor.label[0]);
    XtSetArg(wargs[5], XmNbottomWidget, new->color_map_editor.label[10]);
    XtSetArg(wargs[6], XmNleftPosition, HUE_L);
    XtSetArg(wargs[7], XmNrightPosition, SAT_L);
    XtSetArg(wargs[8], XmNtopOffset, 1);
    XtSetArg(wargs[9], XmNbottomOffset, 1);
    XtSetArg(wargs[10], XmNleftOffset, 1);
    XtSetArg(wargs[11], XmNrightOffset, 1);

    /*
     * Set up font inforamtion
     */
    XtVaGetValues(new->color_map_editor.label[7],XmNfontList, &font_list, NULL);

#if (XmVersion < 1001)
    new->color_map_editor.font = font_list->font;
#else
    XmFontListInitFontContext(&context, font_list);
    XmFontListGetNextFont(context, &charset, &new->color_map_editor.font);
    XmFontListFreeFontContext(context);
#endif

    valuemask = GCFont;
    values.font = new->color_map_editor.font->fid;
    new->color_map_editor.fontgc = XtGetGC((Widget)new, valuemask, &values);
    XSetForeground(XtDisplay(new), new->color_map_editor.fontgc, 
	BlackPixel(XtDisplay(new), 0));

    /* Create the Hue graph */
    new->color_map_editor.g.field[0] =
      CreateControlField((Widget)new, "Hue", NUM_LEVELS, 
			0, 0, WIDTH, HEIGHT, wargs, 12);

    /*  Add callback for some special decorations  */
    XtSetArg(wargs[6], XmNleftPosition, SAT_L);
    XtSetArg(wargs[7], XmNrightPosition, VAL_L);

    /* Create the Saturation graph */
    new->color_map_editor.g.field[1] =
      CreateControlField((Widget)new, "Saturation", NUM_LEVELS, 
			0, 0, WIDTH, HEIGHT, wargs, 12);

    XtSetArg(wargs[6], XmNleftPosition, VAL_L);
    XtSetArg(wargs[7], XmNrightPosition, OP_L);

    /* Create the Value graph */
    new->color_map_editor.g.field[2] =
      CreateControlField((Widget)new, "Value", NUM_LEVELS, 
			0, 0, WIDTH, HEIGHT, wargs, 12);

    XtSetArg(wargs[6], XmNleftPosition, OP_L);
    XtSetArg(wargs[7], XmNrightPosition, 99);

    /* Create the Opacity graph */
    new->color_map_editor.g.field[3] =
      CreateControlField((Widget)new, "Opacity", NUM_LEVELS, 
			0, 0, WIDTH, HEIGHT, wargs, 12);

    new->color_map_editor.g.field[0]->line[0] = 
		CreateControlLine(new->color_map_editor.g.field[0], 1, black);
    new->color_map_editor.g.field[1]->line[0] = 
		CreateControlLine(new->color_map_editor.g.field[1], 1, black);
    new->color_map_editor.g.field[2]->line[0] = 
		CreateControlLine(new->color_map_editor.g.field[2], 1, black);
    new->color_map_editor.g.field[3]->line[0] = 
		CreateControlLine(new->color_map_editor.g.field[3], 1, black);

    new->color_map_editor.g.field[0]->num_lines = 1;
    new->color_map_editor.g.field[1]->num_lines = 1;
    new->color_map_editor.g.field[2]->num_lines = 1;
    new->color_map_editor.g.field[3]->num_lines = 1;

    new->color_map_editor.g.field[0]->map[0] =
      CreateControlMap(new->color_map_editor.g.field[0], 
		new->color_map_editor.g.field[0]->line[0], black, TRUE);
    new->color_map_editor.g.field[0]->map[0]->toggle_on = True;
    new->color_map_editor.g.field[1]->map[0] =
      CreateControlMap(new->color_map_editor.g.field[1], 
		new->color_map_editor.g.field[1]->line[0], black, TRUE);
    new->color_map_editor.g.field[2]->map[0] =
      CreateControlMap(new->color_map_editor.g.field[2], 
		new->color_map_editor.g.field[2]->line[0], black, TRUE);
    new->color_map_editor.g.field[3]->map[0] =
      CreateControlMap(new->color_map_editor.g.field[3], 
		new->color_map_editor.g.field[3]->line[0], black, TRUE);

    new->color_map_editor.g.field[0]->num_maps = 1;
    new->color_map_editor.g.field[1]->num_maps = 1;
    new->color_map_editor.g.field[2]->num_maps = 1;
    new->color_map_editor.g.field[3]->num_maps = 1;

    new->color_map_editor.g.field[0]->user_data = 
	(XtPointer)&(new->color_map_editor.g);
    new->color_map_editor.g.field[1]->user_data = 
	(XtPointer)&(new->color_map_editor.g);
    new->color_map_editor.g.field[2]->user_data = 
	(XtPointer)&(new->color_map_editor.g);
    new->color_map_editor.g.field[3]->user_data = 
	(XtPointer)&(new->color_map_editor.g);

    new->color_map_editor.g.field[0]->user_callback = update_callback;
    new->color_map_editor.g.field[1]->user_callback = update_callback;
    new->color_map_editor.g.field[2]->user_callback = update_callback;
    new->color_map_editor.g.field[3]->user_callback = update_callback;

    new->color_map_editor.g.color = NULL;

    XtSetArg(wargs[6], XmNleftPosition, RGB_L);
    XtSetArg(wargs[7], XmNrightPosition, COL_L);
    new->color_map_editor.g.rgb =
      CreateControlField((Widget)new, "RGB", NUM_LEVELS, 
		0, 0, WIDTH, HEIGHT, wargs, 12);

    new->color_map_editor.g.rgb->line[0] = 
	CreateControlLine(new->color_map_editor.g.rgb, 1, red);
    new->color_map_editor.g.rgb->line[1] = 
	CreateControlLine(new->color_map_editor.g.rgb, 1, green);
    new->color_map_editor.g.rgb->line[2] = 
	CreateControlLine(new->color_map_editor.g.rgb, 1, blue);

    new->color_map_editor.g.rgb->num_lines = 3;
    new->color_map_editor.g.rgb->user_callback = update_callback;
    new->color_map_editor.g.rgb->user_data = 
	(XtPointer)&(new->color_map_editor.g);

    XtSetArg(wargs[0], XmNshadowThickness, &shadow_thickness);
    XtGetValues((Widget)new, wargs, 1);

    /* Note: Number widgets must be created AFTER the default cmap is loaded */
    n=0;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, VALUE_L + 1); n++;
    XtSetArg(wargs[n], XmNbottomPosition, BOTTOM_L + 5); n++;
    DoubleSetArg(wargs[n], XmNdValue, new->color_map_editor.value_minimum); n++;
    XtSetArg(wargs[n], XmNdecimalPlaces, -1); n++;
    XtSetArg(wargs[n], XmNeditable, True); n++;
    XtSetArg(wargs[n], XmNcenter, TRUE); n++;
    XtSetArg(wargs[n], XmNdataType, DOUBLE); n++;
    XtSetArg(wargs[n], XmNshadowThickness, shadow_thickness); n++;
    XtSetArg(wargs[n], XmNfixedNotation, False); n++;
    new->color_map_editor.min_num = 
	(XmNumberWidget)XmCreateNumber((Widget)new, "min_num", wargs, n);
    XtManageChild((Widget)new->color_map_editor.min_num);
    XtAddCallback((Widget)new->color_map_editor.min_num, XmNdisarmCallback, 
	(XtCallbackProc)MinNumCallback, new); 
    XtSetSensitive((Widget)new->color_map_editor.min_num, 
	new->color_map_editor.min_editable);

    n=0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNtopPosition, 2); n++;
    XtSetArg(wargs[n], XmNleftPosition, VALUE_L + 1); n++;
    DoubleSetArg(wargs[n], XmNdValue, new->color_map_editor.value_maximum); n++;
    XtSetArg(wargs[n], XmNdecimalPlaces, -1); n++;
    XtSetArg(wargs[n], XmNeditable, True); n++;
    XtSetArg(wargs[n], XmNcenter, TRUE); n++;
    XtSetArg(wargs[n], XmNdataType, DOUBLE); n++;
    XtSetArg(wargs[n], XmNshadowThickness, shadow_thickness); n++;
    XtSetArg(wargs[n], XmNfixedNotation, False); n++;
    new->color_map_editor.max_num = 
	(XmNumberWidget)XmCreateNumber((Widget)new, "max_num", wargs, n);
    XtManageChild((Widget)new->color_map_editor.max_num);
    XtAddCallback((Widget)new->color_map_editor.max_num, XmNdisarmCallback, 
	(XtCallbackProc)MaxNumCallback, new); 
    XtSetSensitive((Widget)new->color_map_editor.max_num, 
	new->color_map_editor.max_editable);

    new->color_map_editor.g.field[0]->toggle_b = 
	XtVaCreateManagedWidget("Hue", xmToggleButtonWidgetClass, (Widget)new,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, new->color_map_editor.g.field[0]->w,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, new->color_map_editor.g.field[0]->w,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, new->color_map_editor.g.field[0]->w,
			XmNindicatorOn, False,
			XmNset, True,
			NULL);
    XtAddCallback((Widget)new->color_map_editor.g.field[0]->toggle_b, 
	XmNvalueChangedCallback, (XtCallbackProc)FieldControlCB, 0); 

    new->color_map_editor.g.field[1]->toggle_b = 
	XtVaCreateManagedWidget("Saturation", xmToggleButtonWidgetClass, (Widget)new,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, new->color_map_editor.g.field[1]->w,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, new->color_map_editor.g.field[1]->w,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, new->color_map_editor.g.field[1]->w,
			XmNindicatorOn, False,
			NULL);
    XtAddCallback( (Widget)new->color_map_editor.g.field[1]->toggle_b, 
	XmNvalueChangedCallback, (XtCallbackProc)FieldControlCB, (XtPointer)1); 

    new->color_map_editor.g.field[2]->toggle_b = 
	XtVaCreateManagedWidget("Value", xmToggleButtonWidgetClass, (Widget)new,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, new->color_map_editor.g.field[2]->w,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, new->color_map_editor.g.field[2]->w,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, new->color_map_editor.g.field[2]->w,
			XmNindicatorOn, False,
			NULL);
    XtAddCallback((Widget)new->color_map_editor.g.field[2]->toggle_b, 
	XmNvalueChangedCallback, (XtCallbackProc)FieldControlCB, (XtPointer)2); 

    new->color_map_editor.g.field[3]->toggle_b = 
	XtVaCreateManagedWidget("Opacity", xmToggleButtonWidgetClass, (Widget)new,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, new->color_map_editor.g.field[3]->w,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, new->color_map_editor.g.field[3]->w,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, new->color_map_editor.g.field[3]->w,
			XmNindicatorOn, False,
			NULL);
    XtAddCallback((Widget)new->color_map_editor.g.field[3]->toggle_b, 
	XmNvalueChangedCallback, (XtCallbackProc)FieldControlCB, (XtPointer)3); 

    new->color_map_editor.bins = NULL;
    new->color_map_editor.log_bins = NULL;
    new->color_map_editor.grid_pixmap = 0;
    new->color_map_editor.first_grid_expose = True;
    new->color_map_editor.field_cursor_id = 0;

}

static void Destroy(XmColorMapEditorWidget w)
{
ColorBar *bar;
ControlField *field;
ControlMap *map;
ControlLine *line;
ControlColor *color;
int i,j;

    if (w->color_map_editor.field_cursor_id != 0)
	XFreeCursor(XtDisplay(w), w->color_map_editor.field_cursor_id);
    if (w->color_map_editor.bins != NULL)
	XtFree((char*)w->color_map_editor.bins);
    if (w->color_map_editor.log_bins != NULL)
	XtFree((char*)w->color_map_editor.log_bins);
#if 00
    if (w->color_map_editor.g.color)
	XtFree((char *)w->color_map_editor.g.color);
#endif

    /* 
     * plug memory leaks
     * n.b.   The undo list is still leaking beaucoup memory
     * but it appears to share pointers with map and so it could be a toughie
     */
    bar = w->color_map_editor.g.colorbar;
    if (bar) {
        if (bar->ximage) {
           XDestroyImage (bar->ximage);
           bar->ximage = 0;
        }
        if (bar->dithered_cells) {
           XtFree((char *)bar->dithered_cells);
           bar->dithered_cells = 0;
        }
        if (bar->index_line) {
           XtFree((char *)bar->index_line);
           bar->index_line = 0;
        }
        XtFree ((char *)w->color_map_editor.g.colorbar);
        w->color_map_editor.g.colorbar = 0;
    }

    color = w->color_map_editor.g.color;
    if (color) {
        XtFree((char *)color);
        w->color_map_editor.g.color = 0;
    }

    for (i=0; i<5; i++) {
        if (i==4) field = w->color_map_editor.g.rgb;
        else field = w->color_map_editor.g.field[i];
        if (field) {
           if (field->levels) {
                XtFree((char *)field->levels);
                field->levels = 0;
           }
           if (field->coords) {
                XtFree((char *)field->coords);
                field->coords = 0;
           }

           for (j=0; j<field->num_lines; j++) {
                line = field->line[j];
                if (line) {
                   if (line->values) {
                        XtFree((char *)line->values);
                        line->values = 0;
                   }
                   if (line->points) {
                        XtFree((char *)line->points);
                        line->points = 0;
                   }
                   if (line->load) {
                        XtFree((char *)line->load);
                        line->load = 0;
                   }
                   XtFree((char *)line);
                   field->line[j] = 0;
                }
           }

           for (j=0; j<field->num_maps; j++) {
                map = field->map[j];
                if (map) {
                   if (map->points) {
                        XtFree((char *)map->points);
                        map->points = 0;
                   }
                   if (map->boxes) {
                        XtFree((char *)map->boxes);
                        map->boxes = 0;
                   }
                   XtFree((char *)map);
                   field->map[j] = 0;
                }
           }

           XtFree((char *)field);
           if (i==4) w->color_map_editor.g.rgb = 0;
           else w->color_map_editor.g.field[i] = 0;
        }
    }
}

void update_callback( ControlField* field )
{
    struct groupRec* g = (struct groupRec *)field->user_data;
    SetControlColor(g->color,
		    g->field[0]->line[0]->values,
		    g->field[1]->line[0]->values,
		    g->field[2]->line[0]->values);
    if (field != g->field[3])
	{
	SetRGBLines(g->color->undithered_cells, g->rgb,
	      g->field[0]->line[0], g->field[1]->line[0], g->field[2]->line[0]);
	}
    DrawColorBar(g->colorbar, g->color->undithered_cells);
}


static void CreateColors( Widget toplevel, Pixel *black,
			  Pixel *red, Pixel *green, Pixel *blue )
{
    XColor screen_color, exact_color;
    int screen = XScreenNumberOfScreen(XtScreen(toplevel));

    *black = BlackPixel(XtDisplay(toplevel), 0);
    if( XAllocNamedColor(XtDisplay(toplevel),
			 DefaultColormap(XtDisplay(toplevel), screen),
			 "Red", &screen_color, &exact_color) )
	*red = screen_color.pixel;
    else
	*red = 68;
    if( XAllocNamedColor(XtDisplay(toplevel),
			 DefaultColormap(XtDisplay(toplevel), screen),
			 "Green", &screen_color, &exact_color) )
	*green = screen_color.pixel;
    else
	*green = 40;
    if( XAllocNamedColor(XtDisplay(toplevel),
			 DefaultColormap(XtDisplay(toplevel), screen),
			 "Blue", &screen_color, &exact_color) )
	*blue = screen_color.pixel;
    else
	*blue = 40;
}


static Widget CreateFormLabel( Widget form, char *title, int attach_left,
			       int attach_right, Boolean attach_top )
{
    Widget label;
    XmString text;
    int num_args;
    Arg wargs[20];

    /*  Create the string for the label, and then the label  */
    text = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[0], XmNlabelString, text);
    XtSetArg(wargs[1], XmNleftOffset, 1);
    XtSetArg(wargs[2], XmNrightOffset, 1);
    if( attach_top )
        {
	XtSetArg(wargs[3], XmNtopAttachment, XmATTACH_POSITION);
	XtSetArg(wargs[4], XmNtopPosition, TOP_L);
        num_args = 5;
        }
    else
        {
	XtSetArg(wargs[3], XmNbottomAttachment, XmATTACH_POSITION);
	XtSetArg(wargs[4], XmNbottomPosition, BOTTOM_L);
        num_args = 5;
        }
    if( attach_left >= 0 )
    {
	XtSetArg(wargs[num_args], XmNleftAttachment, XmATTACH_POSITION);
	num_args++;
	XtSetArg(wargs[num_args], XmNleftPosition, attach_left);
	num_args++;
    }
    if( attach_right >= 0 )
    {
	XtSetArg(wargs[num_args], XmNrightAttachment, XmATTACH_POSITION);
	num_args++;
	XtSetArg(wargs[num_args], XmNrightPosition, attach_right);
	num_args++;
    }
    label = XmCreateLabel(form, "label", wargs, num_args);
    XtManageChild(label);
    return label;
}


/************************************************************************
 *
 *  XmCreateColorMapEditor
 *      Create an instance of a ColorMapEditor and return the widget id.
 *
 ************************************************************************/

Widget XmCreateColorMapEditor (parent, name, arglist, argcount)
Widget parent;
char * name;
ArgList arglist;
Cardinal argcount;

{
   return(XtCreateWidget(name, xmColorMapEditorWidgetClass, parent, 
				arglist, argcount));
}
/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the resolution independence get values resource tabels.
 *
 ************************************************************************/

static void ClassInitialize()
{
    xmCMEWidgetClass->color_map_editor_class.cut_points = NULL;
    xmCMEWidgetClass->color_map_editor_class.color = NULL;

    /*  Install converters for type XmRDouble, XmRFloat  */
    XmAddFloatConverters();
}

/************************************************************************
 *
 *  Expose method
 *  ... added 2/95 because control point values don't get redrawn.  The
 *  XmDrawingArea which contains that text does not receive expose events
 *  as expected and so its area is left blank when the ui changes the
 *  value of print_points.  c1tignor
 *
 ************************************************************************/
static void exposeCME (XmColorMapEditorWidget cmew, XExposeEvent *xev, Region reg)
{
   (xmFormClassRec.core_class.expose) ((Widget)cmew, (XEvent *)xev, reg);
   PrintAllControlPoints(cmew);
}


/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
static Boolean SetValues (XmColorMapEditorWidget current, 
			  XmColorMapEditorWidget request, 
			  XmColorMapEditorWidget new)

{
Boolean		print_pts = False; 
#ifdef PASSDOUBLEVALUE
XtArgVal dx_l;
#endif
   if(new->color_map_editor.trigger_callback != 
      current->color_map_editor.trigger_callback)
	{
	CallActivateCallback(new, XmCR_MODIFIED);
   	new->color_map_editor.trigger_callback = False;
	}
   if(new->color_map_editor.color_bar_background_style != 
      current->color_map_editor.color_bar_background_style)
	{
	new->color_map_editor.g.colorbar->background_style = 
		new->color_map_editor.color_bar_background_style;
    	DrawColorBar(new->color_map_editor.g.colorbar, 
		new->color_map_editor.g.color->undithered_cells);
	}
   if(new->color_map_editor.draw_mode != current->color_map_editor.draw_mode)
	{
	if(new->color_map_editor.draw_mode == CME_GRID)
	    {
		DrawGrid(new->color_map_editor.grid_w, 
		    BlackPixel(XtDisplay(new), 0));
	    }
	else
	    {
		if(!new->color_map_editor.bins)
		    XtWarning("No Histogram Data to display");
		DrawHistogram(new->color_map_editor.grid_w, 
		    BlackPixel(XtDisplay(new), 0));
	    }
	}
   if(new->color_map_editor.min_editable != 
	current->color_map_editor.min_editable)
	{
	XtSetSensitive((Widget)new->color_map_editor.min_num, 
		new->color_map_editor.min_editable);
	}
   if(new->color_map_editor.max_editable != 
	current->color_map_editor.max_editable)
	{
	XtSetSensitive((Widget)new->color_map_editor.max_num, 
		new->color_map_editor.max_editable);
	}
    if(new->color_map_editor.value_minimum != 
	current->color_map_editor.value_minimum)
	{
	XtVaSetValues((Widget)new->color_map_editor.min_num, XmNdValue, 
		DoubleVal(new->color_map_editor.value_minimum,dx_l), NULL);
	print_pts = True;
	}
    if(new->color_map_editor.value_maximum != 
	current->color_map_editor.value_maximum)
	{
	XtVaSetValues((Widget)new->color_map_editor.max_num, XmNdValue, 
		DoubleVal(new->color_map_editor.value_maximum,dx_l), NULL);
	print_pts = True;
	}
   if(new->color_map_editor.print_points != current->color_map_editor.print_points)
	{
	print_pts = True;
	}

   if (print_pts && XtIsRealized((Widget)new)) 
	{
	PrintAllControlPoints(new);
   	}

   return TRUE;
}
/************************************************************************
 *
 *  Resize   
 *
 ************************************************************************/
static void Resize (new)
Widget new;
{
    /* Execute the resize method from the superclass (form) */
    (xmFormClassRec.core_class.resize) (new);
}
/************************************************************************
 *
 *  Realize   
 *
 ************************************************************************/
static void Realize (XmColorMapEditorWidget new, XtValueMask *value_mask, 
			XSetWindowAttributes *attributes)
{
Arg wargs[20];
Pixmap pix1;
XColor foreground, background;
int	i;

    /* Execute the realize method from the superclass (form) */
    (*superclass->core_class.realize) ((Widget)new, value_mask, attributes);

    /*
     * Only create the color mapping once.
     */
    if(xmCMEWidgetClass->color_map_editor_class.color == NULL)
	xmCMEWidgetClass->color_map_editor_class.color = 
	    CreateColorMap((Widget)new, NUM_LEVELS, False);
    /*
     * Reuse the color mapping stored in the class rec.  You must
     * memcpy it because it contains undithered_cells which is read/write.
     * This is for gresh550.  But does this really save anything... ? c1tignor
     */
    new->color_map_editor.g.color = (ControlColor *)XtMalloc(sizeof(ControlColor));
    memcpy (new->color_map_editor.g.color, 
	xmCMEWidgetClass->color_map_editor_class.color, sizeof(ControlColor));

    new->color_map_editor.g.colorbar = 
		CreateColorBar(new, MARGIN, 0, NUM_LEVELS - 1,
			new->color_map_editor.g.color);
    new->color_map_editor.g.colorbar->background_style = 
		new->color_map_editor.color_bar_background_style;
    SetControlColor(new->color_map_editor.g.color,
		    new->color_map_editor.g.field[0]->line[0]->values,
		    new->color_map_editor.g.field[1]->line[0]->values,
		    new->color_map_editor.g.field[2]->line[0]->values);
    SetColorCoords(&(new->color_map_editor.g.color->undithered_cells[0]),
		   new->color_map_editor.g.rgb,
		   new->color_map_editor.g.rgb->line[0]->load, 3);
    XtSetArg(wargs[0], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET);
    XtSetArg(wargs[1], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET);
    XtSetArg(wargs[2], XmNleftAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[3], XmNrightAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[4], XmNtopWidget, new->color_map_editor.g.rgb->w);
    XtSetArg(wargs[5], XmNbottomWidget, new->color_map_editor.g.rgb->w);
    XtSetArg(wargs[6], XmNleftPosition, IND_L);
    XtSetArg(wargs[7], XmNrightPosition, HUE_L);
    XtSetArg(wargs[8], XmNtopOffset, 1);
    XtSetArg(wargs[9], XmNbottomOffset, 1);
    XtSetArg(wargs[10], XmNleftOffset, 1);
    XtSetArg(wargs[11], XmNrightOffset, 1);
    new->color_map_editor.grid_w = 
	CreateGrid((Widget)new, BlackPixel(XtDisplay(new), 0), wargs, 12);
    /*  This stuff cannot occur between CreateControlField and XtRealize  */
    XtSetArg(wargs[6], XmNleftPosition, COL_L);
    XtSetArg(wargs[7], XmNrightPosition, IND_L);
#ifndef sgi
    XtSetValues(new->color_map_editor.g.colorbar->frame, wargs, 12);
    XtManageChild((Widget)new->color_map_editor.g.colorbar->w);
    XtManageChild(new->color_map_editor.g.colorbar->frame);
#else
    XtSetValues((Widget)new->color_map_editor.g.colorbar->w, wargs, 12);
    XtManageChild((Widget)new->color_map_editor.g.colorbar->w);
#endif
    XtSetArg(wargs[0], XmNdepth, 
		&new->color_map_editor.g.colorbar->depth);
    XtGetValues((Widget)new->color_map_editor.g.colorbar->w, wargs, 1);

    foreground.red = foreground.green = foreground.blue = 0; 
    background.red = background.green = background.blue = 0xffff; 
    foreground.flags = background.flags = DoRed | DoGreen | DoBlue;
    foreground.pixel = BlackPixel(XtDisplay(new), 0);
    background.pixel = WhitePixel(XtDisplay(new), 0);
    pix1 = XCreateBitmapFromData(
		    XtDisplay(new), 
		    XtWindow(new),
		    FieldCursor_bits, 
		    FieldCursor_width, 
		    FieldCursor_height);
    new->color_map_editor.field_cursor_id = XCreatePixmapCursor(
		    XtDisplay(new),
		    pix1,
		    pix1,
		    &foreground,
		    &background,
		    FieldCursor_x_hot,	
		    FieldCursor_y_hot);
    XFreePixmap(XtDisplay(new), pix1);
    XtRealizeWidget((Widget)new->color_map_editor.g.field[0]->w);
    XtRealizeWidget((Widget)new->color_map_editor.g.field[1]->w);
    XtRealizeWidget((Widget)new->color_map_editor.g.field[2]->w);
    XtRealizeWidget((Widget)new->color_map_editor.g.field[3]->w);
    XDefineCursor( XtDisplay(new->color_map_editor.g.field[0]->w), 
		   XtWindow(new->color_map_editor.g.field[0]->w),  
		   new->color_map_editor.field_cursor_id);
    XDefineCursor( XtDisplay(new->color_map_editor.g.field[1]->w), 
		   XtWindow(new->color_map_editor.g.field[1]->w),  
		   new->color_map_editor.field_cursor_id);
    XDefineCursor( XtDisplay(new->color_map_editor.g.field[2]->w), 
		   XtWindow(new->color_map_editor.g.field[2]->w),  
		   new->color_map_editor.field_cursor_id);
    XDefineCursor( XtDisplay(new->color_map_editor.g.field[3]->w), 
		   XtWindow(new->color_map_editor.g.field[3]->w),  
		   new->color_map_editor.field_cursor_id);
    for(i = 0; i < 4; i++)
    {
	SetArrayValues(new->color_map_editor.g.field[i]->map[0], 
		new->color_map_editor.g.field[i]->line[0]);
	SetLineCoords(new->color_map_editor.g.field[i]->line[0]);
	update_callback(new->color_map_editor.g.field[i]);
    }
}

static void FieldControlCB(Widget w, XtPointer client,
	 XmAnyCallbackStruct* call_data)
{
Boolean			set;
int			field_num = (int)(long)client;
XmColorMapEditorWidget 	cmew;
int			i;
ControlField 		*field;

    cmew = (XmColorMapEditorWidget)XtParent(w);
    XtVaSetValues(w, XmNset, True, NULL);
    cmew->color_map_editor.g.field[field_num]->map[0]->toggle_on = True;
    for(i = 0; i < 4; i++)
    {
	if(i != field_num)
	{
	    field = cmew->color_map_editor.g.field[i];
	    field->map[0]->toggle_on = False;
	    XtVaGetValues(field->toggle_b, XmNset, &set, NULL);
	    if(set) {
		/* on the rs6000, XtSetValues doesn't seem to work here. */
		/*XtVaSetValues(field->toggle_b, XmNset, False, NULL);*/
		XmToggleButtonSetState (field->toggle_b, False, False);
	    }
	}
    }
    PrintAllControlPoints(cmew);
    CallActivateCallback(cmew, XmCR_SELECT);
}
static void MinNumCallback( XmNumberWidget nw, XmColorMapEditorWidget cmew,
	 XmDoubleCallbackStruct* call_data)
{
Arg wargs[12];

    cmew->color_map_editor.value_minimum = call_data->value;
    if(cmew->color_map_editor.value_minimum >
	cmew->color_map_editor.value_maximum)
	{
	cmew->color_map_editor.value_maximum = call_data->value;
	DoubleSetArg(wargs[0], XmNdValue, call_data->value);
	XtSetValues((Widget)cmew->color_map_editor.max_num, wargs, 1);
	} 
    PrintAllControlPoints(cmew);
    CallActivateCallback(cmew, XmCR_MODIFIED);
}
static void MaxNumCallback( XmNumberWidget nw, XmColorMapEditorWidget cmew,
	 XmDoubleCallbackStruct* call_data)
{
Arg wargs[12];

    cmew->color_map_editor.value_maximum = call_data->value;
    if(cmew->color_map_editor.value_minimum >
	cmew->color_map_editor.value_maximum)
	{
	cmew->color_map_editor.value_minimum = call_data->value;
	DoubleSetArg(wargs[0], XmNdValue, call_data->value);
	XtSetValues((Widget)cmew->color_map_editor.min_num, wargs, 1);
	} 
    CallActivateCallback(cmew, XmCR_MODIFIED);
    PrintAllControlPoints(cmew);
}

/************************************************************************
 *
 *  XmColorMapEditorSave
 *      Write out a ColorMapFile
 *
 ************************************************************************/
Boolean XmColorMapEditorSave(Widget w, char *filename)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
int i,j;
ControlPoint *cp_ptr;
FILE *fp;

  if ( (fp = fopen(filename, "w")) == NULL)
    {
    XtWarning("opening colormap file");
    return False;
    }
 
  if (fprintf(fp,"%g\n",cmew->color_map_editor.value_minimum) < 0)
	{
	XtWarning("Error writing to color map file");
	return False;
	}
  if (fprintf(fp,"%g\n",cmew->color_map_editor.value_maximum) < 0)
	{
	XtWarning("Error writing to color map file");
	return False;
	}
  for (i=0; i<4; i++)				/* for all four fields */
    {
    if (fprintf(fp,"%d\n",
	cmew->color_map_editor.g.field[i]->map[0]->num_points) < 0)
	    {
	    XtWarning("Error writing to color map file");
	    return False;
	    }
    for (j=0;j < cmew->color_map_editor.g.field[i]->map[0]->num_points; j++)
  	{
	cp_ptr = cmew->color_map_editor.g.field[i]->map[0]->points;
	if (fprintf(fp,"%.20f  ",cp_ptr[j].level) < 0)
	    {
	    XtWarning("Error writing to color map file");
	    return False;
	    }
	if (fprintf(fp,"%.20f  ",cp_ptr[j].value) < 0)
	    {
	    XtWarning("Error writing to color map file");
	    return False;
	    }
	if (fprintf(fp,"%d  ",cp_ptr[j].wrap_around) < 0)
	    {
	    XtWarning("Error writing to color map file");
	    return False;
	    }
	if (fprintf(fp,"%d  ",cp_ptr[j].x) < 0)
	    {
	    XtWarning("Error writing to color map file");
	    return False;
	    }
	if (fprintf(fp,"%d\n",cp_ptr[j].y) < 0)
	    {
	    XtWarning("Error writing to color map file");
	    return False;
	    }
	}
    }
  fflush(fp);
  if (fclose(fp) != 0)
	{
	XtWarning("Error writing to color map file");
	return False;
	}
  return True;
}

/************************************************************************
 *
 *  XmColorMapEditorRead
 *      Read in a ColorMapFile
 *
 ************************************************************************/
Boolean XmColorMapEditorRead( Widget w, char *filename)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;

    PushUndoPoints(cmew);
    LoadColormapFromFile(filename, cmew, False);
    cmew->color_map_editor.g.field[0]->map[0]->grabbed = -1;
    cmew->color_map_editor.g.field[1]->map[0]->grabbed = -1;
    cmew->color_map_editor.g.field[2]->map[0]->grabbed = -1;
    cmew->color_map_editor.g.field[3]->map[0]->grabbed = -1;
    CallActivateCallback(cmew, XmCR_MODIFIED);
    return True;
}

/************************************************************************
 *
 *  XmColorMapEditorDeleteCP
 *      Delete Selected Control Points in a field
 *
 ************************************************************************/

void XmColorMapEditorDeleteCP (Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;

    PushUndoPoints(cmew);
    RemoveSelectedControlPoints(cmew);
    CallActivateCallback(cmew, XmCR_MODIFIED);
}

/************************************************************************
 *
 *  XmColorMapEditorLoadHistogram
 *
 ************************************************************************/
void XmColorMapEditorLoadHistogram(Widget w, int *bins, int num_bins)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
int i;
    
    if (cmew->color_map_editor.bins != NULL)
	XtFree((char*)cmew->color_map_editor.bins);
    if (cmew->color_map_editor.log_bins != NULL)
	XtFree((char*)cmew->color_map_editor.log_bins);

    if ((bins == NULL) || num_bins == 0) 
    {
	cmew->color_map_editor.bins = NULL; 
	cmew->color_map_editor.log_bins = NULL; 
    } 
    else 
    {
	cmew->color_map_editor.bins = (int*)XtMalloc(num_bins*sizeof(bins[0]));
	memcpy(cmew->color_map_editor.bins, bins, num_bins * sizeof(bins[0]));
	cmew->color_map_editor.log_bins = 
		(int*)XtMalloc(num_bins*sizeof(bins[0]));
	for(i = 0; i < num_bins; i++)
	{
	    cmew->color_map_editor.log_bins[i] = 100*log10((double)(bins[i]+1));
	}
    }
    cmew->color_map_editor.num_bins = num_bins;

    if ((cmew->color_map_editor.draw_mode == CME_HISTOGRAM) ||
	(cmew->color_map_editor.draw_mode == CME_LOGHISTOGRAM))
	DrawHistogram(cmew->color_map_editor.grid_w, 
	    BlackPixel(XtDisplay(cmew), 0));
}
/************************************************************************
 *
 *  XmColorMapEditorHasHistogram
 *
 ************************************************************************/
Boolean XmColorMapEditorHasHistogram(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;

    return (cmew->color_map_editor.bins != NULL);
}

/************************************************************************
 *
 *  XmColorMapReset
 *
 ************************************************************************/
void XmColorMapReset(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;

    PushUndoPoints(cmew);
    LoadHardCodeDefaults(cmew, False);
    PrintAllControlPoints(cmew);
    CallActivateCallback(cmew, XmCR_MODIFIED);
}
/************************************************************************
 *
 *  XmColorMapPastable
 *
 ************************************************************************/
Boolean XmColorMapPastable()
{
    return(xmCMEWidgetClass->color_map_editor_class.cut_points != NULL);
}
/************************************************************************
 *
 *  XmColorMapPaste
 *
 ************************************************************************/
void XmColorMapPaste(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
int i;
ControlField *field=NULL;
XmColorMapEditorClassPart *color_map_editor_class;

    color_map_editor_class = &(xmCMEWidgetClass->color_map_editor_class);

    PushUndoPoints(cmew);
    for(i= 0; i < 4; i++)
    {
	if(cmew->color_map_editor.g.field[i]->map[0]->toggle_on)
	{
	    field = cmew->color_map_editor.g.field[i];
	}
    }
    DrawControlBoxes(field->map[0], field->map[0]->erase_gc);
    DrawLine(field->map[0]->line, field->map[0]->line->erase_gc);
    for(i= 0; i < color_map_editor_class->num_cut_points; i++)
    {
	AddControlPoint(field->map[0], 
		color_map_editor_class->cut_points[i].level, 
		color_map_editor_class->cut_points[i].value, 
		True);
    }
    SetArrayValues(field->map[0], field->line[0]);
    SetLineCoords(field->line[0]);
    DrawControlBoxes(field->map[0], field->map[0]->gc);
    DrawLine(field->map[0]->line, field->map[0]->line->gc);
    PrintAllControlPoints(cmew);
    update_callback(field);
}
/************************************************************************
 *
 *  XmColorMapCopy
 *
 ************************************************************************/
void XmColorMapCopy(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
XmColorMapEditorClassPart *color_map_editor_class;
ControlField *field;
ControlMap *map;
int i;
int j;
int k;

    color_map_editor_class = &(xmCMEWidgetClass->color_map_editor_class);
    if(color_map_editor_class->cut_points)
	XtFree((char*)color_map_editor_class->cut_points);
    for(i= 0; i < 4; i++)
    {
	field = cmew->color_map_editor.g.field[i];
	map = field->map[0];
	if(map->toggle_on)
	{
	    if(map->grabbed >= 0)
	    {
		color_map_editor_class->num_cut_points = 1;
		color_map_editor_class->cut_points = 
		    (ControlPoint *)XtCalloc(1, sizeof(ControlPoint));
		color_map_editor_class->cut_points[0]=map->points[map->grabbed];
	    }
	    else
	    {
		color_map_editor_class->num_cut_points = 0;
		for(j = 0; j < map->num_points; j++)
		{
		    if(map->points[j].multi_point_grab)
		    {
			color_map_editor_class->num_cut_points++;
		    }
		}
		color_map_editor_class->cut_points = (ControlPoint *)
			XtCalloc(color_map_editor_class->num_cut_points, 
				 sizeof(ControlPoint));
		k = 0;
		for(j = 0; j < map->num_points; j++)
		{
		    if(map->points[j].multi_point_grab)
		    {
			color_map_editor_class->cut_points[k++]=map->points[j];
		    }
		}
	    }
	}
    }
}
/************************************************************************
 *
 *  XmColorMapSelectAllCP
 *
 ************************************************************************/
void XmColorMapSelectAllCP(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
ControlField *field;
ControlMap *map;
int i;
int j;

    for(i= 0; i < 4; i++)
    {
	field = cmew->color_map_editor.g.field[i];
	map = field->map[0];
	if(map->toggle_on)
	{
	    DrawControlBoxes(map, map->erase_gc);
	    DrawLine(map->line, map->line->erase_gc);
	    map->grabbed = -1;
	    for(j = 0; j < map->num_points; j++)
	    {
		map->points[j].multi_point_grab = True;
	    }
	    DrawControlBoxes(map, map->gc);
	    DrawLine(map->line, map->line->gc);
	}
    }
    CallActivateCallback(cmew, XmCR_SELECT);
    PrintAllControlPoints(cmew);
    return;
}
#ifdef SPLINE
/************************************************************************
 *
 *  XmColorMapSpline
 *
 ************************************************************************/
void XmColorMapSpline(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
ControlField *field;
ControlMap *map;
ControlLine *line;
int i;
int j;
int k;
double *s;
double a, b, c, d;
double x1;
int interval;
double h;
double si;
double si1;
double yi;
double yi1;
double *x;
double *y;
double value;
double xstart;
double xend;
double value_range;
int margin;


    PushUndoPoints(cmew);
    for(i= 0; i < 4; i++)
    {
	if(cmew->color_map_editor.g.field[i]->map[0]->toggle_on)
	{
	    field = cmew->color_map_editor.g.field[i];
	    map = field->map[0];
	    line = map->line;
	}
    }
    value_range = (double)(line->field->width - 1);
    margin = line->field->left_margin;

    DrawControlBoxes(field->map[0], field->map[0]->erase_gc);
    DrawLine(field->map[0]->line, field->map[0]->line->erase_gc);

    x = (double *)XtCalloc(map->num_points, sizeof(double));
    y = (double *)XtCalloc(map->num_points, sizeof(double));
    s = (double *)XtCalloc(map->num_points, sizeof(double));
    for(i = 0; i < map->num_points; i++)
    {
	x[i] = map->points[i].level;
	y[i] = map->points[i].value;
    }

    cubic_spline(x, y, s, map->num_points, 1);

    xstart = 0.0;
    xend   = 1.0;
    for(i= 0; i < map->line->num_levels; i++)
    {
	/*
	 * Break the space between the first and last control point into 50
	 * evenly spaced intervals.
	 */
	x1 = xstart + (xend - xstart)*(double)i/(double)map->line->num_levels;

	/*
	 * Locate the interval we are delealing with
	 */
	for(interval = 0; interval < map->num_points; interval++)
	{
	    if( (x[interval] <= x1) && x[interval+1] >= x1) break;
	}
	si = s[interval];
	si1 = s[interval+1];
	h = x[interval+1] - x[interval];
	yi1 = y[interval+1];
	yi = y[interval];

	a = (si1 - si)/(6*h);
	b = si/2;
	c = (yi1 - yi)/h - (2*h*si + h*si1)/6;
	d = yi;

	x1 = x1 - x[interval];
	value = MAX(0.0, a*x1*x1*x1 + b*x1*x1 + c*x1 + d);
	value = MIN(1.0, value);
	map->line->values[i] = value;

	line->points[i].x = (value * (map->field->width - 1))
	  + map->field->left_margin;
	line->points[i].y = ((1.0 -  (double)(x1 + x[interval])) * 
		(map->field->height - 1))
	  + map->field->top_margin; 
    }
    line->num_points = map->line->num_levels;
    DrawControlBoxes(field->map[0], field->map[0]->gc);
    DrawLine(field->map[0]->line, field->map[0]->line->gc);
    PrintAllControlPoints(cmew);
    update_callback(field);

    XtFree(x);
    XtFree(y);
    XtFree(s);
}
/************************************************************************
 *
 *  XmColorMapSplinePoints - This version creates a spline curve by adding
 *		       50 ControlPoints at the appropriate levels & values
 *
 ************************************************************************/
void XmColorMapSplinePoints(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
ControlField *field;
ControlMap *map;
int i;
int j;
int k;
double *s;
double a, b, c, d;
double x1;
int interval;
double h;
double si;
double si1;
double yi;
double yi1;
double *x;
double *y;
double value;
double xstart;
double xend;

    PushUndoPoints(cmew);
    for(i= 0; i < 4; i++)
    {
	if(cmew->color_map_editor.g.field[i]->map[0]->toggle_on)
	{
	    field = cmew->color_map_editor.g.field[i];
	    map = field->map[0];
	}
    }
    DrawControlBoxes(field->map[0], field->map[0]->erase_gc);
    DrawLine(field->map[0]->line, field->map[0]->line->erase_gc);

    x = (double *)XtCalloc(map->num_points, sizeof(double));
    y = (double *)XtCalloc(map->num_points, sizeof(double));
    s = (double *)XtCalloc(map->num_points, sizeof(double));
    for(i = 0; i < map->num_points; i++)
    {
	x[i] = map->points[i].level;
	y[i] = map->points[i].value;
    }

    cubic_spline(x, y, s, map->num_points, 1);

    xstart = x[0];
    xend   = x[map->num_points-1];
    for(i= 0; i < 50; i++)
    {
	/*
	 * Break the space between the first and last control point into 50
	 * evenly spaced intervals.
	 */
	x1 = xstart + (xend - xstart)*i/50.0;

	/*
	 * Locate the interval we are delealing with
	 */
	for(interval = 0; interval < map->num_points; interval++)
	{
	    if( (x[interval] <= x1) && x[interval+1] >= x1) break;
	}
	si = s[interval];
	si1 = s[interval+1];
	h = x[interval+1] - x[interval];
	yi1 = y[interval+1];
	yi = y[interval];

	a = (si1 - si)/(6*h);
	b = si/2;
	c = (yi1 - yi)/h - (2*h*si + h*si1)/6;
	d = yi;

	x1 = x1 - x[interval];
	value = MAX(0.0, a*x1*x1*x1 + b*x1*x1 + c*x1 + d);
	value = MIN(1.0, value);
	AddControlPoint(map, (double)(x1 + x[interval]), value, True);
    }
    SetArrayValues(field->map[0], field->line[0]);
    SetLineCoords(field->line[0]);
    DrawControlBoxes(field->map[0], field->map[0]->gc);
    DrawLine(field->map[0]->line, field->map[0]->line->gc);
    PrintAllControlPoints(cmew);
    update_callback(field);

    XtFree(x);
    XtFree(y);
    XtFree(s);
}
#endif
/************************************************************************
 *
 *  XmColorMapUndoable - Is there anything to undo?
 *
 ************************************************************************/
Boolean XmColorMapUndoable(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
ControlField *field;
ControlMap *map;
int i;

    /*
     * In practice, all four maps have the same number of undoable operations
     */
    for(i= 0; i < 4; i++)
    {
	field = cmew->color_map_editor.g.field[i];
	map = field->map[0];
	if(map->num_undoable > 0)
	{
	    return True;
	}
    }
    return False;
}
/************************************************************************
 *
 *  XmColorMapUndo - Undo an operation
 *
 ************************************************************************/
void XmColorMapUndo(Widget w)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
ControlField *field;
ControlMap *map;
int i;

    for(i= 0; i < 4; i++)
    {
	field = cmew->color_map_editor.g.field[i];
	map = field->map[0];
	if(map->num_undoable > 0)
	{
	    DrawControlBoxes(map, map->erase_gc);
	    DrawLine(map->line, map->line->erase_gc);

	    XtFree((char*)map->points);
	    XtFree((char*)map->boxes);

	    map->points = map->undo_stack[map->undo_top].undo_points;
	    map->boxes  = map->undo_stack[map->undo_top].undo_boxes ;
	    map->grabbed = -1;
	    map->num_points = map->undo_stack[map->undo_top].num_undo_points;
	    map->size_buf = map->undo_stack[map->undo_top].undo_size_buf;

	    SetArrayValues(map, field->line[0]);
	    SetLineCoords(field->line[0]);
	    DrawControlBoxes(map, map->gc);
	    DrawLine(map->line, map->line->gc);
	    update_callback(field);

	    map->undo_stack[map->undo_top].undo_points = NULL;
	    map->undo_stack[map->undo_top].undo_boxes = NULL;
	    map->undo_stack[map->undo_top].num_undo_points = -1;

	    map->undo_top--;
	    if(map->undo_top < 0) map->undo_top = UNDO_STACK_SIZE-1;
	    map->num_undoable--;
	}
    }
    CallActivateCallback(cmew, XmCR_MODIFIED);
    PrintAllControlPoints(cmew);
}

/************************************************************************
 *
 *  PushUndoPoints - Save points to be undone later
 *
 ************************************************************************/
void PushUndoPoints(XmColorMapEditorWidget cmew)
{
ControlField *field;
ControlMap *map;
int i;
int j;

    for(i= 0; i < 4; i++)
    {
	field = cmew->color_map_editor.g.field[i];
	map = field->map[0];
	map->undo_top++;
	if(map->undo_top >= UNDO_STACK_SIZE) map->undo_top = 0;
	if(map->num_undoable == UNDO_STACK_SIZE)
	{
	    if(map->undo_stack[map->undo_top].undo_points)
		XtFree((char*)map->undo_stack[map->undo_top].undo_points);
	    if(map->undo_stack[map->undo_top].undo_boxes)
		XtFree((char*)map->undo_stack[map->undo_top].undo_boxes);
	}
	else
	{
	    map->num_undoable++;
	}
	map->undo_stack[map->undo_top].undo_points = (ControlPoint *)
	    XtCalloc(map->size_buf, sizeof(ControlPoint));
	map->undo_stack[map->undo_top].undo_boxes = 
	    (XRectangle *)XtCalloc(map->size_buf, sizeof(XRectangle));
	for(j = 0; j < map->size_buf; j++)
	{
	    map->undo_stack[map->undo_top].undo_points[j] = map->points[j];
	    map->undo_stack[map->undo_top].undo_boxes[j] = map->boxes[j];
	}
	map->undo_stack[map->undo_top].num_undo_points = map->num_points;
	map->undo_stack[map->undo_top].undo_size_buf = map->size_buf;
    }
}
void XmColormapSetHSVSensitive(Widget w, Boolean b)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget) w;

    if(!XtIsRealized(w)) return;

    XtSetSensitive(cmew->color_map_editor.g.field[0]->toggle_b, b);
    XtSetSensitive(cmew->color_map_editor.g.field[1]->toggle_b, b);
    XtSetSensitive(cmew->color_map_editor.g.field[2]->toggle_b, b);

    XtSetSensitive((Widget)cmew->color_map_editor.g.field[0]->w, b);
    XtSetSensitive((Widget)cmew->color_map_editor.g.field[1]->w, b);
    XtSetSensitive((Widget)cmew->color_map_editor.g.field[2]->w, b);

    if(!b)
	XmToggleButtonSetState(cmew->color_map_editor.g.field[3]->toggle_b, 
	    True, True);
}
void XmColormapSetOpacitySensitive(Widget w, Boolean b)
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget) w;

    if(!XtIsRealized(w)) return;

    XtSetSensitive(cmew->color_map_editor.g.field[3]->toggle_b, b);

    XtSetSensitive((Widget)cmew->color_map_editor.g.field[3]->w, b);

    if(!b)
	XmToggleButtonSetState(cmew->color_map_editor.g.field[0]->toggle_b, 
	    True, True);
}
