/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define PNTPTS 6 
#define PNTLNS 3 
static Point points[PNTPTS] = {
 { -0.5000000000F,      0.0000000000F,      0.0000000000F }, 
 {  0.5000000000F,      0.0000000000F,      0.0000000000F },
 {  0.0000000000F,     -0.5000000000F,      0.0000000000F },
 {  0.0000000000F,      0.5000000000F,      0.0000000000F },
 {  0.0000000000F,      0.0000000000F,     -0.5000000000F },
 {  0.0000000000F,      0.0000000000F,      0.5000000000F }
};

#include <dxconfig.h>

   
static Line lines[PNTLNS] = {
  { 0,   1 },
  { 2,   3 },
  { 4,   5 }
};

