/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#define MACRO_MODULES 1

#include <math.h>
#include "config.h"
#include "parse.h"
#include "graph.h"

/*
 * $$$$ Because of byte ordering the loop fails on the pvs
 * $$$$ look at this later.
 */

#define	MEMCPY(d,s,n)		memcpy (d, s, n)
#if 0
#define	MEMCPY(d,s,n)\
{\
    if (n & 3)\
	memcpy (d, s, n);\
    else\
    {\
	int	*__dst = (int *) d;\
	int	*__src = (int *) s;\
	int	__iter = n >> 1;\
	while (__iter--)\
	    *__dst++ = *__src++;\
    }\
}
#endif

#define	WITHIN_EPSILON(num,of,ep)	(fabs ((num) - (of)) <= (ep))

int _dxf_ExNode__Delete(node *n);

static PFIP
node_methods[] =
{
    _dxf_ExNode__Delete
};

#if 0
static void PrintNodeList(register node *n);
#endif

static node		ConstantHandle;

int GetNextMacroIndex()
{
    static int nextid = 0;
    nextid++;
    return(nextid);
}
    
/*
 * Create and initialize a parse tree node.  All nodes are created through
 * this routine.
 */
node *_dxf_ExPCreateNode (_ntype type)
{
    node	*n;

    n = (node *) _dxf_EXO_create_object_local (EXO_CLASS_FUNCTION,
					  sizeof (node),
					  node_methods);
    n->type = type;
    ExReference (n);
    return (n);
}

void _dxf_ExPDestroyNode (node *n)
{
    if (! n)
	return;

    ExDelete (n);
}

/*
 * This will actually delete an entire parse tree.  It chases down the
 * node links and deletes them.  Any new node types will have to be
 * added here.
 */

int _dxf_ExNode__Delete(register node *n)
{
    node	*next;
    node	*prev;

    if (n == NULL)
	return (0);

    switch (n->type)
    {
	case NT_MACRO:
	case NT_MODULE:
	    _dxf_ExPDestroyNode (n->v.function.id);
	    _dxf_ExPDestroyNode (n->v.function.in);
	    _dxf_ExPDestroyNode (n->v.function.out);
            if(n->type == NT_MACRO)
	        _dxf_ExPDestroyNode (n->v.function.def.stmt);
	    break;

	case NT_ASSIGNMENT:
	    _dxf_ExPDestroyNode (n->v.assign.lval);
	    _dxf_ExPDestroyNode (n->v.assign.rval);
	    break;

	case NT_PRINT:
	    _dxf_ExPDestroyNode (n->v.print.val);
	    break;

	case NT_ATTRIBUTE:
	    _dxf_ExPDestroyNode (n->v.attr.id);
	    _dxf_ExPDestroyNode (n->v.attr.val);
	    break;

	case NT_CALL:
	    _dxf_ExPDestroyNode (n->v.call.id);
	    _dxf_ExPDestroyNode (n->v.call.arg);
	    break;

	case NT_ARGUMENT:
	    _dxf_ExPDestroyNode (n->v.arg.id);
	    _dxf_ExPDestroyNode (n->v.arg.val);
	    break;

	case NT_ARITHMETIC:
	    _dxf_ExPDestroyNode (n->v.arith.lhs);
	    _dxf_ExPDestroyNode (n->v.arith.rhs);
	    break;

	case NT_LOGICAL:
	    _dxf_ExPDestroyNode (n->v.logic.lhs);
	    _dxf_ExPDestroyNode (n->v.logic.rhs);
	    break;

	case NT_CONSTANT:
	    next = n->v.constant.cnext;
	    prev = n->v.constant.cprev;
	    if(next) next->v.constant.cprev = prev;
	    if(prev) prev->v.constant.cnext = next;

	    if (n->v.constant.shape != n->v.constant.lshape)
		DXFree ((Pointer) n->v.constant.shape);
	    if (n->v.constant.data != n->v.constant.ldata)
		DXFree ((Pointer) n->v.constant.data);
	    if (n->v.constant.obj)
		DXDelete ((Object) n->v.constant.obj);
            if (n->v.constant.origin)
                DXFree((Pointer) n->v.constant.origin);
            if (n->v.constant.delta)
                DXFree((Pointer) n->v.constant.delta);

	    break;

	case NT_ID:
	    DXFree ((Pointer) n->v.id.id);
	    _dxf_ExPDestroyNode (n->v.id.dflt);
	    break;

	case NT_EXID:
	    DXFree ((Pointer) n->v.exid.id);
	    break;

	case NT_BACKGROUND:
	    _dxf_ExPDestroyNode (n->v.background.statement);
	    break;

	case NT_PACKET:
	    _dxf_ExPDestroyNode (n->v.packet.packet);
	    break;
	case NT_DATA: /* This was missing, can someone verify? */
		break;
    }

    _dxf_ExPDestroyNode (n->attr);
    _dxf_ExPDestroyNode (n->next);
    return (0);
}

