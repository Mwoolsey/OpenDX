/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/*
 * component level manipulations of hierarchial groups.
 */


#include <string.h>
#include <dx/dx.h>

#define MAXRANK 64

static Object Field__Extract(Object o, char *name);
static Object Field__Insert(Object o, Object add, char *name);
static Object Field__Replace(Object o, Object add, char *old, char *new, int flag);
static Object Field__Rename(Object o, char *old, char *new, int flag);
static Object Field__Remove(Object o, char *name);
static Object Field__Exists(Object o, char *name);
static Object Field__Exists2(Object o, char *name, char *name2);
static Object Field__Swap(Object o, char *c1, char *c2);
static Object Field__Empty(Object o);

static void set_gtype(Group g);
static void unset_gtype(Group g);

/* public with internal name for now */
Object _dxfDXEmptyObject(Object o);

/* public for edfparse.c */
Array _dxfReallyCopyArray(Array a);

static Array CopyArray(Array a);
static Array MakeArray(String s);

#if 0
static void fix_attrs(Field f, char *old, char *new);
#endif

static int tree_match(Object o1, Object o2, int flag);
#define EXACT    0	/* structures much match exactly */
#define ARRAYOK  1	/* fields can match arrays */
#define ARRAYREQ 2	/* fields MUST match arrays */



#if 0
/* default attribute workaround1:
 * if the component doesn't have a 'dep' attribute and it's being made
 *  the 'data' component, give it an artificial 'dep' on itself.
 *
 * (i think this would work, except there is no 'DeleteAttribute()' call,
 *  so it's hard to undo.  i want to call make_dep in Mark, and unmake_dep
 *  in unmark if the component is dep itself.)
 */
#define make_dep(a, n)  if (!(DXGetAttribute((Object)a, "dep")) \
                            DXSetStringAttribute((Object)a, "dep", (char *)n))
#endif


/* default attribute workaround2:
 *  don't call endfield ever, to avoid adding unwanted dependencies.
 */
#define CALL_ENDFIELD 1

/* default attribute workaround3:
 *  add special calls which Mark uses, which don't call DXEndField.
 *  this is not the right solution, but it is expedient.
 */
Object _dxf_RenameX(Object o, char *old, char *new);
Object _dxf_ReplaceX(Object o, Object add, char *from, char *to);


/*
 *  rename component in an object
 */
Object DXRename(Object o, char *old, char *new)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if (!Field__Exists(o, old))
	DXErrorReturn(ERROR_MISSING_DATA, "source component not found");
    
    return Field__Rename(o, old, new, 1);
}

/*
 *  rename component in an object without calling DXEndField
 */
Object _dxf_RenameX(Object o, char *old, char *new)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if (!Field__Exists(o, old))
	DXErrorReturn(ERROR_MISSING_DATA, "source component not found");
    
    return Field__Rename(o, old, new, 0);
}


static Object Field__Rename(Object o, char *old, char *new, int flag)
{
    Object subo, comp;
    Object subo2;
    Matrix m;
    int fixed, z;
    char *cname;
    int i, istyped = 0;

    if (!o) 
	return NULL;

    switch(DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	if (!strcmp(new, "data"))
	    istyped++;
	
	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &cname)); i++) 
	    if (!Field__Rename(subo, old, new, flag))
		return NULL;
	
	if (istyped)
	    set_gtype((Group)o);
	
	break;
	
      case CLASS_FIELD:
	comp = DXGetComponentValue((Field)o, old);
	if (!comp)
	    break;

        /* get rid of component about to be replaced, because otherwise
         *  the attributes of the deleted component will override attributes
         *  of the new component.
         */
	DXDeleteComponent((Field)o, new);

	if (!DXSetComponentValue((Field)o, new, comp))
	    return NULL;
	
	if (!DXDeleteComponent((Field)o, old))
	    return NULL;
	
        if ((!DXChangedComponentValues((Field)o, old))
         || (!DXChangedComponentValues((Field)o, new)))
	    return NULL;
	
