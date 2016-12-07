/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Description:
 * This module defines operations on several basic types:
 * 1)  The gvar.   This is the executives basic handle for Objects.  It
 *     gets put in the cache, held on to by Programs, etc.
 * 2)  The Program.  It is the basic executable unit.  Parse trees are
 *     turned into a Program, which contains lists of:
 *     --  All inputs, outputs, and undefined variables
 *     --  All variables used as either the above or intermediates
 *     --  All functions (modules) that need execution
 * Also, it provides the ability to turn a parse tree into a Program.
 * _dxf_ExGraph is called with a parse tree.  It creates and returns a program
 * ExGraphTraverse traverses a parse tree, resolving the easy cases
 * ExGraphExpression handles the expression cases (x-1, ...)
 * ExGraphCall  handles calls to either Modules or Macros.
 */

#include <dx/dx.h>
#include "pmodflags.h"

#include "config.h"
#include "_macro.h"
#include "_variable.h"
#include "attribute.h"
#include "background.h"
#include "cache.h"
#include "graph.h"
#include "parse.h"
#include "utils.h"
#include "log.h"
#include "packet.h"
#include "path.h"
#include "context.h"
#include "graphIntr.h"
#include "obmodule.h"
#include "sysvars.h"
#include "distp.h"
#include "rq.h"
#include "function.h"

#define DICT_SIZE 64

static int graphing_error = FALSE;	/* error while constructing graph */
static int subgraph_id = 0;
EXDictionary _dxd_exGraphCache = NULL;
gfunc *_dxd_exCurrentFunc = NULL;

typedef struct indexobj {
    exo_object obj;
    int      index;
} indexobj;

typedef struct progobj {
    exo_object obj;
    Program    *p;
} progobj;

typedef LIST(int) list_int;

#ifdef ALLOC_UPFRONT
#define MAX_FREED 2
static Program *freedProgs[MAX_FREED] = {NULL};
#endif

#define REINIT_LIST(list) ( (list).nused = 0 )
/*
 * Stack of dictionaries used to resolve variable scope
 */

Group _dxfQueryImportInfo (char *filename);

static void ExMakeGfunc(node_function ndfunc, _ntype type, int instance, 
			_excache excache, node *n, node **inAttrs, Program *p,
                        int *inArgs, list_int *out, Program *macro);
static void ExCopySubP(Program *toP, Program *fromP, int top); 
static Program *GetNewMacro(Program *p, int *map, int *resolved);
static void ExGraphAssignment (Program *p, node *n, int top, 
			       EXDictionary dict);
static void ExGraphTraverse (Program *p, node *n, int top, EXDictionary dict);
static void ExGraphCall (Program *p, node *n, int top, list_int *out, 
			 EXDictionary dict, int *flags);
static void ExCreateSendModules (Program *p);  
static void ExBuildSendModule (Program *p, gfunc *sgf, gfunc *tgf, int srcfn,
			       int tgfn, int in_tab, int out_tab, int *outdex);
static int GvarDelete (gvar *p);
static int progobjDelete (progobj *p);
static void ExRemapVars(Program *p, Program *subP, int *map, int *resolved, char *fname, int inst);
static void ExFixAsyncVarName(Program *p,
			ProgramVariable *pv, char *fname, int instance);

static int _dxf_ExPathStringLen( ModPath *path );
static void _dxf_ExGFuncPathToStringBuf( gfunc *fnode, char *path_str );
static void _dxf_ExGFuncPathToCacheStringBuf( gfunc *fnode, char *path_str );
static void _dxf_ExPathSet( Program *p, char fname[], int instance, gfunc *fnode );
static void _dxf_ExPathPrepend( Program *p, char fname[], int instance,
                                gfunc *fnode );
static void _dxf_ExPathAppend( Program *p, char fname[], int instance,
                               gfunc *fnode );
static int _dxf_ExPathCacheStringLen( ModPath *path );

static char *_dxf_ExCacheStrPrepend( Program *p, char *name,
                               int instance, char *path );

Error m__badfunc(Object *in, Object *out)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "function does not exist");
    return ERROR;
}

static PFIP gvar_methods[] =
{
    GvarDelete
};

static PFIP progobj_methods[] =
{
    progobjDelete
};

void ResetSubGraphIds()
{
    subgraph_id = 0;
}
 
int NextSubGraphId()
{
    int sgid;

    sgid = ++(subgraph_id);
    return (sgid);
}


/*
 * Initialize the code which constructs graphs from parse trees
 */
void
_dxf_ExGraphInit (void)
{
    ExDebug("*1","In _dxf_ExGraphInit");
    _dxd_exCurrentFunc = NULL;
    graphing_error = FALSE;
    _dxf_ExMacroRecursionInit ();
    if (_dxd_exGraphCache == NULL)
	_dxd_exGraphCache = _dxf_ExDictionaryCreate (32, TRUE, FALSE);
}

int
DXGetModulePathLen()
{
    return _dxf_ExPathStringLen( &_dxd_exCurrentFunc->mod_path );
}

int
DXGetModulePath(char *path)
{
    _dxf_ExGFuncPathToStringBuf( _dxd_exCurrentFunc, path );
    return(OK);
}

int
DXGetModuleCacheStrLen()
{
    return _dxf_ExPathCacheStringLen( &_dxd_exCurrentFunc->mod_path );
}

int
DXGetModuleCacheStr(char *path)
{
    _dxf_ExGFuncPathToCacheStringBuf( _dxd_exCurrentFunc, path );
     return(OK);
}



int
_dxf_ExGetCurrentInstance()
{
    return(_dxd_exCurrentFunc->instance);
}


/*
 * Create a new gvar node.  These nodes represent the interconnections
 * between the gfunc nodes which are the executable nodes.  They are used
 * as place holders to allow an indirect access to the data.
 */
static indexobj *CreateIndex (int ind)
{
    indexobj	*n;

    n = (indexobj *) _dxf_EXO_create_object_local (EXO_CLASS_TASK,
				    sizeof (indexobj),
				    NULL);
    n->index = ind;
    return (n);
}

/*
 * Create a new gvar node.  These nodes represent the interconnections
 * between the gfunc nodes which are the executable nodes.  They are used
 * as place holders to allow an indirect access to the data.
 */
static progobj *CreateProgobj (Program *p)
{
    progobj	*n;

    n = (progobj *) _dxf_EXO_create_object_local (EXO_CLASS_TASK,
				    sizeof (progobj),
				    progobj_methods);
    n->p = p;
    return (n);
}
/*
 * Create a new gvar node.  These nodes represent the interconnections
 * between the gfunc nodes which are the executable nodes.  They are used
 * as place holders to allow an indirect access to the data.
 */
gvar *_dxf_ExCreateGvar (_gvtype type)
{
    gvar	*n;

    n = (gvar *) _dxf_EXO_create_object (EXO_CLASS_GVAR,
				    sizeof (gvar),
				    gvar_methods);
    n->type = type;

    return (n);
}

/*
 * procId == 0		OK to delete, created somewhere, all global
 * procId != pin proc	OK to delete, created somewhere and must
 *				be all global since can't guarantee where
 *				it will be run.
 * procId == exJID		OK to delete, created here
 * 1 processor		OK to delete, created here
 */

static void
DeleteGvarObj(gvar *var)
{
    int		procId;
    Object	obj = var->obj;

    if (obj != NULL)
    {
	procId = var->procId;
	if (var->type != GV_CACHE	||
	    procId == 0			||
	    procId != EX_PIN_PROC	||
	    procId == exJID		||
	    DXProcessors (0) == 1)
	    DXDelete (obj);
	else
	{
#if 0
    /*
     * $$$$$ THIS IS A TEMPORARY SOLUTION BELOW, THE CORRECT IS DESCRIBED
     * $$$$$ What we really need is to augment the ExReclaimDisable function
     * $$$$$ so that it loops on a DXtry_lock, which if it doesn't get causes
     * $$$$$ it to look for high priority jobs destined for its JID since
     * $$$$$ they may be deletes issued from within some other invocation
     * $$$$$ of the memory reclaimer that MUST run on that processor
     * $$$$$ before the world, e.g. the running reclaimer, can really 
     * $$$$$ continue.
     */
	    _dxf_ExRunOn (procId, DXDelete, (Pointer) obj, 0);
#endif
	    _dxf_ExRQEnqueue (DXDelete, (Pointer) obj, 1, 0, procId, TRUE);
	}
    }
}


/*
 * Defines a gvar to have a value.
 */
void
_dxf_ExDefineGvar (gvar *gv, Object o)
{
    DXReference (o);

    if (gv->obj && gv->type != GV_UNRESOLVED)
	DeleteGvarObj(gv);

    gv->obj = o;
    if (gv->type == GV_UNRESOLVED)
	gv->type = GV_DEFINED;
}

/*
 * Makes the gvar unresolved and deletes previous object
 * leaves the reference counts alone. 
 */
void
_dxf_ExUndefineGvar (gvar *gv)
{
    if (gv->obj && gv->type != GV_UNRESOLVED)
	DeleteGvarObj(gv);

    gv->obj = NULL;
    gv->skip = GV_DONTSKIP;
    gv->type = GV_UNRESOLVED;
    gv->reccrc = 0;
    gv->cost = 0;
    gv->disable_cache = 0;
    gv->procId = 0;
}

/*
 * DXDelete a gvar
 */
static int
GvarDelete (gvar *var)
{
#if OLD_DEBUG
    ExDebug ("*6", "DELETING:  [%08x] with cache tag 0x%08x", var, var->reccrc);
#endif

    if (var->oneshot)
    {
	DXDelete(var->oneshot);
	var->oneshot = NULL;
    }
    DeleteGvarObj(var);

#if OLD_DEBUG
    ExDebug ("*6", "DELETED:   [%08x]", var);
#endif 
    return (OK);
}

static int
progobjDelete (progobj *var)
{
    _dxf_ExGraphDelete (var->p);

    return (OK);
}

/*
 * Create and initialize a new gfunc node.  These nodes represent the
 * executable entities of the graph.
 */
gfunc *_dxf_ExInitGfunc (gfunc *n)
{
    bzero (n, sizeof (gfunc));

    n->required = -1;
    INIT_LIST(n->inputs);
    INIT_LIST(n->outputs);
    INIT_LIST(n->reroute);
    INIT_LIST(n->cache_ddi);
    return (n);
}

#if 0
/*
 * Delete a gfunc node and remove it from the enclosing graph
 */
static int
exGfuncFree (gfunc *p)
{
    char        s[100];
    strcpy (s, ">GfDel ");
    strcat (s, p->name);

    ExDebug ("*1", "DELETING:  [%08x] %d::%s", p, p->cpath);

    FREE_LIST(p->inputs);
    FREE_LIST(p->outputs);
    FREE_LIST(p->attrs);
    DXFree(p->name);
    DXDelete((Object)p->path);
    ExDebug ("*1", "DELETED:   [%08x] <gone>", p);

    return (OK);
}
#endif

static void InitProgram(Program *p)
{
    p->runable = 0;
    p->deleted = FALSE;
    p->loopctl.first = TRUE;
    p->loopctl.done = TRUE;
    p->loopctl.counter = 0;
    p->loopctl.isdoneset = 0;
    p->returnP = NULL;
}

static Program *
AllocateProgram(void)
{
    Program *p = NULL;

#ifdef ALLOC_UPFRONT
    int i;
    for (i = 0; i < MAX_FREED; ++i)
    {
        if (freedProgs[i] != NULL)
        {
	    p = freedProgs[i];
	    freedProgs[i] = NULL;
	    return (p);
	 }
    }
#endif

    p = (Program*)DXAllocateZero (sizeof (Program));
    if (p == NULL)
	return (NULL);

#if ALLOC_UPFRONT
    p->local = FALSE;
#endif
    INIT_LIST (p->inputs);
    INIT_LIST (p->outputs);
    INIT_LIST (p->undefineds);
    INIT_LIST (p->wiredvars);
    INIT_LIST (p->macros);
    INIT_LIST (p->async_vars);
    INIT_LIST (p->vars);
    INIT_LIST (p->funcs);
    INIT_LIST (p->foreach_id);
 
    InitProgram(p);

    return (p);
}

