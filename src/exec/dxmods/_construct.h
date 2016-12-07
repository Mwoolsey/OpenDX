/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_construct.h,v 1.1 2000/08/24 20:04:10 davidt Exp $
 */

#include <dxconfig.h>


#ifndef  __CONSTRUCT_H_
#define  __CONSTRUCT_H_

#include <dx/dx.h>

Array _dxfReallyCopyArray(Array);
Field _dxfConstruct(Array, Array, Array, Array);

#endif /* __CONSTRUCT_H_ */

