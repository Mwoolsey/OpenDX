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

#include <stdio.h> 
#include <string.h> 

#include <Xm/Xm.h>
#include <X11/Shell.h>

#if XmVersion > 1000 
#include <Xm/Protocols.h>
#else
#include <X11/Protocols.h>
#endif

#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/BulletinB.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/Command.h>
#include <Xm/CutPaste.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawnB.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>

#include "../widgets/MultiText.h"

#include "help.h"

#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))

#if defined(intelnt) || defined(WIN32)
const char *AssertMsgString = "Internal error detected at \"%s\":%d.\n";
#endif

#ifdef DXD_NEEDS_MKTEMP
char* mktemp(char*);
#else
extern char* mktemp(char*);
#endif

extern void RemoveHelpIndexFileCB();

extern void PushMenu(Widget, UserData*, LinkType, char*); /* from history.c */
extern int  SearchList(SpotList *, char *); /* from helplist.c */
extern void DeleteLastNode(HistoryList*); /* from history.c */
extern void Pop(Stack*); /* from helpstack.c */
extern void FreeSpotList(SpotList *); /* from helplist.c */
extern void FreeStack(Stack *); /* from helpstack.c */

Bool HelpOn(Widget, int, char *, char *, int);

/*
 * CALLBACK PROCEDURE DECLARATIONS
 */
extern void CloseCB();
extern void GoBackCB();
extern void LinkCB();
extern void DestroyCB();

extern XmFontList FONT_LIST();
extern int SendWidgetText(); 
extern SpotList *NewList();


static char ReadmeNotAvailable[] = "HelpNotAvailable";


/*
 * AddToHistory -
 * 
 * Keeps track of the state of the "Go Back" button.
 */
void AddToHistory(Widget    RefWidget,
		  int       linkType,
		  char*     LinkData,
		  char*     Label,
		  UserData* userdata)
{
    int   argcnt;
    Arg   args[8];

    argcnt = 0;
  
    /*
     * If the linkType == RETURN  the user clicked on the go back button, 
     * so don't add the file to the history menu.
     * If there is no popup menu (as in the Sun case), don't add the file
     * to the history menu.
     */
    if (linkType != RETURN && userdata->filename[0] != '\0')
    {
	/* PushMenu(RefWidget, userdata, linkType, Label, LinkData); */
	PushMenu(RefWidget, userdata, (LinkType) linkType, Label);
    }

    /*
     * Enable the go back button.
     */
    if (userdata->historylist->length == 2)
    {
	argcnt = 0;
	XtSetArg(args[argcnt], XmNsensitive, True); argcnt++;
	XtSetValues(userdata->goback, args, argcnt);
    }

    /*
     * Disable the go back button.
     */
    else if (userdata->historylist->length == 1)
    {
	argcnt = 0;
	XtSetArg(args[argcnt], XmNsensitive, False); argcnt++;
	XtSetValues(userdata->goback, args, argcnt); 
    }
}


void MoveText(UserData *userdata, int offset)
{
 Arg         args[1];
 int         maxvalue;
 int         slidersize;


 XmScrollBarCallbackStruct callValue;

       XtSetArg(args[0],XmNmaximum,&maxvalue);
       XtGetValues(userdata->vbar,args,1);

       XtSetArg(args[0],XmNsliderSize,&slidersize);
       XtGetValues(userdata->vbar,args,1);

       if (offset > maxvalue - slidersize)
          offset = maxvalue - slidersize;

         
       XtSetArg(args[0],XmNvalue,offset);
       XtSetValues(userdata->vbar,args,1);
       callValue.reason = XmCR_DRAG;
       callValue.event = NULL;
       callValue.value = offset;
       callValue.pixel = offset;
       XtCallCallbacks(userdata->vbar, XmNdragCallback, (XtPointer)&callValue);
}
 
