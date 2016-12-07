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


static CompositeField
_dxf_NewCompositeField(struct compositefield_class *class)
{
    CompositeField s = (CompositeField) _dxf_NewGroup((struct group_class *)class);
    if (!s)
	return NULL;
    return s;
}


CompositeField
DXNewCompositeField(void)
{
    return _dxf_NewCompositeField(&_dxdcompositefield_class);
}


Group
_dxfCompositeField_SetMember(CompositeField s, char *name, Object value)
{
    if (!s->group.typed && value && !_dxf_SetType((Group)s, value))
	return NULL;
    return _dxfGroup_SetMember((Group)s, name, value);
}


Group
_dxfCompositeField_SetEnumeratedMember(CompositeField s, int n, Object value)
{
    if (!s->group.typed && value && !_dxf_SetType((Group)s, value))
	return NULL;
    return _dxfGroup_SetEnumeratedMember((Group)s, n, value);
}

