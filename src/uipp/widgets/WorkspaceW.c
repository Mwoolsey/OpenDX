/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <stdio.h> /* for sprintf */
#include <ctype.h>              /*  define isupper()  */
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/XmP.h>
#include <Xm/Xm.h>
#include "WorkspaceW.h"
#include "WorkspaceP.h"
#include "WorkspaceCallback.h"
#include <string.h>
#include <X11/keysym.h>

#if !defined(XK_MISCELLANY)
#define XK_MISCELLANY 1
#endif


/* External functions */
extern void ChangeWidgetInCollideList(); /* From Findroute.c */

extern void _XmManagerEnter();
extern void _XmManagerFocusIn();

#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))

/*
 * Explanation of default Actions in response to mouse pressing and motion:
 *
 * Action started on background:
 *
 *  ~SHIFT Button1Down: Start rubberband
 *                      Unhighlight previously selected children
 *
 *  SHIFT Button1Down:  Start rubberband
 *
 *  (~SHIFT)Button1Move:        Stretch rubberband
 *                      (A) Highlight children as included
 *                      (A) Unhighlight children as unincluded
 *
 *  (SHIFT)Button1Move: Stretch rubberband
 *                      (A) Toggle highlight of children as included
 *                      (A) Toggle highlight of children as unincluded
 *
 *  (~SHIFT)Button1Up:  Hide rubberband
 *                      (B) Highlight new included children
 *                      Callback w/ unselect new excluded children
 *                      Callback w/ select new included children
 *
 *  (SHIFT)Button1Up:   Hide rubberband
 *                      (B) Toggle highlight of all included children
 *                      Callback w/ unselect new unhighlighted children
 *                      Callback w/ select new highlighted children
 *
 * Action started in child:
 *
 *  If child was not already selected:
 *    ~SHIFT Button1Down:         Unhighlight prior highlighted children
 *                        Highlight child
 *                        Callback w/ unselect to prior selected children
 *                        Callback w/ select to child
 *                        enable move
 *
 *    SHIFT Button1Down:          Highlight child
 *                        Callback w/ select to child
 *                        enable move
 *
 *  If child was already selected:
 *    ~SHIFT Button1Down:   enable move
 *
 *    SHIFT Button1Down:    Unhighlight child
 *                        Callback w/ unselect to child
 *                        disable move
 *
 *  Button1Move:                if move enabled:
 *                        (A) Move outline of each selected child
 *                        (B) Move bounding box of selected children extremes
 *                        (C) (B) plus outline of grabbed child
 *
 *  Button1Up:          if move enabled:
 *                      Hide rubberband(s)
 *                      Move selected children
 *                      (?) Action to update graphics
 *
 */


/*
 * Explanation of Automated widget movement applied when widget overlap is
 * not allowed:
 *
 *  Space wars rules:
 *
 *    1. No moved widget may overlap any other.
 *    2. Movement to clear overlaps is to the right only
 *    3. Any widget moved, whether by user or by space wars, will be placed
 *       according to all existing gridding rules.
 *    4. All adjustment to avoid overlap is done by moving widgets to the right.
 *
 *  Option Rules:
 *   (Option 2 is the default)
 *   (Option 0 may be a bit more efficient if only 1 child can be moved at
 *     a time).
 *    0. Automated movement for space wars does not change horizontal widget
 *       ordering (established at time selected children are dropped).
 *    1. a. The left-most selected child maintains its horizontal order as
 *          placed.
 *       b. All other selected children maintain their position relative to
 *          that left-most child
 *    2. a. Selected widgets are placed (and snapped-to-grid if called for) with
 *          no consideration of other widgets.
 *       b. All other widgets move right to accomodate new placement.
 *       c. Movement of non-selected widgets does not alter the horizontal
 *       ordering among those non-selected widgets (e.g. a small child to
 *          the right of a large one will NOT be moved ahead of it to fill a
 *          convenient hole too small for the larger one).
 *
 *  Rules of Horizontal Ordering
 *    1. Horizontal ordering is based on the x coordinate of each widget's
 *       left edge, center, or right edge as per alignment_policy.
 *    2. a. When two widgets have the same x coordinate for alignment, the
 *          selected widget is ordered before one that is not selected.
 *       b. When both widgets have the same x coordinate and selection status,
 *          the ordering will be based on the y coordinates as per alignment.
 */


/*  Define structure of link elements in a sort list of child widgets  */
struct SortRec {
    struct SortRec* next;
    Widget child;
    XmWorkspaceConstraints constraints;
    short x_left, x_right;
    short y_upper, y_lower;
    int x_index;
    Boolean is_selected;
    Boolean is_displaced;
};

/*  Parameter to control size of arms of alignment hash marks: ray+1+ray  */
#define HASH_RAY 4
#define GRID_DASH_MIN 10

/*  Codes and indexes for rubberbanding cursor  */
#define SZ_NONE 4
#define SZ_LEFT 0
#define SZ_RIGHT 1
#define SZ_UPPER 0
#define SZ_LOWER 2
#define SZ_UL 0
#define SZ_UR 1
#define SZ_LL 2
#define SZ_LR 3

#define FC_LEFT    0
#define FC_RIGHT   1
#define FC_TOP     2
#define FC_BOTTOM  3

#define superclass (&xmFormClassRec)

LineElement *Manhattan(XmWorkspaceWidget ww, int srcx, int srcy,
                              int dstx, int dsty,
                              int level, int *cost,
			      Widget source, Widget destination,
			      XmWorkspaceLine new, int *failnum);
void CopyPointsToLine(XmWorkspaceWidget ww,LineElement *list, 
						XmWorkspaceLine line);
extern void AddWidgetToCollideList(Widget child);
extern void HideWidgetInCollideList(XmWorkspaceWidget ww, Widget child);
extern void DeleteWidgetFromCollideList(Widget child);
extern void RemoveLineFromCollideList(XmWorkspaceWidget ww, 
					XmWorkspaceLine line);
extern void AddLineToCollideList(XmWorkspaceWidget ww, XmWorkspaceLine line);

static void     ClassInitialize         ();
static void     Initialize              (XmWorkspaceWidget request,
	                                 XmWorkspaceWidget new);
static void     Realize                 (XmWorkspaceWidget ww,
	                                 Mask* p_valueMask,
	                                 XSetWindowAttributes *attributes);
static void     Destroy                 (XmWorkspaceWidget ww);
static void     Redisplay               (XmWorkspaceWidget ww,
	                                 XExposeEvent* event,
	                                 Region region);
static void	Resize			(XmWorkspaceWidget ww);
static Boolean  RedisplayRectangle      (XmWorkspaceWidget ww,
	                                 XRectangle* rect,
	                                 Region region,
	                                 Boolean clip_region_set);
static Boolean  SetValues               (XmWorkspaceWidget current,
	                                 XmWorkspaceWidget request,
	                                 XmWorkspaceWidget new);
static void     ChangeManaged           (XmWorkspaceWidget ww);
static XtGeometryResult
	        _GeometryManager         (Widget w,
	                                 XtWidgetGeometry *request,
	                                 XtWidgetGeometry *reply,
	                                 Boolean adjust_constraints);
static XtGeometryResult
	        GeometryManager         (Widget w,
	                                 XtWidgetGeometry *request,
	                                 XtWidgetGeometry *reply);
static void     Arm                     (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     Drag                    (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     Disarm                  (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     KeyboardNavigation      (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     ConstraintInitialize    (Widget req,
	                                 Widget new);
static Boolean  ConstraintSetValues     (Widget current,
	                                 Widget request,
	                                 Widget new);
static void     ConstraintDestroy       (Widget w);
static void     GetRubberbandGC         (XmWorkspaceWidget ww);
static void     DrawRubberband          (XmWorkspaceWidget ww);
static void     GrabSelections          (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     MoveResizeSelections     (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     ChildNavigation         (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
#if RESIZE_HANDLES
static void     ResizeNE 	        (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     ResizeNW 	        (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     ResizeSE 	        (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     ResizeSW 	        (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     NewDrop 	        (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
#endif
static void     DropSelections          (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     RaiseSelections         (Widget w,
	                                 XEvent* event,
	                                 String* params,
	                                 Cardinal* num_params);
static void     MoveChild               (XmWorkspaceWidget ww,
	                                 Widget child,
	                                 XmWorkspaceConstraints constraints);
static void     ResetChild              (XmWorkspaceWidget ww,
	                                 XmWorkspaceConstraints constraints);
static void     SnapToGrid              (XmWorkspaceWidget ww,
	                                 Widget child,
	                                 XmWorkspaceConstraints constraints,
	                                 Boolean increase);
static void     Align1D                 (int* x,
	                                 int width,
	                                 int grid,
	                                 unsigned char alignment,
	                                 Boolean increase_only);
static void     UpdateRubberbandSelections
	                                (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     UnselectAll             (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     SelectChild             (XmWorkspaceWidget ww,
	                                 Widget child,
	                                 XmWorkspaceConstraints constraints,
	                                 XEvent* event);
static void     UnselectChild           (XmWorkspaceWidget ww,
	                                 Widget child,
	                                 XmWorkspaceConstraints constraints,
	                                 XEvent* event);
static void     StartRubberband         (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     StretchRubberband       (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     EndRubberband           (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     AccentChild             (XmWorkspaceWidget ww,
	                                 Widget child,
	                                 XmWorkspaceConstraints constraints,
	                                 XEvent* event);
static void     UnaccentChild           (XmWorkspaceWidget ww,
	                                 Widget child,
	                                 XmWorkspaceConstraints constraints,
	                                 XEvent* event);
static void     UnaccentAll             (XmWorkspaceWidget ww,
	                                 XEvent* event);
static void     RestackSelectedChildren (XmWorkspaceWidget ww);
static Boolean  ChildWindowIsSelected   (XmWorkspaceWidget ww,
	                                 Window child);
static void     SetGridBackground       (XmWorkspaceWidget ww, Boolean change);
static void     DrawGridMarks           (XmWorkspaceWidget ww,
	                                 GC gc,
	                                 Pixmap pixmap);
static void     OutlineChild            (XRectangle* surrogate,
	                                 XmWorkspaceConstraints constraints);
static void     SaveOutline             (XmWorkspaceWidget ww,
	                                 Widget grab_child);
static void     SaveOutlines            (XmWorkspaceWidget ww);
static void     DiscardSurrogate        (XmWorkspaceWidget ww);
static void     CreateSurrogate         (XmWorkspaceWidget ww,
	                                 Widget grab_child);
static void     DrawSurrogate           (XmWorkspaceWidget ww);
static void     MoveSurrogate           (XmWorkspaceWidget ww,
	                                 int delta_x,
	                                 int delta_y);
static void     ResizeSurrogate         (XmWorkspaceWidget ww, 
					 int width, 
					 int height);
static Boolean  PerformSpaceWars        (XmWorkspaceWidget ww,
	                                 Boolean change_managed,
					 XEvent* event);
static Boolean ResolveOverlaps         (XmWorkspaceWidget ww,
	                                 struct SortRec* sortlist,
	                                 Boolean* first_selected,
					 XEvent* event);
static Boolean  MoveIfDisplaced         (XmWorkspaceWidget ww,
	                                 struct SortRec *list,
	                                 int max_x,
	                                 int max_y);
static int      MoveAToRightOfB         (XmWorkspaceWidget ww,
	                                 struct SortRec* movee,
	                                 struct SortRec *mover,
					 XEvent* event);
static void  RepositionSelectedChildren (struct SortRec* current,
	                                 short delta_x);
static struct SortRec* GetSortList      (XmWorkspaceWidget ww,
	                                 Boolean change_managed,
	                                 struct SortRec** sortmem);
void     RerouteLines            (XmWorkspaceWidget ww,
	                                 Boolean reroute_all);
static void     SetLineRoute            (XmWorkspaceWidget ww,
	                                 XmWorkspaceLine new);
static Boolean  DestroyLine             (XmWorkspaceWidget ww,
	                                 XmWorkspaceLine old);
void  AugmentExposureAreaForLine (XmWorkspaceWidget ww,
	                                 XmWorkspaceLine line);
static void     UnsetExposureArea       (XmWorkspaceWidget ww);
void     RefreshLines            (XmWorkspaceWidget ww);
static void     InitLineGC              (XmWorkspaceWidget ww,
	                                 int color);
static void     CvtStringToWorkspaceType(XrmValue* args,
	                                 Cardinal num_args,
	                                 XrmValue* from_val,
	                                 XrmValue* to_val);
static Boolean  StringsAreEqual         (register char * in_str,
	                                 register char * test_str);
static void 	childRelative 		(XmWorkspaceWidget ww, 
					 XEvent *xev, int *x, int *y);
static Boolean Overlapping( XmWorkspaceWidget , int , int );

void ReallocCollideLists(XmWorkspaceWidget ww);
static void MyInsertChild( Widget w);
static void MyDeleteChild( Widget w);
extern void MarkCommonLines(XmWorkspaceWidget ww);
static void dropResizedSurrogates (XmWorkspaceWidget ww, int delta_x, int delta_y, XEvent* event);

#if (OLD_LESSTIF == 1)
#define GetFormConstraint(w) (&((XmFormConstraints) (w)->core.constraints)->form)
#else
#define GetFormConstraint(w) (&((XmFormConstraintPtr) (w)->core.constraints)->form)
#endif

static XtResource defaultResources[] =
{
    { XmNforceRoute, XmCForceRoute, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.force_route),
      XmRImmediate, (XtPointer) FALSE
    },
    {
      XmNcollisionSpacing, XmCCollisionSpacing, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceWidget, workspace.collision_spacing),
      XmRImmediate, (XtPointer) 20
    },
    {
      XmNhaloThickness, XmCHaloThickness, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceWidget, workspace.halo_thickness),
      XmRImmediate, (XtPointer) 2
    },
    {
      XmNpositionChangeCallback, XmCPositionChangeCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmWorkspaceWidget, workspace.position_change_callback),
      XmRCallback, NULL
    },
    {
      XmNerrorCallback, XmCErrorCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmWorkspaceWidget, workspace.error_callback),
      XmRCallback, NULL
    },
    {
      XmNbackgroundCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffset(XmWorkspaceWidget, workspace.background_callback),
      XmRPointer, (XtPointer) NULL
    },
    {
      XmNdefaultActionCallback, XmCCallback, XmRCallback,
      sizeof(XtCallbackList),
      XtOffset(XmWorkspaceWidget, workspace.action_callback),
      XmRPointer, (XtPointer) NULL
    },
    {
      XmNselectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList), 
      XtOffset(XmWorkspaceWidget, workspace.selection_change_callback),
      XmRPointer, (XtPointer) NULL
    },
    {
      XmNresizeCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList), 
      XtOffset(XmWorkspaceWidget, workspace.resize_callback),
      XmRPointer, (XtPointer) NULL
    },
    { XmNaccentColor, XmCAccentColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmWorkspaceWidget, workspace.accent_color),
      XmRImmediate, (XtPointer) -1
    },
    { XmNaccentPolicy, XmCAccentPolicy,
      XmRWorkspaceType, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.accent_policy),
      XmRImmediate, (XtPointer) XmACCENT_BACKGROUND
    },
    { XmNallowMovement, XmCAllowMovement, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.movement_is_allowed),
      XmRImmediate, (XtPointer) TRUE
    },
    { XmNmanhattanRoute, XmCManhattanRoute, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.manhattan_route),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNgridWidth, XmCGridWidth, XmRShort, sizeof(short),
      XtOffset(XmWorkspaceWidget, workspace.grid_width),
      XmRImmediate, (XtPointer) 1
    },
    { XmNgridHeight, XmCGridHeight, XmRShort, sizeof(short),
      XtOffset(XmWorkspaceWidget, workspace.grid_height),
      XmRImmediate, (XtPointer) 1
    },
    { XmNhorizontalAlignment, XmCAlignment, XmRAlignment, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.horizontal_alignment),
      XmRImmediate, (XtPointer) XmALIGNMENT_CENTER
    },
    { XmNhorizontalDrawGrid, XmCDrawGrid,
      XmRWorkspaceType, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.horizontal_draw_grid),
      XmRImmediate, (XtPointer) XmDRAW_NONE
    },
    { XmNverticalAlignment, XmCAlignment, XmRAlignment, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.vertical_alignment),
      XmRImmediate, (XtPointer) XmALIGNMENT_CENTER
    },
    { XmNverticalDrawGrid, XmCDrawGrid,
      XmRWorkspaceType, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.vertical_draw_grid),
      XmRImmediate, (XtPointer) XmDRAW_NONE
    },
    { XmNsortPolicy, XmCAlignment, XmRAlignment, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.sort_policy),
      XmRImmediate, (XtPointer) XmALIGNMENT_CENTER
    },
    { XmNsnapToGrid, XmCSnapToGrid, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.snap_to_grid),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNlineThickness, XmCLineThickness, XmRShort, sizeof(short),
      XtOffset(XmWorkspaceWidget, workspace.line_thickness),
      XmRImmediate, (XtPointer) 0
    },
    { XmNdoubleClickInterval, XmCDoubleClickInterval, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceWidget, workspace.double_click_interval),
      XmRImmediate, (XtPointer) 500
    },
    { XmNinclusionPolicy, XmCInclusionPolicy,
      XmRWorkspaceType, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.inclusion_policy),
      XmRImmediate, (XtPointer) XmINCLUDE_ALL
    },
    { XmNselectionPolicy, XmCSelectionPolicy,
      XmRSelectionPolicy, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.selection_policy),
      XmRImmediate, (XtPointer) XmEXTENDED_SELECT
    },
    { XmNoutlineType, XmCOutlineType,
      XmRWorkspaceType, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.surrogate_type),
      XmRImmediate, (XtPointer) XmOUTLINE_EACH
    },
    { XmNplacementPolicy, XmCPlacementPolicy, XmRChar, sizeof(unsigned char),
      XtOffset(XmWorkspaceWidget, workspace.placement_policy),
      XmRImmediate, (XtPointer) XmSPACE_WARS_SELECTED_STAYS
    },
    { XmNbutton1PressMode, XmCButton1PressMode, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.button1_press_mode),
      XmRImmediate, (XtPointer) True
    },
    { XmNlineTolerance, XmCLineTolerance, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceWidget, workspace.ltol),
      XmRImmediate, (XtPointer) 2
    },
    { XmNwidgetTolerance, XmCWidgetTolerance, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceWidget, workspace.wtol),
      XmRImmediate, (XtPointer) 1
    },
    { XmNlineDrawingEnabled, XmCLineDrawingEnabled, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.line_drawing_enabled),
      XmRImmediate, (XtPointer) True
    },
    { XmNselectable, XmCSelectable, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.is_selectable),
      XmRImmediate, (XtPointer) TRUE
    },
    { XmNpreventOverlap, XmCAllowOverlap, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.check_overlap),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNautoArrange, XmCAutoArrange, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceWidget, workspace.auto_arrange),
      XmRImmediate, (XtPointer) FALSE
    }
};

#define OFFSET_DEFAULT -1
static XtResource constraintResources[] = {
    { XmNselectionCallback, XmCSelectionCallback,
      XmRCallback, sizeof(WsCallbackList),
      XtOffset(XmWorkspaceConstraints, workspace.select_callbacks),
      XmRImmediate, (XtPointer) NULL
    },
    { XmNresizingCallback, XmCResizingCallback,
      XmRCallback, sizeof(WsCallbackList),
      XtOffset(XmWorkspaceConstraints, workspace.resizing_callbacks),
      XmRImmediate, (XtPointer) NULL
    },
    { XmNaccentCallback, XmCAccentCallback, XmRCallback,sizeof(WsCallbackList),
      XtOffset(XmWorkspaceConstraints, workspace.accent_callbacks),
      XmRImmediate, (XtPointer) NULL
    },
    { XmNselectable, XmCSelectable, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.is_selectable),
      XmRImmediate, (XtPointer) TRUE
    },
    { XmNselected, XmCSelected, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.is_selected),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNid, XmCId, XmRChar, sizeof(char),
      XtOffset(XmWorkspaceConstraints, workspace.id), XmRImmediate, (XtPointer) 0
    },
    { XmNallowVerticalResizing, XmCAllowResizing, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.is_v_resizable),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNallowHorizontalResizing, XmCAllowResizing, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.is_h_resizable),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNpinLeftRight, XmCPinSides, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.pin_to_sides_lr),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNpinTopBottom, XmCPinSides, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.pin_to_sides_tb),
      XmRImmediate, (XtPointer) FALSE
    },
    {
      XmNwwTopAttachment, XmCAttachment, XmRAttachment, sizeof(unsigned char),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_TOP].type),
      XmRImmediate, (XtPointer) XmATTACH_NONE
   },
   {
      XmNwwBottomAttachment, XmCAttachment, XmRAttachment, sizeof(unsigned char),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_BOTTOM].type),
      XmRImmediate, (XtPointer) XmATTACH_NONE
   },
   {
      XmNwwLeftAttachment, XmCAttachment, XmRAttachment, sizeof(unsigned char),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_LEFT].type),
      XmRImmediate, (XtPointer) XmATTACH_NONE
   },
   {
      XmNwwRightAttachment, XmCAttachment, XmRAttachment, sizeof(unsigned char),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_RIGHT].type),
      XmRImmediate, (XtPointer) XmATTACH_NONE
   },
   {
      XmNwwTopWidget, XmCWidget, XmRWindow, sizeof(Widget),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_TOP].w),
      XmRWindow, (XtPointer) NULL
   },
   {
      XmNwwBottomWidget, XmCWidget, XmRWindow, sizeof(Widget),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_BOTTOM].w),
      XmRWindow, (XtPointer) NULL
   },
   {
      XmNwwLeftWidget, XmCWidget, XmRWindow, sizeof(Widget),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_LEFT].w),
      XmRWindow, (XtPointer) NULL
   },
   {
      XmNwwRightWidget, XmCWidget, XmRWindow, sizeof(Widget),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_RIGHT].w),
      XmRWindow, (XtPointer) NULL
   },
   {
      XmNwwTopPosition, XmCAttachment, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_TOP].percent),
      XmRImmediate, (XtPointer) 0
   },
   {
      XmNwwBottomPosition, XmCAttachment, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_BOTTOM].percent),
      XmRImmediate, (XtPointer) 0
   },
   {
      XmNwwLeftPosition, XmCAttachment, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_LEFT].percent),
      XmRImmediate, (XtPointer) 0
   },
   {
      XmNwwRightPosition, XmCAttachment, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_RIGHT].percent),
      XmRImmediate, (XtPointer) 0
   },
   {
      XmNwwTopOffset, XmCOffset, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_TOP].offset),
      XmRImmediate, (XtPointer) (Dimension) OFFSET_DEFAULT
   },
   {
      XmNwwBottomOffset, XmCOffset, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_BOTTOM].offset),
      XmRImmediate, (XtPointer) (Dimension) OFFSET_DEFAULT
   },
   {
      XmNwwLeftOffset, XmCOffset, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_LEFT].offset),
      XmRImmediate, (XtPointer) (Dimension) OFFSET_DEFAULT
   },

   {
      XmNwwRightOffset, XmCOffset, XmRInt, sizeof(int),
      XtOffset(XmWorkspaceConstraints, workspace.att[FC_RIGHT].offset),
      XmRImmediate, (XtPointer) (Dimension) OFFSET_DEFAULT
   },
   { 
      XmNlineInvisibility, XmCLineInvisibility, XmRBoolean, sizeof(Boolean),
      XtOffset(XmWorkspaceConstraints, workspace.line_invisibility),
      XmRImmediate, (XtPointer) FALSE
   },
};



/*  This translation table maps to rubberbanding actions  */
static char defaultTranslations[] =
	"<Btn1Down>:     arm() \n\
	<Btn1Motion>:   drag()\n\
	<Key>osfHelp:   ManagerGadgetHelp()\n\
	<Key>:          kbd_nav()\n\
	<Btn1Up>:       disarm()";

/*  Translation table to be installed on the children for selection actions  */
static char acceleratorTranslations[] = "#override\n\
	<Btn1Motion>:   move_w()\n\
        <Btn1Down>:     select_w()\n\
	<Btn1Up>(2+):   select_w() release_w() select_w() release_w()\n\
	<Key>osfUp:     child_nav()\n\
	<Key>osfDown:   child_nav()\n\
	<Key>osfLeft:   child_nav()\n\
	<Key>osfRight:  child_nav()\n\
        <Btn1Up>:       release_w()";

static char traversalTranslations[] =
"<EnterWindow>:  ManagerEnter()\n\
<LeaveWindow>:  ManagerLeave()\n\
<FocusOut>:     ManagerFocusOut()\n\
<FocusIn>:      ManagerFocusIn()";


