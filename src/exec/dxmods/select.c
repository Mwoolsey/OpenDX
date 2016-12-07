/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

#define EmptyField2(o)  (DXGetObjectClass((Object)o) == CLASS_FIELD) && \
                         DXEmptyField((Field)o)

#define MAXRANK 16

/* types of objects which can be selected from: */
#define UNKNOWN    0
#define IS_SERIES  1
#define IS_GROUP   2
#define IS_STRING  3
#define IS_LIST    4

static Object doobject(Object o, Object whichone, int fexcept);
static Object doselect(Object o, Object whichone, int fexcept, int whattype); 

static Object selectnamedgroup(Group g, Object names);
static Object selectnumberedgroup(Group g, Object numbers);
static Object selectnumberedseries(Series s, Object numbers);
static Array  selectnumberedlist(Array a, Object numbers);
static Object selectnumberedstring(Object o, Object numbers);
static Object omitnamedgroup(Group g, Object names);
static Object omitnumberedgroup(Group g, Object numbers);
static Object omitnumberedseries(Series s, Object numbers);
static Array  omitnumberedlist(Array a, Object numbers);
static Object omitnumberedstring(Object o, Object numbers);

static int int_compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}
static int str_compare(const void *a, const void *b)
{
    return strcmp((char *)a, (char *)b);
}


/* if this is 0, then fields are not accepted as input.  if this is 1,
 * then if the input is a field the data component is extracted and then
 * handled as a list.  the upside of allowing fields as input is the 
 * user doesn't have to extract the data component first; the downside
 * is that most of the time when someone passes a field into select they
 * are doing it by mistake and we can help them out by giving them an error.
 */
#define DO_FIELDS 0


int
m_Select(Object *in, Object *out)
{
    int fexcept;

    out[0] = NULL;

    /* no input object 
     */
    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "group or list");
	return ERROR;
    }

    /* empty input object
     */
    if (EmptyField2(in[0])) {
	out[0] = in[0];
	return OK;
    }

    /* flag which says copy everything except the members listed
     */
    if (in[2]) {
	if (!DXExtractInteger(in[2], &fexcept)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "except"); 
	    return ERROR;
	}

	if (fexcept != 0 && fexcept != 1) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "except");
	    return ERROR;
	}
    } else
	fexcept = 0;


    out[0] = doobject(in[0], in[1], fexcept);

    
    return ((out[0] == NULL) ? ERROR : OK);
}


/* recurse thru container objects like xforms until we get to something
 * we can select from.  should this include screen objects?  clips?
 */
static Object
doobject(Object o, Object whichone, int fexcept)
{
    Object newo = NULL;
    Object subo = NULL;

    switch (DXGetObjectClass(o)) {
      case CLASS_STRING:
	return doselect(o, whichone, IS_STRING, fexcept);
	
      case CLASS_GROUP:
	switch (DXGetGroupClass((Group)o)) {
	  case CLASS_SERIES:
	    return doselect(o, whichone, IS_SERIES, fexcept);

	  default:
	    return doselect(o, whichone, IS_GROUP, fexcept);
	    
	}
	break;
	
#if DO_FIELDS
      case CLASS_FIELD:
        subo = DXGetComponentValue((Field)o, "data");
        if (!subo) {
            DXSetError(ERROR_DATA_INVALID, 
  	       "no data component found in field for selecting list items");
            return NULL;
        }

	return doselect(subo, whichone, IS_LIST, fexcept);
#endif

      case CLASS_ARRAY:
	return doselect(o, whichone, IS_LIST, fexcept);
	
      case CLASS_XFORM:
	/* get xform object, call select and put object back into
	 * a copy of xform.
	 */
	if (!DXGetXformInfo((Xform)o, &subo, NULL))
	    return NULL;
	
	if (!(subo = doobject(subo, whichone, fexcept)))
	    return NULL;
	
	if (!(newo = DXCopy(o, COPY_ATTRIBUTES))) {
	    DXDelete(subo);
	    return NULL;
	}
	
	if (!DXSetXformObject((Xform)newo, subo)) {
	    DXDelete(newo);
	    DXDelete(subo);
	    return NULL;
	}
	    
       default:
         break;

       return newo;
    }

    DXSetError(ERROR_BAD_PARAMETER, "input must be group or list");
    return NULL;
}