/*
 * There is not a lot to tell about the following routines.  They are used
 * to create specific node types in the parse tree.  They are generally
 * very straight forward and are commented appropriately where they are
 * not.  See the node structure in parse.h for details about the members
 * in the nodes.
 */

node *_dxf_ExPCreateExid (char *id)
{
    node	*n;

    n = _dxf_ExPCreateNode (NT_EXID);
    n->v.exid.id = id;

    return (n);
}


node *_dxf_ExPCreateId (char *id)
{
    node	*n;

    n = _dxf_ExPCreateNode (NT_ID);
    n->v.id.id = id;

    return (n);
}


node *_dxf_ExPDotDotList (node *n1, node *n2, node *n3)
{
    node	*n	= NULL;
    int		isv=0, iev=0, idv=0;	/* int   start/end/delta values	*/
    float	fsv, fev, fdv;		/* float start/end/delta values	*/
    double	dsv, dev, ddv;		/* doubles for better precision	*/
    Type	st, et, dt, ot;		/* types			*/
    int		rev;			/* reverse direction		*/
    int		num=0;			/* element count		*/

    if (n1->v.constant.cat != CATEGORY_REAL ||
	n2->v.constant.cat != CATEGORY_REAL ||
	(n3 && n3->v.constant.cat != CATEGORY_REAL))
    {
	_dxd_exParseMesg = "semantic error:  invalid category";
	goto error;
    }

    if (n1->v.constant.rank != 0 ||
	n2->v.constant.rank != 0 ||
	(n3 && n3->v.constant.rank != 0))
    {
	_dxd_exParseMesg = "semantic error:  invalid rank";
	goto error;
    }

    /*
     * For now only handle ints and floats.  If we add other types that
     * can be directly read by the rule 'real' then we need to do some
     * more work here.
     */

    st = n1->v.constant.type;
    et = n2->v.constant.type;
    dt = n3 ? n3->v.constant.type : st;
    if ((st != TYPE_INT && st != TYPE_FLOAT) ||
	(et != TYPE_INT && et != TYPE_FLOAT) ||
	(dt != TYPE_INT && dt != TYPE_FLOAT))
    {
	_dxd_exParseMesg = "semantic error:  invalid type";
	goto error;
    }

    if (st == TYPE_INT)
    {
	isv = * (int   *) n1->v.constant.data;
	fsv = (float) isv;
    }
    else
	fsv = * (float *) n1->v.constant.data;

    if (et == TYPE_INT)
    {
	iev = * (int   *) n2->v.constant.data;
	fev = (float) iev;
    }
    else
	fev = * (float *) n2->v.constant.data;

    if (n3)
    {
	if (dt == TYPE_INT)
	{
	    idv = * (int   *) n3->v.constant.data;
	    fdv = (float) idv;
	}
	else
	    fdv = * (float *) n3->v.constant.data;
    }
    else
    {
	idv = 1;
	fdv = (float) 1.0;
    }

    if (st == TYPE_FLOAT || et == TYPE_FLOAT || dt == TYPE_FLOAT)
	ot = TYPE_FLOAT;
    else
	ot = TYPE_INT;

    /*
     * In addition to extracting the endpoint values, we also  determine
     * whether we need to count up or down.  Finally, we make sure that
     * the delta value is positive since we will either add or subtract
     * is as is appropriate for the counting direction.
     */

    switch (ot)
    {
	case TYPE_INT:
	    rev = (isv > iev);
	    idv = idv < 0 ? -idv : idv;
	    if (idv == 0)
	    {
		_dxd_exParseMesg = "semantic error:  invalid list delta of 0";
		goto error;
	    }
	    num = (rev ? isv - iev : iev - isv) / idv;
	    idv = rev ? -idv : idv;
	    num++;
	    break;

	case TYPE_FLOAT:
	    dsv = (double) fsv;
	    dev = (double) fev;
	    ddv = (double) fdv;
	    rev = (dsv > dev);
	    ddv = ddv < 0 ? -ddv : ddv;
	    if (ddv == 0.0)
	    {
		_dxd_exParseMesg = "semantic error:  invalid list delta of 0";
		goto error;
	    }
	    num = (rev ? dsv - dev : dev - dsv) / ddv;
	    ddv = rev ? -ddv : ddv;
	    num++;
	    break;
        default:   /* Fix if ever allow other position types. */
           break;
    }

    n = _dxf_ExPCreateConst (ot, CATEGORY_REAL, 0, NULL);
    if (n == NULL)
	goto error;

    switch (ot)
    {
        case TYPE_INT:
            n->v.constant.origin = (int *)DXAllocate(sizeof(int));
            n->v.constant.delta = (int *)DXAllocate(sizeof(int));
            *(int *)(n->v.constant.origin) = isv;
            *(int *)(n->v.constant.delta) = idv;
            break;
               
        case TYPE_FLOAT:
            n->v.constant.origin = (float *)DXAllocate(sizeof(float));
            n->v.constant.delta = (float *)DXAllocate(sizeof(float));
            *(float *)(n->v.constant.origin) = fsv;
            *(float *)(n->v.constant.delta) = fdv;
            break;
        default:   /* Fix if ever allow other position types. */
	    break;
    }

    n->v.constant.items = num;
    return (n);

error:
    _dxd_exParseError = TRUE;
    return (NULL);
}


