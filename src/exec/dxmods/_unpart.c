/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_unpart.c,v 1.5 2006/01/03 17:02:22 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif
 
#include "unpart.h"
 
#define	GAC(_x)		DXGetArrayClass ((Array) _x)
#define	GGC(_x)		DXGetGroupClass ((Group) _x)
#define	GOC(_x)		DXGetObjectClass ((Object) _x)
 

static int
GetGroupMemberCount (Group g)
{
    int		mem	= 0;
 
    while (DXGetEnumeratedMember (g, mem, NULL))
	mem++;
    return (mem);
}
 

Error
_dxfGetFieldInformation (FieldInfo *info)
{
    Object		f	= info->obj;
    FieldInfo		local;
    FieldInfo		finfo;
    int			i, j;
    int			e, s, de, ds;
    int			items;
    int			rank;
    int			members;
    int			shape[MAXDIMS];
    Object		dep;
    char		*str;
    Error		ret	= ERROR;

    memset (&local, 0, sizeof (local));

    local.obj   = f;

    local.class = DXGetObjectClass ((Object) f);
    if (local.class == CLASS_GROUP)
	local.class = DXGetGroupClass ((Group) f);
    if (local.class != CLASS_FIELD && local.class != CLASS_COMPOSITEFIELD)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10191", "object");
	goto cleanup;
    }

    if (local.class == CLASS_FIELD)
    {
	if (DXEmptyField ((Field) f))
	{
	    ret = OK;
	    goto cleanup;
	}

	local.data = (Array) DXGetComponentValue ((Field) f, "data");
	local.con  = (Array) DXGetComponentValue ((Field) f, "connections");
	local.pos  = (Array) DXGetComponentValue ((Field) f, "positions");
	if (local.data == NULL || local.con == NULL || local.pos == NULL)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10240",
		      "data, positions, or connections");
	    goto cleanup;
	}

	/*
	 * Get information about what each item in the data array
	 * looks like and compute how many elements each item has.
	 */

	if (! DXGetArrayInfo (local.data, &items, &local.type, &local.cat,
			    &rank, shape))
	    goto cleanup;
	
	local.epi = 1;
	for (i = 0; i < rank; i++)
	    local.epi *= shape[i];

	/*
	 * Get the dimensioning information that describes how the
	 * data is arranged and what it depends on.
	 */

	local.dep = DEP_OTHER;
	dep = DXGetAttribute ((Object) local.data, "dep");
	if (! dep)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "missing data dependency");
	    goto cleanup;
	}
	dep = DXExtractString (dep, &str);
	if (dep && str)
	{
	    if (! strcmp (str, "positions"))
		local.dep = DEP_POSITIONS;
	    else if (! strcmp (str, "connections"))
		local.dep = DEP_CONNECTIONS;
	}
	if (local.dep == DEP_OTHER)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#11250", "data");
	    goto cleanup;
	}

	/*
	 * OK, get the count in each dimension
	 */

	if (GOC (local.con) != CLASS_ARRAY ||
	    (GAC (local.con) != CLASS_MESHARRAY &&
	     GAC (local.con) != CLASS_PATHARRAY) ||
	    ! DXQueryGridConnections (local.con, &local.ndims, local.counts))
	{
	    DXSetError (ERROR_BAD_PARAMETER, "irregular connections");
	    goto cleanup;
	}
 
	/*
	 * This tells us where each partition's origin is.
	 */

	if (! DXGetMeshOffsets ((MeshArray) local.con, local.origin))
	{
	    DXSetError (ERROR_BAD_PARAMETER, "missing offsets");
	    goto cleanup;
	}

	/*
	 * Decrement the actual number of data values represented by one
	 * when dependent on connections since the data values refer to
	 * the cells NOT the vertices.  And the connections' counts always
	 * refer to the vertices.
	 */

	if (local.dep == DEP_CONNECTIONS)
	{
	    for (i = 0; i < local.ndims; i++)
		local.counts[i]--;
	}
    }
    else	/* local.class == CLASS_COMPOSITEFIELD			*/
    {
	members = GetGroupMemberCount ((Group) f);
	for (i = 0; i < members; i++)
	{
	    finfo.obj = DXGetEnumeratedMember ((Group) f, i, NULL);
	    if (! _dxfGetFieldInformation (&finfo))
		goto cleanup;
	    
	    if (i == 0)
	    {
		local = finfo;
		local.class = CLASS_COMPOSITEFIELD;
		local.members = members;
		local.data = local.con = local.pos = NULL;
		continue;
	    }

	    if (local.type  != finfo.type  ||
		local.cat   != finfo.cat   ||
		local.epi   != finfo.epi   ||
		local.ndims != finfo.ndims)
	    {
		DXSetError (ERROR_BAD_PARAMETER, "data mismatch");
		goto cleanup;
	    }

	    for (j = 0; j < local.ndims; j++)
	    {
		s  = local.origin[j];
		e  = s + local.counts[j];
		ds = s - finfo.origin[j];
		de = finfo.origin[j] + finfo.counts[j] - e;
		if (ds > 0)
		    s -= ds;
		if (de > 0)
		    e += de;
		local.origin[j] = s;
		local.counts[j] = e - s;
	    }
	}
    }

    *info = local;
    ret = OK;

