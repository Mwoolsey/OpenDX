/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_plot.h,v 1.1 2000/08/24 20:04:17 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  __PLOT_H_
#define  __PLOT_H_

#include <dx/dx.h>

void _dxfaxes_delta(float *, float *, float, int *, 
           float *, float *, char *, int *, int, int);
int _dxfHowManyTics(int, int, float, float, float, float, int,
	   int *, int *); 


#endif /* _PLOT_H_ */