node *_dxf_ExPCreateArith (_op op, node *lhs, node *rhs)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_ARITHMETIC);

    n->v.arith.op  = op;
    n->v.arith.lhs = lhs;
    n->v.arith.rhs = rhs;

    return (n);
}


node *_dxf_ExPCreateLogical (_op op, node *lhs, node *rhs)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_LOGICAL);

    n->v.logic.op  = op;
    n->v.logic.lhs = lhs;
    n->v.logic.rhs = rhs;

    return (n);
}


node *_dxf_ExPCreateArgument (node *id, node *val)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_ARGUMENT);

    n->v.arg.id  = id;
    n->v.arg.val = val;

    return (n);
}


node *_dxf_ExPCreateCall (node *id, node *arg)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_CALL);

    n->v.call.id  = id;
    n->v.call.arg = arg;

    return (n);
}


node *_dxf_ExPCreateAttribute (node *id, node *val)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_ATTRIBUTE);

    n->v.attr.id  = id;
    n->v.attr.val = val;

    return (n);
}


node *_dxf_ExPCreatePrint (_ptype type, node *val)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_PRINT);

    n->v.print.type = type;
    n->v.print.val  = val;

    return (n);
}


node *_dxf_ExPCreateAssign (node *lval, node *rval)
{
    node	*n;
    
    n = _dxf_ExPCreateNode (NT_ASSIGNMENT);

    n->v.assign.lval = lval;
    n->v.assign.rval = rval;

    return (n);
}