static Program *
AllocateProgramLocal(void)
{
    Program *p = NULL;
    p = (Program*)DXAllocateLocalZero (sizeof (Program));
    if (p == NULL)
	return (NULL);

#if ALLOC_UPFRONT
    p->local = TRUE;
#endif
    if (INIT_LIST_LOCAL (ProgramRef, p->inputs) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (ProgramRef, p->outputs) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (ProgramRef, p->undefineds) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (int, p->wiredvars) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (MacroRef, p->macros) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (AsyncVars, p->async_vars) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (ProgramVariable, p->vars) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (gfunc, p->funcs) == NULL)
	return (NULL);
    if (INIT_LIST_LOCAL (char *, p->foreach_id) == NULL)
	return (NULL);

    InitProgram(p);

    return (p);
}

void
ExSubGraphDelete(Program *subp, int top)
{
    gfunc *gf;
    ProgramRef *pr;
    int i, j, limit, jlimit;

    for (i = 0, limit = SIZE_LIST(subp->funcs); i < limit; ++i)
    {
	gf = FETCH_LIST(subp->funcs, i);
        if(gf->ftype == F_MACRO)
	    DXFree(gf->name);
	FREE_LIST(gf->inputs);
	FREE_LIST(gf->outputs);
        FREE_LIST(gf->cache_ddi);
        FREE_LIST(gf->reroute);
        if((gf->ftype == F_MACRO) && gf->func.subP) {
            jlimit = SIZE_LIST(gf->func.subP->undefineds);
            for (j = 0; j < jlimit; ++j) {
	        pr = FETCH_LIST(gf->func.subP->undefineds, j);
	        DXFree(pr->name);
	        pr->name = NULL;
            }
            ExSubGraphDelete(gf->func.subP, top+1);
	    FREE_LIST(gf->func.subP->funcs);
	    FREE_LIST(gf->func.subP->undefineds);
	    FREE_LIST(gf->func.subP->wiredvars);
            FREE_LIST(gf->func.subP->async_vars);
	    DXFree((Pointer)gf->func.subP);
        }
    }
}

void
_dxf_ExGraphDelete(Program *p)
{
    ProgramVariable *pv;
    ProgramRef *pr;
    MacroRef *mr;
    int i;
    int limit;
 
#ifdef ALLOC_UPFRONT
    int delete;
#endif
    
    if (p == NULL)
	return;

    ExDebug("3", "In ExGraphDelete");
    if (p == _dxd_exContext->program) {
	_dxd_exContext->program = NULL;
        if(_dxd_exContext->subp) {
            DXFree(_dxd_exContext->subp);
            _dxd_exContext->subp = NULL;
        }
    }

    p->error = FALSE;

    for (i = 0, limit = SIZE_LIST(p->inputs); i < limit; ++i)
    {
	pr = FETCH_LIST(p->inputs, i);
	DXFree(pr->name);
	pr->name = NULL;
    }
    for (i = 0, limit = SIZE_LIST(p->outputs); i < limit; ++i)
    {
	pr = FETCH_LIST(p->outputs, i);
	DXFree(pr->name);
	pr->name = NULL;
    }
    for (i = 0, limit = SIZE_LIST(p->undefineds); i < limit; ++i)
    {
	pr = FETCH_LIST(p->undefineds, i);
	DXFree(pr->name);
	pr->name = NULL;
    }
    for (i = 0, limit = SIZE_LIST(p->macros); i < limit; ++i)
    {
	mr = FETCH_LIST(p->macros, i);
	DXFree(mr->name);
	mr->name = NULL;
    }
    for (i = 0, limit = SIZE_LIST(p->vars); i < limit; ++i)
    {
	pv = FETCH_LIST(p->vars, i);
	if (pv->defined)
	    DXDelete(pv->obj);
	DXDelete(pv->dflt);
	pv->defined = FALSE;
	pv->obj = NULL;

#ifdef CPV_DEBUG
if (pv->refs > 0 && pv->refs > pv->gv->object.refs)
    printf("wierd during graph cleanup\n");
#endif

	for (;pv->refs > 0; --pv->refs)
	    ExDelete(pv->gv);
        FREE_LIST(pv->cts);

	/* The execution code covers the following deletion
	 * 	ExDelete((FETCH_LIST(p->vars, i))->gv);
	 */
    }

#ifdef ALLOC_UPFRONT
    delete = TRUE;
    for (i = 0; !p->local && i < MAX_FREED; ++i)
    {
	if (freedProgs[i] == NULL)
	{
	    freedProgs[i] = p;
	    delete = FALSE;
	    break;
	}
    }

    if (delete)
    {
#endif
	FREE_LIST(p->inputs);
	FREE_LIST(p->outputs);
	FREE_LIST(p->undefineds);
	FREE_LIST(p->wiredvars);
	FREE_LIST(p->macros);
	FREE_LIST(p->async_vars);
	FREE_LIST(p->vars);
	FREE_LIST(p->foreach_id);
#ifdef ALLOC_UPFRONT
    }
    else
    {
	REINIT_LIST(p->inputs);
	REINIT_LIST(p->outputs);
	REINIT_LIST(p->undefineds);
	REINIT_LIST(p->wiredvars);
	REINIT_LIST(p->macros);
	REINIT_LIST(p->async_vars);
	REINIT_LIST(p->vars);
	REINIT_LIST(p->foreach_id);
    }
#endif

    if(SIZE_LIST(p->funcs) > 0)
        ExSubGraphDelete(p, 0);

#ifdef ALLOC_UPFRONT
    if (delete)
    {
#endif
	FREE_LIST(p->funcs);
	DXFree((Pointer)p);
#ifdef ALLOC_UPFRONT
    }
    else
    {
	REINIT_LIST(p->funcs);
    }
#endif
}

int
_dxf_ExGraphCompact (void)
{
    if (exJID == 1 && _dxd_exGraphCache != NULL)
    {
	if (_dxf_ExDictionaryPurge(_dxd_exGraphCache))
	    return (_dxf_ExDictionaryCompact(_dxd_exGraphCache));
    }
    return (FALSE);
	
}

/*
 * Construct a graph from a parse tree.  The graph consists of 'gfunc' and
 * 'gvar' nodes.  The 'gfunc' nodes represent the executable modules in the
 * graph.  The 'gvar' nodes represent the connections between the executable
 * nodes.
 */
Program *_dxf_ExGraph (node *n)
{
    Program *p;
    EXDictionary dict;

    ExMarkTime(0x10, ">_graph");
    if (n == NULL)
    {
	ExMarkTime(0x10, "<_graph");
	return (NULL);
    }
    p = AllocateProgram();
    p->graphId = _dxf_NextGraphId();
    ResetSubGraphIds();

    dict = _dxf_ExDictionaryCreate (DICT_SIZE, TRUE, FALSE);
    ExGraphTraverse(p, n, 0, dict);

    _dxf_ExDictionaryDestroy (dict);

    if (graphing_error ||
	(SIZE_LIST(p->funcs) == 0 && 
	 SIZE_LIST(p->outputs) == 0))
    {
	_dxf_ExGraphDelete(p);
	p = NULL;
    }

    if (p != NULL && SIZE_LIST(p->outputs) != 0)
	_dxf_ExBackgroundChange();

    ExMarkTime(0x10, "<_graph");
    return (p);
}

/*
 * Recursively traverse the parse tree and construct an equivalent
 * graph structure which is more easily executed
 */
static void ExGraphTraverse (Program *p, node *n, int top, EXDictionary dict)
{
    int		nout;
    char	*name;
    char	*is_are;
    list_int	out;
    int         flags;

    if (graphing_error)
	return;

    for (; n; n = n->next)
    {
	switch (n->type)
	{
	    case NT_MACRO:				/* macro definition */
		_dxf_ExDictionaryPurge (_dxd_exGraphCache);
		_dxf_ExMacroInsert (n->v.macro.id->v.id.id, (EXObj) n); 
		_dxf_ExBackgroundRedo();
		break;

	    case NT_CALL:				/* function call */
		INIT_LIST(out);
		ExGraphCall (p, n, top, &out, dict, &flags);
		nout = SIZE_LIST(out);
		FREE_LIST(out);
		if ((nout != 0) && !(flags & MODULE_SIDE_EFFECT))
		{
		    name   = n->v.call.id->v.id.id;
		    is_are = (nout == 1) ? "value is" : "values are";
		    DXWarning ("#4730", is_are, name);
		}

		break;

	    case NT_ASSIGNMENT:			/* assignment statement */
		ExGraphAssignment (p, n, top, dict);
		break;

	    case NT_PACKET:				/* packet command */
		_dxd_exContext->graphId = p->graphId = n->v.packet.number;
		switch (n->v.packet.type)
		{
		    case PK_INTERRUPT:
			break;

		    case PK_SYSTEM:
		    case PK_MACRO:
		    case PK_FOREGROUND:
		    case PK_BACKGROUND:
			ExGraphTraverse (p, n->v.packet.packet, top, dict);

		    case PK_LINQ:
		    case PK_SINQ:
		    case PK_VINQ:
			break;

		    case PK_IMPORT:
		    {
			Group	info;
			int		i;
			int		len;
			String	string;
			char	*buffer;
			char	*p;

			info = _dxfQueryImportInfo (n->v.packet.packet->v.data.data);
			if (info)
			{
			    DXReference ((Object) info);
			    len = 0;
			    for (i = 0; ; i++)
			    {
				string =
				    (String) DXGetEnumeratedMember (info, i, NULL);
				if (string == NULL)
				    break;
				len += strlen (DXGetString (string));
			    }
			    buffer = (char *) DXAllocateLocal (len + 1);
			    if (buffer == NULL)
			    {
				DXWarning ("#4740");
				return;
			    }
			    buffer[0] = '\0';
			    for (i = 0, p = buffer; ; i++)
			    {
				string =
				    (String) DXGetEnumeratedMember (info, i, NULL);
				if (string == NULL)
				    break;
				
				strcat (p, DXGetString (string));
				p += strlen (p);
			    }
			    _dxf_ExSPack (PACK_IMPORTINFO, n->v.packet.number,
				  buffer, strlen (buffer));
			    DXFree ((Pointer) buffer);
			    DXDelete ((Object) info);
			}
			else
			{
			    _dxf_ExSPack (PACK_IMPORTINFO, n->v.packet.number, NULL, 0);
			}
			break;
		    }
		    default:
		        break;
		}
		break;

	    default:
		DXWarning ("#4750", n->type);
		break;
	}
    }
}

static int ExParseExprLeaves (node *n, int loc, char *opstr, node *args)
{
    char	*id;
    _op		op;
    int		unary;
    char	buf[64];

    if (n == NULL)
	return (loc);
    
    if (n->type != NT_LOGICAL && n->type != NT_ARITHMETIC)
    {
	sprintf (buf, "$%d", loc);
	strcat (opstr, buf);
	args[loc].v.arg.val = n;
	return (loc + 1);
    }

    op = n->v.expr.op;
    unary = (op == AO_NEGATE || op == LO_NOT);

    strcat (opstr, "(");

    /*
     * If we have a unary op it stores its single argument in the lhs rather
     * than the rhs of the expression parse node.  This gives rise to the
     * unary check here and below.
     */

    if (! unary)
	loc = ExParseExprLeaves (n->v.expr.lhs, loc, opstr, args);

    switch (op)
    {
	case LO_LT:	id = " < " ;	break;
	case LO_GT:	id = " > " ;	break;
	case LO_LEQ:	id = " <= ";	break;
	case LO_GEQ:	id = " >= ";	break;
	case LO_EQ:	id = " == ";	break;
	case LO_NEQ:	id = " != ";	break;
	case LO_AND:	id = " && ";	break;
	case LO_OR:	id = " || ";	break;
	case LO_NOT:	id = " ! " ;	break;
	case AO_EXP:	id = " ^ " ;	break;
	case AO_TIMES:	id = " * " ;	break;
	case AO_DIV:	id = " / " ;	break;
	case AO_IDIV:	id = " / " ;	break;
	case AO_MOD:	id = " % " ;	break;
	case AO_PLUS:	id = " + " ;	break;
	case AO_MINUS:	id = " - " ;	break;
	case AO_NEGATE:	id = " - " ;	break;
	default:	id = " ?? ";	break;
    }
    strcat (opstr, id);

    loc = ExParseExprLeaves (unary ? n->v.expr.lhs : n->v.expr.rhs,
			     loc, opstr, args);
    strcat (opstr, ")");

    return (loc);
}