XtActionsRec defaultActions[] = {
    {"arm",             (XtActionProc)Arm},
    {"drag",            (XtActionProc)Drag},
    {"disarm",          (XtActionProc)Disarm},
    {"kbd_nav",         (XtActionProc)KeyboardNavigation},
    {"select_w",        (XtActionProc)GrabSelections },
    {"move_w",          (XtActionProc)MoveResizeSelections },
    {"child_nav",       (XtActionProc)ChildNavigation},
#if RESIZE_HANDLES
    {"grow_ne",         (XtActionProc)ResizeNE },
    {"grow_nw",         (XtActionProc)ResizeNW },
    {"grow_se",         (XtActionProc)ResizeSE },
    {"grow_sw",         (XtActionProc)ResizeSW },
    {"new_release",     (XtActionProc)NewDrop },
#endif 
    {"release_w",       (XtActionProc)DropSelections },
    {"raise",		(XtActionProc)RaiseSelections },
   { "Enter",    (XtActionProc) _XmManagerEnter },      /* Motif 1.0 */
   { "FocusIn",  (XtActionProc) _XmManagerFocusIn },    /* Motif 1.0 */
};


/*
 *  Workspace class record definition:
 */
XmWorkspaceClassRec xmWorkspaceClassRec = {
    /*
     *  CoreClassPart:
     */
    {
	(WidgetClass)&xmFormClassRec,/* superclass                  */
	"XmWorkspace",                  /* class_name                   */
	sizeof(XmWorkspaceRec),         /* widget_size                  */
	ClassInitialize,                /* class_initialize             */
	(XtWidgetClassProc) NULL,       /* class_part_initialize        */
	FALSE,                          /* class_inited                 */
	(XtInitProc)Initialize,         /* initialize                   */
	(XtArgsProc) NULL,              /* initialize_hook              */
	(XtRealizeProc)Realize,         /* realize                      */
	defaultActions,                 /* actions                      */
	XtNumber(defaultActions),       /* num_actions                  */
	defaultResources,               /* resources                    */
	XtNumber(defaultResources),     /* num_resources                */
	NULLQUARK,                      /* xrm_class                    */
	TRUE,                           /* compress_motion              */
	TRUE,                           /* compress_exposure            */
	TRUE,                           /* compress_enterleave          */
	TRUE,                           /* visible_interest             */
	(XtWidgetProc)Destroy,          /* destroy                      */
	(XtWidgetProc) Resize,          /* resize                       */
	(XtExposeProc)Redisplay,        /* expose                       */
	(XtSetValuesFunc)SetValues,     /* set_values                   */
	(XtArgsFunc) NULL,              /* set_values_hook              */
	XtInheritSetValuesAlmost,       /* set_values_almost            */
	(XtArgsProc) NULL,              /* get_values_hook              */
	XtInheritAcceptFocus,           /* accept_focus                 */
	XtVersion,                      /* version                      */
	NULL,    			/* callback private             */
	defaultTranslations,            /* tm_table                     */
	XtInheritQueryGeometry,         /* query_geometry               */
	XtInheritDisplayAccelerator,    /* display_accelerator  */
	(void *) NULL,                 /* extension                    */
    },
    /*
     *  CompositeClassPart:             composite_class
     */
    {
	GeometryManager,                /* geometry_manager             */
	(XtWidgetProc)ChangeManaged,    /* change_managed               */
	MyInsertChild,      		/* insert_child                 */
	MyDeleteChild,		        /* delete_child                 */
	(void *) NULL,                 /* extension                    */
    },
    /*
     *  ConstraintClassPart:            constraint_class
     */
    {
	constraintResources,            /* resource list                */
	XtNumber(constraintResources),  /* num resources                */
	sizeof(XmWorkspaceConstraintRec), /* constraint size            */
	(XtInitProc)ConstraintInitialize, /* init proc                    */
	ConstraintDestroy,              /* destroy proc                 */
	(XtSetValuesFunc)ConstraintSetValues, /* set values proc              */
	(void *) NULL,                 /* extension                    */
    },
    /*
     *  XmManagerClassPart:             manager_class
     */
    {
	traversalTranslations,		/* translations           */
	NULL,                           /* get resources                */
	0,                              /* num get_resources            */
	NULL,                           /* get_cont_resources           */
	0,                              /* num_get_cont_resources       */
	(XmParentProcessProc)NULL,      /* parent_process         */
	NULL,                           /* extension                    */
    },
   {                        /* bulletin_board_class fields */
      FALSE,                                /* always_install_accelerators */
      NULL,                                 /* geo_matrix_create  */
      XmInheritFocusMovedProc,              /* focus_moved_proc   */
      NULL,                                 /* extension          */
   },

   {                        /* form_class fields  */
      NULL,                 /* extension          */
   },

    /*
     *  XmWorkspaceClassPart:           workspace_class
     */
    {
	NULL,                           /* lineGC                       */
	NULL,                           /* lineGC2                      */
	None,                           /* hour_glass                   */
	None,                           /* move_cursor                  */
	{ None, None, None, None },     /* size_cursor[4]               */
	NULL,
    }
};

WidgetClass xmWorkspaceWidgetClass = (WidgetClass)&xmWorkspaceClassRec;

/*  Subroutine: MyDeleteChild
 *  Purpose:    Add a child and populate the collision array
 */
static void MyDeleteChild(Widget w)
{
XmWorkspaceConstraints nc;
    nc = WORKSPACE_CONSTRAINT(w);
    if (!nc->workspace.line_invisibility)
	DeleteWidgetFromCollideList(w);
    (*superclass->composite_class.delete_child) (w);
}

/*  Subroutine: MyInsertChild
 *  Purpose:    Add a child and populate the collision array
 */
static void MyInsertChild( Widget w)
{
Dimension new_width, new_height;
Dimension old_width, old_height;
XmWorkspaceWidget ww;
XtWidgetGeometry request, reply;
XtGeometryResult result;
XmWorkspaceConstraints nc;

    (*superclass->composite_class.insert_child) (w);
    ww = (XmWorkspaceWidget)w->core.parent;
    /* The folloing code will dynamically resize the workspace widget if a
       a child is inserted beyond the current width and height.  If the
       parent does not allow the resize, punt with an XtWarning.  The
       size of the collision lists are then resized.
     */
    new_width = w->core.x + w->core.width;
    old_width = XtWidth(ww);
    new_width = MAX(new_width, old_width);

    new_height = w->core.y + w->core.height;
    old_height = XtHeight(ww);
    new_height = MAX(new_height, old_height);

    if ( (new_width > old_width) || (new_height > old_height) )
        {
	request.width = new_width + 2*(ww->workspace.grid_width);
	request.height = new_height + 2*(ww->workspace.grid_width);
	request.request_mode = CWWidth | CWHeight;
	result = XtMakeGeometryRequest((Widget)ww, &request, &reply);
	if(result != XtGeometryYes)
	    {
	    XtWarning("Geometry Request failed in Workspace.InsertChild.");
	    return;
	    }
	ReallocCollideLists(ww);
	}
    nc = WORKSPACE_CONSTRAINT(w);
    if (!nc->workspace.line_invisibility)
	AddWidgetToCollideList(w);
}

/*  Subroutine: ClassInitialize
 *  Purpose:    Install non-standard type converters needed by this widget class
 */
static void ClassInitialize( )
{
    /*  Install resource converter to parse strings in Xdefaults file  */
    XtAddConverter(XmRString, XmRWorkspaceType,
	           (XtConverter)CvtStringToWorkspaceType, NULL, 0);
}


/*  Subroutine: Initialize
 *  Purpose:    Initialize the workspace widget instance
 */
static void Initialize( XmWorkspaceWidget request, XmWorkspaceWidget new )
{
    XtTranslations trans_table;
    Arg warg;

    /*  Contrary to what is promised, record is not initially cleared  */
    new->workspace.suppress_callbacks = False;
    new->workspace.num_selected = 0;
    new->workspace.is_rubberbanding = FALSE;
    new->workspace.is_moving = FALSE;
    new->workspace.is_resizing = FALSE;
    new->workspace.band_is_visible = FALSE;
    new->workspace.surrogate_is_visible = FALSE;
    new->workspace.bandGC = NULL;
    new->workspace.badBandGC = NULL;
    new->workspace.gridGC = NULL;
    new->workspace.num_surrogates = 0;
    new->workspace.lines = NULL;
    new->workspace.num_lines = 0;
    new->workspace.button_tracker = NULL;
    new->workspace.time = 0;
    new->workspace.move_cursor_installed = 0;
    new->workspace.size_xx_installed = SZ_NONE;
    new->workspace.auto_arrange = FALSE;

    GetRubberbandGC(new);
    if( xmWorkspaceClassRec.workspace_class.move_cursor == None )
	xmWorkspaceClassRec.workspace_class.move_cursor =
	  XCreateFontCursor(XtDisplay(new), XC_fleur);
    if( xmWorkspaceClassRec.workspace_class.hour_glass == None )
	xmWorkspaceClassRec.workspace_class.hour_glass =
	  XCreateFontCursor(XtDisplay(new), XC_watch);
    if( xmWorkspaceClassRec.workspace_class.size_cursor[SZ_UL] == None )
	xmWorkspaceClassRec.workspace_class.size_cursor[SZ_UL] =
	  XCreateFontCursor(XtDisplay(new), XC_ul_angle);
    if( xmWorkspaceClassRec.workspace_class.size_cursor[SZ_UR] == None )
	xmWorkspaceClassRec.workspace_class.size_cursor[SZ_UR] =
	  XCreateFontCursor(XtDisplay(new), XC_ur_angle);
    if( xmWorkspaceClassRec.workspace_class.size_cursor[SZ_LL] == None )
	xmWorkspaceClassRec.workspace_class.size_cursor[SZ_LL] =
	  XCreateFontCursor(XtDisplay(new), XC_ll_angle);
    if( xmWorkspaceClassRec.workspace_class.size_cursor[SZ_LR] == None )
	xmWorkspaceClassRec.workspace_class.size_cursor[SZ_LR] =
	  XCreateFontCursor(XtDisplay(new), XC_lr_angle);
    new->workspace.collide_width  = new->core.width;
    new->workspace.collide_height = new->core.height;
    new->workspace.collide_list_x = 
			(CollideList **)XtCalloc(new->core.width, sizeof(new->workspace.collide_list_x));
    new->workspace.collide_list_y = 
			(CollideList **)XtCalloc(new->core.height, sizeof(new->workspace.collide_list_y));
    new->workspace.widget_list = NULL;

    /* Adjust wtol and ltol to be a multiple of the line thickness */
    new->workspace.wtol = (new->workspace.wtol*new->workspace.line_thickness)/2;
    new->workspace.ltol = new->workspace.ltol * new->workspace.line_thickness;

    UnsetExposureArea(new);

    /*
     * calling XtSetValues inside the Initialize method is a bad
     * thing since the SetValues method will then be called on a widget
     * whose initialization process hasn't completed.
     *
     */
     
    trans_table = XtParseTranslationTable (defaultTranslations);
    XtSetArg (warg, XmNtranslations, trans_table);
#if 0
    // Don't check this in -MST
    //XtSetValues ((Widget)new, &warg, 1);
#endif
}


/*  Subroutine: Realize
 */
static void Realize( XmWorkspaceWidget ww, Mask* p_valueMask,
	             XSetWindowAttributes *attributes )
{
    XmWorkspaceConstraints constraints;
    Mask valueMask;
    int i;
    Widget child;
    XMapEvent event;

    if( ww->workspace.accent_color == ((Pixel)(-1)) )
    {
	Display *display = XtDisplay(ww);
	ww->workspace.accent_color =
	  WhitePixel(display, XScreenNumberOfScreen(XtScreen(ww)));
    }
    valueMask = *p_valueMask | CWBitGravity | CWDontPropagate;
    attributes->bit_gravity = NorthWestGravity;

    attributes->do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask
      | KeyPressMask | KeyReleaseMask | PointerMotionMask;
    XtCreateWindow((Widget)ww, InputOutput, CopyFromParent, valueMask, attributes);
    /*  Create a background pixmap with grid lines, if warranted  */
    SetGridBackground(ww, FALSE);
    /*  Accents pre-selected children and then perform selection  */
    /*  Fake a map event to pass in the callback  */
    event.type = MapNotify;
    event.event = XtWindow(ww);
    event.send_event = TRUE;
    event.display = XtDisplay(ww);
    event.override_redirect = FALSE;
    for( i=0; i<ww->composite.num_children; i++ ) {
	child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(child);
	if( constraints->workspace.is_selected )
	{
	    constraints->workspace.is_selected = FALSE;
	    /*  SelectChild will include call to AccentChild  */
	    event.window = XtWindow(child);
	    SelectChild(ww, child, constraints, (XEvent *)&event);
	}
    }
}


/*  Subroutine: Destroy
 *  Purpose:    Clean up allocated resources when the widget is destroyed.
 */
static void Destroy(  XmWorkspaceWidget ww )
{
CollideList **cl_ptr;
CollideList *ce_ptr;
CollideList *n_ce_ptr;
int i;
MyWidgetList *wl_ptr, *rove;

    if( ww->workspace.gridGC )
	XtReleaseGC((Widget)ww, ww->workspace.gridGC);
    if( ww->workspace.bandGC )
	XtReleaseGC((Widget)ww, ww->workspace.bandGC);
    if( ww->workspace.badBandGC )
	XtReleaseGC((Widget)ww, ww->workspace.badBandGC);
    XtRemoveAllCallbacks((Widget)ww, XmNbackgroundCallback);
    XtRemoveAllCallbacks((Widget)ww, XmNdefaultActionCallback);
    XtRemoveAllCallbacks((Widget)ww, XmNerrorCallback);
    XtRemoveAllCallbacks((Widget)ww, XmNpositionChangeCallback);


    /* Free the collision lists */
    cl_ptr = ww->workspace.collide_list_x;
    for (i = 0; i < ww->workspace.collide_width; ++i)
    {
	for (ce_ptr = cl_ptr[i]; ce_ptr; ce_ptr = n_ce_ptr)
	{
	    n_ce_ptr = ce_ptr->next;
	    XtFree((char*)ce_ptr);
	}
    }
    XtFree((char*)cl_ptr);

    cl_ptr = ww->workspace.collide_list_y;
    for (i = 0; i < ww->workspace.collide_height; ++i)
    {
	for (ce_ptr = cl_ptr[i]; ce_ptr; ce_ptr = n_ce_ptr)
	{
	    n_ce_ptr = ce_ptr->next;
	    XtFree((char*)ce_ptr);
	}
    }
    XtFree((char*)cl_ptr);

    /*
     * Free the widget list
     */
    wl_ptr = ww->workspace.widget_list;
    while (wl_ptr) {
	rove = wl_ptr->next;
	XtFree((char*)wl_ptr);
	wl_ptr = rove;
    }
}

#if STRETCH_SEPARATORS
static void unobstructedHoriz (XmWorkspaceWidget , Widget , int *, int*);
#endif

static void Resize( XmWorkspaceWidget ww)
{
XmWorkspaceCallbackStruct cb;

    if (ww->workspace.auto_arrange)
	(*superclass->core_class.resize) ((Widget)ww);
    if (!XtIsRealized((Widget)ww))
	return;

#if STRETCH_SEPARATORS
{
XtWidgetGeometry req;
int i, bw;
Widget child;
XmWorkspaceConstraints cons;

    /*
     * if !auto_arrange_mode then see if there are widgets who want
     * want to be pinned top/bottom or left/right.  Separator widgets
     * in ControlPanels want this.
     */
    if (!ww->workspace.auto_arrange) {
	int smallx, bigx, bigy;
	XmWorkspaceGetMaxWidthHeight ((Widget)ww, &bigx, &bigy);
	for (i=0; i<ww->composite.num_children; i++) {
	    child = ww->composite.children[i];
	    cons = WORKSPACE_CONSTRAINT(child);
	    if ((cons->workspace.pin_to_sides_lr == False) &&
		(cons->workspace.pin_to_sides_tb == False)) continue; 
	    req.x = child->core.x;
	    req.y = child->core.y;
	    req.width = child->core.width;
	    req.height = child->core.height;
	    bw = child->core.border_width;
	    req.request_mode = 0;
	    /* refuse to pin left,right and top.bottom */
	    if (cons->workspace.pin_to_sides_lr) {
		smallx = 0;
		unobstructedHoriz (ww, child, &smallx, &bigx);
		req.x = smallx;
		req.width = bigx - smallx;
		if (child->core.x > req.x) req.request_mode|= CWX;
		if (child->core.width != req.width) req.request_mode|= CWWidth;
	    } else if (cons->workspace.pin_to_sides_tb) {
	    }
	    XtConfigureWidget (child, req.x, req.y, req.width, req.height, bw);
	    _GeometryManager (child, &req, NULL, False);
	}
    }
}
#endif

    ReallocCollideLists(ww);

    cb.reason = XmCR_RESIZE;
    cb.event = NULL;
    cb.window = XtWindow(ww);
    cb.selected_widget = NULL;
    XtCallCallbackList ((Widget)ww, ww->workspace.resize_callback, &cb);
}

#if STRETCH_SEPARATORS
/*
 * assumes the outputs are initialized by the caller.
 */
static void
unobstructedHoriz (XmWorkspaceWidget ww, Widget cw, int *lx, int *rx)
{
int i, minx, maxx, cw_bottom, cw_right, this_bottom, this_right;
Boolean overlap;
Widget child;
XmWorkspaceConstraints cons;

    minx = *lx;
    maxx = *rx;

    cw_bottom = cw->core.y + cw->core.height;
    cw_right = maxx;

    for (i=0; i<ww->composite.num_children; i++) {
	child = ww->composite.children[i];
	if (child == cw) continue;
	cons = WORKSPACE_CONSTRAINT(child);

	this_bottom = child->core.y + child->core.height;
	this_right = child->core.x + child->core.width;
	if ((child->core.y > cw_bottom)||
	    (cw->core.y    > this_bottom)) overlap = False;
	else if (child->core.x > cw_right) overlap = False;
	else overlap = True;

	if (overlap) {
	    minx = MAX(minx, this_right+1);
	}

	if ((child->core.y > cw_bottom)||
	    (cw->core.y    > this_bottom)) overlap = False;
	else if (child->core.x < cw->core.x) overlap = False;
	else overlap = True;

	if (overlap) {
	    maxx = MIN(maxx, child->core.x-1);
	}

    }

    *lx = minx;
    *rx = maxx;
}
#endif


/*  Subroutine: Expose
 *  Effect:     Redraw portion of the window exposed by movement of other
 *              windows
 *  Note:       Redrawing of xor'd features must be precisely fit to
 *              regions newly exposed.
 */
#define IF_MORE(a,b) if((a)>(b))(b)=(a)
#define IF_LESS(a,b) if((a)<(b))(b)=(a)
static void Redisplay( XmWorkspaceWidget ww, XExposeEvent* event,
	               Region region )
{
    int i;
    Boolean clip_region_set = FALSE;
    short line_excess;

    if (ww->workspace.auto_arrange)
	(*superclass->core_class.expose) ((Widget)ww, (XEvent *)event, region);

    if( !XtIsRealized((Widget)ww) )
	return;
    /* The user can enter events while the application is in the process of   *
     * rearranging its windows.  Since the server may be slow to make all of  *
     * the changes, events can be entered during meaningless intermediate     *
     * states.                                                                */
    /* Discard or suppress all events that represent user input.  Do not let  *
     * these events get queued up for later processing.  Expose events must   *
     * still be accepted as they are used to synchronize changes and          *
     * redrawing.                                                             */
    line_excess = ww->workspace.line_thickness / 2;
    /*  Augment exposure rectangle (in case it's already being augmented)  */
    IF_LESS(event->x - line_excess, ww->workspace.expose_left);
    IF_MORE(line_excess + event->width + event->x - 1,
	    ww->workspace.expose_right);
    IF_LESS(event->y - line_excess, ww->workspace.expose_upper);
    IF_MORE(line_excess + event->y + event->height - 1,
	    ww->workspace.expose_lower);
    /*  Redraw application lines if needed (uses exposure rectangle)  */
    if( ww->workspace.lines )
	RefreshLines(ww);
    /*  Grid lines are handled by server as background pixmap so do nothing  */
    /*  Redraw rubberband bounding box, if visible  */
    if( ww->workspace.band_is_visible )
    {
	if( RedisplayRectangle(ww, ww->workspace.rubberband, region,
	                       clip_region_set) )
	    XSetClipMask(XtDisplay(ww), ww->workspace.bandGC, None);
    }
    /*  Redraw surrogate outline(s), if visible  */
    else if( ww->workspace.surrogate_is_visible )
    {
	for( i=0; i<ww->workspace.num_surrogates; i++ )
	     clip_region_set =
	       RedisplayRectangle(ww, &ww->workspace.surrogates[i], region,
	                          clip_region_set);
	/*  Restore Clipping mask to None  */
	if( clip_region_set )
	    XSetClipMask(XtDisplay(ww), ww->workspace.bandGC, None);
    }
    UnsetExposureArea(ww);
}
#undef IF_MORE
#undef IF_LESS


static Boolean RedisplayRectangle( XmWorkspaceWidget ww, XRectangle* rect,
	                           Region region, Boolean clip_region_set )
{
    if( XRectInRegion(region, rect->x, rect->y, rect->width, rect->height) )
    {
	if( region && (clip_region_set == FALSE) )
	    XSetRegion(XtDisplay(ww), ww->workspace.bandGC, region);
	XDrawRectangles(XtDisplay(ww), XtWindow(ww), ww->workspace.bandGC,
	                rect, 1);
	if( region )
	    return TRUE;
	else
	    return FALSE;
    }
    else
	return FALSE;
}

typedef struct {
  char *type;
  char *w;
  char *offset;
  char *percent;
} ResNamePlaceHolder;

static ResNamePlaceHolder rname[] = {
 { XmNleftAttachment, XmNleftWidget, XmNleftOffset, XmNleftPosition },
 { XmNrightAttachment, XmNrightWidget, XmNrightOffset, XmNrightPosition },
 { XmNtopAttachment, XmNtopWidget, XmNtopOffset, XmNtopPosition },
 { XmNbottomAttachment, XmNbottomWidget, XmNbottomOffset, XmNbottomPosition }
};


/********************************************************************************/
/* return True if this child widget can scoot over to the left edge of the      */
/* Workspace without bumping into anyone. Used when switching into XmForm mode. */
/* Had been assuming that coords within a few pixels of the edge guaranteed     */
/* we could stretch, but then Rich S. started making vertical separators along  */
/* the edges of his control panels.                                             */
/********************************************************************************/
static Boolean
safe2StretchLeft (XRectangle rangle[], int nkids, int kid)
{
int i;

    for (i=0; i<nkids; i++) {
	if (i == kid) continue;
	if (rangle[i].y > (rangle[kid].y + rangle[kid].height)) continue;
	if ((rangle[i].y + rangle[i].height) < rangle[kid].y) continue;
	if (rangle[i].x < rangle[kid].x) return False;
    }
    return True;
}
static Boolean
safe2StretchRight (XRectangle rangle[], int nkids, int kid)
{
int i;

    for (i=0; i<nkids; i++) {
	if (i == kid) continue;
	if (rangle[i].y > (rangle[kid].y + rangle[kid].height)) continue;
	if ((rangle[i].y + rangle[i].height) < rangle[kid].y) continue;
	if (rangle[i].x > rangle[kid].x) return False;
    }
    return True;
}



/*  Subroutine: SetValues
 *  Purpose:    Check for changes that require other action to bring all things
 *              into consistency.
 */