#define LIST_PREPEND(list, node)	\
{					\
    node->next = list;			\
    node->last = list->last;		\
    node->prev = NULL;			\
					\
    list->prev = node;			\
    list = node;			\
}

node *_dxf_ExPCreateMacro (node *id, node *in, node *out, node *stmt)
{
    node	*n;
    node	*p;
    int		nin;
    int		nout;
    
    n = _dxf_ExPCreateNode (NT_MACRO);

    n->v.macro.id   = id;

    /*
     * count the number of inputs
     */
    for (p = in, nin = 0; p; p = p->next)
	nin++;

    n->v.macro.in   = in;
    n->v.macro.nin  = nin;

#if MACRO_MODULES
    /*
     * if there are any inputs, create a MacroStart module
     * for them and make it the first in the stmt list
     */
    if (nin)
    {
	node *ms_stmt, *ms_call, *msin, *msout, *newid, *newarg; 
        node *cacheid, *newval, *newattr;
	char *str;
        int cache_attr = CACHE_OFF;

	newid = _dxf_ExPCreateConst(TYPE_INT,
				CATEGORY_REAL, 1, (Pointer)&nin);
	msin = _dxf_ExPCreateArgument(NULL, newid);

	LIST_CREATE(msin);
	msout = NULL;

	for (p = in; p; p = p->next)
	{
	    if (p->type == NT_ID)
	    {
		str = DXAllocate(strlen(p->v.id.id) + 1);
		strcpy(str, p->v.id.id);
		newid = _dxf_ExPCreateId(str);
		newarg = _dxf_ExPCreateArgument(NULL, newid);
		LIST_APPEND(msin, newarg);

		str = DXAllocate(strlen(p->v.id.id) + 1);
		strcpy(str, p->v.id.id);
		newid = _dxf_ExPCreateId(str);
                
		if (msout)
		{
		    LIST_APPEND(msout, newid);
		}
		else
		{
		    msout = newid;
		    LIST_CREATE(msout);
		}
	    }
	    else
		_dxf_ExDie ("non-ID node found in input args");
	    
	}

	str = DXAllocate(strlen("MacroStart") + 1);
	strcpy(str, "MacroStart");
	ms_call = _dxf_ExPCreateCall(_dxf_ExPCreateId(str), msin);

	str = DXAllocate(strlen("cache") + 1);
	strcpy(str, "cache");
	cacheid = _dxf_ExPCreateId(str);
	newval = _dxf_ExPCreateConst(TYPE_INT,
			CATEGORY_REAL, 1, (Pointer)&cache_attr);
	newattr = _dxf_ExPCreateAttribute (cacheid, newval);
	ms_call->attr = newattr;

	ms_stmt = _dxf_ExPCreateAssign(msout, ms_call);

	if (stmt)
	{
	    LIST_PREPEND(stmt, ms_stmt);
	}
	else
	{
	    LIST_CREATE(ms_stmt);
	    stmt = ms_stmt;
	}
    }
#endif

    /*
     * count number of outputs
     */
    for (p = out, nout = 0; p; p = p->next)
	nout++;

    n->v.macro.out  = out;
    n->v.macro.nout = nout;

#if MACRO_MODULES
    /*
     * if there are any outputs, create a MacroEnd module
     * for them and make it the last in the stmt list
     */
    if (nout)
    {
	node *me_stmt, *me_call, *mein, *meout, *newid, *newarg;  
        node *cacheid, *newval, *newattr;
	char *str;
        int cache_attr = CACHE_OFF;

	newid = _dxf_ExPCreateConst(TYPE_INT,
				CATEGORY_REAL, 1, (Pointer)&nout);
	mein = _dxf_ExPCreateArgument(NULL, newid);

	LIST_CREATE(mein);
	meout = NULL;

	for (p = out; p; p = p->next)
	{
	    if (p->type == NT_ID)
	    {
		str = DXAllocate(strlen(p->v.id.id) + 1);
		strcpy(str, p->v.id.id);
		newid = _dxf_ExPCreateId(str);
		newarg = _dxf_ExPCreateArgument(NULL, newid);
		LIST_APPEND(mein, newarg);

		str = DXAllocate(strlen(p->v.id.id) + 1);
		strcpy(str, p->v.id.id);
		newid = _dxf_ExPCreateId(str);
		if (meout)
		{
		    LIST_APPEND(meout, newid);
		}
		else
		{
		    meout = newid;
		    LIST_CREATE(meout);
		}
	    }
	    else
		_dxf_ExDie ("non-ID node found in input args");
	    
	}

	str = DXAllocate(strlen("MacroEnd") + 1);
	strcpy(str, "MacroEnd");
	me_call = _dxf_ExPCreateCall(_dxf_ExPCreateId(str), mein);

	str = DXAllocate(strlen("cache") + 1);
	strcpy(str, "cache");
	cacheid = _dxf_ExPCreateId(str);
	newval = _dxf_ExPCreateConst(TYPE_INT,
			CATEGORY_REAL, 1, (Pointer)&cache_attr);
	newattr = _dxf_ExPCreateAttribute (cacheid, newval);
	me_call->attr = newattr;

	me_stmt = _dxf_ExPCreateAssign(meout, me_call);

	if (stmt)
	{
	    LIST_APPEND(stmt, me_stmt);
	}
	else
	{
	    LIST_CREATE(me_stmt);
	    stmt = me_stmt;
	}
    }
#endif

    n->v.macro.def.stmt = stmt;

    /* hidden inputs (number before and after normal inputs) */
    n->v.macro.prehidden = n->v.macro.posthidden = 0;
    n->v.macro.index = GetNextMacroIndex();
    return (n);
}

