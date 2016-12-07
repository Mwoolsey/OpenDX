/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include "groupClass.h"
#include "internals.h"


static Series
_NewSeries(struct series_class *class)
{
    Series s = (Series) _dxf_NewGroup((struct group_class *)class);
    if (!s)
	return NULL;
    return s;
}


Series
DXNewSeries(void)
{
    return _NewSeries(&_dxdseries_class);
}


Group
_dxfSeries_SetMember(Series s, char *name, Object value)
{
    if (!s->group.typed && !_dxf_SetType((Group)s, value))
	return NULL;
    return _dxfGroup_SetMember((Group)s, name, value);
}


Group
_dxfSeries_SetEnumeratedMember(Series s, int n, Object value)
{
    if (!s->group.typed && !_dxf_SetType((Group)s, value))
	return NULL;
    return _dxfGroup_SetEnumeratedMember((Group)s, n, value);
}


Series
DXSetSeriesMember(Series s, int n, double position, Object value)
{
    if (!s->group.typed && !_dxf_SetType((Group)s, value))
	return NULL;
    if (!DXSetFloatAttribute((Object)value, "series position", position))
	return NULL;
    return (Series) _dxf_SetEnumeratedMember((Group)s, n, position, value);
}


Object
DXGetSeriesMember(Series s, int n, float *position)
{
    return _dxf_GetEnumeratedMember((Group)s, n, position, NULL);
}


static Series
_CopySeries(Series new, Series old, enum _dxd_copy copy)
{
    int i;
    float position;
    Object val;

    /* copy superclass */
    if (!_dxf_CopyObject((Object)new, (Object)old, copy))
	return NULL;

    /* done? */
    if (copy==COPY_ATTRIBUTES)
	return new;

    /*
     * XXX - should COPY_ATTRIBUTES copy the type also?  Since 
     * there is no way to reset it, it is inconvenient if it does so.
     * It seems unlikely that anyone would want to create an empty
     * series and copy the type.
     */
    new->group.typed = old->group.typed;
    new->group.type = old->group.type;
    new->group.category = old->group.category;
    new->group.rank = old->group.rank;
    if (old->group.shape) {
	new->group.shape = (int *)
	    DXAllocate(old->group.rank * sizeof(*(old->group.shape)));
	for (i=0; i<old->group.rank; i++)
	    new->group.shape[i] = old->group.shape[i];
    } else
	new->group.shape = NULL;

    /* copy the members */
    for (i=0; (val=DXGetSeriesMember(old, i, &position)); i++) {
	if (copy!=COPY_HEADER) {
	    val = DXCopy(val, copy);
	    if (!val)
		return NULL;
	}
	DXSetSeriesMember(new, i, position, val);
    }
    return new;
}

/*
 * DXCopy.  DXWarning: this code is essentially the same as the
 * code for Group_Copy, until the loop at the end that does the copy.
 * If you change anything here, check that code also.
 */

Object
_dxfSeries_Copy(Series old, enum _dxd_copy copy)
{
    Series new;

    new = DXNewSeries();
    if (!new)
	return NULL;

    /* XXX - check return code and delete new */
    return (Object) _CopySeries(new, old, copy);
}

