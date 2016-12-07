/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ControlField.h"
#include <X11/keysym.h>		/* Define XK_Left, XK_Right */
#include <Xm/DrawP.h>
#include "ColorMapEditorP.h"

GC InitGC( XmDrawingAreaWidget w, Pixel color );
static void ExposeCallback( Widget w, ControlField* field,
			    XmDrawingAreaCallbackStruct* cb );
static void ResizeCallback( XmDrawingAreaWidget w, ControlField* field,
			    XmDrawingAreaCallbackStruct* cb );
static void FieldInput( Widget w, ControlField* field,
		        XmDrawingAreaCallbackStruct* call_data );
extern void update_callback( ControlField* field );
extern void SetColorCoords( XColor *cells, ControlField* field,
			    short* load, short load_limit );
static GC GetRubberbandGC(XmColorMapEditorWidget cmew);


/* External Functions */
void SetRGBLines( XColor* cells, ControlField* field,
                  ControlLine* h, ControlLine* s, ControlLine* v ); /* From ColorRGB.c */


/*  Subroutine:	FieldActionProc
 *  Purpose:	Installable event action proc for button input to control
 *		markers
 */
void FieldActionProc( XmDrawingAreaWidget w, XtPointer client_data, 
		XEvent* event, Boolean *continue_to_dispatch)
{
    ControlField* field;
    XmDrawingAreaCallbackStruct call_data;
    if( (field = (ControlField *)w->manager.user_data) )
    {
	call_data.event = event;
	FieldInput((Widget)w, field, &call_data);
    }
}


/*  Subroutine:	CreateControlField
 *  Purpose:	Create a DrawingArea widget and structure for a control field
 */
ControlField* CreateControlField( Widget parent, char *name, short num_levels,
				  short x, short y, short width, short height,
				  Arg* args, int num_args )
{
    ControlField* field;
    Arg wargs[40];

    field = (ControlField *)XtMalloc(sizeof(struct ControlFieldRec));
    (void)memcpy(wargs, args, num_args * sizeof(Arg));
    field->width = width - (MARGIN + MARGIN);
    field->height = height - (MARGIN + MARGIN);
    field->left_margin = MARGIN;
    field->right_margin = MARGIN;
    field->top_margin = MARGIN;
    field->bottom_margin = MARGIN;
    field->num_maps = 0;
    field->num_lines = 0;
    field->levels = NULL;
    field->coords = NULL;
    field->num_levels = 0;
    field->input_mode = 0;
    field->vertical = TRUE;
    field->expose_callback = NULL;
    field->user_callback = NULL;
    field->init = TRUE;
    field->cmew = (XmColorMapEditorWidget)parent;
    field->rubber_band_gc = GetRubberbandGC(field->cmew);
    /*  Create and set up reference value arrays  */
    InitializeLevels(field, num_levels);
    XtSetArg(wargs[num_args], XmNx, x); num_args++;
    XtSetArg(wargs[num_args], XmNy, y); num_args++;
    XtSetArg(wargs[num_args], XmNwidth, width);num_args++;
    XtSetArg(wargs[num_args], XmNheight, height); num_args++;
    field->w = 
	(XmDrawingAreaWidget)XmCreateDrawingArea(parent, name, wargs, 
						num_args);
    XtAddCallback((Widget)field->w, XmNexposeCallback, (XtCallbackProc)ExposeCallback, field);
    XtAddCallback((Widget)field->w, XmNresizeCallback, (XtCallbackProc)ResizeCallback, field);
    XtAddCallback((Widget)field->w, XmNinputCallback, (XtCallbackProc)FieldInput, (XtPointer)field);
    XtAddEventHandler((Widget)field->w, Button1MotionMask, False, (XtEventHandler)FieldActionProc,NULL);
    field->w->manager.user_data = (XtPointer)(field);
    XtManageChild((Widget)field->w);
    return field;
}

/*  Subroutine: GetRubberbandGC
 *  Purpose:    Get a GC with the right resources for drawing the bounding box
 */
static GC GetRubberbandGC(XmColorMapEditorWidget cmew)
{
    XGCValues values;
    XtGCMask  valueMask;

    values.graphics_exposures = False;
    values.foreground = cmew->manager.foreground ^ cmew->core.background_pixel;
    values.background = 0;
    values.function = GXxor;
    values.line_style = LineOnOffDash;
    valueMask = GCForeground | GCBackground | GCGraphicsExposures
        | GCFunction | GCLineStyle;
    return (XtGetGC((Widget)cmew, valueMask, &values));
}


/*  Subroutine:	InitGC
 *  Purpose:	Create a gc for drawing lines of control point hash boxes
 */
