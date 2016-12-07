/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "config.h"
#include "pmodflags.h"
#include "d.h"
#include "_macro.h"
#include "_variable.h"
#include "attribute.h"
#include "graph.h"
#include "parse.h"
#include "path.h"
#include "utils.h"
#include "log.h"
#include "graphIntr.h"

#define	MACRO_DEPTH	512
#define	MACRO_QUIT	MACRO_DEPTH

/*
 * These can be statics since graph construction takes place on a single
 * processor.
 *
 * $$$$$ If we need to save some space we can dynamically allocate
 * $$$$$ cells to hold the names etc. but at a performance penalty.
 */

static char	*_macro_stack[MACRO_DEPTH];
static int	_macro_depth = 0;

static int	_nocache_depth = 0;
static int	_nocache_stack[MACRO_DEPTH];

int
_dxf_ExNoCachePush (int n)
{
    int		nc	= FALSE;

    if (_nocache_depth > 0)
    {
	if (_nocache_depth < MACRO_DEPTH)
	    nc = _nocache_stack[_nocache_depth - 1];
	else
	    nc = _nocache_stack[MACRO_DEPTH - 1];
    }

    nc = nc || n;

    if (_nocache_depth < MACRO_DEPTH)
	_nocache_stack[_nocache_depth++] = nc;
    else
	_nocache_depth++;

    return (nc);
}


void
_dxf_ExNoCachePop ()
{
    if (--_nocache_depth < 0)
	_nocache_depth = 0;
}

/*
 * Determines whether a recursive macro call has been made.  If so
 * generates an error message, if not pushes the macro name onto the 
 * stack.
 */

#define	ADVANCE(_bp)	while (*(_bp)) (_bp)++
#define	ADDARROW(_bp)	{strcpy (_bp, " -> "); ADVANCE (_bp);}

int _dxf_ExMacroRecursionCheck (char *name, _ntype type)
{
    int		i;
    int		j;
    char	*warning = "Macro recursion:  ";
    char	*message;
    char	*mptr;
    int		len;
    int		d;

    if (type != NT_MACRO)
	return (FALSE);
    
    d = _macro_depth < MACRO_DEPTH ? _macro_depth : MACRO_DEPTH;

    for (i = 0; i < d; i++)
    {
	if (name == _macro_stack[i] && ! strcmp (name, _macro_stack[i]))
	{
	    /* Figure out how much space we need for the message */
	    for (j = 0, len = 0; j < d; j++)
		len += strlen (_macro_stack[j]);

	    len += strlen (warning);	/* for inital message		*/
	    len += 4 * _macro_depth;	/* for ' -> '			*/
	    len += strlen (name);	/* for bad guy			*/
	    len++;			/* for '\000'			*/

	    if ((mptr = message = DXAllocate (len)) == NULL)
		DXUIMessage ("ERROR", "%s%s", warning, name);
	    else
	    {
		strcpy (mptr, warning);
		ADVANCE (mptr);

		for (j = 0; j < d; j++)
		{
		    strcpy (mptr, _macro_stack[j]);
		    ADVANCE (mptr);
		    ADDARROW (mptr);
		}

		strcpy (mptr, name);
		ADVANCE (mptr);
		*mptr = '\000';

		DXUIMessage ("ERROR", message);
		DXFree ((Pointer) message);
	    }

	    return (TRUE);
	}
    }

    if (_macro_depth >= MACRO_QUIT)
    {
	DXUIMessage ("ERROR", "Macro depth = %d, assuming macro recursion", _macro_depth);
	return (TRUE);
    }

    if (_macro_depth < MACRO_DEPTH)
	_macro_stack[_macro_depth++] = name;
    else
	_macro_depth++;

    return (FALSE);
}

/*
 * Clears the macro recursion checking structure.  Shouldn't be
 * necessary but just in case something left us in a funky state.
 */

void _dxf_ExMacroRecursionInit ()
{
    _macro_depth = 0;
    _nocache_depth = 0;
}

/*
 * Pops one level of macro call from the recursion check.
 */

void _dxf_ExMacroRecursionPop (char *name, _ntype type)
{
    if (type != NT_MACRO)
	return;
    
    if (--_macro_depth < 0)
	_macro_depth = 0;
}




void _dxf_ExPrintNode(node*n)
{
    node *sn;

    if (n == NULL)
    {
	DXMessage("NULL");
	return;
    }
    switch(n->type) {
    case NT_MACRO:
	DXMessage("macro");
	_dxf_ExPrintNode(n->v.macro.id);
	DXMessage("\t(");
	for (sn = n->v.macro.in; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	DXMessage("\t) -> (");
	for (sn = n->v.macro.out; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	DXMessage("\t)");
	DXMessage("{");
	for (sn = n->v.macro.def.stmt; sn; sn = sn->next)
	{
	    _dxf_ExPrintNode(sn);
	    DXMessage(";");
	}
	DXMessage("}");
	break;

    case NT_MODULE:
	for (sn = n->v.module.out; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	DXMessage("=");
	_dxf_ExPrintNode(n->v.module.id);
	DXMessage("(");
	for (sn = n->v.module.in; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	DXMessage(")");
	break;

    case NT_ASSIGNMENT:
	for (sn = n->v.assign.lval; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	DXMessage("=");
	for (sn = n->v.assign.rval; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	break;

    case NT_PRINT:
	DXMessage("NT_PRINT");
	break;

    case NT_ATTRIBUTE:
	DXMessage("NT_ATTRIBUTE");
	break;

    case NT_CALL:
	_dxf_ExPrintNode(n->v.call.id);
	DXMessage("(");
	for (sn = n->v.call.arg; sn; sn = sn->next)
	    _dxf_ExPrintNode(sn);
	DXMessage(")");
	break;

    case NT_ARGUMENT:
	_dxf_ExPrintNode(n->v.arg.val);
	break;

    case NT_LOGICAL:
	DXMessage("NT_LOGICAL");
	break;

    case NT_ARITHMETIC:
	DXMessage("NT_ARITHMETIC");
	break;

    case NT_CONSTANT:
	DXMessage("NT_CONSTANT");
	break;

    case NT_ID:
	if (n->v.id.dflt != NULL)
	{
	    DXMessage("%s = ", n->v.id.id);
	    _dxf_ExPrintNode(n->v.id.dflt);
	}
	else
	{
	    DXMessage("%s", n->v.id.id);
	}
	break;

    case NT_EXID:
	DXMessage("NT_EXID");
	break;

    case NT_BACKGROUND:
	DXMessage("NT_BACKGROUND");
	break;

    case NT_PACKET:
	DXMessage("NT_PACKET");
	break;

    case NT_DATA:
	DXMessage("NT_DATA");
	break;

    default:
	DXMessage("Unknown type %d", n->type);
	break;
    }
}
