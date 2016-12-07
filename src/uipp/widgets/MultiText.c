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
#include <stdlib.h>
#if defined (HAVE_CTYPE_H)
#include <ctype.h>
#endif
#if defined (HAVE_TYPES_H)
#include <types.h>
#endif
#include <X11/IntrinsicP.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>


#include "MultiTextP.h"

#if defined(intelnt)
/* winuser.h defines ScrollWindow which conflicts with code in this file */
#define ScrollWindow _ScrollWindow
#endif

#define CR			13
#define TAB			9
#define BACKSPACE		8
#define SPACE			32
#define CTRL_C			3

#define DEFAULT_WORD_SPACING	1
#define DEFAULT_LINE_SPACING	0
#define DEFAULT_FONT_NAME	"fixed"
#define MAX_BUFF_SIZE		32767
#define MAX_STR_LEN		1024

#define UP			0
#define DOWN			1
#define LEFT			2
#define RIGHT			3

#define TOP			1
#define BOTTOM			2
#define ON			TRUE
#define OFF			FALSE

#ifndef ABS
#define ABS(a)			(((a) > 0) ? (a) : -(a))
#endif

#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))

/*
 * Forward declaration of methods:
 */
static void ClassInitialize();

static void Initialize(Widget, Widget, ArgList, Cardinal*);

static void Destroy(Widget);

static void Redisplay(Widget, XEvent*, Region);

static void Resize(Widget);

static Boolean SetValues();

/*
 * Private routines used by the XmMultiTextWidget.
 */
static char* NewString(char*);

static XtGeometryResult	ChangeHeight(XmMultiTextWidget,
				     Dimension);

static void GetBoundingBox(XmMultiTextWidget,
			   char*,
			   XFontStruct*,
			   XCharStruct*,
			   int);

static void ShiftFollowingLines(LineList, int);

static Boolean          PtInLine                 (int x, int y, LineList lp);
static Boolean          PtInWord                 (int x, int y, WordList wp, LineList lp);
static Boolean          SameLinks                (WordList wp1, WordList wp2);
static void             SetCurrentFont           (XmMultiTextWidget w, WordList wp,
						  XFontStruct *font);
static void             SetCurrentColor          (XmMultiTextWidget w, WordList wp,
						  unsigned long color);
static void             DisplayImage             (XmMultiTextWidget w, WordList wp);
static void             DisplayWord              (XmMultiTextWidget w, WordList wp);
static void             DisplaySelectedWord      (XmMultiTextWidget w, WordList wp);
static void             DisplayDeselectedWord    (XmMultiTextWidget w, WordList wp);
static void             DrawLinkMarking          (XmMultiTextWidget w, WordList wp);
static void             CalculateSpacing         (Widget w, WordList wp, int *spaceBefore,
						  int *spaceAfter);
static void             DrawWord                 (XmMultiTextWidget w, WordList wp,
						  Boolean selected);
static void             DrawLine                 (XmMultiTextWidget w, LineList lp);
static void             ResizeTheLine            (XmMultiTextWidget w, LineList lp,
						  WordList theWord);
static void             FreeCharRecord           (XmMultiTextWidget w, CharList cp);
static void             FreeWordRecord           (XmMultiTextWidget w, WordList wp);
static void             FreeLineRecord           (XmMultiTextWidget w, LineList lp);
static int              NextTab                  (XmMultiTextWidget w, int *tabWidth,
						  int indent);
static unsigned long    Color                    (XmMultiTextWidget w, char *name);
static XFontStruct     *NamedFont                (XmMultiTextWidget w, char *name);
static LineList         NewLine                  (XmMultiTextWidget w, int indent, int yposn,
						  int lineSpacing);
static Boolean          StuffWordOntoCurrentLine (XmMultiTextWidget w, LineList lp, WordList wp,
						  int neededWidth, Boolean firstWordOfLine, 
						  Boolean isTab, Boolean prevWordIsTab);
static int              GetNeededWidth           (XmMultiTextWidget w, WordList wPtr, int indent,
						  Boolean *firstWordOfLine, Boolean *isTab,
						  Boolean *prevWordIsTab);
static unsigned short   StoreWord                (XmMultiTextWidget w, WordList wordPtr,
						  int indent);
static unsigned int     AppendWordToTopLine      (XmMultiTextWidget cw, WordList wordPtr,
						  int indent);
static void             PositionCursor           (XmMultiTextWidget cw, int x, int y,
						  LineList *lp, int *actualX);
static void             UpdateCursor             (XmMultiTextWidget cw, LineList lp, int x,
						  int actualX, int actualY);
static void             MakeCursorPixmaps        (XmMultiTextWidget cw);
static void             BlinkCursor              (XmMultiTextWidget cw);
static void             TurnOnCursor             (XmMultiTextWidget cw);
static void             TurnOffCursor            (XmMultiTextWidget cw);


/* actions */
static void MoveTo(Widget, XEvent*, String*, Cardinal*);
static void BtnMoveTo     (Widget, XEvent*, String*, Cardinal*);
static void BtnMoveNew    (Widget, XEvent*, String*, Cardinal*);
static void BtnRelease    (Widget, XEvent*, String*, Cardinal*);

static void KeyPush       (Widget, XEvent*, String*, Cardinal*);
static void NewLineCR     (Widget, XEvent*, String*, Cardinal*);
static void InsertSpace   (Widget, XEvent*, String*, Cardinal*);
static void MoveUp        (Widget, XEvent*, String*, Cardinal*);
static void MoveDown      (Widget, XEvent*, String*, Cardinal*);
static void MoveLeft      (Widget, XEvent*, String*, Cardinal*);
static void MoveRight     (Widget, XEvent*, String*, Cardinal*);
static void Enter         (Widget, XEvent*, String*, Cardinal*);
static void Leave         (Widget, XEvent*, String*, Cardinal*);
static void InFocus       (Widget, XEvent*, String*, Cardinal*);
static void OutFocus      (Widget, XEvent*, String*, Cardinal*);
static void DeleteLine    (Widget w, XKeyPressedEvent *event, String *argv, Cardinal *argc);
static void OpenLineTop   (Widget, XEvent*, String*, Cardinal*);
static void AddWordTop    (Widget, XEvent*, String*, Cardinal*);
static void Dummy         (Widget, XEvent*, String*, Cardinal*);

#ifndef NODPS
extern void ShowScale     (Widget, XEvent*, String*, Cardinal*);
#endif
static void Refresh       (Widget, XEvent*, String*, Cardinal*);
static void Cut           (Widget, XEvent*, String*, Cardinal*);


/*
 * Event handler?
 */
static void ChangeFocus   (Widget, XEvent*, String*, Cardinal*);


/* Public routines defined external in public header file. */
void           XmMultiTextClearText         (Widget w);
int            XmMultiTextGetPosition       (Widget w);
int            XmMultiTextGetCursorPosition (Widget w);
void           XmMultiTextQueryCursor       (Widget w, int *lineNumber, int *y);
void           XmMultiTextDeselectAll       (Widget w);
void           XmMultiTextSetTabStops       (Widget w, int tabList[], int tabCount);
LinkInfoPtr    XmMultiTextMakeLinkRecord    (Widget w, LinkType linkType, LinkPosition linkPosition,
					     char *linkData);
void           XmMultiTextAppendNewLine     (Widget w, char *theWord, char *fontName, int indent);
unsigned short XmMultiTextAppendWord        (Widget w, char *theWord, char *fontName,
					     char *colorName,
					     int indent, LinkInfoPtr linkInfo);
unsigned short XmMultiTextAppendWordTop     (Widget w,  char *theWord, char *fontName,
					     char *colorName, int indent, LinkInfoPtr linkInfo);
void           XmMultiTextOpenLineTop       (Widget w, int indent);
int            XmMultiTextDeleteLineTop     (Widget w, int linesToDelete);
void           XmMultiTextDeleteLineBottom  (Widget w);
int            XmMultiTextLongestLineLength (Widget w);
unsigned short XmMultiTextAppendImage       (Widget w, char *theWord, char *fontName,
					     char *colorName,
					     int indent, Pixmap image, unsigned int width,
					     unsigned int height, LinkInfoPtr linkInfo);
unsigned short XmMultiTextAppendWidget      (Widget w, char *theWord, char *fontName,
					     char *colorName,
					     int indent, Widget newW);
unsigned short XmMultiTextAppendDPS         (Widget w, char *theWord, char *fontName,
					     char *colorName,
					     int indent, char *dpsFileName, unsigned int width,
					     unsigned int height, LinkInfoPtr linkInfo);
Boolean        XmMultiTextAppendChar        (Widget w, char str);
Boolean        XmMultiTextAppendLine        (Widget w, char *str);

static XtResource resources[] = 
{
    {
	XmNdpsCapable,
	XmCDPSCapable,
	XmRBoolean,
	sizeof(Boolean),
	offset(multiText.dpsCapable),
	XmRImmediate,
	(XtPointer)TRUE
    },
    {
	XmNwordWrap,
	XmCWordWrap,
	XmRBoolean,
	sizeof(Boolean),
	offset(multiText.wordWrap),
	XmRImmediate,
	(XtPointer)TRUE
    },
    {
	XmNmarginWidth,
	XmCMarginWidth,
	XmRInt,
	sizeof(int),
	offset(multiText.marginWidth),
	XmRImmediate,
	(XtPointer)10
    },
    {
	XmNmarginHeight,
	XmCMarginHeight,
	XmRInt,
	sizeof(int),
	offset(multiText.marginHeight),
	XmRImmediate,
	(XtPointer)10
    },
    {
	XmNwaitCursorCount,
	XmCWaitCursorCount,
	XmRInt,
	sizeof(int),
	offset(multiText.waitCursorCount),
	XmRImmediate,
	(XtPointer)1
    },
    {
	XmNblinkRate,
	XmCBlinkRate,
	XmRInt,
	sizeof(int),
	offset(multiText.blinkRate),
	XmRImmediate,
	(XtPointer)250
    },
    {
	XmNshowCursor,
	XmCShowCursor,
	XmRBoolean,
	sizeof(Boolean),
	offset(multiText.showCursor),
	XmRImmediate,
	(XtPointer)TRUE
    },
    {
	XmNfocusSensitive,
	XmCFocusSensitive,
	XmRBoolean,
	sizeof(Boolean),
	offset(multiText.focusSensitive),
	XmRImmediate,
	(XtPointer)FALSE
    },
    {
	XmNcursorColor,
	XmCForeground,
	XmRPixel,
	sizeof(Pixel),
	offset(multiText.cursorColor),
	XmRString,
	"yellow"
    },
    {
	XmNscaleDPSpercent,
	XmCScaleDPSpercent,
	XmRInt,
	sizeof(int),
	offset(multiText.scaleDPSpercent),
	XmRImmediate,
	(XtPointer)100
    },
    {
	XmNsmartSpacing,
	XmCSmartSpacing, XmRBoolean,
	sizeof(Boolean),
	offset(multiText.smartSpacing),
	XmRImmediate,
	(XtPointer)TRUE
    },
    {
	XmNsmoothScroll,
	XmCSmoothScroll,
	XmRBoolean,
	sizeof(Boolean),
	offset(multiText.smoothScroll),
	XmRImmediate,
	(XtPointer)FALSE
    },
    {
	XmNexposeOnly,
	XmCExposeOnly,
	XmRBoolean,
	sizeof(Boolean),
	offset(multiText.exposeOnly),
	XmRImmediate,
	(XtPointer)FALSE
    },
    {
	XmNlinkCallback, XmCCallback, XmRCallback,
	sizeof(XtCallbackList), offset(multiText.linkCallback),
	XmRImmediate, (XtPointer) NULL
    },
    {
	XmNselectCallback,
	XmCCallback,
	XmRCallback,
	sizeof(XtCallbackList),
	offset(multiText.selectCallback),
	XmRImmediate,
	(XtPointer)NULL
    }
};


/*
 * Default Translation Table:
 *  This table maps event sequences into action names.
 */
#ifndef NODPS
static char defaultTranslations[] =
    "Ctrl Shift<Btn1Down>:  ShowScale()           \n\
     <Btn1Motion>:          BtnMoveTo()           \n\
     <Btn1Up>:              BtnRelease()          \n\
     <Btn1Down>:            MoveTo()              \n\
     <Key>BackSpace:        Cut()                 \n\
     <Key>Up:               MoveUp()              \n\
      <Key>Down:             MoveDown()           \n\
     <Key>Left:             MoveLeft()            \n\
     <Key>Right:            MoveRight()           \n\
     <Key>Delete:           Dummy()               \n\
     <Key>Return:           NewLineCR()           \n\
     <FocusIn>:             InFocus()             \n\
     <FocusOut>:            OutFocus()            \n\
     <Key>F1:               DeleteLine(1)         \n\
     <Key>F2:               DeleteLine(2)         \n\
     <Key>F3:               OpenLineTop()         \n\
     <Key>F5:               AddWordTop()";
#else
static char defaultTranslations[] =
    "<Btn1Motion>:          BtnMoveTo()           \n\
     <Btn1Up>:              BtnRelease()          \n\
     <Btn1Down>:            MoveTo()              \n\
     <Key>BackSpace:        Cut()                 \n\
     <Key>Up:               MoveUp()              \n\
     <Key>Down:             MoveDown()            \n\
     <Key>Left:             MoveLeft()            \n\
     <Key>Right:            MoveRight()           \n\
     <Key>Delete:           Dummy()               \n\
     <Key>Return:           NewLineCR()           \n\
     <FocusIn>:             InFocus()             \n\
     <FocusOut>:            OutFocus()            \n\
     <Key>F1:               DeleteLine(1)         \n\
     <Key>F2:               DeleteLine(2)         \n\
     <Key>F3:               OpenLineTop()         \n\
     <Key>F5:               AddWordTop()";
#endif


/*=======================================================*
 |                      Actions Table                    |
 | the action table maps string action names into actual |
 | action functions.                                     |
 *=======================================================*/

static XtActionsRec actions[] = {
  {"MoveTo",      MoveTo      },
  {"BtnMoveTo",   BtnMoveTo   },
  {"BtnMoveNew",  BtnMoveNew  },
  {"BtnRelease",  BtnRelease  },
  {"Refresh",     Refresh     },
  {"Cut",         Cut         },
  {"KeyPush",     KeyPush     },
  {"NewLineCR",   NewLineCR   },
  {"InsertSpace", InsertSpace },
#ifndef NODPS
  {"ShowScale",   ShowScale   },
#endif
  {"MoveUp",      MoveUp      },
  {"MoveDown",    MoveDown    },
  {"MoveLeft",    MoveLeft    },
  {"MoveRight",   MoveRight   },
  {"Enter",       Enter       },
  {"Leave",       Leave       },
  {"InFocus",     InFocus     },
  {"OutFocus",    OutFocus    },
  {"DeleteLine",  (XtActionProc)DeleteLine  },
  {"Dummy",       Dummy       },
  {"OpenLineTop", OpenLineTop },
  {"AddWordTop",  AddWordTop  },
};



/*  The MultiText class record definition  */