cleanup:
    return (ret);
}


#define	ST_B	0x10000000
#define	ST_UB	0x20000000
#define	ST_S	0x40000000
#define	ST_US	0x80000000
#define	ST_I	0x01000000
#define	ST_UI	0x02000000
#define	ST_H	0x04000000
#define	ST_F	0x08000000
#define	ST_D	0x00100000

#define	SC_R	0x00010000
#define	SC_C	0x00020000
#define	SC_Q	0x00040000

#define	DT_B	0x00001000
#define	DT_UB	0x00002000
#define	DT_S	0x00004000
#define	DT_US	0x00008000
#define	DT_I	0x00000100
#define	DT_UI	0x00000200
#define	DT_H	0x00000400
#define	DT_F	0x00000800
#define	DT_D	0x00000010

#define	DC_R	0x00000001
#define	DC_C	0x00000002
#define	DC_Q	0x00000004


static int
SetTodoMask (FieldInfo *src, FieldInfo *dst)
{
    int		todo	= 0;

    switch (src->type)
    {
	case TYPE_BYTE:			todo |= ST_B;  break;
	case TYPE_UBYTE:		todo |= ST_UB; break;
	case TYPE_SHORT:		todo |= ST_S;  break;
	case TYPE_USHORT:		todo |= ST_US; break;
	case TYPE_INT:			todo |= ST_I;  break;
	case TYPE_UINT:			todo |= ST_UI; break;
	case TYPE_HYPER:		todo |= ST_H;  break;
	case TYPE_FLOAT:		todo |= ST_F;  break;
	case TYPE_DOUBLE:		todo |= ST_D;  break;
        default: break;
    }

    switch (src->cat)
    {
	case CATEGORY_REAL:		todo |= SC_R; break;
	case CATEGORY_COMPLEX:		todo |= SC_C; break;
	case CATEGORY_QUATERNION:	todo |= SC_C; break;
    }

    switch (dst->type)
    {
	case TYPE_BYTE:			todo |= DT_B;  break;
	case TYPE_UBYTE:		todo |= DT_UB; break;
	case TYPE_SHORT:		todo |= DT_S;  break;
	case TYPE_USHORT:		todo |= DT_US; break;
	case TYPE_INT:			todo |= DT_I;  break;
	case TYPE_UINT:			todo |= DT_UI; break;
	case TYPE_HYPER:		todo |= DT_H;  break;
	case TYPE_FLOAT:		todo |= DT_F;  break;
	case TYPE_DOUBLE:		todo |= DT_D;  break;
        default: break;
    }

    switch (dst->cat)
    {
	case CATEGORY_REAL:		todo |= DC_R; break;
	case CATEGORY_COMPLEX:		todo |= DC_C; break;
	case CATEGORY_QUATERNION:	todo |= DC_Q; break;
    }

    return (todo);
}