#if CALL_ENDFIELD
	if (flag && !DXEndField((Field)o))
	    return NULL;
#endif
	
	break;
	
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
    
	if (!Field__Rename(subo, old, new, flag))
	    return NULL;
	
	break;
	
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
	
	if (!Field__Rename(subo, old, new, flag))
	    return NULL;
 
	break;
  
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
	
	if (!Field__Rename(subo, old, new, flag))
	    return NULL;
	
	break;
	
      default:
	/* anything which can't contain a group or field, pass thru unchanged.
	 */
	break;

    }
    
    return o;
}



/*
 *  swap components in an object.
 */
Object DXSwap(Object o, char *c1, char *c2)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if (!Field__Exists2(o, c1, c2))
        DXErrorReturn(ERROR_MISSING_DATA, "missing component(s)");
    
    return Field__Swap(o, c1, c2);
}


static Object Field__Swap(Object o, char *c1, char *c2)
{
    Object subo, comp1, comp2;
    Object subo2;
    Matrix m;
    int fixed, z;
    char *cname;
    int i, istyped = 0;

    if (!o) 
	return NULL;
    
    switch(DXGetObjectClass(o)) {
	    
      case CLASS_GROUP:
	if (!strcmp(c1, "data") || !strcmp(c2, "data"))
	    istyped++;

	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &cname)); i++)
	    if (!Field__Swap(subo, c1, c2))
		return NULL;
	
	if (istyped)
	    set_gtype((Group)o);
	
	break;
	
      case CLASS_FIELD:
	if (!(comp1 = DXGetComponentValue((Field)o, c1)) 
	 || !(comp2 = DXGetComponentValue((Field)o, c2)))
	    break;
	
	/* do this to prevent c1 & c2 from being deleted;
         *  if they aren't deleted, their attributes won't be
         *  copied correctly.
	 */
	DXReference((Object)comp1);
	DXReference((Object)comp2);
	
        if (!DXDeleteComponent((Field)o, c1)) {
            DXDelete((Object)comp1);
            DXDelete((Object)comp2);
            return NULL;
        }
        
        if (!DXDeleteComponent((Field)o, c2)) {
            DXDelete((Object)comp2);
            return NULL;
        }
        
	if (!DXSetComponentValue((Field)o, c2, comp1)) {
	    DXDelete((Object)comp2);
	    return NULL;
	}
	if (!DXSetComponentValue((Field)o, c1, comp2)) {
	    DXDelete((Object)comp1);
	    return NULL;
	}
	
	/* to match DXReference above */
	DXDelete((Object)comp1);
	DXDelete((Object)comp2);

        /* remove any 'der' components, since they are tagged as being
         *  derived from the old names.
         */
        if ((!DXChangedComponentValues((Field)o, c1))
	||  (!DXChangedComponentValues((Field)o, c2)))
            return NULL;

#if CALL_ENDFIELD
	if (!DXEndField((Field)o))
	    return NULL;
#endif

	break;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (!Field__Swap(subo, c1, c2))
	    return NULL;

	break;
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
 
	if (!Field__Swap(subo, c1, c2))
	    return NULL;

	break;
 
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
 
	if (!Field__Swap(subo, c1, c2))
	    return NULL;

	break;
	
      default:
	/* anything which can't contain a group or field, pass thru unchanged.
	 */
	break;

    }
    
    return o;
}



/*
 *  remove a component from an object
 */
Object DXRemove(Object o, char *name)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");

    if (!Field__Exists(o, name))
        DXErrorReturn(ERROR_MISSING_DATA, "component not found");

    return Field__Remove(o, name);
}


