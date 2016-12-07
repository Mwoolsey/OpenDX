/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/graph.h,v 1.10 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>


#ifndef	_GRAPH_H
#define	_GRAPH_H

#include "exobject.h"
#include "parse.h"
#include "d.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum
{
    GV_CACHE,			/* user cache entry */
    GV_UNRESOLVED,		/* executive variable */
    GV_DEFINED,			/* executive variable */
    GV_CONSTANT			/* executive variable */
} _gvtype;

typedef enum
{
   GV_DONTSKIP = 0,
   GV_SKIPERROR,
   GV_SKIPROUTE,
   GV_SKIPMACROEND
} _skip_state;

typedef enum
{
    GS_TBD = 0,			/* node has not be processed */
    GS_QUEUED,			/* node is queued for execution */
    GS_SERIALIZE,		/* node should be queued serially */
    GS_RUNNING,			/* node is executing */
    GS_DONE			/* node has completed */
} _gstatus;


typedef enum
{
    GO_UNKNOWN = 0,
    GO_FOREGROUND,		/* a normal foreground command		*/
    GO_BACKGROUND,		/* a background command			*/
    GO_VCR			/* created by the VCR			*/
} _gorigin;

typedef enum
{
    CACHE_OFF = 0,
    CACHE_ALL,	 	 	/* cache all occurrences 		*/
    CACHE_LAST,	 	 	/* cache last occurrence only		*/
    CACHE_LAST_PERM 	 	/* cache last occurrence permanently	*/
} _excache;

typedef enum
{
    F_UNKNOWN = 0,
    F_MODULE, 
    F_MACRO,
    F_LOOP
} _ftype;

#define __BIT(x) (1<<(x))
#define RouteFlag 	__BIT(0) 
#define SwitchFlag 	__BIT(1)
#define FastSwitchFlag 	__BIT(2)

#define IsRoute(gf) ((gf)->flowtype & RouteFlag)
#define IsSwitch(gf) ((gf)->flowtype & SwitchFlag)
#define IsFastSwitch(gf) ((gf)->flowtype & FastSwitchFlag)


#define RECIPE_SIZE 11				/* 8 hex digits + _R + \0 */
/* NOTE: anything added to gvar may need to be added in distp.h for */
/* distributed. Gvars get sent back and forth between peers in distributed */
typedef struct gvar
{
    struct exo_object	object;
    struct gvar		*next;
    _gvtype		type;
    uint32 		reccrc;		/* cache tag		*/
    double		cost;
    _skip_state		skip;		/* error or turned off by route */
    int			disable_cache;	/* disable output cacheing */
    int			procId;
    Object		obj;
    Object		oneshot;	/* Default object for oneshots */
} gvar;


#define INIT_PVAR(pv) ( \
	(pv).required = (pv).defined = (pv).is_output = FALSE, \
	(pv).excache = CACHE_ALL, \
	(pv).cacheattr = FALSE, \
	(pv).reccrc = (pv).refs = 0, \
        (pv).cts.nalloc = (pv).cts.nused = 0, (pv).cts.vals = NULL, \
	(pv).gv = NULL, \
	(pv).obj = (pv).dflt = NULL)

typedef struct ProgramVariable {
    int 	required;		/* Is this variable required?	*/
    gvar	*gv;			/* Pointer to the gvar		*/
    int 	defined;		/* Is obj defined?		*/
    Object	obj;			/* Constant and Default values 	*/
    Object	dflt;			
    _excache	excache;	        /* output caching indicator	*/
    int		cacheattr;        	/* cache attr specified ?       */
    int		refs;			/* How many refs does the prog have? */
    uint32 	reccrc;			/* cache tag			*/
    LIST(uint32) cts;			/* Stack of cache tags 		*/
    int          is_output;             /* is this an output            */
} ProgramVariable;

typedef struct ProgramRef {
    char *name;				/* Name of the variable		*/
    int index;				/* indices into the "vars" array */
    Object oneshot;			/* Default reset value for oneshots */
} ProgramRef;

typedef struct MacroRef {
   char *name;				/* macro name */
   int index;				/* ref number */
} MacroRef;

typedef struct AsyncVars {
   int nameindx;			/* pvar # that contains var name */
   int valueindx;			/* pvar # that contains var value */
} AsyncVars;

typedef struct ReRouteMap {
    int input_n; 
    int output_n;
} ReRouteMap;

#define MAX_PATH_COMP 10

typedef struct {
    uint16 modules  [ MAX_PATH_COMP ];  /*  Module string indices             */
    uint16 instances[ MAX_PATH_COMP ];  /*  Module instance nums              */
    int    num_comp;                    /*  Number of path components         */
} ModPath;

