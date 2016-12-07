/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _CLIPNOTIFY_H_
#define _CLIPNOTIFY_H_
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
/*
 *-----------------------------------------------------------------------------
 */
int XClipNotifyGetVersion(
    Display *dpy, int major_version_return, int minor_version_return);
/*
 *      Gets the major and minor version numbers of the extension.  The return
 *      value is zero if an error occurs or non-zero if no error happens.
 *
 *-----------------------------------------------------------------------------
 */
int XClipNotifyGetLUN(Display *dpy, unsigned char *lun);
/*
 *      Gets the number of the HPPI LUN for this display
 *
 *-----------------------------------------------------------------------------
 */
int XClipNotifyGetTransparent(Display *dpy, unsigned char *pixel);
/*
 *      Gets the number of the transparent pixel for this display
 *
 *-----------------------------------------------------------------------------
 */
void XClipNotifyAddWin(Display *dpy, Window w);
/*
 *      Request notification of any changes made in this window's cliplist.
 *      Add this client to the list of clients to be notified when this
 *      window's cliplist changes.
 *
 *-----------------------------------------------------------------------------
 */
void XClipNotifyRemoveWin(Display *dpy, Window w);
/*
 *      Request no further notification of any changes made in this
 *      window's cliplist. Remove this client from the list of cleints to be
 *      notified when this window's cliplist changes.
 *
 *-----------------------------------------------------------------------------
 */
void XClipNotifyRemoveAll(Display *dpy);
/*
 *      Request no further notification of any changes made to
 *      all the window's cliplists previously requested. Remove this client
 *      all windows' clipnotify lists.
 *
 *-----------------------------------------------------------------------------
 */
void XClipNotifyInitHPPI (Display *dpy);
/*
 *      Request initialization of the HPPI interface card to receive data.
 *
 *-----------------------------------------------------------------------------
 */

#define CLIPNOTIFY_PROTOCOL_NAME "ClipNotify"

#define CLIPNOTIFY_MAJOR_VERSION        1       /* current version numbers */
#define CLIPNOTIFY_MINOR_VERSION        0

/*
 * ClipNotify request type values
*/
#define X_ClipNotifyGetVersion          0
#define X_ClipNotifyGetLUN              1
#define X_ClipNotifyGetTransparent      2
#define X_ClipNotifyAddWin              3
#define X_ClipNotifyRemoveWin           4
#define X_ClipNotifyRemoveAll           5
#define X_ClipNotifyInitHPPI            6
#define X_ClipNotifyStopHPPI            7
#define X_ClipNotifyStartHPPI           8

/*
 * ClipNotify event type values
*/
#define ClipNotifyEvent                 0
#define ClipNotifyHdrError              1
#define ClipNotifyDataErrors            2
#define ClipNotifyNumberEvents          (ClipNotifyDataErrors + 1)


#ifndef _CLIPNOTIFY_SERVER_

/*
 * Extra definitions that will only be needed in the client
 */

typedef struct {
    int type;               /* of event */
    unsigned long serial;   /* # of last request processed by server */
    int send_event;         /* true if this came frome a SendEvent request */
    Display *display;       /* Display the event was read from */
    Window  window;
    int     xorg, yorg;     /* screen adr of the window origin */
    int     x,y;            /* box upper left corner relative to the window */
    int     width,height;   /* box width and box height */
    int     count;          /* if non-zero, at least this many more */
} XClipNotifyEvent;
	
typedef struct {
    int type;               /* of event */
    unsigned long serial;   /* # of last request processed by server */
    int send_event;         /* true if this came frome a SendEvent request */
    Display *display;       /* Display the event was read from */
    Window  window;
} XClipNotifyHdrErrorEvent;

typedef struct {
    int type;               /* of event */
    unsigned long serial;   /* # of last request processed by server */
    int send_event;         /* true if this came frome a SendEvent request */
    Display *display;       /* Display the event was read from */
    Window  window;
    int     num_errors;     /* number of errors detected */
    int     etime;          /* elapses time interval in seconds */
} XClipNotifyDataErrorsEvent;

#endif /* _CLIPNOTIFY_SERVER_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CLIPNOTIFY_H_ */