static Object Field__Remove(Object o, char *name)
{
    Object subo;
    Object subo2;
    Matrix m;
    int fixed, z;
    char *cname;
    int i, istyped = 0;

    if (!o) 
	return NULL;

    switch(DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	if (!strcmp(name, "data"))
	    istyped++;

	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &cname)); i++) 
	    if (!Field__Remove(subo, name))
		return NULL;

	if (istyped)
	    unset_gtype((Group)o);

	break;
	
      case CLASS_FIELD:
	/* if component exists, remove it and delete anything dep on it. 
         *  don't call DXEndField here, because it may put back the component
	 *  you just removed (e.g. bounding box).
	 */
	if (DXDeleteComponent((Field)o, name))
	    if (!DXChangedComponentValues((Field)o, name))
		return NULL;

	break;
	
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (!Field__Remove(subo, name))
	    return NULL;

	break;
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
 
	if (!Field__Remove(subo, name))
	    return NULL;

	break;
 
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
 
	if (!Field__Remove(subo, name))
	    return NULL;

	break;
 
      default:
	/* anything which can't contain a group or field, pass thru unchanged.
	 */
	break;
	
    }
    
    return o;
}



/*
 *  tell whether a component exists anywhere in an object
 */
Object DXExists(Object o, char *name)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if (!name)
	DXErrorReturn(ERROR_MISSING_DATA, "null name");
    
    return Field__Exists(o, name);
}


static Object Field__Exists(Object o, char *name)
{
    Object subo, comp;
    Object subo2;
    Matrix m;
    int fixed, z;
    int i;

    if (!o) 
	return NULL;
    
    switch(DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
	    if (Field__Exists(subo, name))
		return o;
	
	break;
	
      case CLASS_FIELD:
	comp = DXGetComponentValue((Field)o, name);
	if (!comp)
	    break;
	
	return o;
	
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (Field__Exists(subo, name))
	    return o;
	
	break;
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
	
	if (Field__Exists(subo, name))
	    return o;

	break;
	
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
	
	if (Field__Exists(subo, name))
	    return o;

	break;
	
      default:
	/* anything which can't contain a group or field, can't contain
         *  this component.  break out and return NULL.
	 */
	break;
	
    }

    return NULL;
}



/*
 *  tell whether all fields which contain a component of name also
 *  contain a component of name2
 */
Object Exists2(Object o, char *name, char *name2)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");

    if (!name || !name2)
	DXErrorReturn(ERROR_MISSING_DATA, "null name");

    return Field__Exists2(o, name, name2);
}


static Object Field__Exists2(Object o, char *name, char *name2)
{
    Object subo;
    Object subo2;
    Matrix m;
    int fixed, z;
    int i;

    if (!o) 
	return NULL;
    
    switch(DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
	    if (!Field__Exists2(subo, name, name2))
		return NULL;
	
	break;
	
      case CLASS_FIELD:
	/* if the field has either component, it has to have the 
	 *  other component also.
	 */
	if (DXGetComponentValue((Field)o, name)) {
	    if (!DXGetComponentValue((Field)o, name2))
		return NULL;
	}
	
	break;
	
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (!Field__Exists2(subo, name, name2))
	    return NULL;

	break;
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    break;
 
	if (!Field__Exists2(subo, name, name2))
	    return NULL;

	break;
 
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    break;
 
	if (!Field__Exists2(subo, name, name2))
	    return NULL;

	break;
 
      default:
	/* anything which can't contain a group or field, can't contain
	 *  one component without the other.  break and return obj.
	 */
	break;

    }

    return o;
}


/*
 *  see if any fields in the object contain components.
 */
Object _dxfDXEmptyObject(Object o)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    return Field__Empty(o);
}

static Object Field__Empty(Object o)
{
    Object subo, comp;
    Object subo2;
    Matrix m;
    char *name;
    int fixed, z;
    int i;

    if (!o) 
	return NULL;
    
    switch(DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
	    if (!Field__Empty(subo))
		return NULL;
	
	break;
	
      case CLASS_FIELD:
	comp = DXGetEnumeratedComponentValue((Field)o, 0, &name);
	if (comp)
	    return NULL;
	
	return o;
	
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (!Field__Empty(subo))
	    return NULL;
	
	break;
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
	
	if (!Field__Empty(subo))
	    return NULL;

	break;
	
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
	
	if (!Field__Empty(subo))
	    return NULL;

	break;
	
      default:
	/* anything which can't contain a group or field, can't contain
         *  valid components.  break out and return OK.
	 */
	break;
	
    }

    return o;
}