static int ExCountExprLeaves (node *n, int cnt)
{
    _op		op;
    int		unary;

    if (n == NULL)
	return (cnt);
    
    if (n->type != NT_LOGICAL && n->type != NT_ARITHMETIC)
	return (cnt + 1);

    op = n->v.expr.op;
    unary = (op == AO_NEGATE || op == LO_NOT);

    if (! unary)
	cnt = ExCountExprLeaves (n->v.expr.lhs, cnt);

    cnt = ExCountExprLeaves (unary ? n->v.expr.lhs : n->v.expr.rhs, cnt);
    return (cnt);
}


/*
 * Convert a parse tree expression into the a call to Compute
 */

static void ExGraphExpression (Program *p, node *n, int top, list_int *out, EXDictionary dict)
{
    int		cnt;
    int		size;
    char	*opstr	= NULL;
    node	*nodes	= NULL;
    node	*name;
    node	*func;
    node	*cons;
    node	*args;
    int		i, flags;

    cnt = ExCountExprLeaves (n, 0);

    /*
     * We're assuming a varargs Compute here by allocating as many arg blocks
     * as there are parameters.
     * We allocate 4 extra, 1 for the name, 1 for the function call, 1 for
     * the constant to hold the format string and 1 for the format string
     * argument itself.
     */

    if (cnt > 16)
    {
	graphing_error = TRUE;
	DXWarning ("#4760");
	return;
    }

    /*
     * $$$$$
     * $$$$$ Until the assumption is true we will have to check for > 16 args,
     * $$$$$ hence the test above.
     * $$$$$
     */

    size  = (cnt + 4) * sizeof (node);
    nodes = (node *) DXAllocateLocal (size);
    if (nodes == NULL)
	goto error;
    memset (nodes, 0, size);

    /*
     * We'll be conservative and use 16 although each leaf of the expression
     * probably won't add more than 6 characters to the operation description
     * string.
     */

    size  = cnt * 16;
    opstr = (char *) DXAllocateLocal (size);
    if (opstr == NULL)
	goto error;
    memset (opstr, 0, size);

    /*
     * Now we dummy up the parse tree nodes that we need to construct a
     * call to Compute.
     */

    name = nodes;
    func = nodes + 1;
    cons = nodes + 2;
    args = nodes + 3;

    name->type		 = NT_ID;
    name->v.id.id	 = "Compute";
 
    func->type		 = NT_CALL;
    func->v.call.id	 = name;
    func->v.call.arg	 = args;

    cons->type		 = NT_CONSTANT;
    /* value filled in below */

    args[0].v.arg.val	 = cons;

    for (i = 0; i <= cnt; i++)
    {
	args[i].type	= NT_ARGUMENT;
	args[i].next	= &args[i+1];
	args[i].prev	= &args[i-1];
    }
    args[0  ].prev 	= NULL;
    args[cnt].next	= NULL;

    /*
     * We reserve arg0 for the format string thus we must step forward by one.
     */

    ExParseExprLeaves (n, 0, opstr, args + 1);

    cons->v.constant.obj = (Array)DXNewString(opstr);
    if (!cons->v.constant.obj)
	goto error;
    
    ExGraphCall (p, func, top, out, dict, &flags);
    DXFree ((Pointer) nodes);
    DXFree ((Pointer) opstr);
    return;

error:
    _dxf_ExDie ("ExGraphExpression:  can't allocate memory");
    return;
}


/*
 * Convert a parse tree assignment into the proper graph structure.
 * Insert the assigned variables into the current dictionary.
 */
static void
ExGraphAssignment (Program *p, node *n, int top, EXDictionary dict)
{
    node	*lhs, *rhs;
    node	*function;
    int		rvals[64];
    int		i;
    _excache    excache;
    int         attr_flag;
    int         nout, flags;

    ProgramRef  r;
    ProgramVariable pv;
    ProgramVariable *ppv;
    indexobj	*ind;
    list_int	out;

    if (graphing_error)
	return;

    INIT_LIST(out);

    /* Insert all outputs in either output list (if at top), or
     * local list.
     */
    for (i = 0, rhs = n->v.assign.rval;
	 rhs && i < 64;
	 ++i, rhs = rhs->next)
    {
	switch (rhs->type)
	{
	case NT_CONSTANT:
	    /* Create a variable and stick its index in rvals */
	    INIT_PVAR(pv);
	    pv.obj = DXReference ((Object) rhs->v.constant.obj);
	    pv.defined = TRUE;
	    APPEND_LIST (ProgramVariable, p->vars, pv);
	    rvals[i] = SIZE_LIST (p->vars) - 1;
	    break;

	case NT_ID:
	    /* Look up a variable and stick its index in rvals, 
	     * if it isn't found, stick a note in the "undefineds" 
	     */
	    ind = (indexobj *)_dxf_ExVariableSearch (rhs->v.id.id, dict);
	    if (ind == NULL)
	    {
		r.name = _dxf_ExCopyString/*Local*/ (rhs->v.id.id);
		rvals[i] = r.index = SIZE_LIST (p->vars);
		APPEND_LIST (ProgramRef, p->undefineds, r);
		INIT_PVAR(pv);
		APPEND_LIST (ProgramVariable, p->vars, pv);
	    }
	    else
	    {
		rvals[i] = ind->index;
		ExDelete(ind);
	    }
	    break;

	case NT_LOGICAL:
	case NT_ARITHMETIC:
	    /* Create a variable and stick its index in rvals.
	     * Make up a (or set of) Compute operation(s), and stick
	     * them in the list.
	     */
	    ExGraphExpression (p, rhs, top, &out, dict);
	    if (graphing_error)
		return;
	    break;

	case NT_CALL:
	    function = n->v.assign.rval;
	    ExGraphCall (p, function, top, &out, dict, &flags);
	    if (graphing_error)
		return;
            nout = SIZE_LIST(out);
            if (nout == 0)
                DXWarning ("#4732", function->v.function.id->v.id.id);
	    break;

	default:
	    DXWarning ("#4770");
	    break;
	}
    }

    /* Loop through the left hand sides, and set up the assignments
     */
    for (i = 0, lhs = n->v.assign.lval, rhs = n->v.assign.rval;
	 lhs && i < 64;
	 ++i, lhs = lhs->next)
    {
	if (_dxf_ExHasIntegerAttribute (lhs->attr, ATTR_CACHE, (int *)&excache))
	    attr_flag = TRUE;
	else 
        {
	    excache = CACHE_ALL;
	    attr_flag = FALSE;
	}

	/* If there is no rhs, make a null one */
	if (rhs == NULL)
	{
	    /* Create a variable and stick its index in rvals */
	    INIT_PVAR(pv);
	    pv.obj = NULL;
	    pv.defined = TRUE;
            pv.excache = excache;
            pv.cacheattr = attr_flag;
	    APPEND_LIST (ProgramVariable, p->vars, pv);
	    rvals[i] = SIZE_LIST (p->vars) - 1;
	}
	/* If the rhs is a module, */
	else if (rhs->type == NT_CALL ||
		 rhs->type == NT_LOGICAL ||
		 rhs->type == NT_ARITHMETIC)
	{
	    /* if the module defines this output */
	    if (i < SIZE_LIST(out))
	    {
		/* Create a variable and stick its index in rvals */
		INIT_PVAR(pv);
                pv.excache = excache;
                pv.cacheattr = attr_flag;
		APPEND_LIST (int, p->wiredvars, SIZE_LIST(p->vars));
		APPEND_LIST (ProgramVariable, p->vars, pv);
		rvals[i] = *FETCH_LIST(out, i);
                ppv = FETCH_LIST(p->vars, rvals[i]);
                ppv->excache = excache;
                ppv->cacheattr = attr_flag;
	    }
	    /* if the module doesn't define this output (x,y,z = Compute...) */
	    else
	    {
		/* Create a variable and stick its index in rvals */
		INIT_PVAR(pv);
		pv.obj = NULL;
		pv.defined = TRUE;
                pv.excache = excache;
                pv.cacheattr = attr_flag;
		APPEND_LIST (ProgramVariable, p->vars, pv);
		rvals[i] = SIZE_LIST (p->vars) - 1;
		rhs = NULL;
	    }
	}
	/* Else we've got a simple assignment */
	else
	{
	    rhs = rhs->next;
	}

	ind = CreateIndex(rvals[i]);
        /* Check to see if variable name is NULL, if we don't check here
         * we will get two warnings about NULL assignment, one now and
         * one later when the actual variable assignment happens.
	 */
        if (strcmp (lhs->v.id.id, "NULL"))
            _dxf_ExVariableInsert (lhs->v.id.id, dict, (EXO_Object) ind);
	if (top == 0)
	{
	    r.index = rvals[i];
	    r.name = _dxf_ExCopyString/*Local*/(lhs->v.id.id);
	    r.oneshot =
		DXReference (_dxf_ExGetAttribute (lhs->attr, ATTR_ONESHOT));
	    APPEND_LIST (ProgramRef, p->outputs, r);
	}
    }
    if (SIZE_LIST(out) > 0)
	FREE_LIST(out);
}

/*
 * Translate a parse tree call statment (macro expansion or module call)
 * into the appropriate graph structure.  This function takes a parse node, 
 * program, and dictionary.  It modifies the program and produces a list
 * of indices into the programs "vars" list that are the functions
 * output.  "top" is used when recursing.
 */