/* the input to this routine should be something you can select from:
 *  a group or list.  whichone should either be a list of indicies or
 *  a list of names (for a group with named members) and whattype should
 *  have been set by the calling routine saying whether the input object
 *  is a group, series, number or string list or simple string.
 */
static Object
doselect(Object o, Object whichone, int whattype, int fexcept)
{
    char *cp;
    int i;


    /* whichone default: return first group member or first member of list
     */
    if (!whichone) {
	switch (whattype) {
	  case IS_SERIES:
	    if (fexcept)
		return omitnumberedseries((Series)o, NULL);
	    else
		return selectnumberedseries((Series)o, NULL);
	    
	  case IS_GROUP:
	    if (fexcept)
		return omitnumberedgroup((Group)o, NULL);
	    else
		return selectnumberedgroup((Group)o, NULL);
	    
	  case IS_STRING:
	    if (fexcept)
		return (Object)omitnumberedstring(o, NULL);
	    else
		return (Object)selectnumberedstring(o, NULL);
	    
	  case IS_LIST:
	  default:
	    if (fexcept)
		return (Object)omitnumberedlist((Array)o, NULL);
	    else 
		return (Object)selectnumberedlist((Array)o, NULL);

        }
    }


    /* if user specified a name, select by name.  if user specified an int,
     *  select by enumerated member number (this is not series position, which
     *  is a float.)
     */
    if (DXExtractString(whichone, &cp) || DXExtractNthString(whichone, 0, &cp)) {
	
	switch (whattype) {
	  case IS_SERIES:
	  case IS_GROUP:
	    if (fexcept)
		return omitnamedgroup((Group)o, whichone);
	    else
		return selectnamedgroup((Group)o, whichone);

	  case IS_STRING:
            DXSetError(ERROR_BAD_PARAMETER, 
                     "cannot extract items from a string by name");
            return NULL;

	  case IS_LIST:
	  default:
            DXSetError(ERROR_BAD_PARAMETER, 
		       "cannot extract items from a list by name");
            return NULL;
        }
        
    } else if (DXQueryParameter(whichone, TYPE_INT, 0, &i)) {
        
	switch (whattype) {
	  case IS_SERIES:
	    if (fexcept)
		return omitnumberedseries((Series)o, whichone);
	    else
		return selectnumberedseries((Series)o, whichone);
            
	  case IS_GROUP:
	    if (fexcept)
		return omitnumberedgroup((Group)o, whichone);
	    else
		return selectnumberedgroup((Group)o, whichone);
            
	  case IS_STRING:
	    if (fexcept)
		return (Object)omitnumberedstring(o, whichone);
	    else
		return (Object)selectnumberedstring(o, whichone);

	  case IS_LIST:
	  default:
	    if (fexcept)
		return (Object)omitnumberedlist((Array)o, whichone);
	    else
		return (Object)selectnumberedlist((Array)o, whichone);
        }
        
    } 
	
    DXSetError(ERROR_BAD_PARAMETER, "#10051", "which member");
    return NULL;
}

/* select members from a series by index number.  the index is different from
 * the series position; the series position is a float and can be any value.
 * series groups are handled differently from other types of groups because 
 * the series position has to be preserved if the output is also a group
 * (which happens when more than one member is selected).
 */