/*
 * extract - return a new object with all fields replaced with just the
 *   named component.
 *
 *   if input is a field, output is just component.  if group,
 *   output is group of components.  this routine modifies the input object,
 *   so it expects the caller to have called copy first.
 */
Object DXExtract(Object o, char *name)
{
    if (!o)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if (!Field__Exists(o, name))
	DXErrorReturn(ERROR_MISSING_DATA, "component not found");

    
    return Field__Extract(o, name);
}


static Object Field__Extract(Object o, char *name)
{
    Object subo;
    Object subo2, newo;
    Matrix m;
    int fixed, z;
    char *cname;
    int i, istyped = 0;

    if (!o)
	return NULL;

    switch(DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	istyped = (DXGetType(o, NULL, NULL, NULL, NULL) == o);
	
	/* if the input field is typed and we are not extracting
	 *  the data component, remove the existing data component
	 *  so we don't get errors about mismatched types 
	 */
	if (istyped && strcmp(name, "data")) {
	    
	    /* the normal case is that the group is typed because it
	     * contains a data component, but it is possible to type
	     * a group without one.  if there is data, field_remove
	     * will untype the group; otherwise do it explicitly.
	     */
	    if (DXExists(o, "data")) {
		if (!Field__Remove(o, "data"))
		    return NULL;
	    } else
		unset_gtype((Group)o);
	}
	
        /* replace each field with one array */
	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &cname)); i++) {
	    
            /* replace member with object which was extracted */
	    if (!(newo = Field__Extract(subo, name)))
		return NULL;
	    
	    if (newo != subo) {
		if (! (cname ? DXSetMember((Group)o, cname, newo) :
		               DXSetEnumeratedMember((Group)o, i, newo))) {
		    DXDelete(newo);		    
		    return NULL;
		}
	    }
        }
	
	if (istyped)
	    set_gtype((Group)o);
	
	break;
	
      case CLASS_FIELD:
        /* replace object with the component value */
        subo = DXGetComponentValue((Field)o, name);
	if (subo) {
	    /* this is getting ridiculous.  the following 4 lines are:
	     *  1. keep the array from being deleted.
	     *  2. keep the field from being deleted if it is part of
	     *   another object.
	     *  3. delete the field if it is the top level object; otherwise
	     *   decrement the ref count so it will be deleted later.
	     *  4. unreference the array; if it is the top level object
	     *   it will have a ref count of 0, otherwise it will have
	     *   whatever ref count it had before.
	     */
	    DXReference(subo);
	    DXReference(o);
	    DXDelete(o);
	    DXUnreference(subo);
	    o = subo;
	} else {
	    /* see items 2 & 3 above. */
	    DXReference(o);
	    DXDelete(o);
	    o = (Object)DXEndField(DXNewField());  /* should be empty array */
	}

        break;
	
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
	
	if (!(newo = Field__Extract(subo, name)))
	    return NULL;
	
	if (newo == subo)
	    break;
	
	if (!DXSetXformObject((Xform)o, newo)) {
	    DXDelete(newo);
	    return NULL;
	}
	break;
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
 
	if (!(newo = Field__Extract(subo, name)))
	    return NULL;

	if (newo == subo)
	    break;

	if (!DXSetScreenObject((Screen)o, newo)) {
	    DXDelete(newo);
	    return NULL;
	}
	break;
	
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
	
	if (!(newo = Field__Extract(subo, name)))
	    return NULL;
	
	if (newo == subo)
	    break;

	if (!DXSetClippedObjects((Clipped)o, newo, subo2)) {
	    DXDelete(newo);
	    return NULL;
	}
	break;
	
      default:
	/* anything else can't contain a field with a component in it.
	 *  return it unchanged. 
	 */
	break;
    }
    
    return o;
}


