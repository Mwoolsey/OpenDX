/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "hwDeclarations.h"
#include "hwList.h"
#include "hwClipped.h"
#include "hwMatrix.h"
#include "hwMemory.h"
#include "hwPortLayer.h"
#include "hwWindow.h"
#include "hwXfield.h"
#include "hwObjectHash.h"

/**************************************************************************************
 *
 * Two forms of object hashing.  We hash objects by object tag to cache triangle
 * strips and texture display lists.  We also hash objects by object tag, approximation 
 * type and density to cache geometry display lists.
 *
 **************************************************************************************/

typedef struct
{
    Key		key;
    int    	hit;
    dxObject 	obj;
} *OHashP, OHashS;

HashTable
_dxf_InitObjectHash()
{
    HashTable ht = DXCreateHash(sizeof(OHashS), NULL, NULL);
    return ht;
}

void
_dxf_ResetObjectHash(HashTable ht)
{
    OHashP ptr;

    if (ht)
    {
	DXInitGetNextHashElement(ht);
	while (NULL != (ptr = (OHashP)DXGetNextHashElement(ht)))
	    ptr->hit = 0;
    }
}
       

dxObject
_dxf_QueryObject(HashTable ht, dxObject keyO)
{
    Key key;
    OHashP ptr;

    if (! ht)
        return NULL;

    key = (Key)DXGetObjectTag(keyO);
    ptr = (OHashP)DXQueryHashElement(ht, (Pointer)&key);
    if (ptr)
    {
        ptr->hit++;
	return ptr->obj;
    }
    else
        return NULL;
}

void
_dxf_InsertObject(HashTable ht, dxObject keyO, dxObject obj)
{
    OHashS str;
    Key key;
    OHashP ptr;

    if (! ht)
        return;

    key = (Key)DXGetObjectTag(keyO);
    ptr = (OHashP)DXQueryHashElement(ht, (Pointer)&key);
    if (ptr)
    {
        if (ptr->obj != obj)
		{
			if (ptr->obj)
				DXDelete(ptr->obj);
	        ptr->obj = DXReference(obj);
		}
    }
    else
    {
	str.key   = key;
	str.hit   = 1;
	str.obj   = DXReference(obj);

	if (! DXInsertHashElement(ht, (Element)&str))
	    DXDelete(obj);
    }
}

void
_dxf_DeleteObjectHash(HashTable ht)
{
    OHashP ptr;

    if (ht)
    {
	DXInitGetNextHashElement(ht);
	while (NULL != (ptr = (OHashP)DXGetNextHashElement(ht)))
	{
	    if (ptr->obj)
		DXDelete(ptr->obj);
	}

	DXDestroyHash(ht);
    }
}

extern void _dxf_deleteHashElt(HashTable h, Element e);

HashTable 
_dxf_CullObjectHash(HashTable ht)
{
	HashTable new = NULL;
	
	if (! ht)
	{
		return NULL;
	}
	else
	{
		OHashP ptr;
		new = DXCreateHash(sizeof(OHashS), NULL, NULL);
		
		DXInitGetNextHashElement(ht);
		while (NULL != (ptr = (OHashP)DXGetNextHashElement(ht)))
		{
			if (ptr->hit)
			{
				OHashS str;
				str.key = ptr->key;
				str.hit = 0;
				str.obj = DXReference(ptr->obj);
				if (! DXInsertHashElement(new, (Element)&str))
					DXDelete(ptr->obj);
			}
			
			if (ptr->obj)
				DXDelete(ptr->obj);
				
		}
		
		DXDestroyHash(ht);
		return new;
	}
	
	if (new)
		DXDestroyHash(new);
		
		return NULL;
}

/**************************************************************************************
 *
 * Here is the second form.  We need to use hash and compare functions.
 *
 **************************************************************************************/


typedef struct
{
    int		 tag;
    int 	 approx;
    int 	 density;
    float 	 linewidth;
    int 	 aalines;
} *DLKeyP, DLKeyS;

typedef struct
{
    DLKeyS	 key;
    int    	 hit;
    dxObject 	 obj;
} *DLHashP, DLHashS;

static PseudoKey
dlhash(Key key)
{
    DLKeyP dlkey = (DLKeyP)key;
    int a;
    long l;

    switch(dlkey->approx)
    {
	case approx_none:  a = 0; break;
	case approx_flat:  a = 1; break;
	case approx_lines: a = 2; break;
	case approx_dots:  a = 3; break;
	default:           a = 4; break;
    }

    l = (((dlkey->tag ^ a) ^ dlkey->density) ^
			(*(int *)&dlkey->linewidth)) ^ (dlkey->aalines*29);

    return (PseudoKey)l;
}