static void
ExGraphCall (Program *p, node *n, int top, list_int *out, EXDictionary dict, int *flags)
{
    char		*fname;
    char		*name;
    node		*function=NULL;
    node		*formal;
    node		*arg;
    int			slot;
    gfunc		fnode;
    gfunc		*pFnode;
    node		*val;
    _ntype		t;
    int			instance;
    _excache		excache;
    char                *macro_procgroupid;
    int                 keep_caching;
    int			*inArgs;
    node		**inAttrs = NULL; 
    ReRouteMap          *reroutem, rr_map;
    int			nin;
    int			namedArgs;
    int			prehidden;
    int			posthidden;
    node_function	localFunct;
    _ntype		functionType;
    int			i, j;

    ProgramVariable	pv;
    ProgramRef		pr;
    MacroRef            mr;
    AsyncVars 		*avars, av; 
    indexobj		*ind;
    list_int		subOut;

    Program		*subP, *copyP;
    EXDictionary	subDict;
    progobj		*pobj;
    int			*mapping;
    int			*resolved, numresolved;
    ProgramVariable	*pPv;
    ProgramRef		*pPr;
    int			ilimit;
    int			jlimit;

    if (graphing_error)
	goto graphing_error_return;

    /*
     * Look up the identifier in the function dictionary.
     */
    fname = name    = n->v.call.id->v.id.id;
#ifdef INPUT_TIMING
    DXMarkTimeLocal (name);
#endif

    function = (node *)_dxf_ExMacroSearch (name);
    if (function == NULL)
    {
        char *modname;
        modname = DXAllocateLocal(strlen(name) + 1);
        strcpy(modname, name);
        DXAddModule(modname, m__badfunc, 0,
            21, "input1", "input2", "input3", "input4", "input5", "input6",
            "input7", "input8", "input9", "input10", "input11", "input12",
            "input13", "input14", "input15", "input16", "input17", "input18",
            "input19", "input20", "input21",
            21, "output", "output1", "output2", "output3", "output4", "output5",
            "output6", "output7", "output8", "output9", "output10", "output11",
            "output12", "output13", "output14", "output15", "output16",
            "output17", "output18", "output19", "output20");
        function = (node *)_dxf_ExMacroSearch (name);
    }
    if(function->v.function.def.func == m__badfunc) {
        if(!_dxd_exRemoteSlave)
    	    DXUIMessage ("ERROR", "%s: function does not exist", name);
        else 
    	    DXUIMessage ("ERROR", 
            "%s: function does not exist on host %s", name, _dxd_exHostName);
    }

    functionType = function->type;

    if (_dxf_ExMacroRecursionCheck (name, functionType))
    {
	graphing_error = TRUE;
	goto graphing_error_return;
    }

    localFunct = function->v.function;
    *flags = localFunct.flags;

    /* 
     * Setup positional formal inputs
     */
#ifdef INPUT_TIMING
    DXMarkTimeLocal ("init inputs");
#endif

    nin = fnode.nin = localFunct.nin;
    prehidden = localFunct.prehidden;
    posthidden = localFunct.posthidden;

    fnode.nout = localFunct.nout;
    inArgs = (int*) DXAllocateLocal (nin * sizeof (int));
    inAttrs = (node **) DXAllocateLocal (nin * sizeof (node*));

    for (i = 0; i < nin; ++i) {
        inArgs[i]  = -1;
        inAttrs[i] = NULL;
    }

    for (i = 0, formal = localFunct.in; 
         i < prehidden;
         ++i, formal = formal->next);

    namedArgs = FALSE;
    for (arg = n->v.call.arg, slot = prehidden;
	 slot < nin - posthidden;
	 arg = (arg) ? arg->next : arg, slot++,formal = formal->next)
    {
	/* A name substitution has been found, no more positional args */
	if (arg)
	{
	    if (arg->v.arg.id)  {
		namedArgs = TRUE;
            }
	    else if (namedArgs)
		DXWarning ("#4780", fname);
	}

	/*
	 * If an constant argument was specified, create a defined variable.
	 * If it is an ID, look it up in the dictionary (defining it to be 
	 * undefined if it's not there).
	 * If it's an expression, recurse.
	 */
	if (!namedArgs && arg)
	{
	    val = arg->v.arg.val;
	    t = val->type;
            
	    switch (t)
	    {
	    case NT_CONSTANT:
		INIT_PVAR(pv);
		pv.obj = DXReference ((Object) val->v.constant.obj);
		pv.defined = TRUE;
		APPEND_LIST (ProgramVariable, p->vars, pv);
		inArgs[slot] = SIZE_LIST(p->vars) - 1;
                inAttrs[slot] = (node *) formal->attr;
		break;

	    case NT_ID:
		ind = (indexobj *)_dxf_ExVariableSearch (val->v.id.id, dict);
		if (ind == NULL)
		{
		    pr.name = _dxf_ExCopyString/*Local*/ (val->v.id.id);
            pr.oneshot = NULL;
		    inArgs[slot] = pr.index = SIZE_LIST (p->vars);
                    ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
		    APPEND_LIST (ProgramRef, p->undefineds, pr);
		    INIT_PVAR(pv);
                    inAttrs[slot] = (node *) formal->attr;
		    APPEND_LIST (ProgramVariable, p->vars, pv);
		}
		else
		{
		    inArgs[slot] = ind->index;
                    inAttrs[slot] = (node *) formal->attr;
		    ExDelete (ind);
		}
		break;

	    case NT_LOGICAL:
	    case NT_ARITHMETIC:
		INIT_LIST(subOut);
		ExGraphExpression (p, val, top, &subOut, dict);
		if (graphing_error)
		    goto graphing_error_return;
		inArgs[slot] = *FETCH_LIST(subOut, 0);
		FREE_LIST(subOut);
		break;

	    default:
                if(!_dxd_exRemoteSlave)
		    DXUIMessage ("ERROR",
		       "_graphCall Internal:  invalid argument type = %d", t);
		graphing_error = TRUE;
		goto call_error;
	    }
	}
    }
    if (arg)
	DXWarning ("#4790", fname);
#ifdef INPUT_TIMING
    DXMarkTimeLocal ("inited unnamed");
#endif

    /*
     * Setup explicitly named formals
     */
    if (namedArgs)
    {
	for (arg = n->v.call.arg; arg; arg = arg->next)
	{
	    /* 
	     * this is not a name substitution argument, don't bother 
	     */
	    if (! arg->v.arg.id)
		continue;

	    /*
	     * look up formal name in formal list
	     */
	    for (slot = 0, formal = localFunct.in; 
		formal;
		formal = formal->next, slot++)
	    {
		if (strcmp (arg->v.arg.id->v.id.id, formal->v.id.id) == 0) 
		{
		    break;
		}
	    }

	    if (slot >= nin)
	    {
		DXWarning ("#4800", fname, arg->v.arg.id->v.id.id);
		continue;
	    }

	    /*
	     * If an constant argument was specified, 
	     * create a defined variable.
	     * If it is an ID, look it up in the dictionary (defining it to be 
	     * undefined if it's not there).
	     * If it's an expression, recurse.
	     */
	    val = arg->v.arg.val;
	    t = val->type;
	    switch (t)
	    {
	    case NT_CONSTANT:
		INIT_PVAR(pv);
		pv.obj = DXReference ((Object) val->v.constant.obj);
		pv.defined = TRUE;
		APPEND_LIST (ProgramVariable, p->vars, pv);
		inArgs[slot] = SIZE_LIST(p->vars) - 1;
                inAttrs[slot] = (node *) formal->attr;
		break;

	    case NT_ID:
		ind = (indexobj *)_dxf_ExVariableSearch (val->v.id.id, dict);
		if (ind == NULL)
		{
		    pr.name = _dxf_ExCopyString/*Local*/ (val->v.id.id);
		    inArgs[slot] = pr.index = SIZE_LIST (p->vars);
                    ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
		    APPEND_LIST (ProgramRef, p->undefineds, pr);
		    INIT_PVAR(pv);
                    inAttrs[slot] = (node *) formal->attr;
		    APPEND_LIST (ProgramVariable, p->vars, pv);
		}
		else
		{
		    inArgs[slot] = ind->index;
                    inAttrs[slot] = (node *) formal->attr;
		    ExDelete (ind);
		}
		break;

	    case NT_LOGICAL:
	    case NT_ARITHMETIC:
		INIT_LIST(subOut);
		ExGraphExpression (p, val, top, &subOut, dict);
		if (graphing_error)
		    goto graphing_error_return;
		inArgs[slot] = *FETCH_LIST(subOut, 0);
		break;

	    default:
                if(!_dxd_exRemoteSlave)
		    DXUIMessage ("ERROR",
		       "_graphCall Internal:  invalid argument type = %d", t);
		graphing_error = TRUE;
		goto call_error;
	    }
	}
    }

    /* 
     * Fill in defaults, where they exist.  Set the default value even if
     * something has been assigned to this input, because if it ends up
     * being evaluated to NULL, the default should be used instead.
     */
    for (formal = localFunct.in, slot = 0;
	 slot < nin;
	 ++slot, formal = formal->next)
    {
	/* 
	 * if no default, don't bother
	 */
	if (!formal->v.id.dflt)
	    continue;


	if (inArgs[slot] < 0)
	{
	    INIT_PVAR(pv);
	    APPEND_LIST (ProgramVariable, p->vars, pv);
	    inArgs[slot] = SIZE_LIST(p->vars) - 1;
	}
	pPv = FETCH_LIST(p->vars, inArgs[slot]);
	if (formal->v.id.dflt->type != NT_CONSTANT)
	{
            if(!_dxd_exRemoteSlave)
	        DXUIMessage ("ERROR",
			 "%s:  default values must be constants", fname);
	    graphing_error = TRUE;
	    goto call_error;
	}
	else
	{
	    pPv->dflt =
		DXReference ((Object)formal->v.id.dflt->v.constant.obj);
	}
    }
#ifdef INPUT_TIMING
    DXMarkTimeLocal ("inited named");
#endif

    /*
     * Find the function's attributes, e.g. instance and caching if it
     * has any attributed to it
     */
    instance = _dxf_ExGetIntegerAttribute (n->attr, ATTR_INSTANCE);

    if (! _dxf_ExHasIntegerAttribute (n->attr, ATTR_CACHE, (int *)&excache))
	excache = CACHE_ALL;

    /*
     * ExNoCachePush expects an on/off flag. Since all values > 0 imply some
     * kind of caching, pass a 1 to ExNoCachePush for all values > 0, and
     * set excache to the cache attr value unless a 0 is returned, in which
     * case caching should be suppressed. (is this OK? )
     */
    keep_caching = _dxf_ExNoCachePush (excache != CACHE_OFF);
    if (!keep_caching)
	excache = CACHE_OFF;

    /*
     * Macro calls are expanded recursively.  Module calls create input
     * and output lists, and insert the gfunc structure into the program.
     */
    switch (functionType)
    {
    case NT_MACRO:

	/* Look up the macro in the graph cache, and make one (inserting
	 * it) if it's not found.
	 */

        macro_procgroupid = _dxf_ExGetStringAttribute (n->attr, ATTR_PGRP);

	pobj = (progobj*) _dxf_ExDictionarySearch (_dxd_exGraphCache, name);
	if (pobj == NULL)
	{
#ifdef INPUT_TIMING
	    DXMarkTimeLocal ("init formals");
#endif
	    /* Create sub{dictionary,program} */
	    subDict = _dxf_ExDictionaryCreate (DICT_SIZE, TRUE, FALSE);
	    subP = AllocateProgramLocal();

	    /* Insert supplied inputs into new local dictionary */
	    for (formal = localFunct.in, slot = 0;
			formal;
			formal = formal->next, slot++)
	    {
		pr.index = SIZE_LIST(subP->vars);
		pr.name = _dxf_ExCopyString/*Local*/ (formal->v.id.id);
                ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
		APPEND_LIST (ProgramRef, subP->inputs, pr);
		ind = CreateIndex(pr.index);
		_dxf_ExVariableInsert (pr.name, subDict, (EXObj) ind);
		INIT_PVAR (pv);
		APPEND_LIST (ProgramVariable, subP->vars, pv);
	    }

#ifdef INPUT_TIMING
	    DXMarkTimeLocal ("inited formals");
#endif
	    /* Recurse */
	    ExGraphTraverse (subP, localFunct.def.stmt, top + 1, subDict);
            if(graphing_error)
                goto graphing_error_return;

            /* add itself to it's list of macros */
            mr.name = _dxf_ExCopyString(name);
            mr.index = localFunct.index;
            APPEND_LIST(MacroRef, subP->macros, mr);
 
	    /* Extract outputs from the dictionary and put them in the 
	     * output list
	     */
	    for (formal = localFunct.out, slot = 0;
			formal;
			formal = formal->next, slot++)
	    {
		ind = (indexobj*) _dxf_ExVariableSearch (formal->v.id.id, 
							 subDict);
		/* If there was no definition for this output, set to null */
		if (ind == NULL)
		{
		    INIT_PVAR (pv);
		    pv.defined = TRUE;
		    APPEND_LIST (ProgramVariable, subP->vars, pv);
		    i = SIZE_LIST (subP->vars) - 1;
		}
		else
		{
		    i = ind->index;
		    ExDelete(ind);
		}
		pr.index = i;
		pr.name = _dxf_ExCopyString/*Local*/ (formal->v.id.id);
                ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
		APPEND_LIST (ProgramRef, subP->outputs, pr);
	    }

	    /* Clean up the dictionary and create the cached object */
	    _dxf_ExDictionaryDestroy (subDict);
	    pobj = CreateProgobj (subP);
	    ExReference (pobj);

	    if (top == 0)
	    {
	        /* Insert any SendModules that may be required */
		ExDebug("*1","Before ExCreateSendModules with nbr gfuncs = %d",
			SIZE_LIST(subP->funcs));
		ExCreateSendModules(subP);  
		ExDebug("*1","After ExCreateSendModules with nbr gfuncs = %d",
		    SIZE_LIST(subP->funcs));
	    }
	    _dxf_ExDictionaryInsert (_dxd_exGraphCache, name, (EXObj)pobj);
	}
	else
	    subP = pobj->p;

        /* parent program gets a copy of all macros in it's children */
	GROW_LIST(MacroRef, p->macros, SIZE_LIST(subP->macros));

	for (i = 0, ilimit = SIZE_LIST(subP->macros); i < ilimit; ++i)
        {
            MacroRef *macro; 
	    macro = FETCH_LIST(subP->macros, i);
            mr.name = _dxf_ExCopyString(macro->name);
            mr.index = macro->index;
            APPEND_LIST(MacroRef, p->macros, mr);
        }
	/* From here on, we're copying the relocatable (compiled) version
	 * of the macro into the caller's program.
	 *
	 * Start by Create a mapping array... mapping [i] is valid 
	 * iff resolved[i] is TRUE.  p->vars[mapping[subPindex]]
	 */
	mapping  = (int*)DXAllocateLocal     (SIZE_LIST(subP->vars) * 
					      sizeof(int));
	resolved = (int*)DXAllocateLocalZero (SIZE_LIST(subP->vars) * 
					      sizeof(int));
        numresolved = 0;
	if (mapping == NULL || resolved == NULL)
	{
	    graphing_error = TRUE;
	    _dxf_ExDie ("ExGraphCall:  can't allocate memory");
	}

	/* Define mappings for the inputs in the caller's variable list */
	for (i = 0, ilimit = SIZE_LIST(subP->inputs); i < ilimit; ++i)
	{
	    pPr = FETCH_LIST(subP->inputs, i);
	    mapping [pPr->index] = inArgs[i];
	    resolved[pPr->index] = TRUE;
            numresolved++;
	}

	/* Try to define all undefineds.  If it exists in the caller's
	 * dictionary, define the value (that's the new mapping).  If it's not
	 * there, create a new undefined.
	 */
	GROW_LIST(ProgramVariable, p->vars, SIZE_LIST(subP->undefineds));
	GROW_LIST(ProgramRef, p->undefineds, SIZE_LIST(subP->undefineds));
	for (i = 0, ilimit = SIZE_LIST(subP->undefineds); i < ilimit; ++i)
	{
	    pPr = FETCH_LIST(subP->undefineds, i);
	    ind = (indexobj *) _dxf_ExVariableSearch(pPr->name, dict);
	    if (ind == NULL)
	    {
		pr.index = SIZE_LIST(p->vars);
		pr.name  = _dxf_ExCopyString/*Local*/ (pPr->name);
                ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
		mapping [pPr->index] = pr.index;
		resolved[pPr->index] = TRUE;
                numresolved++;
		pPv = FETCH_LIST(subP->vars, pPr->index);
		pv = *pPv;
		if (pPv->defined) 
		    DXReference (pPv->obj);
		if (pPv->dflt != NULL)
		    DXReference (pPv->dflt);
		APPEND_LIST (ProgramVariable, p->vars, pv);
		APPEND_LIST (ProgramRef,      p->undefineds, pr);
	    }
	    else
	    {
		mapping [pPr->index] = ind->index;
		resolved[pPr->index] = TRUE;
                numresolved++;
		ExDelete (ind);
	    }
	}

	/* Loop through and create mappings for the other variables */
        ilimit = SIZE_LIST(subP->vars);
        GROW_LIST(ProgramVariable, p->vars, ilimit-numresolved);
	for (i = 0; i < ilimit; ++i)
	{
	    pPv = FETCH_LIST(subP->vars, i);
	    if (!resolved[i])
	    {
		pv = *pPv;
		mapping [i] = SIZE_LIST(p->vars);
		resolved[i] = TRUE;
		APPEND_LIST (ProgramVariable, p->vars, pv);
		if (pPv->defined) 
		    DXReference (pPv->obj);
		if (pPv->dflt != NULL)
		    DXReference (pPv->dflt);
	    }
	}

	/* Put the outputs in the "out" list which is used by the
	 * macro caller.
	 */
	for (i = 0, ilimit = SIZE_LIST(subP->outputs); i < ilimit; ++i)
	{
	    pPr = FETCH_LIST(subP->outputs, i);
	    APPEND_LIST(int, *out, mapping[pPr->index]);
	}

        /* parent program gets a copy of all async variable in it's children */
	for (i = 0, ilimit = SIZE_LIST(subP->async_vars); i < ilimit; ++i)
        {
	    avars = FETCH_LIST(subP->async_vars, i);
            av.nameindx = mapping[avars->nameindx];
            av.valueindx = mapping[avars->valueindx];
            APPEND_LIST(int, p->wiredvars, av.valueindx);
            APPEND_LIST(AsyncVars, p->async_vars, av);
        }

        /* We will copy each function into a separate subprogram that will 
         * be referenced from the caller's program.
	 */

        copyP = GetNewMacro(subP, mapping, resolved);
        if(top == 0)
            copyP->subgraphId = NextSubGraphId();

        for(i = 0; i < SIZE_LIST(subP->async_vars); i++) {
            avars = FETCH_LIST(subP->async_vars, i);
	    slot = avars->nameindx;
            if(slot != -1) {
	        if (!resolved[slot])
	            _dxf_ExDie ("Executive inconsistency -- ExGraphCall 001");
                av.nameindx = mapping[slot];
            }
            else av.nameindx = slot;
            pPv = FETCH_LIST(p->vars, mapping[slot]);
            ExFixAsyncVarName(p, pPv, fname, instance);
        
	    slot = avars->valueindx;
            if(slot != -1) {
	        if (!resolved[slot])
	            _dxf_ExDie ("Executive inconsistency -- ExGraphCall 002");
                av.valueindx = mapping[slot];
            }
            else av.valueindx = slot;
            APPEND_LIST (AsyncVars, copyP->async_vars, av);
        }
        
	GROW_LIST(gfunc, copyP->funcs, SIZE_LIST(subP->funcs));

	for (i = 0, ilimit = SIZE_LIST(subP->funcs); i < ilimit; ++i)
	{
	    pFnode = FETCH_LIST(subP->funcs, i);
	    fnode = *pFnode;
            if(fnode.ftype == F_MACRO) {
                Program *newp;
                newp = AllocateProgram();
                ExCopySubP(newp, fnode.func.subP, top);
                if(top == 0)
                    newp->subgraphId = NextSubGraphId();
                ExRemapVars(p, newp, mapping, resolved, fname, instance);
                fnode.name = _dxf_ExCopyString(pFnode->name);
                fnode.func.subP = newp;
                if(fnode.flags & MODULE_SIDE_EFFECT)
                    localFunct.flags |= MODULE_SIDE_EFFECT;
                /* we are going to make state macros ASYNCLOCAL */
                /* so that they get the extra 2 hidden inputs   */
                /* added. They are a bit different but share    */
                /* enough code that they should use this flag.  */
                if(fnode.flags & MODULE_CONTAINS_STATE) {
                    localFunct.flags |= MODULE_CONTAINS_STATE;
                    localFunct.flags |= MODULE_ASYNCLOCAL;
                }
                if(fnode.flags & MODULE_CHANGES_STATE) {
                    localFunct.flags |= MODULE_CONTAINS_STATE;
                    localFunct.flags |= MODULE_ASYNCLOCAL;
                }
            }
            else {
                if(fnode.flags & MODULE_CHANGES_STATE) {
                    localFunct.flags |= MODULE_CHANGES_STATE;
                    localFunct.flags |= MODULE_ASYNCLOCAL;
                }
                if(fnode.flags & MODULE_SIDE_EFFECT)
                    localFunct.flags |= MODULE_SIDE_EFFECT;
            }

	    /* 
	     * make changes to the copy of the fnode just created.
             */
	    _dxf_ExPathPrepend( subP, fname, instance, &fnode );


	    /*
	     * macro process group assignments override individual settings.
	     * included function if it has one.
	     */
            if (macro_procgroupid) 
                fnode.procgroupid = _dxf_ExCopyString (macro_procgroupid);


	    INIT_LIST (fnode.inputs);
	    INIT_LIST (fnode.outputs);
	    INIT_LIST (fnode.reroute);
	    INIT_LIST (fnode.cache_ddi);

	    GROW_LIST(int, fnode.inputs, SIZE_LIST(pFnode->inputs));
	    GROW_LIST(int, fnode.outputs, SIZE_LIST(pFnode->outputs));
	    GROW_LIST(ReRouteMap, fnode.reroute, SIZE_LIST(pFnode->reroute));
	    GROW_LIST(int, fnode.cache_ddi, SIZE_LIST(pFnode->cache_ddi));

	    for (j = 0, jlimit = SIZE_LIST(pFnode->inputs); j < jlimit; ++j)
	    {
		slot = *FETCH_LIST(pFnode->inputs, j);
		if (slot == -1)
		{
		    APPEND_LIST(int, fnode.inputs, slot);
		}
		else
		{
		    if (!resolved[slot])
		        _dxf_ExDie ("Executive inconsistency -- ExGraphCall 003");
		    APPEND_LIST(int, fnode.inputs, mapping[slot]);
		}
	    }

            /* The name of the async variable is fixed for ASYNC modules */
            /* above in the call to ExFixAsyncVarName. Here we already   */
            /* fixed the path name of the module so we just use that.    */
            if(fnode.flags & MODULE_ASYNCLOCAL) { 
		Object new_path;

                slot = *FETCH_LIST(fnode.inputs, fnode.nin - 2);
                pPv = FETCH_LIST(p->vars, slot);

		new_path = (Object)DXNewString( _dxf_ExGFuncPathToCacheString(&fnode) );
		DXReference ((Object)new_path);
                if(pPv->obj)
                    DXDelete((Object)pPv->obj);
                pPv->obj = new_path;
            }

	    for (j = 0, jlimit = SIZE_LIST(pFnode->outputs); j < jlimit; ++j)
	    {
		slot = *FETCH_LIST(pFnode->outputs, j);
		if (slot == -1)
		{
		    APPEND_LIST(int, fnode.outputs, slot);
		}
		else
		{
		    if (!resolved[slot])
			_dxf_ExDie ("Executive inconsistency -- ExGraphCall 004");
		    APPEND_LIST(int, fnode.outputs, mapping[slot]);
		}
	    }

	    for (j = 0, jlimit = SIZE_LIST(pFnode->reroute); j < jlimit; ++j)
            {
		reroutem = FETCH_LIST(pFnode->reroute, j);
                rr_map = *reroutem; 
	        APPEND_LIST(ReRouteMap, fnode.reroute, rr_map);
            }

	    for (j = 0, jlimit = SIZE_LIST(pFnode->cache_ddi); j < jlimit; ++j)
            {
		slot = *FETCH_LIST(pFnode->cache_ddi, j);
	        APPEND_LIST(int, fnode.cache_ddi, slot);
            }


	    APPEND_LIST(gfunc, copyP->funcs, fnode);
	}

        ExMakeGfunc(localFunct, NT_MACRO, instance, excache, n, inAttrs, p,
                    inArgs, out, copyP); 

	ExDelete (pobj);
	DXFree ((Pointer)mapping);
	DXFree ((Pointer)resolved);
	break;

    case NT_MODULE:
#ifdef INPUT_TIMING
	DXMarkTimeLocal ("create gfunc");
#endif
        ExMakeGfunc(localFunct, NT_MODULE, instance, excache, n, inAttrs, p, 
                    inArgs, out, NULL); 

#ifdef INPUT_TIMING
	DXMarkTimeLocal ("created gfunc");
#endif
	break;

    default:
	break;
    }

call_error:
    _dxf_ExNoCachePop ();
    _dxf_ExMacroRecursionPop (name, functionType);
    DXFree ((Pointer) inArgs);
    DXFree ((Pointer) inAttrs);


graphing_error_return:

    ExDelete(function);

#ifdef INPUT_TIMING
    DXMarkTimeLocal (fname);
#endif
}

