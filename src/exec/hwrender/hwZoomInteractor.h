/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 *  Zoom interactor private data structure.
 */

typedef struct {
  /* zoom center in pixels */
  int cx, cy ;
  /* zoom box coordinates in pixels */
  int x1, x2, y1, y2 ;
  /* button stroke started with */
  int btn ;
  /* previous mouse position */
  int px, py ;
  /* frame buffer configuration */
  int displaymode, buffermode ;
} tdmZoomData ;

