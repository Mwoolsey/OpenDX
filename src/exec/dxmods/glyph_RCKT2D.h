/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define RCKT2DPTS 7 
#define RCKT2DTRS 3 

#include <dxconfig.h>


static Point points[RCKT2DPTS] = {
 {  0.2000000000F,      0.0000000000F,    0.0F },
 { -0.2000000000F,      0.0000000000F,    0.0F }, 
 {  0.2000000000F,      0.7000000000F,    0.0F },  
 { -0.2000000000F,      0.7000000000F,    0.0F },  
 {  0.6000000000F,      0.7000000000F,    0.0F },  
 { -0.6000000000F,      0.7000000000F,    0.0F },  
 {  0.0000000000F,      1.0000000000F,    0.0F }
};
   
static Triangle triangles[] = {
  {  0,       1,      2 }, 
  {  1,       3,      2 }, 
  {  4,       5,      6 }
};