void ResizeTheWindow(Widget w,
    XtPointer closure,
    XEvent    *event,
    Boolean   *continue_to_dispatch)
{
    Widget mtextWidget = (Widget)closure;
    Dimension width;
    UserData *userdata;


    XtVaGetValues(XtParent(mtextWidget), XmNwidth, &width, NULL);
    XtVaSetValues(mtextWidget, XmNwidth, width, NULL);

/*  printf ("In ResizeTheWindow getting width of mtext parent: %d\n",width); */
    *continue_to_dispatch = True;

    /* take the top thing off of the history, pop it off and redisplay it in
       the new width */
    XtVaGetValues(mtextWidget, XmNuserData, &userdata, NULL);
    if (userdata && userdata->historylist && userdata->historylist->tail)
	HelpOn(mtextWidget, RETURN, userdata->historylist->tail->filename,
				    userdata->historylist->tail->label,
				    userdata->historylist->tail->position);
}

/****************************************************************************** 
 *
 * This function is used to provide the user with an index into several 
 * different help topics.  It uses the mktemp() system call to create a 
 * temporary file.  The topics passed in are formatted and written to this 
 * file.  A callback is set up so when the widget is destroyed the file 
 * will be removed. 
 * 
 *
 * Widget RefWidget -  The reference widget
 * IndexTable - 
 *     typedef struct {
 *        char *String;
 *        char *FileName;
 *        char *Font;
 *        char *Color;
 *      } HelpIndex;
 * This structure contains the string, file name, font and color of the 
 * link.  If the font or color fields are set to NULL the function will
 * use the value of the previous link.  
 * int Entries - The number of string entries in IndexTable.
 *
 ******************************************************************************/

#define DEFAULT_INDEX_FONT "-adobe-times-medium-r-normal--14*"
#define DEFAULT_INDEX_COLOR "black"

char *BuildHelpIndex(RefWidget, IndexTable, Entries)
    Widget RefWidget; 
    HelpIndex **IndexTable;
    int Entries;
{
    FILE *outfile;
    int i;
    char name[MAXPATHLEN];
    static char str[MAXPATHLEN];
    char *fname;
    char CurrentFont[127];
    char CurrentColor[127];

    strcpy(CurrentFont, DEFAULT_INDEX_FONT);
    strcpy(CurrentColor, DEFAULT_INDEX_COLOR);

    strcpy(name, "helpXXXXXX");
    fname = mktemp(name);

    sprintf(str, "%s/%s", "/tmp", fname);
    outfile = fopen(str,"w");

    if (outfile == NULL)
    {
	/* XXX
	 * Used to be uiqErrorMessageDialog, should somehow be ErrorMessage
	 */
	printf("Unable to open index file %s.", str);
	return (NULL);
    }

    for (i = 0; i < Entries; i++)
    {
	if (IndexTable[i]->Font != NULL)
	    strcpy(CurrentFont,IndexTable[i]->Font);
	if (IndexTable[i]->Color != NULL)
	    strcpy(CurrentColor,IndexTable[i]->Color);

	fprintf(outfile,"#!N \n");
	fprintf(outfile,"#!F%s\n",CurrentFont);
	fprintf(outfile,"#!C%s\n",CurrentColor);
	fprintf(outfile,"#!L%s l %s #!EL\n",IndexTable[i]->FileName,
		IndexTable[i]->String);
	fprintf(outfile,"#!N \n");
	fprintf(outfile,"#!N \n");
    }

    XtAddCallback(RefWidget, XmNdestroyCallback, RemoveHelpIndexFileCB,
		  (XtPointer) strdup(str));
    fclose(outfile);
    return(str);
}


Bool HelpOn(Widget RefWidget,
	    int    linkType,
	    char*  LinkData,
	    char*  Label,
	    int    position)

