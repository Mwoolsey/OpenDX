/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>

#define MAXOBJECTS 21    /* must match mdf-c generated dx.mdf file */

int
m_Collect(Object *in, Object *out)
{
    int i;
    int seen_null = 0;
    int say_warn = 0;

    /* always make a new generic group, regardless of the inputs */
    out[0] = (Object)DXNewGroup();
    if (!out[0])
	return ERROR;

    /* count up, stopping at the first non-NULL input */
    for (i=0; i<MAXOBJECTS && !in[i]; i++)
	;

    /* if there are no args, return a null group and warn */
    if (i == MAXOBJECTS) {
	/* DXWarning("#4014", "group");  -* creating an empty group */
	return OK;
    }


    /* complain if there are valid objects which follow null objects,
     *  but collect them anyway.
     */
    for (i=0; i<MAXOBJECTS; i++) {

	if (!in[i]) {
	    seen_null = 1;
	    continue;
	}
	
	if (seen_null) {
            say_warn = 1;
	    seen_null = 0;
	}

	if (!DXSetMember((Group)out[0], NULL, in[i]))
	    goto error;
	
    }

    if (say_warn)
	; /* DXWarning("#4010");   -* valid objects following NULL object */

    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}

