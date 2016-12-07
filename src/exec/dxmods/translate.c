/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/translate.c,v 1.4 2000/08/24 20:04:53 davidt Exp $
 */

#include <dxconfig.h>

#include <dx/dx.h>


int
m_Translate(Object *in, Object *out)
{
    Vector v;

    out[0] = NULL;
    
    if(!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }
    
    if(!in[1]) {	
	out[0] = (Object)DXNewXform(in[0], DXTranslate(DXVec(0.0, 0.0, 0.0)));
	return(out[0] ? OK : ERROR);
    }	

    if(!DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&v)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10230", "translation", 3);
	return ERROR;
    }
	
    out[0] = (Object)DXNewXform(in[0], DXTranslate(v));

    return(out[0] ? OK : ERROR);
}




