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
#include "background.h"
#include "graph.h"
#include "graphqueue.h"
#include "log.h"
#include "distp.h"
#include "vcr.h"

typedef struct
{
    int		change;			/* variable has been changed	*/
    struct node	*tree;			/* parse tree to execute	*/
    Program	*graph;			/* pre-constructed `tree'	*/
} _background;

static	_background	background;

/*************************************************************************/

/*
 * Top level initialization of the background process by the executive.
 */

Error _dxf_ExInitBackground (void)
{
    int		size;

    size = sizeof (_background);

    memset (&background, 0, size);

    return (OK);
}


/*
 * Cleanup function.
 */

void _dxf_ExCleanupBackground (void)
{
}


/*
 * Main loop interface to see whether any background jobs need to be done.
 */

int _dxf_ExCheckBackground (EXDictionary dict, int multiProc)
{
    Program		*graph;

    if (background.tree == NULL)
	return (FALSE);
    
    if (background.graph == NULL &&
	background.change && 
	(multiProc || _dxf_ExGQAllDone())) {
	_dxf_ExGraphInit ();
	graph = _dxf_ExGraph (background.tree);
	if (graph)
	    graph->origin = GO_BACKGROUND;
	else
	    background.change = FALSE;
	background.graph = graph;
	ExDebug ("1", "Creating graph %x for background", graph);
    }


    if (background.graph && _dxf_ExGQAllDone())
    {
        /* we are the master, send a copy of the parse tree
         * to all slaves
         */
        _dxf_ExSendParseTree(background.tree);
	_dxf_ExGQEnqueue (background.graph);
	ExDebug ("1", "Enqueuing graph %x for background", background.graph);
	background.graph = NULL;
	background.change = FALSE;
	return (TRUE);
    }
    else 
	return (FALSE);
}


/*
 * Called whenever a variable changes in the global dictionary.
 */

void _dxf_ExBackgroundChange (void)
{
    _dxf_ExVCRChange();

    if (background.tree == NULL)
	return;

    background.change = TRUE;
}

void _dxf_ExBackgroundChangeReset (void)
{
    _dxf_ExVCRChangeReset();

    if (background.tree == NULL)
	return;

    background.change = FALSE;
}

/*
 * Called whenever a macro is redefined.
 */
void _dxf_ExBackgroundRedo (void)
{
    _dxf_ExVCRRedo();

    if (! background.tree)
	return;

    if (background.graph != NULL)
    {
	_dxf_ExGraphDelete(background.graph);
	background.graph = NULL;
    }

    background.change = TRUE;
}



/*
 * The background command processor.  Used to set up or cancel the
 * background graph.
 */

void _dxf_ExBackgroundCommand (_bg comm, struct node * n)
{
    struct node		*old = NULL;

    switch (comm)
    {
	case BG_STATEMENT:
            /* do not automatically execute graph once when put */
            /* in background mode, UI will take care of first execution */
	    background.change = FALSE;
	    old = background.tree; 
	    background.tree = n;
	    break;

	case BG_CANCEL:
	    background.change = FALSE;
	    old = background.tree; 
	    background.tree = NULL;
	    break;
	
	default:
	    DXWarning ("#4590", "Background", comm);
	    break;
    }
    if (background.graph)
	_dxf_ExGraphDelete(background.graph);
    background.graph = NULL;

    if (old)
	_dxf_ExPDestroyNode (old);
}