GC InitGC( XmDrawingAreaWidget w, Pixel color )
{
    XGCValues	values;
    XtGCMask	valueMask;

    values.graphics_exposures = False;
    values.foreground = color;
    valueMask = GCForeground | GCGraphicsExposures;
    return XtGetGC((Widget)w, valueMask, &values);
}


/*  Subroutine:	ExposeCallback
 *  Purpose:	Redraw things needing to be redrawn when window newly exposed
 */
static void ExposeCallback( Widget w, ControlField* field,
			    XmDrawingAreaCallbackStruct* cb )
{
int i;
static Boolean init = TRUE;
Boolean draw_lines = TRUE;

    if( XtIsRealized((Widget)field->w) )
    {
	if( field->init )
	    {
	    if(field->num_lines == 3)	
		{
	        if(field->line[0]->gc ==NULL)
		    field->line[0]->gc = 
			InitGC((XmDrawingAreaWidget)w, field->line[0]->color);
	        if(field->line[0]->erase_gc ==NULL)
		    field->line[0]->erase_gc = 
			InitGC((XmDrawingAreaWidget)w,w->core.background_pixel);
	        if(field->line[1]->gc ==NULL)
		    field->line[1]->gc = 
			InitGC((XmDrawingAreaWidget)w, field->line[1]->color);
	        if(field->line[1]->erase_gc ==NULL)
		    field->line[1]->erase_gc = 
			InitGC((XmDrawingAreaWidget)w,w->core.background_pixel);
	        if(field->line[2]->gc ==NULL)
		    field->line[2]->gc = 
			InitGC((XmDrawingAreaWidget)w, field->line[2]->color);
	        if(field->line[2]->erase_gc ==NULL)
		    field->line[2]->erase_gc = 
			InitGC((XmDrawingAreaWidget)w,w->core.background_pixel);
		SetRGBLines(
		    &(field->cmew->color_map_editor.g.color->undithered_cells[0]), 
		    field,
                    field->cmew->color_map_editor.g.field[0]->line[0], 
		    field->cmew->color_map_editor.g.field[1]->line[0], 
		    field->cmew->color_map_editor.g.field[2]->line[0]);
		draw_lines = False;
		}
	    field->init = FALSE;
	    }
	if( init )
	    {
	    init = False;
	    }
	else 
	    {
	    XClearWindow(XtDisplay(w), XtWindow(w));
	    }

	/*  Call an application installed callback for initial dressing  */
	if( field->expose_callback )
	    field->expose_callback((Widget)w, field, cb);
	for( i=0; i<field->num_maps; i++ )
	{
	    if( field->map[i]->gc == NULL )
		field->map[i]->gc = 
		    InitGC((XmDrawingAreaWidget)w, field->map[i]->color);
	    if( field->map[i]->erase_gc == NULL )
		field->map[i]->erase_gc = 
		    InitGC((XmDrawingAreaWidget)w, w->core.background_pixel);
	    RedrawControlBoxes(field->map[i]);
	}
	if(draw_lines)
	{
	    for( i=0; i<field->num_lines; i++ )
	    {
		if( field->line[i]->gc == NULL )
		    field->line[i]->gc = 
			InitGC((XmDrawingAreaWidget)w, field->line[i]->color);
		if( field->line[i]->erase_gc == NULL )
		    field->line[i]->erase_gc = 
			InitGC((XmDrawingAreaWidget)w,w->core.background_pixel);
		DrawLine(field->line[i], field->line[i]->gc);
	    }
	}
#if (OLD_LESSTIF != 1)
#if    (XmVersion < 2000)
	_XmDrawShadowType((Widget)field->w, XmSHADOW_IN,
		field->w->core.width, field->w->core.height,
		field->w->manager.shadow_thickness,
		0, field->w->manager.top_shadow_GC,
		field->w->manager.bottom_shadow_GC);
#else    /* XmVersion >= 2000 */
	XmeDrawShadows(XtDisplay((Widget)field->w), XtWindow((Widget)field->w),
			field->w->manager.top_shadow_GC,
			field->w->manager.bottom_shadow_GC,
			0, 0,
			field->w->core.width,
			field->w->core.height,
			field->w->manager.shadow_thickness,
			XmSHADOW_IN);
#endif    /* XmVersion */
#endif /* OLD_LESSTIF */
    }
}


/*  Subroutine:	ResizeCallback
 *  Purpose:	Clear the old image and draw a new one
 */
