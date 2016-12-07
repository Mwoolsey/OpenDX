/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* #define CPV_DEBUG */

#include <dxconfig.h>
#include <dx/dx.h>

/*
 * Description:
 * This module contains these basic functions:
 *    1)  ExQueueGraph: 
 *        It takes a Program and resolves inputs and outputs from the global
 *	  dictionary, checks the cache, and decides to what to run.
 *    2)  ExStartModules
 *        It provides a function (called by step 1 and 3) that gets the next
 *        module to run, and puts it in the run queue.
 *    3)  _execGnode
 *        It actually builds the argument list and calls a module.  It also
 *        sticks the result in the cache.  It handles errors, deciding to
 *        skip or kill a module, ....
 */

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_SYS_TIMES_H)
#include <sys/times.h>
#endif

#include "evalgraph.h"
#include "config.h"
#include "graph.h"
#include "pmodflags.h"
#include "context.h"
#include "utils.h"
#include "cache.h"
#include "cachegraph.h"
#include "swap.h"
#include "log.h"
#include "packet.h"
#include "attribute.h"
#include "rq.h"
#include "status.h"
#include "vcr.h"
#include "_variable.h"
#include "sysvars.h"
#include "distp.h"
#include "graphIntr.h"
#include "exobject.h"

extern void DXMarkTimeX(char *s); /* from libdx/timing.c */
extern int _dxfHostIsLocal(char *host); /* from libdx/ */

/* Internal use functions */
static int  QueueSendRequest(qsr_args *args);
static void ExExecError(Program *p, gfunc *n, ErrorCode code, Error status);
static int  _execGnode(Pointer n);
static void ExStartModules(Program *p);
static int  ExFixSwitchSelector(Program *p, gfunc *gf,
                                int executionTime, int requiredFlag);
static int  ExFixRouteSelector(Program *p, gfunc *gf,
                                 int executionTime, int requiredFlag);
static int  ExProcessGraphFunc(Program *p, int fnbr, int executionTime);
static void ExRefFuncIO(Program *p, int fnbr);
static int  ExCheckDDI(Program *p, gfunc *gf);
static int  ExCacheDDIInputs(Program *p, gfunc *gf);
static void ExDoPreExecPreparation(Program *p);
static int  ExProcessDPSENDFunction(Program *p, gfunc *gf, Object *DPSEND_obj);
static int  ExRunOnThisProcessor(char *procid);
static int  ExCheckInputAvailability(Program *p, gfunc *gf, int fnbr);
static void ExFixConstantSwitch(Program *p);
static void ExCleanupForState();

long
ExTime()
{
    uint32 dxtime;
#if DXD_HAS_LIBIOP
    /*
     * SVS time is floating-point seconds to microsecond
     * resolution; multiply by 100 to get time to 100ths
     */
    dxtime = (uint32) SVS_double_time()*100;
#else
#ifdef DXD_WIN
    time_t ltime;
    dxtime = time(&ltime);  /* from 2.1.5 branch   */
#else
    struct tms buf;
    dxtime = (uint32)times(&buf);
#endif
#endif
    return dxtime;
}

#define CHECK_EXEC_OBJS         1

#define	MARK_1(_x)		ExMarkTime (0x10, _x)

#define LOCAL_ARG_MAX           512	/* max bytes to hold statically */
#define MAX_LABEL_LEN		64
#define MAX_ROUTE_OUTPUTS       21

#define INCR_RUNABLE \
    { p->runable++;\
    if(_dxd_exContext->subp[0] != p) _dxd_exContext->subp[0]->runable++; }

#define DECR_RUNABLE \
    { if(p->runable > 0) p->runable--; \
    if(_dxd_exContext->subp[0] != p && _dxd_exContext->subp[0]->runable > 0) \
        _dxd_exContext->subp[0]->runable--; }

enum REQ_TYPE
{
    NOT_SET = -1, NOT_REQUIRED = 0, REQUIRED, ALREADY_RUN, REQD_IFNOT_CACHED, RUNNING
};


typedef struct execArg
{
    Program        *p;
    gfunc          *gf;
}               execArg;

static int      graph_id;

int            *_dxd_exKillGraph = NULL;
int            *_dxd_exBadFunc = NULL;

void ExCacheNewGvar(gvar *gv)
{
    gvar *cachegvar;
    cachegvar = _dxf_ExCreateGvar(GV_UNRESOLVED);
    _dxf_ExDefineGvar(cachegvar, gv->obj);
    cachegvar->reccrc = gv->reccrc;
    cachegvar->cost = gv->cost;
    ExReference(gv->oneshot);
    cachegvar->oneshot = gv->oneshot;
    _dxf_ExCacheInsert(cachegvar);
}

/*
 * Initialize graph execution environment 
 */
Error
_dxf_ExQueueGraphInit()
{
    graph_id = 0;

    if ((_dxd_exKillGraph = (int *) DXAllocate(sizeof(int))) == NULL)
	return (ERROR);

    if ((_dxd_exBadFunc = (int *) DXAllocate(sizeof(int))) == NULL)
	return (ERROR);

    *_dxd_exKillGraph = FALSE;
    *_dxd_exBadFunc = FALSE;

    return (OK);
}


/*
 * evaluate a graph 
 */
