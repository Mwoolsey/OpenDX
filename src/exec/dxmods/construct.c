/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 */

#include <dxconfig.h>


#include <stdio.h>
#include <dx/dx.h>
#include "_construct.h"


int
m_Construct(Object *in, Object *out)
{
    out[0] = (Object)_dxfConstruct((Array)in[0], (Array)in[1],
					(Array)in[2], (Array)in[3]);
    if (! out[0])
	return ERROR;
    else
       return OK;
}