#define	RR_INNER(_dtype)\
{\
    *(dptr    ) = (_dtype) *(sptr    );\
}

#define	RC_INNER(_dtype)\
{\
    *(dptr    ) = (_dtype) *(sptr    );\
    *(dptr + 1) = (_dtype) 0;\
}

#define	CC_INNER(_dtype)\
{\
    *(dptr    ) = (_dtype) *(sptr    );\
    *(dptr + 1) = (_dtype) *(sptr + 1);\
}


/*
 * $$$$$ For now we are limited to, and in fact expect, 3 dimensions.
 * $$$$$$ ADD GENERAL LOOPING MECHANISM HERE $$$$$$
 */

#define	CLOOP_BODY(_stype,_dtype,_inner)\
{\
    int		i, in, di;\
    int		j, jn, dj;\
    int		k, kn, dk;\
    _stype	*sptr	= ((_stype *) sbase) + element*DXCategorySize(src->cat);\
    _dtype	*dptr;\
\
    in = src->counts[0];\
    jn = src->counts[1];\
    kn = src->counts[2];\
    di = ddelta[1] * (dst->counts[1] - src->counts[1]);\
    dj = ddelta[2] * (dst->counts[2] - src->counts[2]);\
    dk = ddelta[2];\
    dptr  = (_dtype *) dbase;\
    dptr += ddelta[0] * origin[0];\
    dptr += ddelta[1] * origin[1];\
    dptr += ddelta[2] * origin[2];\
    for (i = 0; i < in; i++)\
    {\
	for (j = 0; j < jn; j++)\
	{\
	    for (k = 0; k < kn; k++)\
	    {\
		_inner;\
		sptr += sdelta;\
		dptr += dk;\
	    }\
	    dptr += dj;\
	}\
	dptr += di;\
    }\
}

#define	ELOOP_BODY(_stype,_dtype,_inner)\
{\
    int		i, in, di;\
    int		j, jn, dj;\
    int		k, kn, dk;\
    _stype	*sptr;\
    _dtype	*dptr	= ((_dtype *) dbase) + element*DXCategorySize(dst->cat);\
\
    in = dst->counts[0];\
    jn = dst->counts[1];\
    kn = dst->counts[2];\
    di = sdelta[1] * (src->counts[1] - dst->counts[1]);\
    dj = sdelta[2] * (src->counts[2] - dst->counts[2]);\
    dk = sdelta[2];\
    sptr  = (_stype *) sbase;\
    sptr += sdelta[0] * origin[0];\
    sptr += sdelta[1] * origin[1];\
    sptr += sdelta[2] * origin[2];\
    for (i = 0; i < in; i++)\
    {\
	for (j = 0; j < jn; j++)\
	{\
	    for (k = 0; k < kn; k++)\
	    {\
		_inner;\
		dptr += ddelta;\
		sptr += dk;\
	    }\
	    sptr += dj;\
	}\
	sptr += di;\
    }\
}

#define	RR_LOOP(_stype,_dtype)	CLOOP_BODY(_stype,_dtype,RR_INNER(_dtype))
#define	RC_LOOP(_stype,_dtype)	CLOOP_BODY(_stype,_dtype,RC_INNER(_dtype))
#define	CC_LOOP(_stype,_dtype)	CLOOP_BODY(_stype,_dtype,CC_INNER(_dtype))

/*
 * $$$$$ For now the inputs "me" and "total" are not used and each
 * $$$$$ field is transfered independently.  In the future this will
 * $$$$$ be used to parallelize the collection/aggregation of single
 * $$$$$ fields and partitions where there are not enough partitions
 * $$$$$ to satisfy all of the processors.
 */

