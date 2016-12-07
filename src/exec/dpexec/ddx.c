/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>

#if defined(DDX)
int GetMPINodeId() { return 0; }
int SlaveBcastLoop() { return 0; }
#endif
void GetBaseConnection(FILE **fptr, char **str)
{
    setvbuf (stdin, NULL, _IONBF, 0);
    *fptr = stdin;
    *str  = "STDIN";
}