static Program *GetNewMacro(Program *p, int *map, int *resolved)
{
    Program *newp;
    int i, slot;
    ProgramRef pr, *pPr;

    newp = AllocateProgram();
    GROW_LIST(ProgramRef, newp->undefineds, SIZE_LIST(p->undefineds));
    for(i = 0; i < SIZE_LIST(p->undefineds); i++) {
        pPr = FETCH_LIST(p->undefineds, i);
	slot = pPr->index;
        if(slot != -1) {
	    if (!resolved[slot])
	        _dxf_ExDie ("Executive inconsistency -- GetNewMacro 001");
            pr.index = map[slot];
        }
        else pr.index = slot;
        pr.name = _dxf_ExCopyString(pPr->name);
        pr.oneshot = NULL;
        ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
        APPEND_LIST (ProgramRef, newp->undefineds, pr);
    }
        
    GROW_LIST(int, newp->wiredvars, SIZE_LIST(p->wiredvars));
    for(i = 0; i < SIZE_LIST(p->wiredvars); i++) {
        slot = *FETCH_LIST(p->wiredvars, i);
        if(slot != -1) {
            if(!resolved[slot])
	        _dxf_ExDie ("Executive inconsistency -- GetNewMacro 001");
        
            APPEND_LIST(int, newp->wiredvars, map[slot]); 
        }
        else APPEND_LIST(int, newp->wiredvars, slot);
    }

    COPY_LIST(newp->macros, p->macros);
    newp->vars = p->vars;
    newp->loopctl = p->loopctl;
    return(newp);
}