static Object 
selectnumberedseries(Series s, Object numbers)
{
    Object selected;
    Series news = NULL;
    float position;
    int i, count, members;
    int *memberlist = NULL;

    /* if empty group, can't select.
     */
    if (!DXGetMemberCount((Group)s, &members) || members == 0) {
	DXSetError(ERROR_DATA_INVALID, "#11310"); 
	/* group has no members */
	return NULL;
    }

    /* if no numbers, return the first member */
    if (!numbers) {
        selected = DXGetEnumeratedMember((Group)s, 0, NULL);
        if(!selected)
            return NULL;
        
        return selected;
    }
    
    /* if one number, return it */
    if (DXExtractInteger(numbers, &i)) {
        selected = DXGetEnumeratedMember((Group)s, i, NULL);
	if(!selected) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0, since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
                     "which member", 0, members-1);
	    }
	    return NULL;
	}
        
        return selected;
    }

    /* if a list of numbers, build up a new series */
    news = (Series)DXCopy((Object)s, COPY_ATTRIBUTES);
    if (!news)
        return NULL;
    
    if (!DXQueryParameter(numbers, TYPE_INT, 0, &count)) {
        DXSetError(ERROR_BAD_PARAMETER, 
                 "'%s' must be an integer or integer list",
                 "which member");
        goto error;
    }

    memberlist = (int *)DXAllocate(sizeof(int) * count);
    if (!memberlist)
        goto error;
    
    if (!DXExtractParameter(numbers, TYPE_INT, 0, count, (Pointer)memberlist))
        goto error;


    for (i=0; i<count; i++) {
        selected = DXGetSeriesMember(s, memberlist[i], &position);
	if(!selected) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0, since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
                  "'%s' must be an integer or integer list between %d and %d",
                  "which member", 0, members-1);
	    }
	    goto error;
	}
        
        if (!DXSetSeriesMember(news, i, (double)position, selected))
            goto error;
        
    }
     
    DXFree((Pointer)memberlist);
    return (Object)news;

  error:
    DXFree((Pointer)memberlist);
    DXDelete((Object)news);
    return NULL;
}

/* select members from a group by name.  normally used on generic groups,
 * composite fields and multigrid groups, but technically not illegal for
 * series groups (it is hard to create a series group where the members have
 * names, and if there is more than one member in the output (so the output is
 * a series and not just the individual member), the series positions will
 * be lost.)
 */
static Object 
selectnamedgroup(Group g, Object names)
{
    Object selected;
    Group newg = NULL;
    char *cp;
    int i;

    /* if empty group, can't select.
     */
    if (!DXGetMemberCount(g, &i) || i == 0) {
	DXSetError(ERROR_DATA_INVALID, "#11310");  /* group has no members */
	return NULL;
    }
    
    /* if no names, return the first member */
    if (!names) {
        selected = DXGetEnumeratedMember(g, 0, NULL);
        if(!selected)
            return NULL;
	
        return selected;
    }
    
    /* if just one name, return it */
    if (DXExtractString(names, &cp)) {
        selected = DXGetMember(g, cp);
	if(!selected) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11320", cp);
	    return NULL;
	}
       
        return selected;
    }

    /* if a list of names, build up a new group */
    newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
    if (!newg)
        return NULL;

    for (i=0; DXExtractNthString(names, i, &cp); i++) {
        selected = DXGetMember(g, cp);
        if(!selected) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11320", cp);
	    goto error;
	}
        
        if (!DXSetMember(newg, cp, selected))
            goto error;
    }
     
    return (Object)newg;

  error:
    DXDelete((Object)newg);
    return NULL;
}

/* select group members by index number.  preserve names if present.
 * see the comment on selectnumberedseries for why series groups are
 * treated separately.
 */