static Boolean SetValues( XmWorkspaceWidget current,
	               XmWorkspaceWidget request,
	               XmWorkspaceWidget new )
{
    Boolean clear_window = FALSE;
    XmWorkspaceLine line;
    int i;
    XmWorkspaceConstraints nc;
    Widget child;
    XEvent event;
    Boolean reroute = False, refresh = False, redraw = False;
    Boolean *was_selected;

#if 0
    /*
     * this is how you make lines disappear when the value of the resource changes
     */
    if ((new->workspace.line_drawing_enabled != current->workspace.line_drawing_enabled)&&
	(new->workspace.line_drawing_enabled == False)) redraw = True;
#endif
	
    if (new->workspace.button1_press_mode != 
		current->workspace.button1_press_mode)
	{
	return FALSE;
	}

    /*  If grid visibility changes or grid is visible and its form changes  */
    if(   (new->workspace.snap_to_grid != current->workspace.snap_to_grid)
       || (   new->workspace.horizontal_draw_grid
	   != current->workspace.horizontal_draw_grid)
       || (   new->workspace.vertical_draw_grid
	   != current->workspace.vertical_draw_grid)
       || (   (   current->workspace.snap_to_grid
	       && (   (current->workspace.horizontal_draw_grid != XmDRAW_NONE)
	           || (current->workspace.vertical_draw_grid != XmDRAW_NONE)))
	   && (   (   new->workspace.horizontal_alignment
	           != current->workspace.horizontal_alignment)
	       || (   new->workspace.vertical_alignment
	           != current->workspace.vertical_alignment)
	       || (new->workspace.grid_width != current->workspace.grid_width)
	       || (   new->workspace.grid_height
	           != current->workspace.grid_height))
	   )
       )
    {
	/*  Create the new gridding situation  */
	SetGridBackground(new, TRUE);
	clear_window = TRUE;
    }

    /*  if line routing changed  */
    if (new->workspace.manhattan_route != 
	 current->workspace.manhattan_route)
    {
#if 11
	reroute = True;
#else
	RerouteLines(new, TRUE);
#endif
	clear_window = TRUE;
    }
    if ((new->workspace.line_drawing_enabled == TRUE) &&
	 (current->workspace.line_drawing_enabled == FALSE))
	{
	if (XtIsRealized ((Widget)new))
	    XClearWindow(XtDisplay(new), XtWindow(new));
#if 11
	reroute = True;
	refresh = True;
#else
	RerouteLines(new, TRUE);
	RefreshLines(new);
#endif
	}
    if( (new->core.width >= current->core.width) ||
	(new->core.height >= current->core.height) )
	{
	ReallocCollideLists(new);
	}

    /*  Replace the old with the new  */
    if (clear_window)
	{
	if ( (new->workspace.snap_to_grid == TRUE) && 
		(current->workspace.snap_to_grid == FALSE) )
	    {
	    new->workspace.suppress_callbacks = True;
	    was_selected = (Boolean *)
		XtMalloc(new->composite.num_children*sizeof(Boolean));
	    for(i=0; i<new->composite.num_children; i++)
		{
		child = new->composite.children[i];
	        nc = WORKSPACE_CONSTRAINT(child);
		was_selected[i] = nc->workspace.is_selected;
		if(!was_selected[i])
		    SelectChild(new, child, nc, (XEvent *)NULL);
		}
	    if(new->composite.num_children > 0)
		{
		child = new->composite.children[0];
		event.xbutton.x = child->core.x+1;
		event.xbutton.y = child->core.y+1;
		event.xbutton.window = new->composite.children[0]->core.window;
		event.xbutton.time = new->workspace.time - 
				2*new->workspace.double_click_interval;
		event.xbutton.state = 0;
		GrabSelections((Widget)new, &event, NULL, NULL);
		event.xbutton.x = child->core.x;
		event.xbutton.y = child->core.y;
		DropSelections((Widget)new, &event, NULL, NULL);
		}
	    for(i=0; i<new->composite.num_children; i++)
		{
		child = new->composite.children[i];
	        nc = WORKSPACE_CONSTRAINT(child);
		if(was_selected[i])
		    SelectChild(new, child, nc, (XEvent *)NULL);
		else
		    UnselectChild(new, child, nc, (XEvent *)NULL);
		}
	    if(new->composite.num_children > 0)
		XtFree(was_selected);
	    new->workspace.suppress_callbacks = False;
	    }
	if (XtWindow(new))
	    XClearWindow(XtDisplay(new), XtWindow(new));
	if(new->workspace.lines)
	    {
	    line = new->workspace.lines;
	    while(line)
		{
		line->is_to_be_drawn = TRUE;
		line = line->next;
		}
	    }
	redraw =  TRUE;
	}
    if ((new->bulletin_board.allow_overlap !=
	 current->bulletin_board.allow_overlap) &&
	!new->bulletin_board.allow_overlap)
	{
	/*
	 * Pretend like all are newly managed so space wars works
	 */
	Boolean moved;
	for( i=0; i<new->composite.num_children; i++ )
	    {
	    child = new->composite.children[i];
	    nc = WORKSPACE_CONSTRAINT(child);
	    nc->workspace.is_newly_managed = True;
	    }
	moved = PerformSpaceWars(new, TRUE, NULL);
	for( i=0; i<new->composite.num_children; i++ )
	    {
	    child = new->composite.children[i];
	    nc = WORKSPACE_CONSTRAINT(child);
	    nc->workspace.is_newly_managed = False;
	    }
	/*
	 * If we move anything, be sure to reroute the lines
	 */
	if( moved && new->workspace.lines ) 
	    {
	    if (XtWindow(new)) 
		XClearWindow(XtDisplay(new), XtWindow(new)); 
#if 11
	    reroute = True;
	    refresh = True;
#else
	    RerouteLines(new, FALSE);
	    RefreshLines(new); 
#endif
	    }
	}

    /*
     * form attachments will be set all at once when XmNautoArrange is set to
     * True.  Any settings which have been put into XmNwwTopAttachment etc will
     * be used first.  Any settings == XmATTACH_NONE will be calculated and set
     * to XmATTACH_POSITION.  It would be nice to have a real way to specify
     * the attachments.  One possibility would be sorting the edges.
     */
    if (current->workspace.auto_arrange != new->workspace.auto_arrange) {
	int n, wwidth, wheight, bw;
	int bottomp, rightp, leftp, topp, fbase;
	XmFormAttachmentRec *att;
	Arg args[20];

	/*
	 * We're switching into XmForm mode... do attachment calculations.
	 */
	if (new->workspace.auto_arrange) {
	    
	    int maxw, maxh;
	    XRectangle *rangle;
	    int nkids = current->composite.num_children;
	    wwidth = new->core.width;
	    wheight = new->core.height;
	    rangle = (XRectangle *)malloc((1+nkids) * sizeof(XRectangle));

	    /*
	     * must hang on to dimensions before superclass get its hands on them
	     * must also inform the form widget of the child's preferred dimensions.
	     * Ordinarily a form widget would have this info, but the form side of
	     * personality has been asleep and needs to be roused carefully.
	     */
	    for (i=0; i<current->composite.num_children; i++) {
#if (OLD_LESSTIF == 1)
		XmFormConstraintPart* formcons;
#else
		XmFormConstraint formcons;
#endif
		rangle[i].x = current->composite.children[i]->core.x;
		rangle[i].y = current->composite.children[i]->core.y;
		rangle[i].width = current->composite.children[i]->core.width;
		rangle[i].height = current->composite.children[i]->core.height;

		formcons = GetFormConstraint(current->composite.children[i]);
		formcons->preferred_width = rangle[i].width;
		formcons->preferred_height = rangle[i].height;
	    }
	    (*superclass->composite_class.change_managed) ((Widget)current);


	    /*
	     * From this point on, must not use core.{x,y,width,height} because
	     * the superclass might have touched these values.  Can only use
	     * values in rangle.
	     */
	    XtVaGetValues ((Widget)current, XmNfractionBase, &fbase, NULL);
	    XmWorkspaceGetMaxWidthHeight  ((Widget)current, &maxw, &maxh);
	    for (i=0; i<current->composite.num_children; i++) {
		int j;
		int x2use, y2use, w2use, h2use;
		float posfl, wfactor, hfactor;
		struct {
		    int top,left,right,bottom;
		} limits;

		posfl = fbase * 0.05;
		limits.top = limits.left = (int)posfl;
		posfl = fbase * 0.95;
		limits.right = limits.bottom = (int)posfl;

		child = current->composite.children[i];
		nc = WORKSPACE_CONSTRAINT(child);
		att = nc->workspace.att;

		x2use = rangle[i].x;
		y2use = rangle[i].y;
		w2use = rangle[i].width;
		h2use = rangle[i].height;

		nc->workspace.orig_base.x = x2use;
		nc->workspace.orig_base.y = y2use;
		nc->workspace.orig_base.width = w2use;
		nc->workspace.orig_base.height = h2use;

		/*
		 * perform accurate rounding
		 */
		if (wwidth <= 0) wwidth = 1;
		if (wheight <= 0) wheight = 1;
		wfactor = (float)fbase/(float)wwidth;
		hfactor = (float)fbase/(float)wheight;
 
		posfl = (float)x2use * wfactor;
		leftp = (int)posfl;
		if ((posfl - (float)leftp) >= 0.5) leftp++;
 
		posfl = (float)y2use * hfactor;
		topp = (int)posfl;
		if ((posfl - (float)topp) >= 0.5) topp++;
 
		posfl = (float)(x2use+w2use) * wfactor;
		rightp = (int)posfl;
		if ((posfl - (float)rightp) >= 0.5) rightp++;
 
		posfl = (float)(y2use+h2use) * hfactor;
		bottomp = (int)posfl;
		if ((posfl - (float)bottomp) >= 0.5) bottomp++;
 
		/*
		 * ASSERT (rightp >= (leftp+1))
		 * ASSERT (bottomp >= (topp+1))
		 * You would expect not to have to do this but because of
		 * an old bug it might be possible to find a dimension set to
		 * something like 0 or 1 in the .cfg file.  So this prevents
		 * pages and pages of motif warnings from XmForm.
		 */
		if (rightp < (leftp+1)) rightp = leftp+1;
		if (bottomp < (topp+1)) bottomp = topp+1;
 


		n = 0;

		/*
		 * separator decorators have this resource set so that they will
		 * be stretched on a window resize.
		 */
		if ((nc->workspace.pin_to_sides_lr) && (!nc->workspace.pin_to_sides_tb)) {

		    /*
		     * The separator is currently going all the way across
		     */
		    if ((leftp<=limits.left)&&(safe2StretchLeft(rangle, nkids, i)) && 
			(rightp>=limits.right)&&(safe2StretchRight(rangle, nkids, i))) {
			for (j=0; j<4; j++) {
			    if (att[j].offset != OFFSET_DEFAULT) {
				XtSetArg (args[n], rname[j].offset, att[j].offset); n++;
			    } else {
				XtSetArg (args[n], rname[j].offset, 0); n++;
			    }
			    if (att[j].type != XmATTACH_NONE) {
				XtSetArg (args[n], rname[j].type, att[j].type); n++;
				XtSetArg (args[n], rname[j].w, att[j].w); n++;
				XtSetArg (args[n], rname[j].percent, att[j].percent); n++;
			    } else {
				switch (j) {
				    case FC_LEFT:
					XtSetArg (args[n], 
					  XmNleftAttachment, XmATTACH_FORM); n++;
					break;
				    case FC_RIGHT:
					XtSetArg (args[n], 
					  XmNrightAttachment, XmATTACH_FORM); n++;
					break;
				    case FC_TOP:
					XtSetArg (args[n], 
					  XmNtopAttachment, XmATTACH_POSITION); n++;
					XtSetArg (args[n], XmNtopPosition, topp); n++;
					break;
				    case FC_BOTTOM:
					break;
				}
			    }
			}
		    } else if ((leftp<=limits.left)&&(safe2StretchLeft(rangle, nkids, i))) {
			/*
			 * The separator goes from the left side, part way across.
			 */
			for (j=0; j<4; j++) {
			    if (att[j].offset != OFFSET_DEFAULT) {
				XtSetArg (args[n], rname[j].offset, att[j].offset); n++;
			    } else {
				XtSetArg (args[n], rname[j].offset, 0); n++;
			    }
			    if (att[j].type != XmATTACH_NONE) {
				XtSetArg (args[n], rname[j].type, att[j].type); n++;
				XtSetArg (args[n], rname[j].w, att[j].w); n++;
				XtSetArg (args[n], rname[j].percent, att[j].percent); n++;
			    } else {
				switch (j) {
				    case FC_LEFT:
					XtSetArg (args[n], 
					    XmNleftAttachment, XmATTACH_FORM); n++;
				        break;
				    case FC_RIGHT:
		    			XtSetArg (args[n], 
					    XmNrightAttachment, XmATTACH_POSITION); n++;
					XtSetArg (args[n], XmNrightPosition, rightp); n++;
				        break;
				    case FC_TOP:
					XtSetArg (args[n], 
					    XmNtopAttachment, XmATTACH_POSITION); n++;
					XtSetArg (args[n], XmNtopPosition, topp); n++;
				        break;
				    case FC_BOTTOM:
				        break;
				}
			    }
			}
		    } else if ((rightp>=limits.right)&&(safe2StretchRight(rangle, nkids, i))) {
			/*
			 * The separator goes from somewhere in the middle all the
			 * way to the right side.
			 */
			for (j=0; j<4; j++) {
			    if (att[j].offset != OFFSET_DEFAULT) {
				XtSetArg (args[n], rname[j].offset, att[j].offset); n++;
			    } else {
				XtSetArg (args[n], rname[j].offset, 0); n++;
			    }
			    if (att[j].type != XmATTACH_NONE) {
				XtSetArg (args[n], rname[j].type, att[j].type); n++;
				XtSetArg (args[n], rname[j].w, att[j].w); n++;
				XtSetArg (args[n], rname[j].percent, att[j].percent); n++;
			    } else {
				switch (j) {
				    case FC_LEFT:
		    			XtSetArg (args[n], 
					    XmNleftAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], XmNleftPosition, leftp); n++;
				        break;
				    case FC_RIGHT:
		    			XtSetArg (args[n], 
					    XmNrightAttachment, XmATTACH_FORM); n++; 
				        break;
				    case FC_TOP:
		    			XtSetArg (args[n], 
					    XmNtopAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], XmNtopPosition, topp); n++;
				        break;
				    case FC_BOTTOM:
				        break;
				}
			    }
			}
		    } else {
			/*
			 * The separator starts somewhere in the middle and ends
			 * somewhere in the middle
			 */
			for (j=0; j<4; j++) {
			    if (att[j].offset != OFFSET_DEFAULT) {
				XtSetArg (args[n], rname[j].offset, att[j].offset); n++;
			    } else {
				XtSetArg (args[n], rname[j].offset, 0); n++;
			    }
			    if (att[j].type != XmATTACH_NONE) {
				XtSetArg (args[n], rname[j].type, att[j].type); n++;
				XtSetArg (args[n], rname[j].w, att[j].w); n++;
				XtSetArg (args[n], rname[j].percent, att[j].percent); n++;
			    } else {
				switch (j) {
				    case FC_LEFT:
		    			XtSetArg (args[n], 
					    XmNleftAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], XmNleftPosition, leftp); n++;
				        break;
				    case FC_RIGHT:
		    			XtSetArg (args[n], 
					    XmNrightAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], XmNrightPosition, rightp); n++;
				        break;
				    case FC_TOP:
		    			XtSetArg (args[n], 
					    XmNtopAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], XmNtopPosition, topp); n++;
				        break;
				    case FC_BOTTOM:
				        break;
				}
			    }
			}
		    }
		} else {
		    /*
		     * handle everything except separator decorators.
		     */
		    for (j=0; j<4; j++) {
			    if (att[j].offset != OFFSET_DEFAULT) {
				XtSetArg (args[n], rname[j].offset, att[j].offset); n++;
			    } else {
				XtSetArg (args[n], rname[j].offset, 0); n++;
			    }
			if (att[j].type != XmATTACH_NONE) {
			    XtSetArg (args[n], rname[j].type, att[j].type); n++;
			    XtSetArg (args[n], rname[j].w, att[j].w); n++;
			    XtSetArg (args[n], rname[j].percent, att[j].percent); n++;
			} else { 
			    switch (j) {
				case FC_LEFT:
				    XtSetArg (args[n], 
					XmNleftAttachment, XmATTACH_POSITION); n++; 
				    XtSetArg (args[n], XmNleftPosition, leftp); n++;
				    break;
				case FC_TOP:
				    XtSetArg (args[n], 
					XmNtopAttachment, XmATTACH_POSITION); n++; 
				    XtSetArg (args[n], XmNtopPosition, topp); n++;
				    break;
				case FC_RIGHT:
				    if (nc->workspace.pin_to_sides_lr) {
					XtSetArg (args[n], 
					    XmNrightAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], 
					    XmNrightPosition, rightp); n++;
				    } else {
					XtSetArg (args[n], 
					    XmNrightAttachment, XmATTACH_NONE); n++; 
				    }
				    break;
				case FC_BOTTOM:
				    if (nc->workspace.pin_to_sides_tb) {
					XtSetArg (args[n], 
					    XmNbottomAttachment, XmATTACH_POSITION); n++; 
					XtSetArg (args[n], 
					    XmNbottomPosition, bottomp); n++;
				    } else {
					XtSetArg (args[n], 
					    XmNbottomAttachment, XmATTACH_NONE); n++; 
				    }
				    break;
			    }
			}
		    }
		}

		XtSetValues (child, args, n);

		bw = child->core.border_width;
		XtResizeWidget (child, rangle[i].width, rangle[i].height, bw);
		if (nc->workspace.is_selected) {
		    UnselectChild(new, child, nc, (XEvent *)NULL);
		}
		
	    }
	    free(rangle);
	} else {
	    /*
	     * We're switching from XmForm mode to Workspace mode.
	     */
	    Boolean tmp;
	    XtWidgetGeometry req;
	    tmp = current->bulletin_board.allow_overlap;
	    current->bulletin_board.allow_overlap = True;
	    for (i=0; i<current->composite.num_children; i++) {
		child = current->composite.children[i];
		XtVaSetValues (child, XmNtopAttachment, XmATTACH_NONE,
		  XmNleftAttachment, XmATTACH_NONE, XmNbottomAttachment, 
		  XmATTACH_NONE, XmNrightAttachment, XmATTACH_NONE, NULL);
	    }
	    for (i=0; i<current->composite.num_children; i++) {
		child = current->composite.children[i];
		nc = WORKSPACE_CONSTRAINT(child);
		req.request_mode = CWX|CWY|CWWidth|CWHeight;
		req.x = nc->workspace.orig_base.x;
		req.y = nc->workspace.orig_base.y;
		req.width = nc->workspace.orig_base.width;
		req.height = nc->workspace.orig_base.height;
		bw = child->core.border_width;
		XtResizeWidget (child, req.width, req.height, bw);
		XtMoveWidget (child, req.x, req.y);
		/*XtConfigureWidget (child, req.x, req.y, req.width, req.height, bw);*/
            	_GeometryManager (child, &req, NULL, False);
	    }
	    current->workspace.orig_width = 0;
	    current->workspace.orig_height = 0;
	    current->bulletin_board.allow_overlap = tmp;
	}
    }

#if 11
    if (reroute)
	RerouteLines(new, TRUE);
    if (refresh)
	RefreshLines(new);
#endif
/*  Watch out for line management %%  */
    return redraw;
}


/*  Subroutine: ChangeManaged
 *  Effect:     Makes sure everyone is in their places before the curtain opens
 */
static void ChangeManaged( XmWorkspaceWidget ww )
{
    XmWorkspaceConstraints nc=0;
    Widget child=0;
    int i;
    Boolean resolve_overlap = FALSE;
    Boolean new_node = FALSE;
#if (OLD_LESSTIF == 1)
    XmFormConstraintPart *formcon;
#else
    XmFormConstraint formcon;
#endif

    if (ww->workspace.auto_arrange)
	(*superclass->composite_class.change_managed) ((Widget)ww);
    for( i=0; i<ww->composite.num_children; i++ )
    {
	child = ww->composite.children[i];
	nc = WORKSPACE_CONSTRAINT(child);
	if (((child->core.managed) && (!nc->workspace.is_managed)) &&
	     (!ww->workspace.auto_arrange))
	{

	    /*
	     * Certainly don't want to do this if auto_arrange==True
	     */
	   formcon = GetFormConstraint(child);
#if (OLD_LESSTIF == 1)
	   formcon->atta[0].type = XmATTACH_NONE;
	   formcon->atta[1].type = XmATTACH_NONE;
	   formcon->atta[2].type = XmATTACH_NONE;
	   formcon->atta[3].type = XmATTACH_NONE;
#else
	   formcon->att[0].type = XmATTACH_NONE;
	   formcon->att[1].type = XmATTACH_NONE;
	   formcon->att[2].type = XmATTACH_NONE;
	   formcon->att[3].type = XmATTACH_NONE;
#endif

	    new_node = True;
#if 1
	    /*
	     * grow the workspace if the child won't fit
	     */
	    { 
		XtWidgetGeometry compromise,req; int newdim;
		XtGeometryResult result;
		newdim = child->core.x + child->core.width;
		req.request_mode = 0;
		if (newdim > ww->core.width) {
		    req.request_mode|= CWWidth;
		    req.width = newdim + 100;
		}
		newdim = child->core.y + child->core.height;
		if (newdim > ww->core.height) {
		    req.request_mode = CWHeight;
		    req.height = newdim + 100;
		}
		if (req.request_mode) {
		    result = XtMakeGeometryRequest ((Widget)ww, &req, &compromise);
		    if (result != XtGeometryNo) {
			if (result == XtGeometryAlmost) {
			    XtMakeGeometryRequest ((Widget)ww, &compromise, NULL);
			}
			ReallocCollideLists(ww);
		    }
		}
	    }
#endif
	    ChangeWidgetInCollideList(ww, child);

 

	    nc->workspace.x_left = child->core.x;
	    nc->workspace.x_right = child->core.x + child->core.width - 1;
	    nc->workspace.y_upper = child->core.y;
	    nc->workspace.y_lower = child->core.y + child->core.height - 1;
	    nc->workspace.x_delta = nc->workspace.y_delta = 0;
	    nc->workspace.x_center = child->core.x + (child->core.width / 2);
	    nc->workspace.y_center = child->core.y + (child->core.height / 2);
	    /*  Register initial sort position (before any adjustment)  */
	    if( ww->workspace.sort_policy == XmALIGNMENT_BEGINNING )
	        nc->workspace.x_index = nc->workspace.x_left;
	    else if( ww->workspace.sort_policy == XmALIGNMENT_CENTER )
	        nc->workspace.x_index = nc->workspace.x_center;
	    else
	        nc->workspace.x_index = nc->workspace.x_right;
	    /*  Adjust position to constraint-grid location  */
	    if( ww->workspace.snap_to_grid )
	    {
	        /*  Move into closest grid position (any direction)  */
	        SnapToGrid(ww, child, nc, FALSE);
	        /*  If no other adjustments will be made, move it now  */
	        if( ww->bulletin_board.allow_overlap )
	            MoveChild(ww, child, nc);
	    }
	    if( ww->bulletin_board.allow_overlap )
	        nc->workspace.is_newly_managed = FALSE;
	    else
	    {
	        nc->workspace.is_newly_managed = TRUE;
	        resolve_overlap = TRUE;
	    }
	}
	nc->workspace.is_managed = child->core.managed;
    }

    /*  If a child has been added, make sure it doesn't add any overlaps  */
    if ((resolve_overlap) && (!ww->workspace.auto_arrange))
	(void)PerformSpaceWars(ww, TRUE, NULL);
    /* %% Check window size and extents for need to resize */
    if( ww->workspace.lines )
	{
	if ( (!(nc->workspace.is_managed && child->core.managed)) ||
		new_node )
	    {
	    /*  Establish new routes for lines that were moved  */
	    RerouteLines(ww, FALSE);
	    if( ww->workspace.expose_left < ww->workspace.expose_right )
		{
		if (XtWindow(ww))
		    XClearArea(XtDisplay(ww), XtWindow(ww),
			       ww->workspace.expose_left,
			       ww->workspace.expose_upper,
			       ww->workspace.expose_right - 
			       ww->workspace.expose_left,
			       ww->workspace.expose_lower - 
			       ww->workspace.expose_upper, False);
		}
	    RefreshLines(ww);
	    }
	}
}

/*
 *  Subroutine: XtGeometryResult
 *  Purpose:    Get informed of a change in a child widget's geometry.
 */
static XtGeometryResult 
GeometryManager( Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply )
{
    return _GeometryManager (w, request, reply, True);
}

/*
 * Expose the ability to move standins programatically.  Using
 * XtSetValues() doesn't work for this because our GeometryManager
 * is never called.  Using XtMakeGeometryRequest() almost does the job
 * but that function is doc'ed as being only for widget writers
 * to use and not for application programmers.  In addition to
 * changing x,y location of standins, we need to ensure that
 * line routing is handled at the same time.
 */
Boolean XmWorkspaceSetLocation (Widget w, Dimension x, Dimension y)
{
    XmWorkspaceWidget ww;
    XtWidgetGeometry req;
    XtGeometryResult result;

    ww = (XmWorkspaceWidget)XtParent(w);
    req.request_mode = 0;


    /* Using 32767 is a hack.  I just happen to know that that it's
     * what we use elsewhere in dxui to indicate no-change.
     */
    if (x != 32767) {
	req.request_mode|= CWX;
	req.x = x;
    }
    if (y != 32767) {
	req.request_mode|= CWY;
	req.y = y;
    }

    if (req.request_mode == 0) return TRUE;
	result = XtMakeGeometryRequest (w, &req, NULL);
    return (result == XtGeometryYes);
}