node *_dxf_ExPCreateBackground (int handle, _bg type, node *stmt)
{
    node *n;

    n = _dxf_ExPCreateNode(NT_BACKGROUND);

    n->v.background.type = type;
    n->v.background.handle = handle;
    n->v.background.statement = stmt;
    return(n);
}

node *_dxf_ExPCreatePacket (_pack type, int number, int size, node *packet)
{
    node *n;

    n = _dxf_ExPCreateNode(NT_PACKET);

    n->v.packet.type = type;
    n->v.packet.number = number;
    n->v.packet.size = size;
    n->v.packet.packet = packet;
    return(n);
}

node *_dxf_ExPCreateData (Pointer data, int len)
{
    node *n;

    n = _dxf_ExPCreateNode(NT_DATA);
    n->v.data.data = data;
    n->v.data.len = len;
    return(n);
}


/*
 * New routines to deal with constants.  Basically we defer on creating
 * the real Array objects until the last possible moment since if we
 * can control the structure we can append lists and change types and
 * structures far more easily and without incurring the time penalties
 * associated with creating and deleting objects.
 */

void
_dxf_ExEvaluateConstants (int eval)
{
    node	*n;

    if (eval)
    {
	for (n = ConstantHandle.v.constant.cnext;
	     n != &ConstantHandle;
	     n = n->v.constant.cnext)
	    _dxf_ExEvalConstant (n);
    }
    else
    {
	n = &ConstantHandle;
	ConstantHandle.v.constant.cnext = n;
	ConstantHandle.v.constant.cprev = n;
    }
}


