/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _MultiText_h
#define _MultiText_h

#include <Xm/Xm.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define NODPS

#define MAX_TAB_COUNT 32

#define APPEND_OK     (unsigned short)0
#define APPEND_RESIZE (unsigned short)1
#define APPEND_FIRST  (unsigned short)2
#define APPEND_ERROR  (unsigned short)3


/*
 * Basic type definitions:
 */
typedef enum
{
  NOOP,		/* specifies that this is not a link.			*/
  LINK,		/* standard link type - data should be a file name.	*/
  RETURN,	/* used for links to previous screens			*/
  FOOTNOTE,	/* footnote link requires special processing -		*/
		/*  no link support inside				*/
  SPOTREF,	/* spot reference - link reference in middle of a file	*/
  TABLE,	/* table containings columns.				*/
  POSTSCRIPT,	/* postscript image data.				*/
  PIXMAP,	/* pixmap image.					*/
  WIDGET	/* imbeded widget.					*/

} LinkType;


typedef char* LinkPosition;


typedef struct LinkRec
{
    LinkType		linkType;
    LinkPosition	linkPosn;
    char*		linkData;

}* LinkInfoPtr;



/*
 * Pointer to the MultiText widget class:
 */
extern WidgetClass xmMultiTextWidgetClass;


/*
 * MultiText widget/class pointers:
 */
typedef struct _XmMultiTextRec* XmMultiTextWidget;

typedef struct _XmMultiTextClassRec* XmMultiTextWidgetClass;


/*
 * Resource strings for the MultiText widget:
 */
#define XmNcursorColor		"cursorColor"
#define XmNdpsCapable		"dpsCapable"
#define XmNexposeOnly		"exposeOnly"
#define XmNfocusSensitive	"focusSensitive"
#define XmNlinkCallback		"linkCallback"
#define XmNscaleDPSpercent	"scaleDPSpercent"
#ifndef XmNselectCallback
#define XmNselectCallback	"selectCallback"
#endif
#define XmNshowCursor		"showCursor"
#define XmNsmartSpacing		"smartSpacing"
#define XmNsmoothScroll		"smoothScroll"
#define XmNwaitCursorCount	"waitCursorCount"

#if !(defined(_Xm_h) || defined(XM_H))
#define XmNmarginHeight		"marginHeight"
#define XmNmarginWidth		"marginWidth"
#define XmNwordWrap		"wordWrap"
#endif

#define XmCCursorColor		"CursorColor"
#define XmCDPSCapable		"DPSCapable"
#define XmCExposeOnly		"ExposeOnly"
#define XmCFocusSensitive	"FocusSensitive"
#define XmCLinkCallback		"LinkCallback"
#define XmCScaleDPSpercent	"ScaleDPSpercent"
#define XmCSelectCallback	"SelectCallback"
#define XmCShowCursor		"ShowCursor"
#define XmCSmartSpacing		"SmartSpacing"
#define XmCSmoothScroll		"SmoothScroll"
#define XmCWaitCursorCount	"WaitCursorCount"

#if !(defined(_Xm_h) || defined(XM_H))
#define XmCMarginHeight		"MarginHeight"
#define XmCMarginWidth		"MarginWidth"
#define XmCWordWrap		"WordWrap"
#endif

/*
 * Callback reasons:
 */
#define XmCR_LINK		1
#define XmCR_SELECT		2


/*
 * Callback structures:
 */
typedef struct
{
    int			reason;
    XEvent*		event;
    LinkType		type;
    LinkPosition	posn;
    char*		data;
    char*		word;

} XmMultiTextLinkCallbackStruct;


typedef struct
{
    int			reason;
    XEvent*		event;
    char*		data;

} XmMultiTextSelectCallbackStruct;


/*
 * Convenience routines:
 */
extern void
    XmMultiTextClearText(Widget);

extern int
    XmMultiTextGetPosition(Widget);

extern int
    XmMultiTextGetCursorPosition(Widget);

extern void
    XmMultiTextQueryCursor(Widget,
			   int*,
			   int*);

extern void
    XmMultiTextDeselectAll(Widget);

extern void
    XmMultiTextSetTabStops(Widget,
			   int*,
			   int);

extern LinkInfoPtr
    XmMultiTextMakeLinkRecord(Widget,
			      LinkType,
			      LinkPosition,
			      char*);

extern void
    XmMultiTextAppendNewLine(Widget,
			     char*,
			     char*,
			     int);

extern void
    XmMultiTextOpenLineTop(Widget,
			   int);

extern unsigned short
    XmMultiTextAppendWord(Widget,
			  char*,
			  char*,
			  char*,
			  int,
			  LinkInfoPtr);

extern unsigned short
    XmMultiTextAppendImage(Widget,
			   char*,
			   char*,
			   char*,
			   int,
			   Pixmap,
			   unsigned int,
			   unsigned int,
			   LinkInfoPtr);

extern unsigned short
    XmMultiTextAppendWidget(Widget,
			    char*,
			    char*,
			    char*,
			    int,
			    Widget);

extern unsigned short
    XmMultiTextAppendDPS(Widget,
			 char*,
			 char*,
			 char*,
			 int,
			 char*,
			 unsigned int,
			 unsigned int,
			 LinkInfoPtr);
extern unsigned short
    XmMultiTextAppendWordTop(Widget,
			     char*,
			     char*,
			     char*,
			     int,
			     LinkInfoPtr);

extern int
    XmMultiTextDeleteLineTop(Widget,
			     int);

extern void
    XmMultiTextDeleteLineBottom(Widget);

extern int
    XmMultiTextLongestLineLength(Widget);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif /* _MultiText_h */