/*
 * insert - add an object as a component in a field.
 *          at the end, object o->name is object add
 *
 *  force objects to match exactly in structure - with the exception
 *   that a field in o matches an array in add.
 *
 */
Object DXInsert(Object o, Object add, char *name)
{
    if (!o || !add)
	DXErrorReturn(ERROR_MISSING_DATA, "null object");

    if (!tree_match(o, add, ARRAYREQ))
	DXErrorReturn(ERROR_BAD_CLASS, "object hierarchies don't match");

    return Field__Insert(o, add, name);
}


static Object Field__Insert(Object o, Object add, char *name)
{
    Object subo, subadd;
    Object subo2, newo;
    Matrix m;
    int fixed, z;
    char *cname;
    int i, istyped = 0;

    if (!o) 
	return NULL;

    switch(DXGetObjectClass(o)) {
	    
      case CLASS_GROUP:
	if (!strcmp(name, "data"))
	    istyped++;

	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &cname)) &&
	          (subadd=DXGetEnumeratedMember((Group)add, i, NULL)); i++) {
	
	    if (!(newo = Field__Insert(subo, subadd, name)))
		return NULL;

	    if (newo != subo) {
		if (! (cname ? DXSetMember((Group)o, cname, newo) :
                               DXSetEnumeratedMember((Group)o, i, newo))) {
		    DXDelete(newo);
		    return NULL;
		}
	    }
	}

	if (istyped)
	    set_gtype((Group)o);

	break;
	    
      case CLASS_FIELD:
        DXDeleteComponent((Field)o, name);

	if (!DXSetComponentValue((Field)o, name, add))
	    return NULL;
	
	if (!DXChangedComponentValues((Field)o, name))
	    return NULL;

#if CALL_ENDFIELD
        if (!DXEndField((Field)o))
	    return NULL;
#endif
	
	break;
	
      case CLASS_XFORM:
	if ((!DXGetXformInfo((Xform)o, &subo, &m))
	 || (!DXGetXformInfo((Xform)add, &subadd, NULL)))
	    return NULL;
 
	if (!(newo = Field__Insert(subo, subadd, name)))
	    return NULL;

	if (newo == subo)
	    break;

	if (!DXSetXformObject((Xform)o, newo)) {
	    DXDelete(newo);
	    return NULL;
	}
	
	break;
	
      case CLASS_SCREEN:
	if ((!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	|| (!DXGetScreenInfo((Screen)add, &subadd, NULL, NULL)))
	    return NULL;
	
	if (!(newo = Field__Insert(subo, subadd, name)))
	    return NULL;
	
	if (newo == subo)
	    break;
	
	if (!(Object)DXSetScreenObject((Screen)o, newo)) {
	    DXDelete(newo);
	    return NULL;
	}
	
	break;
 
      case CLASS_CLIPPED:
	if ((!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	|| (!DXGetClippedInfo((Clipped)add, &subadd, NULL)))
	    return NULL;
 
	if (!(newo = Field__Insert(subo, subadd, name)))
	    return NULL;
	
	if (newo == subo)
	    break;
	
	if (!DXSetClippedObjects((Clipped)o, newo, subo2)) {
	    DXDelete(newo);
	    return NULL;
	}

	break;
	
      default:
	/* ignore anything else which isn't a group or field.
	 */
	break;
    }

    return o;
}


/*
 * replace - copy the named component from one object into another.
 *           at the end, object o->to has add->from
 *
 *  force objects to match exactly in structure.
 *
 */