static void ResizeCallback( XmDrawingAreaWidget w, ControlField* field,
			    XmDrawingAreaCallbackStruct* cb )
{
    int i;

    field->width = w->core.width - (field->left_margin + field->right_margin);
    field->height = w->core.height - (field->top_margin + field->bottom_margin);
    InitializeLevels(field, field->num_levels);
    for( i=0; i<field->num_maps; i++ )
	ResizeControlBoxes(field->map[i]);
    for( i=0; i<field->num_lines; i++ )
	SetLineCoords(field->line[i]);
    ExposeCallback((Widget)w, field, cb);
    if ( (field->num_lines == 3) && (XtIsRealized((Widget)field->w)) )
	{
	field->user_callback(field);
	}
}


/*  Subroutine:	FieldInput
 *  Purpose:	Handle input of button events in field window
 */
static void FieldInput( Widget w, ControlField* field,
		        XmDrawingAreaCallbackStruct* call_data )
{
    Boolean change;

    /*  User input can only be fielded by (or for) ControlPoint maps  */
    if( (field->num_maps <= 0) || (field->input_mode >= field->num_maps) )
	return;
    if(   (call_data->event->type == ButtonPress)
       || (call_data->event->type == MotionNotify)
       || (call_data->event->type == ButtonRelease) )
    {
	/* Will always be zero if there is one graph per map - jgv */
	switch( field->input_mode )
	{
	  case 0:			/* MapInput() is in ControlPoint.c */
	    change = MapInput(w, field->map[0], call_data);
	    break;
	  case 1:
	    change = MapInput(w, field->map[1], call_data);
	    break;
	  case 2:
	    change = MapInput(w, field->map[2], call_data);
	    break;
	  case 3:
	    change = MapInput(w, field->map[3], call_data);
	    break;
	  default:
	    change = FALSE;
	    return;
	}
	if( change && field->user_callback )
	    field->user_callback(field);
    }
    /*  Kludge mechanism to select which line gets events  */
    /*  This was used when we tried all three lines in the same window  */
    else if( call_data->event->type == KeyPress )
    {
	KeySym keysym;			/* X code for key that was struck    */
	XComposeStatus compose;
	char string[256];	/* buffer to recieve ASCII code	     */
	/*num_chars =*/ XLookupString((XKeyEvent*)call_data->event, string,
                                sizeof(string), &keysym, &compose);
	switch( keysym )
	{
	  case XK_H:
	  case XK_h:
	    field->input_mode = HUE;		/* 0 */
	    break;
	  case XK_S:
	  case XK_s:
	    field->input_mode = SATURATION;	/* 1 */
	    break;
	  case XK_V:
	  case XK_v:
	    field->input_mode = VALUE;		/* 2 */
	    break;
	}
    }
}
	

/*  Subroutine:	InitializeLevels
 *  Purpose:	Set the fixed parameters of the drawn line for the current
 *		dimensions
 */
void InitializeLevels( ControlField* field, int num_levels )
{
    double span, position;
    double level, level_increment;
    int clicks, i, last_level_index;
    LineCoord* end_coord;
    register LineCoord* coord;

    if( num_levels > field->num_levels )
    {
	if( field->levels )
	    XtFree((char*)field->levels);
	field->levels = (double *)XtMalloc(num_levels * sizeof(double));
	if( field->coords )
	    XtFree((char*)field->coords);
	field->coords = (LineCoord *)XtMalloc(num_levels * sizeof(LineCoord));
    }
    field->num_levels = num_levels;
    /*  Initialize the level reference values */
    last_level_index = num_levels - 1;
    level_increment = 1.0 / (double)last_level_index;
    for( i=0, level = 0.0; i<num_levels; i++, level += level_increment )
	field->levels[i] = level;
    clicks = 2 * last_level_index;
    if( field->vertical )
    {
	span = -((double)field->height / (double)clicks);
	field->coords[0].low = field->top_margin + (field->height - 1);
	field->coords[last_level_index].high = field->top_margin;
    }
    else
    {
	span = (double)field->width / (double)clicks;
	field->coords[0].low = field->left_margin;
	field->coords[last_level_index].high =
	  field->left_margin + field->width;
    }
    /*  Stuff at field->coords[0]  */
    coord = field->coords;
    coord->mid = coord->low;
    position = (double)coord->low + span;
    coord->high = (int)(position + 0.5);
    end_coord = coord + last_level_index;
    while( ++coord < end_coord )
    {
	coord->low = (int)(position + 0.5);
	position += span;
	coord->mid = (int)(position + 0.5);
	position += span;
	coord->high = (int)(position + 0.5);
    }
    /*  Stuff at field->coords[last_level_index]  */
    coord->mid = coord->high;
    coord->low = (int)(0.5 + (double)coord->mid - span);
}