static int
dlcmp(Key k0, Key k1)
{
    DLKeyP dlkey0 = (DLKeyP)k0;
    DLKeyP dlkey1 = (DLKeyP)k1;

    if ((dlkey0->tag        != dlkey1->tag)       ||
        (dlkey0->approx     != dlkey1->approx)    ||
        (dlkey0->linewidth  != dlkey1->linewidth) ||
        (dlkey0->aalines    != dlkey1->aalines)   ||
        (dlkey0->density    != dlkey1->density))
	return 1;
    else
	return 0;
}
    
HashTable
_dxf_InitDisplayListHash()
{
    HashTable ht = DXCreateHash(sizeof(DLHashS), dlhash, dlcmp);
    return ht;
}

void
_dxf_ResetDisplayListHash(HashTable ht)
{
    DLHashP ptr;

    if (ht)
    {
	DXInitGetNextHashElement(ht);
	while (NULL != (ptr = (DLHashP)DXGetNextHashElement(ht)))
	    ptr->hit = 0;
    }
}
       
dxObject
_dxf_QueryDisplayList(HashTable ht, xfieldP xf, int buttonUp)
{
    DLKeyS key;
    DLHashP ptr;

    if (! ht)
        return NULL;

    key.tag       = DXGetObjectTag((dxObject)xf->field);
    key.approx    = buttonUp ?
			xf->attributes.buttonUp.approx :
			xf->attributes.buttonDown.approx;
    key.density   = buttonUp ?
			xf->attributes.buttonUp.density :
			xf->attributes.buttonDown.density;
    key.linewidth = xf->attributes.linewidth;
    key.aalines   = xf->attributes.aalines;

    ptr = (DLHashP)DXQueryHashElement(ht, (Pointer)&key);
    if (ptr)
    {
        ptr->hit++;
	return ptr->obj;
    }
    else
        return NULL;
}

void
_dxf_InsertDisplayList(HashTable ht, xfieldP xf, int buttonUp, dxObject obj)
{
    DLHashS str;
    DLHashP ptr;

    if (! ht)
        return;

    str.key.tag       = DXGetObjectTag((dxObject)xf->field);
    str.key.approx    = buttonUp ?
			xf->attributes.buttonUp.approx :
			xf->attributes.buttonDown.approx;
    str.key.density   = buttonUp ?
			xf->attributes.buttonUp.density :
			xf->attributes.buttonDown.density;
    str.key.linewidth = xf->attributes.linewidth;
    str.key.aalines   = xf->attributes.aalines;

    ptr = (DLHashP)DXQueryHashElement(ht, (Pointer)&str.key);
    if (ptr)
    {
        if (ptr->obj != obj)
	{
	    if (ptr->obj) DXDelete(ptr->obj);
	    ptr->obj = DXReference(obj);
	}
    }
    else
    {
	str.hit   = 1;
	str.obj   = DXReference(obj);

	if (! DXInsertHashElement(ht, (Element)&str))
	    DXDelete(obj);
    }
}

void
_dxf_DeleteDisplayListHash(HashTable ht)
{
    DLHashP ptr;

    if (ht)
    {
	DXInitGetNextHashElement(ht);
	while (NULL != (ptr = (DLHashP)DXGetNextHashElement(ht)))
	{
	    if (ptr->obj)
		DXDelete(ptr->obj);
	}

	DXDestroyHash(ht);
    }
}

extern void _dxf_deleteHashElt(HashTable h, Element e);

HashTable 
_dxf_CullDisplayListHash(HashTable ht)
{
	HashTable new = NULL;
	
	if (! ht)
	{
		return NULL;
	}
	else
	{
		DLHashP ptr;
		new = DXCreateHash(sizeof(DLHashS), dlhash, dlcmp);
		
		DXInitGetNextHashElement(ht);
		while (NULL != (ptr = (DLHashP)DXGetNextHashElement(ht)))
		{
			if (ptr->hit)
			{
				DLHashS str;
				str.key = ptr->key;
				str.hit = 0;
				str.obj = DXReference(ptr->obj);
				if (! DXInsertHashElement(new, (Element)&str))
					DXDelete(ptr->obj);
			}
			
			if (ptr->obj)
				DXDelete(ptr->obj);
				
		}
		
		DXDestroyHash(ht);
		return new;
	}
	
	if (new)
		DXDestroyHash(new);
		
		return NULL;
}
