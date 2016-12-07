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
#include "config.h"
#include "vcr.h"
#include "graph.h"
#include "_variable.h"
#include "sysvars.h"
#include "log.h"
#include "packet.h"
#include "rq.h"
#include "distp.h"
#include "graphqueue.h"
#include "command.h"

typedef enum
{
    VCR_M_STOP = 0,
    VCR_M_PAUSE,
    VCR_M_STEP,
    VCR_M_PLAY
} vcr_mode;


typedef struct
{
    lock_type	lock;			/* vcr semaphore		*/
    int		dir;			/* current direction		*/
    vcr_mode	mode;			/* play mode			*/

    int		loop;			/* loop step/play		*/
    int		palin;			/* palindrome step/play		*/
    int		flip;			/* palindrome has flipped	*/
    int		steps;

    int		ngraphs;		/* number of outstanding graphs	*/
    int		graphId;		/* Id of most recent graph	*/	

    int		nframe;			/* what we think next frame is	*/

    struct node	*tree;			/* parse tree to execute	*/
    Program	*graph;
    int		change;
} _vcr;


#define	VCR_DEFAULT_START	1
#define	VCR_DEFAULT_END		100
#define	VCR_DEFAULT_DELTA	1
#define	VCR_DEFAULT_NEXT	VCR_DEFAULT_START
#define	VCR_DEFAULT_FRAME	VCR_DEFAULT_START

static	_vcr	*vcr = NULL;
static	EXDictionary	vcr_dict;

typedef struct {
    int	s_frame;
    int	e_frame;
    int	n_frame;
    int	d_frame;
    int	c_frame;
} VCRState;

static VCRState now = {		/* The frame that has a graph. */
    VCR_DEFAULT_START, 
    VCR_DEFAULT_END,
    VCR_DEFAULT_NEXT,
    VCR_DEFAULT_DELTA,
    VCR_DEFAULT_FRAME
};
static VCRState old = {		/* The frame that is currently running. */
    VCR_DEFAULT_START, 
    VCR_DEFAULT_END,
    VCR_DEFAULT_NEXT,
    VCR_DEFAULT_DELTA,
    VCR_DEFAULT_FRAME
};
static VCRState prior = {	/* The frame that's on the screen. */
    VCR_DEFAULT_START, 
    VCR_DEFAULT_END,
    VCR_DEFAULT_NEXT,
    VCR_DEFAULT_DELTA,
    VCR_DEFAULT_FRAME
};

typedef struct {
    int comm;
    long arg1;
    int arg2;
} VCRCommandTaskArg;

/*
 * Static functions.
 */
static	void	ExVCRSet	(char *var, int val);
static 	int	ExVCRGet	(char *name);
static	void	ExVCRLoad	(void);
static	void	ExVCRAdvance	(void);


/*************************************************************************/

/*
 * Top level initialization of the VCR by the executive.
 */

Error _dxf_ExInitVCR (EXDictionary dict)
{
    int		size;

    vcr_dict = dict;
    size = sizeof (_vcr);

    if (vcr)
	_dxf_ExCleanupVCR ();

    if ((vcr = (_vcr *) DXAllocate (size)) == NULL)
	return (ERROR);

    memset (vcr, 0, size);

    if (DXcreate_lock (&vcr->lock, "vcr lock") != OK)
    {
	DXFree ((Pointer) vcr);
	vcr = NULL;
	return (ERROR);
    }

    _dxf_ExInitVCRVars ();
    vcr->change = TRUE;
    vcr->graph = NULL;
    return (OK);
}


/*
 * Cleanup function.
 */

void _dxf_ExCleanupVCR ()
{
    if (vcr == NULL)
	return;

    DXdestroy_lock (&vcr->lock);
    DXFree ((Pointer) vcr);
    vcr = NULL;
}


int _dxf_ExVCRRunning ()
{
    int		ret;

    /* a quick out		*/
    if (vcr->mode != VCR_M_STOP && vcr->mode != VCR_M_PAUSE)
	return (TRUE);
    
    if (! DXtry_lock (&vcr->lock, 0))		/* Someone else has it	*/
	return (TRUE);

    /* now really make sure	*/
    ret = (vcr->mode != VCR_M_STOP && vcr->mode != VCR_M_PAUSE);

    DXunlock (&vcr->lock, 0);

    return (ret);
}