static Object 
selectnumberedgroup(Group g, Object numbers)
{
    Object selected;
    Group newg = NULL;
    char *cp;
    int i, count, members;
    int *memberlist = NULL;

    /* if empty group, can't select.
     */
    if (!DXGetMemberCount(g, &members) || members == 0) {
	DXSetError(ERROR_DATA_INVALID, "#11310"); 
	/* group has no members */
	return NULL;
    }

    /* if no numbers, return the first member */
    if (!numbers) {
        selected = DXGetEnumeratedMember(g, 0, NULL);
        if(!selected)
            return NULL;
        
        return selected;
    }
    
    /* if one number, return it */
    if (DXExtractInteger(numbers, &i)) {
        selected = DXGetEnumeratedMember(g, i, NULL);
	if(!selected) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "'%s' must be 0, since the group has only one member",
			 "which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
                     "which member", 0, members-1);
	    }
	    return NULL;
	}
        
        return selected;
    }

    /* if a list of numbers, build up a new group */
    newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
    if (!newg)
        return NULL;
    
    if (!DXQueryParameter(numbers, TYPE_INT, 0, &count)) {
        DXSetError(ERROR_BAD_PARAMETER, 
                 "'%s' must be an integer or integer list",
                 "which member");
        goto error;
    }

    memberlist = (int *)DXAllocate(sizeof(int) * count);
    if (!memberlist)
        goto error;
    
    if (!DXExtractParameter(numbers, TYPE_INT, 0, count, (Pointer)memberlist))
        goto error;


    for (i=0; i<count; i++) {
        selected = DXGetEnumeratedMember(g, memberlist[i], &cp);
	if(!selected) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "'%s' must be 0, since the group has only one member",
			 "which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
                  "'%s' must be an integer or integer list between %d and %d",
                     "which member", 0, members-1);
	    }
	    goto error;
	}
        
        if (!DXSetMember(newg, cp, selected))
            goto error;
        
    }
     
    DXFree((Pointer)memberlist);
    return (Object)newg;

  error:
    DXFree((Pointer)memberlist);
    DXDelete((Object)newg);
    return NULL;
}

/* select an item from a list by index number.  this works for number lists
 * as well as string lists.
 */
static Array
selectnumberedlist(Array a, Object numbers)
{
    Array new = NULL;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];
    int items;
    int size;
    int i, count;
    int *memberlist = NULL;
    char *src, *src2;


    /* get info about the old array, and make an empty new array.
     */
    if (!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
        return NULL;
    
    if (items == 0) {
	DXSetError(ERROR_DATA_INVALID, "list contains no items");
	return NULL;
    }

    new = DXNewArrayV(type, cat, rank, shape);
    if (!new)
        goto error;
    

    /* if numbers is NULL, return first item in array.
     */
    if (!numbers) {
        
	/* copy first data from old to new */
	src = (char *)DXGetArrayData(a);
        if (!src)
            goto error;
        
        if (!DXAddArrayData(new, 0, 1, (Pointer)src))
            goto error;
        
        return new;
	
    }


    /* if numbers is one item, return it
     */

    if (DXExtractInteger(numbers, &i)) {

        /* range check */
        if (i < 0 || i >= items) {
	    if (items == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "'%s' must be 0, since the list has only one item",
			 "which item");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
			 "which item", 0, items-1);
	    }
	    goto error;
        }
	
        size = DXGetItemSize(a);
	if (size <= 0)
	    goto error;

        /* copy Nth data item data from old to new */
        src = (char *)DXGetArrayData(a);
        if (!src)
            goto error;

        src += size * i;
        
        if (!DXAddArrayData(new, 0, 1, (Pointer)src))
            goto error;
        
        return new;

    }

    /* if a list of numbers, build up a new array
     */
    if (!DXQueryParameter(numbers, TYPE_INT, 0, &count)) {
        DXSetError(ERROR_BAD_PARAMETER, 
                 "'%s' must be an integer or integer list",
                 "which item");
        goto error;
    }
    
    memberlist = (int *)DXAllocate(sizeof(int) * count);
    if (!memberlist)
        goto error;
    
    if (!DXExtractParameter(numbers, TYPE_INT, 0, count, (Pointer)memberlist))
        goto error;
    
    
    src = (char *)DXGetArrayData(a);
    if (!src)
	goto error;

    size = DXGetItemSize(a);
    if (size <= 0)
	goto error;
    
    
    for (i=0; i<count; i++) {

        /* range check */
        if (memberlist[i] < 0 || memberlist[i] >= items) {
	    if (items == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "'%s' must be 0, since the list has only one item",
			 "which item");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
			 "which item", 0, items-1);
	    }
	    goto error;
        }
	
        src2 = src + (size * memberlist[i]);

	if (!DXAddArrayData(new, i, 1, (Pointer)src2))
	    goto error;
	
    }

    DXFree((Pointer)memberlist);
    return new;

  error:
    DXFree((Pointer)memberlist);
    DXDelete((Object)new);
    return ERROR;
}