Error
_dxfCoalesceFieldElement (FieldInfo *src, FieldInfo *dst,
		      int element, int me, int total)
{
    int			d;
    int			i, in;
    Pointer		sbase;
    Pointer		dbase;
    int			sdelta;
    int			ddelta[MAXDIMS];
    int			origin[MAXDIMS];
    Error		ret	= ERROR;

    /* $$$$$$$ SEE ABOVE re: 3 dimensions $$$$$$$ */
    if (src->ndims != 3)
    {
	DXSetError (ERROR_NOT_IMPLEMENTED, "must be 3 dimensional");
	goto cleanup;
    }

    /*
     * Compute how far apart the elements in the source and destination are.
     * While we're at it compute the real offset (since the origin of the
     * space may not be at [0,0,...,0]) between the source and destination.
     */

    sdelta = DXCategorySize (src->cat) * src->epi;

    d = DXCategorySize (dst->cat);
    for (i = dst->ndims; i--; )
    {
	ddelta[i] = d;
	d *= dst->counts[i];
    }

    for (i = 0, in = src->ndims; i < in; i++)
	origin[i] = src->origin[i] - dst->origin[i];

    sbase = DXGetArrayData (src->data);
    dbase = DXGetArrayData (dst->data);

    switch (SetTodoMask (src, dst))
    {
	/* the popular cases first */
	case ST_B  | SC_R | DT_F | DC_R:	RR_LOOP (byte,float);	break;
	case ST_UB | SC_R | DT_F | DC_R:	RR_LOOP (ubyte,float);	break;
	case ST_S  | SC_R | DT_F | DC_R:	RR_LOOP (short,float);	break;
	case ST_US | SC_R | DT_F | DC_R:	RR_LOOP (ushort,float);	break;
	case ST_I  | SC_R | DT_F | DC_R:	RR_LOOP (int,float);	break;
	case ST_UI | SC_R | DT_F | DC_R:	RR_LOOP (uint,float);	break;
	case ST_F  | SC_R | DT_F | DC_R:	RR_LOOP (float,float);	break;

	case ST_B  | SC_R | DT_F | DC_C:	RC_LOOP (byte,float);	break;
	case ST_UB | SC_R | DT_F | DC_C:	RC_LOOP (ubyte,float);	break;
	case ST_S  | SC_R | DT_F | DC_C:	RC_LOOP (short,float);	break;
	case ST_US | SC_R | DT_F | DC_C:	RC_LOOP (ushort,float);	break;
	case ST_I  | SC_R | DT_F | DC_C:	RC_LOOP (int,float);	break;
	case ST_UI | SC_R | DT_F | DC_C:	RC_LOOP (uint,float);	break;
	case ST_F  | SC_R | DT_F | DC_C:	RC_LOOP (float,float);	break;

	case ST_B  | SC_C | DT_F | DC_C:	CC_LOOP (byte,float);	break;
	case ST_UB | SC_C | DT_F | DC_C:	CC_LOOP (ubyte,float);	break;
	case ST_S  | SC_C | DT_F | DC_C:	CC_LOOP (short,float);	break;
	case ST_US | SC_C | DT_F | DC_C:	CC_LOOP (ushort,float);	break;
	case ST_I  | SC_C | DT_F | DC_C:	CC_LOOP (int,float);	break;
	case ST_UI | SC_C | DT_F | DC_C:	CC_LOOP (uint,float);	break;
	case ST_F  | SC_C | DT_F | DC_C:	CC_LOOP (float,float);	break;


	/* the less likely cases */
	case ST_B  | SC_R | DT_B  | DC_R:	RR_LOOP (byte,byte); 	break;
	case ST_B  | SC_R | DT_S  | DC_R:	RR_LOOP (byte,short); 	break;
	case ST_B  | SC_R | DT_I  | DC_R:	RR_LOOP (byte,int);	break;
	case ST_B  | SC_R | DT_D  | DC_R:	RR_LOOP (byte,double);	break;

	case ST_B  | SC_R | DT_B  | DC_C:	RC_LOOP (byte,byte); 	break;
	case ST_B  | SC_R | DT_S  | DC_C:	RC_LOOP (byte,short); 	break;
	case ST_B  | SC_R | DT_I  | DC_C:	RC_LOOP (byte,int);	break;
	case ST_B  | SC_R | DT_D  | DC_C:	RC_LOOP (byte,double);	break;

	case ST_B  | SC_C | DT_B  | DC_C:	CC_LOOP (byte,byte); 	break;
	case ST_B  | SC_C | DT_S  | DC_C:	CC_LOOP (byte,short); 	break;
	case ST_B  | SC_C | DT_I  | DC_C:	CC_LOOP (byte,int);	break;
	case ST_B  | SC_C | DT_D  | DC_C:	CC_LOOP (byte,double);	break;


	case ST_UB | SC_R | DT_UB | DC_R:	RR_LOOP (ubyte,ubyte); 	break;
	case ST_UB | SC_R | DT_S  | DC_R:	RR_LOOP (ubyte,short); 	break;
	case ST_UB | SC_R | DT_US | DC_R:	RR_LOOP (ubyte,ushort);	break;
	case ST_UB | SC_R | DT_I  | DC_R:	RR_LOOP (ubyte,int);	break;
	case ST_UB | SC_R | DT_UI | DC_R:	RR_LOOP (ubyte,uint);	break;
	case ST_UB | SC_R | DT_D  | DC_R:	RR_LOOP (ubyte,double);	break;

	case ST_UB | SC_R | DT_UB | DC_C:	RC_LOOP (ubyte,ubyte); 	break;
	case ST_UB | SC_R | DT_S  | DC_C:	RC_LOOP (ubyte,short); 	break;
	case ST_UB | SC_R | DT_US | DC_C:	RC_LOOP (ubyte,ushort);	break;
	case ST_UB | SC_R | DT_I  | DC_C:	RC_LOOP (ubyte,int);	break;
	case ST_UB | SC_R | DT_UI | DC_C:	RC_LOOP (ubyte,uint);	break;
	case ST_UB | SC_R | DT_D  | DC_C:	RC_LOOP (ubyte,double);	break;

	case ST_UB | SC_C | DT_UB | DC_C:	CC_LOOP (ubyte,ubyte); 	break;
	case ST_UB | SC_C | DT_S  | DC_C:	CC_LOOP (ubyte,short); 	break;
	case ST_UB | SC_C | DT_US | DC_C:	CC_LOOP (ubyte,ushort);	break;
	case ST_UB | SC_C | DT_I  | DC_C:	CC_LOOP (ubyte,int);	break;
	case ST_UB | SC_C | DT_UI | DC_C:	CC_LOOP (ubyte,uint);	break;
	case ST_UB | SC_C | DT_D  | DC_C:	CC_LOOP (ubyte,double);	break;


	case ST_S  | SC_R | DT_S  | DC_R:	RR_LOOP (short,short); 	break;
	case ST_S  | SC_R | DT_I  | DC_R:	RR_LOOP (short,int);	break;
	case ST_S  | SC_R | DT_D  | DC_R:	RR_LOOP (short,double);	break;

	case ST_S  | SC_R | DT_S  | DC_C:	RC_LOOP (short,short); 	break;
	case ST_S  | SC_R | DT_I  | DC_C:	RC_LOOP (short,int);	break;
	case ST_S  | SC_R | DT_D  | DC_C:	RC_LOOP (short,double);	break;

	case ST_S  | SC_C | DT_S  | DC_C:	CC_LOOP (short,short); 	break;
	case ST_S  | SC_C | DT_I  | DC_C:	CC_LOOP (short,int);	break;
	case ST_S  | SC_C | DT_D  | DC_C:	CC_LOOP (short,double);	break;


	case ST_US | SC_R | DT_US | DC_R:	RR_LOOP (ushort,ushort); break;
	case ST_US | SC_R | DT_I  | DC_R:	RR_LOOP (ushort,int);	break;
	case ST_US | SC_R | DT_UI | DC_R:	RR_LOOP (ushort,uint);	break;
	case ST_US | SC_R | DT_D  | DC_R:	RR_LOOP (ushort,double);break;

	case ST_US | SC_R | DT_US | DC_C:	RC_LOOP (ushort,ushort); break;
	case ST_US | SC_R | DT_I  | DC_C:	RC_LOOP (ushort,int);	break;
	case ST_US | SC_R | DT_UI | DC_C:	RC_LOOP (ushort,uint);	break;
	case ST_US | SC_R | DT_D  | DC_C:	RC_LOOP (ushort,double);break;

	case ST_US | SC_C | DT_US | DC_C:	CC_LOOP (ushort,ushort); break;
	case ST_US | SC_C | DT_I  | DC_C:	CC_LOOP (ushort,int);	break;
	case ST_US | SC_C | DT_UI | DC_C:	CC_LOOP (ushort,uint);	break;
	case ST_US | SC_C | DT_D  | DC_C:	CC_LOOP (ushort,double);break;


	case ST_I  | SC_R | DT_I  | DC_R:	RR_LOOP (int,int);	break;
	case ST_I  | SC_R | DT_D  | DC_R:	RR_LOOP (int,double);	break;

	case ST_I  | SC_R | DT_I  | DC_C:	RC_LOOP (int,int);	break;
	case ST_I  | SC_R | DT_D  | DC_C:	RC_LOOP (int,double);	break;

	case ST_I  | SC_C | DT_I  | DC_C:	CC_LOOP (int,int);	break;
	case ST_I  | SC_C | DT_D  | DC_C:	CC_LOOP (int,double);	break;


	case ST_UI | SC_R | DT_UI | DC_R:	RR_LOOP (uint,uint);	break;
	case ST_UI | SC_R | DT_D  | DC_R:	RR_LOOP (uint,double);	break;

	case ST_UI | SC_R | DT_UI | DC_C:	RC_LOOP (uint,uint);	break;
	case ST_UI | SC_R | DT_D  | DC_C:	RC_LOOP (uint,double);	break;

	case ST_UI | SC_C | DT_UI | DC_C:	CC_LOOP (uint,uint);	break;
	case ST_UI | SC_C | DT_D  | DC_C:	CC_LOOP (uint,double);	break;


	case ST_F  | SC_R | DT_D  | DC_R:	RR_LOOP (float,double);	break;
	case ST_F  | SC_R | DT_D  | DC_C:	RC_LOOP (float,double);	break;
	case ST_F  | SC_C | DT_D  | DC_C:	CC_LOOP (float,double);	break;

	case ST_D  | SC_R | DT_D  | DC_R:	RR_LOOP (double,double);break;
	case ST_D  | SC_R | DT_D  | DC_C:	RC_LOOP (double,double);break;
	case ST_D  | SC_C | DT_D  | DC_C:	CC_LOOP (double,double);break;

	default:
	    DXSetError (ERROR_NOT_IMPLEMENTED, "unsupported conversion");
	    goto cleanup;
    }

    ret = OK;

cleanup:
    return (ret);
}