static void ExCopySubP(Program *toP, Program *fromP, int top)
{
    Program *mp;
    gfunc *pFnode, fnode;
    int i, ilimit, j, jlimit, slot;
    ReRouteMap *reroutem, rr_map;
    ProgramRef pr, *pPr;
    AsyncVars *avars, av;

    *toP = *fromP;
    INIT_LIST(toP->undefineds);
    GROW_LIST(ProgramRef, toP->undefineds, SIZE_LIST(fromP->undefineds));
    for(i = 0; i < SIZE_LIST(fromP->undefineds); i++) {
        pPr = FETCH_LIST(fromP->undefineds, i);
        pr.index = pPr->index;
        pr.name = _dxf_ExCopyString(pPr->name);
        pr.oneshot = NULL;
        ExDebug("1","pr.name %s, pr.index %d\n", pr.name, pr.index);
        APPEND_LIST (ProgramRef, toP->undefineds, pr);
    }

    INIT_LIST(toP->wiredvars);
    GROW_LIST(int, toP->wiredvars, SIZE_LIST(fromP->wiredvars));
    for(i = 0; i < SIZE_LIST(fromP->wiredvars); i++) {
        slot = *FETCH_LIST(fromP->wiredvars, i);
        APPEND_LIST (int, toP->wiredvars, slot);
    }

    INIT_LIST(toP->async_vars);
    GROW_LIST(AsyncVars, toP->async_vars, SIZE_LIST(fromP->async_vars));
    for(i = 0; i < SIZE_LIST(fromP->async_vars); i++) {
        avars = FETCH_LIST(fromP->async_vars, i);
        av.nameindx = avars->nameindx;
        av.valueindx = avars->valueindx;
        APPEND_LIST (AsyncVars, toP->async_vars, av);
    }
    COPY_LIST(toP->macros, fromP->macros);
    INIT_LIST (toP->funcs);

    GROW_LIST(gfunc, toP->funcs, SIZE_LIST(fromP->funcs));
 
    for (i = 0, ilimit = SIZE_LIST(fromP->funcs); i < ilimit; ++i)
    {
        pFnode = FETCH_LIST(fromP->funcs, i);
        fnode = *pFnode;

        if(fnode.ftype == F_MACRO) {
            fnode.name = _dxf_ExCopyString(pFnode->name);
            mp = AllocateProgram();
            ExCopySubP(mp, fnode.func.subP, top);
            if(top == 0)
                mp->subgraphId = NextSubGraphId();
            fnode.func.subP = mp;
        }

        INIT_LIST (fnode.inputs);
        INIT_LIST (fnode.outputs);
        INIT_LIST (fnode.reroute);
        INIT_LIST (fnode.cache_ddi);

        GROW_LIST(int, fnode.inputs, SIZE_LIST(pFnode->inputs));
	GROW_LIST(int, fnode.outputs, SIZE_LIST(pFnode->outputs));
	GROW_LIST(ReRouteMap, fnode.reroute, SIZE_LIST(pFnode->reroute));
	GROW_LIST(int, fnode.cache_ddi, SIZE_LIST(pFnode->cache_ddi));

        for (j = 0, jlimit = SIZE_LIST(pFnode->inputs); j < jlimit; ++j)
        {
	    slot = *FETCH_LIST(pFnode->inputs, j);
   	    APPEND_LIST(int, fnode.inputs, slot);
	}

	for (j = 0, jlimit = SIZE_LIST(pFnode->outputs); j < jlimit; ++j)
	{
	    slot = *FETCH_LIST(pFnode->outputs, j);
	    APPEND_LIST(int, fnode.outputs, slot);
        }

	for (j = 0, jlimit = SIZE_LIST(pFnode->reroute); j < jlimit; ++j)
        {
	    reroutem = FETCH_LIST(pFnode->reroute, j);
            rr_map = *reroutem; 
	    APPEND_LIST(ReRouteMap, fnode.reroute, rr_map);
        }

	for (j = 0, jlimit = SIZE_LIST(pFnode->cache_ddi); j < jlimit; ++j)
        {
	    slot = *FETCH_LIST(pFnode->cache_ddi, j);
	    APPEND_LIST(int, fnode.cache_ddi, slot);
        }

        APPEND_LIST(gfunc, toP->funcs, fnode);
    }
}

static void ExMakeGfunc(node_function ndfunc, _ntype type, int instance, 
                        _excache excache, node *n, node **inAttrs, Program *p, 
                        int *inArgs, list_int *out, Program *macro)
{
    gfunc fnode;
    int slot, i;
    ProgramVariable pv;
    ProgramRef *pr;
    node *nodep;
    int nin = ndfunc.nin;
    int nout = ndfunc.nout;
    int found = FALSE;

    _dxf_ExInitGfunc(&fnode);

    switch(type) {
	case NT_MACRO:				/* macro definition */
            fnode.ftype = F_MACRO;
            fnode.func.subP = macro;
            fnode.name = _dxf_ExCopyString(ndfunc.id->v.id.id);
            break;
	case NT_MODULE:				/* module definition */
            fnode.ftype    = F_MODULE;
            fnode.func.module = ndfunc.def.func;
#if 0
            if(ndfunc.flags & MODULE_SIDE_EFFECT)
                p->loopctl.side_effects = TRUE;
#endif
            fnode.name = ndfunc.id->v.id.id;
            break;
        default:
	    break;
    }
    fnode.instance = instance;
    fnode.excache = excache;
	
    _dxf_ExPathSet( p, n->v.call.id->v.id.id, instance, &fnode );
	
    fnode.procgroupid = 
	    _dxf_ExCopyString (_dxf_ExGetStringAttribute (n->attr, ATTR_PGRP));

#if 0
    fnode.led[0] = ndfunc.led[0];
    fnode.led[1] = ndfunc.led[1];
    fnode.led[2] = ndfunc.led[2];
    fnode.led[3] = ndfunc.led[3];
#endif
    fnode.flags = ndfunc.flags;
#if 0
    if(fnode.ftype == F_MACRO && (macro->loopctl.side_effects ||
                                  (ndfunc.nout == 0 && ndfunc.nin == 0))) {
        fnode.flags = MODULE_SIDE_EFFECT;
        p->loopctl.side_effects = TRUE;
    }
#endif
    if(fnode.ftype == F_MACRO && ndfunc.nout == 0 && ndfunc.nin == 0) 
        fnode.flags = MODULE_SIDE_EFFECT;
    fnode.nout = ndfunc.nout;
    fnode.nin = ndfunc.nin;

    if(fnode.ftype == F_MODULE) {
        /* 
         * if asynchronous, the last two inputs are special and need to 
         * have their default values modified.
         */

        if (ndfunc.flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL))
        {
            AsyncVars avars;
            slot = nin - 2;
            if (inArgs[slot] < 0) 
            {
		Object new_path;

	        INIT_PVAR(pv);
	        avars.nameindx = inArgs[slot] = SIZE_LIST(p->vars);
                pv.defined = TRUE;

		new_path = (Object)DXNewString( _dxf_ExGFuncPathToString(&fnode) );
		DXReference ((Object)new_path);
		pv.obj = new_path;
                APPEND_LIST (ProgramVariable, p->vars, pv);
	    }

	    slot = nin - 1;
	    if (inArgs[slot] < 0) 
	    {
	        INIT_PVAR(pv);
	        avars.valueindx = inArgs[slot] = SIZE_LIST(p->vars);
	        APPEND_LIST (int, p->wiredvars, avars.valueindx);
	        APPEND_LIST (ProgramVariable, p->vars, pv);
	    }
            if(ndfunc.flags & MODULE_ASYNC)
                APPEND_LIST(AsyncVars, p->async_vars, avars);
        }
        
        /* 
         * mark these as special, because unlike other modules, you have to
         * compute part of their inputs before you can tell which of their
         * other inputs/outputs should be generated.
         */

        if (strcmp (fnode.name, "Switch") == 0)
            fnode.flowtype = SwitchFlag;
        else if (strcmp (fnode.name, "Route") == 0)
	    fnode.flowtype = RouteFlag;

	if (ndfunc.flags & MODULE_REROUTABLE) {
            Object _attr;
            ReRouteMap rr_map;

	    for (i = 0; i < nin; ++i) {
                nodep = inAttrs[i];
                if(nodep == NULL) 
                    continue;

                _attr = _dxf_ExGetAttribute(nodep, ATTR_DIREROUTE);
                if (_attr)
                {
                    DXExtractInteger(_attr, &(rr_map.output_n));
                    rr_map.input_n = i;
	            APPEND_LIST(ReRouteMap, fnode.reroute, rr_map);
                }

                _attr = _dxf_ExGetAttribute(nodep, ATTR_CACHE);
                /* for inputs the cache value is always 2. I am only
                 * storing which input # needs to be cached and not
                 * the integer value of the cache attribute since it
                 * is currently never used. 
                 */
                if(_attr)
	            APPEND_LIST(int, fnode.cache_ddi, i);
            }
        }
    } /* type is F_MODULE */
    
    /* Add each input to the function's input array, and define new
     * variables for each output.
     */

    GROW_LIST(int, fnode.inputs, nin);
 
    for (i = 0; i < nin; ++i) {
        if (fnode.ftype == F_MODULE && 
            (ndfunc.flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL))) {
            int rerun_attr;
	    if(_dxf_ExHasIntegerAttribute(inAttrs[i], ATTR_RERUNKEY, &rerun_attr)) {
                if(found) 
                    DXWarning("Multiple rerun keys found for module %s, using first occurence of rerun key");
                else {
                    fnode.flags |= MODULE_ASYNCNAMED; 
                    fnode.rerun_index = i;
                    found = TRUE;
                }
	    }
        }
        APPEND_LIST(int, fnode.inputs, inArgs[i]);
    }

    /* do not append to input list if macro has no input and no outputs */
    /* ie. main() */
    if(fnode.ftype == F_MACRO && !(nin == 0 && nout == 0)) {
        /* add named undefineds to end of input list */
        for (i = 0; i < SIZE_LIST(macro->undefineds); i++) {
            pr = FETCH_LIST(macro->undefineds, i);
            if (strcmp(pr->name, "NULL") != 0) {
                APPEND_LIST(int, fnode.inputs, pr->index);
                fnode.nin++;
            }
        }
        /* add last 2 inputs for macros marked asynclocal */
        if (ndfunc.flags & MODULE_ASYNCLOCAL)
        {
            Object new_path;

	    INIT_PVAR(pv);
            pv.defined = TRUE;

	    new_path = (Object)DXNewString( _dxf_ExGFuncPathToCacheString(&fnode) );
	    DXReference ((Object)new_path);
	    pv.obj = new_path;

            APPEND_LIST(int, fnode.inputs, SIZE_LIST(p->vars));
            fnode.nin++;
            APPEND_LIST (ProgramVariable, p->vars, pv);
	    INIT_PVAR(pv);
            APPEND_LIST(int, fnode.inputs, SIZE_LIST(p->vars));
            fnode.nin++;
	    APPEND_LIST (int, p->wiredvars, SIZE_LIST(p->vars));
	    APPEND_LIST (ProgramVariable, p->vars, pv);
	}
    }

    for (i = 0; i < nout; ++i)
    {
        if(fnode.ftype == F_MODULE) {	 
            INIT_PVAR(pv);
            APPEND_LIST(int, p->wiredvars, SIZE_LIST(p->vars));
            APPEND_LIST(ProgramVariable, p->vars, pv);
            APPEND_LIST(int, *out, SIZE_LIST(p->vars) - 1);
            APPEND_LIST(int, fnode.outputs, SIZE_LIST(p->vars) - 1);
        }
        else {
            slot = *FETCH_LIST(*out, i);
            APPEND_LIST(int, p->wiredvars, slot);
            APPEND_LIST(int, fnode.outputs, slot);
        }
    }

    APPEND_LIST(gfunc, p->funcs, fnode);
}

/*
 * Generate SEND modules where outputs to modules are inputs to modules
 * that cross process groups.
 *
 *  For each output of each gfunc:
 *    Match var index # against var index # of each input of each
 *    other gfunc. When match, compare process group name, and create
 *    a SEND module when they don't match.
 */