XmMultiTextClassRec xmMultiTextClassRec =
{
    /*
     * Core class part:
     */
    {
	(WidgetClass)&xmDrawingAreaClassRec,
					/* superclass			*/
	"XmMultiText",			/* class_name			*/
	sizeof(XmMultiTextRec),		/* widget_size			*/
	ClassInitialize,		/* class_initialize		*/
	NULL,				/* class_part_initialize 	*/
	FALSE,				/* class_inited			*/
	Initialize,			/* initialize			*/	
	NULL,				/* initialize_hook		*/
	XtInheritRealize,		/* realize			*/
	actions,			/* actions			*/
	XtNumber(actions),		/* num_actions			*/
	resources,			/* resources			*/
	XtNumber(resources),		/* num_resources		*/
	NULLQUARK,			/* xrm_class			*/
	TRUE,				/* compress_motion		*/
	TRUE,				/* compress_exposure		*/
	TRUE,				/* compress_enterleave		*/
	FALSE,				/* visible_interest		*/
	Destroy,			/* destroy			*/
	Resize,				/* resize			*/
	Redisplay,			/* expose			*/
	(XtSetValuesFunc)SetValues,	/* set_values			*/
	NULL,				/* set_values_hook		*/
	XtInheritSetValuesAlmost,	/* set_values_almost		*/
	NULL,				/* get_values_hook		*/
	NULL,				/* accept_focus			*/
	XtVersion,			/* version			*/
	NULL,				/* callback private		*/
	defaultTranslations,		/* tm_table			*/
	XtInheritQueryGeometry,		/* query_geometry		*/
	NULL,				/* display_accelerator		*/
	NULL				/* extension			*/
    },

    /*
     * Compositie class part:
     */
    {
	XtInheritGeometryManager,	/* Geometry Manager		*/
	XtInheritChangeManaged,		/* Change Managed		*/
	XtInheritInsertChild,		/* Insert Child			*/
	XtInheritDeleteChild,		/* Delete Child			*/
	NULL				/* extension			*/
    },

    /*
     * Constraint class part:
     */
    {
	NULL,				/* resources			*/
	0,				/* num resources		*/
	0,				/* constraint record		*/
	NULL,				/* initialize			*/
	NULL,				/* destroy			*/
	NULL,				/* set values			*/
	NULL				/* extension			*/
    },

   /*
     * Manager class part:
     */
    {
	NULL,				/* default_translations		*/
	NULL,				/* get_resources		*/
	0,				/* num_get_resources		*/
	NULL,				/* get_cont_resources		*/
	0,				/* num_get_cont_resources	*/
	(XmParentProcessProc)NULL,      /* parent_process         	*/
	NULL				/* extension			*/
    },

    /*
     * DrawingArea class part:
     */
    {
	NULL				/* dummy field			*/
    },

	    
    {
	NULL				/* extension			*/
    },
};


WidgetClass xmMultiTextWidgetClass = (WidgetClass)&xmMultiTextClassRec;



/*======================================================================*
 |                      XmMultiText Widget Methods                      |
 |                      --------------------------                      |
 | These are private to XmMultiText and can't be accessed by the        |
 | application programmer directly.                                     |
 *======================================================================*/

/*
 * This routine creates an XFontStruct and a GC.  Both of these are put
 * into the instance record of the MultiTextWidget.
 */
static void
GetTextGC(XmMultiTextWidget cw)
{
    XtGCMask     mask;
    XGCValues    values;
    XFontStruct* font;

    mask =
	GCForeground |
	GCBackground |
	GCFont       |
        GCGraphicsExposures |
        GCSubwindowMode;

    font = XLoadQueryFont(XtDisplay(cw), "fixed");

    values.font               = font->fid;
    values.foreground         = cw->manager.foreground;
    values.background         = cw->core.background_pixel;
    values.graphics_exposures = False;
    values.subwindow_mode     = ClipByChildren;

    cw->multiText.textGC =
	XCreateGC(XtDisplay(cw),
		  XRootWindowOfScreen(XtScreen(cw)),
		  mask,
		  &values);

    values.foreground = cw->multiText.cursorColor;
    cw->multiText.cursorGC =
      XCreateGC(XtDisplay(cw),
		XRootWindowOfScreen(XtScreen(cw)),
		mask,
		&values);

    XFreeFont(XtDisplay(cw), font);
}



/*
 * Only draws the lines that are effected by the refreshed area.
 */
static void
DoRedrawText(XmMultiTextWidget cw,
	     int               y,
	     int               height)
{
    LineList lp = cw->multiText.firstLine;
    int      y1;
    int      y2;
    int      ey1;
    int      ey2;


    if (cw->multiText.drawing)
	return;

    cw->multiText.drawing = TRUE;

    ey1 = y;
    ey2 = y + height;


    while (lp != NULL)
    {
	y1 = lp->y;
	y2 = lp->y + lp->bbox.ascent + lp->bbox.descent;

	if (!(((ey1 < y1) && (ey2 < y1)) || ((y1 < ey1) && (y2 < ey1))))
	{
	    DrawLine(cw, lp);
	}

	lp = lp->next;
    }

    cw->multiText.drawing = FALSE;
}



static void
StringToFloatConverter(XrmValue* args,
		       Cardinal* nargs,
		       XrmValue* fromVal,
		       XrmValue* toVal)
{
    static float result;

    /*
     * Make sure the number of args is correct.
     */
    if (*nargs != 0)
    {
	XtWarning("String to Float conversion needs no arguments");
    }

    /*
     * Convert the string in the fromVal to a floating pt.
     */
    if (sscanf((char*)fromVal->addr, "%f", &result) == 1)
    {
	/*
	 * Make the toVal point to the result.
	 */
	toVal->size = sizeof(float);
	toVal->addr = (void *)&result;
    }
}



static void
ClassInitialize(WidgetClass wc)
{
#ifdef	intelnt /* Exceed on WINDOWS NT has _XmRegisterConverters()   */
    _XmRegisterConverters();
#else
    XmRegisterConverters();
#endif
    XtAddConverter(XmRString, XmRFloat, StringToFloatConverter, NULL, 0);
}



/*
 * The instance record of each widget must be initialized at run time.
 * This method is invoked when a new widget is created.
 * 'newWidget' is the real widget, 'request' is a copy.  'request' contains
 * the original resource values.  'newWidget' may have been modified by other
 * initialize routines of superclasses.  All changes are made to 'newWidget'.
 */
static void Initialize(Widget		 reqWidget,
		       Widget		 newWidgetWidget,
		       ArgList           arg,
		       Cardinal*         argc)
{
    XmMultiTextWidget req = (XmMultiTextWidget)reqWidget;
    XmMultiTextWidget newWidget = (XmMultiTextWidget)newWidgetWidget;
    int   i;
    char  buf[MAX_STR_LEN];


#ifndef NODPS

    int   firstEvent;
    int   firstError;
    int   majorOpCode;
    if (!XQueryExtension(XtDisplay(req),
			 DPSNAME,
			 &majorOpCode,
			 &firstEvent,
			 &firstError))
    {
	WarningDialog(newWidget, MSG7);
    }

#endif

    newWidget->manager.traversal_on = True;

    newWidget->multiText.firstLine     = NULL;
    newWidget->multiText.currentLine   = NULL;
    newWidget->multiText.lastLine      = NULL;

    newWidget->multiText.drawing       = FALSE;
    newWidget->multiText.selecting     = FALSE;

    newWidget->multiText.actualX       = 0;
    newWidget->multiText.cursorX       = 0;
    newWidget->multiText.cursorY       = 0;
    newWidget->multiText.oldX          = 0;
    newWidget->multiText.oldY          = 0;
    newWidget->multiText.cursorFg      = None;
    newWidget->multiText.cursorBg      = None;
    newWidget->multiText.blinkState    = FALSE;
    newWidget->multiText.cursorAvailable = TRUE;
    newWidget->multiText.blinkTimeOutID = 0;

    newWidget->multiText.wordSpacing   = DEFAULT_WORD_SPACING;

    newWidget->multiText.currentFontID = 0;
    newWidget->multiText.currentColor  = 0;
    newWidget->multiText.lastColorName = NewString("");
    newWidget->multiText.numFonts      = 0;
    newWidget->multiText.fontCache     = NULL;
    newWidget->multiText.showCursor = req->multiText.showCursor;
    if (newWidget->multiText.showCursor)
    {
	TurnOnCursor(newWidget);
    }
    else
    {
	TurnOffCursor(newWidget);
    }

    for (i = 0; i < MAX_TAB_COUNT; i++) newWidget->multiText.tabs[i] = 0;
    newWidget->multiText.tabCount = 0;

    newWidget->multiText.waitCursorIndex = 1;

    if ((newWidget->multiText.waitCursorCount < 1) ||
	(newWidget->multiText.waitCursorCount > CURSOR_COUNT))
    {
	sprintf (buf, MSG1, CURSOR_COUNT);
	XtWarning(buf);
	newWidget->multiText.waitCursorCount = 1;
    }

    if (newWidget->multiText.scaleDPSpercent <= 0)
    {
	XtWarning(MSG2);
	newWidget->multiText.scaleDPSpercent = 1;
    }

    GetTextGC(newWidget);

    XtAddEventHandler((Widget)newWidget, FocusChangeMask, FALSE, (XtEventHandler)ChangeFocus, (Opaque)NULL);
    XtAddEventHandler((Widget)newWidget, EnterWindowMask, FALSE, (XtEventHandler)Enter,       (Opaque)NULL);
    XtAddEventHandler((Widget)newWidget, LeaveWindowMask, FALSE, (XtEventHandler)Leave,       (Opaque)NULL);
}



/*---------------------------================---------------------------*
 |                           Redisplay method                           |
 | This is where the widget gets redrawn when an Expose event occurs.   |
 | Note that this routine can't be called 'Expose' since X.h defined    |
 | 'Expose' as 12.                                                      |
 | w      - defines the widget instance to be redisplayed.              |
 | event  - defines a pointer to an Expose event.                       |
 | region - is the region to be redrawn.  If compress_exposures is true |
 |          then the region is the sum of rectangles reported on all    |
 |          expose events, and the event parameter contains the bounding|
 |          box of the region.  Otherwise, region is NULL.              |
 *---------------------------================---------------------------*/
static void Redisplay (Widget w, XEvent *event, Region region)
{
  XmMultiTextWidget cw = (XmMultiTextWidget)w;
  XExposeEvent *expose = (XExposeEvent*)event;

  if (XtIsRealized((Widget)cw))
    {
      DoRedrawText(cw, expose->y, expose->height);
      
      if ((cw->multiText.cursorFg == None) && cw->multiText.cursorAvailable)
	{
	  if (cw->multiText.blinkTimeOutID != 0)
	    {
	      XtRemoveTimeOut(cw->multiText.blinkTimeOutID);
	      cw->multiText.blinkTimeOutID = 0;
	    }
	  BlinkCursor(cw);
	}
    }
}


/*---------------------------================---------------------------*
 |                           SetValues method                           |
 | This method allows a widget to be notified when one of its resources |
 | is set or changed.  This can be called when a widget is initialized  |
 | by the resource manager, or when an application sets a resource with |
 | XtSetValues().  The SetValues methods are chained and called from    |
 | superclass to subclass order.                                        |
 | current - the previous, unaltered state of the widget.               |
 | request - the values requested for the widget.                       |
 | newWidget     - the state of the widget after all superclass's SetValues   |
 |           methods were called.  All changes must be made to newWidget.     |
 | SetValues returns a Boolean value specifying whether or not the      |
 | widget needs to be redrawn. If TRUE, the Intrinsics causes an Expose |
 | event to be generated for the entire window.                         |
 | Note that SetValues must not perform any graphics operations on the  |
 | widget's window unless the widget is realized.                       |
 *---------------------------================---------------------------*/
static Boolean SetValues (XmMultiTextWidget current, XmMultiTextWidget request,
			  XmMultiTextWidget newWidget)
{
  Boolean redraw = FALSE;
  char    buf[MAX_STR_LEN];

  if (current->multiText.waitCursorCount != newWidget->multiText.waitCursorCount)
    if ((newWidget->multiText.waitCursorCount < 1) || (newWidget->multiText.waitCursorCount > CURSOR_COUNT))
      {
	sprintf(buf, MSG1, CURSOR_COUNT);
	XtWarning(buf);
	newWidget->multiText.waitCursorCount = 1;
      }

  /* there really isn't any way to screw-up the values of the MultiTextWidget, yet... */
  /* however, it is possible to cause a redraw by changing the wordwrap value.        */

  if (current->multiText.wordWrap != newWidget->multiText.wordWrap)
    redraw = TRUE;

  /* check that the DPSscaling percentage is correct. */
  if (newWidget->multiText.scaleDPSpercent <= 0)
    {
      XtWarning(MSG2);
      newWidget->multiText.scaleDPSpercent = 1;
    }

  /* check if there's a change in the showCursor state. */
  if (current->multiText.showCursor != newWidget->multiText.showCursor)
    {
      if (newWidget->multiText.showCursor)
	TurnOnCursor(newWidget);
      else
	TurnOffCursor(newWidget);
    }

  /* should add checking for margins here... */

  return redraw;
}


/*----------------------------==============----------------------------*
 |                            Destroy method                            |
 | The Destroy method is invoked when a widget is destroyed.  Chaining  |
 | is in reverse order from other chaining; the widget's method is      |
 | called first followed by its superclass's Destroy method.            |
 *----------------------------==============----------------------------*/
static void Destroy (Widget w)
{
  XmMultiTextWidget cw = (XmMultiTextWidget)w;
  int i;

  /* free the font cache. */
  for (i=0; i<cw->multiText.numFonts; i++ )
    {
      XFreeFont(XtDisplay(cw), cw->multiText.fontCache[i].font);
#ifdef COMMENT
      XtFree(cw->multiText.fontCache[i].font);
#endif
      XtFree(cw->multiText.fontCache[i].name);
    }
  XtFree((char*)cw->multiText.fontCache);
  XtFree(cw->multiText.lastColorName);

  if (cw->multiText.textGC)   XFreeGC(XtDisplay(cw), cw->multiText.textGC);
  if (cw->multiText.cursorGC) XFreeGC(XtDisplay(cw), cw->multiText.cursorGC);
  if (cw->multiText.blinkTimeOutID != 0) XtRemoveTimeOut(cw->multiText.blinkTimeOutID);
}


/*-----------------------------=============----------------------------*
 |                             Resize method                            |
 | The Resize method is invoked when the widget's window is reconfigured|
 | in any way.  Since the X server generates an Expose event if the     |
 | contents of a window are corrupted, the Resize method only updates   |
 | the data needed to allow the Redisplay method to redraw the widget   |
 | correctly.                                                           |
 *-----------------------------=============----------------------------*/
static void Resize (Widget w)
{
  XmMultiTextWidget cw = (XmMultiTextWidget)w;
  if (cw->multiText.smoothScroll)
    {
      XRectangle recs[1];

      recs[0].x      = cw->multiText.marginWidth;
      recs[0].y      = cw->multiText.marginHeight;
      recs[0].width  = cw->core.width;
      recs[0].height = cw->core.height;

      XSetClipRectangles(XtDisplay(cw), cw->multiText.textGC, 0, 0, recs, 1, Unsorted);
    }
}
 




/*======================================================================*
 |                XmMultiText Widget Action Procedures                  |
 |                ------------------------------------                  |
 | The(se) routine(s) are defined at the beginning of the file in the   |
 | list of actions.                                                     |
 *======================================================================*/


/*----------------------------------------------------------------------*
 |                               PtInWord                               |
 *----------------------------------------------------------------------*/
static Boolean PtInWord (int x, int y, WordList wp, LineList lp)
{
  if ((x >= lp->x + wp->x) && (x < lp->x + wp->x + wp->bbox.width))
    return TRUE;
  else

    /* now for a bit more complex - if this is a link then the bounding box */
    /* is different depending if there is a link behind or after this word. */
    /* in this case, we must account for the spacing between words as well  */
    /* as the actual word itself. - yuk!                                    */
  if (wp->linkInfo != NULL)
    {
      int x1 = lp->x + wp->x;
      int x2 = lp->x + wp->x + wp->bbox.width;

      if ((wp->prev != NULL) && (wp->prev->linkInfo != NULL))
	x1 -= wp->spacing;

      if ((wp->next != NULL) && (wp->next->linkInfo != NULL))
	x2 += wp->spacing;

      /* now let's see if it's in the new larger bounding box. */
      if ((x >= x1) && (x < x2))
	return TRUE;
      else
	return FALSE;
    }

  else
    return FALSE;
}



/*----------------------------------------------------------------------*
 |                           HighlightWord                              |
 *----------------------------------------------------------------------*/
static void HighlightWord (XmMultiTextWidget cw, LineList lp, WordList wp, int x2, int y2)
{
  int     x1   = cw->multiText.startX;
  int     y1   = cw->multiText.startY;
  int     wpx1 = wp->x + lp->x;
  int     wpy1 = lp->y;
  int     wpx2 = wp->x + lp->x + wp->bbox.width;
  int     wpy2 = lp->y + lp->bbox.ascent + lp->bbox.descent;
  Boolean startAfter, startBefore, endAfter, endBefore, startInside, endInside;


  startAfter  = (((x1  > wpx2) && (y1 >= wpy1))  ||  (y1 > wpy2));
  startBefore = (((x1  < wpx1) && (y1 <= wpy2))  ||  (y1 < wpy1));
  endAfter    = (((x2  > wpx2) && (y2 >  wpy1))  ||  (y2 > wpy2));
  endBefore   = (((x2  < wpx1) && (y2 <  wpy2))  ||  (y2 <= wpy1));
  startInside = ((wpx1 < x1)   && (x1 <= wpx2) && (wpy1 <  y1) && (y1 <= wpy2));
  endInside   = ((wpx1 < x2)   && (x2 <= wpx2) && (wpy1 <  y2) && (y2 <=  wpy2));

  if ((startBefore && endAfter) || (startAfter && endBefore) || startInside || endInside)
    {
      if (!wp->selected)
	{
	  DrawWord(cw, wp, TRUE);
	  wp->selected = TRUE;
	}
    }
  else
    if (wp->selected)
      {
	DrawWord(cw, wp, FALSE);
	wp->selected = FALSE;
      }
}


