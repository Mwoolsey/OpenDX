/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define SQUAREPTS 4 
#define SQUARETRS 2 
static Point points[SQUAREPTS] = {
 {  -1.0000000000F,     -1.0000000000F,      0.0000000000F },
 {  -1.0000000000F,      1.0000000000F,      0.0000000000F },
 {   1.0000000000F,     -1.0000000000F,      0.0000000000F },
 {   1.0000000000F,      1.0000000000F,      0.0000000000F }
};
static Triangle triangles[] = {
 {   0,     3,     1 }, 
 {   0,     2,     3 }
};