static void ExCreateSendModules(Program *p)  
{
    int		srcfn, tgfn, in_tab, out_tab;
    int		*index, *outdex;   
    gfunc	*sgf, *tgf;
 
    for (srcfn = 0; srcfn < SIZE_LIST(p->funcs); ++srcfn)  {
	sgf = FETCH_LIST(p->funcs,srcfn);
	if(!strcmp(sgf->name, "DPSEND"))
	    continue;

	for (tgfn = srcfn + 1; tgfn < SIZE_LIST(p->funcs); ++tgfn) {
	    tgf = FETCH_LIST(p->funcs,tgfn);
            /* if target and source functions belong to same execution
             * group then we don't need to build a DPSEND module.
             */
	    if (sgf->procgroupid == NULL && tgf->procgroupid == NULL) 
                continue;
	    if (sgf->procgroupid != NULL && tgf->procgroupid != NULL &&
                !strcmp(sgf->procgroupid, tgf->procgroupid))
                continue;

	    for (out_tab = 0; out_tab < SIZE_LIST(sgf->outputs); ++out_tab) {
		outdex = FETCH_LIST (sgf->outputs, out_tab);
		if (outdex == NULL)
                    _dxf_ExDie("Inconsistency building distributed graph");
                if (*outdex < 0) 
                    continue;

		for (in_tab = 0; in_tab < SIZE_LIST(tgf->inputs); ++in_tab) {
		    index = FETCH_LIST (tgf->inputs, in_tab);
	            if (index == NULL)
                        _dxf_ExDie("Inconsistency building distributed graph");
                    if (*index < 0) 
                        continue;
		    if (*index == *outdex) {
	                ExBuildSendModule(p, sgf, tgf, srcfn, tgfn,
		                          in_tab, out_tab, outdex);
                        /* new module was inserted in list and list may
                         * have been moved to new location in memory
                         * so do list lookup again.
                         */
	                sgf = FETCH_LIST(p->funcs,srcfn);
                        tgfn++; /* to account for module inserted in list */
	                tgf = FETCH_LIST(p->funcs,tgfn);
	            }  /* output of one mod points to input of other */
	        } /* end of input loop for inner-most function */

	    }  /* end of output loop for outer-most function */

	}  /* end of inner-most function loop */

    }  /* end of outer-most function loop */


    if(_dxd_exDebug) {
	for (srcfn = 0; srcfn < SIZE_LIST(p->funcs); ++srcfn)  {
	    sgf = FETCH_LIST(p->funcs,srcfn);
	    printf("%s ", sgf->name);
	}
	if(srcfn > 0)
	    printf("\n");
    }
}

static void 
ExBuildSendModule(Program *p,
		  gfunc *sgf,
		  gfunc *tgf, 
		  int srcfn, 
		  int tgfn, 
		  int in_tab,
		  int out_tab,
		  int *outdex) 
{
    gfunc		fnode;
    int		 	invars[2];
    int			*inArgs;
    int			i;
    ProgramVariable	pv;
    node                *val;
    int                 slot,*index;

    ExDebug("*1","In ExBuildSendModule:");
    ExDebug("*1",
	 "Sending ouput %d from module %s in procgroup %s at var index %d to:",
          out_tab, _dxf_ExGFuncPathToString(sgf), sgf->procgroupid, *outdex); 

    ExDebug("*1", "input %d of module %s in procgroup %s ",
            in_tab, _dxf_ExGFuncPathToString(tgf), tgf->procgroupid); 

    _dxf_ExInitGfunc(&fnode);

    /* will never be calling a function in execGnode for DPSENDs */ 
    fnode.ftype       = F_MODULE;
    fnode.func.module = NULL;   
    fnode.name        = _dxf_ExCopyString/*Local*/ ("DPSEND");
    fnode.instance    = 0;  
    fnode.excache     = CACHE_ALL;
    fnode.procgroupid = _dxf_ExCopyString (sgf->procgroupid);


    /* fix this */
    /*  FIXME:  Assumes these are in the same program (common string table)  */
    _dxf_ExPathCopy( &fnode.mod_path, &sgf->mod_path );
    _dxf_ExPathAppend( p, fnode.name, fnode.instance, &fnode );

    fnode.nin         = 5;
    fnode.nout        = 1;	
    fnode.flags       = MODULE_ERR_CONT;
    invars[0]         = tgfn; 
    invars[1]         = in_tab;

    inArgs = (int*) DXAllocate/*Local*/ (fnode.nin * sizeof (int));

 /* Create PVARs for DPSEND module inputs  */
 /* 0th = target module procid */
 /* 1st = source module procid */
 /* 2nd = target module gfunc# */
 /* 3rd = target module input tab nbr */ 
 /* 4th = index# of pvar being sent   */

    for (i = 0; i < fnode.nin - 1 ; ++i)
    {
	INIT_PVAR(pv);
        if (i == 0) {
	    pv.obj = DXReference ((Object) DXNewString(tgf->procgroupid));
        }
        else if (i == 1) {
	    pv.obj = DXReference ((Object) DXNewString(sgf->procgroupid));
        }
        else {
            _dxf_ExEvaluateConstants (FALSE);
            val = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL, 1, 
				       (Pointer) &invars[i-2]);
            _dxf_ExEvaluateConstants (TRUE);
	    pv.obj = DXReference ((Object) val->v.constant.obj);
	}
	pv.defined = TRUE;
	APPEND_LIST (ProgramVariable, p->vars, pv);
	inArgs[i] = SIZE_LIST(p->vars) - 1;
        APPEND_LIST(int, fnode.inputs, inArgs[i]);
        if (i > 2 ) {
          ExDebug("*1",
		  "Appending fnode.input at %d with index (%d) to input %d",
                  i, inArgs[i], invars[i-2]);
        }
    }

    /* input object is output (pointed to by outdex) being sent   */
    APPEND_LIST(int, fnode.inputs, *outdex); 
    
    /* create an output object */
    INIT_PVAR(pv);
    pv.obj = NULL;
    APPEND_LIST (ProgramVariable, p->vars, pv);
    slot = SIZE_LIST(p->vars) - 1;
    APPEND_LIST(int, p->wiredvars, slot);
    APPEND_LIST(int, fnode.outputs, slot);
    
/* also need to update the var index of the input tab of the target gfunc */ 
    index = FETCH_LIST(tgf->inputs,in_tab);
    ExDebug("*1","Changing index # of %s/%d from %d to %d in BuildSendModule",
            _dxf_ExGFuncPathToString(tgf), in_tab, *index,slot);
    
    UPDATE_LIST(tgf->inputs,slot,in_tab);
    if(tgf->ftype == F_MACRO) {
        gfunc *mstartgf; 
        /* We know that MacroStart is the first module in macro */
        mstartgf = FETCH_LIST(tgf->func.subP->funcs, 0);
        /* The first input to MacroStart is a count so we must use in_tab+1 */
        UPDATE_LIST(mstartgf->inputs, slot, in_tab+1);
    }
    
    INSERT_LIST(gfunc, p->funcs, fnode, srcfn+1);
    DXFree ((Pointer) inArgs);
}

static void
ExRemapVars(Program *p, Program *subP, int *map, int *resolved, char *fname, int inst)
{
    int i, ilimit, j, jlimit, slot;
    gfunc *gf;
    ProgramRef *pr;
    ProgramVariable *pv;
    AsyncVars *avars;

    for (i = 0, ilimit = SIZE_LIST(subP->funcs); i < ilimit; ++i) {
        gf = FETCH_LIST(subP->funcs, i);
        if(gf->ftype == F_MACRO)
            ExRemapVars(p, gf->func.subP, map, resolved, fname, inst);
	for (j = 0, jlimit = SIZE_LIST(gf->inputs); j < jlimit; ++j) {
            slot = *FETCH_LIST(gf->inputs, j);
	    if (slot != -1) {
                if (!resolved[slot])
		    _dxf_ExDie ("Executive inconsistency -- ExRemapVars 001");
		UPDATE_LIST(gf->inputs, map[slot], j);
	    }
        }
        for (j = 0, jlimit = SIZE_LIST(gf->outputs); j < jlimit; ++j) {
	    slot = *FETCH_LIST(gf->outputs, j);
	    if (slot != -1) {
	        if (!resolved[slot])
	            _dxf_ExDie ("Executive inconsistency -- ExRemapVars 002");
	        UPDATE_LIST(gf->outputs, map[slot], j);
	    }
        }    
	_dxf_ExPathPrepend( subP, fname, inst, gf );

        if(gf->flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL)) {
	    Object new_path;

            slot = *FETCH_LIST(gf->inputs, gf->nin - 2);
            pv = FETCH_LIST(p->vars, slot);

	    new_path = (Object)DXNewString( _dxf_ExGFuncPathToCacheString(gf) );
	    DXReference ((Object)new_path);
            if(pv->obj)
                DXDelete((Object)pv->obj);
            pv->obj = new_path;
        }
    }
    for (i = 0, ilimit = SIZE_LIST(subP->undefineds); i < ilimit; ++i)
    {
	pr = FETCH_LIST(subP->undefineds, i);
	slot = pr->index;
        if(slot != -1) {
	    if (!resolved[slot])
	        _dxf_ExDie ("Executive inconsistency -- ExRemapVars 003");
            pr->index = map[slot];
        }
    }

    for (i = 0, ilimit = SIZE_LIST(subP->wiredvars); i < ilimit; ++i)
    {
	slot = *FETCH_LIST(subP->wiredvars, i);
        if(slot != -1) {
	    if (!resolved[slot])
	        _dxf_ExDie ("Executive inconsistency -- ExRemapVars 003");
	    UPDATE_LIST(subP->wiredvars, map[slot], i);
        }
    }

    for(i = 0; i < SIZE_LIST(subP->async_vars); i++) {
        avars = FETCH_LIST(subP->async_vars, i);
        slot = avars->nameindx;
        if(slot != -1) {
            if (!resolved[slot])
                _dxf_ExDie ("Executive inconsistency -- ExRemapVars 004");
            avars->nameindx = map[slot];
        }
        else avars->nameindx = slot;

        slot = avars->valueindx;
        if(slot != -1) {
            if (!resolved[slot])
                _dxf_ExDie ("Executive inconsistency -- ExRemapVars 005");
            avars->valueindx = map[slot];
        }
        else avars->valueindx = slot;
    }
}

static void
ExFixAsyncVarName(Program *program, ProgramVariable *pv, char *fname, int instance)
{
    Object saveobj;
    char *path;

    if(!pv->obj)
        _dxf_ExDie("Executive inconsistency -- bad async variable name");
    saveobj = pv->obj;
    path = DXGetString((String)pv->obj);
    pv->obj = (Object)DXNewString (_dxf_ExCacheStrPrepend(program, fname, instance, path));
    DXReference ((Object)pv->obj);
    DXDelete((Object)saveobj);
}

/*----------------------------------------------------------------------------*/

#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

struct mod_name_info {
    PseudoKey key;     /* psuedokey:  based on string contents */
    char     *str;     /*     value:  actual string value      */
    int       index;   /*       key:  string index             */
};

static lock_type    *mod_name_tables_lock;
static LIST(char *) *mod_name_str_table;     /*  FIXME: Attach to pgm  */
static HashTable     mod_name_hash;          /*  FIXME: Attach to pgm  */



static int hashCompare(Key search, Element matched)
{
    return (strcmp(((struct mod_name_info *)search)->str,
                  ((struct mod_name_info *)matched)->str));
}

static PseudoKey hashFunc(char *str)
{
    unsigned long h = 1;
    while(*str)
       h = (17*h + *str++);
    return (PseudoKey)h;

}

Error _dxf_ModNameTablesInit()
{
  /*  Initialize mutex lock  */
  if ((mod_name_tables_lock =
       (lock_type *) DXAllocate (sizeof (lock_type))) == NULL) {
    _dxf_ExDie ("_dxf_ModNameTablesInit:  lock create failed");
    return ERROR;
  }

  DXcreate_lock(mod_name_tables_lock, NULL);

  /*  FIXME:  Attach this list and its hash to the program structure  */
  mod_name_str_table = DXAllocate( sizeof( LIST(char *) ) );

  INIT_LIST(*mod_name_str_table);

  mod_name_hash = DXCreateHash( sizeof( struct mod_name_info ), NULL,
                                hashCompare );
  if ( !mod_name_hash ) {
    _dxf_ExDie("_dxf_ModNameTablesInit:  hash create failed");
    return ERROR;
  }
  return OK;
}