void
_dxf_ExQueueGraph(Program * p)
{
    int             i, j, index;
    gvar           *global;
    ProgramVariable *pv;
    ProgramRef     *pr;
    gfunc          *gf, *gf2;
    int             ilimit, jlimit;
    DistMsg	    msgtype;

    MARK_1("_dxf_ExQueueGraph");

    *_dxd_extoplevelloop = FALSE;
    /*
     * Send msg to all slave processors to execute their graphs
     */
    if (!_dxd_exRemoteSlave) {
        _dxf_ResetSlavesDone();
        msgtype = DM_EXECGRAPH;
        for (i = 0, ilimit = SIZE_LIST(p->funcs); i < ilimit; ++i) {
	    gf = FETCH_LIST(p->funcs, i);
            if(!_dxf_ExValidGroupAttach(gf->procgroupid)) {
                *_dxd_exKillGraph = TRUE;
                DXUIMessage("ERROR", 
                  "Bad group attachment: Terminating graph execution");
	        msgtype = DM_KILLEXECGRAPH;
                break;
            }
            if(ilimit == 1 && gf->ftype == F_MACRO)
                for(j=0,jlimit=SIZE_LIST(gf->func.subP->funcs); j<jlimit; ++j) {
	            gf2 = FETCH_LIST(gf->func.subP->funcs, j);
                    if(!_dxf_ExValidGroupAttach(gf2->procgroupid)) {
                        *_dxd_exKillGraph = TRUE;
                        DXUIMessage("ERROR", 
                          "Bad group attachment: Terminating graph execution");
	                msgtype = DM_KILLEXECGRAPH;
                        break;
                    }
                }
        }
        if(ilimit > 0) /* we have at least one function in graph */
            _dxf_ExDistributeMsg(msgtype, (Pointer)p->graphId, 0, TOSLAVES);
    }

    if(SIZE_LIST(p->funcs) != 0) {
        _dxd_exContext->program = p;
        _dxd_exContext->subp = (Program **)DXAllocateZero(
                                 sizeof(Program *) * (SIZE_LIST(p->macros)+1));
        if(_dxd_exContext->subp == NULL)
            _dxf_ExDie("Could not allocate memory for list of subprograms");
        _dxd_exContext->subp[0] = p;
    }

    p->informedMaster = FALSE;

    /*
     * Look up all undefineds, keep a reference to them until the end.  If
     * the undefined isn't found (or is named NULL) define a null gvar. 
     */
    for (i = 0, ilimit = SIZE_LIST(p->undefineds); i < ilimit; ++i)
    {
	pr = FETCH_LIST(p->undefineds, i);
	pv = FETCH_LIST(p->vars, pr->index);
	if (pv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 001");
	if (strcmp(pr->name, "NULL") == 0)
	{
	    pv->gv = _dxf_ExCreateGvar(GV_DEFINED);
	    ExReference(pv->gv);
	    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for NULL pvar\n");
#endif
	}
	else
	{
	    global = (gvar *)_dxf_ExVariableSearch(pr->name, _dxd_exGlobalDict);
	    if (global == NULL)
	    {
		if (!_dxd_exRemote && !_dxd_exRemoteSlave)
		{
		    DXWarning("#4680", pr->name);
		}
		pv->gv = _dxf_ExCreateGvar(GV_DEFINED);
		ExReference(pv->gv);
		pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for undefined pvar\n");
#endif
	    }
	    else
	    {
		pv->gv = global;
		pv->gv->skip = GV_DONTSKIP;
		pv->gv->disable_cache = FALSE;
		pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for global pvar\n");
#endif
	    }
	}
	/* Put an extra reference on all variables from the dictionary. */
	ExReference(pv->gv);
	pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for variable pvar\n");
#endif
    }

    /* Reset oneshots and free names. */
    for (i = 0, ilimit = SIZE_LIST(p->undefineds); i < ilimit; ++i)
    {
	pr = FETCH_LIST(p->undefineds, i);
	if (strcmp(pr->name, "NULL") != 0)
	{
	    global = (gvar *)_dxf_ExVariableSearch(pr->name, _dxd_exGlobalDict);
	    if (global != NULL)
	    {
		/*
		 * If the original variable was a oneshot, reset it.
		 * Null out the gvar's version so it doesn't get deleted twice.
		 */
		if (global->oneshot)
		{
		    Object o = global->oneshot;
		    global->oneshot = NULL;
                    ExDelete(global);
		    global = _dxf_ExCreateGvar(GV_UNRESOLVED);
		    _dxf_ExDefineGvar(global, o);
		    _dxf_ExVariableInsertNoBackground(pr->name,
					  _dxd_exGlobalDict,
					  (EXObj) global);
		    DXUIMessage(pr->name, "Resetting oneshot");
		}
                else
                    ExDelete(global);
	    }
	}
	DXFree((Pointer) pr->name);
	pr->name = NULL;
    }



    MARK_1("undefineds");

    /*
     * Define all variables: If there is not yet a gvar for each variable,
     * make one, and if this is a constant or default (pv->defined == TRUE),
     * set the gvar to be this value 
     */
    for (i = 0, ilimit = SIZE_LIST(p->vars); i < ilimit; ++i)
    {
	pv = FETCH_LIST(p->vars, i);
	if (pv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 004");
	/*
	 * This first clause takes care of everything but the undefineds
	 * noted above.  All vars are refed again because non-intermidiate
	 * variables need an extra reference to be held on to.
	 */
#if 0
	if (pv->gv == NULL)
	{
	    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
	    ExReference(pv->gv);
	    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for some pvar\n");
#endif
	    if (pv->defined)
	    {
		_dxf_ExDefineGvar(pv->gv, pv->obj);
		/* Put an extra reference on all constants. */
		ExReference(pv->gv);
		pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for defined pvar\n");
#endif
	    }
	}
#endif
	if(pv->gv == NULL && pv->defined)
	{
	    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
	    ExReference(pv->gv);
	    pv->refs++;
	    _dxf_ExDefineGvar(pv->gv, pv->obj);
/*??????*/
            /* Put an extra reference on all constants. */
	    ExReference(pv->gv);
	    pv->refs++;
        }
    }

    MARK_1("define vars");

    for (i = 0, ilimit = SIZE_LIST(p->wiredvars); i < ilimit; ++i)
    {
        index = *FETCH_LIST(p->wiredvars, i);
        ExDebug("*4", "program %x, wired var %d\n", p, index);
	pv = FETCH_LIST(p->vars, index);
	if (pv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 004");
	if (pv->gv == NULL)
	{
	    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
	    ExReference(pv->gv);
	    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for some pvar\n");
#endif
        }
        ExDebug("*4", "wired var already has gv 1\n");
    }

    MARK_1("define wired vars");

    /* Mark all outputs as required. */
    for (i = 0, ilimit = SIZE_LIST(p->outputs); i < ilimit; ++i)
    {
	pr = FETCH_LIST(p->outputs, i);
	if (pr == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 002");
	pv = FETCH_LIST(p->vars, pr->index);
	if (pv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 003");
	if (!pv->defined)
	{
	    ExDebug("*1", "Marking output %d at index %d as required",
		    i, pr->index);
	    pv->required = REQUIRED;
	}
    }
    MARK_1("require outputs");

    ExFixConstantSwitch(p);

	
    /* Put all outputs in global dictionary, remembering oneshot defaults */
    for (i = 0, ilimit = SIZE_LIST(p->outputs); i < ilimit; ++i)
    {
	pr = FETCH_LIST(p->outputs, i);
	if (pr == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 002");
	pv = FETCH_LIST(p->vars, pr->index);
	if (pv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 003");
	pv->gv->oneshot = pr->oneshot;
	_dxf_ExVariableInsert(pr->name, _dxd_exGlobalDict, (EXObj) pv->gv);
        /* put an extra reference on gvar to be deleted just after */
        /* sending outputs to slaves */
	ExReference(pv->gv);
	pv->refs++;
    }

    MARK_1("define outputs");

    /* Compute Cache recipes */
    for (i = 0, ilimit = SIZE_LIST(p->funcs); i < ilimit; ++i)
    {
	_dxf_ExComputeRecipes(p, i);
    }

#if 0
    /* moved this into ComputeRecipes */
    for (i = 0, ilimit = SIZE_LIST(p->vars); i < ilimit; ++i)
    {
	pv = FETCH_LIST(p->vars, i);
        if(pv->gv)
	    pv->reccrc = pv->gv->reccrc;
    }

#endif
    MARK_1("compute recipes");


    /*
     * Loop thru all functions in graph and det. which are required
     */
    for (i = SIZE_LIST(p->funcs) - 1; i >= 0; --i)
	ExProcessGraphFunc(p, i, 0);

    MARK_1("process funcs");

    if (p->origin == GO_BACKGROUND)
    {
	ExDebug("*1", "Sending BG msg to UI");
	DXUIMessage("BG", "begin");
    }
    p->first_skipped = -1;
    p->cursor = -1;
    ExDoPreExecPreparation(p);

    /* we used to delete unneccessary variables at this point but  */
    /* we are no longer evaluating the whole graph at once.     */
    /* With the changes for macros we only evaluate a macro if  */
    /* it is needed. Is it a problem to leave unused variables  */
    /* around until the program is deleted? I don't think they  */
    /* are using much space.  */

    ExDebug("*1", "Before ExDelete of pvar loop at end of graph execution");
    for (i = 0, ilimit = SIZE_LIST(p->wiredvars); i < ilimit; ++i)
    {
        index = *FETCH_LIST(p->wiredvars, i);
	pv = FETCH_LIST(p->vars, index);
        ExDebug("*6", "var %d, count %d\n", index, pv->gv->object.refs);
	ExDelete(pv->gv);
	if (--pv->refs == 0)
	    pv->gv = NULL;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for deleting pvar\n");
#endif
    }
    MARK_1("delete vars");

    p->cursor = -1;
    ExStartModules(p);

}

void ExCacheMacroResults(Program *p)
{
    gfunc *n;
    gvar *gv;
    int i, *ip, index;
    ProgramVariable *pv;
    double cost;

    n = p->saved_gf;
     
    if(n->ftype == F_MODULE)
        return;
       
    n->endtime = ExTime();
    cost = (double) n->endtime - n->starttime;

    if(n->func.subP->error && n->nout == 0 && !(n->flags & MODULE_SIDE_EFFECT)
)
        _dxf_ExManageCacheTable(&n->mod_path, 0, 0);

    for (i = 0; i < n->nout; ++i) {
	ip = FETCH_LIST(n->outputs, i);
	if (ip && *ip >= 0) {
	    pv = FETCH_LIST(p->vars, *ip);
	    if (pv == NULL)
	        _dxf_ExDie("Executive Inconsistency: _execGnode 003");
            gv = pv->gv;

            /* restore the saved cache tags */
            index = SIZE_LIST(pv->cts) - 1;
            pv->reccrc = *FETCH_LIST(pv->cts, index);
            /* gvar may have been deleted if it's not needed anymore */
            if(pv->gv)
                pv->gv->reccrc = pv->reccrc;
            DELETE_LIST(uint32, pv->cts, index);

            if(gv->skip || n->func.subP->error)
                continue;
            if(n->flags & MODULE_CHANGES_STATE)
	        _dxf_ExSetGVarCost(gv, CACHE_PERMANENT);
            else
	        _dxf_ExSetGVarCost(gv, cost);
	    if (_dxd_exCacheOn) {
 	        if (pv->excache != CACHE_OFF && pv->cacheattr)
                    ExCacheNewGvar(gv);
		else if (n->excache != CACHE_OFF &&
			(pv->excache != CACHE_OFF || !pv->cacheattr))
                    ExCacheNewGvar(gv);
	    }
	}
    }
    /*
     * Delete inputs and outputs (outputs first), just in case someone passed
     * an input through unchanged.  If the output was generated in error, don't
     * delete it (to be sure it sticks around for subsequent modules).
     */
    for (i = 0; i < n->nout; i++)
    {
	index = *FETCH_LIST(n->outputs, i);
	if (index > -1)
	{
	    pv = FETCH_LIST(p->vars, index);
 	    ExDelete(pv->gv);
	    if (--pv->refs == 0) {
		pv->gv = NULL;
                ExDebug("*6","deleting %s output %d %d\n",
		               _dxf_ExGFuncPathToString(n),i,index);
            }
	    pv->required = NOT_REQUIRED;
	}
    }
    for (i = 0; i < n->nin; i++)
    {
	index = *FETCH_LIST(n->inputs, i);
	if (index > -1)
	{
	    pv = FETCH_LIST(p->vars, index);
	    ExDelete(pv->gv);
	    if (--pv->refs == 0) {
		pv->gv = NULL;
                ExDebug("*6","deleting %s input %d %d\n",
		               _dxf_ExGFuncPathToString(n),i,index);
            }
	}
    }
    
    DECR_RUNABLE
}

Error
ExQueueSubGraph(Program * p)
{
    int i, ilimit, nfuncs;
    int j, jlimit, index;
    ProgramVariable *pv;
    gfunc *gf;

    _dxd_exContext->program = p;
    _dxd_exContext->subp[p->subgraphId] = p;

    for (i = 0, ilimit = SIZE_LIST(p->wiredvars); i < ilimit; ++i)
    {
        index = *FETCH_LIST(p->wiredvars, i);
        ExDebug("*4", "program %x, wired var %d\n", p, index);
	pv = FETCH_LIST(p->vars, index);
	if (pv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 004");
	if (pv->gv == NULL)
	{
	    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
	    ExReference(pv->gv);
	    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for some pvar\n");
#endif
        }
        else {
	    ExReference(pv->gv);
	    pv->refs++;
        }
    }

    MARK_1("define wired vars");

    nfuncs = SIZE_LIST(p->funcs);

    for (i = 0, ilimit = SIZE_LIST(p->funcs); i < ilimit; ++i)
    {
	gf = FETCH_LIST(p->funcs, i);
        if(p->subgraphId == 1 && (gf->flags & MODULE_LOOP)) {
            /* First time we set it send warning message */
            if(*_dxd_extoplevelloop == FALSE && !_dxd_exRemoteSlave &&
               *_dxd_exNSlaves > 0) 
                DXUIMessage ("WARNING MSGERRUP",
                             "Cannot distribute parts of loop, all modules will run on host %s", _dxd_exHostName);
            *_dxd_extoplevelloop = TRUE;
        }

        /* This is not longer needed because UndefineGvar takes care of it*/
#if 0
        /* GDA */
	/*
	 * Any modules skipped due to a route on the last time through
	 * the loop must be reconsidered this time.
	 */

	if (gf->required == ALREADY_RUN)
	{
	    int j, jlimit;

	    for (j = 0, jlimit = SIZE_LIST(gf->inputs); j < jlimit; ++j)
	    {
                 
	    	int index = *FETCH_LIST(gf->inputs, j);
	    	if (index >= 0)
		{
		    pv = FETCH_LIST(p->vars, index);
		    if (pv && pv->gv && pv->gv->skip == GV_SKIPROUTE)
		        pv->gv->skip = GV_DONTSKIP;
		} 
	    }
	    
	    if (IsRoute(gf))
	        gf->required = REQUIRED;
        }

#endif
	for (j = 0, jlimit = SIZE_LIST(gf->outputs); j < jlimit; ++j)
	{
	    index = *FETCH_LIST(gf->outputs, j);
	    if (index >= 0)
	    {
		pv = FETCH_LIST(p->vars, index);
                if(!pv->gv)
	            _dxf_ExDie(
                      "Executive Inconsistency: missing gv structure for %dth variable, in function %s ", index, gf->name);
                if(pv->gv->type == GV_UNRESOLVED)
                    pv->reccrc = pv->gv->reccrc = 0;
            }
        }

        if(gf->required != NOT_SET)
            gf->required = NOT_REQUIRED;
    }

    ExFixConstantSwitch(p);

    /* Compute Cache recipes */
    for (i = 0; i < nfuncs; ++i)
    {
	_dxf_ExComputeRecipes(p, i);
    }

#if 0
    /* moved this into ComputeRecipes */
    /* Can this be moved into ComputeRecipes?????*/
    for (i = 0, ilimit = SIZE_LIST(p->vars); i < ilimit; ++i)
    {
	pv = FETCH_LIST(p->vars, i);
        if(pv->gv)
	    pv->reccrc = pv->gv->reccrc;
    }

#endif
    MARK_1("compute recipes");

    /*
     * Loop thru all functions in graph and det. which are required
     */
    for (i = nfuncs - 1; i >= 0; --i)
	ExProcessGraphFunc(p, i, 0);

    MARK_1("process funcs in subprogram");

    p->first_skipped = -1;
    p->cursor = -1;
    ExDoPreExecPreparation(p);

    ExDebug("*1", "Before ExDelete of pvar loop at end of graph execution");
    for (i = 0, ilimit = SIZE_LIST(p->wiredvars); i < ilimit; ++i)
    {
        index = *FETCH_LIST(p->wiredvars, i);
	pv = FETCH_LIST(p->vars, index);
        ExDebug("*6", "var %d, count %d\n", index, pv->gv->object.refs);
	ExDelete(pv->gv);
	if (--pv->refs == 0)
	    pv->gv = NULL;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for deleting pvar\n");
#endif
    }
    MARK_1("delete vars");

    ExStartModules(p);

    return OK; 
}

static int _QueueableStartModules(Pointer pp)
{
    Program *p = (Program *)pp;
    ExStartModules(p);
    return (0); 
}

static int _QueueableStartModulesReset(Pointer pp)
{
    Program *p = (Program *)pp;
    p->cursor = -1;
    p->outstandingRequests--;

    ExStartModules(p);
    return (0);
}


static void
ExStartModules(Program *p)
{
    int             i;
    gfunc          *gf;
    int             procId;
    execArg        *ta;
    char            pbuf[64];
    int             found = FALSE;
    int             past;
    int             ilimit;
    ProgramVariable *pv;
    ProgramRef     *pr;
    int             nfuncs;
    int		    oldCursor;
    gvar	    *gv;

    ExDebug("*1", "In ExStartModules with program %x\n", 
            _dxd_exContext->program);
    /* check to see if program was deleted already */
    /*
    if(_dxd_exContext->program == NULL)
        return;
    */

    nfuncs = SIZE_LIST(p->funcs);

    past = p->cursor + 1 >= nfuncs;

    ilimit = nfuncs;

    oldCursor = p->cursor;
    for (i = p->cursor + 1; i < ilimit; ++i)
    {
	if (p->cursor != oldCursor)
	{
	    i = p->cursor + 1;
	    oldCursor = p->cursor;
	}
	gf = FETCH_LIST(p->funcs, i);
        if(*_dxd_exBadFunc == FALSE && gf->ftype == F_MODULE && 
           gf->func.module == m__badfunc &&
          (_dxd_exRemoteSlave || *_dxd_exNSlaves > 0)) {
            m__badfunc(NULL, NULL);
            ExExecError(p, gf, ERROR_NOT_IMPLEMENTED, ERROR);
            *_dxd_exKillGraph = TRUE;
            *_dxd_exBadFunc = TRUE;
            _dxf_ExDistributeMsg(DM_BADFUNC, NULL, 0, TOPEERS);
        }
	if (gf->required == REQUIRED)
	{
            /* for top level macro we shouldn't have a process */
            /* group assignment and all machines will have to  */
            /* evaluate the top level macro.                   */
            if((gf->ftype == F_MACRO && gf->procgroupid == NULL &&
               p == _dxd_exContext->subp[0]) ||
	       ((ExRunOnThisProcessor(gf->procgroupid) ||
		 !strcmp(gf->name, "Trace"))
		&& ExCheckInputAvailability(p, gf, i)))
	    {
		found = TRUE;
		ta = (execArg *) DXAllocate(sizeof(execArg));
		if (ta == NULL)
		    _dxf_ExDie("Executive Inconsistency: ExStartModules 001");

		ta->p = p;
		ta->gf = gf;
		procId = ((gf->flags & MODULE_PIN) != 0 && DXProcessors(0) > 1) ?
		    EX_PIN_PROC :
		    0;
		MARK_1("RQEnqueue");
		if (!IsSwitch(gf) && !IsRoute(gf))
		    gf->required = RUNNING;
		p->cursor = i;
		_dxf_ExRQEnqueue(_execGnode, (Pointer) ta, 1, 0, procId, FALSE);
		break;
	    }
	}
    }

    if (!found || past)
    {
	if (p->first_skipped != -1)
	{
            if(p->cursor != (p->first_skipped -1)) {
                if(p->cursor > -1) {
                    gfunc *dgf;
                    dgf = FETCH_LIST(p->funcs, p->first_skipped);
                    ExDebug("1", "Queueing StartModules with cursor at %s", 
		                           _dxf_ExGFuncPathToString(dgf));
                }
            }
	    p->cursor = p->first_skipped - 1;
	    ExDebug("5", "Should reset cursor to %d at this point", p->first_skipped);
	    p->first_skipped = -1;
	    _dxf_ExRQEnqueue(_QueueableStartModules, (Pointer) p,
			     1, 0, 0, FALSE);
            return;
	}
        /* if this is a loop and we are not done, re-execute the loop */
        if(p->loopctl.isdoneset)
            p->loopctl.done = TRUE;
        if(!p->loopctl.done) {
            /* need to redo cache tags */
            p->loopctl.first = FALSE;
            p->loopctl.counter++;
            MARK_1("rerun loop");
            _dxf_ExRunOn(1, ExQueueSubGraph, (Pointer)p, 0);
            return;
        }

        if(p->returnP)  {
            ExCleanupForState();
	    ExCacheMacroResults(p->returnP);
            p->loopctl.first = TRUE;
            p->loopctl.done = TRUE;
            p->loopctl.isdoneset = FALSE;
            p->loopctl.counter = 0;
            _dxd_exContext->program = p->returnP;
            if(p->error) {
                DXLoopDone(1);
                p->returnP->error = TRUE;
            }
            ExStartModules(p->returnP);
        }
	else
	{
	    GDictSend   pkg;

	    _gorigin        origin = p->origin;
	    int             graphId = p->graphId;
	    int             error = p->error;
	    int             frame = p->vcr.frame;
	    int             nframe = p->vcr.nframe;
	    int             stop = p->vcr.stop;
            int             kill = *_dxd_exKillGraph;
            gvar            *new_gvar;
            
	    /* Send all outputs to peers */
            /* Make a seperate gvar for each oneshot. Oneshots can */
            /* not share a gvar with another variable. */
            /* If they share a gvar then the wrong variable might get */
            /* reset after it is referenced once. */
            for (i = 0, ilimit = SIZE_LIST (p->outputs); i < ilimit; ++i)
	    {
	        pr = FETCH_LIST(p->outputs, i);
                pv = FETCH_LIST(p->vars, pr->index);
                gv = pv->gv;
                if(gv == NULL)
                    _dxf_ExDie("Executive Inconsistency: Null Gvar - cannot send output var to peers");
                if(gv->oneshot) {
		    new_gvar = _dxf_ExCreateGvar(GV_UNRESOLVED);
		    _dxf_ExDefineGvar(new_gvar, gv->obj);
                    new_gvar->oneshot = gv->oneshot;
                    new_gvar->reccrc = gv->reccrc;
		    gv->oneshot = NULL;
		    _dxf_ExVariableInsertNoBackground(pr->name,
					  _dxd_exGlobalDict,
					  (EXObj) new_gvar);
                }    
		if (gv->type != GV_UNRESOLVED) {
		    _dxf_ExCreateGDictPkg(&pkg, pr->name, &gv->object);
		    ExDebug("7", "sending cache for variable %s", pr->name);
		    _dxf_ExDistributeMsg(DM_INSERTGDICT, (Pointer)&pkg,
                                         sizeof(GDictSend), TOSLAVES);
		}
                /* we put an extra ref count on gvar at beginning of */
                /* _dxf_ExQueueGraph so the gvar would still be      */
                /* available here. */
                ExDelete(gv);
	        pv->refs--;
		DXFree ((Pointer) pr->name);
		pr->name = NULL;
       	    }

	    if (_dxd_exRemoteSlave)
	    {
		if (!p->informedMaster)
		    _dxf_ExDistributeMsg(DM_GRAPHDONE, NULL, 0, TOMASTER);
		p->informedMaster = TRUE;
		if (p->deleted && p->runable == 0)
		    _dxf_ExGQDecrement(p);
	    }
	    else 	/* master, wait for children to finish */
	    {
	        if(nfuncs > 0 && !p->deleted && !p->waited) {
		    p->waited = TRUE;
                    _dxf_ExRunOn(1, _dxf_ExWaitOnSlaves, NULL, 0);
                }
		if (p->outstandingRequests == 0)
		    _dxf_ExGQDecrement(p);
	    }

	    switch (origin)
	    {
	    case GO_FOREGROUND:
		ExDebug("*1", "Completing %d [%08x] FOREGROUND",
			graphId, p);
		break;

	    case GO_BACKGROUND:
		DXUIMessage("BG", "end");
		ExDebug("*1", "Completing %d [%08x] BACKGROUND",
			graphId, p);
		break;

	    case GO_VCR:
		if (_dxd_exRemote && !error && !kill)
		{
		    sprintf(pbuf, "frame %12d %12d", frame, nframe);
		    _dxf_ExSPack(PACK_INTERRUPT, graphId, pbuf, 31);
		}
		_dxf_ExVCRCallBack(graphId);

		ExDebug("*1", "Completing %d [%08x] VCR frame %d %s",
			graphId, p, frame, stop ? "STOP" : "");
		break;

	    default:
		ExDebug("*1", "Completing %d [%08x] ???",
			graphId, p);
		break;
	    }
	    return;
	}
    }
}

static void
ModBeginMsg(Program *p, gfunc *n, char *bell, int *len)
{

    if (_dxd_exShowBells)
    {
        strcpy(bell, "begin ");
        strcat(bell, _dxf_ExGFuncPathToString(n));
        *len = strlen(bell);
        _dxf_ExSPack(PACK_INTERRUPT, p->graphId, bell, *len);
    }
    if(_dxd_exRemoteSlave)
        DXDebug("*0", "Planning [%08x] %d::%s on %s", n, 
                       p->graphId, _dxf_ExGFuncPathToString(n), _dxd_exHostName);
    else
        DXDebug("*0", "Planning [%08x] %d::%s", n, p->graphId, 
                       _dxf_ExGFuncPathToString(n));
}


static void
ModEndMsg(Program *p, char *bell, int len)
{

    if (_dxd_exShowBells) {
        bell[0] = 'e';
        bell[1] = 'n';
        bell[2] = 'd';
        bell[3] = ' ';
        bell[4] = ' ';
        _dxf_ExSPack(PACK_INTERRUPT, p->graphId, bell, len);
    }
}

/*
 * Once all of the inputs to a node are available, this routine does the
 * actual execution of the module code. 
 */
static int
_execGnode(Pointer ptr)
{
    execArg        *ta = (execArg *) ptr;
    Program        *p = ta->p;
    gfunc          *n = ta->gf;
    int             i;
    int             index;
    gvar           *gv;
    Error           status = ERROR;
    ErrorCode       code;
    int             runStatus;
    int             nin;
    int             nout;
    int             argsize;
    static int      argsalloc = 0;
    static Object  *args = NULL;

    ProgramVariable *pv;
    int            *ip;
    double          cost;
    char            bell[512];
    int             len = 0;
    char            buf[33];
    int             skipping = FALSE;
    int             ilimit;
    int             nbr_inputs_inerror;
    int             nbr_inputs_routeoff;
    int             nbr_inputs_null;
    Context         savedContext;
    int		    dpSendOtherProc = FALSE;

    MARK_1("_execGnode");

    DXFree((Pointer) ptr);

    if (*_dxd_exKillGraph)
    {
	DXDebug("*0", "Killing  [%08x] %d::%s", n, p->graphId, 
			 _dxf_ExGFuncPathToString(n));
        if(n->nout == 0 && !(n->flags & MODULE_SIDE_EFFECT))
            _dxf_ExManageCacheTable(&n->mod_path, 0, 0);

	for (i = 0, ilimit = SIZE_LIST(n->outputs); i < ilimit; ++i)
	{
	    ip = FETCH_LIST(n->outputs, i);
	    if (ip && *ip >= 0)
	    {
		pv = FETCH_LIST(p->vars, *ip);
		if (pv == NULL)
		    _dxf_ExDie("Executive Inconsistency: _execGnode 005");
		pv->gv->skip = GV_SKIPERROR;
		_dxf_ExDefineGvar(pv->gv, NULL);
	    }
	}
        p->loopctl.done = TRUE;
        p->loopctl.isdoneset = TRUE;
    }
    else
    {
	ExDebug("*1", "In execGnode for func :%s", _dxf_ExGFuncPathToString(n));
	if (IsSwitch(n))
	{
	    if (ExFixSwitchSelector(p, n, TRUE, REQUIRED))
	    {
		ExDoPreExecPreparation(p);
		p->cursor = -1;
		ExStartModules(p);
		return (OK);
	    }
	    else
	    {
	        if (_dxd_exEnableDebug)
	            printf("Starting Module  %s\n", _dxf_ExGFuncPathToString(n));
                /* you won't be able to see the light go green */
                /* since the messages are close together but   */
                /* the UI can do something with the messages   */
                /* to tell that this Switch has been run.      */
                ModBeginMsg(p, n, bell, &len);
	        n->required = ALREADY_RUN;
                ModEndMsg(p, bell, len);
	    }
	    goto module_cleanup;
	}
	if (IsRoute(n))
	{

	    if (ExFixRouteSelector(p, n, TRUE, REQUIRED))
	    {
		ExDoPreExecPreparation(p);
		p->cursor = -1;
		ExStartModules(p);
		return (OK);
	    }
	    else
	    {
	        if (_dxd_exEnableDebug)
	            printf("Starting Module  %s\n", _dxf_ExGFuncPathToString(n));
                /* you won't be able to see the light go green */
                /* since the messages are close together but   */
                /* the UI can do something with the messages   */
                /* to tell that this Route has been run.       */
                ModBeginMsg(p, n, bell, &len);
		n->required = ALREADY_RUN;
                ModEndMsg(p, bell, len);
	    }
	    goto module_cleanup;
	}

	nin = n->nin;
	nout = n->nout;

/*---------------------------------------------------------------------*/
/* See if module needs to be scheduled. The return from ExCheckDDI     */
/* will be FALSE if it does not need to be scheduled, and the input(s) */
/* will have been properly passed thru to the receiving module(s)      */
/*---------------------------------------------------------------------*/
	if (n->flags & MODULE_REROUTABLE)
	{
	    if (!ExCheckDDI(p, n))
	    {
		goto module_cleanup;
	    }
	}

	/* Allocate arg-list if not enough has been allocated so far */
	argsize = (nin + nout + 1) * sizeof(Object);
	if (argsalloc <= argsize)
	{
	    argsize = MAX(argsize, LOCAL_ARG_MAX);
	    if (args != NULL)
		DXFree((Pointer) args);
	    args = (Object *) DXAllocateLocal(argsize);
	    if (args == NULL)
		_dxf_ExDie("_execGnode:  can't allocate local memory");
	    argsalloc = argsize;
	}
	ExZero(args, argsize);

	/*
	 * build up the vector of inputs 
	 */
	nbr_inputs_inerror = 0;
	nbr_inputs_routeoff = 0;
	nbr_inputs_null = 0;
	MARK_1("build input");
	for (i = 0; i < nin; i++)
	{
	    index = *FETCH_LIST(n->inputs, i);
	    if (index == -1)
	    {
		args[i] = NULL;
		nbr_inputs_null++;
	    }
	    else
	    {
		pv = FETCH_LIST(p->vars, index);
		gv = pv->gv;
                switch(gv->skip) {
                  case GV_DONTSKIP:
                      break;
                  case GV_SKIPERROR:
                      nbr_inputs_inerror++;
                      break;
                  case GV_SKIPROUTE:
                      nbr_inputs_routeoff++;
                      break;
		  case GV_SKIPMACROEND:
		      break;
                }

		/*
		 * If we shouldn't run after an error, mark outputs as
		 * errors, and return.  Otherwise, set the args array
		 * appropriately, and remember if we were executed under
		 * error conditions or not. 
		 */
		if (gv->skip && (n->flags & MODULE_ERR_CONT) == 0)
		{
		    DXDebug("*0", "Skipping [%08x] %d::%s",
			    n, p->graphId, _dxf_ExGFuncPathToString(n));
		    for (i = 0, ilimit = SIZE_LIST(n->outputs); i < ilimit; ++i)
		    {
			ip = FETCH_LIST(n->outputs, i);
			if (ip && *ip >= 0)
			{
			    pv = FETCH_LIST(p->vars, *ip);
			    if (pv == NULL)
				_dxf_ExDie("Executive Inconsistency: _execGnode 004");
			    pv->gv->skip = gv->skip;
			    _dxf_ExDefineGvar(pv->gv, NULL);
			}
		    }
		    goto error_continue;
		}
		else
		{
		    skipping |= ((gv->skip == GV_SKIPERROR) || gv->disable_cache);
		    if (gv == NULL || gv->skip)
			args[i] = NULL;
		    else
		    if (pv->dflt == NULL && gv->type == GV_UNRESOLVED)
		    {
			ExDebug("*1", " _execGnode 001:   [%2d] type = %2d %d::%s",
				i, gv->type, p->graphId, _dxf_ExGFuncPathToString(n));
			ExDebug("*1", "  _execGnode 001: at input#:%d of %d with index#:%d",
				i, nin, index);
			_dxf_ExDie("Executive Inconsistency: _execGnode 001");
		    }
		    else
		    {
			if (gv->obj == NULL && pv->dflt != NULL)
			    _dxf_ExDefineGvar(gv, pv->dflt);
			args[i] = gv->obj;

#if CHECK_EXEC_OBJS
			_dxf_EXOCheck((EXO_Object) gv);
#endif
			ExDebug("*1", "    [%2d] type = %2d %d::%s",
				i, gv->type, p->graphId, _dxf_ExGFuncPathToString(n));
		    }
		}
	    }
	}
	ExDebug("*1", "Total inputs = %d. Nbr in error = %d, Nbr NULL = %d",
		n->nin, nbr_inputs_inerror, nbr_inputs_null);
	if (nbr_inputs_inerror + nbr_inputs_routeoff + nbr_inputs_null 
              == n->nin)
	{
            if(nbr_inputs_inerror > 0) 
            {
	        ExDebug("*1", "No valid inputs: Skipping [%08x] %d::%s",
		    n, p->graphId, _dxf_ExGFuncPathToString(n));
	        DXWarning("#4690", _dxf_ExGFuncPathToString(n));
                DXLoopDone(1);
            
	        for (i = 0, ilimit = SIZE_LIST(n->outputs); i < ilimit; ++i)
	        {
		    ip = FETCH_LIST(n->outputs, i);
		    if (ip && *ip >= 0)
		    {
		        pv = FETCH_LIST(p->vars, *ip);
		        if (pv == NULL)
		  	    _dxf_ExDie("Executive Inconsistency: _execGnode 004");
		        pv->gv->skip = GV_SKIPERROR;
		        _dxf_ExDefineGvar(pv->gv, NULL);
		    }
	        }
	        goto error_continue;
            } 
            if(nbr_inputs_routeoff > 0) 
            {
                DXDebug("*0", "Skipping [%08x] %d::%s", n, p->graphId, 
			_dxf_ExGFuncPathToString(n));
	        ExDebug("*1", "inputs all routed off: Skipping [%08x] %d::%s",
		    n, p->graphId, _dxf_ExGFuncPathToString(n));
            
	        for (i = 0, ilimit = SIZE_LIST(n->outputs); i < ilimit; ++i)
	        {
		    ip = FETCH_LIST(n->outputs, i);
		    if (ip && *ip >= 0)
		    {
		        pv = FETCH_LIST(p->vars, *ip);
		        if (pv == NULL)
		  	    _dxf_ExDie("Executive Inconsistency: _execGnode 004");
		        pv->gv->skip = GV_SKIPROUTE;
		        _dxf_ExDefineGvar(pv->gv, NULL);
		    }
	        }
	        goto error_continue;
            } 
	}
        /* Make sure the error flag get passed through to the outputs
         * of MacroEnd.
         */
        if(nbr_inputs_inerror > 0 && (n->flags & MODULE_ERR_CONT) &&
            strcmp(n->name, "MacroEnd") == 0) {
	    for (i = 0, ilimit = SIZE_LIST(n->outputs); i < ilimit; ++i)
	    {
                ProgramVariable *inpv;

	        index = *FETCH_LIST(n->inputs, i+1);
	        if (index == -1)
                    continue;
		inpv = FETCH_LIST(p->vars, index);
		ip = FETCH_LIST(n->outputs, i);
		if (ip && *ip >= 0)
		{
		    pv = FETCH_LIST(p->vars, *ip);
		    if (pv == NULL)
			_dxf_ExDie("Executive Inconsistency: _execGnode 004");
                    if(inpv->gv->skip == GV_SKIPERROR)
		        pv->gv->skip = GV_SKIPERROR;
		}
	    }
        }

        /* DPSEND modules have a null func ptr, no need to send a message
         * to UI for DPSEND modules.
         */
      
        if (n->ftype == F_MODULE && n->func.module != NULL)
            ModBeginMsg(p, n, bell, &len);

	/* plan/execute the module */

	DXResetError();

	buf[0] = '>';
	buf[1] = ' ';
	strncpy(buf + 2, n->name, 30);
	buf[32] = '\0';
	if (_dxd_exMarkMask & 0x20)
	    DXMarkTimeX(buf);
	else
	    DXMarkTime(buf);

        if(n->ftype == F_MODULE) {
	    _dxfCopyContext(&savedContext, _dxd_exContext);
	    _dxd_exCurrentFunc = n;
        }

	runStatus = get_status();
	set_status(PS_RUN);
	n->starttime = ExTime();

	/*
	 * See if module is a "DPSEND" module generated for distributed 
	 * processing.  If it is, call a ProcessDPSENDFunction. That function 
	 * determines if the target module's processgroup is running on the 
	 * same processor as the input, in which case the DPSEND is ignored.
	 * Otherwise, a message is sent to the target processor. In both 
	 * cases, processing continues with the next module in the gfunc list.
	 */
	if (n->ftype == F_MODULE && n->func.module == NULL &&          
            strcmp(n->name, "DPSEND") == 0)
	{
	    if (_dxd_exEnableDebug)
		printf("Starting Module  %s\n", _dxf_ExGFuncPathToString(n));
	    status = OK;
	    dpSendOtherProc = ExProcessDPSENDFunction(p, n, &args[nin + 1]);
	    goto after_module_scheduling;
	}

	if ((!_dxd_exSkipExecution ||
	     (strcmp(n->name, "Trace") == 0) ||
	     (strcmp(n->name, "Usage") == 0)))
	{
            if(n->ftype == F_MACRO) {
                /* save cache tags for outputs */
                for (i = 0, ilimit = SIZE_LIST(n->outputs); i < ilimit; ++i)
                {
                    ip = FETCH_LIST(n->outputs, i);
                    if (ip && *ip >= 0)
                    {
                        pv = FETCH_LIST(p->vars, *ip);
                        APPEND_LIST(uint32, pv->cts, pv->reccrc);
                    }
                }
                n->func.subP->loopctl.first = TRUE;
                n->func.subP->loopctl.counter = 0;
                COPY_LIST(n->func.subP->vars, p->vars);
                n->func.subP->graphId = p->graphId;
                n->func.subP->returnP = p;

                if(SIZE_LIST(n->func.subP->funcs) > 0) {
                    p->saved_gf = n;
                    /* This extra reference is to make sure that the 
                     * graph does not get deleted before returning to
                     * this routine and cleaning up. This will happen
                     * if the subgraph has no modules to run and is
                     * at the top level.
                     */
                    INCR_RUNABLE
                    MARK_1("queue sub graph");
                    _dxf_ExRunOn(1, ExQueueSubGraph, (Pointer)n->func.subP, 0);
                    status = OK;
                }
                else
                    status = ERROR;
                goto error_continue;
            }
            else {
	        if (_dxd_exEnableDebug)
		    printf("Starting Module  %s\n", _dxf_ExGFuncPathToString(n));
	        status = (*n->func.module) (args, &args[nin + 1]);
            }
	}
	else
	{
	    status = OK;
	}

after_module_scheduling:;

	n->endtime = ExTime();

	set_status(runStatus);
        if(n->ftype == F_MODULE) {
	    _dxfCopyContext(_dxd_exContext, &savedContext);
            _dxd_exCurrentFunc = NULL;
        }

	buf[0] = '<';
	if (_dxd_exMarkMask & 0x20)
	    DXMarkTimeX(buf);
	else
	    DXMarkTime(buf);

        /* DPSEND modules have a null func ptr, no need to send this
         * message to UI since DPSEND modules don't turn green. 
         */
        if (n->ftype == F_MODULE && n->func.module != NULL)
            ModEndMsg(p, bell, len);

        /* clear cache tag */
        if(status == ERROR && n->nout == 0 && !(n->flags & MODULE_SIDE_EFFECT))
            _dxf_ExManageCacheTable(&n->mod_path, 0, 0);

	/*
	 * report any error in a meaningful manner.
	 * If no error occurred, cache inputs with the "cache" attr in the
	 * mdf for modules with REROUTABLE inputs
	 */
        if(n->ftype == F_MODULE) {
	    code = DXGetError();
	    if (code != ERROR_NONE || status == ERROR) {
                DXLoopDone(1);
	        ExExecError(p, n, code, status);
            }
	    else
	    {
	        status = OK;
	        if (n->flags & MODULE_REROUTABLE)
		    ExCacheDDIInputs(p, n);
	    }
        }

	MARK_1("completing");

	/*
	 * For each output: if the function resulted in an error, mark our
	 * outputs as coming from an error (warning if the Module returned
	 * any outputs). else if the output was used, define the gvar and put
	 * it in the cache. else the output wasn't used, delete it. 
	 */
	if (!dpSendOtherProc) for (i = 0; i < n->nout; ++i)
	{
	    ip = FETCH_LIST(n->outputs, i);
	    if (status == ERROR)
	    {
		if (args[nin + 1 + i])
		    DXWarning("#4700", n->name, i);

		ip = FETCH_LIST(n->outputs, i);
		if (ip && *ip >= 0)
		{
		    pv = FETCH_LIST(p->vars, *ip);
		    if (pv == NULL)
			_dxf_ExDie("Executive Inconsistency: _execGnode 003");
		    pv->gv->skip = GV_SKIPERROR;
		    _dxf_ExDefineGvar(pv->gv, NULL);
		}
	    }
	    else		/* if (status == OK) */
	    {
		Class           class = DXGetObjectClass(args[nin + 1 + i]);
		if (args[nin + 1 + i] != NULL &&
		    ((int) class <= (int) CLASS_MIN ||
		     (int) class >= (int) CLASS_MAX))
		{
		    DXWarning("#4710", n->name, i);
		    args[nin + 1 + i] = NULL;
		}
		cost = (double) n->endtime - n->starttime;
		if (ip && *ip >= 0)
		{
		    pv = FETCH_LIST(p->vars, *ip);
		    if (pv == NULL)
			_dxf_ExDie("Executive Inconsistency: _execGnode 002");

		    gv = pv->gv;
		    gv->disable_cache = skipping;

		    _dxf_ExDefineGvar(gv, args[nin + 1 + i]);
		    _dxf_ExSetGVarCost(gv, cost);
		    DXSetObjectTag(gv->obj, gv->reccrc);
		    if (_dxd_exCacheOn && !skipping)
		    {
			if (pv->excache != CACHE_OFF && pv->cacheattr)
			    ExCacheNewGvar(gv);
			else if (n->excache != CACHE_OFF &&
				(pv->excache != CACHE_OFF || !pv->cacheattr))
			    ExCacheNewGvar(gv);
		    }
		}
		else
		{
		    DXReference(args[nin + 1 + i]);
		    DXDelete(args[nin + 1 + i]);
		}
	    }
	}

error_continue:;
	if (argsalloc > LOCAL_ARG_MAX)
	{
	    DXFree((Pointer) args);
	    args = NULL;
	    argsalloc = 0;
	}
    }
module_cleanup:
    ExDebug("*1", "At module_cleanup for %s ", _dxf_ExGFuncPathToString(n));

    n->required = ALREADY_RUN;
    if((n->ftype == F_MACRO) && status == OK) { 
        DECR_RUNABLE
        /* this is necessary when you have a main program with no */
        /* executable modules in it. */
        if(p->runable == 0 && p->deleted)
            ExStartModules(p);
        return OK;
    }
    /*
     * Delete inputs and outputs (outputs first), just in case someone passed
     * an input through unchanged.  If the output was generated in error, don't
     * delete it (to be sure it sticks around for subsequent modules).
     */
    for (i = 0; i < n->nout; i++)
    {
	index = *FETCH_LIST(n->outputs, i);
	if (index > -1)
	{
	    pv = FETCH_LIST(p->vars, index);
	    if (!skipping && !pv->gv->skip && !pv->gv->disable_cache)
	    {
		ExDelete(pv->gv);
		if (--pv->refs == 0) {
		    pv->gv = NULL;
                    ExDebug("*6","deleting %s output %d %d\n",
		             _dxf_ExGFuncPathToString(n),i,index);
                }
	    }
            if(!(n->flags & MODULE_PASSBACK))
	        pv->required = NOT_REQUIRED;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for deleted output pvar\n");
#endif
	}
    }
    for (i = 0; i < n->nin; i++)
    {
	index = *FETCH_LIST(n->inputs, i);
	if (index > -1)
	{
	    pv = FETCH_LIST(p->vars, index);
	    ExDelete(pv->gv);
	    if (--pv->refs == 0) {
		pv->gv = NULL;
                ExDebug("*6","deleting %s input %d %d\n",
		                   _dxf_ExGFuncPathToString(n),i,index);
            }
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for deleted input pvar\n");
#endif
	}
    }
    
#ifdef CPV_DEBUG
if (p->runable <= 0)
    printf("weird for p->runable\n");
#endif
    DECR_RUNABLE
#if 0
    if(p->runable == 0 && p->returnP && p->loopctl.done)
        ExCacheMacroResults(p->returnP);
#endif

    /* Kick off the next module(s) in the list, and clean up. */
    /* If we are skipping over a macro make sure we continue  */
    /* with next module in this program.                      */
    if(n->ftype == F_MODULE || (n->ftype == F_MACRO && status == ERROR))
        ExStartModules(p);

    MARK_1("done");
    return (OK);
}


static void
ExExecError(Program *p, gfunc *n, ErrorCode code, Error status)
{
    char            buf[2048];

    if (status != ERROR)
	DXWarning("#4720", p->graphId, _dxf_ExGFuncPathToString(n));

    sprintf(buf, "%d::%s", p->graphId, _dxf_ExGFuncPathToString(n));
    DXPrintError(buf);

    if (_dxf_ExVCRRunning())
	_dxf_ExVCRCommand(VCR_C_PAUSE, 0, 0);

    p->error = TRUE;
}

/*
 * get a new, unique graph identifier 
 */
int 
_dxf_NextGraphId()
{
    int             gid;

    gid = ++(graph_id);

    /* avoid wrapping around to 0 */
    if (gid == 0)
	gid = ++(graph_id);

    return (gid);
}

static void
ExFixConstantSwitch(Program *p)
{
    int i, j, m, ilimit;
    gfunc *gf;
    ProgramVariable *pv;

    /* Fix constant Switches */
    for (i = 0, ilimit = SIZE_LIST(p->funcs); i < ilimit; ++i)
    {
	gf = FETCH_LIST(p->funcs, i);
        if (IsSwitch(gf))
	{
            /* this will make computed switches have to run each time */
            /* through a loop, I think this is OK because they are quick */
            /* to calculate. The problem is to determine if the switch */
            /* value changed since the last iteration. Maybe I can be */
            /* smarter about this */
            if(!IsFastSwitch(gf)) {
                m = *FETCH_LIST(gf->outputs, 0);
                pv = FETCH_LIST(p->vars, m);
                if(pv->gv)
                    _dxf_ExUndefineGvar(pv->gv);
            }
            /* if this is not the first time evaluating the switch */
            /* and it wasn't a FastSwitch the first time, then it  */
            /* won't be a fast switch this time.                   */
            if(gf->required != NOT_SET && !IsFastSwitch(gf))
                continue;
            gf->flowtype &= ~FastSwitchFlag;
	    m = *FETCH_LIST(gf->inputs, 0);
	    if (m >= 0)
	    {
		pv = FETCH_LIST(p->vars, m);
		if (pv->gv != NULL && pv->gv->type != GV_UNRESOLVED && !pv->gv->skip)
		{
		    int sw, len;
		    ProgramVariable *switchOvar;
                    char bell[512];
                    ModBeginMsg(p, gf, bell, &len);
                    gf->flowtype |= FastSwitchFlag;
		    if (pv->gv->obj == NULL)
		    {
		        if (pv->dflt)
			{
			    if (DXExtractInteger(pv->dflt, &sw) == NULL)
				sw = 0;
			}
			else
			    sw = 0;
		    }
		    else if (DXExtractInteger(pv->gv->obj, &sw) == NULL)
		    {
			sw = 0;
			/* make this go through computed switch code to */
                        /* produce an error. */
                        gf->flowtype &= ~FastSwitchFlag;
                        continue;
		    }

		    if (sw > 0 && sw < gf->nin)
		    {
			ProgramVariable *switchIvar;
			/* hook the output to the input */
			m = *FETCH_LIST(gf->outputs, 0);
			switchOvar = FETCH_LIST(p->vars, m);
			m = *FETCH_LIST(gf->inputs, sw);
			if (m < 0)
			{
			    _dxf_ExDefineGvar(switchOvar->gv, NULL);
			    ExReference(switchOvar->gv);
			    switchOvar->refs++;
			}
			else
			{
			    switchIvar = FETCH_LIST(p->vars, m);
			    for (j = 0; j < switchOvar->refs; ++j)
			    {
				ExReference(switchIvar->gv);
				ExDelete(switchOvar->gv);
			    }
			    ExReference(switchIvar->gv);
			    ExReference(switchIvar->gv);
			    switchOvar->gv = switchIvar->gv;
			    switchIvar->refs++;
			    switchOvar->refs++;
			}
		    }
		    else if (sw <= 0 || sw >= gf->nin)
		    {
			/* define the output to be NULL.  */
			m = *FETCH_LIST(gf->outputs, 0);
			switchOvar = FETCH_LIST(p->vars, m);
			_dxf_ExDefineGvar(switchOvar->gv, NULL);
		    }
                    ModEndMsg(p, bell, len);
		}
	    }
	}
    }
    MARK_1("fix switches");
}

/*
 * This routine determines if gf needs to be rerun, or is OK as is.
 * If this routine returns TRUE, then it marked another set of functions
 * as "required", and the graph has to be reevaluated.  If it returns
 * FALSE, this gfunc is done.
 */
static int 
ExFixSwitchSelector(Program *p, gfunc *gf, int executionTime, int requiredFlag)
{
    ProgramVariable *pv;
    int             switchOut;
    int             switchIn;
    int             sw;
    int             ret = FALSE;
    ProgramVariable *switchIvar;
    ProgramVariable *switchOvar;
    int 	    i;
    int		    requiredWas;

    /*
     * If this is not a computed switch,bind the correct input to its
     * output. If this is a computed switch, mark the 0th input to
     * the switch module as being required. Dependent modules will
     * be flagged at execution time (in execGnode) once the Switch
     * input is resolved.
     */

    switchOut = *FETCH_LIST(gf->outputs, 0);
    switchOvar = FETCH_LIST(p->vars, switchOut);
    switchIn = *FETCH_LIST(gf->inputs, 0);
    pv = FETCH_LIST(p->vars, switchIn);

    if (pv->gv == NULL)
	_dxf_ExDie("NULL gvar in ExFixSwitchSelector");

    /*
     * if input selector (pv) is skipped, then set the skip
     * indicator in the output object as well 
     */
    if (pv->gv->skip && switchOut >= 0)
    {
        DXDebug("*0", "Skipping [%08x] %d::%s", gf, p->graphId, 
	                _dxf_ExGFuncPathToString(gf));
	switchOvar->gv->skip = pv->gv->skip;
	if (pv->gv->disable_cache)
	    switchOvar->gv->disable_cache = TRUE;
	_dxf_ExDefineGvar(switchOvar->gv, (Object) NULL);
	return(FALSE);
    }

    /*
     * get the selector object.  this if condition checks for all valid cases.
     */
    if (pv->gv->obj == NULL || DXExtractInteger(pv->gv->obj, &sw) != NULL)
    {
	if (pv->gv->obj == NULL)
	{
	    if (pv->dflt == NULL ||
		DXExtractInteger(pv->dflt, &sw) == NULL)
	    {
		sw = 0;
	    }
	}
	if (sw <= 0 || sw >= gf->nin ||
	    (switchIn = *FETCH_LIST(gf->inputs, sw)) < 0)
	{
	    /* define the output as NULL, carrying on disable_cache value */
	    ExDebug("*1", "Setting output to NULL in ExFixSwitchSelector");

	    if (pv->gv->disable_cache)
		switchOvar->gv->disable_cache = TRUE;

	    _dxf_ExDefineGvar(switchOvar->gv, (Object) NULL);
	    ret = FALSE;
	}
	else
	{
	    int fnbr = gf - p->funcs.vals;
	    ExDebug("*1",
		"Setting output to input in ExFixSwitchSelector for %s",
		_dxf_ExGFuncPathToString(gf));
	    switchIvar = FETCH_LIST(p->vars, switchIn);
	    requiredWas = switchIvar->required;
	    if (switchIvar->gv != NULL &&
		switchIvar->gv->type != GV_UNRESOLVED)
	    {
		ret = FALSE;	/* fixed the switch since a constant */
		if (pv->gv->disable_cache || switchIvar->gv->disable_cache)
		    switchOvar->gv->disable_cache = TRUE;
		if (switchIvar->gv->skip)
		    switchOvar->gv->skip = switchIvar->gv->skip;
		_dxf_ExDefineGvar(switchOvar->gv, switchIvar->gv->obj);

                /* Mark switchIvar as potentially required */
		switchIvar->required = REQD_IFNOT_CACHED;
		for (i = fnbr - 1; i >= 0; --i)
		    if (ExProcessGraphFunc(p, i, executionTime) &&
			   ((p->first_skipped == -1 || p->first_skipped >= i)))
			p->first_skipped = i;
	    }
	    else
	    {
		ret = TRUE;	/* marked stuff required, rerun */
		/* Mark switchIvar as required */
		switchIvar->required = requiredFlag;
		for (i = fnbr - 1; i >= 0; --i)
		    ExProcessGraphFunc(p, i, executionTime);
	    }
#ifdef CPV_DEBUG
	    {
		gfunc *gf = FETCH_LIST(_dxd_exContext->program->funcs,fnbr);
		int i;
		for (i = 0; i < SIZE_LIST(gf->inputs); ++i)
		    if (*FETCH_LIST(gf->inputs, i) == switchIn)
			printf("Send mark of input %d of %s required "
			    "(0 based)\n",
			    i, _dxf_ExGFuncPathToString(gf));
	    }
#endif
	    if (requiredWas != switchIvar->required &&
		    ExRunOnThisProcessor(gf->procgroupid))
		_dxf_ExSendPVRequired(p->graphId, p->subgraphId,
		                      fnbr, switchIn, switchIvar->required);
	}
    }
    else
    {
	/*
	 * The only way we get to this code is if DXExtractInteger fails.
	 */
	ExDebug("*1", "Switch selector is not in integer !!!");
	ExDebug("*1", "Setting output to NULL in ExFixSwitchSelector "
		      "because of error");

	ret = FALSE;	/* ret OK since setting error... */
	_dxf_ExDefineGvar(switchOvar->gv, (Object) NULL);
	switchOvar->gv->skip = GV_SKIPERROR;
        DXSetError(ERROR_BAD_PARAMETER, "#10010", "switch value");
	ExExecError(p, gf, ERROR_BAD_PARAMETER, ERROR);
    }
    return (ret);
}
/*
 * This routine determines if gf needs to be rerun, or is OK as is.
 * If this routine returns TRUE, then it marked another set of functions
 * as "required", and the graph has to be reevaluated.  If it returns
 * FALSE, this gfunc is done.
 */
static int 
ExFixRouteSelector(Program * p, gfunc * gf, int executionTime, int requiredFlag)
{
    ProgramVariable *pv;
    int             switchOut;
    int             switchIn;
    int             single_route;
    int             ret = FALSE;
    ProgramVariable *switchIvar;
    ProgramVariable *switchOvar;
    int 	    i, ilimit;
    Object 	    route_obj = NULL;
    int             num_routeout;
    int             *route_out = NULL;
    int             output_kill[MAX_ROUTE_OUTPUTS];
    int             fnbr; 
    int             mark_required = FALSE;
   
    /*
     * If this is not a computed route, bind the correct input to its
     * output. If this is a computed switch, mark the 0th input to
     * the switch module as being required. Dependent modules will
     * be flagged at execution time (in execGnode) once the Route
     * input is resolved.
     */

    switchIn = *FETCH_LIST(gf->inputs, 0);
    pv = FETCH_LIST(p->vars, switchIn);

    if (pv->gv == NULL)
	_dxf_ExDie("NULL gvar in ExFixRouteSelector");

    /*
     * if input selector (pv) is skipped, then set the skip
     * indicator in the output object as well 
     */
    if (pv->gv->skip)
    {
        DXDebug("*0", "Skipping [%08x] %d::%s", gf, p->graphId, 
	                     _dxf_ExGFuncPathToString(gf));
	for (i = 0, ilimit = SIZE_LIST(gf->outputs); i < ilimit; ++i)
	{
	    switchOut = *FETCH_LIST(gf->outputs, i);
	    if (switchOut >= 0)
	    {
		switchOvar = FETCH_LIST(p->vars, switchOut);
		switchOvar->gv->skip = pv->gv->skip;
		if (pv->gv->disable_cache)
		    switchOvar->gv->disable_cache = TRUE;
		_dxf_ExDefineGvar(switchOvar->gv, (Object) NULL);
	    }
	}
	return(FALSE);
    }

    for(i = 0; i < MAX_ROUTE_OUTPUTS; i++)
        output_kill[i] = TRUE;

    if (pv->gv->obj == NULL)
    {
        if (pv->dflt == NULL) 
            route_obj = NULL;
        else 
            route_obj = pv->dflt;
    }
    else route_obj = pv->gv->obj;


    if (route_obj) {
        if (!DXQueryParameter(route_obj, TYPE_INT, 1, &num_routeout)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10050", "Route selector");
        }

        if (num_routeout <= 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#10050", "Route selector");
        }

        if(num_routeout == 1)
            route_out = &single_route;
        else
            route_out = (int *)DXAllocate(sizeof(int) * num_routeout);
        if(route_out == NULL)
            DXSetError(ERROR_NO_MEMORY, "#8360");
        else if (!DXExtractParameter(route_obj, TYPE_INT, 1,
                              num_routeout, (Pointer)route_out)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10050", "route selector");
            DXFree(route_out);
            route_out = NULL;
        }
    }
    else {
        num_routeout = 1;
        route_out = &single_route;
        *route_out = 0;  /* all outputs will be shut off */
    }

    if (route_out == NULL)
    {
	ExDebug("*1", "Setting output to NULL in ExFixRouteSelector "
		      "because of error");

	for (i = 0, ilimit = SIZE_LIST(gf->outputs); i < ilimit; ++i)
	{
	    switchOut = *FETCH_LIST(gf->outputs, i);
	    if (switchOut >= 0)
	    {
		switchOvar = FETCH_LIST(p->vars, switchOut);
		switchOvar->gv->skip = GV_SKIPERROR;
		if (pv->gv->disable_cache)
		    switchOvar->gv->disable_cache = TRUE;
		_dxf_ExDefineGvar(switchOvar->gv, (Object) NULL);
	    }
	}
	ExExecError(p, gf, DXGetError(), ERROR);
        return(FALSE);
    }

    if ((switchIn = *FETCH_LIST(gf->inputs, 1)) >= 0 ) {
        switchIvar = FETCH_LIST(p->vars, switchIn);
        for(i = 0; i < num_routeout; i++) {
            if(route_out[i] > 0 && route_out[i] <= gf->nout) 
                output_kill[(route_out[i] - 1)] = FALSE; 
        }
    }
    else 
        switchIvar = NULL;

    fnbr = gf - p->funcs.vals;

    if (route_out[0] != 0 && (switchIvar->gv == NULL || 
                          switchIvar->gv->type == GV_UNRESOLVED)) {
        ExDebug("*1", "In ExFixRouteSelector for %s, mark input 1 required",
		    _dxf_ExGFuncPathToString(gf));
	/* Mark switchIvar as required */
	switchIvar->required = requiredFlag;
	for (i = fnbr - 1; i >= 0; --i)
	    ExProcessGraphFunc(p, i, executionTime);
	if (ExRunOnThisProcessor(gf->procgroupid))
	    _dxf_ExSendPVRequired(p->graphId, p->subgraphId, fnbr, switchIn,
                                  switchIvar->required);
        ret = TRUE;
        goto done;
    }

    for (i = 0, ilimit = SIZE_LIST(gf->outputs); i < ilimit; ++i) {
	switchOut = *FETCH_LIST(gf->outputs, i);
        if(switchOut < 0)
            _dxf_ExDie("Route error: Can't get outputs for Route");

        switchOvar = FETCH_LIST(p->vars, switchOut);
        if(output_kill[i]) {
            if (pv->gv->disable_cache || 
               (switchIvar && switchIvar->gv->disable_cache))
                switchOvar->gv->disable_cache = TRUE;
            if (switchIvar && switchIvar->gv->skip)
	        switchOvar->gv->skip = switchIvar->gv->skip;
            else 
                switchOvar->gv->skip = GV_SKIPROUTE;
            _dxf_ExDefineGvar(switchOvar->gv, (Object) NULL);
        }
        else {
            ExDebug("*1", "Setting output %d to input in ExFixRouteSelector "
		    "for %s", i+1, _dxf_ExGFuncPathToString(gf));
	    if (pv->gv->disable_cache || switchIvar->gv->disable_cache)
	        switchOvar->gv->disable_cache = TRUE;
            else
	        switchOvar->gv->disable_cache = FALSE;
            switchOvar->gv->skip = switchIvar->gv->skip;
            _dxf_ExDefineGvar(switchOvar->gv, switchIvar->gv->obj);
            mark_required = TRUE;
	}
    }

    if(mark_required) {
        /* Mark switchIvar as potentially required */
        switchIvar->required = REQD_IFNOT_CACHED;
        for (i = fnbr - 1; i >= 0; --i)
    	    if (ExProcessGraphFunc(p, i, executionTime) &&
	       ((p->first_skipped == -1 || p->first_skipped >= i)))
	        p->first_skipped = i;
        if (ExRunOnThisProcessor(gf->procgroupid))
	    _dxf_ExSendPVRequired(p->graphId, p->subgraphId, fnbr, switchIn, 
                                  switchIvar->required);
    }
    ret = FALSE;
 
done:
    if(num_routeout > 1)
        DXFree(route_out);
    return(ret);
}

static int
OKtoCache(ProgramVariable *pv, gfunc *gf)
{
    if (pv->excache == CACHE_OFF)
        return(FALSE);
    if (gf->excache == CACHE_OFF && !pv->cacheattr)
        return(FALSE);
    return(TRUE);
}

/*
 * Search for all functs: If "SIDE_EFFECT" mark them as "required".
 * Mark their inputs as "required" if they aren't in the cache.
 * For all other modules except switches, cycle thru the outputs and 
 * if they are required, mark their inputs as "required" if they aren't 
 * in the cache.
 */
static int 
ExProcessGraphFunc(Program *p, int fnbr, int executionTime)
{
    int             index, i, j;
    int             ilimit, jlimit;
    gfunc          *gf;
    gvar           *g;
    gvar           *cachedGvar;
    int		    requiredFlag;
    int		    returnValue;

    gf = FETCH_LIST(p->funcs, fnbr);
    if (!executionTime && ((gf->flags & MODULE_SIDE_EFFECT) ||
                          (gf->nout == 0 && gf->tag_changed) ||
                          (gf->flags & MODULE_LOOP)))
    {
	if (gf->required != REQUIRED && ExRunOnThisProcessor(gf->procgroupid))
            INCR_RUNABLE
	gf->required = REQUIRED;
	ExDebug("*1", "Marking func %s (# %d) as required ",
	    _dxf_ExGFuncPathToString(gf), fnbr);
	for (j = 0, jlimit = SIZE_LIST(gf->inputs); j < jlimit; ++j)
	{
	    index = *FETCH_LIST(gf->inputs, j);
	    if (index != -1)
	    {
		ProgramVariable *pv = FETCH_LIST(p->vars, index);
                if((gf->flags & MODULE_PASSBACK) && j != 0) {
                    int out = *FETCH_LIST(gf->outputs, j-1);
                    ProgramVariable *pv_out = FETCH_LIST(p->vars, out);
                    if(pv_out->required == NOT_REQUIRED) {
                        pv->required = NOT_REQUIRED;
                        /* we have to mark the gv as skip otherwise the 
                         * module can't run at all. This works because it is 
                         * MODULE_ERR_CONT. */
                        pv->gv->skip = GV_SKIPMACROEND;
                        continue;
                    }
                }
		g = pv->gv;
		if (pv->refs == 0 || !g)
		{
		    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
		    ExReference(pv->gv);
		    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
printf("weird for new in pvar\n");
#endif
		    pv->gv->reccrc = pv->reccrc;
		}
		else 
                {
                    if (g && g->type == GV_UNRESOLVED && _dxd_exCacheOn &&
                           OKtoCache(pv, gf) &&
	           	  (cachedGvar = _dxf_ExCacheSearch(pv->gv->reccrc)) != NULL)
		    {
		        _dxf_ExDefineGvar(pv->gv, cachedGvar->obj);
		        ExDelete(cachedGvar);
		        pv->required = REQD_IFNOT_CACHED;
		    }
		    else
		    {
		        pv->required = REQUIRED;
		        ExDebug("*1", "Marking func %s input %d as required ",
			    _dxf_ExGFuncPathToString(gf), j);
		    }
               }
	    }
	}
	return TRUE;
    }				/* end of side-effect */

    if (gf->required)
    {
        ExDebug("*1", "Func %s (# %d) already marked as %d",
	        _dxf_ExGFuncPathToString(gf), fnbr, gf->required);
        if (gf->required == REQUIRED)
            return TRUE;
        else if (gf->required == RUNNING)
            return FALSE;
    }

    /* Check to see if any outputs are required */
    requiredFlag = NOT_REQUIRED;
    returnValue = FALSE;
    for (j = 0, jlimit = SIZE_LIST(gf->outputs);
	 j < jlimit && requiredFlag != REQUIRED;
	 ++j)
    {
	index = *FETCH_LIST(gf->outputs, j);
	if (index != -1)
	{
	    ProgramVariable *pv = FETCH_LIST(p->vars, index);
	    if (pv->required)
	    {
                if(pv->required == REQUIRED && pv->gv && pv->gv->skip == GV_SKIPMACROEND)
                    pv->gv->skip = GV_DONTSKIP;
		for (i = 0, ilimit = SIZE_LIST(gf->outputs);
			  i < ilimit;
			  ++i)
		{
		    index = *FETCH_LIST(gf->outputs, i);
		    if (index != -1)
		    {
			ProgramVariable *pv = FETCH_LIST(p->vars, index);
			g = pv->gv;
			if (pv->refs == 0 || !g)
			{
			    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
			    ExReference(pv->gv);
			    pv->refs++;
			    pv->gv->reccrc = pv->reccrc;
			}
		    }
		}
		/*
		 * Check to see if this output (perhaps erroneously marked
		 * required by the "Mark outputs" section) is in the cache 
		 */
		if (pv->required == REQUIRED)
		{
		    if (pv->gv && pv->gv->type != GV_UNRESOLVED)
		    {
			pv->required = REQD_IFNOT_CACHED;
			if (requiredFlag != REQUIRED)
			    requiredFlag = REQD_IFNOT_CACHED;
		    }
		    else if (_dxd_exCacheOn && OKtoCache(pv, gf) &&
			(cachedGvar =
			 _dxf_ExCacheSearch(pv->gv->reccrc)) != NULL)
		    {
			pv->required = REQD_IFNOT_CACHED;
			if (requiredFlag != REQUIRED)
			    requiredFlag = REQD_IFNOT_CACHED;
			ExDebug("*1",
			    "  output at index %d already cached for gfunc %s",
			    index, _dxf_ExGFuncPathToString(gf));
			_dxf_ExDefineGvar(pv->gv, cachedGvar->obj);
			ExDelete(cachedGvar);
		    }
		}
		if (gf->flags & MODULE_REACH && gf->required <= NOT_REQUIRED)
		    requiredFlag = REQUIRED;
		else if (requiredFlag != REQUIRED)
		    requiredFlag = pv->required;
	    }
	}
    }

    /* If any outputs were required */
    if (requiredFlag)
    {
	/* Ensure that all inputs have gvars */
	for (i = 0, ilimit = SIZE_LIST(gf->inputs);
		  i < ilimit;
		  ++i)
	{
	    index = *FETCH_LIST(gf->inputs, i);
	    if (index != -1)
	    {
		ProgramVariable *pv = FETCH_LIST(p->vars, index);
		g = pv->gv;
		if (pv->refs == 0 || !g)
		{
		    pv->gv = _dxf_ExCreateGvar(GV_UNRESOLVED);
		    ExReference(pv->gv);
		    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
printf("weird for new in pvar\n");
#endif
		    pv->gv->reccrc = pv->reccrc;
		}
	    }
	}

	/* Patch fast switches */
	if (IsFastSwitch(gf))
	{
	    /* We know the next two conditionals will be true, but check
	     * just the same */
	    index = *FETCH_LIST(gf->inputs, 0);
	    if (index != -1)
	    {
		ProgramVariable *pv = FETCH_LIST(p->vars, index);
		if (pv->gv && pv->gv->type != GV_UNRESOLVED)
		{
		    int sw;
                    Object switch_obj;
 
                    if(pv->gv->obj == NULL) 
                        switch_obj = pv->dflt;
                    else switch_obj = pv->gv->obj;
		    if (DXExtractInteger(switch_obj, &sw) &&
			sw > 0 && sw < gf->nin)
		    {
			index = *FETCH_LIST(gf->inputs, sw);
			if (index >= 0)
			{
			    pv = FETCH_LIST(p->vars, index);
			    if (pv->required != REQUIRED)
				pv->required = requiredFlag;
			}
			return FALSE;
		    }
		}
	    }
	}

	/* If this module is really going to run here, update the program */
	if (requiredFlag == REQUIRED && gf->required != REQUIRED &&
		ExRunOnThisProcessor(gf->procgroupid))
            INCR_RUNABLE
	gf->required = requiredFlag;
	if (requiredFlag == REQUIRED)
	{
	    returnValue = TRUE;
	    ExDebug("*1", "Marking func %s (# %d) as REQUIRED",
		_dxf_ExGFuncPathToString(gf), fnbr);
	}
	else
	    ExDebug("*1", "Marking func %s (# %d) as REQD_IFNOT_CACHED",
		_dxf_ExGFuncPathToString(gf), fnbr);

	if (IsSwitch(gf) || IsRoute(gf))
	{
	    /* if first input is unresolved, mark it as required,
	     * else fix the switch
	     */
	    index = *FETCH_LIST(gf->inputs, 0);
	    if (index != -1)
	    {
		ProgramVariable *pv = FETCH_LIST(p->vars, index);
		g = pv->gv;
		if (requiredFlag == REQUIRED)
		{
		    if (g->type != GV_UNRESOLVED)
		    {
			if (pv->required != REQUIRED)
			    pv->required = REQD_IFNOT_CACHED;

			if (IsSwitch(gf) && 
			    !ExFixSwitchSelector(p, gf, executionTime,
				requiredFlag))
			{
			    gf->required = ALREADY_RUN;
			    if (ExRunOnThisProcessor(gf->procgroupid)) 
                                DECR_RUNABLE
			}
			else if (IsRoute(gf) &&
			    !ExFixRouteSelector(p, gf, executionTime,
				requiredFlag))
			{
			    gf->required = ALREADY_RUN;
			    if (ExRunOnThisProcessor(gf->procgroupid)) 
				DECR_RUNABLE
			}
		    }
		    else if (_dxd_exCacheOn && OKtoCache(pv, gf) &&
			(cachedGvar =
			 _dxf_ExCacheSearch(pv->gv->reccrc)) != NULL)
		    {
			_dxf_ExDefineGvar(pv->gv, cachedGvar->obj);
			ExDelete(cachedGvar);

			if (pv->required != REQUIRED)
			    pv->required = REQD_IFNOT_CACHED;

			if (IsSwitch(gf) && 
			    !ExFixSwitchSelector(p, gf, executionTime,
				requiredFlag))
			{
			    gf->required = ALREADY_RUN;
			    if (ExRunOnThisProcessor(gf->procgroupid)) 
				DECR_RUNABLE
			}
			else if (IsRoute(gf) &&
			    !ExFixRouteSelector(p, gf, executionTime,
				requiredFlag))
			{
			    gf->required = ALREADY_RUN;
			    if (ExRunOnThisProcessor(gf->procgroupid)) 
				DECR_RUNABLE
			}
		    }
		    else
		    {
			if (pv->required != REQUIRED)
			    pv->required = requiredFlag;
		    }
		}
		else
		{
		    if (pv->required != REQUIRED)
			pv->required = requiredFlag;
		}
	    }
	}
	else
	{
	    for (i = 0, ilimit = SIZE_LIST(gf->inputs);
		      i < ilimit;
		      ++i)
	    {
		index = *FETCH_LIST(gf->inputs, i);
		if (index != -1)
		{
		    ProgramVariable *pv = FETCH_LIST(p->vars, index);
                    if((gf->flags & MODULE_PASSBACK) && j != 0) {
                        int out = *FETCH_LIST(gf->outputs, j-1);
                        ProgramVariable *pv_out = FETCH_LIST(p->vars, out);
                        if(pv_out->required == NOT_REQUIRED) {
                            pv->required = NOT_REQUIRED;
                            /* we have to mark the gv as skip otherwise the 
                             * module can't run at all. This works because 
                             * it is MODULE_ERR_CONT. */
                            pv->gv->skip = GV_SKIPMACROEND;
                            continue;
                        }
                    }
		    g = pv->gv;
		    if (requiredFlag == REQUIRED)
		    {
			if (g->type != GV_UNRESOLVED &&
				pv->required != REQUIRED)
			    pv->required = REQD_IFNOT_CACHED;
			else if (_dxd_exCacheOn && OKtoCache(pv, gf) &&
			    (cachedGvar =
			     _dxf_ExCacheSearch(pv->gv->reccrc)) != NULL)
			{
			    _dxf_ExDefineGvar(pv->gv, cachedGvar->obj);
			    ExDelete(cachedGvar);

			    if (pv->required != REQUIRED)
				pv->required = REQD_IFNOT_CACHED;
			}
			else
			{
			    if (pv->required != REQUIRED)
				pv->required = requiredFlag;
			}
		    }
		    else
		    {
			if (pv->required != REQUIRED)
			    pv->required = requiredFlag;
		    }
		}
	    }		/* end of i (input) loop   */
	}
    }			/* end of output req'd cond   */
    else {
        /* I believe this is to fix a problem with switches getting */
        /* defined as a constant switch the second time through a program */
        if(gf->required == NOT_SET) 
            gf->required = NOT_REQUIRED;
    }

    return returnValue;
}
/*----------------------------------------------------------------------*/
/* For each required function in current program/graph, reference req'd */
/* inputs and outputs.                                                  */
/*----------------------------------------------------------------------*/
static void 
ExRefFuncIO(Program *p, int fnbr)
{
    int             j, jlimit;
    ProgramVariable *pv;
    gfunc          *gf;
    int            *var;

    gf = FETCH_LIST(p->funcs, fnbr);
    if (gf == NULL)
	_dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 005");
    if (gf->required == REQUIRED)
    {
	for (j = 0, jlimit = SIZE_LIST(gf->inputs); j < jlimit; ++j)
	{
	    var = FETCH_LIST(gf->inputs, j);
	    if (var == NULL)
		_dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 006");
	    if (*var != -1)
	    {
		pv = FETCH_LIST(p->vars, *var);
		if (pv == NULL)
		    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 007");
		if (pv->gv != NULL)
		{
		    ExReference(pv->gv);
		    pv->refs++;
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for input pvar\n");
#endif
		}
	    }
	}
	for (j = 0, jlimit = SIZE_LIST(gf->outputs); j < jlimit; ++j)
	{
	    var = FETCH_LIST(gf->outputs, j);
	    if (var == NULL)
		_dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 006");
	    if (*var != -1)
	    {
		pv = FETCH_LIST(p->vars, *var);
		if (pv == NULL)
		    _dxf_ExDie("Executive Inconsistency: _dxf_ExQueueGraph 008");
		if (pv->gv != NULL)
		{
		    ExReference(pv->gv);
		    pv->refs++;
                    /* Put an extra reference on outputs of side effect
                       and async modules. Now that we can reclaim inter.
                       results a Switch could cause something upstream to
                       be run again because it was run once and then
                       it's output was discarded because at the time it
                       didn't seem like we would need it.
                    */
                    if(p->loopctl.first && (gf->flags &
                       (MODULE_SIDE_EFFECT | MODULE_ASYNC | MODULE_ASYNCLOCAL))) {
		        ExReference(pv->gv);
		        pv->refs++;
                    }
#ifdef CPV_DEBUG
if (pv->refs > pv->gv->object.refs)
    printf("weird for output pvar\n");
#endif
		}
	    }
	}
    }				/* end of gf required */
}
/*-----------------------------------------------------------------*/
/* For each input, get associated mdf input attributes             */
/* If the cache attribute is set to 2 for a given input, check to  */
/* see if it has changed from the previous iteration. If no inputs */
/* with the cache:2 attribute have changed, simply reroute the     */
/* input(s) with the direroute:n attribute set through to the      */
/* output(s) identified by the value of the direroute attribute,   */
/* and mark the module as not needing scheduling (by returning     */
/* (FALSE).                                                        */
/*-----------------------------------------------------------------*/
static int 
ExCheckDDI(Program * p, gfunc * gf)
{
    int             i, ilimit;
    int             *ip, *op, *input_p, *pvar_p;
    ReRouteMap      *rmap;
    ProgramVariable *inpv, *outpv;
    int             input_changed = FALSE;
    Object          temp_obj, cached_obj;
    char            *temp_str;
    int             tmp;
    gvar            *cachedGvar;

    if(SIZE_LIST(gf->cache_ddi) == 0)
	input_changed = TRUE;	/* force re-scheduling if no inputs with
				 * cache attrs found */

    for (i = 0, ilimit = SIZE_LIST(gf->cache_ddi); i < ilimit; ++i) {
        input_p = FETCH_LIST(gf->cache_ddi, i);
	if (!input_p || *input_p < 0)
	    _dxf_ExDie(
               "Executive Inconsistency: _ExCheckDDI - bad index to variable");

        pvar_p = FETCH_LIST(gf->inputs, *input_p);
	if (!pvar_p || *pvar_p < 0)
	    _dxf_ExDie(
               "Executive Inconsistency: _ExCheckDDI bad index to variable");

	inpv = FETCH_LIST(p->vars, *pvar_p);
	if (inpv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _ExCheckDDI 001");

	ExDebug("8",
                "Checking cache in ExCheckDDI for obj with label/key: %s/%d",
	        _dxf_ExGFuncPathToString(gf), *input_p);

        if (inpv->gv && inpv->gv->obj != NULL) {
	    cached_obj = inpv->gv->obj;
	    ExDebug("8", "  obj to be cached is inpv->gv->obj ");
        }
        else {
	    if (inpv->obj != NULL)
	        cached_obj = inpv->obj;
       	    else
	        cached_obj = inpv->dflt;
        }

        temp_obj = DXGetCacheEntry(_dxf_ExGFuncPathToCacheString(gf), *input_p, 0);
        if(!temp_obj) {
	    ExDebug("8", "Input obj with label/key: %s/%d not found in cache",
          	          _dxf_ExGFuncPathToCacheString(gf), *input_p);
	    input_changed = TRUE;
            break;
        }
        else 
            DXDelete(temp_obj);  /* we don't want an extra reference on this */
        if(temp_obj != cached_obj) {
            if(!cached_obj) {
	        temp_str = DXGetString((String) temp_obj);
                if(temp_str && !strcmp(temp_str, "NULL"));
                    continue;
	        ExDebug("8", "Input obj with label/key: %s/%d changed",
		    _dxf_ExGFuncPathToCacheString(gf), *input_p);
            }
	    input_changed = TRUE;
            break;
        }
    }

    /*
     * route input(s) thru appropriate output(s) as indicated by direroute
     * attr.
     * Value is stored in reroute_index
     */
    if (!input_changed) {
	for (i = 0, ilimit = SIZE_LIST(gf->reroute); i < ilimit; ++i) {
            rmap = FETCH_LIST(gf->reroute, i);
            if(rmap == NULL)
                _dxf_ExDie(
                "Executive Inconsistency: _ExCheckDDI - bad reroute information");
	    op = FETCH_LIST(gf->outputs, rmap->output_n);
	    if (op == NULL || *op < 0)
		_dxf_ExDie("Executive Inconsistency: _ExCheckDDI 002");
	    outpv = FETCH_LIST(p->vars, *op);
	    if (outpv == NULL)
		_dxf_ExDie("Executive Inconsistency: _ExCheckDDI 003");
	    ip = FETCH_LIST(gf->inputs, rmap->input_n);
	    if (ip == NULL || *ip < 0)
		_dxf_ExDie("Executive Inconsistency: _ExCheckDDI 004");
     	    inpv = FETCH_LIST(p->vars, *ip);
	    if (inpv == NULL)
	        _dxf_ExDie("Executive Inconsistency: _ExCheckDDI 005");
            /* is this possible ??? */
	    if (inpv->gv && inpv->gv->type == GV_UNRESOLVED) {
	        if(_dxd_exCacheOn && OKtoCache(inpv, gf) &&
		  (cachedGvar = _dxf_ExCacheSearch(inpv->gv->reccrc)) != NULL) {
	            ExDebug("8", "Routing input# %d thru output# %d", 
                             rmap->input_n, rmap->output_n);
		    ExDebug("8", "   (input gv unresolved)");
		    _dxf_ExDefineGvar(outpv->gv, cachedGvar->obj);
		    ExDelete(cachedGvar);
	        }
	        else {
                    /* if we can't reroute output then we need to rerun */
                    /* the module */
                    input_changed = TRUE; 
                }
	    }
	    else {
	        if(rmap->output_n >= 0 && rmap->output_n < gf->nout) {
		    _dxf_ExDefineGvar(outpv->gv, inpv->gv->obj);
		    ExDebug("8", 
                   "Routing input# %d (obj %08x) w/ tag 0x%08x thru output# %d",
			rmap->input_n, outpv->gv->obj, outpv->gv->reccrc, 
                        rmap->output_n);
		    DXExtractInteger(inpv->gv->obj, &tmp);
		    ExDebug("8","Value of object being passed thru is %d", tmp);
  	        }
                else {
                    /* pks 03/01/94 */
                    /* this used to be FALSE but I can't imagine why! */
		    input_changed = TRUE;
                }
	    }
	}
        if(SIZE_LIST(gf->reroute) == 0) {
            /* need to retrieve last outputs from cache */
	    for (i = 0, ilimit = SIZE_LIST(gf->outputs); i < ilimit; ++i) {
                op = FETCH_LIST(gf->outputs, i);
                if (op == NULL || *op < 0)
                    _dxf_ExDie("Executive Inconsistency: _ExCheckDDI 002");
                outpv = FETCH_LIST(p->vars, *op);
                if (outpv == NULL)
                    _dxf_ExDie("Executive Inconsistency: _ExCheckDDI 003");
                if(_dxd_exCacheOn && OKtoCache(outpv, gf) &&
                  (cachedGvar = _dxf_ExCacheSearch(outpv->gv->reccrc))!=NULL) {
                    ExDebug("8", "Routing output# %d from cache", i);
                    _dxf_ExDefineGvar(outpv->gv, cachedGvar->obj);
                    ExDelete(cachedGvar);
                }
                else {
                    /* if we can't reroute output then we need to rerun */
                    /* the module */
                    input_changed = TRUE;
                }
            }
        } /* no reroute list */  
    }
    else { 
        /* input has changed, put an extra reference on rerouted outputs. */
        /* This fixes the problem of a DDI running twice in once execution */
        /* if its output was thrown out and later a switch needed its */
        /* output causing it to reexeute. */
	for (i = 0, ilimit = SIZE_LIST(gf->reroute); i < ilimit; ++i) {
            rmap = FETCH_LIST(gf->reroute, i);
            if(rmap == NULL)
                _dxf_ExDie(
                "Executive Inconsistency: _ExCheckDDI - bad reroute information");
	    op = FETCH_LIST(gf->outputs, rmap->output_n);
	    if (op == NULL || *op < 0)
		_dxf_ExDie("Executive Inconsistency: _ExCheckDDI 002");
	    outpv = FETCH_LIST(p->vars, *op);
	    if (outpv == NULL)
		_dxf_ExDie("Executive Inconsistency: _ExCheckDDI 003");
            ExReference(outpv->gv);
            outpv->refs++;
        }
    }
    return (input_changed);	/* module scheduling required if input
				 * changes exist */
}
static int 
ExCacheDDIInputs(Program *p, gfunc *gf)
{
    int             i, ilimit;
    int            *pvar_p, *input_p;
    ProgramVariable *inpv;
    Object          cached_obj;

    for (i = 0, ilimit = SIZE_LIST(gf->cache_ddi); i < ilimit; ++i) {
        input_p = FETCH_LIST(gf->cache_ddi, i);
	if (!input_p || *input_p < 0)
	    _dxf_ExDie(
               "Executive Inconsistency: _ExCheckDDI - bad index to variable");

        pvar_p = FETCH_LIST(gf->inputs, *input_p);
	if (!pvar_p || *pvar_p < 0)
	    _dxf_ExDie(
               "Executive Inconsistency: _ExCheckDDI bad index to variable");

	inpv = FETCH_LIST(p->vars, *pvar_p);
	if (inpv == NULL)
	    _dxf_ExDie("Executive Inconsistency: _ExCacheDDI 001");

        ExDebug("8", 
             "Checking cache in ExCacheDDIInputs for obj with label/key: %s/%d",
	      _dxf_ExGFuncPathToCacheString(gf), *input_p);

        if (inpv->gv->obj != NULL) {
	    cached_obj = inpv->gv->obj;
	    ExDebug("8", "  obj to be cached is inpv->gv->obj ");
        }
        else {
	    if (inpv->obj != NULL)
	        cached_obj = inpv->obj;
	    else
	        cached_obj = inpv->dflt;
        }
	if (!cached_obj)
	    cached_obj = (Object) DXNewString("NULL");

        if(!DXSetCacheEntry(cached_obj,CACHE_PERMANENT,
	               _dxf_ExGFuncPathToCacheString(gf),*input_p,0))
	    return (FALSE);
    }
    return (TRUE);
}
static void 
ExDoPreExecPreparation(Program *p)
{
    int             i, ilimit;

    /*
     * If we're caching, we must leave the cache disabled until we've looked
     * up and referenced all function inputs 
     */

    if (_dxd_exCacheOn)
	_dxf_ExReclaimDisable();

    /* For each required function, reference all inputs, and outputs */
    if (p->cursor == -1)
	ilimit = SIZE_LIST(p->funcs);
    else
	ilimit = p->cursor;

    for (i = 0; i < ilimit; ++i)
	ExRefFuncIO(p, i);

    if (_dxd_exCacheOn)
	_dxf_ExReclaimEnable();

    MARK_1("ref inputs");
}
/*
 * Function to process DPSEND module - either passes input thru
 * to output if target module's process group is being run by the
 * same processor that is running the source module, or sends a msg
 * to the target processor that includes the required object and its
 * cache tag.
 * Module Inputs are:
 *	tgprocid
 *	srcprocid
 *	tgfuncnbr
 *	tginputnbr
 *	inputobj
 */
static int 
ExProcessDPSENDFunction(Program *p, gfunc *gf, Object *DPSEND_obj)
{
    ProgramVariable *pv, *inpv;
    int             tgfuncnbr, tginputnbr;
    char           *tgprocid = NULL;
    char           *srcprocid = NULL;
    int             index, outdex;

    ExDebug("*1", "In ExProcessDPSENDFunction");
    index = *FETCH_LIST(gf->inputs, 0);
    pv = FETCH_LIST(p->vars, index);
    if (pv->gv != NULL && pv->gv->type != GV_UNRESOLVED && pv->gv->obj != NULL)
    {
	DXExtractString(pv->gv->obj, &tgprocid);
	ExDebug("*1", "tgprocid for gfunc %s is %s", _dxf_ExGFuncPathToCacheString(gf), tgprocid);
    }
    else
	ExDebug("*1", "tgprocid for gfunc %s is NULL or UNRESOLVED!!");

    index = *FETCH_LIST(gf->inputs, 1);
    pv = FETCH_LIST(p->vars, index);
    if (pv->gv != NULL && pv->gv->type != GV_UNRESOLVED && pv->gv->obj != NULL)
    {
	DXExtractString(pv->gv->obj, &srcprocid);
	ExDebug("*1", "srcprocid for gfunc %s is %s", _dxf_ExGFuncPathToCacheString(gf), srcprocid);
    }
    else
	ExDebug("*1", "srcprocid for gfunc %s is NULL or UNRESOLVED!!");

    index = *FETCH_LIST(gf->inputs, 2);
    pv = FETCH_LIST(p->vars, index);
    if (pv->gv != NULL && pv->gv->type != GV_UNRESOLVED && pv->gv->obj != NULL)
    {
	DXExtractInteger(pv->gv->obj, &tgfuncnbr);
	ExDebug("*1", "tgfuncnbr for gfunc %s is %d", _dxf_ExGFuncPathToCacheString(gf), tgfuncnbr);
    }
    else
	ExDebug("*1", "tgfuncnbr for gfunc %s is NULL or UNRESOLVED!!");

    index = *FETCH_LIST(gf->inputs, 3);
    pv = FETCH_LIST(p->vars, index);
    if (pv->gv != NULL && pv->gv->type != GV_UNRESOLVED && pv->gv->obj != NULL)
    {
	DXExtractInteger(pv->gv->obj, &tginputnbr);
	ExDebug("*1", "tginputnbr for gfunc %s is %d", _dxf_ExGFuncPathToCacheString(gf), tginputnbr);
    }
    else
	ExDebug("*1", "tginputnbr for gfunc %s is NULL or UNRESOLVED!!");

    /* Pass input object to output */
    index = *FETCH_LIST(gf->inputs, 4);	/* input 4 is the object to be passed
					 * to output */
    inpv = FETCH_LIST(p->vars, index);
    outdex = *FETCH_LIST(gf->outputs, 0);	/* output 0 is the object to
						 * be sent            */
    pv = FETCH_LIST(p->vars, outdex);
    if (inpv->gv->skip)
    {
	ExDebug("*1", "DPSEND input object being passed thru is in error");
    }
    pv->gv->disable_cache = inpv->gv->disable_cache;
    pv->gv->skip = inpv->gv->skip;
    _dxf_ExDefineGvar(pv->gv, inpv->gv->obj);	/* to send to target
						 * processor      */
    *DPSEND_obj = inpv->gv->obj;/* updates object array upon return */


    if (!ExRunOnThisProcessor(tgprocid)) {
        qsr_args *args;
        args = DXAllocate(sizeof(qsr_args));
        if(args == NULL)
            _dxf_ExDie("Can't allocate space to process distributed send");
        args->procid = tgprocid, 
        args->pv = pv;
        args->index = outdex;
        _dxf_ExRunOn(1, QueueSendRequest, args, 0);
        DXFree(args);
        return(OK);
    }

    return (FALSE);
}

static int QueueSendRequest(qsr_args *args)
{
    char        *host = NULL;
    int		limit, k;
    char        *procid = args->procid;
    ProgramVariable *pv = args->pv;
    int         varindex = args->index;
    SlavePeers  *spindex=NULL;

    if (procid != NULL)
        host = _dxf_ExGVariableGetStr(procid);
    if (procid != NULL && host != NULL)
    {
        for (k = 0, limit = SIZE_LIST(_dxd_slavepeers); k < limit; ++k)
	{
	    spindex = FETCH_LIST(_dxd_slavepeers, k);
	    if (spindex->sfd >= 0 && !strcmp(spindex->peername, host))
	        break;
	}
    }	/* procid != NULL && host != NULL */
    else
    {	/* should only get here if we are a slave */
        /* no process group assigned, assume this is running on master */
	/* master peer will always be first in table */
	spindex = FETCH_LIST(_dxd_slavepeers, 0);
    }
    _dxf_ExReqDPSend(pv, varindex, spindex);
    return (OK);
}

/*--------------------------------------------------------------------*/
/* Determines if this module should be running on this processor      */
/*--------------------------------------------------------------------*/
static int 
ExRunOnThisProcessor(char *procid)
{
    char	*prochost = NULL;
    int		i; 	
    dphosts     *index, dphostent;
    int		ret;

    if(*_dxd_exBadFunc == TRUE && *_dxd_exKillGraph == TRUE)
        return(TRUE);

    if(*_dxd_extoplevelloop) {
	if (_dxd_exRemoteSlave)
            return(FALSE);
        else return(TRUE);
    }

    if (procid)
	prochost = _dxf_ExGVariableGetStr(procid);
    if (procid == NULL || prochost == NULL)
    {
	if (!_dxd_exRemoteSlave)
	{
	    return (TRUE);
	}
	else
	{
	    return (FALSE);
	}
    }

    DXlock (&_dxd_dphostslock, 0);

    for(i = 0; i < SIZE_LIST(*_dxd_dphosts); i++) {
        index = FETCH_LIST(*_dxd_dphosts, i);
        if(strcmp(index->name, prochost) == 0) {
            ret = index->islocal;
            DXunlock(&_dxd_dphostslock, 0);
            return(ret);
        }
    }
    
    /* hostname not found in table, add it */
    dphostent.name = (char *)DXAllocate(strlen(prochost) + 1);
    strcpy(dphostent.name, prochost);

    if((_dxd_exRemoteSlave && !strcmp(_dxd_exHostName, prochost))
      || (!_dxd_exRemoteSlave && _dxfHostIsLocal(prochost)))
        dphostent.islocal = TRUE;
    else
        dphostent.islocal = FALSE;
    APPEND_LIST(dphosts, *_dxd_dphosts, dphostent);

    DXunlock(&_dxd_dphostslock, 0);
    return(dphostent.islocal);
}

/*
 * Determines if all of module's inputs are available.
 * NRB: Unsure if following tests are accurate.
 */
static int 
ExCheckInputAvailability(Program *p, gfunc *n, int fnbr)
{
    int              index, i;
    ProgramVariable *pv;
    gvar            *gv;
    ProgramVariable *switchPv=NULL;
    int 	     sw;
    int		     switchIn;

    if (*_dxd_exKillGraph)
        return(TRUE);

    for (i = 0; i < n->nin; i++)
    {
	index = *FETCH_LIST(n->inputs, i);
	if (index >= 0)
	{
	    pv = FETCH_LIST(p->vars, index);
	    gv = pv->gv;
	    if ((IsRoute(n) || IsSwitch(n)) && i == 0)
		switchPv = pv;
	    if (gv == NULL || (gv->skip && (n->flags & MODULE_ERR_CONT) != 0))
		continue;
	    if ((pv->dflt == NULL || (pv->dflt != NULL && pv->is_output))
                && gv->type == GV_UNRESOLVED)
	    {
		if (IsSwitch(n) && i > 0)
		{
                    if (switchPv->gv) 
                    {
                        if(switchPv->gv->obj == NULL) 
                        {
                            if (switchPv->dflt)
                            {
                                if(DXExtractInteger(switchPv->dflt, &sw)== NULL)
                                    continue;
                            }
                        }
                        else 
                        {
                            if(DXExtractInteger(switchPv->gv->obj, &sw) == NULL)
                                continue;
                        }
			if (i != sw)
			    continue;
			if (pv->required != REQUIRED)
			    continue;
		    }
		    /* Otherwise, no inputs are required except the 0th */
		    else
			continue;
		}
		if (IsRoute(n) && i > 0)
		{
                    if (switchPv->gv) 
                    {
                        if(switchPv->gv->obj == NULL) 
                        {
                            if (switchPv->dflt)
                            {
                                if(DXExtractInteger(switchPv->dflt, &sw)== NULL)
                                    continue;
                            }
                        }
                        else 
                        {
                            if(DXExtractInteger(switchPv->gv->obj, &sw) == NULL)
                                continue;
                        }
			if (i <= 0 || i >= n->nout ||
				*FETCH_LIST(n->outputs,i) < 0)
			    continue;
			switchIn = *FETCH_LIST(n->inputs, 1);
			if (switchIn < 0)
			    continue;
			pv = FETCH_LIST(p->vars, switchIn);
			/* Otherwise, no inputs are required except the 0th */
			if (pv->required != REQUIRED)
			    continue;
		    }
		    else
			continue;
		}
		if (p->first_skipped == -1 || p->first_skipped >= fnbr)
		    p->first_skipped = fnbr;
		ExDebug("5",
		    "Input %d at index %d for gfunc %s/%d not yet available",
		    i, index, _dxf_ExGFuncPathToString(n), fnbr);
		return (FALSE);
	    }
	}
    }
    return (TRUE);
}

void
_dxf_ExMarkVarRequired(int gid, int sgid, int fnbr, int pvar, int requiredFlag)
{
    int i;
    ProgramVariable *pv;
    Program *subp;

    if (_dxd_exContext->program == NULL)
	return;
    if (_dxd_exContext->program->graphId != gid)
	return;


    pv = FETCH_LIST(_dxd_exContext->program->vars, pvar);
    if (pv == NULL) {
        DXUIMessage("ERROR", "Missing variable in program");
        *_dxd_exKillGraph = TRUE;
        return;
    }
    if (pv->required == REQUIRED ||
	(pv->required == REQD_IFNOT_CACHED &&
	    requiredFlag == REQD_IFNOT_CACHED))
	return;

    pv->required = requiredFlag;

#ifdef CPV_DEBUG
    {
	gfunc *gf = FETCH_LIST(_dxd_exContext->program->funcs, fnbr);
	int i;
	for (i = 0; i < SIZE_LIST(gf->inputs); ++i)
	    if (*FETCH_LIST(gf->inputs, i) == pvar)
		printf("Mark input %d of %s requires (0 based)\n",
		    i, _dxf_ExGFuncPathToString(gf));
    }
#endif

    subp = _dxd_exContext->subp[sgid];
    /* if we haven't evaluated this macro yet just return */
    if(subp == NULL)
        return;

    for (i = fnbr - 1; i >= 0; --i)
	ExProcessGraphFunc(subp, i, TRUE);

    subp->cursor = -1;
    ExDoPreExecPreparation(subp);
    subp->outstandingRequests++;
    _dxf_ExRQEnqueue(_QueueableStartModulesReset,
		     (Pointer) subp, 1, 0, 0, FALSE);
}

int
_dxf_ExPrintProgram(Program *p)
{
    int i;
    gfunc *gf;
    static char *reqnames[] = 
	{"NOT_SET",
	 "NOT_REQUIRED",
	 "REQUIRED",
	 "ALREADY_RUN",
	 "REQD_IFNOT_CACHED",
	 "RUNNING"};
    printf("%d Functions:\n", SIZE_LIST(p->funcs));
    for (i = 0; i < SIZE_LIST(p->funcs); ++i)
    {
	gf = FETCH_LIST(p->funcs, i);
        if(gf->ftype == F_MACRO && gf->func.subP) {
            printf("macro %s ", gf->name);
            _dxf_ExPrintProgram(gf->func.subP);
        }
        else 
	    printf(
              "%c%c %3d: %s: required: %s, procgroupid: %s, flags: 0x%08x\n",
	      p->cursor+1 == i? '>': ' ',
	      p->first_skipped != -1 && p->first_skipped == i? '*': ' ',
	      i,
	      _dxf_ExGFuncPathToString(gf),
	      reqnames[gf->required+1],
	      gf->procgroupid? gf->procgroupid: "(NULL)",
	      gf->flags);
    }
    return (0);
}

int
_dxf_ExPrintProgramIO(Program *p)
{
    int i, j, index;
    gfunc *gf;

    printf("%d Functions:\n", SIZE_LIST(p->funcs));
    for (i = 0; i < SIZE_LIST(p->funcs); ++i)
    {
	gf = FETCH_LIST(p->funcs, i);
	printf("%c%c %3d: %s: in: ",
	  p->cursor+1 == i? '>': ' ',
	  p->first_skipped != -1 && p->first_skipped == i? '*': ' ',
	  i, _dxf_ExGFuncPathToString(gf));
        for (j = 0; j < SIZE_LIST(gf->inputs); ++j) {
            index = *FETCH_LIST(gf->inputs, j);
            if(index != -1) printf("%d ", index);
        }
        printf(" out: ");
        for (j = 0; j < SIZE_LIST(gf->outputs); ++j) {
            index = *FETCH_LIST(gf->outputs, j);
            if(index != -1) printf("%d ", index);
        }
        printf("\n");
        if(gf->ftype == F_MACRO && gf->func.subP) {
            printf("macro %s ", gf->name);
            _dxf_ExPrintProgramIO(gf->func.subP);
        }
    }
    return (0);
}

void DXLoopDone(int done)
{
    if(done)
        _dxd_exContext->program->loopctl.isdoneset = TRUE;
    _dxd_exContext->program->loopctl.done = done;
}

int DXLoopFirst()
{
    return(_dxd_exContext->program->loopctl.first);
}

void DXSaveForId(char *modid)
{
    APPEND_LIST(char *, _dxd_exContext->program->foreach_id, modid);
}

static void ExCleanupForState()
{
    int i;
    char *modid;

    for(i = SIZE_LIST(_dxd_exContext->program->foreach_id) - 1; i >= 0; --i) {
        modid = *FETCH_LIST(_dxd_exContext->program->foreach_id, i); 
        DXSetCacheEntryV(NULL, 0, modid, 0, 0, NULL);
        DXFree(modid);
    }
    FREE_LIST(_dxd_exContext->program->foreach_id);
}

int _dxf_ExIsExecuting()
{
   if (_dxd_exContext->program == NULL)
       return FALSE;
   else return TRUE;
}

int DXPrintCurrentModule(char *string) 
{
    DXMessage("%s: %s", string, _dxf_ExGFuncPathToString(_dxd_exCurrentFunc)); 
    return (0);
}

