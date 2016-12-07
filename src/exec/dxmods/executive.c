/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/***
MODULE:
 Executive
SHORTDESCRIPTION:
 Execute an executive command
CATEGORY:
 Debugging
INPUTS:
    input;     String;  (none); Command to be executed
    input;     Object;  (none); Command-dependent
REPEAT: 1
OUTPUTS:
END:
***/

#include <string.h>
#include <dx/dx.h>

extern Error _dxf_ExExecCommand(Object *in); /* from dpexec/command.c */

Error m_Executive(Object *in, Object *out)
{
    return (_dxf_ExExecCommand (in));
}