/*----------------------------------------------------------------------*
 |                              BtnMoveTo                               |
 | This gets call when the mouse button is pressed while the mouse is   |
 | moved.                                                               |
 *----------------------------------------------------------------------*/
static void BtnMoveTo (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  LineList          lp;
  WordList          wp;


  if (cw->multiText.selecting) /* selecting turns on when button is pressed. Off on release. */
    {
      /* go through each word and highlight if in region and unhighlight if not. */
      for (lp = cw->multiText.firstLine; lp != NULL; lp = lp->next)
	for (wp = lp->firstWord; wp != NULL; wp = wp->next)
	  HighlightWord(cw, lp, wp, event->xbutton.x, event->xbutton.y);
    }
}



/*----------------------------------------------------------------------*
 |                            BtnMoveNew                                |
 | This gets call when the mouse button is pressed while the mouse is   |
 | moved.                                                               |
 | This routine highlights the lines in the region - first find the line|
 | that the pointer is currently in, then find what line the selection  |
 | started in.  Highlight all lines between these line but not including|
 | these.  Finally highlight the region corresponding to what should be |
 | selected for each of the lines containing the start or end points of |
 | the selection region.                                                |
 *----------------------------------------------------------------------*/
static void BtnMoveNew (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  LineList          lp, lp1, lp2;
  int               y1, y2;

  /* find the line containing the starting point (the lowest of the y positions). */
  y1 = MIN(cw->multiText.startY, event->xbutton.y);
  y2 = MAX(cw->multiText.startY, event->xbutton.y);

  for (lp1 = cw->multiText.firstLine;
       (lp1 != NULL) &&  !((lp1->y <= y1) && (y1 <= lp1->y + lp1->bbox.ascent + lp1->bbox.descent));
       lp1 = lp1->next)
    /* move lp1 to start of selection region. */ ;


  /* now lp1 points to the line where the selection region is starting in. */
  /* find the line where the selection ends.                               */

  for (lp2 = lp1;
       (lp2 != NULL) &&  !((lp2->y <= y2) && (y2 <= lp2->y + lp2->bbox.ascent + lp2->bbox.descent));
       lp2 = lp2->next)
    /* lp2 points to where the selection ends. */;


  /* highlight each line between lp1 and lp2. */
  for (lp = lp1; lp != lp2; lp = lp->next)
    /* highlight each line... TBA */ ;
}



/*----------------------------------------------------------------------*
 |                           StuffOntoClipboard                         |
 *----------------------------------------------------------------------*/
void ToClipboard (XmMultiTextWidget cw, char *data)
{
  long  clipID = 0;
  int            status;
  XmString       clipLabel;
  char           buf[32];
  static int     cnt;


  sprintf(buf, "%s-%d", data, ++cnt);
  clipLabel = XmStringCreate(data, XmSTRING_DEFAULT_CHARSET);

  do
    status = XmClipboardStartCopy(XtDisplay(cw), XtWindow(cw),
				  clipLabel, CurrentTime, NULL, NULL, &clipID);
  while (status == ClipboardLocked);
  XmStringFree(clipLabel);

  do
    status = XmClipboardCopy(XtDisplay(cw), XtWindow(cw),
			     clipID, "STRING", buf, (long)strlen(buf)+1, cnt, NULL);
  while (status == ClipboardLocked);

  do
    status = XmClipboardEndCopy(XtDisplay(cw), XtWindow(cw), clipID);
  while (status == ClipboardLocked);
}



/*----------------------------------------------------------------------*
 |                               BtnRelease                             |
 *----------------------------------------------------------------------*/
static void BtnRelease (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget                cw = (XmMultiTextWidget) w;
  static char                      mondoBuffer[MAX_BUFF_SIZE], *space = " ", *cr = "\n";
  LineList                         lp;
  WordList                         wp;
  XmMultiTextSelectCallbackStruct  callValue;
  Boolean                          foundSomething, textIsSelected = FALSE;


  if (cw->multiText.selecting)
    {
      cw->multiText.selecting = FALSE;

      strcpy(mondoBuffer,"");

      /* grab the selected text. */
      foundSomething = FALSE;
      for (lp = cw->multiText.firstLine; lp != NULL; lp = lp->next)
	{
	  foundSomething = FALSE;
	  for (wp = lp->firstWord; wp != NULL; wp = wp->next)
	    if (wp->selected)
	      {
		foundSomething = textIsSelected = TRUE;
		strcat(mondoBuffer, wp->chars);
		if (cw->multiText.smartSpacing) strcat(mondoBuffer, space);
	      }
	    else
	      foundSomething = FALSE;
	  if (foundSomething) strcat(mondoBuffer, cr);
	}

      /* Check if the selection callback exists. */
      callValue.reason   = XmCR_SELECT;
      callValue.event    = event;
      callValue.data     = NewString(mondoBuffer);
      
      if (XtHasCallbacks((Widget)cw, XmNselectCallback) == XtCallbackHasSome)
	{
	  XFlush(XtDisplay(w));
	  XtCallCallbacks((Widget)cw, XmNselectCallback, &callValue);
	}
      XtFree(callValue.data);
    }

  /*
  if (textIsSelected) ToClipboard(cw, mondoBuffer);
  */

  cw->multiText.textIsSelected = textIsSelected;
}





/*----------------------------------------------------------------------*
 |                              Refresh                                 |
 | causes a complete redraw of the text widget.                         |
 *----------------------------------------------------------------------*/
static void Refresh (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  LineList          lp = cw->multiText.firstLine;


  XmMultiTextClearText((Widget)cw);
  
  while (lp != NULL)
    {
      DrawLine(cw, lp);
      lp = lp->next;
    }
}



/*----------------------------------------------------------------------*
 |                        DeleteLineFromWidget                          |
 *----------------------------------------------------------------------*/
void DeleteLineFromWidget (XmMultiTextWidget cw, LineList *lpp)
{
  LineList temp;


  if (*lpp == NULL) return;

  if ((*lpp)->prev == NULL)
    cw->multiText.firstLine = NULL;
  else
    (*lpp)->prev->next = (*lpp)->next;

  if ((*lpp)->next == NULL)
    cw->multiText.lastLine = (*lpp)->prev;
  else
    (*lpp)->next->prev = (*lpp)->prev;


  if ((*lpp)->prev == NULL)
    temp = (*lpp)->next;
  else
    temp = (*lpp)->prev;

  FreeLineRecord(cw, *lpp);

  *lpp = temp;
}




/*----------------------------------------------------------------------*
 |                           LineTooLong                                |
 | A line is too long if it has more than one word AND doesn't fit into |
 | the widget AND the last word is allowed to be wrapped.               |
 *----------------------------------------------------------------------*/
Boolean LineTooLong (XmMultiTextWidget cw, LineList lp)
{
  /* if the line is null or empty, return false. */
  if ((lp == NULL) || (lp->lastWord == NULL))
    return (FALSE);

  /* if the line is too long and the last word is 'wrappable', then return true. */
  if (((lp->x + lp->bbox.width)  >  (cw->core.width - cw->multiText.marginWidth)) &&
      (lp->lastWord->wordWrapping && cw->multiText.wordWrap))
    return (TRUE);
  else
    return (FALSE);
}


/*----------------------------------------------------------------------*
 |                      WrapLastWordToNextLine                          |
 | this routine assumes that the last word is allowed to be wrapped. In |
 | other words - it just performs the wrapping and does what it's told. |
 *----------------------------------------------------------------------*/
void WrapLastWordToNextLine (XmMultiTextWidget cw, LineList lp)
{
  int      dx;
  WordList wp;


  /* let's do some simple checking - these errors should never occur... */
  if ((lp == NULL) || (lp->lastWord == NULL) || (lp->lastWord == lp->firstWord))
    return;

  wp              = lp->lastWord;
  lp->lastWord    = wp->prev;
  dx              = lp->bbox.width - lp->lastWord->x + lp->lastWord->bbox.width;
  lp->bbox.width -= dx;
  lp->remaining  += dx;
  lp->wordCount  --;


  /* if this is the last line, then open a new line after this one. */

  /*********
  if (lp->next == NULL) OpenNewLine();
  *********/

  wp->y           = lp->next->bbox.ascent - wp->bbox.ascent;
  wp->tabCount    = 0;
  wp->spaceCount  = 0;
  wp->linePtr     = lp->next;

  /* now shift horizontally all words in the line. */
  /*****
  if the a word's tabcount is < next word's tabcount, then the next char is a tab.
  do the same for the number of spaces...
  *****/

  /* now adjust the ascent or descent of the line if necessary */
  
}



/*----------------------------------------------------------------------*
 |                              Reflow                                  |
 | reflows all lines after the lineIndex so each line fits correctly    |
 | into the newly sized window.                                         |
 *----------------------------------------------------------------------*/
void Reflow (XmMultiTextWidget cw, LineList reflp)
{
  LineList lp;


  /* start reflowing after the current line unless lp is set to NULL. */
  /* in that case, reflow all lines.                                  */
  if (reflp == NULL)
    lp = cw->multiText.firstLine;
  else
    lp = reflp->next;

  for (; lp != NULL; lp = lp->next)
    {
      while (LineTooLong(cw, lp))
	WrapLastWordToNextLine(cw, lp);
    }
}



/*----------------------------------------------------------------------*
 |                        ShiftFollowingLines2                          |
 *----------------------------------------------------------------------*/
void ShiftFollowingLines2 (XmMultiTextWidget cw, LineList indexLine)
{
  LineList lp, startLine;

  if (indexLine == NULL)
    startLine = cw->multiText.firstLine;
  else
    startLine = indexLine;
  if (startLine == NULL) return;
  
  for (lp = startLine; (lp != NULL) && (lp->next != NULL); lp = lp->next)
    {
      lp->next->y = lp->y + lp->bbox.ascent + lp->bbox.descent + lp->lineSpacing;
    }
}


/*----------------------------------------------------------------------*
 |                        DeleteWordFromLine                            |
 *----------------------------------------------------------------------*/
void DeleteWordFromLine (XmMultiTextWidget cw, WordList wp)
{
  if (wp == NULL) return;


  if (wp->prev == NULL)
    wp->linePtr->firstWord = wp->next;
  else
    wp->prev->next = wp->next;

  if (wp->next == NULL)
    wp->linePtr->lastWord = wp->prev;
  else
    wp->next->prev = wp->prev;

  wp->linePtr->wordCount --;
  wp->linePtr->remaining += wp->bbox.width;

  FreeWordRecord(cw, wp);
}



/*----------------------------------------------------------------------*
 |                               Cut                                    |
 *----------------------------------------------------------------------*/
static void
Cut (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  LineList           lp;
  WordList           wp;


  /* go through all text and find the selected text. */
  /* this should probably not keep checking after the first chunk of selected text. */
  /* however, future versions may allow multiple selections. */

  for (lp = cw->multiText.firstLine; lp != NULL; lp = lp->next)
    for (wp = lp->firstWord; wp != NULL; wp = wp->next)
      if (wp->selected)
	DeleteWordFromLine(cw, wp);

  Reflow(cw, NULL);
}




/*----------------------------------------------------------------------*
 |                            GetWholeLink                              |
 *----------------------------------------------------------------------*/
static char *GetWholeLink (Widget w, WordList wp)
{
  WordList           wpStart, wpTemp;
  char              *buf = "\0";
  int                len = 0;;

  if (!wp->linkInfo) return(buf);

  wpStart = wp;
  while ((wpStart->prev != NULL) && (SameLinks(wpStart->prev, wp)))
    wpStart = wpStart->prev;


  /* scan forward in the line till the end of the line or the end of the line. */
  /* buf first find out how long this string is going to be.                   */

  wpTemp = wpStart;
  while ((wpTemp != NULL) && (SameLinks(wpTemp, wp)))
    {
      len += strlen(wpTemp->chars) + 1;
      wpTemp = wpTemp->next;
    }
  buf = (char *) XtMalloc(sizeof(char) * len + 1);
  strcpy(buf, "");
      
  while ((wpStart != NULL) && (SameLinks(wpStart, wp)))
    {
      if (strlen(buf))
	sprintf(buf,"%s %s", buf, wpStart->chars);
      else
	strcpy(buf, wpStart->chars);
      wpStart = wpStart->next;
    }

  return(buf);
}



/*----------------------------------------------------------------------*
 |                            DeselectAll                               |
 *----------------------------------------------------------------------*/
static void DeselectAll (XmMultiTextWidget cw)
{
  LineList lp;
  WordList wp;

  for (lp = cw->multiText.firstLine; lp != NULL; lp = lp->next)
    for (wp = lp->firstWord; wp != NULL; wp = wp->next)
      if (wp->selected)
	{
	  DrawWord(cw, wp, !wp->selected);
	  wp->selected = FALSE;
	}

  cw->multiText.textIsSelected = FALSE;
}



/*-----------------------=======================------------------------*
 |                       MoveTo action procedure                        |
 | This routine is invoked (default case) on a Btn1Down event occurs in |
 | the widget's window.  It then positions the cursor at the new        |
 | location.  Well, actually it does all sorts of things - none of which|
 | is permanent, so I really won't go into a full description here, yet.|
 *-----------------------=======================------------------------*/
static void MoveTo (Widget    w,
		    XEvent*   event,
		    String*   params,
		    Cardinal* numParams)
{
    XmMultiTextWidget              cw        = (XmMultiTextWidget) w;
    LineList                       lp        = cw->multiText.firstLine;
    LineList                       clp;
    int                            actualX;
    WordList                       wp;
    Boolean                        foundLine = FALSE;
    Boolean                        foundWord = FALSE;
    XmMultiTextLinkCallbackStruct  callValue;


  /* First scan all the lines until correct line is found. */
  while ((lp != NULL) && !foundLine)
    {
      foundLine = PtInLine(event->xbutton.x, event->xbutton.y, lp);
      if (!foundLine) lp = lp->next;
    }


  /* Now scan the words to find the word in this line */
  if (foundLine)
    {
      wp = lp->firstWord;

      while ((wp != NULL) && !foundWord)
	{
	  foundWord = PtInWord(event->xbutton.x, event->xbutton.y, wp, lp);
	  if (!foundWord) wp = wp->next;
	}

      /* Check if this is a Link.  If so, setup the callback. */
      if ((foundWord) && (wp->linkInfo != NULL) && (wp->linkInfo->linkType != NOOP))
	{
	  callValue.reason   = XmCR_LINK;
	  callValue.event    = event;
	  callValue.type     = wp->linkInfo->linkType;
	  callValue.posn     = NewString(wp->linkInfo->linkPosn);
	  callValue.data     = NewString(wp->linkInfo->linkData);
	  callValue.word     = GetWholeLink(w, wp);

	  if (XtHasCallbacks((Widget)cw, XmNlinkCallback) == XtCallbackHasSome)
	    {
	      XFlush(XtDisplay(w));
	      XtCallCallbacks((Widget)cw, XmNlinkCallback, &callValue);
	      XtFree(callValue.posn); XtFree(callValue.data); XtFree(callValue.word);
	      return;
	    }
	  XtFree(callValue.posn); XtFree(callValue.data); XtFree(callValue.word);
	}
      else
	DeselectAll((XmMultiTextWidget)cw);

      PositionCursor(cw, event->xbutton.x, event->xbutton.y, &clp, &actualX);
      UpdateCursor(cw, clp, event->xbutton.x, actualX, clp->y + clp->bbox.ascent);
    }
  else
    DeselectAll((XmMultiTextWidget)cw);


  /* now check for selections... */
  cw->multiText.selecting      = TRUE;
  
  /* set the widgets selection points. */
  cw->multiText.startX = event->xbutton.x;
  cw->multiText.startY = event->xbutton.y;

  /* that's all the setup that's necessary - actions take care of the rest. */
}