int _dxf_ExVCRCallBack (int n)
{
    vcr_mode	mode;
    int		ngraphs;

    DXlock (&vcr->lock, 0);

    ngraphs = --(vcr->ngraphs);
    mode = vcr->mode;

    ExDebug ("5", "_dxf_ExVCRCallBack,%d: if ngraphs(%d) == 0 && mode (%d) == %d or %d\n                     _dxf_ExSPack \"stop\"", 
	__LINE__, ngraphs, mode, VCR_M_STOP, VCR_M_PAUSE);
    if ((ngraphs == 0) && (mode == VCR_M_STOP || mode == VCR_M_PAUSE))
    {
	_dxf_ExSPack  (PACK_INTERRUPT, n, "stop", 4);
    }

    DXunlock (&vcr->lock, 0);

    return (1);
}


/*
 * Resets the VCR and its associated variables back to its initial state.
 */

int _dxf_ExInitVCRVars ()
{
    /* Initialize the global variables used by the VCR */

    ExVCRSet (VCR_ID_START, VCR_DEFAULT_START);
    ExVCRSet (VCR_ID_END,   VCR_DEFAULT_END);
    ExVCRSet (VCR_ID_DELTA, VCR_DEFAULT_DELTA);
    ExVCRSet (VCR_ID_NEXT,  VCR_DEFAULT_NEXT);
    ExVCRSet (VCR_ID_FRAME, VCR_DEFAULT_FRAME);

    /* Initialize the internal variables used by the VCR */

    vcr->dir	= VCR_D_FORWARD;
    vcr->mode	= VCR_M_STOP;

    vcr->loop	= FALSE;
    vcr->palin	= FALSE;
    vcr->flip	= FALSE;

    vcr->steps	= 0;

    /* Clean up any prior incarnations */

    if (vcr->tree)
	_dxf_ExPDestroyNode (vcr->tree);
    vcr->tree    = NULL;

    return (OK);
}


/*
 * Sets a specific variable with the given integer value.
 */
static void ExVCRSet (char *var, int val)
{
    gvar	*gv;
    Array	arr;
    GDictSend   pkg;

    /* Make sure we remember what the next frame is */
    if (! strcmp (var, VCR_ID_NEXT))
	vcr->nframe = val;

    arr = DXNewArray (TYPE_INT, CATEGORY_REAL, 0);
    DXAddArrayData (arr, 0, 1, (Pointer) &val);
    gv = _dxf_ExCreateGvar (GV_UNRESOLVED);
    _dxf_ExDefineGvar(gv, (Object) arr);
    gv->cost = 1.0;
    _dxf_ExVariableInsert (var, vcr_dict, (EXO_Object) gv);
    _dxf_ExCreateGDictPkg(&pkg, var, (EXObj)gv);
    _dxf_ExDistributeMsg(DM_INSERTGDICT, (Pointer)&pkg, 0, TOSLAVES);
}


/*
 * Retrieves the specified value from the environment, converting it to
 * integer as necessary.
 */

static int ExVCRGet (char *var)
{
    gvar	*gv;
    Array	arr;
    int		val = 0;

    if ((gv = (gvar *) _dxf_ExVariableSearch (var, vcr_dict)) == NULL)
	return (0);

    arr = (Array)  gv->obj;

    DXExtractInteger ((Object) arr, &val);

    ExDelete (gv);

    return (val);
}


/*
 * Loads the local copies of the VCR's frame variables from the environment.
 */

static void ExVCRLoad ()
{
    now.s_frame = ExVCRGet (VCR_ID_START);
    now.e_frame = ExVCRGet (VCR_ID_END);
    now.n_frame = ExVCRGet (VCR_ID_NEXT);
    now.d_frame = ExVCRGet (VCR_ID_DELTA);
    now.c_frame = now.n_frame;

    if (now.d_frame < 1)
    {
	now.d_frame = (now.d_frame == 0) ? 1 : - now.d_frame;
	ExVCRSet (VCR_ID_DELTA, now.d_frame);
    }

    now.n_frame = (now.n_frame < now.s_frame) ? now.s_frame : now.n_frame;
    now.n_frame = (now.n_frame > now.e_frame) ? now.e_frame : now.n_frame;
}


/*
 * Advances the VCR one frame
 */