static XtGeometryResult 
_GeometryManager( Widget w, XtWidgetGeometry *request, 
	XtWidgetGeometry *reply, Boolean adjust_constraints )
{
XmWorkspaceWidget ww;
XmWorkspaceConstraints constraints = WORKSPACE_CONSTRAINT(w);
Boolean resolve_overlap = False;
XtGeometryResult retval;

    ww = (XmWorkspaceWidget)XtParent(w);

    if ((adjust_constraints) && (ww->workspace.auto_arrange))
	retval = (*superclass->composite_class.geometry_manager) (w, request, reply);
    else
	retval = XtGeometryYes;

    if ((retval == XtGeometryYes)||(retval == XtGeometryDone)) {
	if (request->request_mode & CWWidth)
	    w->core.width = request->width;
	if (request->request_mode & CWHeight)
	    w->core.height = request->height;
	if (request->request_mode & CWBorderWidth)
	    w->core.border_width = request->border_width;
	if (request->request_mode & CWX)
	    w->core.x = request->x;
	if (request->request_mode & CWY)
	    w->core.y = request->y;
    }

    /*if ((retval == XtGeometryNo) || (retval == XtGeometryAlmost)) return retval;*/

#if 1
    /*
     * grow the workspace if the child won't fit
     */
    { 
	XtWidgetGeometry compromise,req; int newdim;
	XtGeometryResult result;
	newdim = w->core.x + w->core.width;
	req.request_mode = 0;
	if (newdim > ww->core.width) {
	    req.request_mode|= CWWidth;
	    req.width = newdim + 10;
	}
	newdim = w->core.y + w->core.height;
	if (newdim > ww->core.height) {
	    req.request_mode = CWHeight;
	    req.height = newdim + 10;
	}
	if (req.request_mode) {
	    result = XtMakeGeometryRequest ((Widget)ww, &req, &compromise);
	    if (result != XtGeometryNo) {
		if (result == XtGeometryAlmost) {
		    XtMakeGeometryRequest ((Widget)ww, &compromise, NULL);
		}
		ReallocCollideLists(ww);
	    }
	}
    }
#endif

    constraints->workspace.x_left = w->core.x;
    constraints->workspace.x_right = w->core.x + w->core.width - 1;
    constraints->workspace.y_upper = w->core.y;
    constraints->workspace.y_lower = w->core.y + w->core.height - 1;
    constraints->workspace.x_center = w->core.x + (w->core.width/2);
    constraints->workspace.y_center = w->core.y + (w->core.height/2);
    constraints->workspace.x_delta = 0;
    constraints->workspace.y_delta = 0;
    constraints->workspace.x_center = w->core.x + (w->core.width / 2);
    constraints->workspace.y_center = w->core.y + (w->core.height / 2);

    /*  Register initial sort position (before any adjustment)  */
    if( ww->workspace.sort_policy == XmALIGNMENT_BEGINNING )
        constraints->workspace.x_index = constraints->workspace.x_left;
    else if( ww->workspace.sort_policy == XmALIGNMENT_CENTER )
        constraints->workspace.x_index = constraints->workspace.x_center;
    else
        constraints->workspace.x_index = constraints->workspace.x_right;
    /*  Adjust position to constraint-grid location  */
    if( ww->workspace.snap_to_grid )
        {
        /*  Move into closest grid position (any direction)  */
        SnapToGrid(ww, w, constraints, FALSE);
        /*  If no other adjustments will be made, move it now  */
        if( ww->bulletin_board.allow_overlap )
            MoveChild(ww, w, constraints);
        }
    if( ww->bulletin_board.allow_overlap )
        constraints->workspace.is_newly_managed = FALSE;
    else
        {
        constraints->workspace.is_newly_managed = TRUE;
        resolve_overlap = TRUE;
        }
    if ((resolve_overlap) && (!ww->workspace.auto_arrange))
	{
	(void)PerformSpaceWars(ww, TRUE, NULL);
	}

    if (!constraints->workspace.line_invisibility)
	ChangeWidgetInCollideList(ww, w);

    return retval;
}


/*  Subroutine: ConstraintInitialize
 *  Purpose:    Update parameters in a child widget's constraints (except
 *              those pertaining to size) and install the accerators.
 */
static void ConstraintInitialize( Widget req, Widget new )
{
    XtTranslations acc_table;

    /*  Unitialized fields are not automatically cleared!  */
    XmWorkspaceConstraints constraints = WORKSPACE_CONSTRAINT(new);
    constraints->workspace.source_lines = NULL;
    constraints->workspace.destination_lines = NULL;
    constraints->workspace.will_be_selected = FALSE;
    constraints->workspace.is_accented = FALSE;
    constraints->workspace.is_managed = FALSE;

    /*  Accelerators are per-instance and must be installed at runtime  */
    /*
     * on behalf of vpe pages... don't put accelerators on the child
     * if the child is another workspace.
     */
    if (XtClass(new) != xmWorkspaceWidgetClass) {
	acc_table = XtParseTranslationTable(acceleratorTranslations);
	XtOverrideTranslations (new, acc_table);
    } else {

	/*
	 * FIXME:  there are 2 resources with the same name: XmNselectable (of the
	 * workspace itself) and XmNselectable (a constraint provided by the workspace
	 * to its children).  That means there is no way to set the resource on
	 * workspace whose parent is also a workspace.
	 */
	constraints->workspace.is_selectable = False;
    }
}


/*  Subroutine: ConstraintDestroy
 *  Purpose:    Free space allocated for use in the child's constraints
 */
static void ConstraintDestroy( Widget w )
{
XmWorkspaceWidget ww = (XmWorkspaceWidget)XtParent(w);
XmWorkspaceConstraints constraints = WORKSPACE_CONSTRAINT(w);

    /*  Make sure child is no longer counted among the select  */
    if( constraints->workspace.is_selected )
	ww->workspace.num_selected--;
    if( constraints->workspace.select_callbacks )
	RemoveConstraintCallbacks(&constraints->workspace.select_callbacks);
    if( constraints->workspace.accent_callbacks )
	RemoveConstraintCallbacks(&constraints->workspace.accent_callbacks);
    if( constraints->workspace.resizing_callbacks )
	RemoveConstraintCallbacks(&constraints->workspace.resizing_callbacks);
}


/*  Subroutine: ConstraintSetValues
 *  Purpose:    Check for application changes to constraint parameters and
 *              take actions indicated.
 */
static Boolean ConstraintSetValues( Widget current, Widget request, Widget new )
{
    Boolean retval;
    Cardinal zero = 0;
    XmWorkspaceConstraints new_constraints = WORKSPACE_CONSTRAINT(new);
    XmWorkspaceConstraints old_constraints = WORKSPACE_CONSTRAINT(current);

    retval = (*superclass->constraint_class.set_values)(current,request,new,NULL,&zero);

    new->core.x = current->core.x;
    new->core.y = current->core.y;
    if( new_constraints->workspace.is_selected !=
	old_constraints->workspace.is_selected )
    {
	if( new_constraints->workspace.is_selected )
	{
	    if(   new_constraints->workspace.is_selectable
	       && new_constraints->workspace.is_managed ) {
		new_constraints->workspace.is_selected = FALSE;
	        SelectChild((XmWorkspaceWidget)XtParent(new), new, new_constraints, 
		    (XEvent *)NULL);
	    }
	}
	else {
	    new_constraints->workspace.is_selected = TRUE;
	    UnselectChild((XmWorkspaceWidget)XtParent(new), new, new_constraints, NULL);
	}
    }
    if( new_constraints->workspace.is_selectable !=
	old_constraints->workspace.is_selectable )
    {
	if( (new_constraints->workspace.is_selectable == FALSE)
	   && new_constraints->workspace.is_selected )
	    UnselectChild((XmWorkspaceWidget)XtParent(new), new, new_constraints, NULL);
    }

    if (new_constraints->workspace.line_invisibility != 
	old_constraints->workspace.line_invisibility) {
	if (new_constraints->workspace.line_invisibility) {
	    DeleteWidgetFromCollideList(new);
	} else {
	    AddWidgetToCollideList(new);
	}
	RerouteLines ((XmWorkspaceWidget)XtParent(new), FALSE);
    }

    return retval;
}


/*  Subroutine: Arm
 *  Purpose:    Action for when button1 is pressed
 */
static void Arm( XmWorkspaceWidget ww, XEvent* event )
{

    if (ww->workspace.auto_arrange) return ;

    /*  Check for the second click of a double click  */
    if (ww->workspace.button1_press_mode == False)
	{
	StartRubberband(ww, event);
	}
    else 
	{
	if( (event->xbutton.time - ww->workspace.time)	/* Double click */
       		<= ww->workspace.double_click_interval )
	    {
	    ; /* Nada */
	    }
    	else	/* Single Click */
	    {
	    XmWorkspaceCallbackStruct call_value;

	    call_value.reason = XmCR_BACKGROUND;
	    call_value.event = event;
	    call_value.window = event->xbutton.window;
	    call_value.selected_widget = NULL;
	    XtCallCallbacks((Widget)ww, XmNbackgroundCallback, &call_value);
	    }
	}
	/*  Third click will not put us back here
	ww->workspace.time = 0;
	return; */

    ww->workspace.time = event->xbutton.time;
}

/*  Subroutine: Disarm
 *  Purpose:    Action for when button1 is released
 */
static void Disarm( XmWorkspaceWidget ww, XEvent* event )
{
Boolean bool = True;

    if( ww->workspace.button_tracker != NULL )
    {
	event->xbutton.x += ww->workspace.track_x;
	event->xbutton.y += ww->workspace.track_y;
	ww->workspace.button_tracker(ww->workspace.track_widget,
	                             ww->workspace.track_client_data,
	                             event
				     ,&bool
				     );
    }
    else if( ww->workspace.is_rubberbanding )
	EndRubberband(ww, event);
}

/*
 * Action for using the keyboard in the empty canvas
 * Should move selected children if any
 * Handling keysyms should not be done here.  The proper way is to write a
 * translation table that ties specific key events to their operations.  This
 * isn't working on my computer however.  The keyboard's arrow keys never 
 * percolate this far if the translation table specifies them.
 */
static void KeyboardNavigation( XmWorkspaceWidget ww, XEvent* event )
{
    KeySym keysym;
    int delta_x, delta_y;
    int dx=1, dy=1;
    int i;
    int minx=0, miny=0;
    short first = True;
    XmWorkspaceConstraints constraints;
    switch (event->type) {
	case KeyPress:
	    keysym = XLookupKeysym (&event->xkey, 0);
	    if ((keysym == XK_Left) || (keysym == XK_Right) ||
		(keysym == XK_Up) || (keysym == XK_Down)) {
		if( ww->workspace.movement_is_allowed && (!ww->workspace.is_moving) &&
		    !(event->xbutton.state & ControlMask) )
		{
		    /*  Prepare to move  */
		    ww->workspace.grab_x = 0;
		    ww->workspace.grab_y = 0;
		    ww->workspace.is_moving = TRUE;
		    ww->workspace.is_resizing = FALSE;
		    /*  Use base variables to keep track of move applied so far  */
		    ww->workspace.base_x = 0;
		    ww->workspace.base_y = 0;
		    /* loop over all selected */
		    /* set min_x, min_y so that we know when to stop sliding left,up */
		    for(i=0; i<ww->composite.num_children; i++)
		    {
			Widget child = ww->composite.children[i];
			constraints = WORKSPACE_CONSTRAINT(child);
			if(constraints->workspace.is_selected)
			{
			    if (first) {
				minx = child->core.x;
				miny = child->core.y;
				first = 0;
			    } else {
				minx = MIN(minx, child->core.x);
				miny = MIN(miny, child->core.y);
			    }
			}
		    }
		}
	    }
	    if (first) ww->workspace.is_moving = FALSE;
	    delta_x = delta_y = 0;
	    if (ww->workspace.snap_to_grid) {
		if (ww->workspace.horizontal_alignment != XmALIGNMENT_NONE)
		    dx = ww->workspace.grid_width;
		if (ww->workspace.vertical_alignment != XmALIGNMENT_NONE)
		    dy = ww->workspace.grid_height;
	    }
	    switch (keysym) {
		case XK_Left:
		    if ((minx-dx)<0) ww->workspace.is_moving = FALSE;
		    delta_x = -dx;
		    break;
		case XK_Right:
		    delta_x = dx;
		    break;
		case XK_Up:
		    if ((miny-dy)<0) ww->workspace.is_moving = FALSE;
		    delta_y = -dy;
		    break;
		case XK_Down:
		    delta_y = dy;
		    break;
		default:
		    break;
	    }
	    /*
	     * If check_overlap is turned on then we set up the surrogates
	     * and run the check for overlapping before allow the move to
	     * go ahead.
	     */
	    if ((ww->workspace.check_overlap) && (ww->workspace.is_moving)) {
		SaveOutlines (ww);
		if (Overlapping (ww, delta_x, delta_y ))
		    ww->workspace.is_moving = FALSE;
		DiscardSurrogate (ww);
	    }
	    if (ww->workspace.is_moving) {
		dropResizedSurrogates (ww, delta_x, delta_y, event);
	    }
	    break;
	default:
	    break;
    }
}
/*  Subroutine: Drag
 *  Purpose:    Action for when the mouse is dragged with button1 down
 */
static void Drag( XmWorkspaceWidget ww, XEvent* event )
{
Boolean bool = True;

    if (ww->workspace.auto_arrange) return ;

    if( ww->workspace.button_tracker != NULL )
    {
	event->xbutton.x += ww->workspace.track_x;
	event->xbutton.y += ww->workspace.track_y;
	ww->workspace.button_tracker(ww->workspace.track_widget,
	                             ww->workspace.track_client_data,
	                             event
				     ,&bool
				     );
    }
    else if( ww->workspace.is_rubberbanding )
	StretchRubberband(ww, event);
}


/*  Subroutine: GetRubberbandGC
 *  Purpose:    Get a GC with the right resources for drawing the bounding box
 */
static void GetRubberbandGC( XmWorkspaceWidget ww )
{
    XGCValues values;
    XtGCMask  valueMask;

    values.graphics_exposures = False;
    values.foreground = ww->manager.foreground ^ ww->core.background_pixel;
    values.background = 0;
    values.function = GXxor;
    values.line_style = LineOnOffDash;
    values.subwindow_mode = IncludeInferiors;
    valueMask = GCForeground | GCBackground | GCGraphicsExposures
	| GCFunction | GCSubwindowMode | GCLineStyle;
    ww->workspace.bandGC = XtGetGC((Widget)ww, valueMask, &values);
    values.line_style = LineSolid;
    /*values.foreground = WhitePixel(XtDisplay(ww), XScreenNumberOfScreen(XtScreen(ww)));*/
    ww->workspace.badBandGC = XtGetGC((Widget)ww, valueMask, &values);
}


/*  Subroutine: DrawRubberband
 *  Purpose:    Draw or undraw the rubberband bounding box
 */
static void DrawRubberband( XmWorkspaceWidget ww )
{
    XDrawRectangles(XtDisplay(ww), XtWindow(ww), ww->workspace.bandGC,
	            ww->workspace.rubberband, 1);
    ww->workspace.band_is_visible = !ww->workspace.band_is_visible;
}

static Boolean
inHierarchy (Widget child, XmWorkspaceWidget ww, int i)
{
Widget parent;

    parent = child;
    while ((parent) && (parent!=ww->composite.children[i]) && (parent != (Widget)ww))
	parent = XtParent(parent);

    if (parent == ww->composite.children[i]) return True;
    return False;
}

static XmWorkspaceWidget
WorkspaceOfWidget (Widget w)
{
XmWorkspaceWidget ww;

    /*
     * Make the routine less restrictive in that it can cope with being invoked
     * by any descendant, not just the immediate children.
     */
    ww = (XmWorkspaceWidget)w;

    while ((ww) && (!XtIsSubclass ((Widget)ww, xmWorkspaceWidgetClass)))
        ww = (XmWorkspaceWidget)XtParent((Widget)ww);
    if (!ww) return 0;
    if (!XtIsSubclass ((Widget)ww, xmWorkspaceWidgetClass)) return 0;
    return ww;
}


static void ChildNavigation (Widget w, XEvent* event, String* ignore, Cardinal* ignore2)
{
XmWorkspaceWidget ww;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    KeyboardNavigation (ww, event);
}


/*  Subroutine: GrabSelections
 *  Purpose:    Action for Button1Down in child widget (accelerator translation)
 */
/* ARGSUSED */
static void GrabSelections( Widget w, XEvent* event,
	                    String* params, Cardinal* num_params )
{
    XmWorkspaceWidget ww;
    Widget grab_child;
    XmWorkspaceConstraints constraints;
    int i;

    if (!(ww = WorkspaceOfWidget(w))) return ;
    ww->workspace.overlaps = FALSE;

    if (ww->workspace.auto_arrange) return ;

    /*  
     * Determine which child was grabbed, but use a less restricitve method.
     * Don't assume that the window of the event is the window of a child
     * widget.  Assume only the the window is descendant of a child widget.
     */
    for (i=0; i<ww->composite.num_children; i++)
	if (inHierarchy(XtWindowToWidget(XtDisplay(ww), event->xany.window), ww, i))
	    break;
    /*  This call comes from the accelerator so it should have a child  */
    if( i >= ww->composite.num_children )
	return;
    grab_child = ww->composite.children[i];
    constraints = WORKSPACE_CONSTRAINT(grab_child);
    /*  Check if child is sensitive  */
    if( (constraints->workspace.is_selectable == FALSE ) ||
	(ww->workspace.is_selectable == FALSE ) )
	return;
    /*  Check for the second click of a double click  */
    if( (event->xbutton.time - ww->workspace.time)
       <= XtGetMultiClickTime(XtDisplay((Widget)ww)) )
    {
	XmWorkspaceCallbackStruct call_value;

	call_value.reason = XmCR_DEFAULT_ACTION;
	call_value.event = event;
	call_value.window = event->xbutton.window;
	call_value.selected_widget = grab_child;
	XtCallCallbacks((Widget)ww, XmNdefaultActionCallback, &call_value);

	/*  Third click will not put us back here  */
	ww->workspace.time = 0;
	return;
    }
    ww->workspace.time = event->xbutton.time;
    /*  Handle change of selection status  */
    if( constraints->workspace.is_selected == FALSE )
    {
	if(   (   ((event->xbutton.state & ShiftMask) == 0)
	       || (ww->workspace.selection_policy == XmSINGLE_SELECT))
	   && (ww->workspace.num_selected > 0) )
	    UnselectAll(ww, event);
	SelectChild(ww, grab_child, constraints, (XEvent *)event);
    }
    /*  If this is a deselect, there should be no move  */
    else if( event->xbutton.state & ShiftMask )
    {
	UnselectChild(ww, grab_child, constraints, event);
	return;
    }
    if( ww->workspace.movement_is_allowed &&
	!(event->xbutton.state & ControlMask) )
    {
	/*  Prepare to move  */
	CreateSurrogate(ww, grab_child);
	ww->workspace.grab_x = event->xbutton.x;
	ww->workspace.grab_y = event->xbutton.y;
	ww->workspace.is_moving = TRUE;
	ww->workspace.is_resizing = FALSE;
	/*  Use base variables to keep track of move applied so far  */
	ww->workspace.base_x = 0;
	ww->workspace.base_y = 0;
    }
    if( (constraints->workspace.is_v_resizable || 
	 constraints->workspace.is_h_resizable) &&
	(event->xbutton.state & ControlMask) )
    {
	for(i=0; i<ww->composite.num_children; i++)
	{
	    Widget child = ww->composite.children[i];
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if(!(constraints->workspace.is_v_resizable  ||
		 constraints->workspace.is_h_resizable) &&
		constraints->workspace.is_selected)
		UnselectChild(ww, child, constraints, event);
	}

	/*  Prepare to resize  */
	CreateSurrogate(ww, grab_child);
	ww->workspace.grab_x = grab_child->core.width;
	ww->workspace.grab_y = grab_child->core.height;
	ww->workspace.is_resizing = TRUE;
	ww->workspace.is_moving = FALSE;
	/*  Use base variables to keep track of move applied so far  */
	ww->workspace.base_x = grab_child->core.x;
	ww->workspace.base_y = grab_child->core.y;
	XWarpPointer(XtDisplay(ww), None, XtWindow(grab_child),
	    0,0,0,0, grab_child->core.width-1, grab_child->core.height-1);
    }
}


/*  Subroutine: MoveResizeSelections
 *  Purpose:    Action during move of selected children
 */
/* ARGSUSED */
static void MoveResizeSelections( Widget w, XEvent* event,
	                    String* params, Cardinal* num_params )
{
XmWorkspaceWidget ww;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    if( ww->workspace.is_moving )
    {
	int delta_x, delta_y;
	if( ww->workspace.move_cursor_installed == FALSE )
	{
	    XDefineCursor(XtDisplay(ww), XtWindow(ww),
	                  xmWorkspaceClassRec.workspace_class.move_cursor);
	    ww->workspace.move_cursor_installed = TRUE;
	}
	delta_x = event->xbutton.x - ww->workspace.grab_x;
	delta_y = event->xbutton.y - ww->workspace.grab_y;
	if( delta_x < ww->workspace.min_x )
	    delta_x = ww->workspace.min_x;
	if( delta_y < ww->workspace.min_y )
	    delta_y = ww->workspace.min_y;
	delta_x -= ww->workspace.base_x;
	delta_y -= ww->workspace.base_y;
	MoveSurrogate(ww, delta_x, delta_y);
    }
    else if( ww->workspace.is_resizing)
    {
	int width, height, tx,ty; 
	tx = event->xbutton.x;
	ty = event->xbutton.y;
        childRelative (ww, event, &tx, &ty);

	width = MAX(10, tx);
	height = MAX(10, ty);
	ResizeSurrogate(ww, width, height);
    }
}

/*
 * The coords in event refer to the widget of the event which might
 * not be an immediate child of the workspace.  So, find the immediate
 * child, then translate coords in event to that widget.
 */
static void
childRelative (XmWorkspaceWidget ww, XEvent *xev, int *x, int *y)
{
Widget parent;
int tx, ty, i;
Widget woe = XtWindowToWidget(XtDisplay(ww), xev->xany.window);

    *x = tx = xev->xbutton.x; *y = ty = xev->xbutton.y;

    for (i=0; i<ww->composite.num_children; i++) 
	if (inHierarchy(woe, ww, i)) break;
    if (i==ww->composite.num_children) return ;

    parent = woe;
    while (XtParent(parent) != (Widget)ww) {
	tx+= parent->core.x;
	ty+= parent->core.y;
	parent = XtParent(parent);
    }
    *x = tx; *y = ty;
}



/*  Subroutine: DropSelections
 *  Purpose:    Action at end of move selected children
 */