/* this routine handles the case where string data comes in as a string object
 * instead of an array of type string.  a string object can contain only a
 * single item, so this can only succeed if the "which item" input is either
 * not set or is set and the value is 0.
 * this could have been handled by the selectnumberedlist code but it's so
 * different that it was cleaner to separate it out into a different routine.
 */
static Object 
selectnumberedstring(Object o, Object numbers)
{
    char *cp;
    int i;

    if (DXGetObjectClass(o) != CLASS_STRING) {
	DXSetError(ERROR_INTERNAL, 
		   "selectnumberedstring routine called on non-string input");
	return NULL;
    }

    cp = DXGetString((String)o);
    if (!cp) {
	DXSetError(ERROR_DATA_INVALID, 
		   "input is an empty string, must be group or list");
	return NULL;
    }

    /* default - return first (and only) member of list. */
    if (numbers == NULL)
        return o;

    /* if numbers contains one item and the value is 0, return it;
     * otherwise if there is more than one item in the list
     * or the value isn't 0, it's an error.
     */
    if (DXExtractInteger(numbers, &i) && (i == 0))
	return o;


    DXSetError(ERROR_BAD_PARAMETER, 
	       "'%s' must be 0, since the string has only one item",
	       "which item");
    return NULL;
}


/* omit members from a series by index number.  the index is different from
 * the series position; the series position is a float and can be any value.
 * series groups are handled differently from other types of groups because 
 * the series position has to be preserved if the output is also a group
 * (which happens unless all but one member is omitted).
 */
static Object 
omitnumberedseries(Series s, Object numbers)
{
    Object omitted;
    Series news = NULL;
    float position;
    int i, newi, selected;
    int count, members;
    int *memberlist = NULL;

    /* if empty group, return error
     */
    if (!DXGetMemberCount((Group)s, &members) || members == 0) {
	DXSetError(ERROR_DATA_INVALID, "#11310"); 
	/* group has no members */
	return NULL;
    }

    /* if no numbers, return all but the first member */
    if (!numbers) {

	news = (Series)DXCopy((Object)s, COPY_ATTRIBUTES);
	if (!news)
	    return NULL;
	
	for (i=1; i<members; i++) {
	    omitted = DXGetSeriesMember(s, i, &position);
	    if (!omitted)
		goto error;
        
	    if (!DXSetSeriesMember(news, i-1, (double)position, omitted))
		goto error;

	}

        return (Object)news;
    }
    
    /* if one number, return all but it */
    if (DXExtractInteger(numbers, &selected)) {

	if (selected >= members) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0, since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
                     "which member", 0, members-1);
	    }
	    return NULL;
	}

	news = (Series)DXCopy((Object)s, COPY_ATTRIBUTES);
	if (!news)
	    return NULL;
	
	for (i=0, newi=0; i<members; i++) {
	    if (i == selected)
		continue;

	    omitted = DXGetSeriesMember(s, i, &position);
	    if (!omitted)
		goto error;
	    
	    if (!DXSetSeriesMember(news, newi++, (double)position, omitted))
		goto error;

	}

        return (Object)news;
    }

    /* if a list of numbers, build up a new series */
    news = (Series)DXCopy((Object)s, COPY_ATTRIBUTES);
    if (!news)
        return NULL;
    
    if (!DXQueryParameter(numbers, TYPE_INT, 0, &count)) {
        DXSetError(ERROR_BAD_PARAMETER, 
                 "'%s' must be an integer or integer list",
                 "which member");
        goto error;
    }

    memberlist = (int *)DXAllocate(sizeof(int) * count);
    if (!memberlist)
        goto error;
    
    if (!DXExtractParameter(numbers, TYPE_INT, 0, count, (Pointer)memberlist))
        goto error;

    /* sort list here into numeric order */
    qsort((Pointer)memberlist, count, sizeof(int), int_compare);

    /* look for illegal values */
    for (i=0; i<count; i++) {
	if (memberlist[i] >= members) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0 since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
	"'%s' (item %d) must be an integer or integer list between %d and %d",
		   "which member", i, 0, members-1);
	    }
	    goto error;
	}
    }

    for (i=0, newi=0; i<members; i++) {

	/* anything on the list doesn't get copied */
	if (bsearch((Pointer)&i, (Pointer)memberlist, count, sizeof(int),
		    int_compare))
	    continue;
	
        omitted = DXGetSeriesMember(s, i, &position);
	if (!omitted)
	    goto error;
        
        if (!DXSetSeriesMember(news, newi++, (double)position, omitted))
            goto error;
        
    }
     
    DXFree((Pointer)memberlist);
    return (Object)news;

  error:
    DXFree((Pointer)memberlist);
    DXDelete((Object)news);
    return NULL;
}

