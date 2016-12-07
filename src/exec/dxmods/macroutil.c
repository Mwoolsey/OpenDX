/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "dx/dx.h"

#include <dxconfig.h>
#include "macroutil.h"

Error
m_MacroStart(Object *in, Object *out)
{
    int i = 0, j;

    if (in[0])
	if (! DXExtractInteger(in[0], &i))
	{
	    DXSetError(ERROR_BAD_PARAMETER, 
		"first argument of MacroStart must be integer");
	    return ERROR;
	}
    
    for (j = 0; j < i; j++)
	out[j] = in[j+1];
    
    return OK;
}


Error
m_MacroEnd(Object *in, Object *out)
{
    int i = 0, j;

    if (in[0])
	if (! DXExtractInteger(in[0], &i))
	{
	    DXSetError(ERROR_BAD_PARAMETER, 
		"first argument of MacroStart must be integer");
	    return ERROR;
	}
    
    for (j = 0; j < i; j++)
	out[j] = DXCopy(in[j+1], COPY_HEADER);
    
    return OK;
}
