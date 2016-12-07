/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "utils.h"

#if 0
char *strsaven (char *old, int n)
{
    char	*new;

    new = DXAllocate (n + 1);
    if (new)
    {
	memcpy (new, old, n);
	new[n] = NULL;
    }
    return (new);
}
#endif

#if 0
char *lstrsaven (char *old, int n)
{
    char	*new;

    new = DXAllocateLocal (n + 1);
    if (new)
    {
	memcpy (new, old, n);
	new[n] = NULL;
    }
    return (new);
}
#endif

char *_dxf_ExStrSave (char *old)
{
    char	*new;
    int		n;

    n   = strlen (old);
    new = DXAllocate (n + 1);
    if (new)
    {
	memcpy (new, old, n);
	new[n] = '\0';
    }
    return (new);
}

#if 0
char *lstrsave (char *old)
{
    char	*new;
    int		n;

    n   = strlen (old);
    new = DXAllocateLocal (n + 1);
    if (new)
    {
	memcpy (new, old, n);
	new[n] = NULL;
    }
    return (new);
}
#endif

/*
 * Adjusts a pointer value such that it falls on an n byte aligned boundary.
 *
 * WARNING:	Only works if n is a power of 2.
 */

Pointer _dxf_ExAlignBoundary	(long n, Pointer p)
{
    long	mask;

    if (n == 0 || n == 1)
	return (p);
    
    mask = n - 1;

    return ((mask & (long) p)
		? (Pointer) (((long) p + n) & ~mask)
		: p);
}


char *_dxf_ExCopyStringN (char *old, int len)
{
    char        *new;

    if (old == NULL)
	return (NULL);

    new = (char *) DXAllocate (len + 1);

    if (! new)
    {
        DXResetError ();
        return (NULL);
    }

    strncpy (new, old, len);
    new[len] = '\0';

    return (new);
}



char *_dxf_ExCopyString (char *old)
{
    char	*new;

    if (old == NULL)
	return (NULL);

    new = (char *) DXAllocate (strlen (old) + 1);

    if (! new)
    {
	DXResetError ();
	return (NULL);
    }
    
    strcpy (new, old);

    return (new);
}


char *_dxf_ExCopyStringLocal (char *old)
{
    char		*new;
    int			size;

    size = strlen (old) + 1;

    new = (char *) DXAllocateLocal (size);
    
    if (! new)
    {
	DXResetError ();
	return (NULL);
    }

    strcpy (new, old);

    return (new);
}

int _dxf_ExNextPowerOf2 (int n)
{
    int		i;
    int		v;

    for (i = 0, v = 1; i < 31; i++, v <<= 1)
	if (v >= n)
	    return (v);
    return 0; 
}

Array _dxfExNewInteger (int n)
{
    return DXAddArrayData (DXNewArray (TYPE_INT, CATEGORY_REAL, 0), 
			0, 1, (Pointer) &n);
}