{
    FILE*       infile;
    int         argcnt,i;
    char        *ref,*gptr,*plptr;
    UserData    *userdata;
    int         len, offset = 0;
    Bool        ManPage = FALSE;
    char        fname[MAXPATHLEN];
    char        tempname[MAXPATHLEN];
    Arg         args[8];
    int         gpos,plpos;

    if (RefWidget == NULL)
    {
	printf("invalid parent widget HelpOn()\n");
	return(FALSE);
    }
    if (Label == NULL)
    {
	Label = "Introduction";
    }

    ref = LinkData;

    argcnt = 0;
    XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
    XtGetValues(RefWidget, args, argcnt);

    if (linkType == SPOTREF || linkType == FOOTNOTE)
    {
	if (strchr(ref,',') != NULL)
	{
	    *strchr(ref,',') = '\0';
	}
	LinkData = ref + strlen(ref) + 1; 

	/* is it a spot reference into the current file ?           */
	/* if it is we already have the y position of the reference */

	if (STRCMP(LinkData,userdata->filename) == 0)
	{
	    offset = SearchList(userdata->spotlist,ref); 
	    MoveText(userdata,offset);
	    return(TRUE);
	} 
    }
    else 
    {
	ref = NULL;
    }

    if (LinkData[0] != '#')
    {
	if (*LinkData != '/')
	    sprintf(fname,"%s/%s", GetHelpDirectory(), LinkData);
	else
	    strcpy(fname, LinkData);

  /*-------------------------------------------------------------------*/
  /* Special case the README files. Treat like man pages and construct */
  /* actual file name depending on platform-related ifdefs             */
  /*-------------------------------------------------------------------*/
        plptr = strstr(fname,README_PLATFORM);
        gptr  = strstr(fname,README_GENERIC);
        if (plptr !=NULL) {
          ManPage = TRUE;
          plpos = (abs(plptr - fname));
          tempname[0] = '\0';
          strncat(tempname,fname,plpos);
          fname[0] = '\0'; 
          sprintf(fname,"%s%s%s%s",tempname,README_PREFIX,"_",DXD_ARCHNAME);
       }
       else if (gptr !=NULL) {
          ManPage = TRUE;
          gpos = (abs(gptr - fname));
          tempname[0] = '\0';
          strncat(tempname,fname,gpos);
          fname[0] = '\0'; 
          sprintf(fname,"%s%s",tempname,README_PREFIX);
        }
	if ((infile = fopen(fname,"r")) == NULL){
           ManPage = FALSE;
#ifdef DEBUG          
           printf("\007File %s not found\n", fname);
#endif
	   sprintf(fname,"%s/%s", GetHelpDirectory(), ReadmeNotAvailable);
           infile = fopen(fname,"r"); 
        }
    }
    else
    {
	ManPage = TRUE;
	sprintf(fname,"%s %s","man",LinkData + 1);
	infile = popen(fname,"r");
    }

    strcpy(userdata->filename,LinkData);
    strcpy(userdata->label,Label);

    if (infile == NULL)
    {
	printf("Error: can't open %s\n\n",fname);
	return(FALSE);
    }

    if (XtIsRealized(userdata->toplevel) != TRUE)
    { 
        XtRealizeWidget(userdata->toplevel);
        XtUnmapWidget(userdata->hbar);
    }


    if  (userdata->mapped != TRUE)
    {
	userdata->mapped = TRUE;

	len = userdata->historylist->length;
	for (i = 0; i < len; i++)
	    DeleteLastNode(userdata->historylist);

	len = userdata->fontstack->length;
	for (i = 0; i < len; i++)
	    Pop(userdata->fontstack);

	len = userdata->colorstack->length;
	for (i = 0; i < len; i++)
	    Pop(userdata->colorstack);

	XtMapWidget(userdata->toplevel);
    }

    /*
     * Deiconify (if necessary) and bring the help window to the top.
     */
    XMapRaised(XtDisplay(userdata->toplevel), XtWindow(userdata->toplevel));

    AddToHistory(RefWidget, linkType, LinkData, Label, userdata);

    XmMultiTextClearText(RefWidget);
    SendWidgetText(RefWidget,infile,ManPage,ref);

    if (LinkData[0] != '#')
	fclose(infile);
    else
	pclose(infile);

    if (linkType == SPOTREF)
    {
	offset = SearchList(userdata->spotlist,ref);
	if (offset != 0)
	{
	    MoveText(userdata,offset);
	}
    }

    return(TRUE);
}