/* ARGSUSED */
static void DropSelections( Widget w, XEvent* event,
	                    String* params, Cardinal* num_params )
{
    XmWorkspaceChildCallbackStruct cb;
    XmWorkspaceConstraints constraints;
    XmWorkspaceWidget ww;
    Widget child;
    int i;
    short delta_x, delta_y;
    short resize_x = 0;
    short resize_y = 0;
    Dimension width, height;
    XtWidgetGeometry request, reply;
    XtGeometryResult result;
    int n;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    if( ww->workspace.is_moving )
    {
	/*  We're done, so get rid of surrogate box(es)  */
	if( ww->workspace.surrogate_is_visible )
	    DrawSurrogate(ww);
	XSync(XtDisplay(ww), 0);
	/*  Determine the move, and clip to limits  */
#if RESIZE_HANDLES
	if ((num_params) && (*num_params==1) && (!strcmp(params[0], "NE"))) delta_x = 0;
	else 
#endif
	    delta_x = (event->xbutton.x - ww->workspace.grab_x);
	if( delta_x < ww->workspace.min_x )
	    delta_x = ww->workspace.min_x;
        else if( delta_x > ww->workspace.max_x )
            resize_x = delta_x - ww->workspace.max_x;
#if RESIZE_HANDLES
	if ((num_params) && (*num_params==1) && (!strcmp(params[0], "SW"))) delta_y = 0;
	else 
#endif
	    delta_y = (event->xbutton.y - ww->workspace.grab_y);
	if( delta_y < ww->workspace.min_y )
	    delta_y = ww->workspace.min_y;
        else if( delta_y > ww->workspace.max_y )
            resize_y = delta_y - ww->workspace.max_y;
	if(Overlapping (ww, delta_x-ww->workspace.base_x, delta_y-ww->workspace.base_y)) {
	    delta_x = delta_y = 0;
	    resize_x = resize_y = 0;
	}
	if( (resize_x > 0) || (resize_y > 0) )
	{
	    /*old_width = ww->core.width;*/
	    /*old_height = ww->core.height;*/
	    if(resize_x > 0)
		request.width = ww->core.width + resize_x + 
			ww->workspace.grid_width;
	    else
		request.width = ww->core.width;
	    if(resize_y > 0)
		request.height = ww->core.height + resize_y +
			ww->workspace.grid_height;
	    else
		request.height = ww->core.height;
	    request.request_mode = CWWidth | CWHeight;
	    result = XtMakeGeometryRequest((Widget)ww, &request, &reply);
	    if( result == XtGeometryAlmost )
	    {
	        short x_diff, y_diff;
	        x_diff = request.width - reply.width;
	        y_diff = request.height - reply.height;
	        result = XtMakeGeometryRequest((Widget)ww, &reply, &request);
	        if( x_diff > 0 )
	            delta_x -= x_diff;
	        if( y_diff > 0 )
	            delta_y -= y_diff;
	    }
	    else if( result == XtGeometryNo )
	    {
	        if( resize_x > 0 )
	            delta_x -= resize_x;
	        if( resize_y > 0 )
	            delta_y -= resize_y;
	    }
	    /* If the size of the workspace changed, reallocate the collision
	       lists */
	    if ( (result == XtGeometryAlmost) || (result == XtGeometryYes) )
		{
		ReallocCollideLists(ww);
		}
	}
	dropResizedSurrogates (ww, delta_x, delta_y, event);
    }
    if( ww->workspace.is_resizing)
    {
	int tx, ty;
	if( ww->workspace.surrogate_is_visible )
	    DrawSurrogate(ww);
	XSync(XtDisplay(ww), 0);

	/*  Determine the size, and clip to limits  */
	childRelative (ww, event, &tx, &ty);
	width = MAX(10, tx);
	height = MAX(10, ty);
#if RESIZE_HANDLES
	if ((num_params) && (*num_params == 1)) {
	    int i;

	    for (i=0; i<ww->composite.num_children; i++) {
		child = ww->composite.children[i];
		constraints = WORKSPACE_CONSTRAINT(child);
		if (constraints->workspace.is_selected) {
		    if (!strcmp (params[0], "NW")) {
			width = child->core.width - event->xbutton.x;
			height = child->core.height - event->xbutton.y;
		    } else if (!strcmp (params[0], "NE")) {
			height = child->core.height - event->xbutton.y;
		    } else if (!strcmp (params[0], "SW")) {
			width = child->core.width - event->xbutton.x;
		    }
		    break;
		}
	    }
	}
#endif
       	for( i=ww->composite.num_children-1; i>=0; i-- )
	{
	    child = ww->composite.children[i];
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if(constraints->workspace.is_selected)
	    {
		n = 0;
		cb.height = 0;
		cb.width  = 0;
		if(constraints->workspace.is_h_resizable)
		{
		    /*XtSetArg(wargs[n], XmNwidth, width);*/ n++;
		    cb.width = width;
		} else cb.width = child->core.width;
		if(constraints->workspace.is_v_resizable)
		{
		    /*XtSetArg(wargs[n], XmNheight, height);*/ n++;
		    cb.height = height;
		} else cb.height = child->core.height;
		if(n > 0)
		{
		    /*XtSetValues(child, wargs, n);*/
		    XtResizeWidget (child, cb.width, cb.height, child->core.border_width);
		    
		    constraints->workspace.x_right = constraints->workspace.x_left +
			cb.width;
		    constraints->workspace.y_lower = constraints->workspace.y_upper +
			cb.height;
		    CallConstraintCallbacks(child, 
			constraints->workspace.resizing_callbacks,
	                    (XmAnyCallbackStruct *)&cb);
		}
	    }
	}
	ww->workspace.is_resizing = FALSE;
	/*  Deal with overlaps by movement or stacking  */
	if( ww->bulletin_board.allow_overlap  )
	    RestackSelectedChildren(ww);
	else
	    PerformSpaceWars(ww, FALSE, event);
    }
}

/*
 * Broken out of DropSelections so that the code can be called from a similar
 * operation performed using the keyboard instead of the mouse
 */
static void dropResizedSurrogates (XmWorkspaceWidget ww, int delta_x, int delta_y, XEvent* event)
{
    XmWorkspaceConstraints constraints;
    XmWorkspaceLine line;
    Widget child;
    int i;
    if( (delta_x != 0) || (delta_y != 0) ) {
	/* Go through all the children and see if they were selected.  If
	 * they were, "hide" them in the collide lists, so they do not 
	 * cause collisions with the ones that are moved first.  For the
	 * lines, simply remove them from the lists.  They will be added
	 * back in when they are re-reouted 
	 */
	for( i=ww->composite.num_children-1; i>=0; i-- )
	    {
	    child = ww->composite.children[i];
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if( constraints->workspace.is_selected == TRUE )
		{
		constraints->workspace.x_delta = delta_x;
		constraints->workspace.y_delta = delta_y;
		if( ww->workspace.snap_to_grid )
		    SnapToGrid(ww, child, constraints, FALSE);
		if ( (constraints->workspace.x_delta != 0 ) ||
		     (constraints->workspace.y_delta != 0) )
		    {
		    if (!constraints->workspace.line_invisibility)
			HideWidgetInCollideList(ww, child);
		    for( line = constraints->workspace.source_lines;
			 line;
			 line = line->src_next )
			{
			if( line->is_to_be_moved == FALSE )
			    {
			    RemoveLineFromCollideList(ww, line);
			    line->is_to_be_moved = TRUE;
			    AugmentExposureAreaForLine(ww, line);
			    }
			}
		    for( line = constraints->workspace.destination_lines;
			line;
			line = line->dst_next )
			{
			if( line->is_to_be_moved == FALSE )
			    {
			    RemoveLineFromCollideList(ww, line);
			    line->is_to_be_moved = TRUE;
			    AugmentExposureAreaForLine(ww, line);
			    }
			}
		    }
		}
	    }
	for( i=ww->composite.num_children-1; i>=0; i-- )
	{
	    child = ww->composite.children[i];
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if( constraints->workspace.is_selected == TRUE )
	    {
		constraints->workspace.x_delta = delta_x;
		constraints->workspace.y_delta = delta_y;
		if( ww->workspace.snap_to_grid )
		    SnapToGrid(ww, child, constraints, FALSE);
		if ( ((constraints->workspace.x_delta != 0) ||
		     (constraints->workspace.y_delta != 0)) && 
		      !ww->workspace.suppress_callbacks )
		    {
		    XmWorkspacePositionChangeCallbackStruct call_data;

		    call_data.child = child;
		    call_data.x = constraints->workspace.x_left + 
			constraints->workspace.x_delta;
		    call_data.y = constraints->workspace.y_upper + 
			constraints->workspace.y_delta;
		    call_data.event = event;
		    XtCallCallbacks((Widget)
			ww, XmNpositionChangeCallback, &call_data);
		    }
		if( ww->bulletin_board.allow_overlap )
		    MoveChild(ww, child, constraints);
		else
		{
		    if ((POLICY(ww) == XmSPACE_WARS_LEFT_MOST_STAYS)||
			(POLICY(ww) == XmSPACE_WARS_SELECTED_MIGRATE))
			/*  Register position where widget was dropped  */
			constraints->workspace.x_index += delta_x;
		    else
			/*  Register position post-snap_to_grid  */
			constraints->workspace.x_index +=
			  constraints->workspace.x_delta;
		}
	    }
	}
    }
    /*  Deal with overlaps by movement or stacking  */
    if( ww->bulletin_board.allow_overlap  )
	RestackSelectedChildren(ww);
    else
	(void)PerformSpaceWars(ww, FALSE, event);
    ww->workspace.is_moving = FALSE;
    /*  Redraw any affected application lines  */
    if( ww->workspace.lines )
    {
	/*  Establish new routes for lines that were moved  */
	RerouteLines(ww, FALSE);
	if ( (delta_x != 0) || (delta_y != 0) )
	{
	    if( ww->workspace.expose_left < ww->workspace.expose_right )
	    {
		if (XtWindow(ww))
		    XClearArea(XtDisplay(ww), XtWindow(ww),
		       ww->workspace.expose_left,
		       ww->workspace.expose_upper,
		       ww->workspace.expose_right - 
		       ww->workspace.expose_left,
		       ww->workspace.expose_lower - 
		       ww->workspace.expose_upper, False);
	    }
	}
    }
    if( ww->workspace.move_cursor_installed )
    {
	if (XtWindow(ww))
	    XUndefineCursor(XtDisplay(ww), XtWindow(ww));
	ww->workspace.move_cursor_installed = FALSE;
    }
}

/*  Subroutine: MoveChild
 *  Purpose:    Move child widget by given offsets and update constraints notes
 */
static void MoveChild( XmWorkspaceWidget ww, Widget child,
	               XmWorkspaceConstraints constraints )
{
    XmWorkspaceLine line;
    register short x_delta, y_delta;

    x_delta = constraints->workspace.x_delta;
    y_delta = constraints->workspace.y_delta;
    if( (x_delta == 0) && (y_delta == 0) )
	return;
    constraints->workspace.x_left += x_delta;
    constraints->workspace.x_center += x_delta;
    constraints->workspace.x_right += x_delta;
    constraints->workspace.y_upper += y_delta;
    constraints->workspace.y_center += y_delta;
    constraints->workspace.y_lower += y_delta;
    XtMoveWidget(child, constraints->workspace.x_left,
	         constraints->workspace.y_upper);
    if (!constraints->workspace.line_invisibility)
	ChangeWidgetInCollideList(ww, child);
    ResetChild(ww, constraints);
    /*  Note movement for managed lines  */
    for( line = constraints->workspace.source_lines;
	 line;
	 line = line->src_next )
    {
	line->src_x += x_delta;
	line->src_y += y_delta;
	if( line->is_to_be_moved == FALSE )
	{
	    line->is_to_be_moved = TRUE;
	    AugmentExposureAreaForLine(ww, line);
	}
    }
    for( line = constraints->workspace.destination_lines;
	 line;
	 line = line->dst_next )
    {
	line->dst_x += x_delta;
	line->dst_y += y_delta;
	if( line->is_to_be_moved == FALSE )
	{
	    line->is_to_be_moved = TRUE;
	    AugmentExposureAreaForLine(ww, line);
	}
    }
}


/*  Subroutine: ResetChild
 *  Purpose:    Reset parameters changed during transitional state to
 *              their default or static state values.
 */
static void ResetChild( XmWorkspaceWidget ww,
	                XmWorkspaceConstraints constraints )
{
    /*  Unset move request  */
    constraints->workspace.x_delta = 0;
    constraints->workspace.y_delta = 0;
    /*  Register current sort position  */
    if( ww->workspace.sort_policy == XmALIGNMENT_BEGINNING )
	constraints->workspace.x_index = constraints->workspace.x_left;
    else if( ww->workspace.sort_policy == XmALIGNMENT_CENTER )
	constraints->workspace.x_index = constraints->workspace.x_center;
    else
	constraints->workspace.x_index = constraints->workspace.x_right;
}


/*  Subroutine: SnapToGrid
 *  Purpose:    Given a widget, choose the nearest grid position based
 *              on the current policy;
 */
static void SnapToGrid( XmWorkspaceWidget ww, Widget child,
	                XmWorkspaceConstraints constraints, Boolean increase )
{
    int x, width;
    int y, height;

    if (XtClass(child) == xmWorkspaceWidgetClass) return ;
    x = constraints->workspace.x_left + constraints->workspace.x_delta;
    width = 1 + constraints->workspace.x_right - constraints->workspace.x_left;
    Align1D(&x, width, ww->workspace.grid_width,
	    ww->workspace.horizontal_alignment, increase);
    constraints->workspace.x_delta = x - constraints->workspace.x_left;
    y = constraints->workspace.y_upper + constraints->workspace.y_delta;
    height =
      1 + constraints->workspace.y_lower - constraints->workspace.y_upper;
    Align1D(&y, height, ww->workspace.grid_height,
	    ww->workspace.vertical_alignment, increase);
    constraints->workspace.y_delta = y - constraints->workspace.y_upper;
}


/*  Subroutine: Align1D
 *  Purpose:    Set position on one axis according to rules of alignment
 *  Note:       If increase is set, only positive adjustments will be made
 */
static void Align1D( int* x, int width, int grid, unsigned char alignment,
	             Boolean increase_only )
{
    int index, x_off, center;
    Boolean  odd;            /* Is (width / grid) an odd number? */

    if( grid == 1 )
	return;

    switch( alignment )
    {
      case XmALIGNMENT_NONE:
	break;
      case XmALIGNMENT_BEGINNING:
	if( increase_only )
	    index = (*x + 1.5*grid - 1) / grid;
	else
	    index = (*x + .5*grid) / grid;
	*x = index * grid;
	break;
      case XmALIGNMENT_CENTER:
	/*  x_off is center in widget for first grid cell if 1 or more used  */
	/*  (-1 seems to work better, but I don't really know why)  */
	x_off = (width % grid) / 2;
        odd   = (width / grid) % 2;
	center = grid / 2;

	if( increase_only )
	    /* -1 prevents advancing if perfect fit */
            index = (*x + x_off + center - 1) / grid;
	else
	    index = (*x + x_off + odd * center) / grid;
	*x = (grid * index) + !odd * center - x_off;
        while(*x < 0)
            *x += grid;
	break;

      case XmALIGNMENT_END:
	index = (*x + width + 0.5*grid - 1) / grid;
	*x = (index * grid) - width;
	break;
      default:
	break;
    }
    if (*x < 0) *x = 0;
}


/*  Subroutine: StartRubberband
 *  Purpose:    Initialize rubberbanding action
 */
static void StartRubberband( XmWorkspaceWidget ww, XEvent* event )
{
    if( ww->workspace.selection_policy != XmEXTENDED_SELECT )
	return;
    if( ww->workspace.bandGC == NULL )
	GetRubberbandGC(ww);
    /*  Shift at this point means leave unboxed items alone  */
    if( (event->xbutton.state & ShiftMask) == 0 )
    {
	UnaccentAll(ww, event);
	ww->workspace.shift_is_applied = FALSE;
    }
    else
    {
	register XmWorkspaceConstraints constraints;
	int i;
	/*  Update looks at prior inclusion status in this case, so clean up  */
	/*  Clear all inclusion marks for a fresh start  */
	for( i=0; i<ww->composite.num_children; i++ )
	{
	    constraints = WORKSPACE_CONSTRAINT(ww->composite.children[i]);
	    constraints->workspace.is_within_band = FALSE;
	}
	ww->workspace.shift_is_applied = TRUE;
    }
    ww->workspace.is_rubberbanding = TRUE;
    ww->workspace.base_x = ww->workspace.rubberband[0].x = event->xbutton.x;
    ww->workspace.base_y = ww->workspace.rubberband[0].y = event->xbutton.y;
    ww->workspace.rubberband[0].width = ww->workspace.rubberband[0].height = 1;
/*
    DrawRubberband(ww);
*/
}


/*  Subroutine: StretchRubberband
 *  Purpose:    Change the rubberband bounding box and do all that implies
 */
static void StretchRubberband( XmWorkspaceWidget ww, XEvent * event )
{
    unsigned char direction;

    if( ww->workspace.is_rubberbanding )
    {
	if( ww->workspace.band_is_visible )
	    DrawRubberband(ww);
	if( event->xbutton.x > ww->workspace.base_x )
	{
	    ww->workspace.rubberband[0].x = ww->workspace.base_x;
	    ww->workspace.rubberband[0].width =
	      1 + event->xbutton.x - ww->workspace.base_x;
	    direction = SZ_RIGHT;
	}
	else
	{
	    ww->workspace.rubberband[0].x = event->xbutton.x;
	    ww->workspace.rubberband[0].width =
	      1 + ww->workspace.base_x - event->xbutton.x;
	    direction = SZ_LEFT;
	}
	if( event->xbutton.y > ww->workspace.base_y )
	{
	    ww->workspace.rubberband[0].y = ww->workspace.base_y;
	    ww->workspace.rubberband[0].height =
	      1 + event->xbutton.y - ww->workspace.base_y;
	    direction += SZ_LOWER;
	}
	else
	{
	    ww->workspace.rubberband[0].y = event->xbutton.y;
	    ww->workspace.rubberband[0].height =
	      1 + ww->workspace.base_y - event->xbutton.y;
	    direction += SZ_UPPER;
	}
	/*  Make sure band is not present when/if widget changes appearance  */
	XSync(XtDisplay(ww), False);
	if( ww->workspace.size_xx_installed != direction )
	{
	    if (XtWindow(ww))
		XDefineCursor(XtDisplay(ww), XtWindow(ww),
	          xmWorkspaceClassRec.workspace_class.size_cursor[direction]);
	    ww->workspace.size_xx_installed = direction;
	}
	/*  Check for selection changes  */
	UpdateRubberbandSelections(ww, event);
	/*  Make sure selection changes are done before drawing new band  */
	XSync(XtDisplay(ww), False);
	DrawRubberband(ww);
	XSync(XtDisplay(ww), False);
    }
}


/*  Subroutine: EndRubberband
 *  Purpose:    Terminate rubberbanding action
 */
static void EndRubberband( XmWorkspaceWidget ww, XEvent* event )
{
    Widget child;
    XmWorkspaceConstraints constraints;
    int i;

    if( ww->workspace.is_rubberbanding )
    {
	ww->workspace.is_rubberbanding = FALSE;
	if( ww->workspace.band_is_visible )
	{
	    DrawRubberband(ww);
	    XSync(XtDisplay(ww), 0);
	}
	if( ww->workspace.size_xx_installed != SZ_NONE )
	{
	    XUndefineCursor(XtDisplay(ww), XtWindow(ww));
	    ww->workspace.size_xx_installed = SZ_NONE;
	}
	/*  Send notification to children whose selection status has changed  */
	for( i=0; i<ww->composite.num_children; i++ )
	{
	    child = ww->composite.children[i];
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if( constraints->workspace.will_be_selected !=
	        constraints->workspace.is_selected )
	    {
	        if( constraints->workspace.will_be_selected )
	            SelectChild(ww, ww->composite.children[i],
	                        constraints, (XEvent *)event);
	        else
	            UnselectChild(ww, ww->composite.children[i],
	                        constraints, event);
	    }
	}
/*
	RestackSelectedChildren(ww);
*/
    }
}


/*  Subroutine: UpdateRubberbandSelections
 *  Purpose:    Update selections to reflect current bounding box
 *  Note:       is_within_band shows whether widget was previously counted in
 *              will_be_selected shows whether has been marked for selection
 *              is_accented shows if widget has visible sign of being marked
 *              is_selected shows if last callback for widget was select true.
 */
static void UpdateRubberbandSelections( XmWorkspaceWidget ww, XEvent* event )
{
    XmWorkspaceConstraints constraints;
    register short x1, x2, y1, y2;
    int i;
    Boolean included;

    if(!ww->workspace.is_selectable) return;

    x1 = ww->workspace.rubberband[0].x;
    x2 = x1 + ww->workspace.rubberband[0].width - 1;
    y1 = ww->workspace.rubberband[0].y;
    y2 = y1 + ww->workspace.rubberband[0].height - 1;
    for( i=ww->composite.num_children-1; i>=0; i-- )
    {
	constraints = WORKSPACE_CONSTRAINT(ww->composite.children[i]);
	if( constraints->workspace.is_selectable )
	{
	    if( ww->workspace.inclusion_policy == XmINCLUDE_ANY )
	    {
	        /*  If any side beyond rubberband range, no part is included  */
	        if(   (x2 < constraints->workspace.x_left)
	           || (x1 > constraints->workspace.x_right)
	           || (y2 < constraints->workspace.y_upper)
	           || (y1 > constraints->workspace.y_lower) )
	            included = FALSE;
	        else
	            included = TRUE;
	    }
	    else if( ww->workspace.inclusion_policy == XmINCLUDE_CENTER )
	    {
	        if(   (x1 < constraints->workspace.x_center)
	           && (x2 > constraints->workspace.x_center)
	           && (y1 < constraints->workspace.y_center)
	           && (y2 > constraints->workspace.y_center) )
	            included = TRUE;
	        else
	            included = FALSE;
	    }
	    else        /*  XmINCLUDE_ALL  */
	    {
	        if(   (x1 < constraints->workspace.x_left)
	           && (x2 > constraints->workspace.x_right)
	           && (y1 < constraints->workspace.y_upper)
	           && (y2 > constraints->workspace.y_lower) )
	            included = TRUE;
	        else
	            included = FALSE;
	    }
	    /*  With shift, we are toggling states  */
	    if( ww->workspace.shift_is_applied )
	    {
	        /*  If inclusion status changed since last we checked  */
	        if( constraints->workspace.is_within_band != included )
	        {
	            if( constraints->workspace.will_be_selected )
	                UnaccentChild(ww, ww->composite.children[i],
	                              constraints, event);
	            else
	                AccentChild(ww, ww->composite.children[i],
	                            constraints, event);
	            constraints->workspace.is_within_band = included;
	        }
	    }
	    else
	    {
	        if( included )
	        {
	            if( constraints->workspace.will_be_selected == FALSE )
	                AccentChild(ww, ww->composite.children[i],
	                            constraints, event);
	        }
	        else
	        {
	            if( constraints->workspace.will_be_selected == TRUE )
	                UnaccentChild(ww, ww->composite.children[i],
	                              constraints, event);
	        }
	    }
	}
    }
}


/*  Subroutine: UnselectAll
 *  Purpose:    Unselect all selected children widgets
 */
static void UnselectAll( XmWorkspaceWidget ww, XEvent* event )
{
    XmWorkspaceConstraints constraints;
    Widget child;
    int i;

    for( i=ww->composite.num_children-1; i>=0; i-- )
    {
	child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(child);
	if( constraints->workspace.is_selected == TRUE )
	    UnselectChild(ww, child, constraints, event);
    }
}


/*  Subroutine: SelectChild
 *  Purpose:    Perform selection on given child widget
 */
static void SelectChild( XmWorkspaceWidget ww, Widget child,
	                 XmWorkspaceConstraints constraints, XEvent* event )
{
    XmWorkspaceChildCallbackStruct cb;

    /*  Make sure selection is visually accented  */
    if( constraints->workspace.will_be_selected == FALSE )
	AccentChild(ww, child, constraints, event);
    /*  Select this widget  */
    if (constraints->workspace.is_selected == FALSE)
	++ww->workspace.num_selected;
    constraints->workspace.is_selected = TRUE;
    /*  Tell the widget that it has been Selected  */
    cb.reason = XmCR_SELECT;
    cb.event = event;
    cb.status = TRUE;
    /*  Call the selection callback  */
    if(!ww->workspace.suppress_callbacks)
    {
	CallConstraintCallbacks(child, constraints->workspace.select_callbacks,
	                    (XmAnyCallbackStruct *)&cb);
    }
    XtCallCallbacks((Widget)ww, XmNselectionCallback, &cb);
}


/*  Subroutine: UnselectChild
 *  Purpose:    Unperform selection on given child widget
 */
static void UnselectChild( XmWorkspaceWidget ww, Widget child,
	                   XmWorkspaceConstraints constraints, XEvent* event )
{
    XmWorkspaceChildCallbackStruct cb;

    /*  Make sure selection is visually unaccented  */
    if( constraints->workspace.will_be_selected == TRUE )
	UnaccentChild(ww, child, constraints, event);
    /*  Deselect this widget  */
    if (constraints->workspace.is_selected == TRUE)
	--ww->workspace.num_selected;
    constraints->workspace.is_selected = FALSE;

    if(ww->workspace.suppress_callbacks)
	return;

    /*  Tell the widget that it has been deselected  */
    cb.reason = XmCR_SELECT;
    cb.event = event;
    cb.status = FALSE;
    /*  Call the selection callback  */
    CallConstraintCallbacks(child, constraints->workspace.select_callbacks,
	                    (XmAnyCallbackStruct *)&cb);
    XtCallCallbacks((Widget)ww, XmNselectionCallback, &cb);
}


/*  Subroutine: UnaccentAll
 *  Purpose:    Remove accents from all accented children
 */
static void UnaccentAll( XmWorkspaceWidget ww, XEvent* event )
{
    XmWorkspaceConstraints constraints;
    Widget child;
    int i;

    for( i=ww->composite.num_children-1; i>=0; i-- )
    {
	child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(child);
	if( constraints->workspace.will_be_selected == TRUE )
	    UnaccentChild(ww, child, constraints, event);
    }
}