Object DXReplace(Object o, Object add, char *from, char *to)
{
    if (!o || !add) 
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if ((o != add)  &&  !tree_match(o, add, ARRAYOK))
	DXErrorReturn(ERROR_BAD_CLASS, "object hierarchies don't match");

#if 0    
    if (!Field__Exists(add, from))
	DXErrorReturn(ERROR_MISSING_DATA, "source component not found");
#endif
    
    return Field__Replace(o, add, from, to, 1);
}

/*
 * same as above, without calling DXEndField.
 */
Object _dxf_ReplaceX(Object o, Object add, char *from, char *to)
{
    if (!o || !add) 
	DXErrorReturn(ERROR_MISSING_DATA, "null object");
    
    if ((o != add)  &&  !tree_match(o, add, ARRAYOK))
	DXErrorReturn(ERROR_BAD_CLASS, "object hierarchies don't match");
    
    if (!Field__Exists(add, from))
	DXErrorReturn(ERROR_MISSING_DATA, "source component not found");
    
    return Field__Replace(o, add, from, to, 0);
}


static Object Field__Replace(Object o, Object add, char *from, char *to, int flag)
{
    Object subo, subadd, comp;
    Object subo2;
    Matrix m;
    int fixed, z;
    char *cname;
    int i, istyped = 0;
    
    if (!o) 
	return NULL;

    switch(DXGetObjectClass(o)) {
	    
      case CLASS_GROUP:
	if (!strcmp(to, "data"))
	    istyped++;

	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &cname)) &&
	          (subadd=DXGetEnumeratedMember((Group)add, i, NULL)); i++)

	    if (!Field__Replace(subo, subadd, from, to, flag))
		return NULL;

	    
	if (istyped)
	    set_gtype((Group)o);

	break;
	
      case CLASS_FIELD:
	if (DXGetObjectClass(add) == CLASS_FIELD) {
	    comp = DXGetComponentValue((Field)add, from);
	    if (!comp)
		break;
	    
	    if (DXGetObjectClass(comp) == CLASS_ARRAY) {
		comp = (Object)CopyArray((Array)comp);
		if (!comp)
		    return NULL;
	    }
	} else if (DXGetObjectClass(add) == CLASS_STRING) {
	    comp = (Object)MakeArray((String)add);
	    if (!comp)
		return NULL;
	} else
	    /* must already be CLASS_ARRAY (tree_match() ensures this) */
	    comp = add;
        
        DXDeleteComponent((Field)o, to);
	
	if (!DXSetComponentValue((Field)o, to, comp))
	    return NULL;
	
	if (!DXChangedComponentValues((Field)o, to))
	    return NULL;

#if CALL_ENDFIELD
        if (flag && !DXEndField((Field)o))
	    return NULL;
#endif
	
	break;
	
      case CLASS_XFORM:
	if ((!DXGetXformInfo((Xform)o, &subo, &m))
         || (!DXGetXformInfo((Xform)add, &subadd, NULL)))
	    return NULL;
 
	if (!Field__Replace(subo, subadd, from, to, flag))
	    return NULL;

	break;
  
      case CLASS_SCREEN:
	if ((!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	 || (!DXGetScreenInfo((Screen)add, &subadd, NULL, NULL)))
	    return NULL;
 
	if (!Field__Replace(subo, subadd, from, to, flag))
	    return NULL;

	break;
 
      case CLASS_CLIPPED:
	if ((!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	 || (!DXGetClippedInfo((Clipped)add, &subadd, NULL)))
	    return NULL;
 
	if (!Field__Replace(subo, subadd, from, to, flag))
	    return NULL;
	
	break;
	
      default:
	/* anything which can't contain a group or field, pass thru unchanged.
	 */
	break;
    }
    
    return o;
}

/*
 * make sure Group type information matches the data component type
 *
 *  if the data component has been changed, and if the group is marked as
 *  typed, the group type must be updated.
 */