static uint32 _dxf_ExGraphInsertAndLookupName( Program *p, char name[] )
{
  uint32 i;
  char  *new;
  struct mod_name_info search, *found, info;

  DXlock (mod_name_tables_lock, 0);

  /*  See if string is already in the list (via hash table)  */
  search.key = hashFunc(name);
  search.str = name;

  if ((found = (struct mod_name_info *)
               DXQueryHashElement( mod_name_hash,
                                   (Pointer)&search)) != NULL)
  {
    DXunlock (mod_name_tables_lock, 0);
    return found->index;
  }

  /*  It's not there, so add it to the list, and to the lookup hash  */
  new = (char *) DXAllocate (strlen (name) + 1);
  if ( !new ) {
    _dxf_ExDie("_dxf_ExGraphInsertAndLookupName:  DXAllocate failed");
    DXunlock (mod_name_tables_lock, 0);
    return (uint32) -1;
  }
  strcpy (new, name);

  APPEND_LIST( char *, *mod_name_str_table, new );

  i = SIZE_LIST( *mod_name_str_table ) - 1;

  info.key   = hashFunc( name );
  info.str   = new;
  info.index = i;

  if ( !DXInsertHashElement( mod_name_hash, (Pointer)&info) ) {
    _dxf_ExDie("_dxf_ExGraphInsertAndLookupName:  DXInsertHashElement failed");
    DXunlock (mod_name_tables_lock, 0);
    return (uint32) -1;
  }

  DXunlock (mod_name_tables_lock, 0);

  return info.index;
}

/*  FIXME:  These are duplicated in path.c  */
#define        EX_I_SEP        ':'             /* instance #   separator       */
#define        EX_M_SEP        '/'             /* macro/module separator       */


static char *_dxf_BuildInstanceNumString(int instance)
{
    static char localstr[10];
    char *tail = localstr;
    int np;

    if (instance < 0 || instance > 999)
    {
    	/* sprintf automatically puts a \0 at the end of a string
    	   so why print a second one? */
        /* sprintf(tail, "%d\0", instance); */
        sprintf(tail, "%d", instance);
        tail += strlen(tail);
    }
    else if (instance > 99)
    {
       np = instance / 100;
       *(tail++) = np + '0';
       instance -= np * 100;
       np = instance / 10;
       *(tail++) = np + '0';
       instance -= np * 10;
       *(tail++) = instance + '0';
    }
    else if (instance > 9)
    {
       np = instance / 10;
       *(tail++) = np + '0';
       instance -= np * 10;
       *(tail++) = instance + '0';
    }
    else
       *(tail++) = instance + '0';
    *(tail) = '\0';

    return localstr;
}

static int _dxf_GetInstanceNumStringLen(int instance)
{
  if ( instance < 0 || instance > 999 )
    return strlen( _dxf_BuildInstanceNumString( instance ) );
  else if ( instance > 99 )
    return 3;
  else if ( instance > 9 )
    return 2;
  else 
    return 1;
} 
  
  
static int _dxf_ExPathStringLen( ModPath *path )
{ 
  int32 i;
  int      len = 0;
    
  for ( i = path->num_comp-1; i >= 0; i-- )
    len += 1 /*EX_M_SEP*/ +
           strlen( *FETCH_LIST( *mod_name_str_table, path->modules[i] ) ) +
           1 /*EX_I_SEP*/ +
           _dxf_GetInstanceNumStringLen( path->instances[i] ) +
           1 /*'\0'*/;

  return len;
}
  
static void _dxf_ExPathToStringBuf( ModPath *path, char *path_str )
{
  int32    i;
  char    *p = path_str;
  char    *module_name;

  for ( i = path->num_comp-1; i >= 0; i-- ) {
    module_name = *FETCH_LIST( *mod_name_str_table, path->modules[i] );
    p[0] = EX_M_SEP;
    strcpy( p+1, module_name );
    p += strlen(p);
    p[0] = EX_I_SEP;
    strcpy( p+1, _dxf_BuildInstanceNumString( path->instances[i] ) );
    p += strlen(p);
  }
}

char *_dxf_ExPathToString( ModPath *path )
{
  static char str[ MAX_PATH_STR_LEN ];
  int len;

  len = _dxf_ExPathStringLen( path );

  if ( len >= sizeof(str) ) {
    _dxf_ExDie ("_dxf_ExPathToString:  Path is too long");
    return NULL;
  }

  _dxf_ExPathToStringBuf( path, str );
  return str;
}

static char *
int16tohex (char *buf, int16 l)
{
    static char *htab = "0123456789abcdef";

    buf[4] = '\0';
    buf[3] = htab[l & 0xf]; l >>= 4;
    buf[2] = htab[l & 0xf]; l >>= 4;
    buf[1] = htab[l & 0xf]; l >>= 4;
    buf[0] = htab[l & 0xf];

    return (buf + 4);
}

static int16
hextoint16 (char *buf)
{
    static char *htab = "0123456789abcdef";
    int16  val = 0;
    int    i;

    for ( i = 0; i < 4; i++ )
      val = ( val << 4 ) | ( strchr( htab, buf[i] ) - htab );
    return val;
}

#define MODPATH_COMP_STR_LEN  \
           (sizeof(_dxd_exCurrentFunc->mod_path.modules  [0]) * 2 + \
            sizeof(_dxd_exCurrentFunc->mod_path.instances[0]) * 2)


/* compare the macro portion of two module ids
 */
Error DXCompareModuleMacroBase(Pointer id1, Pointer id2)
{
    int len;
    char *str1 = (char *)id1;
    char *str2 = (char *)id2;

    if (str1 == NULL || str2 == NULL)
       return ERROR;

    /*  Module ID's are hexified ModPaths: <mod1><inst1><mod2><inst2>...,  */
    /*    stored from highest call level to lowest.                        */
    len = strlen(str1);
    if ( len < MODPATH_COMP_STR_LEN )
       return ERROR;

    return ((strncmp(str1, str2,
                     len - MODPATH_COMP_STR_LEN) == 0) ? OK : ERROR);
}

/* obtain a string for the i'th component of the module ID (e.g. "GetLocal")
 */
const char *DXGetModuleComponentName(Pointer id, int index)
{
  int   i;
  int   len = strlen(id);
  int   num_comp;
  char *str = (char *)id;

  /*  First, determine which component.  Support Python-like negative  */
  /*    indices to count back from the end.                            */
  if ( !str || ( len % MODPATH_COMP_STR_LEN ) != 0 )
    return NULL;

  num_comp = strlen(str) / MODPATH_COMP_STR_LEN;

  if (( index >= num_comp ) || ( index < -num_comp ))
    return NULL;
  else if ( index >= 0 )
    i = index;
  else
    i = num_comp + index;
  i *= MODPATH_COMP_STR_LEN;

  return *FETCH_LIST( *mod_name_str_table, hextoint16( &str[i] ) );
}


static int _dxf_ExPathCacheStringLen( ModPath *path )
{
  return (path->num_comp * MODPATH_COMP_STR_LEN);
}


static void _dxf_ExPathToCacheStringBuf( ModPath *path, char *path_str )
{
  int32    i;
  char    *p = path_str;

  for ( i = path->num_comp-1; i >= 0; i-- ) {
    p = int16tohex( p, path->modules[i] );
    p = int16tohex( p, path->instances[i] );
  }
}

char *_dxf_ExPathToCacheString( ModPath *path )
{
  static char str[ MAX_PATH_STR_LEN ];

  _dxf_ExPathToCacheStringBuf( path, str );
  return str;
}

static char *_dxf_ExCacheStrPrepend( Program *program,
                                     char *name, int instance, char *path )
{
  static char str[ MAX_PATH_STR_LEN ];
  static char tmp[ MAX_PATH_STR_LEN ];
  char        *p = str, *colon;

  /*  This oddball cache-string hacking rtn is only used in one place.  */

  /*  Prepend the IDs for the module name and instance as specified.  */
  p = int16tohex( p, _dxf_ExGraphInsertAndLookupName( program, name ) );
  p = int16tohex( p, instance );

  strcpy(tmp, path);
  /*
  if (tmp[0] != '/')
      s = tmp;
  else
      s = tmp + 1;
  */

  colon = strchr(tmp, ':');
  if (colon)
      *colon = '\0';

  p = int16tohex( p, _dxf_ExGraphInsertAndLookupName( program, tmp+1 ));

  if (colon)
      p = int16tohex( p, atoi(colon+1));
  else
      p = int16tohex( p, 0);

  /*
  strcpy( p, path );
  */
  return str;
}

#ifdef TESTING
static void _dxf_ExPathPrint( Program *p, gfunc *fnode, char *header )
{
  int i;
  ModPath *path = &fnode->mod_path;

  if ( header )
    printf( "\t%s:  ", header );
  else
    printf( "\t" );

  for ( i = path->num_comp-1; i >= 0; i-- )
    printf( "/%s:%d",
            *FETCH_LIST( *mod_name_str_table, path->modules[i] ),
            path->instances[i] );
  printf( "\n" );
}
#endif

static void _dxf_ExPathSet( Program *p, char fname[], int instance, gfunc *fnode )
{
  uint32   fname_key;
  ModPath *path = &fnode->mod_path;

  /*  Note, we store paths in reverse order in mod_path,  */
  /*    so prepend is actually append.                    */
  fname_key = _dxf_ExGraphInsertAndLookupName( p, fname );

  path->modules  [ 0 ] = fname_key;
  path->instances[ 0 ] = instance;
  path->num_comp = 1;

#ifdef TESTING
  _dxf_ExPathPrint( p, fnode, "_dxf_ExPathSet" );
#endif
}




static void _dxf_ExPathPrepend( Program *p, char fname[], int instance,
                                gfunc *fnode )
{
  uint32   fname_key;
  ModPath *path = &fnode->mod_path;

  /*  Note, we store paths in reverse order in mod_path,  */
  /*    so prepend is fast (it's an append).              */
  if ( path->num_comp >= ARRAY_LEN( path->modules ) )
  {
    DXSetError(ERROR_NOT_IMPLEMENTED, "gfunc execution path too deep");
    printf( "gfunc execution path too deep: %d", path->num_comp );
    _dxf_ExDie("gfunc execution path too deep");
    return;
  }

  fname_key = _dxf_ExGraphInsertAndLookupName( p, fname );

  path->modules  [ path->num_comp ] = fname_key;
  path->instances[ path->num_comp ] = instance;
  path->num_comp++;

#ifdef TESTING
  _dxf_ExPathPrint( p, fnode, "_dxf_ExPathPrepend" );
#endif
}

static void _dxf_ExPathAppend( Program *p, char fname[], int instance,
                               gfunc *fnode )
{
  uint32   fname_key;
  ModPath *path = &fnode->mod_path;

  /*  Note, we store paths in reverse order in mod_path,  */
  /*    so append is really a prepend.                    */
  if ( path->num_comp >= ARRAY_LEN( path->modules ) )
  {
    DXSetError(ERROR_NOT_IMPLEMENTED, "gfunc execution path too deep");
    printf( "gfunc execution path too deep: %d", path->num_comp );
    _dxf_ExDie("gfunc execution path too deep");
    return;
  }

  fname_key = _dxf_ExGraphInsertAndLookupName( p, fname );

  memmove( &path->modules[1], &path->modules[0],
           sizeof(path->modules[0]) * path->num_comp );

  path->modules  [ 0 ] = fname_key;
  path->instances[ 0 ] = instance;
  path->num_comp++;

#ifdef TESTING
  _dxf_ExPathPrint( p, fnode, "_dxf_ExPathAppend" );
#endif
}

int _dxf_ExPathsEqual( ModPath *p1, ModPath *p2 )
{
  int cmp;

  /*  We use the same string table for all ModPaths, so this comparison  */
  /*    is quick and simple.  f                                          */
  cmp = ( p1->num_comp == p2->num_comp ) &&
        ( memcmp( p1->modules, p2->modules,
                  sizeof(p1->modules[0]) * p1->num_comp   ) == 0 ) &&
        ( memcmp( p1->instances, p2->instances,
                  sizeof(p1->instances[0]) * p1->num_comp ) == 0 );
  return cmp;
}

void _dxf_ExPathCopy( ModPath *dest, ModPath *src )
{
  memcpy( dest, src, sizeof( *dest ) );
}

char *_dxf_ExGFuncPathToString( gfunc *fnode )
{
  return _dxf_ExPathToString( &fnode->mod_path );
}

static void _dxf_ExGFuncPathToStringBuf( gfunc *fnode, char *path_str )
{
  _dxf_ExPathToStringBuf( &fnode->mod_path, path_str );
}

static void _dxf_ExGFuncPathToCacheStringBuf( gfunc *fnode, char *path_str )
{
  _dxf_ExPathToCacheStringBuf( &fnode->mod_path, path_str );
}

char *_dxf_ExGFuncPathToCacheString( gfunc *fnode )
{
  return _dxf_ExPathToCacheString( &fnode->mod_path );
}

