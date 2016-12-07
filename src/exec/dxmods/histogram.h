/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/histogram.h,v 1.1 2000/08/24 20:04:36 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  _HISTOGRAM_H_
#define  _HISTOGRAM_H_

#include <dx/dx.h>

/* this routine could be in libdx */
Object _dxfHistogram(Object o, int bins, float *min, float *max, int inout);

#endif /* _HISTOGRAM_H_ */