/* omit members from a group by name.  normally used on generic groups,
 * composite fields and multigrid groups, but technically not illegal for
 * series groups (it is hard to create a series group where the members have
 * names, and if there is more than one member in the output (so the output is
 * a series and not just the individual member), the series positions will
 * be lost.)
 */
static Object 
omitnamedgroup(Group g, Object names)
{
    Object omitted;
    Group newg = NULL;
    char *membername, *cp, *namelist = NULL;
    int namlen, count, members;
    int i;

    /* if empty group, return error
     */
    if (!DXGetMemberCount(g, &members) || members == 0) {
	DXSetError(ERROR_DATA_INVALID, "#11310");  /* group has no members */
	return NULL;
    }
    
    /* if no names, return all but the first member */
    if (!names) {
	newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
	if (!newg)
	    return NULL;
	
	for (i=1; i<members; i++) {
	    omitted = DXGetEnumeratedMember(g, i, &membername);
	    if (!omitted)
		goto error;
        
	    if (!DXSetMember(newg, membername, omitted))
		goto error;

	}

        return (Object)newg;
    }
    
    /* if just one name, return all but it */
    if (DXExtractString(names, &membername)) {

	/* if you are asking for the output to not have this
	 * member, then it really isn't an error if the group
	 * member isn't there in the first place...
	 * but print a warning in case the name is misspelled.
	 */
        omitted = DXGetMember(g, membername);
	if (!omitted) {
	    DXWarning("#11320", membername);
	    return (Object)g;
	}
	
	newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
	if (!newg)
	    return NULL;
	
	for (i=0; i<members; i++) {
	    omitted = DXGetEnumeratedMember(g, i, &cp);
	    if (!omitted)
		goto error;
        
	    if (cp && !strcmp(membername, cp))
		continue;
	    
	    if (!DXSetMember(newg, cp, omitted))
		goto error;

	}

        return (Object)newg;
    }


    /* if a list of names, build up a new group */
    newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
    if (!newg)
        return NULL;

    /* sort the names */
    namlen = DXGetItemSize((Array)names);
    if (namlen <= 0)
	goto error;

    if (!DXGetArrayInfo((Array)names, &count, NULL, NULL, NULL, NULL))
	goto error;
    
    namelist = (char *)DXAllocate(namlen * count);
    if (!namelist)
	goto error;

    cp = (char *)DXGetArrayData((Array)names);
    if (!cp)
	goto error;

    /* if you are asking for the output to not have these
     * members, then it really isn't an error if any of the group
     * members aren't there in the first place...
     * but print a warning in case the name is misspelled.
     */
    for (i=0; i<count; i++) {
	/* first see if they are all valid names */
        omitted = DXGetMember(g, cp + i*namlen);
	if (!omitted) 
	    DXWarning("#11320", cp + i*namlen);
    }

    /* sort the list so it's easier to look up names in it */
    memcpy(namelist, cp, namlen*count);

    qsort((Pointer)namelist, count, namlen, str_compare);

    for (i=0; i < members; i++) {
        omitted = DXGetEnumeratedMember(g, i, &cp);
        if (!omitted)
	    goto error;

	/* anything on the list doesn't get copied */
        if (cp && bsearch((Pointer)cp, (Pointer)namelist, count, namlen,
			  str_compare))
	    continue;

        if (!DXSetMember(newg, cp, omitted))
            goto error;
    }
     
    DXFree((Pointer)namelist);
    return (Object)newg;

  error:
    DXFree((Pointer)namelist);
    DXDelete((Object)newg);
    return NULL;
}

