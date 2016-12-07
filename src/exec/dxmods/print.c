/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/print.c,v 1.3 1999/05/10 15:45:29 gda Exp $
 */

#include <dxconfig.h>


#include <ctype.h>
#include <dx/dx.h>



char defopts[] = "o";


/* 0 = input
 * 1 = flags: r=recurse, o=object only, d=data, x=expanded, #=number of levels
 * 2 = component namelist - restrict field printing to just these ones
 */
int
m_Print(Object *in, Object *out)
{
    int i = 0, nstrings = 0;
    char *cp, **comps = NULL;
    char *opt = defopts;


    if(!in[0]) {
	DXMessage("Null Object.");
	return OK;
    }

    if(in[1] && !DXExtractString(in[1], &opt))
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad options string");
    
    if(in[2]) {
	nstrings = 0;
	while(DXExtractNthString(in[2], nstrings, &cp))
	    nstrings++;
	
	if(nstrings == 0)
	    DXErrorReturn(ERROR_BAD_PARAMETER, "bad component name(s)");

	if((comps = (char **)DXAllocate(sizeof(char *) * (nstrings+1))) == NULL)
	    return ERROR;

	for(i=0; i<nstrings; i++)
	    DXExtractNthString(in[2], i, &comps[i]);

	comps[i] = NULL;
    }


    DXPrintV(in[0], opt, comps);
    DXFree((Pointer)comps);

    return (DXGetError()==ERROR_NONE ? OK : ERROR);
}