static void ExVCRAdvance ()
{
    int		n;

    n = now.n_frame + (now.d_frame * vcr->dir * (vcr->flip ? -1 : 1));

    /* We are still within the valid region, this was a simple advance */
    if (n >= now.s_frame && n <= now.e_frame)
    {
	now.n_frame = n;
	return;
    }
    
    /* Now we have to decide what to do based upon the state of the VCR */
    if (n > now.e_frame)
    {
	if (vcr->loop)
	{
	    if (vcr->palin)
	    {
		vcr->flip = ! vcr->flip;
		n = now.n_frame + (now.d_frame * vcr->dir * (vcr->flip ? -1 : 1));
	    }
	    else
	    {
		n = now.s_frame;
	    }
	}
	else	/* no vcr->loop */
	{
	    if (vcr->palin)
	    {
		if (vcr->flip)
		{
		    vcr->flip = FALSE;
		    vcr->mode = VCR_M_STOP;
		    n = now.e_frame;
		}
		else
		{
		    vcr->flip = ! vcr->flip;
		    n = now.n_frame + (now.d_frame * vcr->dir * (vcr->flip ? -1 : 1));
		}
	    }
	    else
	    {
		n = now.s_frame;
		vcr->mode = VCR_M_STOP;
	    }
	}
    }
    else	/* n < now.s_frame */
    {
	if (vcr->loop)
	{
	    if (vcr->palin)
	    {
		vcr->flip = ! vcr->flip;
		n = now.n_frame + (now.d_frame * vcr->dir * (vcr->flip ? -1 : 1));
	    }
	    else
	    {
		n = now.e_frame;
	    }
	}
	else	/* no vcr->loop */
	{
	    if (vcr->palin)
	    {
		if (vcr->flip)
		{
		    vcr->flip = FALSE;
		    vcr->mode = VCR_M_STOP;
		    n = now.s_frame;
		}
		else
		{
		    vcr->flip = ! vcr->flip;
		    n = now.n_frame + (now.d_frame * vcr->dir * (vcr->flip ? -1 : 1));
		}
	    }
	    else
	    {
		n = now.e_frame;
		vcr->mode = VCR_M_STOP;
	    }
	}
    }

    now.n_frame = (n < now.s_frame) ? now.s_frame : ((n > now.e_frame)  ? now.e_frame : n);
    return;
}

static int
VCRCommandTask(Pointer pArg)
{

    VCRCommandTaskArg *arg = (VCRCommandTaskArg*)pArg;
    _dxf_ExVCRCommand (arg->comm, arg->arg1, arg->arg2);
    DXFree (pArg);

    return (OK);
}

/*
 * VCR command interpreter.  Sets the values of the VCR's internal data
 * structure.
 */