void
_dxf_ExEvalConstant (node *n)
{
    Array	arr;
    RegularArray regarr;
    String	str;
    Object	obj=NULL;
    int		err	= FALSE;
    int		i;
    int		nstrs;
    int		strstart;
    char	*cp;
    char	**strlist;

    if (n->v.constant.type == TYPE_UBYTE)
    {
	cp = (char *) n->v.constant.data;
	for (i = 0, nstrs = 0; i < n->v.constant.items; i++)
	    if (cp[i] == (char) NULL)
		nstrs++;

	if (nstrs <= 1)
	{
	    str = DXNewString ((char *) n->v.constant.data);
	    if (str == NULL)
		err = TRUE;
	    obj = (Object) str;
	}
	else
	{
	    strlist = (char **) DXAllocate((nstrs+1) * sizeof(char *));
	    if (strlist == NULL)
	    {
		err = TRUE;
		goto error;
	    }

	    strstart = TRUE;
	    for (i = 0, nstrs = 0; i < n->v.constant.items; i++) 
	    {
		if (strstart == TRUE)
		{
		    strlist[nstrs++] = cp+i;
		    strstart = FALSE;
		}
		if (cp[i] == (char) NULL)
		    strstart = TRUE;
	    }

	    arr = DXMakeStringListV(nstrs, strlist);
	    if (arr == NULL)
		err = TRUE;

	    DXFree ((Pointer) strlist);
	    obj = (Object) arr;
	}
    }
    else
    {
        /* check first to see if it's a regular array */
        if(n->v.constant.origin && n->v.constant.delta) { 
	    regarr = DXNewRegularArray (n->v.constant.type, 1,   
                                     n->v.constant.items,
	                             n->v.constant.origin,
		                     n->v.constant.delta);
	    obj = (Object) regarr;
        }
        else {
	    arr = DXNewArrayV (n->v.constant.type,
			   n->v.constant.cat,
			   n->v.constant.rank,
			   n->v.constant.shape);
	    if (arr == NULL || ! DXAddArrayData (arr, 0, n->v.constant.items,
					     n->v.constant.data))
	        err = TRUE;
	    obj = (Object) arr;
        }
    }

  error:
    if (err)
	_dxf_ExDie ("can't evaluate constant");
    
    n->v.constant.obj = (Array) DXReference (obj);
}

/*
 * Creates a constant node whose object is an array of the given
 * type and category, and which contains the given data.
 */

node *_dxf_ExPCreateConst (Type type, Category category, int cnt, Pointer p)
{
    node	*n;
    node	*next;
    int		size;
    Pointer	data;
    
    next = ConstantHandle.v.constant.cnext;

    n = _dxf_ExPCreateNode (NT_CONSTANT);

    n->v.constant.cnext  = next;
    n->v.constant.cprev  = &ConstantHandle;
    ConstantHandle.v.constant.cnext = n;
    next->v.constant.cprev = n;

    n->v.constant.items  = cnt;
    n->v.constant.type   = type;
    n->v.constant.cat    = category;
    n->v.constant.nalloc = LOCAL_DATA;
    n->v.constant.salloc = LOCAL_SHAPE;
    n->v.constant.shape  = n->v.constant.lshape;
    n->v.constant.data   = (Pointer) n->v.constant.ldata;
    n->v.constant.origin = NULL;
    n->v.constant.delta = NULL;

    size = cnt * DXTypeSize (type) * DXCategorySize (category);

    if (size > LOCAL_DATA)
    {
	data = DXAllocateLocal (size);
	if (data == NULL)
	    _dxf_ExDie ("can't create constant");
	n->v.constant.nalloc = size;
	n->v.constant.data   = data;
    }

    if (p)
    {
	MEMCPY (n->v.constant.data, p, size);
	n->v.constant.nused = size;
    }

    return (n);
}