#undef	RR_LOOP
#undef	RC_LOOP
#undef	CC_LOOP


#define	RR_LOOP(_stype,_dtype)	ELOOP_BODY(_stype,_dtype,RR_INNER(_dtype))
#define	CC_LOOP(_stype,_dtype)	ELOOP_BODY(_stype,_dtype,CC_INNER(_dtype))

Error
_dxfExtractFieldElement (FieldInfo *src, FieldInfo *dst,
		      int element, int me, int total)
{
    int			d;
    int			i, in;
    Pointer		sbase;
    Pointer		dbase;
    int			ddelta;
    int			sdelta[MAXDIMS];
    int			origin[MAXDIMS];
    Error		ret	= ERROR;

    /* $$$$$$$ SEE ABOVE re: 3 dimensions $$$$$$$ */
    if (src->ndims != 3)
    {
	DXSetError (ERROR_NOT_IMPLEMENTED, "must be 3 dimensional");
	goto cleanup;
    }

    /*
     * Compute how far apart the elements in the source and destination are.
     * While we're at it compute the real offset (since the origin of the
     * space may not be at [0,0,...,0]) between the source and destination.
     */

    ddelta = DXCategorySize (dst->cat) * dst->epi;

    d = DXCategorySize (src->cat);
    for (i = src->ndims; i--; )
    {
	sdelta[i] = d;
	d *= src->counts[i];
    }

    for (i = 0, in = dst->ndims; i < in; i++)
	origin[i] = dst->origin[i] - src->origin[i];

    sbase = DXGetArrayData (src->data);
    dbase = DXGetArrayData (dst->data);

    switch (SetTodoMask (src, dst))
    {
	/* the popular cases first */
	case ST_B  | SC_R | DT_F | DC_R:	RR_LOOP (byte,float);	break;
	case ST_UB | SC_R | DT_F | DC_R:	RR_LOOP (ubyte,float);	break;
	case ST_S  | SC_R | DT_F | DC_R:	RR_LOOP (short,float);	break;
	case ST_US | SC_R | DT_F | DC_R:	RR_LOOP (ushort,float);	break;
	case ST_I  | SC_R | DT_F | DC_R:	RR_LOOP (int,float);	break;
	case ST_UI | SC_R | DT_F | DC_R:	RR_LOOP (uint,float);	break;
	case ST_F  | SC_R | DT_F | DC_R:	RR_LOOP (float,float);	break;

	case ST_B  | SC_C | DT_F | DC_C:	CC_LOOP (byte,float);	break;
	case ST_UB | SC_C | DT_F | DC_C:	CC_LOOP (ubyte,float);	break;
	case ST_S  | SC_C | DT_F | DC_C:	CC_LOOP (short,float);	break;
	case ST_US | SC_C | DT_F | DC_C:	CC_LOOP (ushort,float);	break;
	case ST_I  | SC_C | DT_F | DC_C:	CC_LOOP (int,float);	break;
	case ST_UI | SC_C | DT_F | DC_C:	CC_LOOP (uint,float);	break;
	case ST_F  | SC_C | DT_F | DC_C:	CC_LOOP (float,float);	break;


	/* the less likely cases */
	case ST_B  | SC_R | DT_B  | DC_R:	RR_LOOP (byte,byte); 	break;
	case ST_B  | SC_R | DT_S  | DC_R:	RR_LOOP (byte,short); 	break;
	case ST_B  | SC_R | DT_I  | DC_R:	RR_LOOP (byte,int);	break;
	case ST_B  | SC_R | DT_D  | DC_R:	RR_LOOP (byte,double);	break;

	case ST_B  | SC_C | DT_B  | DC_C:	CC_LOOP (byte,byte); 	break;
	case ST_B  | SC_C | DT_S  | DC_C:	CC_LOOP (byte,short); 	break;
	case ST_B  | SC_C | DT_I  | DC_C:	CC_LOOP (byte,int);	break;
	case ST_B  | SC_C | DT_D  | DC_C:	CC_LOOP (byte,double);	break;


	case ST_UB | SC_R | DT_UB | DC_R:	RR_LOOP (ubyte,ubyte); 	break;
	case ST_UB | SC_R | DT_S  | DC_R:	RR_LOOP (ubyte,short); 	break;
	case ST_UB | SC_R | DT_US | DC_R:	RR_LOOP (ubyte,ushort);	break;
	case ST_UB | SC_R | DT_I  | DC_R:	RR_LOOP (ubyte,int);	break;
	case ST_UB | SC_R | DT_UI | DC_R:	RR_LOOP (ubyte,uint);	break;
	case ST_UB | SC_R | DT_D  | DC_R:	RR_LOOP (ubyte,double);	break;


	case ST_UB | SC_C | DT_UB | DC_C:	CC_LOOP (ubyte,ubyte); 	break;
	case ST_UB | SC_C | DT_S  | DC_C:	CC_LOOP (ubyte,short); 	break;
	case ST_UB | SC_C | DT_US | DC_C:	CC_LOOP (ubyte,ushort);	break;
	case ST_UB | SC_C | DT_I  | DC_C:	CC_LOOP (ubyte,int);	break;
	case ST_UB | SC_C | DT_UI | DC_C:	CC_LOOP (ubyte,uint);	break;
	case ST_UB | SC_C | DT_D  | DC_C:	CC_LOOP (ubyte,double);	break;


	case ST_S  | SC_R | DT_S  | DC_R:	RR_LOOP (short,short); 	break;
	case ST_S  | SC_R | DT_I  | DC_R:	RR_LOOP (short,int);	break;
	case ST_S  | SC_R | DT_D  | DC_R:	RR_LOOP (short,double);	break;


	case ST_S  | SC_C | DT_S  | DC_C:	CC_LOOP (short,short); 	break;
	case ST_S  | SC_C | DT_I  | DC_C:	CC_LOOP (short,int);	break;
	case ST_S  | SC_C | DT_D  | DC_C:	CC_LOOP (short,double);	break;


	case ST_US | SC_R | DT_US | DC_R:	RR_LOOP (ushort,ushort); break;
	case ST_US | SC_R | DT_I  | DC_R:	RR_LOOP (ushort,int);	break;
	case ST_US | SC_R | DT_UI | DC_R:	RR_LOOP (ushort,uint);	break;
	case ST_US | SC_R | DT_D  | DC_R:	RR_LOOP (ushort,double);break;


	case ST_US | SC_C | DT_US | DC_C:	CC_LOOP (ushort,ushort); break;
	case ST_US | SC_C | DT_I  | DC_C:	CC_LOOP (ushort,int);	break;
	case ST_US | SC_C | DT_UI | DC_C:	CC_LOOP (ushort,uint);	break;
	case ST_US | SC_C | DT_D  | DC_C:	CC_LOOP (ushort,double);break;


	case ST_I  | SC_R | DT_I  | DC_R:	RR_LOOP (int,int);	break;
	case ST_I  | SC_R | DT_D  | DC_R:	RR_LOOP (int,double);	break;
	case ST_I  | SC_C | DT_I  | DC_C:	CC_LOOP (int,int);	break;
	case ST_I  | SC_C | DT_D  | DC_C:	CC_LOOP (int,double);	break;


	case ST_UI | SC_R | DT_UI | DC_R:	RR_LOOP (uint,uint);	break;
	case ST_UI | SC_R | DT_D  | DC_R:	RR_LOOP (uint,double);	break;
	case ST_UI | SC_C | DT_UI | DC_C:	CC_LOOP (uint,uint);	break;
	case ST_UI | SC_C | DT_D  | DC_C:	CC_LOOP (uint,double);	break;


	case ST_F  | SC_R | DT_D  | DC_R:	RR_LOOP (float,double);	break;
	case ST_F  | SC_C | DT_D  | DC_C:	CC_LOOP (float,double);	break;

	case ST_D  | SC_R | DT_D  | DC_R:	RR_LOOP (double,double);break;
	case ST_D  | SC_C | DT_D  | DC_C:	CC_LOOP (double,double);break;

	default:
	    DXSetError (ERROR_NOT_IMPLEMENTED, "unsupported conversion");
	    goto cleanup;
    }

    ret = OK;

cleanup:
    return (ret);
}