void _dxf_ExVCRCommand (int comm, long arg1, int arg2)
{
    struct node		*oldtree = NULL;
    vcr_mode		mode;

    if (exJID != 1) 
    {
	VCRCommandTaskArg *arg = 
	    (VCRCommandTaskArg *)DXAllocate (sizeof (VCRCommandTaskArg));
	if (arg == NULL)
	    _dxf_ExDie ("_dxf_ExVCRCommand: can't allocate");


	arg->comm = comm;
	arg->arg1 = arg1;
	arg->arg2 = arg2;
	_dxf_ExRQEnqueue (VCRCommandTask, (Pointer) arg, 1, 0, 1, FALSE);
	return;
    }

    if (vcr == NULL)
	return;

    DXlock (&vcr->lock, 0);

    /* In most cases we want to delete the next graph that is queued   */
    /* because we are changing the VCR and the next graph to run       */
    /* will probably be different from the one currently running.      */
    /* This has a bad effect though if we are reading a script through */
    /* stdin. At the end of the script we receive and EOF which will   */
    /* call VCRCommand to turn off looping. In this case we do not     */
    /* wish to delete the next graph element.                          */
    if(! *_dxd_exTerminating) {
        if (vcr->graph != NULL)
	    _dxf_ExGraphDelete (vcr->graph);
        vcr->graph = NULL;
    }

    /* The one that I want to advance past is the one on the screen. */
    now = old;

    switch (comm)
    {
    case VCR_C_STOP:
	ExDebug ("1", "VCR  VCR_C_STOP");
	mode = vcr->mode;
	vcr->mode  = VCR_M_STOP;
	vcr->flip  = FALSE;
	vcr->steps = 0;

	if (mode != VCR_M_STOP &&
	    mode != VCR_M_PAUSE)
	{
	    if (!_dxf_ExGQAllDone())
	    {
		_dxf_ExExecCommandStr ("kill");
		ExDebug ("1", "Killing frame %d", old.c_frame);
		old = prior;
	    }
	    ExDebug ("5", "_dxf_ExVCRCommand,%d: if ngraphs(%d) == 0 _dxf_ExSPack \"stop\"",
		__LINE__, vcr->ngraphs);
	    if (vcr->ngraphs == 0)
	    {
		_dxf_ExSPack  (PACK_INTERRUPT, vcr->graphId, "stop", 4);
	    }
	    now = old;
	    ExVCRSet (VCR_ID_FRAME, now.c_frame);
	    ExVCRSet (VCR_ID_NEXT,  now.n_frame);
	}
	break;

    case VCR_C_PAUSE:
	ExDebug ("1", "VCR  VCR_C_PAUSE");
	mode = vcr->mode;
	vcr->mode  = VCR_M_PAUSE;
	if (mode != VCR_M_STOP &&
	    mode != VCR_M_PAUSE)
	{
	    if (!_dxf_ExGQAllDone())
	    {
		_dxf_ExExecCommandStr ("kill");
		ExDebug ("1", "Killing frame %d", old.c_frame);
		old = prior;
	    }
	    ExDebug ("5", "_dxf_ExVCRCommand,%d: if ngraphs(%d) == 0 _dxf_ExSPack \"stop\"",
		__LINE__, vcr->ngraphs);
	    if (vcr->ngraphs == 0)
		_dxf_ExSPack  (PACK_INTERRUPT, vcr->graphId, "stop", 4);
	    now = old;
	    ExVCRSet (VCR_ID_FRAME, now.c_frame);
	    ExVCRSet (VCR_ID_NEXT,  now.n_frame);
	}
	break;

    case VCR_C_PLAY:
	if  (! vcr->tree)
	    break;
	ExDebug ("1", "VCR  VCR_C_PLAY");
	vcr->mode  = VCR_M_PLAY;
	vcr->flip  = FALSE;
	vcr->steps = 0;
	break;

    case VCR_C_STEP:
	if  (! vcr->tree)
	    break;
	ExDebug ("1", "VCR  VCR_C_STEP");
	mode = vcr->mode;
	vcr->steps += vcr->dir;
	vcr->mode = vcr->steps ? VCR_M_STEP : VCR_M_STOP;
	ExDebug ("5", "_dxf_ExVCRCommand,%d: if ngraphs(%d) == 0 && mode (%d) == %d or %d\n                     _dxf_ExSPack \"stop\"", 
	__LINE__, vcr->ngraphs, mode, VCR_M_STOP, VCR_M_PAUSE);
	if (vcr->ngraphs == 0  &&
	    mode != VCR_M_STOP &&
	    mode != VCR_M_PAUSE)
	    _dxf_ExSPack  (PACK_INTERRUPT, vcr->graphId, "stop", 4);
	break;

    case VCR_C_DIR:
	ExDebug ("1", "VCR  VCR_C_DIR");
	if (vcr->dir == (int)arg1)
	    break;
	vcr->dir  = (int)arg1;
	/* vcr->flip = FALSE; */

	/*
	 * Only reposition the current position if the user hasn't
	 * explicitly done it by changing @nextframe.
	 */

	ExVCRLoad ();
	if (vcr->nframe == now.n_frame)
	{
	    ExVCRAdvance ();
	    ExVCRAdvance ();
	    ExVCRSet (VCR_ID_NEXT,  now.n_frame);
	}
	break;

    case VCR_C_FLAG:
	ExDebug ("1", "VCR  VCR_C_FLAG");
	switch ((int)arg1)
	{
	    case VCR_F_LOOP:
		vcr->loop  = arg2;
		break;

	    case VCR_F_PALIN:
		vcr->palin  = arg2;
                if(vcr->flip) {
		    vcr->flip = FALSE;
                    /*
                     * Only reposition the current position if the user 
                     * hasn't explicitly done it by changing @nextframe.
                     */

                    ExVCRLoad ();
                    if (vcr->nframe == now.n_frame)
                    {
                        ExVCRAdvance ();
                        ExVCRAdvance ();
                        ExVCRSet (VCR_ID_NEXT,  now.n_frame);
                    }
                }
		break;
	}
	break;

    case VCR_C_TREE:
	ExDebug ("1", "VCR  VCR_C_TREE");
	oldtree = vcr->tree; 
	vcr->tree = (struct node *) arg1;
	break;
    }

    DXunlock (&vcr->lock, 0);

    if (oldtree)
	_dxf_ExPDestroyNode (oldtree);
}

void
_dxf_ExVCRRedo(void)
{
    if(_dxd_exRemoteSlave) {
        _dxf_ExDistributeMsg(DM_VCRREDO, NULL, 0, TOPEER0);
        return; 
    }

    if (vcr == NULL || vcr->tree == NULL)
	return;

    if (vcr->graph != NULL)
    {
	_dxf_ExGraphDelete(vcr->graph);
	vcr->graph = NULL;
    }
    vcr->change = TRUE;
}

void
_dxf_ExVCRChange(void)
{
    if (vcr == NULL || vcr->tree == NULL)
	return;
    vcr->change = TRUE;
}