/* omit group members by index number.  preserve names if present.
 * see the comment on omitnumberedseries for why series groups are
 * treated separately.
 */
static Object 
omitnumberedgroup(Group g, Object numbers)
{
    Object omitted;
    Group newg = NULL;
    char *name;
    int i, selected;
    int count, members;
    int *memberlist = NULL;

    /* if empty group, return error
     */
    if (!DXGetMemberCount(g, &members) || members == 0) {
	DXSetError(ERROR_DATA_INVALID, "#11310"); 
	/* group has no members */
	return NULL;
    }

    /* if no numbers, return all but the first member */
    if (!numbers) {

	newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
	if (!newg)
	    return NULL;
	
	for (i=1; i<members; i++) {
	    omitted = DXGetEnumeratedMember(g, i, &name);
	    if (!omitted)
		goto error;
        
	    if (!DXSetMember(newg, name, omitted))
		goto error;

	}

        return (Object)newg;
    }
    
    /* if one number, return all but it */
    if (DXExtractInteger(numbers, &selected)) {

	if (selected >= members) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0, since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
                     "which member", 0, members-1);
	    }
	    return NULL;
	}

	newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
	if (!newg)
	    return NULL;
	
	for (i=0; i<members; i++) {
	    if (i == selected)
		continue;

	    omitted = DXGetEnumeratedMember(g, i, &name);
	    if (!omitted)
		goto error;
	    
	    if (!DXSetMember(newg, name, omitted))
		goto error;

	}

        return (Object)newg;
    }

    /* if a list of numbers, build up a new group */
    newg = (Group)DXCopy((Object)g, COPY_ATTRIBUTES);
    if (!newg)
        return NULL;
    
    if (!DXQueryParameter(numbers, TYPE_INT, 0, &count)) {
        DXSetError(ERROR_BAD_PARAMETER, 
                 "'%s' must be an integer or integer list",
                 "which member");
        goto error;
    }

    memberlist = (int *)DXAllocate(sizeof(int) * count);
    if (!memberlist)
        goto error;
    
    if (!DXExtractParameter(numbers, TYPE_INT, 0, count, (Pointer)memberlist))
        goto error;

    /* look for illegal values */
    for (i=0; i<count; i++) {
	if (memberlist[i] >= members) {
	    if (members == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0 since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
	"'%s' (item %d) must be an integer or integer list between %d and %d",
		   "which member", i, 0, members-1);
	    }
	    goto error;
	}
    }

    /* sort list here into numeric order */
    qsort((Pointer)memberlist, count, sizeof(int), int_compare);

    for (i=0; i<members; i++) {

	/* anything on the list doesn't get copied */
	if (bsearch((Pointer)&i, (Pointer)memberlist, count, sizeof(int),
		    int_compare))
	    continue;
	
        omitted = DXGetEnumeratedMember(g, i, &name);
	if (!omitted)
	    goto error;
        
        if (!DXSetMember(newg, name, omitted))
            goto error;
        
    }
     
    DXFree((Pointer)memberlist);
    return (Object)newg;

  error:
    DXFree((Pointer)memberlist);
    DXDelete((Object)newg);
    return NULL;
}


/* omit an item from a list by index number.  this works for number lists
 * as well as string lists.
 */