typedef struct gfunc
{
    char		*name;		/* function name		*/
    int			required;	/* Function needed for this exec */
    int			instance;	/* instance of func (attr from ui) */
    _excache		excache;        /* output caching indicator	*/
    char		*procgroupid;   /* process group id             */
    ModPath		mod_path;	/* module path (<mac/func>:<#> pairs) */
    _ftype 		ftype;          /* macro, loop or module        */
    union {
         PFI	  module;               /* function to call             */
         struct program  *subP;         /* loop or macro sub program    */
    } 			func;
    int			flags;		/* module flags			*/
    LIST(int)		inputs;		/* List of inputs and outputs	*/
    LIST(int)		outputs;
    LIST(ReRouteMap)	reroute;        /* List of input no/output no   */
                                        /* pairs for reroutable modules */
    LIST(int)           cache_ddi;      /* List of inputs that should be */
                                        /* cached for data-driven interactors*/
    int			nin;		/* Number of in(out)puts as defined */
    int			nout;			/* not as used		*/

    int			flowtype;     	/* switch, fastswitch, route */
    EXRefTime		starttime;	/* time node execution started	*/
    EXRefTime		endtime;	/* time node execution ended	*/
    int			tag_changed;	/* cache tag changed since last */
					/* time (if cache last)         */
    int			rerun_index;	/* input # containing name to   */
         				/* use for async named		*/
    
} gfunc;

typedef struct program {
    lock_type			lock;
#ifdef ALLOC_UPFRONT
    int				local;
#endif
    LIST(ProgramRef)		inputs;
    LIST(ProgramRef)		outputs;
    LIST(ProgramRef)		undefineds;
    LIST(int)			wiredvars; /* wired vars for each macro */
    LIST(MacroRef)   		macros;
    LIST(AsyncVars)		async_vars;
    LIST(ProgramVariable)	vars;
    LIST(gfunc)			funcs;
    int				cursor;	/* The first (or currently running)
					 * function (module) */
    int				first_skipped; /* first skipped module   */
    int				graphId;
    int				subgraphId;
    _gorigin			origin;	/* where the graph came from	*/
    int				error;	/* an error occurred		*/
    int				informedMaster;	/* true if slave and    */
					/* GRAPHDONE sent to master	*/
    int 			runable; /* Number of runable modules   */
    int				deleted; /* delete graph when runable = 0 */
    int				outstandingRequests;
					/* #operations in the run queue */
    int				waited;	/* Have we "WaitOnSlaves yet? */
    struct {
        int first; 	  /* for loops */
        int done;  	  /* is this loop finished? */
        int counter;	  /* count inside loop, used for cacheing */
        int isdoneset;    /* was done set during loop execution */
    }				loopctl;
    struct {
	int frame;
	int nframe;
	int stop;
    } 				vcr;
    struct program 		*returnP;
    gfunc			*saved_gf;
    LIST(char *) 		foreach_id;
} Program;
    

extern gfunc            *_dxd_exCurrentFunc; /* from graph.c */
extern  EXDictionary    _dxd_exGraphCache;   /* from graph.c */

gvar	 *_dxf_ExCreateGvar (_gvtype type);
void	  _dxf_ExDefineGvar (gvar *gv, Object o);
void	  _dxf_ExUndefineGvar(gvar *gv);
Program	 *_dxf_ExGraph (node *n);
void	  _dxf_ExGraphDelete (Program *p);
int	  _dxf_ExGraphCompact (void);
void      _dxf_ExGraphInit (void);
void      _dxf_ExMarkVarRequired(int gid, int sgid, int fnbr, int pvar, int requiredFlag);
void 	  _dxf_ExGQDecrement (Program *p);
void      _dxf_ExQueueGraph(Program * p);
int       _dxf_NextGraphId();
int       _dxf_ExMacroRecursionCheck (char *name, _ntype type);
int       _dxf_ExNoCachePush (int n);
void      _dxf_ExNoCachePop ();
void      _dxf_ExMacroRecursionPop (char *name, _ntype type);
int       DXGetModulePathLen();
int       DXGetModulePath(char *path);
int       _dxf_ExGetCurrentInstance();
Error     m__badfunc(Object *in, Object *out);

Error   DXCompareModuleMacroBase(Pointer id1, Pointer id2);
const char *DXGetModuleComponentName(Pointer id, int index);

int     _dxf_ExPathsEqual( ModPath *p1, ModPath *p2 );
void 	_dxf_ExPathCopy( ModPath *dest, ModPath *src );

char 	*_dxf_ExGFuncPathToString( gfunc *fnode );
char 	*_dxf_ExGFuncPathToCacheString( gfunc *fnode );
Error   _dxf_ModNameTablesInit();
char    *_dxf_ExPathToString( ModPath * );
int     DXGetModuleCacheStrLen();
int     DXGetModuleCacheStr(char *);


#define MAX_PATH_STR_LEN 1024
#define EX_PIN_PROC 1

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif	/* _GRAPH_H */
