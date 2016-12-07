/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include <string.h>
#include "help.h"
#include "_macro.h"
#include "parse.h"
#include "log.h"
#include "graph.h"

Error _dxf_ExInitHelp ()
{
    return (OK);
}

#define	ADVANCE(_bp)	while (*(_bp)) (_bp)++
#define	ADDCOMMA(_bp)	{strcpy (_bp, ", "); ADVANCE (_bp);}

char *_dxf_ExHelpFunction (char *f)
{
    node		*function;
    node		*name;
    char		buffer[8192];
    char		*bufp = buffer;
    char		*ret;
    char		*type;
    int			i;
    int			nin;
    int			nout;

    function = (node *) _dxf_ExMacroSearch (f);

    /*
     * We didn't find the function requested.
     */

    if (function == NULL || function->v.module.def.func == m__badfunc)
    {
	sprintf (buffer, "\nThe function %s is not defined\n\n", f);
	ret = (char *) DXAllocate (strlen (buffer) + 1);
	if (ret)
	    strcpy (ret, buffer);
	return (ret);
    }

    switch (function->type)
    {
	case NT_MACRO:	type = "macro";		break;
	case NT_MODULE:	type = "module";	break;
	default:	type = "function";	break;
    }

    sprintf (bufp, "\nThe %s %s has the following usage:\n\n", type, f);
    ADVANCE (bufp);

    nin  = function->v.function.nin;
    nout = function->v.function.nout;

    for (i = 0, name = function->v.function.out; name; i++, name = name->next)
    {
	strcpy (bufp, name->v.id.id);
	ADVANCE (bufp);

	if (i < nout - 1)
	    ADDCOMMA (bufp);
    }

    if (nout)
    {
	strcpy (bufp, " = ");
	ADVANCE (bufp);
    }

    strcpy (bufp, f);
    ADVANCE (bufp);
    strcpy (bufp, " (");
    ADVANCE (bufp);

    for (i = 0, name = function->v.function.in; name; i++, name = name->next)
    {
	strcpy (bufp, name->v.id.id);
	ADVANCE (bufp);

	if (i < nin - 1)
	    ADDCOMMA (bufp);
    }

    strcpy (bufp, ");");
    ADVANCE (bufp);

    strcpy (bufp, "\n\n");
    ADVANCE (bufp);

    EXO_delete ((EXO_Object) function);

    ret = (char *) DXAllocate (strlen (buffer) + 1);
    if (ret)
	strcpy (ret, buffer);
    return (ret);
}
