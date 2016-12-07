/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/separate.h,v 1.1 2000/08/24 20:04:47 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  _SEPARATE_H_
#define  _SEPARATE_H_

#include <dx/dx.h>

Error _dxfnewvalue(float *value, float old_min, float old_max,
                   float min, float max, int remap,int *ip);
Error _dxfinteract_float(char *, Object, float *, float *,
           float *, int *, char *, int,
	   method_type, int *, int []);
int   EndCheck(struct einfo *);

#endif /* _SEPARATE_H_ */

