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
#include <Xm/Xm.h>
#include <Xm/MenuShell.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <X11/Shell.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>

#include "../widgets/MultiText.h"
#include "help.h"

extern Bool HelpOn(Widget, int, char *, char *, int); /* from help.c */
extern unsigned long COLOR();


/*****************************************************************************/
/* NewHistoryList -							     */
/*									     */
/*									     */
/*****************************************************************************/

HistoryList* NewHistoryList()
{
    HistoryList *list;
  
    list = (HistoryList*)malloc(sizeof(HistoryList));

    list->head    = NULL;
    list->tail    = NULL;
    list->current = NULL;
    list->length  = 0;
    return(list);
}


/*****************************************************************************/
/* _InsertHistoryList -							     */
/*									     */
/*									     */
/*****************************************************************************/

static
void _InsertHistoryList(HistoryList*     list,
			HistoryNodeType* newNode)
{
    ASSERT(list);
    ASSERT(newNode);

    if (list->head == NULL)
    {
	newNode->next = NULL;
	newNode->prev = NULL;
	list->head    = newNode;
	list->tail    = newNode;
	list->current = newNode;
    }
    else
    {
	list->tail->next = newNode;
	newNode->next    = NULL;
	newNode->prev    = list->tail; 
	list->tail       = newNode;
    }
    list->length++;
}


/*****************************************************************************/
/* DeleteLastNode -							     */
/*									     */
/*									     */
/*****************************************************************************/

void DeleteLastNode(HistoryList* list)
{
    ASSERT(list);

    if (list->head == NULL)
	return;

    if (list->tail->prev == NULL)
    {
	XtFree((char *)list->tail);
	list->head    = NULL;
	list->tail    = NULL;
	list->current = NULL;
    }
    else
    {
	list->current = list->tail->prev;
	if (list->tail->prev->button)
	{
	    XtDestroyWidget(list->tail->prev->button);
	    list->tail->prev->button = NULL;
	}

	XtFree((char *)list->tail);
	list->tail       = list->current;
	list->tail->next = NULL;
    }
    list->length--;
}


/*****************************************************************************/
/* _SearchHistoryList -							     */
/*									     */
/*									     */
/*****************************************************************************/

static
int _SearchHistoryList(HistoryList* list,
		       Widget       widget)
{
    HistoryNodeType* ref;
    int              index;

    ASSERT(list);
    ASSERT(widget);

    ref   = list->head;
    index = 0;
  
    while (ref != NULL)
    {
	if (ref->button == widget)
	{
	    return(index);
	}
	index++;
	ref = ref->next;
    }

    return -1;  /* node not found */ 
}   
  

/*****************************************************************************/
/* _PostHistoryMenu -							     */
/*									     */
/*									     */
/*****************************************************************************/

static
void _PostHistoryMenu(Widget        w,
		      XtPointer     closure,
		      XEvent* event,
		      Boolean       *continue_to_dispatch)
{
    Widget menu = (Widget) closure;
    XButtonEvent *bevent = (XButtonEvent*)event;
    int button;
    int n;
    Arg args[2];


    n = 0;
    XtSetArg(args[n], XmNwhichButton, &button); n++;
    XtGetValues(menu, args, n);

    if (bevent->button == button)
    {
	XmMenuPosition(menu, bevent);
	XtManageChild(menu);
    }
    *continue_to_dispatch = True;
}


/*****************************************************************************/
/* CreatePopupMenu -							     */
/*									     */
/*									     */
/*****************************************************************************/

Widget DXCreatePopupMenu(Widget parent)
{
    Widget menuShell;
    Widget popupMenu;
    int    n;
    Arg    args[10];

    n = 0;
    XtSetArg(args[n], XmNwidth,  1); n++;
    XtSetArg(args[n], XmNheight, 1); n++;
    menuShell =
	XtCreatePopupShell
	    ("MenuShell", xmMenuShellWidgetClass, parent, args, n);
    XtManageChild(menuShell);

    n = 0;
    XtSetArg(args[n], XmNrowColumnType, XmMENU_POPUP); n++;
    popupMenu =
	XtCreateWidget
	    ("PopupMenu", xmRowColumnWidgetClass, menuShell, args, n);

    XtAddEventHandler
	(parent, ButtonPressMask, False, _PostHistoryMenu, (XtPointer)popupMenu);

    n = 0;
	XtCreateManagedWidget
	    ("HistoryMenuLabel", xmLabelWidgetClass, popupMenu, args, n);
	XtCreateManagedWidget
	    ("Separator", xmSeparatorWidgetClass, popupMenu, NULL, 0);

    return popupMenu;
}


