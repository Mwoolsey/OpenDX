/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_normals.h,v 1.4 2000/08/24 20:04:16 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  __NORMALS_H_
#define  __NORMALS_H_

#include <dx/dx.h>

#define TRUE                    1
#define FALSE                   0

#define ONE_THIRD               0.33333333333333333333
#define ONE_FOURTH              0.25

#define ABS(x)                  (((x)< 0.0)?(-(x)):(x))


Object _dxfNormalsObject(Object, char *);
Object _dxfNormals(Object, char *);
Array _dxf_FLE_Normals(int *, int, int *, int, int *, int, float *, int);

#endif /* __NORMALS_H_ */