Widget InitHelpSystem(Widget parent, Widget *multiText)
{
    Widget	frame;
    Widget      form;
    Widget      panedWindow;
    Widget      scrolledWindow;
    Widget      hbar;
    Widget      vbar;
    Widget      goBackButton;
    Widget      menuShell;
    UserData*   userdata;
    Dimension   width;
    Dimension   height;
    Dimension   thickness;
    int         argcnt;

    Arg         args[64];
    
    frame =
	XtVaCreateManagedWidget
	    ("helpFrame",
	     xmFrameWidgetClass,
	     parent,
	     XmNshadowType,   XmSHADOW_OUT,
	     XmNmarginHeight, 5,
	     XmNmarginWidth,  5,
	     NULL);

    /*
     * Create the top-level form widget.
     */
    form = XtVaCreateManagedWidget("helpForm", xmFormWidgetClass, frame, NULL);

    /*
     * Create the "Go Back" button.
     */
    goBackButton =
	XtVaCreateManagedWidget ("goBackButton", xmPushButtonWidgetClass, form,
	    XmNsensitive,        False,
	    XmNrecomputeSize,    False,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment,   XmATTACH_FORM,
	    NULL);
    /*
     * Create a paned window.
     */
    panedWindow =
	XtVaCreateManagedWidget("panedWin",
	    xmPanedWindowWidgetClass,
	    form,
	    XmNtopAttachment,    XmATTACH_FORM,
	    XmNleftAttachment,   XmATTACH_FORM,
	    XmNrightAttachment,  XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget,     goBackButton,
	    XmNbottomOffset,     5,
	    NULL);

    /*
     * Create a scrolled window.
     */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNscrollingPolicy, XmAUTOMATIC); argcnt++;
    XtSetArg(args[argcnt], XmNallowResize,     True);        argcnt++;

    scrolledWindow =
	XtCreateManagedWidget
	    ("scrolledWin", 
	     xmScrolledWindowWidgetClass,
	     panedWindow,
	     args,
	     argcnt);

    /*
     * Get the horizotal/vertical scrollbar widget ID's.
     */
    XtSetArg(args[0], XmNhorizontalScrollBar, &hbar);
    XtSetArg(args[1], XmNverticalScrollBar,   &vbar);
    XtGetValues(scrolledWindow, args, 2);
    
    /*
     * Override the highlight thickness on the horizontal scrollbar.
     */
    if (hbar)
    {
	XtSetArg(args[0], XmNhighlightThickness, &thickness);
	XtSetArg(args[1], XmNheight,             &height);
	XtGetValues(hbar, args, 2);

	height -= thickness * 2;

	XtSetArg(args[0], XmNhighlightThickness, 0);
	XtSetArg(args[1], XmNheight,             height);
	XtSetValues(hbar, args, 2);
    }

    /*
     * Override the highlight thickness on the vertical scrollbar.
     */
    if (vbar)
    {
	XtSetArg(args[0], XmNhighlightThickness, &thickness);
	XtSetArg(args[1], XmNwidth,              &width);
	XtGetValues(vbar, args, 2);

	width -= thickness * 2;
	XtSetArg(args[0], XmNhighlightThickness, 0);
	XtSetArg(args[1], XmNwidth,              width);
	XtSetValues(vbar, args, 2);
    }

    /*
     * Create the MultiText widget.
     */
    XtVaGetValues(scrolledWindow, XmNwidth, &width, NULL);
    *multiText =
	XtVaCreateManagedWidget(
	    "helpText", xmMultiTextWidgetClass, scrolledWindow,
	    XmNshowCursor, False,
	    XmNwidth, width,
	    NULL);

    XtAddCallback(*multiText, XmNlinkCallback, LinkCB, NULL);

    /*
     * Register a callback to find out when the window is resized.
     */
    XtAddEventHandler
	(panedWindow, StructureNotifyMask, False, ResizeTheWindow, (XtPointer)*multiText);

    XtAddCallback
	(goBackButton, XmNactivateCallback, GoBackCB, (XtPointer)*multiText);

    /*
     * Create a popup menu (only if it's not Sun's X system, since pop-up's
     * tend to hang the session).
     */
    if (strstr(ServerVendor(XtDisplay(scrolledWindow)), "X11/NeWS"))
    {
	menuShell = NULL;
    }
    else
    {
	argcnt = 0;
	XtSetArg(args[argcnt], XmNwidth,  1); argcnt++;
	XtSetArg(args[argcnt], XmNheight, 1); argcnt++;
	menuShell = DXCreatePopupMenu(scrolledWindow);
    }

    /*
     * Save info in the client data.
     */
    userdata = (UserData*)XtMalloc(sizeof(UserData));
    userdata->filename[0] = '\0';
    userdata->label[0] = '\0';
    userdata->swin = scrolledWindow;
    userdata->hbar = hbar;
    userdata->vbar = vbar;
    userdata->toplevel = parent;
    userdata->goback = goBackButton;
    userdata->historylist = NewHistoryList();
    userdata->spotlist = NewList();
    userdata->fontstack = NULL;
    userdata->colorstack = NULL;
    userdata->linkType = NOOP;
    userdata->menushell = menuShell;
    userdata->getposition = FALSE;
    userdata->mapped = TRUE;

    /*
     * Save away the client data.
     */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNuserData,userdata); argcnt++;

    XtSetValues(*multiText, args, argcnt);
    return (frame);
}