static void set_gtype(Group g)
{
    Object o;
    Type t; 
    Category c;
    int i;
    int r, s[MAXRANK];

    switch(DXGetGroupClass(g)) {
      case CLASS_MULTIGRID:
      case CLASS_COMPOSITEFIELD:
      case CLASS_SERIES:
	for (i=0; (o=DXGetEnumeratedMember(g, i, NULL)); i++) {
	    if (DXGetType(o, &t, &c, &r, s)) {
		DXSetGroupTypeV(g, t, c, r, s);
		return;
	    }
	}
      default:
	break;
    }
    return;
}

static void unset_gtype(Group g)
{
    DXUnsetGroupType(g);
    return;
}


/* this currently doesn't seem to be needed - removing the 'der' components
 *  with DXChangedComponentValues seems to solve our problems so far.
 *  if it really becomes necessary to change the targets of 'dep' and 'ref',
 *  then add calls to this routine where necessary.  you have to be careful
 *  of generating cyclical dep/ref paths between components, because some
 *  routines try to follow them; you may also have to watch for things added
 *  by DXEndField, because it knows some default component names.
 */

/*
 * look through all the components in a field, and if any have the old
 *  component name as the target of a ref, dep or der, change it to new.
 */

#if 0

static char *attrlist[] = { "ref", "dep", "der", "" };

static void fix_attrs(Field f, char *old, char *new)
{
    int i;
    Object attr;
    char **cp, *name;

    for (cp = attrlist; *cp[0]; cp++) {
        for (i=0; 
             (attr=DXGetEnumeratedComponentAttribute(f, i, &name, *cp)); 
             i++) {

            if (name && DXGetObjectClass(attr) == CLASS_STRING 
            &&  !strcmp(DXGetString((String)attr), old))
		DXSetComponentAttribute(f, name, *cp, (Object)DXNewString(new));
        }
    }
    
}
#endif


/*
 * make sure objects have matching hierarchies
 *
 *  if flag == EXACT, the hierarchies must be the same
 *  if flag != EXACT, a field in o1 must match an array in o2
 */
static int tree_match(Object o1, Object o2, int flag)
{
    Class c1, c2;
    Object subo1, subo2;
    int i;

    if (!o1 || !o2)
        return ERROR;

    c1 = DXGetObjectClass(o1);
    c2 = DXGetObjectClass(o2);

    switch(flag) {
      case EXACT:
	/* classes must match exactly */
	if (c1 != c2)
	    return ERROR;
	
	break;
	
      case ARRAYOK:
	/* fields match either arrays, strings or fields */
	if (c1 == CLASS_FIELD) {
	    switch (c2) {
	      case CLASS_FIELD:
	      case CLASS_ARRAY:
	      case CLASS_STRING:
		return OK;
		
	      default:
		return ERROR;
	    }
	}
	
	break;

      case ARRAYREQ:
	/* fields match arrays or strings */
	if (c1 == CLASS_FIELD)
	    return (((c2==CLASS_ARRAY)||(c2==CLASS_STRING)) ? OK : ERROR);

	break;
    }


    /* for objects which contain subobjects which might be fields, look
     *  further.
     */
    switch (c1) {
      case CLASS_GROUP:
	for (i=0; (subo1=DXGetEnumeratedMember((Group)o1, i, NULL)) &&
	          (subo2=DXGetEnumeratedMember((Group)o2, i, NULL)); i++) {
	    
	    if (!tree_match(subo1, subo2, flag))
		return ERROR;
	}
	
	/* less things in o1 than o2? */
	if (subo1 && !subo2)
	    return ERROR;
	
	/* more things in o2 than o1? */
	if (!subo1 && DXGetEnumeratedMember((Group)o2, i, NULL))
	    return ERROR;
	
	break;

      case CLASS_XFORM:
	if ((!DXGetXformInfo((Xform)o1, &subo1, NULL))
	 || (!DXGetXformInfo((Xform)o2, &subo2, NULL)))
	    return ERROR;

	if (!tree_match(subo1, subo2, flag))
	    return ERROR;

	break;

      case CLASS_SCREEN:
	if ((!DXGetScreenInfo((Screen)o1, &subo1, NULL, NULL))
	 || (!DXGetScreenInfo((Screen)o2, &subo2, NULL, NULL)))
	    return ERROR;
 
	if (!tree_match(subo1, subo2, flag))
	    return ERROR;

	break;

      case CLASS_CLIPPED:
	if ((!DXGetClippedInfo((Clipped)o1, &subo1, NULL))
	 || (!DXGetClippedInfo((Clipped)o2, &subo2, NULL)))
	    return ERROR;
 
	if (!tree_match(subo1, subo2, flag))
	    return ERROR;

	break;
      default:
	break;
    }

    return OK;
}