/*  Subroutine: AccentChild
 *  Purpose:    Apply any indicated visual accent and call accent callback
 */
static void AccentChild( XmWorkspaceWidget ww, Widget child,
	                 XmWorkspaceConstraints constraints, XEvent* event )
{
    XmWorkspaceChildCallbackStruct cb;
    Arg warg;

    if( ww->workspace.accent_policy == XmACCENT_BACKGROUND )
    {
	constraints->workspace.accent_uncolor = child->core.background_pixel;
	if( XmIsPrimitive(child) || XmIsManager(child) )
	{
	    register int thickness;
	    if( XmIsPrimitive(child) )
	        thickness =
	          ((XmPrimitiveWidget)child)->primitive.shadow_thickness +
	          ((XmPrimitiveWidget)child)->primitive.highlight_thickness;
	    else
	        thickness = ((XmManagerWidget)child)->manager.shadow_thickness;
	    child->core.background_pixel = ww->workspace.accent_color;
	    if (XtWindow(child)) {
		XSetWindowBackground(XtDisplay(child), XtWindow(child),
				     child->core.background_pixel);
		XClearArea(XtDisplay(child), XtWindow(child), thickness, thickness,
			   child->core.width - (thickness + thickness),
			   child->core.height - (thickness + thickness), True);
	    }
	}
	else
	{
	    XtSetArg(warg, XmNbackground, ww->workspace.accent_color);
	    XtSetValues((Widget)child, &warg, 1);
	}
	constraints->workspace.is_accented = TRUE;
    }
    else if( ww->workspace.accent_policy == XmACCENT_BORDER )
    {
	constraints->workspace.accent_uncolor = child->core.border_pixel;
	XtSetArg(warg, XmNborderColor, ww->workspace.accent_color);
	XtSetValues((Widget)child, &warg, 1);
	constraints->workspace.is_accented = TRUE;
    }
    cb.reason = XmCR_ACCENT;
    cb.event = event;
    cb.status = ON;
    CallConstraintCallbacks(child, constraints->workspace.accent_callbacks,
	                    (XmAnyCallbackStruct *)&cb);
    constraints->workspace.will_be_selected = TRUE;
}


/*  Subroutine: UnaccentChild
 *  Purpose:    Remove any visual accents and call accent callback
 */
static void UnaccentChild( XmWorkspaceWidget ww, Widget child,
	                   XmWorkspaceConstraints constraints, XEvent* event )
{
    XmWorkspaceChildCallbackStruct cb;
    Arg warg;

    if( constraints->workspace.is_accented == TRUE )
    {
	if( ww->workspace.accent_policy == XmACCENT_BACKGROUND )
	{
	    if( XmIsPrimitive(child) || XmIsManager(child) )
	    {
	        register int thickness;
	        if( XmIsPrimitive(child) )
	            thickness =
	              ((XmPrimitiveWidget)child)->primitive.shadow_thickness +
	              ((XmPrimitiveWidget)child)->primitive.highlight_thickness;
	        else
	            thickness =
	              ((XmManagerWidget)child)->manager.shadow_thickness;
	        child->core.background_pixel =
	          constraints->workspace.accent_uncolor;
		if (XtWindow(child)) {
		    XSetWindowBackground(XtDisplay(child), XtWindow(child),
					 child->core.background_pixel);
		    XClearArea(XtDisplay(child), XtWindow(child),
			       thickness, thickness,
			       child->core.width - (thickness + thickness),
			       child->core.height - (thickness + thickness), True);
		}
	    }
	    else
	    {
	        XtSetArg(warg, XmNbackground,
	                 constraints->workspace.accent_uncolor);
	        XtSetValues((Widget)child, &warg, 1);
	    }
	}
	else if( ww->workspace.accent_policy == XmACCENT_BORDER )
	{
	    XtSetArg(warg, XmNborderColor,
	             constraints->workspace.accent_uncolor);
	    XtSetValues((Widget)child, &warg, 1);
	}
	constraints->workspace.is_accented = FALSE;
    }
    cb.reason = XmCR_ACCENT;
    cb.event = event;
    cb.status = OFF;
    CallConstraintCallbacks(child, constraints->workspace.accent_callbacks,
	                    (XmAnyCallbackStruct *)&cb);
    constraints->workspace.will_be_selected = FALSE;
}

static void RaiseSelections( Widget w, XEvent* event,
	                    String* params, Cardinal* num_params )
{
XmWorkspaceWidget ww;

    if (!(ww = WorkspaceOfWidget(w))) return ;
    if (ww->workspace.auto_arrange) return ;
    RestackSelectedChildren (ww);
}

/*  Subroutine: RestackSelectedChildren
 *  Purpose:    Change the stacking order such that all moved children appear
 *              in front of all unmoved children.
 */
static void RestackSelectedChildren( XmWorkspaceWidget ww )
{
    int i;
    if( ww->workspace.num_selected == 1 )
    {
	XmWorkspaceConstraints constraints;
	Widget child;

	for( i=0; i<ww->composite.num_children; i++ )
	{
	    child = ww->composite.children[i];
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if( constraints->workspace.is_selected == TRUE )
	    {
		if (XtWindow(ww->composite.children[i]))
		    XRaiseWindow(XtDisplay(ww), XtWindow(ww->composite.children[i]));
	        return;
	    }
	}
    }
    else if (XtWindow(ww))
    {
	Window root, parent, *children, *new_order;
	int selCnt, under, over;
	unsigned int num_children;

	/*
	 * Problem: the number of widgets in ww->composite.num_children may
	 * not be the same a num_children returned from XQueryTree.
	 * That makes this a garbage-in garbage-out situation.
	 */

	/*  Get current stacking order as bottom-first list  */
	(void)XQueryTree(XtDisplay(ww), XtWindow(ww), &root, &parent,
	                 &children, &num_children);
	new_order = (Window *)XtCalloc(num_children, sizeof(Window));
	under = num_children - 1;
	selCnt = 0;
	for( i=0; i<num_children; i++ ) 
	    if( ChildWindowIsSelected(ww, children[i]) ) selCnt++;

	over = selCnt - 1;
	for( i=0; i<num_children; i++ )
	{
	    if( ChildWindowIsSelected(ww, children[i]) )
	    {
	        new_order[over] = children[i];
	        over--;
	    }
	    else
	    {
	        new_order[under] = children[i];
	        under--;
	    }
	}
	/*  Stack according to the top-first list  */
	XRestackWindows(XtDisplay(ww), new_order, num_children);
	XtFree((char*)new_order);
	XFree((void *)children);
    }
}


/*  Subroutine: ChildWindowIsSelected
 *  Purpose:    Determine if passed window belongs to a child which is selected
 */
static Boolean ChildWindowIsSelected( XmWorkspaceWidget ww, Window child )
{
    int i;
    for( i=0; i<ww->composite.num_children; i++ )
    {
	if( XtWindow(ww->composite.children[i]) == child )
	{
	    XmWorkspaceConstraints constraints =
	      WORKSPACE_CONSTRAINT(ww->composite.children[i]);
	    return constraints->workspace.is_selected;
	}
    }
    return FALSE;
}


/*  Subroutine: SetGridBackground
 *  Purpose:    Install a background pixmap with grid marks
 */
static void SetGridBackground( XmWorkspaceWidget ww, Boolean change )
{
    Pixmap pixmap;
    XGCValues values;
    XtGCMask valueMask;
    GC gc;
    int width, height;
    Boolean draw_grid_marks;
    Screen* scrptr = ww->core.screen;
    Window rootw = RootWindowOfScreen(scrptr);

    /*  Check if drawing the grid marks is requested  */
    if(   ww->workspace.snap_to_grid
       && (   (ww->workspace.horizontal_draw_grid != XmDRAW_NONE)
	   || (ww->workspace.vertical_draw_grid != XmDRAW_NONE)) )
    {
	width = ww->workspace.grid_width;
	height = ww->workspace.grid_height;
	draw_grid_marks = True;
    }
    else
    {
#if DUPLICATE_XT_STUFF
#else
	XtVaSetValues ((Widget)ww, XmNbackgroundPixmap, XtUnspecifiedPixmap, NULL);
	return ;
#endif
	/*  When window is created, it has a solid background by default  */
	if( change == False )
	    return;
	/*  If pixmap is changed, we must manufacture a solid background  */
	width = 16;
	height = 16;
	draw_grid_marks = False;
    }

    if (!XtWindow(ww)) return ;

    pixmap = XCreatePixmap(XtDisplay(ww), rootw,
	width, height, ww->core.depth);
    values.graphics_exposures = False;
    values.foreground = ww->core.background_pixel;
    values.background = ww->core.background_pixel;
    values.function = GXcopy;
    values.line_style = LineOnOffDash;
    values.dashes = 1;
    values.plane_mask = AllPlanes;
    valueMask = GCForeground | GCBackground | GCGraphicsExposures
      | GCFunction | GCLineStyle | GCDashList | GCPlaneMask;
    gc = XtGetGC((Widget)ww, valueMask, &values);
    XFillRectangle(XtDisplay(ww), pixmap, gc, 0, 0, width, height);
    if( draw_grid_marks )
    {
  	XSetForeground(XtDisplay(ww), gc, ww->manager.foreground);
	DrawGridMarks(ww, gc, pixmap);
    }
#if DUPLICATE_XT_STUFF
    XSetWindowBackgroundPixmap(XtDisplay(ww), XtWindow(ww), pixmap);
#else
    XtVaSetValues ((Widget)ww, XmNbackgroundPixmap, pixmap, NULL);
#endif
    XtReleaseGC((Widget)ww, gc);
    XFreePixmap(XtDisplay(ww), pixmap);
}


/*  Subroutine: DrawGridMarks
 *  Purpose:    Draw the brid marks in the workspace background pixmap
 */
static void DrawGridMarks( XmWorkspaceWidget ww, GC gc, Pixmap pixmap )
{
    int x_h, y1_h, y2_h, x1_v, x2_v, y_v;

    if( ww->workspace.horizontal_alignment == XmALIGNMENT_END )
    {
	x_h = ww->workspace.grid_width - 1;
	x1_v = ww->workspace.grid_width - (HASH_RAY + HASH_RAY);
    }
    else if((ww->workspace.horizontal_alignment == XmALIGNMENT_CENTER) ||
            (ww->workspace.horizontal_alignment == XmALIGNMENT_NONE))
    {
	x_h = ww->workspace.grid_width / 2;
	x1_v = (ww->workspace.grid_width / 2) - HASH_RAY;
    }
    else
    {
	x_h = 0;
	x1_v = 0;
    }
    if(  ww->workspace.vertical_alignment == XmALIGNMENT_END )
    {
	y1_h = ww->workspace.grid_height - (HASH_RAY + HASH_RAY);
	y_v = ww->workspace.grid_height - 1;
    }
    else if((ww->workspace.vertical_alignment == XmALIGNMENT_CENTER) ||
            (ww->workspace.vertical_alignment == XmALIGNMENT_NONE))
    {
	y1_h = (ww->workspace.grid_height / 2) - HASH_RAY;
	y_v = ww->workspace.grid_height / 2;
    }
    else
    {
	y1_h = 0;
	y_v = 0;
    }
    /*  There is a dash problem in that some PS2 servers don't draw the on .. *
     *  part of a dash unless the off is also included!  So we stretch to ... *
     *  an even size.  Worse, some old PS2 servers don't do dashes: they ...  *
     *  will draw these lines too long.  Sigh!                                */
    if( ww->workspace.horizontal_draw_grid == XmDRAW_HASH )
	/*  I don't know why -1 but OnOffDash comes out short without it  */
	y2_h =  y1_h + HASH_RAY + HASH_RAY + 1;
    else
    {
	if( ww->workspace.horizontal_draw_grid == XmDRAW_OUTLINE )
	    x_h = 0;
	y1_h = 0;
	y2_h = ww->workspace.grid_height - 1;
    }
    if( ww->workspace.vertical_draw_grid == XmDRAW_HASH )
	x2_v =  x1_v + HASH_RAY + HASH_RAY + 1;
    else
    {
	if( ww->workspace.vertical_draw_grid == XmDRAW_OUTLINE )
	    y_v = 0;
	x1_v = 0;
	x2_v = ww->workspace.grid_width - 1;
    }
    /*  If things are too tight in our direction, don't draw anything  */
    if( ww->workspace.horizontal_draw_grid != XmDRAW_NONE )
	XDrawLine(XtDisplay(ww), pixmap, gc, x_h, y1_h, x_h, y2_h);
    if( ww->workspace.vertical_draw_grid != XmDRAW_NONE )
	XDrawLine(XtDisplay(ww), pixmap, gc, x1_v, y_v, x2_v, y_v);
}


/*  Subroutine: OutlineChild
 *  Purpose:    Set a rectangle the outline of a single child widget
 */
static void OutlineChild( XRectangle* surrogate,
	                  XmWorkspaceConstraints constraints )
{
    surrogate->x = constraints->workspace.x_left;
    surrogate->y = constraints->workspace.y_upper;
    /*  X draws box as x<->x+width instead of x<->x+width-1 as it should  */
    surrogate->width =
      constraints->workspace.x_right - constraints->workspace.x_left;
    surrogate->height =
      constraints->workspace.y_lower - constraints->workspace.y_upper;
}


/*  Subroutine: SaveOutline
 *  Purpose:    Make outline of grabbed widget and a box fit around all
 *              selected widgets (if there are more than the one).
 */
static void SaveOutline( XmWorkspaceWidget ww, Widget grab_child )
{
    XmWorkspaceConstraints constraints;

    constraints = WORKSPACE_CONSTRAINT(grab_child);
    OutlineChild(ww->workspace.rubberband, constraints);
    if( constraints && (ww->workspace.num_selected > 1) )
    {
	ww->workspace.rubberband[1].x = ww->workspace.min_x - 1;
	ww->workspace.rubberband[1].width =
	  3 + ww->workspace.max_x - ww->workspace.min_x;
	ww->workspace.rubberband[1].y = ww->workspace.min_y - 1;
	ww->workspace.rubberband[1].height =
	  3 + ww->workspace.max_y - ww->workspace.min_y;
	ww->workspace.num_surrogates = 2;
    }
    else
	ww->workspace.num_surrogates = 1;
    ww->workspace.surrogates = ww->workspace.rubberband;
}


/*  Subroutine: SaveOutlines
 *  Purpose:    Make array of outlines of each of the selected widgets
 */
static void SaveOutlines( XmWorkspaceWidget ww )
{
    XmWorkspaceConstraints constraints;
    Widget child;
    int i, j;

    if ((ww->workspace.num_surrogates)&&(ww->workspace.surrogates)) 
	DiscardSurrogate (ww);

    /*
     * Purify found an abw bug here.  It appears as though num_selected
     * is less than the number of constraint parts which say they
     * are selected.  I added the following loop and the MAX. Beware
     */
    j = 0;
    for (i=0; i<ww->composite.num_children; i++) {
	child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(child);
	if( constraints->workspace.is_selected == TRUE ) j++;
    }
#if 0
    if (j!=ww->workspace.num_selected) {
	char msg[512];
 	sprintf (msg,
"Selection cnt (%d) != number of children who think they are selected (%d). [%s:%d]", 
ww->workspace.num_selected, j, __FILE__, __LINE__);
	XtWarning(msg);
    }
#endif

    ww->workspace.surrogates =
      (XRectangle *)XtMalloc(MAX(j,ww->workspace.num_selected) * sizeof(XRectangle));


    for( j=0, i=0; i<ww->composite.num_children; i++ )
    {
	child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(child);
	if( constraints->workspace.is_selected == TRUE )
	{
	    OutlineChild(&(ww->workspace.surrogates[j]), constraints);
	    ++j;
	}
    }
    ww->workspace.num_surrogates = j;
}


/*  Subroutine: DiscardSurrogate
 *  Purpose:    Free up space occupied by array of surrogate outlines
 */
static void DiscardSurrogate( XmWorkspaceWidget ww )
{
    if( ww->workspace.surrogates != ww->workspace.rubberband )
	XtFree((char*)ww->workspace.surrogates);
    ww->workspace.surrogates = NULL;
    ww->workspace.num_surrogates = 0;
}


/*  Subroutine: CreateSurrogate
 *  Purpose:    Create drawable outlines of some type to aid in interactive
 *              placement of selected children.
 */
#define IFMIN(a,b) if((b)<a)a=(b)
#define IFMAX(a,b) if((b)>a)a=(b)
static void CreateSurrogate( XmWorkspaceWidget ww, Widget grab_child )
{
    Widget child;
    XmWorkspaceConstraints constraints;
    int i, init;

    if( ww->workspace.bandGC == NULL )
	GetRubberbandGC(ww);
    /*  Determine the limits (used to prevent moving anything off the edge)  */
    init = 1;
    for( i=0; i<ww->composite.num_children; i++ )
    {
	child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(child);
	if( constraints->workspace.is_selected == TRUE )
	{
	    constraints->workspace.x_left = child->core.x;
	    constraints->workspace.y_upper = child->core.y;
	    constraints->workspace.x_right = child->core.x + child->core.width - 1;
	    constraints->workspace.y_lower = child->core.y + child->core.height - 1;
	    if( init )
	    {
	        ww->workspace.min_x = constraints->workspace.x_left;
	        ww->workspace.max_x = constraints->workspace.x_right;
	        ww->workspace.min_y = constraints->workspace.y_upper;
	        ww->workspace.max_y = constraints->workspace.y_lower;
	        init = 0;
	    }
	    else
	    {
	        IFMIN(ww->workspace.min_x,constraints->workspace.x_left);
	        IFMAX(ww->workspace.max_x,constraints->workspace.x_right);
	        IFMIN(ww->workspace.min_y,constraints->workspace.y_upper);
	        IFMAX(ww->workspace.max_y,constraints->workspace.y_lower);
	    }
	}
    }
    /*  Draw the kind of visual feedback we will use  */
    switch( ww->workspace.surrogate_type )
    {
      case XmOUTLINE_EACH:
	SaveOutlines(ww);
	break;
      case XmOUTLINE_PLUS:
	SaveOutline(ww, grab_child);
	break;
      case XmOUTLINE_ALL:
      default:
	SaveOutline(ww, NULL);
	break;
    }
    /*  Reset limits to reflect limits of movement  */
    ww->workspace.min_x = -ww->workspace.min_x;
    ww->workspace.max_x = ww->core.width - ww->workspace.max_x;
    ww->workspace.min_y = -ww->workspace.min_y;
    ww->workspace.max_y = ww->core.height - ww->workspace.max_y;
}
#undef IFMIN
#undef IFMAX


/*  Subroutine: DrawSurrogate
 *  Purpose:    Put the surrogate outline(s) on the screen and note their state
 */
static void DrawSurrogate( XmWorkspaceWidget ww )
{
    if (ww->workspace.overlaps)
	XDrawRectangles(XtDisplay(ww), XtWindow(ww), ww->workspace.badBandGC,
	    ww->workspace.surrogates, ww->workspace.num_surrogates);
    else
	XDrawRectangles(XtDisplay(ww), XtWindow(ww), ww->workspace.bandGC,
	    ww->workspace.surrogates, ww->workspace.num_surrogates);
    ww->workspace.surrogate_is_visible = !ww->workspace.surrogate_is_visible;
}


/*  Subroutine: MoveSurrogate
 *  Purpose:    Move the position of the surrogates (to track pointer)
 */
static void MoveSurrogate( XmWorkspaceWidget ww, int delta_x, int delta_y )
{
    int i;
    if( ww->workspace.surrogate_is_visible )
	DrawSurrogate(ww);

    Overlapping (ww, delta_x, delta_y);
    for( i=0; i<ww->workspace.num_surrogates; i++ )
    {
	ww->workspace.surrogates[i].x += delta_x;
	ww->workspace.surrogates[i].y += delta_y;
    }
    ww->workspace.base_x += delta_x;
    ww->workspace.base_y += delta_y;
    DrawSurrogate(ww);
}

static Boolean Overlapping( XmWorkspaceWidget ww, int delta_x, int delta_y )
{
    int i;
    Boolean overlaps = False;
    Region r; 
    XPoint p[4];

    if (ww->workspace.check_overlap == False) return False;

   /*
    * If the position of a surrogate overlaps the position of any
    * tool, then don't move any surrogates.
    */
    p[0].x = ww->workspace.surrogates[0].x + delta_x;
    p[0].y = ww->workspace.surrogates[0].y + delta_y;
    if (ww->workspace.snap_to_grid) {
	int x = p[0].x;
	int y = p[0].y;
	Align1D(&x, ww->workspace.surrogates[0].width + 1, ww->workspace.grid_width,
		ww->workspace.horizontal_alignment, False);
	Align1D(&y, ww->workspace.surrogates[0].height + 1, ww->workspace.grid_height,
		ww->workspace.vertical_alignment, False);
	p[0].x = x;
	p[0].y = y;
    }
    p[2].x = p[0].x + ww->workspace.surrogates[0].width + 1;
    p[2].y = p[0].y + ww->workspace.surrogates[0].height + 1;


    p[1].x = p[2].x;
    p[1].y = p[0].y;
    p[3].x = p[0].x;
    p[3].y = p[2].y;
    r = XPolygonRegion (p, 4, WindingRule);

    for( i=1; i<ww->workspace.num_surrogates; i++ )
    {
	XRectangle rect;

	/*
	 * Use gridded postions
	 */
	rect.width = ww->workspace.surrogates[i].width + 1;
	rect.height = ww->workspace.surrogates[i].height + 1;
	if (ww->workspace.snap_to_grid) {
	    int x = ww->workspace.surrogates[i].x + delta_x;
	    int y = ww->workspace.surrogates[i].y + delta_y;

	    Align1D(&x, rect.width, ww->workspace.grid_width,
		    ww->workspace.horizontal_alignment, False);
	    rect.x = (int)x;

	    Align1D(&y, rect.height, ww->workspace.grid_height,
		    ww->workspace.vertical_alignment, False);
	    rect.y = (int)y;
	} else {
	    rect.x = ww->workspace.surrogates[i].x + delta_x;
	    rect.y = ww->workspace.surrogates[i].y + delta_y;
	}
	
	XUnionRectWithRegion(&rect, r, r);
    }
    for (i=0; i<ww->composite.num_children; i++)
    {
	Widget child = ww->composite.children[i];
	XmWorkspaceConstraints constraints = WORKSPACE_CONSTRAINT(child);
	int x = child->core.x;
	int y = child->core.y;
	int width = child->core.width;
	int height = child->core.height;
	if (!child->core.managed) continue;
	if (!constraints->workspace.is_managed) continue;
	if (!child->core.mapped_when_managed) continue;
        if( constraints->workspace.is_selected == TRUE ) continue;
	if (XRectInRegion (r, x,y,width,height) != RectangleOut) {
	    overlaps = True;
	    break;
	}
    }
    XDestroyRegion(r);
    ww->workspace.overlaps = overlaps;
    return overlaps;
}

/*  Subroutine: ResizeSurrogate
 *  Purpose:    Resize the surrogates (to track pointer)
 */
static void ResizeSurrogate( XmWorkspaceWidget ww, int width, int height )
{
    int i;
    int j;
    Widget child;
    XmWorkspaceConstraints constraints;

    if( ww->workspace.surrogate_is_visible )
	DrawSurrogate(ww);

    for( j=0, i=0; i<ww->composite.num_children; i++ )
    {
        child = ww->composite.children[i];
        constraints = WORKSPACE_CONSTRAINT(child);
        if( constraints->workspace.is_selected == TRUE )
        {
	    if(constraints->workspace.is_v_resizable)
		ww->workspace.surrogates[j].height = height;
	    if(constraints->workspace.is_h_resizable)
		ww->workspace.surrogates[j].width  = width;
            ++j;
        }
    }
    DrawSurrogate(ww);
}


/*  Subroutine: PerformSpaceWars
 *  Purpose:    Prevent move from causing overlap by shifting children in
 *              conflict to the right.
 *  Note:       Rules for "placement_policy" code:
 *              0: widgets are adjusted separately, initial order is maintained
 *              >0: selected children move en-mass (keeping spacing among them)
 *               1: First selected child on left maintains ordered position
 *               2: Selected widgets stay put, all others move right as needed
 */