void FreeWidgetData (Widget w)
{
 Arg    args[10];
 int    argcnt,i,len;
 UserData *userdata;

  argcnt = 0;
  XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
  XtGetValues(w,args,argcnt);
  if (userdata == NULL) printf("Userdata == NULL \n\n");

  if (userdata->historylist != NULL) {
     len = userdata->historylist->length;
     for (i = 0; i < len ; i++)
       DeleteLastNode(userdata->historylist);
  }
  printf("lenght history = %d \n",userdata->historylist->length);

  FreeSpotList(userdata->spotlist);
  FreeStack(userdata->fontstack);
  FreeStack(userdata->colorstack);
  userdata->fontstack = NULL;
  userdata->colorstack = NULL;
}



HelpIndex *MakeIndexEntry (char *File, char *str)
{
 HelpIndex *Index;
 
 Index = (HelpIndex *) XtMalloc(sizeof(HelpIndex));
 Index->FileName = File;
 Index->String  = str;
 Index->Font = DEFAULT_INDEX_FONT;
 Index->Color = DEFAULT_INDEX_COLOR;
 return(Index);
}
 
void FreeIndexEntry(Index)
HelpIndex *Index;
{
  XtFree((char*)Index);
}

unsigned long
COLOR(w, name)
Widget w;
char *name;
{
XrmValue        fromVal, toVal;
unsigned long *pixel;

    fromVal.size = sizeof(char*);
    fromVal.addr = name;

    XtConvert(w, XmRString, &fromVal, XmRPixel, &toVal);
    pixel = (unsigned long*)toVal.addr;
    if( pixel == NULL )
    {
	fromVal.addr = XtDefaultBackground;
	XtConvert(w, XmRString, &fromVal, XmRPixel, &toVal);
	pixel = (unsigned long*)toVal.addr;
    }
    return(*pixel);
}

#ifdef DXD_NEEDS_MKTEMP

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

char *mktemp(char *tmplate)
{
  int i;
  int NX=0;
  int tmplength;
  int random;
  int MAX_TRIES=5;
  int numtry;
  char numbuff[10];
  struct stat buf;
  double ctime, dummy;
  int inttime;

  if (tmplate)
    if ((tmplength=strlen(tmplate)) > 0)
      {
        for (i=tmplength; i>=1; i--)
          if (tmplate[i-1] != 'X')
            {
              NX=tmplength-i;
              break;
            }
        if (NX!=0)
          {
            for (numtry=0; numtry<MAX_TRIES; numtry++)
              {
                ctime = (double) clock();
                inttime = (int) floor(modf(ctime/10000.0,&dummy)*10000.0);
                srand(inttime);
                random = rand();
                sprintf(numbuff,"%d",random);
                for (i=0; i<NX && i<strlen(numbuff); i++)
                  if (numbuff[i] != ' ')
                    tmplate[tmplength-i-1] = numbuff[i];
                if (STATFUNC(tmplate,&buf) != 0)
                  return (tmplate);
              }
            return(NULL);
          }
         else
          if (STATFUNC(tmplate,&buf) != 0)
            return (tmplate);
           else
            return(NULL);
      }
  return(NULL);
}
#endif