/*----------------------------------------------------------------------*
 |                             ChangeFocus                              |
 *----------------------------------------------------------------------*/
static void ChangeFocus(Widget w, XEvent* event, String* params, Cardinal* numParams)
{

}



/*----------------------------------------------------------------------*
 |                                Enter                                 |
 *----------------------------------------------------------------------*/
static void Enter (Widget w, XEvent* event, String* params, Cardinal* numParams)
{

}



/*----------------------------------------------------------------------*
 |                                Leave                                 |
 *----------------------------------------------------------------------*/
static void Leave (Widget w, XEvent* event, String* params, Cardinal* numParams)
{

}



/*----------------------------------------------------------------------*
 |                              InFocus                                 |
 *----------------------------------------------------------------------*/
static void InFocus (Widget w, XEvent* event, String* params, Cardinal* numParams)
{

}



/*----------------------------------------------------------------------*
 |                              OutFocus                                |
 *----------------------------------------------------------------------*/
static void OutFocus (Widget w, XEvent* event, String* params, Cardinal* numParams)
{

}




/*----------------------------------------------------------------------*
 |                             DeleteLine                               |
 *----------------------------------------------------------------------*/
static void DeleteLine (Widget w, XKeyPressedEvent *event, String *argv, Cardinal *argc)
{
  /* argv hold a single argument telling if the line should be deleted */
  /* from the TOP or the BOTTOM.                                       */

  switch (atoi(*argv))
    {
    case TOP:
      XmMultiTextDeleteLineTop(w, 1);
      break;

    case BOTTOM:
      XmMultiTextDeleteLineBottom(w);
      break;

    default: ;
    }
}




/*----------------------------------------------------------------------*
 |                              OpenLineTop                             |
 *----------------------------------------------------------------------*/
static void OpenLineTop (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextOpenLineTop (w, 0);
}




/*----------------------------------------------------------------------*
 |                              AddWordTop                              |
 *----------------------------------------------------------------------*/
static void AddWordTop (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  static int i=1;
  char *str="testing    ";

  sprintf(str, "testing%d", i++);
  XmMultiTextAppendWordTop (w, str, "hodges-24", "red", 0, NULL);
}




/*======================================================================*
 |                Xmmultitext Widget Public Procedures                  |
 |                ------------------------------------                  |
 | The(se) routine(s) are defined at the beginning of the file in the   |
 | list of actions.                                                     |
 *======================================================================*/


/*-----------------------====================---------------------------*
 |                       XmMultiTextClearText                           |
 *-----------------------====================---------------------------*/
void XmMultiTextClearText (Widget w)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  LineList           lp, lpNext;

  lp = cw->multiText.firstLine;

  while (lp != NULL) {
    lpNext = lp->next;
    FreeLineRecord(cw, lp);
    XtFree((char*)lp);
    lp = lpNext;
  }

  cw->multiText.firstLine   = NULL;
  cw->multiText.currentLine = NULL;
  cw->multiText.lastLine    = NULL;

  /* clean up all the cursor stuff... */
  if (cw->multiText.cursorFg != None)
    {
      XFreePixmap(XtDisplay(cw), cw->multiText.cursorFg);
      XFreePixmap(XtDisplay(cw), cw->multiText.cursorBg);
      XFreePixmap(XtDisplay(cw), cw->multiText.cursorMask);
    }
  if (cw->multiText.blinkTimeOutID != 0) XtRemoveTimeOut(cw->multiText.blinkTimeOutID);
  cw->multiText.blinkTimeOutID = 0;
  cw->multiText.selecting      = FALSE;
  cw->multiText.textIsSelected = FALSE;
  cw->multiText.cursorFg       = None;
  cw->multiText.cursorBg       = None;
  cw->multiText.cursorMask     = None;

  XClearArea(XtDisplay(cw), XtWindow(cw), 0, 0, 0, 0, TRUE);

  XSync(XtDisplay(cw), False);
  ChangeHeight((XmMultiTextWidget)w, XtParent(w)->core.height);
}



/*----------------------======================--------------------------*
 |                      XmMultiTextGetPosition                          |
 | Returns the last position of the data already entered.  This tells   |
 | where new data will be appended.                                     |
 *----------------------======================--------------------------*/
int XmMultiTextGetPosition (Widget w)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;


  if (cw->multiText.lastLine == NULL)
    return(0);
  else
    return(cw->multiText.lastLine->y);
}



/*-------------------============================-----------------------*
 |                   XmMultiTextGetCursorPosition                       |
 | Returns the last position of the data already entered.  This tells   |
 | where new data will be appended.                                     |
 *-------------------============================-----------------------*/
int XmMultiTextGetCursorPosition (Widget w)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;

  return(cw->multiText.cursorY);
}



/*-----------------------======================-------------------------*
 |                       XmMultiTextQueryCursor                         |
 *-----------------------======================-------------------------*/
void XmMultiTextQueryCursor (Widget w, int *lineNumber, int *y)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  LineList          lp;

  *lineNumber = 0;

  for (lp = cw->multiText.firstLine;  lp != NULL;  lp = lp->next)
    {
      if (lp->y + lp->bbox.ascent + lp->bbox.descent  >  cw->multiText.cursorY)
	break;
      *lineNumber = *lineNumber + 1;
    }

  if (lp == NULL)
    if (cw->multiText.lastLine != NULL)
      *y = cw->multiText.lastLine->y;
    else
      {
	*y          = 0;
	*lineNumber = 0;
      }
  else
    *y = lp->y;
}



/*-----------------------======================-------------------------*
 |                       XmMultiTextDeselectAll                         |
 *-----------------------======================-------------------------*/
void XmMultiTextDeselectAll (Widget w)
{
  XmMultiTextWidget   cw = (XmMultiTextWidget) w;

  if (XtIsRealized((Widget)cw))
    DeselectAll(cw);
}



/*----------------------======================--------------------------*
 |                      XmMultiTextSetTabStops                          |
 *----------------------======================--------------------------*/
void XmMultiTextSetTabStops (Widget w, int tabList[], int tabCount)
{
  XmMultiTextWidget   cw = (XmMultiTextWidget) w;
  int                 i;

  /* enter the tabs into the widget. */
  for (i=0; i<tabCount; i++)
    cw->multiText.tabs[i] = tabList[i];

  cw->multiText.tabCount = tabCount;
}




/*---------------------=========================------------------------*
 |                     XmMultiTextMakeLinkRecord                        |
 *---------------------=========================------------------------*/
LinkInfoPtr XmMultiTextMakeLinkRecord (Widget w, LinkType linkType, char *linkPosn, char *linkData)
{
  LinkInfoPtr li;

  li = (LinkInfoPtr) XtMalloc(sizeof(struct LinkRec));
  if (li == NULL) XtError(MSG3);
  else
    {
      li->linkType = linkType;
      li->linkPosn = NewString(linkPosn);
      li->linkData = NewString(linkData);
    }

  return(li);
}



/*---------------------========================-------------------------*
 |                     XmMultiTextAppendNewLine                         |
 | (future version...) a new line should be treated as a word and should|
 | be assigned a font as any other word.  This font should be used to   |
 | determine the height of the new line. If a new word is inserted into |
 | the new line that is shorter, the line shouldn't shrink to match the |
 | new height until the font of the newline is changed.                 |
 *---------------------========================-------------------------*/
void XmMultiTextAppendNewLine (Widget w, char *theWord, char *fontName, int indent)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  LineList           lp;
  XFontStruct       *font;
  int                direction, fontAscent, fontDescent, yposn;
  XCharStruct        boundingBox;


  /* Create a new line with the height of the current font.                           */

  /* there are two cases, either the line before has some words in it, or it doesn't. */
  /* if there are words in the previous line, use its height to figure the new posn.  */
  /* if there are no words, then calculate the new posn using the current font.       */

  /* Case 0 - No lines in widget yet. */
  if (cw->multiText.firstLine == NULL)
    {
      font = NamedFont(cw, fontName);
      if (!font)
      {
	return;
      }
      XTextExtents(font, theWord, strlen(theWord),
		   &direction, &fontAscent, &fontDescent, &boundingBox); 
      if (fontAscent  != boundingBox.ascent)  boundingBox.ascent  = fontAscent;
      if (fontDescent != boundingBox.descent) boundingBox.descent = fontDescent;
      yposn = cw->multiText.marginHeight;

      lp                        = NewLine(cw, indent, yposn, DEFAULT_LINE_SPACING);
      lp->bbox.ascent           = fontAscent;
      lp->bbox.descent          = fontDescent;
      cw->multiText.firstLine   = lp;
      cw->multiText.lastLine    = lp;
    }
  else


  /* Case 1 - Previous line has words in it already.                 */
  /* Use the previous word for height info - sorta logical, I guess. */
  if (cw->multiText.lastLine->firstWord != NULL)
    {
      yposn = cw->multiText.lastLine->y +
	      cw->multiText.lastLine->bbox.ascent + cw->multiText.lastLine->bbox.descent +
	      cw->multiText.lastLine->lineSpacing;

      lp = NewLine(cw, indent, yposn, cw->multiText.lastLine->lineSpacing);
      cw->multiText.lastLine->next = lp;
      lp->prev = cw->multiText.lastLine;
      cw->multiText.lastLine       = lp;

      font = NamedFont(cw, fontName);
      if (!font)
      {
	return;
      }
      XTextExtents(font, theWord, strlen(theWord), &direction, &fontAscent, &fontDescent,
		   &boundingBox);
      lp->bbox.ascent           = fontAscent;
      lp->bbox.descent          = fontDescent;
    }


  /* Case 2 - Previous line has no words in it - probably just a newline. */
  else
    {
      font = NamedFont(cw, fontName);
      if (!font)
      {
	return;
      }
      XTextExtents(font, theWord, strlen(theWord), &direction, &fontAscent, &fontDescent,
		   &boundingBox);
      if (fontAscent  != boundingBox.ascent)  boundingBox.ascent  = fontAscent;
      if (fontDescent != boundingBox.descent) boundingBox.descent = fontDescent;
      yposn = cw->multiText.lastLine->y +
	      fontAscent + fontDescent +
	      cw->multiText.lastLine->lineSpacing;

      lp = NewLine(cw, indent, yposn, cw->multiText.lastLine->lineSpacing);
      lp->bbox.ascent              = fontAscent;
      lp->bbox.descent             = fontDescent;
      cw->multiText.lastLine->next = lp;
      lp->prev = cw->multiText.lastLine;
      cw->multiText.lastLine       = lp;
    }

  cw->multiText.currentLine = cw->multiText.lastLine;  /* set this as the current line. */
  lp->newLine = TRUE;                          /* tag this newline as a 'hard' newline. */
}





/*-----------------------=====================--------------------------*
 |                       XmMultiTextAppendWord                          |
 | This is called from the outside world.  Words are appended to the    |
 | current cursor position and have the following associated traits:    |
 | Color, Font, Link.                                                   |
 |                                                                      |
 | From here, the word will be appended as a single word into the data- |
 | structure of the XmMultiTextWidget. Any justification and other text |
 | handling is not done here.                                           |
 |                                                                      |
 | This routine calls the internal routine StoreWord which enters the   |
 | new word structure to the current line.  StoreWord returns status    |
 | information depending on what occured when the new word was added to |
 | the line.                                                            |
 | A tab is entered as a complete word.  The next word shouldn't add    |
 | any wordspacing.                                                     |
 *-----------------------=====================--------------------------*/
unsigned short XmMultiTextAppendWord (Widget w, char *theWord, char *fontName, char *colorName,
				      int indent, LinkInfoPtr linkInfo)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  WordList           wordPtr;
  XFontStruct       *font;
  XCharStruct        boundingBox;
  unsigned short     status;


  font = NamedFont (cw, fontName);
  if (!font)
  {
    return 0;
  }

  /* get the bounding box for this word (or tab). */
  GetBoundingBox(cw, theWord, font, &boundingBox, indent);

  /* build the word record to add to the widget. */
  wordPtr                  = (WordList) XtMalloc(sizeof(struct WordRec));
  if (wordPtr == NULL) XtError(MSG6);
  wordPtr->charList        = NULL;
  wordPtr->bbox            = boundingBox;
  wordPtr->x               = 0;
  wordPtr->y               = 0;
  wordPtr->length          = strlen(theWord);
  wordPtr->chars           = NewString(theWord);
  wordPtr->font            = font;
  wordPtr->spacing         = 0;
  wordPtr->color           = Color (cw, colorName);
  wordPtr->linkInfo        = linkInfo;
  wordPtr->bbox.attributes = 0;
  wordPtr->image           = None;
  wordPtr->widget          = NULL;
  wordPtr->selected        = FALSE;
  wordPtr->linePtr         = NULL;
  wordPtr->next            = NULL;
  wordPtr->prev            = NULL;

  /* append the word structure on the widget's current line. */
  status = StoreWord(cw, wordPtr, indent);

  return (status);
}


/*---------------------========================-------------------------*
 |                     XmMultiTextAppendWordTop                         |
 | This routine adds a single word to the first line in the MultiText   |
 | widget.  It requires that there is a first line.  Additionally, word |
 | wrapping isn't yet supported and a warning is issued if the appended |
 | word is supposed to wrap.  In any case, the word will be appended as |
 | though word wrapping is off.                                         |
 | After the word is inserted, all lines after the current line must be |
 | updated to reflect the new height changes (if necessary).  The prev  |
 | pointer of the following line must also be changed.                  |
 *---------------------========================-------------------------*/
unsigned short XmMultiTextAppendWordTop (Widget w,  char *theWord, char *fontName,
						char *colorName, int indent, LinkInfoPtr linkInfo)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  WordList           wordPtr;
  XFontStruct       *font;
  XCharStruct        boundingBox;
  unsigned short     status;


  /* this routine won't work if there is no top-line to append the new word. */
  /* maybe later, I'll automatically create a new line in this case. But for */
  /* now, just return and don't append the new word.                         */
  if (cw->multiText.firstLine == NULL) {
    XtWarning(MSG9);
    return(APPEND_ERROR);
  }


  font = NamedFont (cw, fontName);
  if (!font)
  {
    return 0;
  }

  /* get the bounding box for this word (or tab). */
  GetBoundingBox(cw, theWord, font, &boundingBox, indent);

  /* build the word record to add to the widget. */
  wordPtr                  = (WordList) XtMalloc(sizeof(struct WordRec));
  if (wordPtr == NULL) XtError(MSG6);
  wordPtr->charList        = NULL;
  wordPtr->bbox            = boundingBox;
  wordPtr->x               = 0;
  wordPtr->y               = 0;
  wordPtr->length          = strlen(theWord);
  wordPtr->chars           = NewString(theWord);
  wordPtr->font            = font;
  wordPtr->spacing         = 0;
  wordPtr->color           = Color (cw, colorName);
  wordPtr->linkInfo        = linkInfo;
  wordPtr->bbox.attributes = 0;
  wordPtr->image           = None;
  wordPtr->widget          = NULL;
  wordPtr->selected        = FALSE;
  wordPtr->linePtr         = NULL;
  wordPtr->next            = NULL;
  wordPtr->prev            = NULL;

  /* append the word structure on the widget's first line. */
  status = AppendWordToTopLine(cw, wordPtr, indent);


  return (status);
}



/*-------------------============================-----------------------*
 |                   XmMultiTextAppendOpenLineTop                       |
 |                                                                      |
 | NOTE - THE NEW LINE IS EMPTY AND HAS NO ASCENT OR DESCENT. YOU MUST  |
 | PUT SOMETHING INTO IT OR IT WILL NOT SHOW UP.                        |
 |                                                                      |
 | (Future...) should handle cases with no lines in the widget yet.     |
 *-------------------============================-----------------------*/
void XmMultiTextOpenLineTop (Widget w, int indent)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  LineList           lp;



  lp = NewLine(cw, indent, cw->multiText.marginHeight, DEFAULT_LINE_SPACING);

  if (lp != NULL)
    {
      lp->next = cw->multiText.firstLine;
      if (lp->next != NULL) lp->next->prev = lp;
      cw->multiText.firstLine = lp;

      /* NOTE - THE ASCENT AND DESCENT OF THE LINE IS ZERO AT THIS POINT. */
    }
}