static Boolean PerformSpaceWars( XmWorkspaceWidget ww, Boolean change_managed, XEvent* event)
{
    struct SortRec* sortlist;
    struct SortRec* current;
    struct SortRec* next;
    struct SortRec* right_sort;
    struct SortRec* sortmem;
    Boolean moved = False, first_selected = TRUE;

    if (ww->workspace.auto_arrange) return False;

    sortlist = GetSortList(ww, change_managed, &sortmem); 
    if (sortlist != NULL)
    {
	/*  Call potentially recursive (but not) space resolution algorithm  */
	moved = ResolveOverlaps(ww, sortlist, &first_selected, event);
	/*  Perform insertion sort of linked list, by x_right  */
	for( current = sortlist->next, sortlist->next = NULL;
	     current != NULL;
	     current = next )
	{
	    next = current->next;
	    right_sort = sortlist;
	    while(   (right_sort->next != NULL)
		  && (right_sort->next->x_right < current->x_right) )
		right_sort = right_sort->next;
	    current->next = right_sort->next;
	    right_sort->next = current;
	}
	/*  Now move the windows in right to left order to avoid expose races */
	(void)MoveIfDisplaced(ww, sortlist, 0, 0);
    }
    if (sortmem)
	XtFree((char*)sortmem);
    return moved;
}


/*  Subroutine: ResolveOverlaps
 *  Purpose:    Move widgets to clear any overlaps caused by the movement of
 *              widgets (children marked as selected or displaced)
 *  Method:     The basic idea is to go through the list in outer & inner loops,
 *              applying the rules at each overlap.  Either outer or inner
 *              progress through the list can be reset back as needed to repeat
 *              overlap checks on sublists of widgets.
 */
static Boolean ResolveOverlaps( XmWorkspaceWidget ww, struct SortRec* sortlist,
	                     Boolean* first_selected, XEvent* event )
{
    struct SortRec* current;
    struct SortRec* current_next;
    struct SortRec* other;
    struct SortRec* other_next;
    short delta_x = 0;
    Boolean moved = False;

    current = sortlist;
    while( current != NULL )
    {
	/*  Set up next for normal progress (may be overwridden to back up)  */
	current_next = current->next;

	if( (current->is_selected || current->is_displaced ) &&
	    ( !current->child->core.being_destroyed ) )
	{
	    /*  Look for overlap with widgets before this in the sort list  */
	    other = sortlist;
	    while((other != current) && (!other->child->core.being_destroyed))
	    {
	        other_next = other->next;
	        if(   (other->y_upper <= current->y_lower)
	           && (other->y_lower >= current->y_upper)
	           && (other->x_left <= current->x_right)
	           && (other->x_right >= current->x_left) )
	        {
		    int policy = ww->workspace.placement_policy;
	            if(   (   (policy == XmSPACE_WARS_SELECTED_STAYS)
	                   && (other->is_selected == FALSE)
	                   && (   current->is_selected
	                       || (other->x_index > current->x_index)))
	               || (   (policy == XmSPACE_WARS_LEFT_MOST_STAYS)
	                   && (*first_selected == FALSE)
	                   && current->is_selected) )
	            {
	                /*  Push overlapping widgets farther to right  */
	                (void)MoveAToRightOfB(ww, other, current, event);
	                /*  Make resolution repeat from displaced widget  */
	                if( current_next == current->next )
	                    current_next = other;
	            }
	            else
	            {
	                /*  Slide to right of overlapping widgets on left  */
	                delta_x += MoveAToRightOfB(ww, current, other, event);
	                /*  Check list of widgets again for more conflict  */
	                other_next = sortlist;
	            }
		    moved = True;
	        }
	        other = other_next;
	    }
	    /*  If we moved the first_selected, adjust the other selected  */
	    if(   (ww->workspace.placement_policy == XmSPACE_WARS_LEFT_MOST_STAYS)
	       && *first_selected
	       && current->is_selected
	       && delta_x > 0 )
	    {
	        /*  Move all other selected widgets by the same amount  */
	        RepositionSelectedChildren(current, delta_x);
	        /*  Indicate that selected widgets are now fixed  */
	        *first_selected = FALSE;
	    }
	    /*  Look for overlap with widgets after this in the sort list  */
	    other = current->next;
	    while(other != NULL)
	    {
		if (other->child->core.being_destroyed)
		{
		    other = other->next;
		    continue;
		}
	        if(   (other->y_upper <= current->y_lower)
	           && (other->y_lower >= current->y_upper)
	           && (other->x_left <= current->x_right)
	           && (other->x_right >= current->x_left) )
	        {
	            /*  For policies 1 and 2, selected widgets stay put     */
	            if(  (ww->workspace.placement_policy != XmSPACE_WARS_SELECTED_MIGRATE)
	               && other->is_selected
	               && (!current->is_selected) )
	            {
	                /*  Slide to right of overlapping widgets on right   */
	                (void)MoveAToRightOfB(ww, current, other, event);
	                /*  Make resolution repeat with this widget  */
	                if( current_next == current->next )
	                    current_next = current;
	            }
	            /*  For policy 0, the order is maintained (move >sort)   */
	            else
	            {
	                /*  Push overlapping widgets on right farther right  */
	                (void)MoveAToRightOfB(ww, other, current, event);
	            }
		    moved = True;
	        }
	        other = other->next;
	    }
	}
	current = current_next;
    }
    return moved;
}


/*  Subroutine: MoveAToRightOfB
 *  Purpose:    Move the x position of movee to right of mover.
 */
static int MoveAToRightOfB( XmWorkspaceWidget ww,
	                    struct SortRec* movee, struct SortRec *mover, XEvent* event )
{
    int delta_x;
    delta_x = 1 + mover->x_right - movee->x_left;
    if (ww->workspace.snap_to_grid)
	{
	delta_x += ww->workspace.grid_width;
	}
    else
	{
	delta_x += ww->workspace.collision_spacing;
	}

    movee->x_left += delta_x;
    movee->x_right += delta_x;
    movee->constraints->workspace.x_delta =
      movee->x_left - movee->constraints->workspace.x_left;
    if( ww->workspace.snap_to_grid )
    {
	short delta;
	/*  Move into closest grid position (left&down)  */
	SnapToGrid(ww, movee->child, movee->constraints, TRUE);
	if ( ((movee->constraints->workspace.x_delta != 0) ||
	      (movee->constraints->workspace.y_delta != 0)) &&
	      !ww->workspace.suppress_callbacks)
	    {
	    XmWorkspacePositionChangeCallbackStruct call_data;

	    call_data.child = movee->child;
	    call_data.x = movee->constraints->workspace.x_left + 
		    movee->constraints->workspace.x_delta;
	    call_data.y = movee->constraints->workspace.y_upper + 
		    movee->constraints->workspace.y_delta;
	    call_data.event = event;
	    XtCallCallbacks((Widget)
		    ww, XmNpositionChangeCallback, &call_data);
	    }
	/*  Update link-list record to new coords  */
	delta = movee->constraints->workspace.x_delta;
	movee->x_left = movee->constraints->workspace.x_left + delta;
	movee->x_right = movee->constraints->workspace.x_right + delta;
	delta = movee->constraints->workspace.y_delta;
	movee->y_upper = movee->constraints->workspace.y_upper + delta;
	movee->y_lower = movee->constraints->workspace.y_lower + delta;
    } else 
    {
	XmWorkspacePositionChangeCallbackStruct call_data;

	call_data.child = movee->child;
	call_data.x = movee->constraints->workspace.x_left + 
		movee->constraints->workspace.x_delta;
	call_data.y = movee->constraints->workspace.y_upper + 
		movee->constraints->workspace.y_delta;
	call_data.event = event;
	XtCallCallbacks((Widget)
		ww, XmNpositionChangeCallback, &call_data);
    }
    movee->is_displaced = TRUE;
    return delta_x;
}


/*  Subroutine: RepositionSelectedChildren
 *  Purpose:    Move the selected children to follow the leader, then
 *              change sort order so everybody isn't necessarily forced
 *              to the right of them
 */
static void RepositionSelectedChildren( struct SortRec* current, short delta_x )
{
    struct SortRec* other;

    /*  Look for the other selected ones  */
    other = current->next;
    while( other != NULL )
    {
	if( other->is_selected )
	{
	    /*  Reposition each other selected one  */
	    other->x_left += delta_x;
	    other->x_right += delta_x;
	    other->is_displaced = TRUE;
	}
	/*  Keep track of connection point  */
	current = other;
	other = other->next;
    }
}


/*  Subroutine: MoveIfDisplaced
 *  Purpose:    Tail recursive call to move chidren being moved rightmost first.
 */
static Boolean MoveIfDisplaced( XmWorkspaceWidget ww, struct SortRec *sortlist,
	                        int max_x, int max_y )
{
    if( sortlist->is_selected || sortlist->is_displaced )
    {
	if( sortlist->x_right > max_x )
	    max_x = sortlist->x_right;
	if( sortlist->y_lower > max_y )
	    max_y = sortlist->y_lower;
    }
    if( sortlist->next != NULL )
    {
	/*  Not the end of the sortlist, recurse  */
	if( MoveIfDisplaced(ww, sortlist->next, max_x, max_y) == FALSE )
	{
	    ResetChild(ww, sortlist->constraints);
	    return FALSE;
	}
    }
    else if( (max_x >= ww->core.width) || (max_y >= ww->core.height) )
    {
	/*  End of the sortlist, check necessity of resize  */
	XtWidgetGeometry request, reply;
	XtGeometryResult result;

	request.width = MAX(max_x + 1, ww->core.width);
	request.height = MAX(max_y + 1, ww->core.height);
	request.request_mode = CWWidth | CWHeight;
	result = XtMakeGeometryRequest((Widget)ww, &request, &reply);
	ReallocCollideLists(ww);
	if( (result == XtGeometryNo) || (result == XtGeometryAlmost) )
	{
	    ResetChild(ww, sortlist->constraints);
	    return FALSE;
	}
    }
    if( sortlist->is_selected || sortlist->is_displaced )
	MoveChild(ww, sortlist->child, sortlist->constraints);
    return TRUE;
}


/*  Subroutine: GetSortList
 *  Purpose:    Create a list of chidren sorted by x position and priority
 *  Note:       Priority is x, then non-selected, then y where x and y can
 *              be left, center, or right based on policy variable.
 *  Note:       List is used to arbitrate who moves in cases of overlap
 */
static struct SortRec *GetSortList( XmWorkspaceWidget ww,
	                            Boolean change_managed,
	                            struct SortRec** sortmem )
#ifdef EXPLAIN
     XmWorkspaceWidget ww;      /* The widget                               */
     Boolean change_managed;    /* Need to mark children just managed       */
     struct SortRec** sortmem;  /* Xtalloc pointer to be XtFree'd when done */
#endif
{
    struct SortRec* sortlist;
    struct SortRec* heap;
#ifdef DEBUG
    /*  Stupid debugger can't access variables not defined at top  */
    struct SortRec* current;
    struct SortRec* place_search;
    XmWorkspaceConstraints nc;
#endif
    int i;
    int count;

    count = ww->composite.num_children;
    if (count > 0) 
	heap = (struct SortRec *) XtCalloc(count, sizeof(struct SortRec));
    else
	heap = NULL;

    *sortmem = heap;
    sortlist = NULL;
    for( i = 0; i < count; i++ )
    {
#ifndef DEBUG
	/*  Debugger can't access variables not defined at top!  */
	struct SortRec* current;
	XmWorkspaceConstraints nc;
#endif
	current = &(heap[i]);
	current->child = ww->composite.children[i];
	nc = WORKSPACE_CONSTRAINT(current->child);
	current->constraints = nc;
	/*  Accomodate special case of children just placed by application  */
	if( change_managed && nc->workspace.is_managed && nc->workspace.is_newly_managed )
	{
	    current->is_displaced = TRUE;
	    nc->workspace.is_newly_managed = FALSE;
	}
	else
	    current->is_displaced = FALSE;
	/*  Only count those children that are managed (mapped to screen)  */
	if( nc->workspace.is_managed )
	{
	    /*  Proposed position is offset by delta's  */
	    current->x_left = nc->workspace.x_left + nc->workspace.x_delta;
	    current->x_right = nc->workspace.x_right + nc->workspace.x_delta;
	    current->y_upper = nc->workspace.y_upper + nc->workspace.y_delta;
	    current->y_lower = nc->workspace.y_lower + nc->workspace.y_delta;
	    current->is_selected = nc->workspace.is_selected;
	    /*  Index is x position only and has been offset as appropriate  */
	    /*  Make an index with x then selected then y absolute priority  */
	    current->x_index = nc->workspace.x_index * 16384;
	    if( current->is_selected == (POLICY(ww) != XmSPACE_WARS_SELECTED_STAYS) )
	        /*  Selected children come first (policy > 1) or last (< 2)  */
	        current->x_index += 8192;
	    if( ww->workspace.sort_policy == XmALIGNMENT_BEGINNING )
	        current->x_index += current->y_upper;
	    else if( ww->workspace.sort_policy == XmALIGNMENT_CENTER )
	        current->x_index +=
	            (nc->workspace.y_center + nc->workspace.y_delta);
	    else
	        current->x_index += current->y_lower;
	    if( (sortlist == NULL) || (sortlist->x_index > current->x_index) )
	    {
	        current->next = sortlist;
	        sortlist = current;
	    }
	    else
	    {
	        /*  Perform insertion sort of linked list  */
#ifndef DEBUG
	        struct SortRec* place_search;
#endif
	        place_search = sortlist;
	        while(   (place_search->next != NULL)
	              && (place_search->next->x_index < current->x_index) )
	            place_search = place_search->next;
	        current->next = place_search->next;
	        place_search->next = current;
	    }
	}
    }
    return sortlist;
}

/*  Subroutine: XmCreateWorkspaceLine
 *  Purpose:    Export routine to register an application line for management
 */
XmWorkspaceLine
  XmCreateWorkspaceLine( XmWorkspaceWidget ww, int color,
	                 Widget source, short src_x_offset, short src_y_offset,
	                 Widget dest, short dst_x_offset, short dst_y_offset )
{
    XmWorkspaceConstraints constraints;
    int i;
    XmWorkspaceLine line;
    Boolean source_not_found, destination_not_found;

    /*  Check to make sure this one is here  */
    source_not_found = TRUE;
    destination_not_found = TRUE;
    for( i=0; i<ww->composite.num_children; i++ )
    {
	if( ww->composite.children[i] == source )
	    source_not_found = FALSE;
	if( ww->composite.children[i] == dest )
	    destination_not_found = FALSE;
    }
    if( source_not_found || destination_not_found )
    {
	XtWarning("Attempt to create line for non-child widget.");
	return NULL;
    }

    line = (XmWorkspaceLine)XtMalloc(sizeof(XmWorkspaceLineRec));
    memset(line, 0, sizeof(XmWorkspaceLineRec));

    /*  Set the source end and install it in the source's list  */
    line->source = source;
    line->src_x = source->core.x + src_x_offset;
    line->src_y = source->core.y + src_y_offset;
    line->src_x_offset = src_x_offset;
    constraints = WORKSPACE_CONSTRAINT(source);
    line->src_next = constraints->workspace.source_lines;
    constraints->workspace.source_lines = line;
    /*  Set the destination end and install it in the destination's list  */
    line->destination = dest;
    line->dst_x = dest->core.x + dst_x_offset;
    line->dst_y = dest->core.y + dst_y_offset;
    line->dst_x_offset = dst_x_offset;
    constraints = WORKSPACE_CONSTRAINT(dest);
    line->dst_next = constraints->workspace.destination_lines;
    constraints->workspace.destination_lines = line;
    /*  Set the color or the default color  */
    if( color == -1 )
	line->color = ww->manager.foreground;
    else
	line->color = color;
    /*  Install it in the master line list  */
    line->next = ww->workspace.lines;
    ww->workspace.lines = line;
    ww->workspace.num_lines++;
    line->is_to_be_moved = TRUE;

    /*  Get the GC and set it to this color  */
    if( XtIsRealized((Widget)ww) )
    {
	if( xmWorkspaceClassRec.workspace_class.lineGC == NULL )
	    InitLineGC(ww, line->color);
	else
	    XSetForeground(XtDisplay(ww),
	                   xmWorkspaceClassRec.workspace_class.lineGC,
	                   line->color);
	line->is_to_be_drawn = FALSE;
    }
    else
	line->is_to_be_drawn = TRUE;

    if (ww->workspace.line_drawing_enabled)
	{
	RerouteLines(ww, FALSE);
	/* Manually clear out the exposure area */
	if (XtWindow(ww))
	    XClearArea(XtDisplay(ww), XtWindow(ww), ww->workspace.expose_left,
	        ww->workspace.expose_upper,
	        ww->workspace.expose_right - ww->workspace.expose_left,
	        ww->workspace.expose_lower - ww->workspace.expose_upper, False);
        RefreshLines(ww);
	}
    UnsetExposureArea(ww);
    line->is_to_be_collapsed = TRUE;
    return line;
}

/*  Subroutine: SetLineRoute
 *  Purpose:    Set the points for drawing the line
 */
static void SetLineRoute( XmWorkspaceWidget ww, XmWorkspaceLine new )
{
short i;
LineElement *line_list;
int cost, failnum;

    /* Point 1 */
    new->point[0].x = new->src_x;
    new->point[0].y = new->src_y;

    if( !ww->workspace.manhattan_route )
	{
	/* Point 2 */
	new->point[1].x = new->dst_x;
	new->point[1].y = new->dst_y;

	new->num_points = 2;
	}

    /*  If doing Manhattan routing...  */
    else
    {
        {
        line_list = Manhattan(ww, new->src_x, new->src_y, new->dst_x,
                                new->dst_y, 0, &cost, new->source,
				new->destination, new, &failnum);
	if (cost < 999)
	    {
            CopyPointsToLine(ww, line_list, new);
	    AddLineToCollideList(ww, new);
	    }
	else
	    {
	    XmWorkspaceErrorCallbackStruct call_value;

	    new->num_points = -1;

	    call_value.reason = XmCR_ERROR;
	    call_value.failnum = failnum;
	    call_value.source = new->source;
	    call_value.destination = new->destination;
	    call_value.srcx = new->src_x;
	    call_value.srcy = new->src_y;
	    call_value.dstx = new->dst_x;
	    call_value.dsty = new->dst_y;
	    XtCallCallbacks((Widget)ww, XmNerrorCallback, &call_value);
	    }
        }
        if (line_list)
            FreeLineElementList(line_list);
    }
    /*  Define the bounding box of this line  */
    new->x_left = new->x_right = new->point[0].x;
    new->y_upper = new->y_lower = new->point[0].y;
    for( i=1; i<new->num_points; i++ )
    {
	if( new->point[i].x < new->x_left )
	    new->x_left = new->point[i].x;
	else if( new->point[i].x > new->x_right )
	    new->x_right = new->point[i].x;
	if( new->point[i].y < new->y_upper )
	    new->y_upper = new->point[i].y;
	else if( new->point[i].y > new->y_lower )
	    new->y_lower = new->point[i].y;
    }
    AugmentExposureAreaForLine(ww, new);
}

/*  Subroutine: RerouteLines
 *  Purpose:    Find suitable routes for all lines waiting to be moved
 */
void RerouteLines( XmWorkspaceWidget ww, Boolean reroute_all )
{
XmWorkspaceLine line;
Widget child;
int i;

    if (!ww->workspace.line_drawing_enabled)
	return;

    for( i=ww->composite.num_children-1; i>=0; i-- )
        {
        child = ww->composite.children[i];
	if (child->core.being_destroyed)
	    {
	    XmWorkspaceConstraints nc;
	    nc = WORKSPACE_CONSTRAINT(child);
	    if (!nc->workspace.line_invisibility)
		DeleteWidgetFromCollideList(child);
	    }
	}

    if(ww->workspace.lines)
    {
    	MarkCommonLines(ww);
	for (line = ww->workspace.lines; line; line = line->next)
	{
	    if( line->is_to_be_moved || reroute_all )
	    {
            	RemoveLineFromCollideList(ww, line);
	    }
	}
	for (line = ww->workspace.lines; line; line = line->next)
	{
	    if( line->is_to_be_moved || reroute_all )
	    {
	        /*  Adjust exposure area to include all of old line's route  */
	        SetLineRoute(ww, line);
	        line->is_to_be_drawn = TRUE;
	        line->is_to_be_moved = FALSE;
	    }
	}
    }
}

/*  Subroutine: XmDestroyWorkspaceLine
 *  Purpose:    Exported routine to remove an application line
 */
void XmDestroyWorkspaceLine( XmWorkspaceWidget ww, XmWorkspaceLine line,
	                     Boolean middle_of_group )
{
    if( line != NULL )
    {
	/* Before this line is removed, mark all lines that share its source
	   or destination to be re-routed, since they may have been following
	   this lines path */	
	line->is_to_be_moved = TRUE;
	MarkCommonLines(ww); 
	AugmentExposureAreaForLine(ww, line);
	if( DestroyLine(ww, line) )
	{
            RemoveLineFromCollideList(ww, line);
            XSetForeground(XtDisplay(ww),
	               xmWorkspaceClassRec.workspace_class.lineGC,
	               ww->core.background_pixel);
	if(ww->workspace.line_drawing_enabled)
	{
	    RerouteLines(ww, FALSE);
	    /* Manually clear out the exposure area */
	    if (XtWindow(ww))
		XClearArea(XtDisplay(ww), XtWindow(ww), ww->workspace.expose_left,
		    ww->workspace.expose_upper,
		    ww->workspace.expose_right - ww->workspace.expose_left,
		    ww->workspace.expose_lower - ww->workspace.expose_upper, True);
	    if (!middle_of_group) RefreshLines(ww);
	    UnsetExposureArea(ww);
	}
	XtFree((char*)line);
	}
    }
    else
    {
	if( ww->workspace.expose_left < ww->workspace.expose_right )
	    middle_of_group = FALSE;
	else
	    middle_of_group = TRUE;
    }
    if( !middle_of_group && ww->workspace.line_drawing_enabled )
    {
	RerouteLines(ww, FALSE);
	RefreshLines(ww);
	UnsetExposureArea(ww);
    }
}

/*  Subroutine: DestroyLine
 *  Purpose:    Remove a single line
 *  Returns:    False if line was not in list
 */
static Boolean DestroyLine( XmWorkspaceWidget ww, XmWorkspaceLine old )
{
    XmWorkspaceLine line;
    XmWorkspaceConstraints constraints;

    /*  Remove link from main list  */
    if( ww->workspace.lines == old )
	ww->workspace.lines = old->next;
    else
    {
	for( line = ww->workspace.lines;
	     (line->next && (line->next != old));
	     line = line->next );
	if( line->next == NULL )
	{
	    XtWarning("Attempt to destroy line not known to workspace.");
	    return False;
	}
	line->next = old->next;
    }
    ww->workspace.num_lines--;
    /*  Remove line from source list  */
    constraints = WORKSPACE_CONSTRAINT(old->source);
    if( constraints->workspace.source_lines == old )
	constraints->workspace.source_lines = old->src_next;
    else
    {
	for( line = constraints->workspace.source_lines;
	     (line->src_next && (line->src_next != old));
	     line = line->src_next );
	if( line->src_next )
	    line->src_next = old->src_next;
    }
    /*  Remove line from destination list  */
    constraints = WORKSPACE_CONSTRAINT(old->destination);
    if( constraints->workspace.destination_lines == old )
	constraints->workspace.destination_lines = old->dst_next;
    else
    {
	for( line = constraints->workspace.destination_lines;
	     (line->dst_next && (line->dst_next != old));
	     line = line->dst_next );
	if( line->dst_next )
	    line->dst_next = old->dst_next;
    }
    return TRUE;
}


#define IF_MORE(a,b) if((a)>(b))(b)=(a)
#define IF_LESS(a,b) if((a)<(b))(b)=(a)
/*  Subroutine: AugmentExposeAreaForLine
 *  Purpose:    Extend expose area to include box of this line
 */
void AugmentExposureAreaForLine( XmWorkspaceWidget ww,
	                                XmWorkspaceLine line )
{
    short line_excess = ww->workspace.line_thickness;
    IF_LESS(line->x_left - line_excess, ww->workspace.expose_left);
    IF_MORE(line->x_right + line_excess, ww->workspace.expose_right);
    IF_LESS(line->y_upper - line_excess, ww->workspace.expose_upper);
    IF_MORE(line->y_lower + line_excess, ww->workspace.expose_lower);
}
#undef IF_MORE
#undef IF_LESS


/*  Subroutine: UnsetExposureArea
 *  Purpose:    Set exposure area to show nothing and be ready for
 *              first augmentation
 */
static void UnsetExposureArea( XmWorkspaceWidget ww )
{
    ww->workspace.expose_left = ww->core.width;
    ww->workspace.expose_right = -1;
    ww->workspace.expose_upper = ww->core.height;
    ww->workspace.expose_lower = -1;
}


/*  Subroutine: RefreshLines
 *  Purpose:    Redraw all lines marked for movement, or within identified
 *              expose bounds.
 */
