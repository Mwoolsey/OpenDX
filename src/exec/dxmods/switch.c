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
    Switch
SHORTDESCRIPTION:
    Passes the nth input object or NULL if the selector is not in valid range.
CATEGORY:
    Special
INPUTS:
    $Select;	Integer;	0;	The selector value
    ...;	Object;		NULL;	The input object list
OUTPUTS:
    Output;   	Object; 	NULL;	The nth input object or NULL
FLAGS:
    MODULE_SWITCH
BUGS:
AUTHOR:
    David A. Epstein
END:
***/

#include <dx/dx.h>
#define NARGS 22

Error
m_Switch (Object *in, Object *out)
{
/* Switch module now handled entirely by the executive.  */
    DXSetError(ERROR_INTERNAL, "#8015", "Switch");
    return(ERROR);

#if 0
    int i;

    if (in[0] == NULL)
    {
	out[0] == NULL;
	return(OK);
    }
    if (DXExtractInteger(in[0], &i) == ERROR)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10010", "switch value");
	return(ERROR);
    }
    if (i <= 0 || i >= NARGS)
    {
	out[0] == NULL;
	return(OK);
    }
    else
    {
	out[0] = in[i];
	return(OK);
    }
#endif
}