/*----------------------------------------------------------------------*
 |                            ScrollWindow                              |
 | Note that a negative dy value scrolls up and a positive scrolls down.|
 | This routine assumes that it is completely visible - ie must be a    |
 | child of a scrolled window (its working area).  In addition, the     |
 | scrolledWindow must be in "applicationDefined" mode.                 |
 *----------------------------------------------------------------------*/
static void ScrollWindow (XmMultiTextWidget cw, int dy)
{
  if (!cw->multiText.exposeOnly) TurnOffCursor(cw);

  /*  XFlush(XtDisplay(cw));        */
  /*  XSync(XtDisplay(cw), False);  */

  if (dy > 0)      /* SCROLL DOWN - shift the contents of the screen down. */
    {
      XCopyArea(XtDisplay(cw), XtWindow(cw), XtWindow(cw), cw->multiText.textGC,
		0,               cw->multiText.marginHeight,
		cw->core.width,  cw->core.height - 2*cw->multiText.marginHeight - dy,
		0,               cw->multiText.marginHeight + dy);

      /* draw the lines that fit into the uncovered region. */
      XClearArea(XtDisplay(cw), XtWindow(cw),
		 0,              0,
		 cw->core.width, cw->multiText.marginHeight + dy, FALSE);
      DoRedrawText(cw, cw->multiText.marginHeight-dy, 2*dy);
    }
  else

  if (dy < 0)      /* SCROLL UP - shift the contents of the screen up. */
    {
      XCopyArea(XtDisplay(cw), XtWindow(cw), XtWindow(cw), cw->multiText.textGC,
		0,              cw->multiText.marginHeight + (-dy),
		cw->core.width, cw->core.height - 2*cw->multiText.marginHeight - (-dy),
		0,              cw->multiText.marginHeight);

      XClearArea(XtDisplay(cw), XtWindow(cw),
		 0,              cw->core.height - cw->multiText.marginHeight - (-dy),
		 cw->core.width, (-dy),  FALSE);
      DoRedrawText(cw, cw->core.height - cw->multiText.marginHeight - (-dy), (-dy));
    }

  if (!cw->multiText.exposeOnly) TurnOnCursor(cw);
}



/*---------------------========================-------------------------*
 |                     XmMultiTextDeleteLineTop                         |
 *---------------------========================-------------------------*/
int XmMultiTextDeleteLineTop (Widget w, int linesToDelete)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  int                dy, i;
  LineList           lp = cw->multiText.firstLine;


  if ((cw->multiText.firstLine == NULL) || (linesToDelete <= 0)) return (0);

  for (i=0; (i<linesToDelete) && (lp != NULL); i++)
    {
      lp = cw->multiText.firstLine;

      cw->multiText.firstLine = lp->next;
      if (lp->next != NULL) lp->next->prev = NULL;

      /* find how much to shift all the lines... note, line's know about the margin height. */
      dy = cw->multiText.marginHeight - cw->multiText.firstLine->y;

      ShiftFollowingLines (lp, dy);

      if (cw->multiText.currentLine == lp)
	if (cw->multiText.firstLine->wordCount > 0)
	  UpdateCursor(cw, cw->multiText.firstLine,
		       cw->multiText.firstLine->x + cw->multiText.firstLine->firstWord->x,
		       cw->multiText.firstLine->x + cw->multiText.firstLine->firstWord->x,
		       cw->multiText.firstLine->y + cw->multiText.firstLine->bbox.ascent);
	else
	  UpdateCursor(cw, cw->multiText.firstLine,
		       cw->multiText.firstLine->x,
		       cw->multiText.firstLine->x,
		       cw->multiText.firstLine->y + cw->multiText.firstLine->bbox.ascent);
      else
	cw->multiText.cursorY += dy;

      if (!cw->multiText.exposeOnly)
	if (cw->multiText.smoothScroll) ScrollWindow(cw, dy);

      /* if you want to automatically update the widget's height, add that here... */
      /*
      XtSetArg(args[0], XmNheight, cw->core.height + dy);
      XtSetValues(cw, args, 1);
      */

      /* now free the memory held by the line (lp). */
      FreeLineRecord (cw, lp);
    }

  if (!cw->multiText.exposeOnly)
    if (!cw->multiText.smoothScroll)
      XClearArea(XtDisplay(cw), XtWindow(cw), 0, 0, 0, 0, TRUE);

  return(i);
}



/*-------------------===========================------------------------*
 |                   XmMultiTextDeleteLineBottom                        |
 *-------------------===========================------------------------*/
void XmMultiTextDeleteLineBottom (Widget w)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  LineList           lp = cw->multiText.firstLine;


  lp = cw->multiText.lastLine;
  if (lp != NULL)
    {
      cw->multiText.lastLine = lp->prev;
      if (lp->prev != NULL) lp->prev->next = NULL;

      /* if you want to automatically update the widget's height, add that here... */
      /*
	dy = lp->bbox.ascent + lp->bbox.descent + lp->lineSpacing;
	XtSetArg(args[0], XmNheight, cw->core.height - dy);
	XtSetValues(cw, args, 1);
      */

      /* now free the memory held by the line (lp). */
      FreeLineRecord (cw, lp);
    }
}




/*------------------============================------------------------*
 |                  XmMultiTextLongestLineLength                        |
 *------------------============================------------------------*/
int XmMultiTextLongestLineLength (Widget w)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  LineList           lp;
  int                widestWidth = 0;

  for (lp = cw->multiText.firstLine; lp != NULL; lp = lp->next)
    if (widestWidth  <  (lp->x + lp->bbox.width) )
      widestWidth = lp->x + lp->bbox.width;

  return (widestWidth);
}





/*-----------------------======================-------------------------*
 |                       XmMultiTextAppendImage                         |
 *-----------------------======================-------------------------*/
unsigned short XmMultiTextAppendImage (Widget w, char *theWord, char *fontName, char *colorName,
				       int indent, Pixmap image, unsigned int width,
				       unsigned int height, LinkInfoPtr linkInfo)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  WordList           wordPtr;
  XFontStruct       *font;
  XCharStruct        boundingBox;

  font = NamedFont (cw, fontName);
  if (!font)
  {
    return 0;
  }

  /* get the bounding box for this word (or tab). */
  GetBoundingBox(cw, theWord, font, &boundingBox, indent);

  /* build the word record to add to the widget. */
  wordPtr                  = (WordList) XtMalloc(sizeof(struct WordRec));
  if (wordPtr == NULL) XtError(MSG6);
  wordPtr->charList        = NULL;
  wordPtr->x               = 0;
  wordPtr->y               = 0;
  wordPtr->length          = strlen(theWord);    /* doesn't really make sense for images...  */
  wordPtr->chars           = NewString(theWord); /* 'theWord' is value returned if selected. */
  wordPtr->font            = font;
  wordPtr->spacing         = 0;
  wordPtr->color           = Color (cw, colorName);
  wordPtr->linkInfo        = linkInfo;
  wordPtr->selected        = FALSE;
  wordPtr->linePtr         = NULL;
  wordPtr->next            = NULL;
  wordPtr->prev            = NULL;

  /* now setup the image information. */
  wordPtr->image           = image;
  wordPtr->widget          = NULL;
  wordPtr->bbox.lbearing   = 0;
  wordPtr->bbox.rbearing   = width;
  wordPtr->bbox.width      = width;
  wordPtr->bbox.ascent     = (height+1) / 2;
  wordPtr->bbox.descent    = height / 2;
  wordPtr->bbox.attributes = 0;


  /* append the word structure on the widget's current line. */
  StoreWord(cw, wordPtr, indent);

  return (1);
}



/*-----------------------=======================------------------------*
 |                       XmMultiTextAppendWidget                        |
 *-----------------------=======================------------------------*/
unsigned short XmMultiTextAppendWidget (Widget w, char *theWord, char *fontName, char *colorName,
					int indent, Widget newW)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  WordList           wordPtr;
  XFontStruct       *font;
  XCharStruct        boundingBox;
  unsigned short     status;
  int                n, width, height;
  Arg                args[5];
  int                xoffset;   /* line's indent + margin width */
  int                yposn;     /* top of current line          */


  font = NamedFont (cw, fontName);
  if (!font)
  {
    return 0;
  }

  /* get the bounding box for this word (or tab). */
  GetBoundingBox(cw, theWord, font, &boundingBox, indent);

  /* build the word record to add to the widget. */
  wordPtr                  = (WordList) XtMalloc(sizeof(struct WordRec));
  if (wordPtr == NULL) XtError(MSG6);
  wordPtr->charList        = NULL;
  wordPtr->x               = 0;
  wordPtr->y               = 0;
  wordPtr->length          = strlen(theWord);    /* doesn't really make sense for widgets... */
  wordPtr->chars           = NewString(theWord); /* 'theWord' is value returned if selected. */
  wordPtr->font            = font;
  wordPtr->spacing         = 0;
  wordPtr->color           = Color (cw, colorName);
  wordPtr->linkInfo        = NULL;               /* can't have a widget as a link... */
  wordPtr->selected        = FALSE;
  wordPtr->linePtr         = NULL;
  wordPtr->next            = NULL;
  wordPtr->prev            = NULL;

  /* find the size of the widget */
  n = 0;
  XtSetArg(args[n], XmNwidth,  &width);   n++;
  XtSetArg(args[n], XmNheight, &height);  n++;
  XtGetValues(newW, args, n);

  /* now setup the image information. */
  wordPtr->image           = None;
  wordPtr->widget          = newW;
  wordPtr->bbox.lbearing   = 0;
  wordPtr->bbox.rbearing   = width;
  wordPtr->bbox.width      = width;
  wordPtr->bbox.ascent     = (height+1) / 2;
  wordPtr->bbox.descent    = height / 2;
  wordPtr->bbox.attributes = 0;


  /* append the word structure on the widget's current line. */
  status = StoreWord(cw, wordPtr, indent);


  xoffset = wordPtr->linePtr->x;
  yposn   = wordPtr->linePtr->y + wordPtr->linePtr->bbox.ascent;

  XtMoveWidget(newW, xoffset + wordPtr->x, yposn - wordPtr->bbox.ascent);
  XtManageChild(newW);

  return (status);
}




/*-----------------------====================---------------------------*
 |                       XmMultiTextAppendDPS                           |
 *-----------------------====================---------------------------*/
unsigned short XmMultiTextAppendDPS (Widget w, char *theWord, char *fontName, char *colorName,
				     int indent, char *dpsFileName, unsigned int width,
				     unsigned int height, LinkInfoPtr linkInfo)
{
  unsigned short     status;


#ifdef NODPS
  /* if the systems doesn't support DPS then just replace it with a substitute string. */
  status = XmMultiTextAppendWord (w, "Graphics omitted from Online Documentation, please see the manual", fontName, colorName, indent, linkInfo);

#else
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  Pixmap             image;

  if (cw->multiText.dpsCapable)
    {
      image = GetDPSImage(cw, dpsFileName, &width, &height);
      /* append the word structure on the widget's current line.          */
      /* Note: 'theWord' is the value returned when an image is selected. */
      status = XmMultiTextAppendImage (w, theWord, fontName, colorName, indent,
				       image, width, height, linkInfo);
    }
  else
    {
      /* if the systems doesn't support DPS then just replace it with a substitute string. */
      status = XmMultiTextAppendWord (w, "Graphics omitted from Online Documentation, please see the manual", fontName, colorName, indent, linkInfo);
    }
}
#endif

  return (status != APPEND_ERROR);
}



/*-----------------------=====================--------------------------*
 |                       XmMultiTextAppendLine                          |
 *-----------------------=====================--------------------------*/
Boolean XmMultiTextAppendLine (Widget w, char *str)
{
  return True;
}


/*-----------------------=====================--------------------------*
 |                       XmMultiTextAppendChar                          |
 | add a character to the current word.  This will cause the current    |
 | word to redisplay.  Perhapse the best method is to just show the     |
 | character unless the character has a different format than the other |
 | letters.  How to manage that is still a mystery...                   |
 *-----------------------=====================--------------------------*/
Boolean XmMultiTextAppendChar (Widget w, char ch)
{
  XmMultiTextWidget  cw = (XmMultiTextWidget) w;
  WordList           wp;
  char              *newWord;


  if ((cw->multiText.firstLine == NULL) || (cw->multiText.lastLine->firstWord == NULL))
    XtWarning("Sorry - this isn't supported yet.");

  else
    {
      /* stuff this character onto the current word. */
      wp = cw->multiText.lastLine->lastWord;
      wp->length++;
      newWord = (char *) XtMalloc(sizeof(char) * wp->length + 2);
      if (newWord == NULL) XtError(MSG6);
      sprintf(newWord, "%s%c", wp->chars, ch);
      XtFree(wp->chars);
      wp->chars = newWord;

      GetBoundingBox(cw, newWord, wp->font, &wp->bbox, cw->multiText.lastLine->indent);

      XClearArea(XtDisplay(w), XtWindow(w),
		 wp->x + cw->multiText.lastLine->x, wp->y + cw->multiText.lastLine->y,
		 wp->bbox.width, wp->bbox.ascent + wp->bbox.descent, TRUE);
    }

  return True;
}





/*======================================================================*
 |               XmMultiText Widget Support Procedures                  |
 |               -------------------------------------                  |
 *======================================================================*/

/*----------------------------------------------------------------------*
 |                              NewString                               |
 *----------------------------------------------------------------------*/
static char *NewString(char *str)
{
  char *retStr;

  retStr = (char *) XtMalloc(sizeof(char) * (str ? strlen(str) : 0) + 1);
  if (retStr == NULL) XtError(MSG6);

  if (str == NULL)
    retStr[0] = '\0';
  else
    strcpy(retStr, str);

  return(retStr);
}


/*----------------------------------------------------------------------*
 |                            ChangeHeight                              |
 *----------------------------------------------------------------------*/
static XtGeometryResult ChangeHeight (XmMultiTextWidget w, Dimension newHeight)
{
  XtWidgetGeometry  request, replyReturn;
  XtGeometryResult  result;

  /* build the request */
  request.request_mode = CWHeight;
  request.height       = newHeight;

  result = XtMakeGeometryRequest((Widget)w, &request, &replyReturn);

  return (result);
}



/*----------------------------------------------------------------------*
 |                           GetBoundingBox                             |
 | sets the bounding box of the word.                                   |
 *----------------------------------------------------------------------*/
static void GetBoundingBox(XmMultiTextWidget cw, char *theWord, XFontStruct *font,
			   XCharStruct *boundingBox, int indent)
{
  int direction, fontAscent, fontDescent, tabWidth;

  /* if the word is a tab, must build bbox from scratch. */
  if (theWord[0] == TAB)
    {
      /* calculate the bounding box depending on the current tab settings. */
      /*tabPos =*/ NextTab(cw, &tabWidth, indent);
      boundingBox->lbearing   = 0;
      boundingBox->rbearing   = tabWidth;
      boundingBox->width      = tabWidth;
      boundingBox->ascent     = 0;
      boundingBox->descent    = 0;
      boundingBox->attributes = 0;
    }
  else
    {
      XTextExtents(font, theWord, strlen(theWord), &direction, &fontAscent, &fontDescent,
		   boundingBox);
      boundingBox->ascent  = fontAscent;
      boundingBox->descent = fontDescent;  /* make sure bounding box uses font's ascent,descent */
    }
}


/*----------------------------------------------------------------------*
 |                          ShiftFollowingLines                         |
 *----------------------------------------------------------------------*/
static void ShiftFollowingLines (LineList lines, int dy)
{
  LineList lp;
  WordList wp;


  for (lp = lines;  lp != NULL;  lp = lp->next)
    {
      /* it shouldn't be necessary to adjust any of the words in the line */
      /* since they are relative to the line's position.                  */

      lp->y += dy;

      /* if the line has widgets - these may need moving.               */
      /* though it should probably be handled on redraw, it is faster   */
      /* if I handle it here.  That way the checking is done only once. */

      /* Scan though all words in the line and check if it's a widget.  */
      /* If so, move it vertically by dy.                               */

      for (wp = lp->firstWord;  wp != NULL;  wp = wp->next)
	if (wp->widget != NULL)
	  XtMoveWidget(wp->widget, wp->widget->core.x, dy + wp->widget->core.y);
    }
}



/*----------------------------------------------------------------------*
 |                               PtInLine                               |
 *----------------------------------------------------------------------*/
