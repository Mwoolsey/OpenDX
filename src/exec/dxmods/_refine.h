/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_refine.h,v 1.1 2000/08/24 20:04:18 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  __REFINE_H_
#define  __REFINE_H_

#include <dx/dx.h>

/* from _refine.c */
Object _dxfRefine( Object, int );
Object _dxfChgTopology( Object, char * );

/* from _refinereg.c */
Field  _dxfRefineReg(Field, int);

/* from _refineirr.c */
Field _dxfRefineIrreg(Field, int);


#endif /* __REFINE_H_ */


