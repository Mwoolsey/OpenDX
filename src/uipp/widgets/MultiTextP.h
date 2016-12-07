/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _MultiTextP_h
#define _MultiTextP_h
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>
#include <Xm/CutPaste.h>
#include "MultiText.h"
#ifndef NODPS
#include <DPS/XDPSlib.h>
#include <DPS/dpsXclient.h>
#include <DPS/psops.h>
#define DPSNAME		"Adobe-DPS-Extension"
#endif
#define COLOR_NAME_SIZE	32
#define FONT_NAME_SIZE	64
#define CURSOR_COUNT	8
#define RESIZE_FRACTION	2
#if (XmVersion < 1002)
    #define XmRFloat	"float"
#endif
#define MSG1	"Wait cursor count (XmNwaitCursor) must be between 0 and %d."
#define MSG2	"DPS scaling percentage (XmNscaleDPSpercent) must be greater then 0"
#define MSG3	"Not enough memory to create link record in routine XmMultiTextMakeLinkRecord."
#define MSG4	"Font '%s' not found.  Substituting '%s'."
#define MSG5	"Gasp! Can't even find '%s' font!"
#define MSG6	"Out of memory - Can't perform XtMalloc."
#define MSG7	"DPS extension not available - DPS Turned off."
#define MSG8	"Word-wrapping not supported when inserting at top. Turn wordWrapping off."
#define MSG9	"XmMultiText requires a line to exists before appending a word."
typedef struct LineRec *LineList;
typedef struct WordRec *WordList;
typedef struct CharRec *CharList;

typedef enum
{
    NORMAL,		/* word is not selected */
    DESELECTED,		/* word was last drawn selected, but isn't selected */
    SELECTED		/* word is selected */

} SelectMode;

struct CharRec
{
    XCharStruct		bbox;		/* size of box */
    int			x;
    int			y;              /* coordinates of origin */
    struct CharRec*	next;
};

struct WordRec
{
    CharList        charList;
    XCharStruct     bbox;              /* size of box */
    int             x, y;              /* coordinates of origin -  RELATIVE TO THE LINE!!! */
    int             length;            /* number of characters in string */
    char           *chars;             /* characters in word */
    int             spacing;           /* width of a space character in this font. */
    int             tabCount;          /* number of tabs preceeding word. */
    int             spaceCount;        /* number of space preceeding word. */
    Boolean         selected;          /* is set if this word is currently selected. */
    SelectMode      mode;              /* tells if the word is selected or not or in between. */
    Pixmap          image;
    Widget          widget;
				     /* Format Info... */
    XFontStruct    *font;              /* number of font */
    unsigned long   color;             /* color to use */
    LinkInfoPtr     linkInfo;          /* all information pertaining to a link. */
    int             tabStops;          /* current tabstop setting. */
    Boolean         wordWrapping;      /* specifies if word should be wrapped if necessary. */

    LineList		linePtr;           /* this points to the line containing this word */
    struct WordRec *next, *prev;

};


struct LineRec
{
    WordList        firstWord;
    WordList        lastWord;         /* last word of line */
    XCharStruct     bbox;             /* size of line's bounding box */
    int             x, y;             /* coordinates of origin */
    int             wordCount;        /* number of Words in line */
    int             indent;           /* number of pixels to indent */
    int             remaining;        /* number of pixels till the line is filled. */
    int             lineSpacing;      /* width in pixels of the space between lines. */
    Boolean         newLine;          /* true if the newline at the end of the line is 'hard'. */
    struct LineRec *next, *prev;

};


/*
 * Resource list for Separator:
 */
#define offset(field) XtOffset(XmMultiTextWidget, field)


/*
 * MultiText class part:
 */
typedef struct _XmMultiTextClassPart
{
    XtPointer		ignore;

} XmMultiTextClassPart;


/*
 * Full MultiText class record declaration:
 */
typedef struct _XmMultiTextClassRec
{
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintClassPart     constraint_class;
    XmManagerClassPart      manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    XmMultiTextClassPart    multiText_class;

} XmMultiTextClassRec;


extern XmMultiTextClassRec xmMultiTextClassRec;