/* extern entry point */
Array _dxfReallyCopyArray(Array a)
{
    return CopyArray(a);
}

/* is this still necessary?  it is used by replace and import because 
 *  if you don't copy the array and you change the attributes on the
 *  new array, you also change the original since they are attached
 *  to the array and not to the component name.
 */

static Array CopyArray(Array a)
{
    Array na = NULL;
    Pointer p = NULL;
    int nitems;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];
    Array terms[MAXRANK], nterms[MAXRANK];
    Pointer origin[MAXRANK*sizeof(double)];
    Pointer delta[MAXRANK*MAXRANK*sizeof(double)];
    int i;
	

    if (!DXGetArrayInfo(a, &nitems, &type, &cat, &rank, shape))
	return NULL;
    
    switch(DXGetArrayClass(a)) {
	
      default:
      case CLASS_ARRAY:
	na = DXNewArrayV(type, cat, rank, shape);
	if (!na)
	    return NULL;
	
	if (nitems == 0)
	    break;
	
	p = DXGetArrayData(a);
	if (!p)
	    goto error;
	
	if (!DXAddArrayData((Array)na, 0, nitems, NULL))
	    goto error;
	
	bcopy(p, DXGetArrayData(na), nitems * DXGetItemSize(na));
	break;
	
      case CLASS_REGULARARRAY:
	DXGetRegularArrayInfo((RegularArray)a, &nitems, 
			      (Pointer)origin, (Pointer)delta);
	na = (Array)DXNewRegularArray(type, shape[0], nitems, 
				      (Pointer)origin, (Pointer)delta);
	break;
	
      case CLASS_CONSTANTARRAY:
	na = (Array)DXNewConstantArrayV(nitems, 
					DXGetConstantArrayData(a),
					type, cat, rank, shape);
	break;
	
      case CLASS_PATHARRAY:
	na = (Array)DXNewPathArray(nitems+1);
	break;
	
      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)a, &nitems, terms);
	for (i=0; i<nitems; i++) {
	    nterms[i] = CopyArray(terms[i]);
	    if (!nterms[i])
		goto error;
	}
	
	na = (Array)DXNewProductArrayV(nitems, nterms);
	break;
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)a, &nitems, terms);
	for (i=0; i<nitems; i++) {
	    nterms[i] = CopyArray(terms[i]);
	    if (!nterms[i])
		goto error;
	}

	na = (Array)DXNewMeshArrayV(nitems, nterms);
	DXGetMeshOffsets((MeshArray)a, (int *)origin);
	DXSetMeshOffsets((MeshArray)na, (int *)origin);
	break;

    }

    if (!na)
	return NULL;
    
    DXCopyAttributes((Object)na, (Object)a);
    return na;

  error:
    DXDelete((Object)na);
    return NULL;
}

/* convert a string object into a string array, since all field
 * components should really be arrays.
 */
static Array MakeArray(String s)
{
    Array a;
    char *cp;
    int len;

    cp = DXGetString(s);
    if (!cp)
	return NULL;

    len = strlen(cp) + 1;
    a = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, len);
    if (!a)
	return NULL;

    if (!DXAddArrayData(a, 0, 1, (Pointer)cp))
	goto error;

    return a;
    
  error:
    DXDelete ((Object)a);
    return NULL;
}
