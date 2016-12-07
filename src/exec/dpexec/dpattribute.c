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
#include "parse.h"

/*
 * if the node attribute list contains this attribute,
 * return the DX object it contains.
 */
Object _dxf_ExGetAttribute (node *n, char *attr)
{
    char		*id;

    if (! n)
	return (NULL);

    if (n->type != NT_ATTRIBUTE)
	return (NULL);
    
    for (; n; n = n->next)
    {
	id  = n->v.attr.id ->v.id.id;

	if (! strcmp (id, attr))
	    return ((Object) n->v.attr.val->v.constant.obj);
    }

    return (NULL);
}

/* 
 * if the node contains this string attribute,
 * extract the contents and return it.
 * NULL means it didn't exist or existed but wasn't a string.
 * use HasStringAttribute instead if you want to know the difference.
 */
char * _dxf_ExGetStringAttribute (node *n, char *attrname)
{
    Object	o;
    char 	*cp;

    if (! (o = _dxf_ExGetAttribute(n, attrname)))
	return (NULL);
    
    if (! DXExtractString (o, &cp))
	return (NULL);

    return (cp);
}

Error _dxf_ExHasStringAttribute (node *n, char *attrname, char **value)
{
    Object	o;

    if (! (o = _dxf_ExGetAttribute(n, attrname)))
	return (ERROR);
    
    if (! DXExtractString (o, value))
	return (ERROR);

    return (OK);
}


int _dxf_ExGetIntegerAttribute (node *n, char *attrname)
{
    Object	o;
    int		i;

    if (! (o = _dxf_ExGetAttribute(n, attrname)))
	return (0);
    
    if (! DXExtractInteger (o, &i))
	return (0);

    return (i);
}


Error _dxf_ExHasIntegerAttribute (node *n, char *attrname, int *i)
{
    Object	o;

    if (! (o = _dxf_ExGetAttribute(n, attrname)))
	return (ERROR);
    
    if (! DXExtractInteger (o, i))
	return (ERROR);

    return (OK);
}