/*ZZZZZ*/
node *_dxf_ExPNegateConst (node *n)
{
    int		count;
    int		rank;
    int		i;
    int		*ival;
    float	*fval;
    double	*dval;

    if (n == NULL)
	return (n);

    /* check first for a regular array */
    if(n->v.constant.origin && n->v.constant.delta) {
        switch (n->v.constant.type)
        {
          case TYPE_INT:
              *(int *)(n->v.constant.origin) *= -1;
              *(int *)(n->v.constant.delta) *= -1;
              break; 
          case TYPE_FLOAT:
              *(float *)(n->v.constant.origin) *= (float) -1.0;
              *(float *)(n->v.constant.delta) *= (float) -1.0;
              break;
	  default: /* Fix if allow other position types. */
	      break;
        }
    }
    else {
        count = n->v.constant.items * DXCategorySize (n->v.constant.cat);
        rank  = n->v.constant.rank;
        for (i = 0; i < rank; i++)
	    count *= n->v.constant.shape[i];

        switch (n->v.constant.type)
        {
	  case TYPE_INT:
	      ival = (int *) n->v.constant.data;
	      for (i = 0; i < count; i++)
		  *ival++ *= -1;
	      break;

	  case TYPE_FLOAT:
	      fval = (float *) n->v.constant.data;
	      for (i = 0; i < count; i++)
	          *fval++ *= (float) -1.0;
	      break;

	  case TYPE_DOUBLE:
	      dval = (double *) n->v.constant.data;
	      for (i = 0; i < count; i++)
		  *dval++ *= -1.0;
	      break;

	  default:
	      break;
        }
    }

    return (n);
}

/*
 * Increases the dimensionality of the data
 *
 */

node *_dxf_ExPExtendConst (node *n)
{
    int		*shape;
    int		rank;
    int		i;

    if (n == NULL)
	return (n);

    rank = n->v.constant.rank;
    if (rank == n->v.constant.salloc)
    {
	shape = (int *) DXAllocateLocal ((rank << 1) * sizeof (int));
	if (shape == NULL)
	    _dxf_ExDie ("can't extend constant");
	MEMCPY (shape, n->v.constant.shape, rank * sizeof (int));
	if (n->v.constant.shape != n->v.constant.lshape)
	    DXFree ((Pointer) n->v.constant.shape);
	n->v.constant.shape = shape;
	n->v.constant.salloc = rank << 1;
    }

    shape = n->v.constant.shape;

    for (i = rank; i; i--)
	shape[i] = shape[i - 1];
    shape[0] = n->v.constant.items;
    n->v.constant.items = 1;
    n->v.constant.rank++;
	
    return (n);
}

/*
 * Appends the singleton element to the end of the list if the types match.
 */