static Boolean PtInLine (int x, int y, LineList lp)
{
  if (lp == NULL) return FALSE;

  if ((y >= lp->y) && (y < lp->y + lp->bbox.ascent + lp->bbox.descent))
    return TRUE;
  else
    return FALSE;
}



/*----------------------------------------------------------------------*
 |                         CalculateSpacing                             |
 *----------------------------------------------------------------------*/
static void CalculateSpacing (Widget w, WordList wp, int *spaceBefore, int *spaceAfter)
{
  *spaceBefore = 0;   /* problems will occur if these aren't initialized. */
  *spaceAfter  = 0;

  if (wp->prev == NULL)
    *spaceBefore = 0;
  else
    *spaceBefore = (int)((wp->x - (wp->prev->x + wp->prev->bbox.width)) / 2);

  if (wp->next == NULL)
    *spaceAfter = 0;
  else
    *spaceAfter = (int)((wp->next->x - (wp->x + wp->bbox.width)) / 2 + 1);
}



/*----------------------------------------------------------------------*
 |                             SameLinks                                |
 | returns true if the links are the same, and false otherwise.         |
 *----------------------------------------------------------------------*/
static Boolean SameLinks (WordList wp1, WordList wp2)
{
  if ((wp1 == NULL) || (wp2 == NULL)) return(FALSE);
     else
  if ((wp1->linkInfo == NULL) || ((wp2->linkInfo == NULL))) return (FALSE);
     else
  if (wp1->linkInfo->linkType != wp2->linkInfo->linkType) return(FALSE);
     else
  if (STRCMP(wp1->linkInfo->linkPosn, wp2->linkInfo->linkPosn)) return(FALSE);
     else
  return(!STRCMP(wp1->linkInfo->linkData, wp2->linkInfo->linkData));
}


/*----------------------------------------------------------------------*
 |                           SetCurrentFont                             |
 *----------------------------------------------------------------------*/
static void SetCurrentFont (XmMultiTextWidget w, WordList wp, XFontStruct *font)
{
  /* to speed things up - only set the font if needed. */
  if (wp->font->fid != w->multiText.currentFontID)
    {
      XSetFont(XtDisplay(w), w->multiText.textGC, wp->font->fid);
      w->multiText.currentFontID = wp->font->fid;
    }
}


/*----------------------------------------------------------------------*
 |                           SetCurrentColor                            |
 *----------------------------------------------------------------------*/
static void SetCurrentColor (XmMultiTextWidget w, WordList wp, unsigned long color)
{
  /* to speed things up - only set the color if needed. */
  if (wp->color != w->multiText.currentColor)
    {
      XSetForeground(XtDisplay(w), w->multiText.textGC, wp->color);
      w->multiText.currentColor = wp->color;
    }
}


/*----------------------------------------------------------------------*
 |                            DisplayImage                              |
 *----------------------------------------------------------------------*/
static void DisplayImage (XmMultiTextWidget w, WordList wp)
{
  int xoffset = wp->linePtr->x;				   /* line's indent + margine width */
  int yposn   = wp->linePtr->y + wp->linePtr->bbox.ascent; /* top of the current line.      */


  XCopyArea(XtDisplay(w), wp->image, XtWindow(w), w->multiText.textGC,
	    0, 0, wp->bbox.width, wp->bbox.ascent + wp->bbox.descent,
	    xoffset + wp->x, yposn - wp->bbox.ascent);
}


/*----------------------------------------------------------------------*
 |                            DisplayWord                               |
 | (future...) to fix the problem of jaged selection, erase the full    |
 | lineheight before drawing the string.                                |
 *----------------------------------------------------------------------*/
static void DisplayWord (XmMultiTextWidget w, WordList wp)
{
  int xoffset = wp->linePtr->x;				   /* line's indent + margine width */
  int yposn   = wp->linePtr->y + wp->linePtr->bbox.ascent; /* baseline position             */
                                                           /* X always draws strings at the */
                                                           /* baseline, not upperleft corner*/
  SetCurrentFont(w, wp, wp->font);
  SetCurrentColor(w, wp, wp->color);

  if (wp->image != None)
    DisplayImage (w, wp);
    else
  if (wp->widget != NULL)
    /* do nothing */;
  else
    XDrawString(XtDisplay(w), XtWindow(w), w->multiText.textGC, wp->x + xoffset,
		yposn, wp->chars, wp->length);
}



/*----------------------------------------------------------------------*
 |                        DisplaySelectedWord                           |
 | (future...) to fix the problem of jaged selection, fill the full line|
 | height on selections, not only the word's bounding box.              |
 | The selection region should eventually be the same as the fill rgn.  |
 *----------------------------------------------------------------------*/
static void DisplaySelectedWord (XmMultiTextWidget w, WordList wp)
{
  Pixel fg      = wp->color;
  Pixel bg      = w->core.background_pixel;
  int   spaceBefore, spaceAfter;
  int   xoffset = wp->linePtr->x;			     /* line's indent + margine width */
  int   yposn   = wp->linePtr->y + wp->linePtr->bbox.ascent; /* baseline position             */
                                                             /* X always draws strings at the */
                                                             /* baseline, not upperleft corner*/
  SetCurrentFont(w, wp, wp->font);
  SetCurrentColor(w, wp, wp->color);

  CalculateSpacing((Widget)w, wp, &spaceBefore, &spaceAfter);

  /* clear the area first. */
  XClearArea(XtDisplay(w), XtWindow(w),
	     xoffset + wp->x - spaceBefore, yposn - wp->bbox.ascent,
	     wp->bbox.width + spaceBefore + spaceAfter,
	     wp->bbox.ascent + wp->bbox.descent, FALSE); 

  if (wp->image != None)
    DisplayImage (w, wp);
    else
  if (wp->widget != NULL)
    /* do nothing */;
  else
    XDrawString(XtDisplay(w), XtWindow(w), w->multiText.textGC, wp->x + xoffset,
		yposn, wp->chars, wp->length);

  /* draw an xor box at the mouse position                          */
  /* should use... ->primitive.foreground ^ ->core.background_pixel */

  XSetForeground(XtDisplay(w), w->multiText.textGC, WhitePixelOfScreen(XtScreen(w)));
  XSetBackground(XtDisplay(w), w->multiText.textGC, BlackPixelOfScreen(XtScreen(w)));
  XSetFunction  (XtDisplay(w), w->multiText.textGC, GXxor);

  XFillRectangle(XtDisplay(w), XtWindow(w), w->multiText.textGC,
		 xoffset + wp->x - spaceBefore,
		 yposn - wp->bbox.ascent,
		 wp->bbox.width + spaceBefore + spaceAfter,
		 wp->bbox.ascent + wp->bbox.descent);

  XSetFunction  (XtDisplay(w), w->multiText.textGC, GXcopy);
  XSetForeground(XtDisplay(w), w->multiText.textGC, fg);
  XSetBackground(XtDisplay(w), w->multiText.textGC, bg);
}


/*----------------------------------------------------------------------*
 |                       DisplayDeselectedWord                          |
 | fix for selections goes here too...                                  |
 *----------------------------------------------------------------------*/
static void DisplayDeselectedWord (XmMultiTextWidget w, WordList wp)
{
  int   spaceBefore, spaceAfter;
  int   xoffset = wp->linePtr->x;			     /* line's indent + margine width */
  int   yposn   = wp->linePtr->y + wp->linePtr->bbox.ascent; /* baseline position             */
                                                             /* X always draws strings at the */
                                                             /* baseline, not upperleft corner*/
  SetCurrentFont(w, wp, wp->font);
  SetCurrentColor(w, wp, wp->color);

  CalculateSpacing((Widget)w, wp, &spaceBefore, &spaceAfter);

  XClearArea(XtDisplay(w), XtWindow(w),
	     xoffset + wp->x - spaceBefore, yposn - wp->bbox.ascent,
	     wp->bbox.width + spaceBefore + spaceAfter,
	     wp->bbox.ascent + wp->bbox.descent, FALSE); 

  if (wp->image != None)
    DisplayImage (w, wp);
    else
  if (wp->widget != NULL)
    {
      XtUnmapWidget(wp->widget);
      XtMapWidget(wp->widget);
    }
  else
    XDrawString(XtDisplay(w), XtWindow(w), w->multiText.textGC, wp->x + xoffset,
		yposn, wp->chars, wp->length);
}



/*----------------------------------------------------------------------*
 |                          DrawLinkMarking                             |
 | this routine draws the rectangle around a link.                      |
 *----------------------------------------------------------------------*/
static void DrawLinkMarking (XmMultiTextWidget w, WordList wp)
{
  int x[4], y[4];
  int xoffset = wp->linePtr->x;				   /* line's indent + margine width */
  int yposn   = wp->linePtr->y + wp->linePtr->bbox.ascent; /* baseline position             */
                                                           /* X always draws strings at the */
                                                           /* baseline, not upperleft corner*/
  /* don't close the box if the next word is the same link */

  if (SameLinks(wp->next, wp))
    x[0] = xoffset + wp->next->x;
  else
    x[0] = xoffset + wp->x + wp->bbox.width;

  y[0] = yposn - wp->linePtr->bbox.ascent -1; 
  x[1] = xoffset + wp->x;           y[1] = y[0];
  x[2] = x[1];                      y[2] = yposn + wp->linePtr->bbox.descent;
  x[3] = x[0];                      y[3] = y[2];

  XDrawLine(XtDisplay(w), XtWindow(w), w->multiText.textGC, x[0], y[0], x[1], y[1]);
  XDrawLine(XtDisplay(w), XtWindow(w), w->multiText.textGC, x[2], y[2], x[3], y[3]);
  if (!SameLinks(wp->next, wp)) XDrawLine(XtDisplay(w), XtWindow(w), w->multiText.textGC,
					  x[3], y[3], x[0], y[0]);
  if (!SameLinks(wp, wp->prev)) XDrawLine(XtDisplay(w), XtWindow(w), w->multiText.textGC,
					  x[1], y[1], x[2], y[2]);
}



/*----------------------------------------------------------------------*
 |                             DrawWord                                 |
 | xoffset is the left edge of the line.  yposn is the baseline of the  |
 | line for this word.                                                  |
 | the boolean 'selected' tells if the word becomes selected or not.    |
 | if it has the same value as the word's internal value, then don't    |
 | change it; ie: if it is not selected and 'selected' is false, then   |
 | don't bother clearing the word's bounding box before drawing it.     |
 *----------------------------------------------------------------------*/
static void DrawWord (XmMultiTextWidget w, WordList wp, Boolean selected)
{

  /* this routine just calls the appropriate version of the drawing */
  /* routine depending on the type and style of the word.           */

  /* if this is a tab, return */
  if (wp->chars[0] == TAB) return;

  /* if you don't want the image to be highlighted when selected, uncomment these lines. */
  /*
  if (wp->image != NULL) DisplayImage(w, wp);
  else
  */


  /* if the word isn't selected and isn't becomming selected, draw normally */
  if (!wp->selected && !selected) DisplayWord(w, wp);
  else

  /* if the word isn't selected and it should become selected, draw it selected */
  if (!wp->selected && selected) DisplaySelectedWord(w, wp);
  else

  /* if the word is selected and should become deselected, unselect it. */
  if (wp->selected && !selected) DisplayDeselectedWord(w, wp);
  else

  /* and finally, if the word was selected and still is, draw it selected */
  if (wp->selected && selected) DisplaySelectedWord(w, wp);
  else

  /* the above cases should cover all possibilities! */
  XtError("DrawWord: detected inconsistency");


  /* now handle links.  Do whatever drawing is required */
  /* to show that this word is a link if needed.        */
  if ((wp->linkInfo != NULL) && (wp->linkInfo->linkType != NOOP))
    DrawLinkMarking(w, wp);
}



/*----------------------------------------------------------------------*
 |                             DrawLine                                 |
 | call DrawWord for each word in the line.                             |
 *----------------------------------------------------------------------*/
static void DrawLine (XmMultiTextWidget w, LineList lp)
{
  WordList wp;
  int      i;

  if (lp != NULL)
    {
      wp = lp->firstWord;

      for (i = 0; i < lp->wordCount; i++)
	{
	  DrawWord(w, wp, wp->selected);
	  wp = wp->next;
	}
    }
}




/*----------------------------------------------------------------------*
 |                           ResizeTheLine                              |
 | this routine will move each word in the line to the correct baseline |
 | position corresponding to its line.                                  |
 | theWord is can be passed to this routine to enable a line to be re-  |
 | sized before theWord is appended.  If you just want to resize the    |
 | line, pass theWord as NULL.                                          |
 *----------------------------------------------------------------------*/
static void ResizeTheLine (XmMultiTextWidget w, LineList lp, WordList theWord)
{
  WordList  wp;
  Dimension newHeight;
  int       xoffset, yposn, maxAscent, maxDescent;


  if (lp      == NULL) return;
  if (theWord == NULL) theWord = lp->firstWord;
  if (theWord == NULL)
    {
      lp->bbox.ascent  = 0;
      lp->bbox.descent = 0;
      return;
    }


  /* know that theWord points to a valid word - may be first in line or new word. */

  maxAscent  = theWord->bbox.ascent;
  maxDescent = theWord->bbox.descent;
  for (wp = lp->firstWord;  wp != NULL;  wp = wp->next)
    {
      maxAscent  = MAX(maxAscent,  wp->bbox.ascent);
      maxDescent = MAX(maxDescent, wp->bbox.descent);
    }
  lp->bbox.ascent  = maxAscent;
  lp->bbox.descent = maxDescent;


  wp = lp->firstWord;
  while (wp != NULL)
    {
      wp->y = lp->bbox.ascent - wp->bbox.ascent;

      /* check if this is a widget, then move the widget. */
      if (wp->widget != NULL)
	{
	  xoffset = wp->linePtr->x;
	  yposn   = wp->linePtr->y + wp->linePtr->bbox.ascent;
	  XtMoveWidget(wp->widget, xoffset + wp->x, yposn - wp->bbox.ascent);
	}

      wp = wp->next;
    }

  /* if this line is below the bottom of the widget, then resize the widget to fit. */
  newHeight = lp->y + lp->bbox.ascent + lp->bbox.descent;
  if (w->core.height < newHeight + w->multiText.marginHeight)
    ChangeHeight(w, newHeight + (XtParent(w)->core.height/2));
}




/*----------------------------------------------------------------------*
 |                            FreeCharRecord                            |
 *----------------------------------------------------------------------*/
static void FreeCharRecord (XmMultiTextWidget w, CharList cp)
{
  XtError("MultiText FreeCharRecord:  charlist not initialized correctly");
}



/*----------------------------------------------------------------------*
 |                            FreeWordRecord                            |
 *----------------------------------------------------------------------*/
static void FreeWordRecord (XmMultiTextWidget w, WordList wp)
{
  CharList cp, cpNext;

  cp = wp->charList;

  while (cp != NULL) {
    cpNext = cp->next;
    FreeCharRecord(w, cp);
    XtFree((char*)cp);
    cp = cpNext;
  }

  if (wp->image    != None)  XFreePixmap(XtDisplay(w), wp->image);
  if (wp->widget   != NULL)  XtDestroyWidget(wp->widget);
  if (wp->chars    != NULL)  XtFree(wp->chars);
  if (wp->linkInfo != NULL)
    {
      XtFree(wp->linkInfo->linkPosn);
      XtFree(wp->linkInfo->linkData);
      XtFree((char*)wp->linkInfo);
    }
  /* if (wp->font     != NULL)  XtFree(wp->font);   -  Don't even think of doing this! */
}



/*----------------------------------------------------------------------*
 |                            FreeLineRecord                            |
 *----------------------------------------------------------------------*/
static void FreeLineRecord (XmMultiTextWidget w, LineList lp)
{
  WordList wp, wpNext;

  wp = lp->firstWord;

  while (wp != NULL) {
    wpNext = wp->next;
    FreeWordRecord(w, wp);
    XtFree((char*)wp);
    wp = wpNext;
  }
}



/*----------------------------------------------------------------------*
 |                               NextTab                                |
 *----------------------------------------------------------------------*/
