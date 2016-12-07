/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_connectgrids.h,v 1.3 2002/03/21 21:20:06 rhh Exp $
 */

#include <dxconfig.h>


#ifndef  __CONNECTGRIDS_H_
#define  __CONNECTGRIDS_H_

#include <dx/dx.h>

Error _dxfConnectNearestObject(Object, Object, int, float *, float, Array);
Error _dxfConnectRadiusObject(Object, Object, float, float, Array);
Error _dxfConnectScatterObject(Object, Object, Array);

#endif /* __CONNECTGRIDS_H_ */

