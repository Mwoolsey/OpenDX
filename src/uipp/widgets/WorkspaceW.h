/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _Workspace_h
#define _Workspace_h

#include "XmDX.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WidgetClass xmWorkspaceWidgetClass;
typedef struct _XmWorkspaceClassRec * XmWorkspaceWidgetClass;
typedef struct _XmWorkspaceRec      * XmWorkspaceWidget;
typedef struct _XmWorkspaceLine     * XmWorkspaceLine;

typedef struct
{
    int         reason;
    XEvent *    event;
    Widget      source;
    Widget      destination;
    int		srcx;
    int		srcy;
    int		dstx;
    int		dsty;
    int		failnum;
} XmWorkspaceErrorCallbackStruct;

typedef struct
{
    int         reason;
    XEvent *    event;
    Window      window;
    Widget      selected_widget;
} XmWorkspaceCallbackStruct;

typedef struct
{
    int         reason;
    XEvent *    event;
    Boolean     status;
    Dimension   width;
    Dimension   height;
} XmWorkspaceChildCallbackStruct;

typedef struct
{
    Widget      child;
    XEvent *    event;
    Dimension	x,y;
} XmWorkspacePositionChangeCallbackStruct;

/*
 *  Define create convenience function and necessary utility function
 */
extern Widget XmCreateWorkspace (Widget parent,
	                         char* name,
	                         Arg arglist[],
	                         int argCount);

/*
 *  Use this function to add callbacks to child's constraints
 *  (Xt isn't supporting callbacks in constraints as of R4)
 */
extern void XmWorkspaceAddCallback (Widget child,
	                            String name,
	                            XtCallbackProc callback,
	                            XtPointer client_data);

/*
 *  Use these functions to establish a child-to-child line and to remove it
 */
XmWorkspaceLine XmCreateWorkspaceLine (XmWorkspaceWidget ww,
	                               int color,
	                               Widget source,
	                               short src_x_offset,
	                               short src_y_offset,
	                               Widget dest,
	                               short dst_x_offset,
	                               short dst_y_offset);
/*  Middle_of_group flag, when True, suppresses redraw of area around line  */
/*  Middle_of_group flag False, or NULL line, redraws accumulated areas     */
void XmDestroyWorkspaceLine (XmWorkspaceWidget ww,
	                     XmWorkspaceLine line,
	                     Boolean middle_of_group);

/*
 * Return the number of distinct points in the path as the return value.
 * Place in the x and y arrays, the list of connecting points.
 * The return arrays x and y, must be XtFree()'d by the caller.
 */
int XmWorkspaceLineGetPath(XmWorkspaceLine wl, int **x, int **y);


void XmAddWorkspaceEventHandler (XmWorkspaceWidget ww,
	                         Widget owner,
	                         XtEventHandler handler,
	                         XtPointer client_data);

void XmWorkspaceGetMaxWidthHeight(Widget, int *, int *);

Boolean XmWorkspaceLocationEmpty (Widget , int , int );
Boolean XmWorkspaceRectangleEmpty (Widget , int , int , int , int );
Boolean XmWorkspaceSetLocation (Widget w, Dimension x, Dimension y);

#define XmNlineInvisibility "lineInvisibility"
#define XmCLineInvisibility "LineInvisibility"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
