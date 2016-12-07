/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "defines.h"

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <stdio.h>
#include <Xm/Xm.h>
#include "../widgets/MultiText.h"
#include "help.h"

extern Bool HelpOn(Widget, int, char *, char *, int); /* from help.c */
extern void DeleteLastNode(HistoryList*);

/*--------------------------------------------------------------------------*
 |                                 LinkCB                                   |
 *--------------------------------------------------------------------------*/
void LinkCB (Widget w, XtPointer data, XmMultiTextLinkCallbackStruct *call)
{
  HelpOn(w, call->type,call->data,call->word,0);
}

/*--------------------------------------------------------------------------*
 |                              RemoveHelpIndexFileCB                       |
 | Remove the file that is created in /tmp when a help index is set up      |
 | by BuildHelpIndex().
 *--------------------------------------------------------------------------*/

void RemoveHelpIndexFileCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
 unlink((char *) client);
 XtFree(client);  /* free the memory that was allocated when the callback */
                  /* was set up                                           */
}

/*--------------------------------------------------------------------------*
 |                                  Close                                   |
 *--------------------------------------------------------------------------*/

void
CloseCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
UserData *userdata;
Arg         args[1];
int         argcnt;

  argcnt = 0;
  XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
  XtGetValues(w,args,argcnt);
  userdata->mapped = FALSE;
  XtUnmapWidget((Widget) client);
}




void DidDestroyCB (Widget w, Widget client, XtPointer call)
{
  printf("Destroy Callbacks were called...\n");
}


void KillCB (Widget w, Widget client, XtPointer call)
{
  XtDestroyWidget(client);
}




/*--------------------------------------------------------------------------*
 |                                 Destroy                                  |
 *--------------------------------------------------------------------------*/

void
DestroyCB(w, client, call)
Widget w;
XtPointer client;
XtPointer call;
{
Widget mtext = (Widget)client;
UserData *userdata;
Arg         args[1];
int         argcnt;

  argcnt = 0;
  XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
  XtGetValues(mtext,args,argcnt);
  userdata->mapped = FALSE;
#if (XmVersion >= 1001)
  if (XtParent(w))
    w = XtParent(w);
#endif
  XtUnmapWidget(w);
}

/*--------------------------------------------------------------------------*
 |                                 Go Back                                  |
 | The length of the list must be at least 2.                               |
 *--------------------------------------------------------------------------*/

void
GoBackCB(w, closure, call)
Widget w;
XtPointer closure;
XtPointer call;
{
 Widget      mtext = (Widget)closure;
 Arg         args[1];
 int         argcnt;
 UserData    *userdata;
 int         position;

  argcnt = 0;
  XtSetArg(args[argcnt], XmNuserData, &userdata); argcnt++;
  XtGetValues(mtext,args,argcnt);
  position = userdata->historylist->tail->position;
  DeleteLastNode(userdata->historylist);
  HelpOn(mtext,RETURN,userdata->historylist->tail->filename ,
                      userdata->historylist->tail->label,
                      position);
}