/*****************************************************************************/
/* _PopMenu -								     */
/*									     */
/*									     */
/*****************************************************************************/

static
void _PopMenu(Widget  w,
	      XtPointer closure,
	      XtPointer call_data)
{
    Widget       mtext = (Widget)closure;
    UserData*    userdata;
    int          index;
    int          limit;
    int          i;
    int          position=0;
    int          argcnt;
    Arg          args[2];

    argcnt = 0;
    XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
    XtGetValues(mtext,args,argcnt);

    index = _SearchHistoryList(userdata->historylist, w);
    if (index == -1)
	return;

    if (userdata->historylist->length == index + 1)
	return;

    limit =  userdata->historylist->length - index;
    for (i =  0; i < limit - 1; i++)
    {
	position = userdata->historylist->tail->position;
	DeleteLastNode(userdata->historylist);
    }

    if (userdata->historylist->length == 0)
    {
	XtWarning("No history available.");
	return;
    }
 
    HelpOn(mtext,
	   RETURN,
	   userdata->historylist->tail->filename,
	   userdata->historylist->tail->label,
	   position);
}


/*****************************************************************************/
/* PushMenu -								     */
/*									     */
/*									     */
/*****************************************************************************/

void PushMenu(Widget    outputWidget,
	      UserData* userdata,
	      LinkType  type,
	      char*     data)
{
    Widget           button = NULL;
    HistoryNodeType* menuDataPtr;
    static LinkType  oldType = NOOP;
    static char      oldData[255];
    Widget           menu;
    int		     argcnt;
    XmString         xmstr;
    Arg		     args[4];
    

    argcnt = 0;
    XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
    XtGetValues(outputWidget, args, argcnt);

    menu = userdata->menushell;

    oldData[0] = '\0';
    switch (type)
    {
      case NOOP:
	oldType = type;
	strcpy(oldData, data);
	break;

      case LINK:
      case FOOTNOTE:     
      case SPOTREF:
	/*
	 * If the code gets to this point the SPOTREF is across files 
	 * so the file should be added to the history menu.
	 */
	if (userdata->historylist->tail != NULL && menu)
	{
	    XtSetArg(args[0], XmNbackground, COLOR(menu, "#7e7eb4b4b4b4")); 
	    xmstr =
		XmStringCreateLtoR
		    (userdata->historylist->tail->label,
		     XmSTRING_DEFAULT_CHARSET);
	    XtSetArg(args[1], XmNlabelString, xmstr);
	    button =
		XtCreateManagedWidget
		    (userdata->historylist->tail->label, 
		     xmPushButtonWidgetClass,
		     menu,
		     args,
		     2);
	    userdata->historylist->tail->button = button;
	    XtAddCallback
		(button, XmNactivateCallback, _PopMenu, (XtPointer)outputWidget);
	    XmStringFree(xmstr);
	}   

	oldType = type;
	strcpy(oldData, data);
	break;

      case RETURN:
      default:
	break;
    }

    /*
     * Make the datastructure to hold the history data.
     */
    menuDataPtr = (HistoryNodeType*)XtMalloc(sizeof(HistoryNodeType));
    menuDataPtr->outputWidget = outputWidget;
    menuDataPtr->menu         = menu;
    menuDataPtr->button       = NULL;
    menuDataPtr->type         = oldType;
    XtSetArg(args[0], XmNvalue, &(menuDataPtr->position));
    XtGetValues(userdata->vbar,args,1);

    strcpy(menuDataPtr->filename, userdata->filename);
    strcpy(menuDataPtr->label, userdata->label);

    _InsertHistoryList(userdata->historylist, menuDataPtr);
}

