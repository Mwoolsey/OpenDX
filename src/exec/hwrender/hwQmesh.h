/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmQmesh_h
#define tdmQmesh_h
/*
 *
 * $Header: /src/master/dx/src/exec/hwrender/hwQmesh.h,v 1.3 1999/05/10 15:45:35 gda Exp $
 */


/*
^L
 *===================================================================
 *      Structure Definitions and Typedefs
 *===================================================================
 */



#if defined(alphax)
#define MaxQstripSize 60
#else
#define MaxQstripSize 100
#endif
#define MinQstripAvg   5


#define Down  0
#define Up    1
#define Left  2
#define Right 3

typedef struct _Qstrip
  {
  int points;
  int *point;
  } Qstrip;


typedef struct _Qmesh
  {
  int qstrips;
  Qstrip **qstrip;
  } Qmesh;



#endif 

