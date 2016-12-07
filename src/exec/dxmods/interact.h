/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/interact.h,v 1.8 2006/06/13 15:08:35 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _INTERACT_H_
#define _INTERACT_H_

#include <dx/dx.h>
#if HAVE_VALUES_H
#include <values.h>
#endif
#include <math.h>

#define CLAMP(c,a,b) ((c < a) ? a: ((c > b) ? b: c))
#define CLAMPMIN(c,a) ((c < a) ? a: c)
#define CLAMPMAX(c,b) ((c > b) ? b: c)
#define OUTOFRANGE(c,a,b) ((c < a) ? 1: ((c > b) ? 1: 0))
#define MAXPRINT 7
/* MAX_MSGLEN should be less than or equal to MSG_BUFLEN in dpexec/distp.h */
#define MAX_MSGLEN 4000
#define SLOP 64
#define AtEnd(p) ((p)->atend)

#define NUMBER_CHARS 16   	/* number of print characters for each number*/
#define NAME_CHARS   10		/* number of print characters for each name */
#define METHOD_CHARS 10     	/* number of print characters for method */

#if (defined(intelnt) || defined(WIN32)) && defined(ABSOLUTE)
#undef	ABSOLUTE
#endif

typedef enum {
        PERCENT_ROUND = 1,
        PERCENT = 2,
        ABSOLUTE = 3
} method_type;

typedef enum {
		START_MINIMUM = 1,
		START_MIDPOINT = 2,
		START_MAXIMUM = 3
} start_type;

struct einfo {
    int maxlen;
    int atend;
    char *mp;
    char *msgbuf;
};

Error _dxfinteract_float(char *, Object,float *, float *,
                         float *,int *,char *,int,method_type,int *,int *);
Error _dxfprint_message(Pointer ,struct einfo *,Type ,int ,int *,int);
int _dxfcheck_obj_cache(Object ,char *,int ,Object);

#endif /* _INTERACT_H_ */