static Array
omitnumberedlist(Array a, Object numbers)
{
    Array new = NULL;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];
    int items;
    int size;
    int i, newi, count;
    int *memberlist = NULL;
    char *src, *src2;


    /* get info about the old array, and make an empty new array.
     */
    if (!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
        return NULL;
    
    if (items == 0) {
	DXSetError(ERROR_DATA_INVALID, "list contains no items");
	return NULL;
    }

    new = DXNewArrayV(type, cat, rank, shape);
    if (!new)
        goto error;
    

    /* if numbers is NULL, copy all but first item in array.
     */
    if (!numbers) {
        
	/* copy data from old to new */
	src = (char *)DXGetArrayData(a);
        if (!src)
            goto error;
        
	size = DXGetItemSize(a);
        if (!DXAddArrayData(new, 0, items-1, (Pointer)(src+size)))
            goto error;
        
        return new;
	
    }


    /* if numbers is one item, copy all but it
     */

    if (DXExtractInteger(numbers, &i)) {

        /* range check */
        if (i < 0 || i >= items) {
	    if (items == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "'%s' must be 0, since the list has only one item",
			 "which item");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "'%s' must be an integer or integer list between %d and %d",
			 "which item", 0, items-1);
	    }
	    goto error;
        }
	
        size = DXGetItemSize(a);
	if (size <= 0)
	    goto error;

        /* copy 0 to Nth-1 and Nth to End data items from old to new */
        src = (char *)DXGetArrayData(a);
        if (!src)
            goto error;

	if (i > 0 && !DXAddArrayData(new, 0, i, (Pointer)src))
	    goto error;
        
        src += size * (i+1);
        
        if (i < items-1 && !DXAddArrayData(new, i, items-i-1, (Pointer)src))
            goto error;
        
        return new;

    }

    /* if a list of numbers, build up a new array
     */
    if (!DXQueryParameter(numbers, TYPE_INT, 0, &count)) {
        DXSetError(ERROR_BAD_PARAMETER, 
                 "'%s' must be an integer or integer list",
                 "which item");
        goto error;
    }
    
    memberlist = (int *)DXAllocate(sizeof(int) * count);
    if (!memberlist)
        goto error;
    
    if (!DXExtractParameter(numbers, TYPE_INT, 0, count, (Pointer)memberlist))
        goto error;
    
    
    /* look for illegal values */
    for (i=0; i<count; i++) {
	if (memberlist[i] >= items) {
	    if (items == 1) {
		DXSetError(ERROR_BAD_PARAMETER, 
			"'%s' must be 0 since the series has only one member",
			"which member");
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, 
	"'%s' (item %d) must be an integer or integer list between %d and %d",
		   "which member", i, 0, items-1);
	    }
	    goto error;
	}
    }

    /* sort list here into numeric order */
    qsort((Pointer)memberlist, count, sizeof(int), int_compare);

    src = (char *)DXGetArrayData(a);
    if (!src)
	goto error;

    size = DXGetItemSize(a);
    if (size <= 0)
	goto error;
    
    
    for (i=0, newi=0; i<items; i++) {

	/* anything on the list doesn't get copied */
	if (bsearch((Pointer)&i, (Pointer)memberlist, count, sizeof(int),
		    int_compare))
	    continue;
	
        src2 = src + (size * i);

	if (!DXAddArrayData(new, newi++, 1, (Pointer)src2))
	    goto error;
	
    }

    DXFree((Pointer)memberlist);
    return new;

  error:
    DXFree((Pointer)memberlist);
    DXDelete((Object)new);
    return ERROR;
}

/* this routine handles the case where string data comes in as a string object
 * instead of an array of type string.  a string object can contain only a
 * single item, so this can only succeed if the "which item" input is either
 * not set or is set and the value is 0.
 * this could have been handled by the omitnumberedlist code but it's so
 * different that it was cleaner to separate it out into a different routine.
 */
static Object 
omitnumberedstring(Object o, Object numbers)
{
    char *cp;
    int i;

    if (DXGetObjectClass(o) != CLASS_STRING) {
	DXSetError(ERROR_INTERNAL, 
		   "omitnumberedstring routine called on non-string input");
	return NULL;
    }

    cp = DXGetString((String)o);
    if (!cp) {
	DXSetError(ERROR_DATA_INVALID, 
		   "input is an empty string, must be group or list");
	return NULL;
    }

    /* default - delete first (and only) member of list. */
    if (numbers == NULL)
        return (Object)DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, 1);

    /* if numbers contains one item and the value is 0, return same as above.
     * otherwise if there is more than one item in the list
     * or the value isn't 0, it's an error.
     */
    if (DXExtractInteger(numbers, &i) && (i == 0))
        return (Object)DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, 1);


    DXSetError(ERROR_BAD_PARAMETER, 
	       "'%s' must be 0, since the string has only one item",
	       "which item");
    return NULL;
}