void
_dxf_ExVCRChangeReset(void)
{
    if (vcr == NULL || vcr->tree == NULL)
	return;
    vcr->change = FALSE;
}

int
_dxf_ExCheckVCR (EXDictionary dict, int multiProc)
{
    Program		*graph;
    int			doGraph;
    int			ret;
    VCRState		lastGraph;

    multiProc = TRUE; /* for debugging */

    /* Early outs */
    if (vcr == NULL)
	return (0);

    if (vcr->tree == NULL)
	return (0);
    
    if ((!multiProc || vcr->graph == NULL) && 
	(vcr->mode == VCR_M_STOP || vcr->mode == VCR_M_PAUSE))
	return (0);

    if (! DXtry_lock (&vcr->lock, 0))		/* Someone else has it	*/
	return (0);

    if (vcr->mode != VCR_M_STOP && vcr->mode != VCR_M_PAUSE)
    {
	/* something still running */
	if ((multiProc && vcr->graph == NULL) || (!multiProc && _dxf_ExGQAllDone ()))
	{
	    ExVCRLoad ();			/* Load vars from dict	*/

	    switch (vcr->mode)
	    {
	    case VCR_M_STEP:
		ExVCRAdvance ();
		vcr->steps += (vcr->steps < 0) ? 1 : -1;
		if (vcr->steps == 0)
		    vcr->mode = VCR_M_STOP;
		break;

	    case VCR_M_PLAY:
		ExVCRAdvance ();
		break;
	    default:
	        break;
	    }
	    doGraph = TRUE;
	}
	/* Else if they've changed something since the last graph,
	 * incorporate the changes (and if they've changed @nextframe, chane
	 * @frame and redo the graph.
	 */
	else if (multiProc && vcr->change)
	{
	    lastGraph = now;
	    ExVCRLoad();
	    /* If they changed anything, reset next and readvance
	     * to make sure that everything is still OK.
	     * If they changed "next", do the one they want.
	     */
	    if (now.s_frame != lastGraph.s_frame ||
		now.e_frame != lastGraph.e_frame ||
		now.d_frame != lastGraph.d_frame ||
		now.n_frame != lastGraph.c_frame)
	    {
		if (now.n_frame == lastGraph.n_frame)
		    now.n_frame = lastGraph.c_frame;

		now.c_frame = now.n_frame;
		switch (vcr->mode)
		{
		case VCR_M_STEP:
		    ExVCRAdvance ();
		    vcr->steps += (vcr->steps < 0) ? 1 : -1;
		    if (vcr->steps == 0)
			vcr->mode = VCR_M_STOP;
		    break;

		case VCR_M_PLAY:
		    ExVCRAdvance ();
		    break;
		default:
		    break;
		}
	    }

	    doGraph = TRUE;
	    if (vcr->graph != NULL)
		_dxf_ExGraphDelete (vcr->graph);
	    vcr->graph = NULL;
	}
	else
	    doGraph = FALSE;

	if (doGraph)
	{
	    ExVCRSet (VCR_ID_FRAME, now.c_frame);
	    ExVCRSet (VCR_ID_NEXT,  now.n_frame);

	    vcr->change = FALSE;

	    _dxf_ExGraphInit ();
	    if ((vcr->graph = graph = _dxf_ExGraph (vcr->tree)) != NULL)
	    {

		graph->origin     = GO_VCR;
		graph->vcr.frame  = now.c_frame;
		graph->vcr.nframe = now.n_frame;
		graph->vcr.stop   = (vcr->mode == VCR_M_STOP) ? TRUE : FALSE;
	    }
	    ExDebug ("1", "VCR Creating  graph %x for frame %3d", 
		graph, now.c_frame);
	}
    }

    /* If we have a graph to do, and there is none being done, do this
     * graph, and indicate that we're working on the old now, and we're
     * done with the old old.
     */
    if (_dxf_ExGQAllDone() && vcr->graph)
    {
	vcr->ngraphs++;
	vcr->graphId      = vcr->graph->graphId;
        /* we are the master, send a copy of the parse tree
         * to all slaves
         */
        _dxf_ExSendParseTree(vcr->tree);
	_dxf_ExGQEnqueue (vcr->graph);
	ExDebug ("1", "VCR Enqueuing graph 0x%x for frame %3d", 
	    vcr->graph, now.c_frame);
	prior = old;
	old = now;
	vcr->graph = NULL;
	ret = TRUE;
    }
    else 
	ret = FALSE;

    DXunlock (&vcr->lock, 0);

    return (ret);
}