typedef struct _XmMultiTextFontCacheRec
{
    XFontStruct*	font;
    char*		name;
    char*		buf;

} XmMultiTextFontCacheRec, *XmMultiTextFontCache;

/*
 * Record structure to store information on sub-strings and corresponding
 * visual attributes.  This info is kept in a "display" list linked records.
 */
typedef struct DisplayRec
{
    XID			fid;		/* sub-string font setting	*/
    unsigned long	color;		/* sub-string color setting	*/
    char*		str;		/* sub-string			*/
  struct DisplayRec*	next;		/* next record pointer		*/

} DisplayStruct;


/*--------------------------------------------------------------------------------*
 | MultiText instance record                                                      |
 | The XmMultiTextPart structure defines resources added by the MultiText widget. |
 | This structure maintains the current state of the MultiText widget.            |
 *--------------------------------------------------------------------------------*/
typedef struct
{
    Boolean		dpsCapable;	/* system DPS-capable?		*/
    Boolean		frameMakerFix;	/* repair DPS FrameMaker bug?	*/
    Boolean		smoothScroll;	/* scroll smoothly (for Bkmstr)	*/
    Boolean		exposeOnly;     /* wait for expose events?	*/
    Boolean		drawing;	/* currently redrawing?		*/
    Boolean		selecting;	/* Currently selected text?	*/
    Boolean		textIsSelected;	/* Is there selected text?	*/
    Boolean		wordWrap;	/* word-wrapping allowed?	*/
    Boolean		smartSpacing;	/* calculate word spacing?	*/
    Boolean		showCursor;	/* cursor is active?		*/
    Boolean		cursorAvailable;/* Cursor is visisble (on)?	*/
    Boolean		focusSensitive;	/* cursor visible iff focused?	*/
    Boolean		blinkState;	/* Blink the cursor?		*/

    int			marginWidth;	/* margin width			*/
    int			marginHeight;	/* margin height		*/

    int			waitCursorCount;/* # watch cursors cycled (1-8)	*/
    int			waitCursorIndex;/* current wait cursor index	*/

    LineList		firstLine;	/* the list of lines to display	*/
    LineList		currentLine;	/* line containing the cursor	*/
    LineList		lastLine;	/* line where appends would go  */

    int			tabs[MAX_TAB_COUNT];
    int			tabCount;

    int			scaleDPSpercent;/*				*/

    int			wordSpacing;	/* current word spacing		*/

    XmMultiTextFontCache fontCache;	/* a list of referenced fonts	*/
    int			numFonts;	/* number of fonts in fontCache	*/

    GC			textGC;		/* GC for displayed text	*/
    GC			selectGC;	/* GC for selected text		*/

    int			startX;
    int			startY;

    Font		currentFontID;
    unsigned long	currentColor;
    XColor		color;
    char*		lastColorName;

    XtIntervalId	blinkTimeOutID;	/* ID for blinking timeout	*/
    int			blinkRate;	/* cursor flash rate, millisec.	*/

    int			oldX;		/* Old cursor location:		*/
    int			oldY;		/* for cursor erase		*/
    int			actualX;	/* actual x-axis cursor pos.	*/
    int 		cursorX;	/* flashing cursor location:	*/
    int			cursorY;	/* may NOT be actual location	*/

    Pixmap		cursorFg;	/* cursor pixmap foreground	*/
    Pixmap		cursorBg;	/* cursor pixmap background	*/
    Pixmap		cursorMask;	/* cursor pixmap mask		*/
    Pixel		cursorColor;	/* cursor color			*/
    GC			cursorGC;	/* GC for cursor		*/

    XtCallbackList	linkCallback;	/* link press callback		*/
    XtCallbackList	selectCallback;	/* text selection callback	*/

} XmMultiTextPart;


/*
 * Full instance record declaration for the MultiText widget:
 */
typedef struct _XmMultiTextRec
{
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    XmManagerPart	manager;
    XmDrawingAreaPart	drawing_area;
    XmMultiTextPart	multiText;

} XmMultiTextRec;


#endif /* _MultiTextP_h */