static int NextTab (XmMultiTextWidget w, int *tabWidth, int indent)
{
  int i, nextTabPos, currentPos;

  /* first find out where we are. */
  if ((w->multiText.lastLine == NULL) || (w->multiText.lastLine->lastWord == NULL))
    currentPos = indent;
  else
    currentPos = w->multiText.lastLine->lastWord->x + w->multiText.lastLine->lastWord->bbox.width;

  /* there are no tabs in this widget.  nextTabPos is current posn. */
  if (w->multiText.tabCount == 0)
    {
      *tabWidth  = 0;
      nextTabPos = currentPos;
    }
  else
    {
      /* must hunt for the correct tab... */
      for (i=0; ((currentPos > w->multiText.tabs[i]) && (i < w->multiText.tabCount)); i++);

      /* if there was no tab after the current posn, ignore the tab. */
      if ((i == w->multiText.tabCount) && (currentPos > w->multiText.tabs[i]))
	nextTabPos = currentPos;
      else
	nextTabPos = w->multiText.tabs[i];
      *tabWidth  = w->multiText.tabs[i] - currentPos;
    }

  return (nextTabPos);
}


/*----------------------------------------------------------------------*
 |                                Color                                 |
 | Note this will only lookup a new color if the name is different than |
 | the last one.                                                        |
 *----------------------------------------------------------------------*/
static unsigned long Color (XmMultiTextWidget w, char *name)
{
  Colormap      cmap;

  if (STRCMP(w->multiText.lastColorName, name) != 0)
    {
      cmap                     = DefaultColormap(XtDisplay(w), XScreenNumberOfScreen(XtScreen(w)));
      w->multiText.color.pixel = (unsigned long)0;
      XParseColor(XtDisplay(w), cmap, name, &w->multiText.color);
      XAllocColor(XtDisplay(w), cmap, &w->multiText.color);

      if (w->multiText.lastColorName != NULL) XtFree(w->multiText.lastColorName);
      w->multiText.lastColorName = NewString(name);
    }

  return (w->multiText.color.pixel);
}


/*----------------------------------------------------------------------*
 |                               NamedFont                              |
 | Note this will only lookup a new font if the name is different than  |
 | the last one.                                                        |
 *----------------------------------------------------------------------*/
static XFontStruct *NamedFont (XmMultiTextWidget w, char *name)
{
  int  width;
  char buf[MAX_STR_LEN];
  int  i=0;


  for (i = 0; i < w->multiText.numFonts; ++i)
  {
    if (STRCMP(w->multiText.fontCache[i].name, name) == 0)
      {
	if (w->multiText.smartSpacing)
	  {
	    width = XTextWidth(w->multiText.fontCache[i].font, " ", 1);
	    if (width != 0) w->multiText.wordSpacing = width;
	  }
	else
	  w->multiText.wordSpacing = 0;

	return(w->multiText.fontCache[i].font);
      }
  }

  /* We didn't find the font, get the info and add it to our cache */
  w->multiText.numFonts++;
  w->multiText.fontCache =
    (XmMultiTextFontCache)XtRealloc((char *)w->multiText.fontCache,
				    (i + 1) * sizeof (XmMultiTextFontCacheRec));
  w->multiText.fontCache[i].name = NewString(name);
  w->multiText.fontCache[i].font = XLoadQueryFont(XtDisplay(w), name);
  if (w->multiText.fontCache[i].font == NULL)
    {
      sprintf(buf, MSG4, name, DEFAULT_FONT_NAME);
      XtWarning(buf);

      w->multiText.fontCache[i].font = XLoadQueryFont(XtDisplay(w), DEFAULT_FONT_NAME);
      if (w->multiText.fontCache[i].font == NULL)
	{
	  sprintf(buf, MSG5, DEFAULT_FONT_NAME);
	  XtWarning(buf);
	  return NULL;
	}
    }

  if (w->multiText.smartSpacing)
    {
      width = XTextWidth(w->multiText.fontCache[i].font, " ", 1);
      if (width != 0) w->multiText.wordSpacing = width;
    }
  else
    w->multiText.wordSpacing = 0;

  return (w->multiText.fontCache[i].font);
}



/*----------------------------------------------------------------------*
 |                               NewLine                                |
 *----------------------------------------------------------------------*/
static LineList NewLine (XmMultiTextWidget cw, int indent, int yposn, int lineSpacing)
{
  LineList  lp;
  Dimension newHeight;


  lp = (LineList) XtMalloc(sizeof(struct LineRec));
  if (lp == NULL) XtError(MSG6);
  else
    {
      lp->firstWord        = NULL;
      lp->lastWord         = NULL;
      lp->bbox.lbearing    = 0;
      lp->bbox.rbearing    = 0;
      lp->bbox.width       = 0;
      lp->bbox.ascent      = 0;
      lp->bbox.descent     = 0;
      lp->bbox.attributes  = 0;
      lp->x                = indent + cw->multiText.marginWidth;
      lp->y                = MAX(yposn, cw->multiText.marginHeight);
      lp->wordCount        = 0;
      lp->indent           = indent;
      lp->remaining        = cw->core.width - 2*cw->multiText.marginWidth - indent;
      lp->lineSpacing      = lineSpacing;
      lp->next             = NULL;
      lp->prev             = NULL;  /* don't want to force the prev to be the last line just yet. */

      /* if this line is below the bottom of the widget, the resize the widget to fit. */
      /* this is also done when an existing line is resized.                           */
      newHeight = (Dimension)(lp->y + lp->bbox.ascent + lp->bbox.descent);
      if (cw->core.height < newHeight + cw->multiText.marginHeight)
	ChangeHeight(cw, newHeight + (XtParent(cw)->core.height/RESIZE_FRACTION));
    }

  return (lp);
}



/*----------------------------------------------------------------------*
 |                       StuffWordOntoCurrentLine                       |
 | each word is stored at an x,y location which is relative to the line |
 | that contains this word.  A word's x,y posn IS NOT the location in   |
 | the multitext window!                                                |
 | If any resizing of the line is needed, it will be handled here.  The |
 | calling routine will be notified if any resizing was done so that    |
 | any redrawing of the line can be performed.                          |
 *----------------------------------------------------------------------*/
static Boolean StuffWordOntoCurrentLine (XmMultiTextWidget w, LineList lp, WordList wp,
					 int neededWidth, Boolean firstWordOfLine, Boolean isTab,
					 Boolean prevWordIsTab)
{
  Boolean resizeLine = FALSE;

  if (lp->firstWord == NULL)
    /* zero, not indent since word offset from lp and is already indented correctly. */
    wp->x = 0;
  else
    /* move current posn to the end of the last word (no spacing included - yet!) */
    wp->x = lp->lastWord->x + lp->lastWord->bbox.width;


  /* now add the spacing that should go before this word.                     */
  /* wordspacing is always the spacing used by the font of the previous word. */
  if (!isTab && !prevWordIsTab && (lp->firstWord != NULL))
    {
      wp->x       += w->multiText.wordSpacing   /* lp->wordSpacing */;
      wp->spacing  = w->multiText.wordSpacing;
    }


  /* find out if resize is necessary. If so, resize the line. */
  if ((wp->bbox.ascent > lp->bbox.ascent) || (wp->bbox.descent > lp->bbox.descent)) {
    ResizeTheLine(w, lp, wp);
    resizeLine       = TRUE;
  }
  

  /* append the wordStructure to the line's structure. */
  wp->prev = lp->lastWord;  /* each word should know about the previous word. */
  if (lp->firstWord == NULL)
    lp->firstWord      = wp;
  else
    lp->lastWord->next = wp;
  lp->lastWord = wp;

  /* update the parameters of the line. */
  lp->bbox.width += neededWidth;
  lp->wordCount  ++;
  lp->remaining  -= neededWidth;

  /* update the parameters of the word. */
  wp->y           = lp->bbox.ascent - wp->bbox.ascent;
  wp->linePtr     = lp;

  return (resizeLine);
}




/*----------------------------------------------------------------------*
 |                            GetNeededWidth                            |
 *----------------------------------------------------------------------*/
static int GetNeededWidth (XmMultiTextWidget w, WordList wPtr, int indent,
			   Boolean *firstWordOfLine, Boolean *isTab, Boolean *prevWordIsTab)
{
  int     neededWidth;

  /* lets start with the width of the word. */
  neededWidth = wPtr->bbox.width;

  /* special handling for: first word, tabs, and words following tabs. */
  *firstWordOfLine = ((w->multiText.lastLine == NULL) ||
		      (w->multiText.lastLine->lastWord == NULL));
  *isTab           = (wPtr->chars[0] == TAB);
  *prevWordIsTab   = ((w->multiText.lastLine != NULL) &&
		      (w->multiText.lastLine->lastWord != NULL) &&
		      (w->multiText.lastLine->lastWord->chars[0] == TAB));

  /* if this isn't a tab, then add the indent to the amount that this word requires.            */
  /* this is added since indented lines are just shifted - bounding boxes don't keep this info. */
  if (!isTab) neededWidth += indent;

  if (!(*firstWordOfLine) && !(*prevWordIsTab) && !(*isTab))
    neededWidth += w->multiText.wordSpacing; /* spacing of current font is held by widget. */


  return(neededWidth);
}



/*----------------------------------------------------------------------*
 |                               StoreWord                              |
 | This routine stores the new word into the current line.  If this is  |
 | the first word in the document, then create a new line before        |
 | inserting it.  Before appending the current word to the line, check  |
 | to make sure that it will fit. The word's size is its bounding plus  |
 | the size of the space between it and the previous word.              |
 | Finally, if the word doesn't fit, print the previous full line and   |
 | create a new line for the word.                                      |
 *----------------------------------------------------------------------*/
static unsigned short StoreWord (XmMultiTextWidget w, WordList wordPtr, int indent)
{
  LineList        lp;
  Boolean         resizeLine, firstWordOfLine, isTab, prevWordIsTab;
  unsigned short  returnVal=0;
  int             neededWidth, yposn;


  /* find the amount of space this word takes.  This includes any indents and wordspacing. */
  /* this is used to determine if this word fits onto this line.  (~Word width + indent)   */
  neededWidth = GetNeededWidth(w, wordPtr, indent, &firstWordOfLine, &isTab, &prevWordIsTab);

  /* case 1: no lines in the widget -> make a new one and add the word. */
  if (w->multiText.firstLine == NULL)
    {
      lp                     = NewLine(w, indent, 0, DEFAULT_LINE_SPACING);
      w->multiText.firstLine = lp;
      w->multiText.lastLine  = lp;
      resizeLine             = StuffWordOntoCurrentLine(w, lp, wordPtr, neededWidth,
							firstWordOfLine, isTab, prevWordIsTab);
      returnVal              = APPEND_FIRST;
    }
  else

  /* case 2: word doesn't fit on this line and wordwrapping is on. */
  if ((neededWidth > w->multiText.lastLine->remaining) && w->multiText.wordWrap)
    {
      yposn = w->multiText.lastLine->y + w->multiText.lastLine->bbox.ascent +
	      w->multiText.lastLine->bbox.descent + w->multiText.lastLine->lineSpacing;
      lp    = NewLine(w, indent, yposn, w->multiText.lastLine->lineSpacing);
      w->multiText.lastLine->next = lp;
      lp->prev                    = w->multiText.lastLine;
      resizeLine                  = StuffWordOntoCurrentLine(w, lp, wordPtr,  neededWidth,
							     firstWordOfLine, isTab,
							     prevWordIsTab);
      w->multiText.lastLine       = lp;
      returnVal                   = APPEND_FIRST;
    }
  else

  /* case 3: word doesn't fit and wordwrapping is off. */
  if ((neededWidth > w->multiText.lastLine->remaining) && !w->multiText.wordWrap)
    {
      resizeLine = StuffWordOntoCurrentLine(w, w->multiText.lastLine, wordPtr, neededWidth,
					    firstWordOfLine, isTab, prevWordIsTab);
      returnVal  = APPEND_OK;
      if (resizeLine)
	returnVal = APPEND_RESIZE;   /* this should probably be APPEND_CLIP_RESIZE... */
      else
	returnVal  = APPEND_OK;      /* this should probably be APPEND_CLIP... */
    }
  else

  /* case 4: the word fits onto the current line. */
  if (neededWidth <= w->multiText.lastLine->remaining)
    {
      resizeLine = StuffWordOntoCurrentLine(w, w->multiText.lastLine, wordPtr, neededWidth,
					    firstWordOfLine, isTab, prevWordIsTab);
      if (resizeLine)
	returnVal = APPEND_RESIZE;
      else
	returnVal  = APPEND_OK;
    }


  /* set the current line to the one that was just appended to. */
  UpdateCursor(w, w->multiText.lastLine,
	       w->multiText.lastLine->x + w->multiText.lastLine->lastWord->x +
	       w->multiText.lastLine->lastWord->bbox.width,
	       w->multiText.lastLine->x + w->multiText.lastLine->lastWord->x +
	       w->multiText.lastLine->lastWord->bbox.width,
	       w->multiText.lastLine->y + w->multiText.lastLine->bbox.ascent);

  /* now draw what's necessary - Compile with -DREALTIME to see each word as it's appended. */
#ifdef REALTIME
  if (XtIsRealized) DrawWord(w, wordPtr, FALSE);
#endif


  return(returnVal);
}




/*----------------------------------------------------------------------*
 |                          AppendWordToTopLine                         |
 | This routine will append the given word to the first line in the     |
 | widget.  There must be a line to append to. In addition, the word is |
 | not allowed to wrap to the next line since this isn't supported. If  |
 | wordWrapping is off - no problem; however, if it is ON then there is |
 | only a problem if the word is too long for the line.                 |
 | All resizing of the top line and all following lines is handled in   |
 | this routine.                                                        |
 | THIS ~ASSUMES THAT THIS IS THE ONLY WORD OF THE LINE!!! SOME CLEANUP |
 | WILL BE NECESSARY FOR GENERAL CASES.                                 |
 *----------------------------------------------------------------------*/
static unsigned int AppendWordToTopLine(XmMultiTextWidget cw, WordList wordPtr, int indent)
{
  int          maxAscent, maxDescent, dy;
  int          textBottomY;
  LineList     lp;


  /* this is redundant since this must be false if we are here... */
  if (cw->multiText.firstLine == NULL)
    {
      XtWarning(MSG9);
      return(APPEND_ERROR);
    }


  /* find the amount that all lines must be shifted down. */
  maxAscent  = MAX(wordPtr->bbox.ascent,  cw->multiText.firstLine->bbox.ascent);
  maxDescent = MAX(wordPtr->bbox.descent, cw->multiText.firstLine->bbox.descent);
  dy = (maxAscent + maxDescent) - 
       (cw->multiText.firstLine->bbox.ascent + cw->multiText.firstLine->bbox.descent);

  /* if the text is too large for the widget, then resize to fit. (Bottom of last line is below window. */
  textBottomY = cw->multiText.lastLine->y + cw->multiText.lastLine->bbox.ascent + cw->multiText.lastLine->bbox.descent;

  if (cw->core.height - cw->multiText.marginHeight  <  dy + textBottomY)
    ChangeHeight(cw, dy + textBottomY + cw->multiText.marginHeight + (XtParent(cw)->core.height/2));

  lp = cw->multiText.firstLine;
  if ((dy > 0) && (lp->next != NULL))
    ShiftFollowingLines(lp->next, dy);

  /* now append the word and adjust the current line - all following lines are now correct.    */
  /* note that this line isn't really the correct format; it's newline parameter may be wrong. */
  if (lp->wordCount == 0)
    {
      wordPtr->x   = 0;
      lp->lastWord = lp->firstWord = wordPtr;
    }
  else
    {
      wordPtr->x         = lp->lastWord->x + lp->lastWord->bbox.width;  /* NOT GENERAL... */
      lp->lastWord->next = wordPtr;
      lp->lastWord       = wordPtr;
    }

  wordPtr->linePtr   = cw->multiText.firstLine;
  lp->bbox.ascent    = maxAscent;
  lp->bbox.descent   = maxDescent;
  lp->wordCount++;


  /* now check if we have a word wrapping problem - in any case, don't wrap! */
  if ((lp->remaining < wordPtr->bbox.width) && (wordPtr->wordWrapping))
    XtWarning(MSG8);

  /* currently, multiText isn't consistent with wordwrapping. Sometimes it uses  */
  /* resource value and sometimes it uses the word's value. THIS WILL HAVE TO BE */
  /* FIXED! however, in the mean time - check both values.                       */
  if ((lp->remaining < wordPtr->bbox.width) && (cw->multiText.wordWrap))
    XtWarning(MSG8);

  /* if, or if not wordwrapping, force the new word at the end of the first line. */
  lp->remaining  -= wordPtr->bbox.width;  /* NOT GENERAL... */
  lp->bbox.width += wordPtr->bbox.width;  /* NOT GENERAL... */

  TurnOffCursor(cw);
  cw->multiText.cursorY += dy;
  if (!cw->multiText.exposeOnly)
    if (dy > 0) ScrollWindow(cw, dy);

  return APPEND_OK;
}


