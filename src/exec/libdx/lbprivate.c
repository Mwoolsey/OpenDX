/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "privateClass.h"

static Private
_NewPrivate(Pointer data, Error (*pdelete)(Pointer),
	    struct private_class *class)
{
    Private p = (Private) _dxf_NewObject((struct object_class *)class);
    if (!p)
	return NULL;
    p->data = data;
    p->delete = pdelete;
    return p;
}

Private
DXNewPrivate(Pointer data, Error (*pdelete)(Pointer))
{
    return _NewPrivate(data, pdelete, &_dxdprivate_class);
}

Error
_dxfPrivate_Delete(Private p)
{
    return p->delete? p->delete(p->data) : OK;
}

Pointer
DXGetPrivateData(Private p)
{
    return p? p->data : NULL;
}
