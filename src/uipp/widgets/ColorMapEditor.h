/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _XmColorMapEditor_h
#define _XmColorMapEditor_h

#include "XmDX.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


extern WidgetClass xmColorMapEditorWidgetClass;

typedef struct _XmColorMapEditorClassRec * XmColorMapEditorWidgetClass;
typedef struct _XmColorMapEditorRec      * XmColorMapEditorWidget;
     
typedef struct {
    double min_value;
    double max_value;
    int reason;
    int hue_selected;
    int sat_selected;
    int val_selected;
    int op_selected;
    int num_points;
    int num_hue_points;
    int num_sat_points;
    int num_val_points;
    int num_op_points;
    double *data;
    double *op_data;
    double *hue_values;
    double *sat_values;
    double *val_values;
    double *op_values;
    int selected_area;
} XmColorMapEditorCallbackStruct;
     
typedef struct {
    int		field;
    int		count;
    double	*level;
    double	*value;
} ControlPointStruct;


/*  Declare types for convenience routine to create the widget  */
extern 
Widget XmCreateColorMapEditor (Widget parent, String name, 
		ArgList args, Cardinal num_args);

extern 
void CMEStep(Widget w, int nsteps, Boolean use_selected);

extern 
void CMESquareWave(Widget w, int nsteps, Boolean start_on_left, 
		   Boolean use_selected);

extern 
void CMESawToothWave(Widget w, int nsteps, Boolean start_on_left, 
		   Boolean use_selected);


extern 
void CMEAddControlPoint(Widget w, double level, double value, Boolean above);

extern
void XmColorMapEditorLoad(Widget cmew, double min, double max, 
			  int nhues, double *hues,
			  int nsats, double *sats,
			  int nvals, double *vals,
			  int nopat, double *opat);

extern 
void XmColorMapEditorLoadHistogram(Widget w, int *bins, int num_bins);

extern 
Boolean XmColorMapEditorHasHistogram(Widget w);


extern 
void XmColorMapReset(Widget w);


extern 
void XmColorMapSelectAllCP(Widget w);


extern 
void XmColorMapEditorDeleteCP(Widget w);


extern 
Boolean XmColorMapEditorSave(Widget w, char *filename);


extern 
Boolean XmColorMapEditorRead(Widget w, char *filename);

#ifdef SPLINE

extern 
void XmColorMapSpline(Widget w);


extern 
void XmColorMapSplinePoints(Widget w);
#endif

extern 
Boolean XmColorMapUndoable(Widget w);
extern 
void XmColorMapUndo(Widget w);

extern
Boolean XmColorMapPastable();

extern
void XmColorMapPaste(Widget w);

extern
void XmColorMapCopy(Widget w);

extern
void XmColormapSetHSVSensitive(Widget w, Boolean b);
extern
void XmColormapSetOpacitySensitive(Widget w, Boolean b);

#define HUE 0
#define SATURATION 1
#define VALUE 2
#define OPACITY 3

#define CME_GRID 0
#define CME_HISTOGRAM 1
#define CME_LOGHISTOGRAM 2

#define XmPRINT_SELECTED 0
#define XmPRINT_ALL 1
#define XmPRINT_NONE 2

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