node *_dxf_ExAppendConst (node *list, node *elem)
{
    Type	lt, et;
    Category	lc;
    char	*ld, *ed;
    int		n;
    int		i;
    int		*iptr;
    float	*fptr;
    int		rank;
    int		*ls, *es;
    int		size;
    int		used;
    int		have;

    /*
     * First make the data types match.
     *
     * NOTE:  	This currently takes advantage of the fact that
     *		sizeof (int) == sizeof (float).  If this is not indeed the
     *		case then this section will have to be changed.
     */

    lt = list->v.constant.type;
    et = elem->v.constant.type;

    if (lt != et)
    {
	if (lt == TYPE_INT && et == TYPE_FLOAT)
	{
	    n = list->v.constant.nused / sizeof (int);
	    iptr = (int   *) list->v.constant.data;
	    fptr = (float *) list->v.constant.data;
	    for (i = 0; i < n; i++)
		*fptr++ = (float) *iptr++;
	    lt = list->v.constant.type = TYPE_FLOAT;
	}
	else if (lt == TYPE_FLOAT && et == TYPE_INT)
	{
	    n = elem->v.constant.nused / sizeof (int);
	    iptr = (int   *) elem->v.constant.data;
	    fptr = (float *) elem->v.constant.data;
	    for (i = 0; i < n; i++)
		*fptr++ = (float) *iptr++;
	    et = elem->v.constant.type = TYPE_FLOAT;
	}
	else
	{
	    _dxd_exParseMesg = "semantic error:  type mismatch";
	    goto error;
	}
    }

    /*
     * Now verify that the category, rank and shape of the elements in the
     * list match that of the singleton element being appended.
     */

    lc = list->v.constant.cat;
    if (lc != elem->v.constant.cat)
    {
	_dxd_exParseMesg = "semantic error:  category mismatch";
	goto error;
    }

    rank = list->v.constant.rank;
    if (rank != elem->v.constant.rank)
    {
	_dxd_exParseMesg = "semantic error:  rank mismatch";
	goto error;
    }

    ls = list->v.constant.shape;
    es = elem->v.constant.shape;

    for (i = 0; i < rank; i++)
    {
	if (*ls++ != *es++)
	{
	    _dxd_exParseMesg = "semantic error:  shape mismatch";
	    goto error;
	}
    }

    /*
     * Check to make sure that there is enough space to add it.  DXAdd
     * more if necessary.
     */

    size = elem->v.constant.nused;
    used = list->v.constant.nused;
    have = list->v.constant.nalloc;
    if (size + used > have)
    {
	n = (size + used) << 1;
	ld = (char *)DXAllocateLocal (n);
	if (ld == NULL)
	{
	    _dxd_exParseMesg = "can't append constant";
	    goto error;
	}
	MEMCPY (ld, list->v.constant.data, used);
	if (list->v.constant.data != list->v.constant.ldata)
	    DXFree ((Pointer) list->v.constant.data);
	list->v.constant.data = (Pointer) ld;
	list->v.constant.nalloc = n;
    }

    /*
     * Append the singleton element to the list.
     */
    
    ld  = (char *) list->v.constant.data;
    ld += used;
    ed  = (char *) elem->v.constant.data;
    MEMCPY (ld, ed, size);
    list->v.constant.items += elem->v.constant.items;
    list->v.constant.nused += size;

ok:
    return (list);

error:
    _dxd_exParseError = TRUE;
    goto ok;
}

#if 0
static void
PrintNodeList(register node *n)
{
    node	*next;
    node	*prev;

    if (n == NULL)
	return;

    switch (n->type)
    {
	case NT_MACRO:
	    fprintf(stderr, "0x%x macro %s\n", n, n->v.function.id->v.id.id);
	    break;

	case NT_MODULE:
	    fprintf(stderr, "0x%x module %s\n", n, n->v.function.id->v.id.id);
	    break;

	case NT_ASSIGNMENT:
	    if (n->v.assign.rval->type == NT_CALL)
		fprintf(stderr, "0x%x function assignment: %s\n", n,
			n->v.assign.rval->v.call.id->v.id.id);
	    else
		fprintf(stderr, "0x%x non-function assignment\n", n);
	    break;

	case NT_PRINT:
	    fprintf(stderr, "0x%x print\n", n);
	    break;

	case NT_ATTRIBUTE:
	    fprintf(stderr, "0x%x attribute\n", n);
	    break;

	case NT_CALL:
	    fprintf(stderr, "0x%x call %s\n", n, n->v.call.id->v.id.id);
	    break;

	case NT_ARGUMENT:
	    fprintf(stderr, "0x%x argument\n", n);
	    break;

	case NT_ARITHMETIC:
	    fprintf(stderr, "0x%x arithmetic\n", n);
	    break;

	case NT_LOGICAL:
	    fprintf(stderr, "0x%x logical\n", n);
	    break;

	case NT_CONSTANT:
	    fprintf(stderr, "0x%x constant\n", n);
	    break;

	case NT_ID:
	    fprintf(stderr, "0x%x id %s\n", n, n->v.id.id);
	    break;

	case NT_EXID:
	    fprintf(stderr, "0x%x exid %s\n", n, n->v.exid.id);
	    DXFree ((Pointer) n->v.exid.id);
	    break;

	case NT_BACKGROUND:
	    fprintf(stderr, "0x%x background\n", n);
	    break;

	case NT_PACKET:
	    fprintf(stderr, "0x%x packet\n", n);
	    break;
    }

    PrintNodeList (n->next);
}
#endif
