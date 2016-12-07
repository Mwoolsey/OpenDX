/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/rih.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _RIH_H
#define _RIH_H

#include <dxconfig.h>
#include <dx/dx.h>
#include "utils.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct
{
    PFE         proc;
    int         fd;
    Pointer     arg;
    PFI         check;
    int         flag;
    int         isWin;
        void    *dpy;
} _EXRIH, *EXRIH;

typedef struct
{
    PFE         proc;
    Pointer     arg;
    int         when;
} _EXRCH, *EXRCH;

int   _dxf_ExCheckRIH (void);
int _dxf_ExCheckRIHBlock (int input);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _RIH_H */