void RefreshLines( XmWorkspaceWidget ww )
{
XmWorkspaceLine line, tmp_line;
int             color = -1;
XRectangle      rect;

    if(!ww->workspace.line_drawing_enabled)
	return;
    if (XtIsRealized((Widget)ww) == False)
	return ;

    /*
     * Redraw all lines that are in the cleared region, but clip the drawing
     * to the cleared region.  This eliminate undesired side effects for
     * "copied" lines with haloing.  The alternative is to redraw all lines
     * every time (too slow).
     */
    if (ww->workspace.lines)
    {
	if ( (ww->workspace.expose_right > ww->workspace.expose_left) &&
	     (ww->workspace.expose_upper < ww->workspace.expose_lower) )
	{
	    rect.x      = ww->workspace.expose_left;
	    rect.y      = ww->workspace.expose_upper;
	    rect.width  = ww->workspace.expose_right - 
			  ww->workspace.expose_left + 
			  ww->workspace.line_thickness;
	    rect.height = ww->workspace.expose_lower - 
			  ww->workspace.expose_upper + 
			  ww->workspace.line_thickness;
	    XSetClipRectangles( XtDisplay(ww),
			        xmWorkspaceClassRec.workspace_class.lineGC,
			        0, 0,
			        &rect, 1,
			        YXBanded);
	    XSetClipRectangles( XtDisplay(ww),
			        xmWorkspaceClassRec.workspace_class.lineGC2,
			        0, 0,
			        &rect, 1,
			        YXBanded);
	}
	else
	{
	    XSetClipMask( XtDisplay(ww), 
			  xmWorkspaceClassRec.workspace_class.lineGC,
			  None);
	    XSetClipMask( XtDisplay(ww), 
			  xmWorkspaceClassRec.workspace_class.lineGC2,
			  None);
	}
    }
    if( (line = ww->workspace.lines) )
    {
	while( line )
	{
	    /*  If lines' region was cleared, redo it  */
	    if( (line->x_right >= ww->workspace.expose_left)  && 
		(line->x_left <= ww->workspace.expose_right)  && 
		(line->y_lower >= ww->workspace.expose_upper) && 
		(line->y_upper <= ww->workspace.expose_lower) )
	    {
		line->is_to_be_drawn = True;
	    }
	    line->two_phase_draw = False;
	    line = line->next;
	}
    }

    if( (line = ww->workspace.lines) )
    {
	while( line )
	{
	    if( line->is_to_be_drawn )
		/*
		 * Look through the rest of the list and see if we have a
		 * "sibling" line (same source x,y).  If we do, mark it for
		 * 2 phase drawing and defer.
		 */
	    {
		tmp_line = line->next;
		while( tmp_line )
		{
		    if ( (line->point[0].x == tmp_line->point[0].x) &&
		         (line->point[0].y == tmp_line->point[0].y) )
		    {
			line->two_phase_draw = True;
			tmp_line->two_phase_draw = True;
			break;
		    }
		    tmp_line = tmp_line->next;
		}
		if( XtIsRealized((Widget)ww) )
		{
		    color = line->color;
		    /*  Get the GC and set it to this color  */
		    if( xmWorkspaceClassRec.workspace_class.lineGC == NULL )
			InitLineGC(ww, color);
		    else
			XSetForeground
			  (XtDisplay(ww),
			   xmWorkspaceClassRec.workspace_class.lineGC,
			   color);
		}
		if( ( XtIsRealized((Widget)ww) ) && (!line->two_phase_draw) )
		{
		    if (line->num_points > 0)
		    {
			XDrawLines(XtDisplay(ww), XtWindow(ww),
			       xmWorkspaceClassRec.workspace_class.lineGC2,
			       line->point, line->num_points,
			       CoordModeOrigin);

			XDrawLines(XtDisplay(ww), XtWindow(ww),
			       xmWorkspaceClassRec.workspace_class.lineGC,
			       line->point, line->num_points,
			       CoordModeOrigin);
			line->is_to_be_drawn = FALSE;
		    }
	        }
	    }
	    line = line->next;
	}
	/*
	 * Do the 2 phase draw
	 */
	line = ww->workspace.lines;
	while( line )
	{
	    if( line->two_phase_draw )
	    {
		if( XtIsRealized((Widget)ww) )
		{
		    color = line->color;
		    /*  Get the GC and set it to this color  */
		    if( xmWorkspaceClassRec.workspace_class.lineGC == NULL )
			InitLineGC(ww, color);
		    else
			XSetForeground
			  (XtDisplay(ww),
			   xmWorkspaceClassRec.workspace_class.lineGC,
			   color);
		}
		if( XtIsRealized((Widget)ww) )
		{
		    if (line->num_points > 0)
		    {
			XDrawLines(XtDisplay(ww), XtWindow(ww),
			       xmWorkspaceClassRec.workspace_class.lineGC2,
			       line->point, line->num_points,
			       CoordModeOrigin);

		    }
		}
		tmp_line = line->next;
		while( tmp_line )
		{
		    if ( (tmp_line->two_phase_draw) &&
			 (tmp_line->point[0].x == line->point[0].x) &&
			 (tmp_line->point[0].y == line->point[0].y) )
		    {
			if (tmp_line->num_points > 0)
			{
			    XDrawLines(XtDisplay(ww), XtWindow(ww),
				   xmWorkspaceClassRec.workspace_class.lineGC2,
				   tmp_line->point, tmp_line->num_points,
				   CoordModeOrigin);

			}
		    }
		    tmp_line = tmp_line->next;
		}
	
		if( XtIsRealized((Widget)ww) )
		{
		    if (line->num_points > 0)
		    {
			XDrawLines(XtDisplay(ww), XtWindow(ww),
			       xmWorkspaceClassRec.workspace_class.lineGC,
			       line->point, line->num_points,
			       CoordModeOrigin);

			line->two_phase_draw = False;
			line->is_to_be_drawn = False;
		    }
		}
		tmp_line = line->next;
		while( tmp_line )
		{
		    if ( (tmp_line->two_phase_draw) &&
			 (tmp_line->point[0].x == line->point[0].x) &&
			 (tmp_line->point[0].y == line->point[0].y) )
		    {
			if (tmp_line->num_points > 0)
			{
			    XDrawLines(XtDisplay(ww), XtWindow(ww),
				   xmWorkspaceClassRec.workspace_class.lineGC,
				   tmp_line->point, tmp_line->num_points,
				   CoordModeOrigin);

			    tmp_line->two_phase_draw = False;
			    tmp_line->is_to_be_drawn = False;
			}
		    }
		    tmp_line = tmp_line->next;
	        }
	    }
	    line = line->next;
	}
    }
}
/*  Subroutine: InitLineGC
 *  Purpose:    Create the GC for drawing application lines in the workspace
 */
static void InitLineGC( XmWorkspaceWidget ww, int color )
{
    XGCValues values;
    unsigned long valuemask;

    values.foreground = (unsigned long)color;
    values.function = GXcopy;
    values.line_width = ww->workspace.line_thickness;
    if( values.line_width > 0 )
    {
	valuemask = GCForeground | GCFunction | GCLineWidth ;
    }
    else
	valuemask = GCForeground | GCFunction ;
    /*Use X call in place of Xt call since we will be changing gc's values  */
    xmWorkspaceClassRec.workspace_class.lineGC =
      XtGetGC((Widget)ww, valuemask, &values);

    /*
     * Create the line halo GC
     */
    values.foreground = ww->core.background_pixel;
    values.line_width = ww->workspace.line_thickness + 
			2*ww->workspace.halo_thickness;
    xmWorkspaceClassRec.workspace_class.lineGC2 =
      XtGetGC((Widget)ww, valuemask, &values);
}


/*  Subroutine: CvtStringToWorkspaceType
 *  Purpose:    Resource converter to be registered in class init to handle
 *              strings XXXX_XXXX which represent XmXXXX_XXXX from
 *              .Xdefaults file
 */
static void CvtStringToWorkspaceType( XrmValue* args, Cardinal num_args,
	                             XrmValue* from_val, XrmValue* to_val )
{
   char * in_str = (char *) (from_val->addr);
   static unsigned char i;

   to_val->size = sizeof (unsigned char);
   to_val->addr = (void *) &i;

   if( StringsAreEqual(in_str, "include_all") )
      i = XmINCLUDE_ALL;
   else if( StringsAreEqual(in_str, "include_any") )
      i = XmINCLUDE_ANY;
   else if( StringsAreEqual(in_str, "include_center") )
      i = XmINCLUDE_CENTER;
   else if( StringsAreEqual(in_str, "accent_background") )
      i = XmACCENT_BACKGROUND;
   else if( StringsAreEqual(in_str, "accent_border") )
      i = XmACCENT_BORDER;
   else if( StringsAreEqual(in_str, "accent_none") )
      i = XmACCENT_NONE;
   else if( StringsAreEqual(in_str, "outline_each") )
      i = XmOUTLINE_EACH;
   else if( StringsAreEqual(in_str, "outline_all") )
      i = XmOUTLINE_ALL;
   else if( StringsAreEqual(in_str, "outline_plus") )
      i = XmOUTLINE_PLUS;
   else if( StringsAreEqual(in_str, "draw_none") )
      i = XmDRAW_NONE;
   else if( StringsAreEqual(in_str, "draw_hash") )
      i = XmDRAW_HASH;
   else if( StringsAreEqual(in_str, "draw_line") )
      i = XmDRAW_LINE;
   else if( StringsAreEqual(in_str, "draw_outline") )
      i = XmDRAW_OUTLINE;
   else
   {
      to_val->size = 0;
      to_val->addr = NULL;
      XtStringConversionWarning ((char *)from_val->addr, XmRWorkspaceType);
   }
}

static Boolean StringsAreEqual( register char * in_str,
	                        register char * test_str )
{
   register int i;
   register int j;

   for (;;)
   {
      i = *in_str;
      j = *test_str;

      if (isupper (i)) i = tolower (i);
      if (i != j) return (False);
      if (i == 0) return (True);

      in_str++;
      test_str++;
   }
}


/*  Subroutine: XmCreateWorkspace
 *  Purpose:    Externally accessable convenience function to create
 *              workspace widget
 */
Widget XmCreateWorkspace( Widget parent, char* name,
	                  Arg arglist[], int argCount )
{
    return XtCreateWidget(name, xmWorkspaceWidgetClass, parent,
	                  arglist, argCount);
}


/*  Subroutine: AddWorkspaceAddCallback
 *  Purpose:    Register a callback for selection changes due to workspace
 *              widget management events.
 *  Note:       This routine must be used in place of XtAddCallback since
 *              Xt code cannot handle callback lists in constraint fields.
 */
void XmWorkspaceAddCallback( Widget child, String name,
	                     XtCallbackProc callback, XtPointer client_data )
{
    XmWorkspaceConstraints constraints;

    /*  Bypass Xt weakness of not handling callbacks in constraint resources  */
    if( callback )
    {
	constraints = WORKSPACE_CONSTRAINT(child);
	if( name && STRCMP(name, XmNaccentCallback) == 0 )
	    AddConstraintCallback(child,
	                          &(constraints->workspace.accent_callbacks),
	                          callback, client_data);
	else if( name && STRCMP(name, XmNselectionCallback) == 0 )
	    AddConstraintCallback(child,
	                          &(constraints->workspace.select_callbacks),
	                          callback, client_data);
	else if( name && STRCMP(name, XmNresizingCallback) == 0 )
	    AddConstraintCallback(child,
	                          &(constraints->workspace.resizing_callbacks),
	                          callback, client_data);
    }
}

#if 0
void XmWorkspaceGetMaxWidthHeight(Widget w, int *width, int *height)
{
XmWorkspaceWidget ww = (XmWorkspaceWidget)w;
int i;
Widget child;
XmWorkspaceConstraints constraints;

    *width = *height = 0;

    for( i=0; i<ww->composite.num_children; i++ )
    {
        child = ww->composite.children[i];
	if(!child->core.being_destroyed)
	{
	    constraints = WORKSPACE_CONSTRAINT(child);
	    if(constraints->workspace.x_right > *width)
		*width = constraints->workspace.x_right;
	    if(constraints->workspace.y_lower > *height)
		*height = constraints->workspace.y_lower;
	}
    }
    (*width)++;
    (*height)++;
}
#else
void XmWorkspaceGetMaxWidthHeight(Widget w, int *width, int *height)
{
XmWorkspaceWidget ww = (XmWorkspaceWidget)w;
int i,mw, mh,ext;
Widget child;

    mw = mh = 0;
    for (i=0; i<ww->composite.num_children; i++) {
	child = ww->composite.children[i];
	if (!XtIsManaged(child)) continue;
	if (XtClass(child) == xmWorkspaceWidgetClass) {
	    int tmpw, tmph;
	    XmWorkspaceGetMaxWidthHeight(child, &tmpw, &tmph);
	    mw = MAX(mw, tmpw);
	    mh = MAX(mh, tmph);
	} else {
	    ext = child->core.x + child->core.width;
	    mw = MAX(mw, ext);
	    ext = child->core.y + child->core.height;
	    mh = MAX(mh, ext);
	}
    }

    *width = mw;
    *height = mh;
}
#endif

/*  Subroutine: XmAddWorkspaceEventHandler
 *  Pupose:     Export routine to add event-handler type callback for button
 *              motion and button-release in Workspace background.
 *  Note:       The event owner widget need not be a child of the workspace.
 */
void XmAddWorkspaceEventHandler( XmWorkspaceWidget ww, Widget owner,
	                         XtEventHandler handler, XtPointer client_data )
{
    Window child;
    int x_offset, y_offset;

    /*  If this is a void call, clear the handler  */
    if( handler == NULL )
	ww->workspace.button_tracker = NULL;
    else
    {
	/*  Install the pointers  */
	ww->workspace.button_tracker = handler;
	ww->workspace.track_widget = owner;
	XTranslateCoordinates(XtDisplay(ww), XtWindow(ww), XtWindow(owner),
	                      0, 0, &x_offset, &y_offset, &child);
	ww->workspace.track_x = x_offset;
	ww->workspace.track_y = y_offset;
	ww->workspace.track_client_data = client_data;
    }
}

/*
 * Note on memory bug:
 * The code in this file and in Findroute.c assumes that all children are within
 * the bounds of this widget. (ww->core.{width,height} > bottom,right corner of
 * every child.)  If that becomes not true, then a core dump is imminent.  I added
 * the use of newWidth,newHeight here and a safety check in Findroute.c to protect
 * against this.
 */
void ReallocCollideLists(XmWorkspaceWidget ww)
{
CollideList **cl_ptr;
CollideList *ce_ptr;
CollideList *n_ce_ptr;
int         old_width  = ww->workspace.collide_width;
int         old_height = ww->workspace.collide_height;
int block_size;
int i;
int newWidth, newHeight, maxw, maxh;

    newWidth = ww->core.width;
    newHeight = ww->core.height;
    XmWorkspaceGetMaxWidthHeight ((Widget)ww, &maxw, &maxh);
    if ((maxw > ww->core.width) || (maxh > ww->core.height)) {
	/* 
	 * Being here is dangerous becuase the code 
	 * assumes that the bottom right of the farthest
	 * widget is inside the Workspace 
	 */
	if (maxw > ww->core.width) newWidth = maxw;
	if (maxh > ww->core.height) newHeight = maxh;
    }

    /* Do our own realloc since the other does not do the copy */
    if (newWidth > old_width) {
        cl_ptr = ww->workspace.collide_list_x;
        ww->workspace.collide_list_x = 
            (CollideList **)XtMalloc(sizeof(CollideList*)*(newWidth));
        memset(ww->workspace.collide_list_x, 0,
	      sizeof (CollideList*) * newWidth);
        block_size = sizeof(CollideList*) * MIN(old_width, newWidth);
        memcpy(ww->workspace.collide_list_x, cl_ptr, block_size);
        XtFree((char*)cl_ptr);
        ww->workspace.collide_width  = ww->core.width;
    } else {
        cl_ptr = ww->workspace.collide_list_x;
        for (i = newWidth; i < old_width; ++i)
        {
	    for (ce_ptr = cl_ptr[i]; ce_ptr; ce_ptr = n_ce_ptr)
	    {
	        n_ce_ptr = ce_ptr->next;
	        XtFree((char*)ce_ptr);
	    }
	    cl_ptr[i] = 0;
        }
    }

    if (newHeight > old_height) {
        cl_ptr = ww->workspace.collide_list_y;
        ww->workspace.collide_list_y = 
            (CollideList **)XtMalloc(sizeof(CollideList*)*(newHeight));
        memset(ww->workspace.collide_list_y, 0,
	      sizeof (CollideList*) * newHeight);
        block_size = sizeof(CollideList*) * MIN(old_height, newHeight);
        memcpy(ww->workspace.collide_list_y, cl_ptr, block_size);
        XtFree((char*)cl_ptr);
        ww->workspace.collide_height = newHeight;
    } else {
        cl_ptr = ww->workspace.collide_list_y;
        for (i = newHeight; i < old_height; ++i)
        {
	    for (ce_ptr = cl_ptr[i]; ce_ptr; ce_ptr = n_ce_ptr)
	    {
	        n_ce_ptr = ce_ptr->next;
	        XtFree((char*)ce_ptr);
	    }
	    cl_ptr[i] = 0;
	}
    }

}
/*
 * Return the number of distinct points in the path as the return value.
 * Place in the x and y arrays, the list of connecting points.
 * The return arrays x and y, must be XtFree()'d by the caller.
 */
int XmWorkspaceLineGetPath(XmWorkspaceLine wl, int **x, int **y)
{
    int *xx, *yy, i, points;

    points = wl->num_points;

    if (points > 0) {
	xx = (int*)XtMalloc(points * sizeof(int));
	yy = (int*)XtMalloc(points * sizeof(int));
	for (i=0; i<points ; i++) {
	    xx[i] = wl->point[i].x;
	    yy[i] = wl->point[i].y;
	} 
	*x = xx;
	*y = yy;
    } else {
	*x = NULL;
	*y = NULL;
    }
    return points;

}


/*
 * A quick-n-dirty way to check if a place is occupied by a standin.  This allows
 * StandIn not to inherit from DropSite.  It's a way to reject drops that would
 * land on top of an existing StandIn.   This was sought due to a theory that
 * said it was very expensive to put a StandIn on the screen if it had to be
 * registered as a drop site in addition to all its other responsibilities.
 * - Martin
 */
Boolean
XmWorkspaceLocationEmpty (Widget w, int x, int y)
{
XmWorkspaceWidget ww = (XmWorkspaceWidget)w;
Boolean unoccupied = True;
Widget child;
int i;

    for (i=0; ((i<ww->composite.num_children)&&(unoccupied)); i++) {
	child = ww->composite.children[i];
	if (!XtIsManaged(child)) continue;

	if (x<child->core.x) continue;
	if (y<child->core.y) continue;
	if (x>(child->core.x + child->core.width)) continue;
	if (y>(child->core.y + child->core.height)) continue;
	unoccupied = False;
    }
    return unoccupied;
}


Boolean
XmWorkspaceRectangleEmpty (Widget w, int x, int y, int width, int height)
{
XmWorkspaceWidget ww = (XmWorkspaceWidget)w;
Boolean unoccupied = True;
Widget child;
int i;

    for (i=0; ((i<ww->composite.num_children)&&(unoccupied)); i++) {
	child = ww->composite.children[i];
	if (!XtIsManaged(child)) continue;

	if ((x+width)<child->core.x) continue;
	if ((y+height)<child->core.y) continue;
	if (x>(child->core.x + child->core.width)) continue;
	if (y>(child->core.y + child->core.height)) continue;
	unoccupied = False;
    }
    return unoccupied;
}



#if RESIZE_HANDLES
static void status_check (Widget w, XEvent *event)
{
XmWorkspaceWidget ww;
Widget grab_child;
int i;
XmWorkspaceConstraints constraints;

    if (!ww->workspace.is_resizing) {
	for (i=0; i<ww->composite.num_children; i++)
	    if (inHierarchy(XtWindowToWidget(XtDisplay(ww), event->xany.window), ww, i))
		break;
	/*  This call comes from the accelerator so it should have a child  */
	if( i >= ww->composite.num_children )
	    return;
	grab_child = ww->composite.children[i];
	constraints = WORKSPACE_CONSTRAINT(grab_child);

	/*  Prepare to resize  */
	CreateSurrogate(ww, grab_child);
	ww->workspace.is_resizing = TRUE;
	/*  Use base variables to keep track of move applied so far  */
	ww->workspace.start.x = 0;
	ww->workspace.start.y = 0;
	ww->workspace.corner.x = grab_child->core.width;
	ww->workspace.corner.y = grab_child->core.height;
	ww->workspace.grab_x = event->xbutton.x;
	ww->workspace.grab_y = event->xbutton.y;
	ww->workspace.base_x = 0;
	ww->workspace.base_y = 0;
    }

    if( ww->workspace.move_cursor_installed == FALSE ) {
	XDefineCursor(XtDisplay(ww), XtWindow(ww),
		      xmWorkspaceClassRec.workspace_class.move_cursor);
	ww->workspace.move_cursor_installed = TRUE;
    }
}

static void ResizeNE( Widget w, XEvent* event, String* params, Cardinal* num_params )
{
XmWorkspaceWidget ww;
XPoint loc, size;
int tx,ty;
Widget child;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    status_check (w, event);
    ww->workspace.is_moving = TRUE;

    loc.x = event->xbutton.x - ww->workspace.start.x;
    loc.y = event->xbutton.y - ww->workspace.start.y;
    size.x = ww->workspace.corner.x + event->xbutton.x;
    size.y = ww->workspace.corner.y - event->xbutton.y;
    MoveSurrogate(ww, 0, loc.y);
    childRelative (ww, event, &tx, &ty);
    size.x = MAX(10, tx); 
    ResizeSurrogate (ww, size.x, size.y);
    ww->workspace.start.x = event->xbutton.x;
    ww->workspace.start.y = event->xbutton.y;
}
static void ResizeNW( Widget w, XEvent* event, String* params, Cardinal* num_params )
{
XmWorkspaceWidget ww;
XPoint loc, size;
int tx,ty;
Widget child;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    status_check (w, event);
    ww->workspace.is_moving = TRUE;

    loc.x = event->xbutton.x - ww->workspace.start.x;
    loc.y = event->xbutton.y - ww->workspace.start.y;
    size.x = ww->workspace.corner.x - event->xbutton.x;
    size.y = ww->workspace.corner.y - event->xbutton.y;
    MoveSurrogate(ww, loc.x, loc.y);
    ResizeSurrogate (ww, size.x, size.y);
    ww->workspace.start.x = event->xbutton.x;
    ww->workspace.start.y = event->xbutton.y;
}
static void ResizeSE( Widget w, XEvent* event, String* params, Cardinal* num_params )
{
XmWorkspaceWidget ww;
int width, height, tx,ty; 

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    status_check (w, event);
    ww->workspace.is_moving = FALSE;
    tx = event->xbutton.x; ty = event->xbutton.y;
    childRelative (ww, event, &tx, &ty);
    width = MAX(10, tx); height = MAX(10, ty);
    ResizeSurrogate(ww, width, height);
}
static void ResizeSW( Widget w, XEvent* event, String* params, Cardinal* num_params )
{
XmWorkspaceWidget ww;
XPoint loc, size;
int tx,ty;
Widget child;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    status_check (w, event);
    ww->workspace.is_moving = TRUE;

    loc.x = event->xbutton.x - ww->workspace.start.x;
    loc.y = event->xbutton.y - ww->workspace.start.y;
    size.x = ww->workspace.corner.x - event->xbutton.x;
    MoveSurrogate(ww, loc.x, 0);
    childRelative (ww, event, &tx, &ty);
    size.y = MAX(10, ty);
    ResizeSurrogate (ww, size.x, size.y);
    ww->workspace.start.x = event->xbutton.x;
    ww->workspace.start.y = event->xbutton.y;

}


static void NewDrop( Widget w, XEvent* event, String* params, Cardinal* num_params )
{
Widget child;
XmWorkspaceWidget ww;

    if (!(ww = WorkspaceOfWidget(w))) return ;

    if (ww->workspace.auto_arrange) return ;

    child = w;
    while ((child) && (XtParent(child) != (Widget)ww))  child = XtParent(child);
    if ((child) && (XtParent(child) == (Widget)ww)) 
	XtVaSetValues (child, XmNmappedWhenManaged, False, NULL);

    DropSelections (w, event, params, num_params);

    if ((child) && (XtParent(child) == (Widget)ww))  {
	XSync (XtDisplay(child), False);
	XtVaSetValues (child, XmNmappedWhenManaged, True, NULL);
    }
}
#endif
