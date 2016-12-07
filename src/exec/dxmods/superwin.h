/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/superwin.h,v 1.4 2000/08/24 20:04:52 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _SUPERWIN_H_
#define _SUPERWIN_H_

#include <dx/error.h>

#define WINDOW_CLOSED	    0
#define WINDOW_OPEN	    1
#define WINDOW_OPEN_RAISED  2

typedef struct _imageWindow
{
    char 	        *displayString;
    char 	  	*mod_id;
    int		  	depth;
    int		  	size[2], sizeFlag;
    int		  	offset[2], offsetFlag;
    struct _iwx	  	*iwx;
    Array		events;
    int			state[3];
    int			x, y, time;
    int			parentId;
    int			pickFlag;
    char		*title;
    char		*cacheTag;
    int			map;
    Object		object;
    char		*where;
    int			decorations;
    int			kstate;
} ImageWindow;

Error saveEvent(ImageWindow *, int, int);
int   _dxf_mapSupervisedWindow(char *, int);

/* from superwinX.c */
int   _dxf_getWindowId(ImageWindow *);
int   _dxf_getParentSize(char *, int, int *, int *);
void  _dxf_setWindowSize(ImageWindow *, int *);
void  _dxf_setWindowOffset(ImageWindow *, int *);
int   _dxf_mapWindowX(ImageWindow *, int);
int   _dxf_checkDepth(ImageWindow *, int);
int   _dxf_createWindowX(ImageWindow *, int);
void  _dxf_deleteWindowX(ImageWindow *);
int   _dxf_samplePointer(ImageWindow *, int);


#endif /* _SUPERWIN_H_ */