/*----------------------------------------------------------------------*
 |                             PositionCursor                           |
 | This routine returns the correct position for a cursor given x, y.   |
 | Since the cursor position must be on a word boundry, the actual X    |
 | position may be different from the cursor X position.  The Y posn is |
 | always correct - or atleast it should be :-)                         |
 | Note - this routine DOES NOT change or move the cursor!              |
 *----------------------------------------------------------------------*/
static void PositionCursor (XmMultiTextWidget cw, int x, int y, LineList *currentLine, int *actualX)
{
  LineList lp;
  WordList wp, closestWord;
  int      dx;


  if (cw->multiText.currentLine == NULL)
    {
      fprintf(stderr, "ERROR - NO-TEXT CASE Should not occur!\n");
      return;
    }


  /* find the line containing the new position.                         */
  /* for speed, check the immediate neighborhood (1 up or 1 down) first */
  if      (PtInLine(x, y, cw->multiText.currentLine))       lp = cw->multiText.currentLine;
  else if (PtInLine(x, y, cw->multiText.currentLine->next)) lp = cw->multiText.currentLine->next;
  else if (PtInLine(x, y, cw->multiText.currentLine->prev)) lp = cw->multiText.currentLine->prev;
  else

  /* else, find line containing the pt. */
  for (lp = cw->multiText.firstLine;  lp != NULL;  lp = lp->next)
    if (PtInLine(x, y, lp))
      break;

  /* if not in any line, then ring bell - went off screen. */
  if (lp == NULL)
    {
      XBell(XtDisplay(cw), 0);

      /* set cursor info to current positions. */
      *actualX     = cw->multiText.actualX;
      *currentLine = cw->multiText.currentLine;

      return;
    }


  /* now to find the horizontal location of the cursor. Either before, after, or middle of line. */
  if     ((x <= lp->x) || (lp->lastWord == NULL))
    *actualX = lp->x;

  else if (x >= (lp->x + lp->lastWord->x + lp->lastWord->bbox.width))
    *actualX = lp->x + lp->lastWord->x + lp->lastWord->bbox.width;

  else
    {
      dx = lp->bbox.width;
      closestWord = lp->firstWord;
      for (wp = lp->firstWord;  wp != NULL;  wp = wp->next)
	if (dx > ABS((x - lp->x) - wp->x))
	  {
	    dx = ABS((x - lp->x) - wp->x);
	    closestWord = wp;
	  }

      *actualX = lp->x + closestWord->x;
    }

  *currentLine = lp;
}



/*----------------------------------------------------------------------*
 |                              UpdateCursor                            |
 *----------------------------------------------------------------------*/
static void UpdateCursor (XmMultiTextWidget cw, LineList lp, int x, int actualX, int actualY)
{
  cw->multiText.cursorX      = x;
  cw->multiText.cursorY      = actualY;
  cw->multiText.actualX      = actualX;
  cw->multiText.currentLine  = lp;
}



/*----------------------------------------------------------------------*
 |                                 MoveUp                               |
 | Moving up dowsn't change the horizontally stored position. That way  |
 | we always scroll back to the same position.                          |
 | Note, cursor is always at the baseline of the line.                  |
 *----------------------------------------------------------------------*/
static void MoveUp (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  int               actualX;
  int               currentX = cw->multiText.cursorX;
  int               currentY;
  LineList          lp;


  if (cw->multiText.textIsSelected) DeselectAll(cw);

  lp = cw->multiText.currentLine;
  if (lp->prev == NULL)
    {
      currentY = cw->multiText.cursorY;
      XBell(XtDisplay(cw), 0);
    }
  else
    currentY = lp->prev->y + lp->prev->bbox.ascent;

  PositionCursor(cw, currentX, currentY, &lp, &actualX);
  UpdateCursor(cw, lp, currentX, actualX, currentY);
}


/*----------------------------------------------------------------------*
 |                                 MoveDown                             |
 | Note, cursor is always at the baseline of the line.                  |
 *----------------------------------------------------------------------*/
static void MoveDown (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  int               actualX;
  int               currentX = cw->multiText.cursorX;
  int               currentY;
  LineList          lp;


  if (cw->multiText.textIsSelected) DeselectAll(cw);

  lp = cw->multiText.currentLine;
  if (lp->next == NULL)
    {
      currentY = cw->multiText.cursorY;
      XBell(XtDisplay(cw), 0);
    }
  else
    currentY = lp->next->y + lp->next->bbox.ascent;

  PositionCursor(cw, currentX, currentY, &lp, &actualX);
  UpdateCursor(cw, lp, currentX, actualX, currentY);;
}



/*----------------------------------------------------------------------*
 |                            ClosestPosition                           |
 | The closest position to a given location in a line can either be the |
 | left side of a word closest to the given position, or the end of the |
 | line. This routine is used to find where the cursor whould be placed |
 | given the reference position.                                        |
 | ClosestWord is returned as the word containing the reference posi-   |
 | tion.                                                                |
 *----------------------------------------------------------------------*/
int ClosestPosition (XmMultiTextWidget cw, LineList lp,
		     WordList *closestWord, int refPosn)
{
  int      dx, x=0;
  WordList wp;


  /* find the current word in this line. It's the word closest to the actual position. */
  dx           = lp->bbox.width;
  *closestWord = lp->firstWord;

  for (wp = lp->firstWord;  wp != NULL;  wp = wp->next)
    if (dx > ABS((cw->multiText.actualX - lp->x) - wp->x))
      {
	dx = ABS((cw->multiText.actualX - lp->x) - wp->x);
	*closestWord = wp;
	x = wp->x + lp->x;
      }

  /* now check if the end of the line is closer than the closest word. */
  if (dx > ABS(lp->x + lp->bbox.width - cw->multiText.actualX))
    {
      *closestWord = NULL;
      x = lp->x + lp->bbox.width;
    }

  return(x);
}



/*----------------------------------------------------------------------*
 |                                 MoveLeft                             |
 | Note, cursor is always at the baseline of the line.                  |
 *----------------------------------------------------------------------*/
static void MoveLeft (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  int               actualX;
  int               currentX;
  int               currentY = cw->multiText.cursorY;
  LineList          lp;
  WordList          closestWord;


  if (cw->multiText.textIsSelected) DeselectAll(cw);

  lp = cw->multiText.currentLine;

  /* find the current word in this line. It's the word closest to the actual position. */
  ClosestPosition(cw, lp, &closestWord, cw->multiText.actualX);


  /* if closestWord is null, then the cursor is off the right side of the line. */
  /* unless there are no words in this line.                                    */

  if ((closestWord == NULL) && (lp->wordCount > 0))  /* move to the left side of the right-most word. */
    {
      if (lp->lastWord != NULL)  /* go to the beginning of the last word. */
	currentX = actualX = lp->x + lp->lastWord->x;
      else
	currentX = actualX = lp->x;
    }
  else
    {
      if ((closestWord != NULL) && (closestWord->prev != NULL))
	currentX = actualX = lp->x + closestWord->prev->x;

      /* should be here if we're in a line that has no words or this is the first line. */
      else
	{
	  if (lp->prev != NULL)
	    {
	      lp = lp->prev;	      /* go to prev line. */
	      currentX = actualX = lp->x + lp->bbox.width;
	      currentY = lp->y + lp->bbox.ascent;
	    }
	  else
	    {
	      /* already at first line. */
	      currentX = actualX = lp->x;
	      XBell(XtDisplay(cw), 0);
	    }
	}
    }

  UpdateCursor(cw, lp, currentX, actualX, currentY);
}


/*----------------------------------------------------------------------*
 |                                 MoveRight                            |
 | Note, cursor is always at the baseline of the line.                  |
 *----------------------------------------------------------------------*/
static void MoveRight (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  XmMultiTextWidget cw = (XmMultiTextWidget) w;
  int               actualX;
  int               currentX;
  int               currentY = cw->multiText.cursorY;
  LineList          lp;
  WordList          closestWord;


  if (cw->multiText.textIsSelected) DeselectAll(cw);

  lp = cw->multiText.currentLine;

  ClosestPosition(cw, lp, &closestWord, cw->multiText.actualX);
  if (closestWord != NULL)
    {
      if (closestWord->next != NULL)
	currentX = actualX = closestWord->next->x + lp->x;
      else
	currentX = actualX = lp->x + lp->bbox.width;
    }
  else
    {
      /* scrolled past end of line - move to beginning of next line if there is one. */
      if (lp->next == NULL)
	{
	  currentX = actualX = lp->x + lp->bbox.width;
	  XBell(XtDisplay(w), 0);
	}
      else
	{
	  lp = lp->next;	      /* go to next line. */
	  currentX = actualX = lp->x;
	  currentY = lp->y + lp->bbox.ascent;
	}
    }

  UpdateCursor(cw, lp, currentX, actualX, currentY);
}


#define cursor_width  7
#define cursor_height 5
/*----------------------------------------------------------------------*
 |                           MakeCursorPixmaps                          |
 | this routine creates (or updates) the cursor's fg and bg pixmaps.    |
 *----------------------------------------------------------------------*/
static void MakeCursorPixmaps (XmMultiTextWidget cw)
{
  if (cw->multiText.cursorFg == None)
    {
      static char cursor_bits[] = {0x08, 0x1c, 0x3e, 0x7f, 0x77};

      cw->multiText.cursorFg =
	XCreatePixmapFromBitmapData(XtDisplay(cw), XtWindow(cw),
				    cursor_bits, cursor_width, cursor_height,
				    cw->multiText.cursorColor,
				    None,
				    DefaultDepthOfScreen(XtScreen(cw)));
      cw->multiText.cursorMask =
	XCreatePixmapFromBitmapData(XtDisplay(cw), XtWindow(cw),
				    cursor_bits, cursor_width, cursor_height,
				    WhitePixelOfScreen(XtScreen(cw)),
				    None,
				    1);
    }

  if (cw->multiText.cursorBg == None)
    cw->multiText.cursorBg = XCreatePixmap(XtDisplay(cw), XtWindow(cw),
					   cursor_width, cursor_height,
					   DefaultDepthOfScreen(XtScreen(cw)));

  /* grab the region under the cursor and store it into the cursorBg field. */
  XCopyArea(XtDisplay(cw), XtWindow(cw), cw->multiText.cursorBg, cw->multiText.cursorGC,
	    cw->multiText.actualX, cw->multiText.cursorY, cursor_width, cursor_height,
	    0, 0);
}



/*----------------------------------------------------------------------*
 |                               BlinkCursor                            |
 *----------------------------------------------------------------------*/
static void BlinkCursor (XmMultiTextWidget cw)
{
  cw->multiText.blinkTimeOutID = 0;

  if ((cw->multiText.showCursor) &&
      (XtIsRealized((Widget)cw) && !cw->multiText.textIsSelected && !cw->multiText.selecting))
    {
      /* this only gets called once. */
      if (cw->multiText.cursorFg == None)
	{
	  MakeCursorPixmaps(cw);
	  cw->multiText.oldX = cw->multiText.actualX;
	  cw->multiText.oldY = cw->multiText.cursorY;
	}

      XCopyArea(XtDisplay(cw), cw->multiText.cursorBg, XtWindow(cw), cw->multiText.cursorGC,
		0, 0, cursor_width, cursor_height,
		cw->multiText.oldX, cw->multiText.oldY);

      if ((cw->multiText.oldX != cw->multiText.actualX) || (cw->multiText.oldY != cw->multiText.cursorY))
	{
	  cw->multiText.blinkState = FALSE;
	  MakeCursorPixmaps(cw);
	}

      if (!cw->multiText.blinkState)
	{
	  XSetClipMask  (XtDisplay(cw), cw->multiText.cursorGC, cw->multiText.cursorMask);
	  XSetClipOrigin(XtDisplay(cw), cw->multiText.cursorGC, cw->multiText.actualX, cw->multiText.cursorY);

	  XCopyArea(XtDisplay(cw), cw->multiText.cursorFg, XtWindow(cw), cw->multiText.cursorGC,
		    0, 0, cursor_width, cursor_height,
		    cw->multiText.actualX, cw->multiText.cursorY);
	  XSetClipMask  (XtDisplay(cw), cw->multiText.cursorGC, None);

	  cw->multiText.oldX = cw->multiText.actualX;
	  cw->multiText.oldY = cw->multiText.cursorY;
	}

      cw->multiText.blinkState = !cw->multiText.blinkState;
    }  


  cw->multiText.blinkTimeOutID =
    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)cw), cw->multiText.blinkRate, (XtTimerCallbackProc)BlinkCursor, cw);
}



/*----------------------------------------------------------------------*
 |                           TurnOffCursor                              |
 *----------------------------------------------------------------------*/
static void TurnOffCursor (XmMultiTextWidget cw)
{
  cw->multiText.cursorAvailable = FALSE;

  if (cw->multiText.blinkTimeOutID != 0)
    {
      XtRemoveTimeOut(cw->multiText.blinkTimeOutID);
      cw->multiText.blinkTimeOutID = 0;
    }

  if (cw->multiText.blinkState == ON)
    {
      BlinkCursor(cw);
      XtRemoveTimeOut(cw->multiText.blinkTimeOutID);
      cw->multiText.blinkTimeOutID = 0;
    }

  /* clearout the pixmaps. */
  if (cw->multiText.cursorFg != None)
    {
      XFreePixmap(XtDisplay(cw), cw->multiText.cursorFg);
      XFreePixmap(XtDisplay(cw), cw->multiText.cursorBg);
      XFreePixmap(XtDisplay(cw), cw->multiText.cursorMask);
    }
  cw->multiText.cursorFg       = None;
  cw->multiText.cursorBg       = None;
  cw->multiText.cursorMask     = None;
}



/*----------------------------------------------------------------------*
 |                           TurnOnCursor                               |
 *----------------------------------------------------------------------*/
static void TurnOnCursor (XmMultiTextWidget cw)
{
  cw->multiText.cursorAvailable = TRUE;
  if (cw->multiText.blinkTimeOutID != 0)
    {
      XtRemoveTimeOut(cw->multiText.blinkTimeOutID);
      cw->multiText.blinkTimeOutID = 0;
    }
  BlinkCursor(cw);
}



/*##########################################################################################*
 #             O U T P U T    R O U T I N E S    F O R    U S E R    I N P U T              #
 # ... future work...                                                                       #
 *##########################################################################################*/



/*-----------------------========================-----------------------*
 |                       KeyPush action procedure                       |
 | This routine is active if the MultiText widget is editable.  There   |
 | is currently no checking for this case.                              |
 |                                                                      |
 *-----------------------========================-----------------------*/
static void KeyPush (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
  KeySym            ks;
  char             *str = "  ";

  XLookupString(&event->xkey, str, 1, &ks, NULL);

  switch (*str)
    {
    case CR:
      printf("<CR>\n");  break;
    case TAB:
      printf("<TAB>"); break;
    case CTRL_C:
      printf("<CTRL_C>\n"); break;
    case SPACE:    
      printf("<space>"); break;
    case BACKSPACE:
      printf("<backspace>"); break;
    default:
      if (isgraph(*str)) printf("%c", *str);
    }

/*
  XmMultiTextAppendChar(w, str[0]);
*/
}


/*-----------------------------=========--------------------------------*
 |                             NewLineCR                                |
 |                                                                      |
 *-----------------------------=========--------------------------------*/
static
void NewLineCR (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
}


/*----------------------------===========-------------------------------*
 |                            InsertSpace                               |
 |                                                                      |
 *----------------------------===========-------------------------------*/
static
void InsertSpace (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
}


/*------------------------------=====-----------------------------------*
 |                              Dummy                                   |
 |                                                                      |
 *------------------------------=====-----------------------------------*/
static
void Dummy (Widget w, XEvent* event, String* params, Cardinal* numParams)
{
}
