/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <dx/dx.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "edf.h"


/* prototypes
 */
static Object namedobjfunc(struct finfo *f, char **namelist, int flag);
static Object numberedobjfunc(struct finfo *f, int *numlist, int idflag, int flag);

#if 0
static void printobjlist(struct finfo *f);   /* for debug */
#endif

struct hashidtolist {
    PseudoKey id;
    struct objlist *ol;
};


/* init list - create hash table.
 */
Error _dxfinitobjlist(struct finfo *f)
{
    if (!f)
	return ERROR;

    f->ol = NULL;

    f->ht = DXCreateHash(sizeof(struct hashidtolist), NULL, NULL);
    if (!f->ht)
	return ERROR;

    return OK;
}

/* delete list
 */
Error _dxfdeleteobjlist(struct finfo *f)
{
    struct objlist *this, *next;

    if (!f)
	return ERROR;

    this = f->ol;
    while(this) {

        next = this->next;
        if (this->name)
            DXFree((Pointer)this->name);

	if (this->odelete == OBJ_CANDELETE)
	    DXDelete(this->o);

	if (this->nrefs > 0)
	    DXFree((Pointer)this->refs);

	if (this->gp) {
	    DXFree((Pointer)this->gp->fname);
	    DXFree((Pointer)this->gp->gbuf);
	    DXFree((Pointer)this->gp);
	}

        DXFree((Pointer)this);
        this = next;
    }

    if (f->ht)
	DXDestroyHash(f->ht);

    f->ht = NULL;
    f->ol = NULL;
    return OK;
}

/* add a new object to the list.
 *  inputs are the object id, the pointer to the object, and optionally
 *  the name of the object (if name != NULL).  if the list is originally
 *  empty, initialize the objlist pointer also.  the new object is added
 *  to the head of the list - a fact the _dxfnumberedobjlist currently
 *  counts on to be able to tell which is the most recently defined object.
 */
Error _dxfaddobjlist(struct finfo *f, int id, Object o, char *name, int start)
{
    struct objlist *new;
    struct hashidtolist he;

    /* error if bad file ptr */
    if (!f)
        return ERROR;

    if (id < 0) {
	if (DXQueryDebug("M"))
	    DXMessage("id = -1, name = %s, o = 0x%08x", name?name:"NULL", o);
    }

    new = (struct objlist *)DXAllocateZero(sizeof(struct objlist));
    if (!new)
        return ERROR;

    if (name) {
        new->name = (char *)DXAllocateLocal(strlen(name)+1);
        if (!new->name) 
            return ERROR;
        strcpy(new->name, name);

	/* if there isn't already a name attribute, give it one */
	if (o) {
	    if (!DXGetAttribute(o, NAMEATTR))
		DXSetAttribute(o, NAMEATTR, (Object)DXNewString(name));
	}
    }

    if (o)
	DXReference(o);

    new->id = id;
    new->o = o;
    new->next = NULL;
    new->odelete = OBJ_CANDELETE;

    /* add new object block to start of list unless start == 0 */
    if (!start && f->ol) {
	new->next = f->ol->next;
	f->ol->next = new;
    } else {
	new->next = f->ol;
	f->ol = new;
    }

    /* add to hash table for quick lookup */
    he.id = id;
    he.ol = new;

    if (!DXInsertHashElement(f->ht, (Pointer)&he))
	return ERROR;

    DXDebug("O", "added object 0x%08x, number %d, name %s", o, id, 
                                                       name ? name : "<NULL>");
    return OK;
}


/* set the object associated with an id
 */
Error _dxfsetobjptr(struct finfo *f, int id, Object o)
{
    struct hashidtolist *he;
    long lid = (long)id;

    if (!f || !f->ol || !f->ht || id < 0)
        return ERROR;

    /* all objects should be in hash table.
     */
    he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lid);
    if (!he)
	return ERROR;

    /* if id is there, return ok if there isn't an object to set.
     */
    if (!o)
	return ERROR;

    he->ol->o = o;
    DXReference(o);
    
    if (he->ol->name) {
	/* if there isn't already a name attribute, give it one */
	if (o) {
	    if (!DXGetAttribute(o, NAMEATTR))
		DXSetAttribute(o, NAMEATTR, (Object)DXNewString(he->ol->name));
	}
    }

    return OK;
}

/* mark the object associated with an id used.
 */
Error _dxfsetobjused(struct finfo *f, int id)
{
    struct hashidtolist *he;
    long lid = (long)id;

    if (!f || !f->ol || !f->ht || id < 0)
        return ERROR;

    /* all objects should be in hash table.
     */
    he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lid);
    if (!he)
	return ERROR;
    
    he->ol->oused = OBJ_USED;
    return OK;
}

/* query if object associated with an id is used.
 */
Error _dxfisobjused(struct finfo *f, int id)
{
    struct hashidtolist *he;
    long lid = (long)id;

    if (!f || !f->ol || !f->ht || id < 0)
        return ERROR;

    he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lid);
    if (!he)
	return ERROR;
    
    return he->ol->oused == OBJ_USED ? OK : ERROR;
}

/* retrieve an object by id.  returns NULL if object id isn't in list.
 *  returns a pointer to the object, if the object is named, return a 
 *  pointer to the name in last parm.
 */
Error _dxflookobjlist(struct finfo *f, int id, Object *o, char **name)
{
    struct hashidtolist *he;
    long lid = (long)id;

    if (!f || !f->ol || !f->ht || id < 0)
        return ERROR;

    /* is object id in hash table?
     */
    he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lid);
    if (!he)
	return ERROR;

    if (o)
	*o = he->ol->o;
    if (name)
	*name = he->ol->name;

    return OK;
}

/* retrieve a pointer to the the getby struct from an object by id.
 */
struct getby **_dxfgetobjgb(struct finfo *f, int id)
{
    struct hashidtolist *he;
    long lid = (long)id;

    if (!f || !f->ol || !f->ht || id < 0)
        return NULL;

    /* is object id in hash table?
     */
    he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lid);
    if (!he)
	return NULL;

    return &(he->ol->gp);
}



/* used to keep track of which objects an object references.
 *  add an object to the ref list if it isn't already there.
 */
Error _dxfobjusesobj(struct finfo *f, int id, int refd_id)
{
    struct hashidtolist *he, *refd_he;
    struct objlist *this;
    long lid = (long)id;
    long lrefd_id = (long)refd_id;

    if (!f || !f->ol || !f->ht || id < 0 || refd_id < 0)
        return ERROR;

    /* all objects should already be in hash table.
     */
    he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lid);
    if (!he)
	return ERROR;

    refd_he = (struct hashidtolist *)DXQueryHashElement(f->ht, (Key)&lrefd_id);
    if (!refd_he)
	return ERROR;

    /* add an object to the list of objects referenced by this object 
     * (should this code check for dups first???)
     * this code needs to NOT realloc each time...
     */
    this = he->ol;
    if (this->nrefs == 0) {
        this->refs = (struct objlist **)DXAllocateLocal(sizeof(struct objlist *));
	this->refs[0] = refd_he->ol;
	this->nrefs++;
    } else {
        this->refs = (struct objlist **)DXReAllocate(this->refs, 
				sizeof(struct objlist *) * (this->nrefs +1));
	this->refs[this->nrefs] = refd_he->ol;
        this->nrefs++;
    }

    return OK;
}

/* flag values */
#define M_MAKE 1
#define M_MARK 2

/* traverse the objectlist, and collect named objects into a group to
 *  return.  if namelist isn't null, names must be in list to be returned.
 */
Object _dxfnamedobjlist(struct finfo *f, char **namelist)
{
    return namedobjfunc(f, namelist, M_MAKE);
}

/* traverse the objectlist and just mark the objects in the namelist as
 *  used.  then mark all the objects they use.
 */
Object _dxfmarknamedobjlist(struct finfo *f, char **namelist)
{
    return namedobjfunc(f, namelist, M_MARK);
}

/* common code for both functions above.
 */
static Object namedobjfunc(struct finfo *f, char **namelist, int flag)
{
    struct objlist *this, *first = NULL;
    Object o = NULL;
    int total = 0;
    int members = 0;
    int named = 0;
    int i, matched;
    char **cp;
    
    if (!f || !f->ol)
	return ERROR;
    
    /* traverse the list, keeping any object which matches the namelist.
     *  when you find the second match, create a group to hold the objects
     *  and return it at the end.  if there is only one match, return it.
     */
    
    for (this=f->ol; this; this = this->next) {
	/* count total number of objects */
	total++;
	
	/* skipped unnamed objects */
	if (!this->name) 
	    continue;
	
	named++;
	
	if (!namelist || !namelist[0])
	    matched = 1;
	else {
	    matched = 0;
	    for (cp=namelist; *cp; cp++) {
		if (strcmp(*cp, this->name))
		    continue;
		
		matched = 1;
		break;
	    }
	}
	
	if (!matched)
	    continue;
	
	this->oused = OBJ_USED;
	
	/* are we really making objects or just marking them used?
	 */
	if (flag == M_MAKE) {

	    /* mark it nodelete until we decide if it is going to be
             * the toplevel (or only) object returned.
	     */
	    this->odelete = OBJ_NODELETE;

	    /* if this is the second match, make a group and put the first
	     *  member in it.
	     */
	    if (first && !o) {
		o = (Object)DXNewGroup();
		if (!o)
		    return ERROR;
		
		o = (Object)DXSetMember((Group)o, first->name, first->o);
		if (!o)
		    return ERROR;
		
		first->odelete = OBJ_CANDELETE;
	    }
	    
	    /* if this is the first object, remember it, else add it to group.
	     */
	    if (!first) {
		first = this;
		
	    } else {
		o = (Object)DXSetMember((Group)o, this->name, this->o);
		if (!o)
		    return ERROR;
		
		this->odelete = OBJ_CANDELETE;

	    }

	} else {
	    /* if just marking, still remember the first object marked */
	    if (!first)
		first = this;
	}

	members++;

    }
    
    /* if we are marking objects used, mark each refd object used also
     *
     *  can we cheat here because objects are added to head of the list
     *   so all references should only be forward?  (yes for now)
     */
    if (flag == M_MARK) {
	for (this=f->ol; this; this = this->next) {
	    
	    if (this->oused != OBJ_USED)
		continue;
	    
	    for (i = 0; i < this->nrefs; i++)
		this->refs[i]->oused = OBJ_USED;
	}
    }
    
    
    /* either no named objects, or no matches with list.  the exception
     *  is if there is only a single object in the file - then return it.
     */
    if (members == 0) {
	if (total == 1) {
	    this = f->ol;
	    if (M_MARK)
		this->oused = OBJ_USED;
	    else
		this->odelete = OBJ_NODELETE;
	    return this->o;
	}  
	
	if (!named) {
	    DXSetError(ERROR_DATA_INVALID, "#10740");
	    return NULL;
	} else {
	    DXSetError(ERROR_DATA_INVALID, "#10742");
	    return NULL;
	}
    }
    
    /* if exactly one object found, return it; else return group.
     */
    if (members == 1)
	return first->o;
    
    return o;
}


/* traverse the objectlist, and collect the requested object numbers into 
 *  a group to return.  unlike _dxfnamedobjlist(), if numlist is null, don't
 *  try to return all numbered objects, just return NULL.  if idflag = 0,
 *  use user numbers.  if idflag = 1, use the real id numbers.  if idflag
 *  = 2, just return the first object in the list, which was the last
 *  object defined from this file.
 */
Object _dxfnumberedobjlist(struct finfo *f, int *numlist, int idflag)
{
    return numberedobjfunc(f, numlist, idflag, M_MAKE);
}

/* same as above but only mark the used obj numbers and then mark
 * the objects they use.
 */
Object _dxfmarknumberedobjlist(struct finfo *f, int *numlist, int idflag)
{
    return numberedobjfunc(f, numlist, idflag, M_MARK);
}

/* common code for both functions above.
 */
static Object numberedobjfunc(struct finfo *f, int *numlist, int idflag, 
			      int flag)
{
    struct objlist *this, *first = NULL;
    Object o = NULL;
    int members = 0;
    int named = 0;
    int i, matched;
    int *ip;

    if (!f || !f->ol)
        return ERROR;

    /* idflag == 2 means mark/return only most recently defined object 
     * if you aren't asking for the last object, you have to give a valid
     * number list.
     */
    if (idflag == 2) {

	this = f->ol;
	if (flag == M_MARK) {
	    this->oused = OBJ_USED;
	    members = 1;
	    first = this;

	    /* continue down to the section where we mark all the
	     * objects referenced by this one.
	     */

	} else {
	    /* return here - we are done */

	    this->odelete = OBJ_NODELETE;
	    return this->o;
	}

    } else {

	if (!numlist)
	    return ERROR;

	/* traverse the list, assuming the return object will be a group.  
	 * when you get to the end, if there is only one member in the group, 
	 * delete the group and return the single member.
	 */
	
	for (this=f->ol; this; this = this->next) {
	    
	    if (this->name)
		named++;
	    
	    matched = 0;
	    for (ip=numlist; *ip >= 0; ip++) {
		if ((!idflag && ((USER_ID(*ip) != this->id)))
		    || (idflag && (*ip != this->id)))
		    continue;
		
		matched = 1;
		break;
	    }
	    
	    if (!matched)
		continue;
	    
	    this->oused = OBJ_USED;
	    
	    /* are we really making objects or just marking them used?
	     */
	    if (flag == M_MAKE) {
		
		/* mark it nodelete until we decide if it is going to be
		 * the toplevel (or only) object returned.
		 */
		this->odelete = OBJ_NODELETE;
		
		/* if this is the second match, make a group and put the first
		 *  member in it.
		 */
		if (first && !o) {
		    o = (Object)DXNewGroup();
		    if (!o)
			return ERROR;
		    
		    o = (Object)DXSetMember((Group)o, first->name, first->o);
		    if (!o)
			return ERROR;
		    
		    first->odelete = OBJ_CANDELETE;
		}
		
		/* if the first object, remember it, else add it to group.
		 */
		if (!first) {
		    first = this;
		    
		} else {
		    o = (Object)DXSetMember((Group)o, this->name, this->o);
		    if (!o)
			return ERROR;
		    
		    this->odelete = OBJ_CANDELETE;
		}
		
		
	    } else {
		/* if just marking, still remember the first object marked */
		if (!first)
		    first = this;
	    }
	    
	    
	    members++;

	}
    }

    /* if we are marking objects used, mark each refd object used also.
     *
     *  can we cheat here because objects are added to head of the list
     *   so all references should only be forward?  (yes for now)
     */
    if (flag == M_MARK) {
      for (this=f->ol; this; this = this->next) {
	
	if (this->oused != OBJ_USED)
	  continue;
	
	for (i = 0; i < this->nrefs; i++)
	  this->refs[i]->oused = OBJ_USED;

      }
    }
    
    
    /* either no named objects, or no matches with list 
     */
    if (members == 0) {
	DXSetError(ERROR_DATA_INVALID, "#10746");
	return NULL;
    }

    /* if exactly one object found, return it; else return group.
     */
    if (members == 1)
	return first->o;

    return o;
}


/* for debug, dump object list
 */
#ifdef DEBUG
static void printobjlist(struct finfo *f)
{
    struct objlist *this;
    
    for (this=f->ol; this; this = this->next) {
	
	DXMessage("id=%d, obj=%x, name=%s", this->id, this->o, this->name);
	DXMessage(" used=%d, del=%d, nrefs=%d", this->oused, this->odelete,
		  this->nrefs);
    }
}
#endif
 


/* 
 * hash on object and return id 
 */

struct hashobjtoid {
    Object o;
    int id;
    int literal;
};


/* init list - create hash table.
 */
Error _dxfinit_objident(HashTable *ht)
{
    if (!ht)
	return ERROR;

    *ht = DXCreateHash(sizeof(struct hashobjtoid), NULL, NULL);
    if (!*ht)
	return ERROR;

    return OK;
}

/* delete list
 */
Error _dxfdelete_objident(HashTable ht)
{
    int rc;

    if (!ht)
	return ERROR;

    rc = DXDestroyHash(ht);

    return rc;
}

/* add a new object to the hash table.
 *  inputs are the object, the associated id, and a flag to say whether
 *  this is a literal number or a dictionary id.
 */
Error _dxfadd_objident(HashTable ht, Object o, int id, int literal)
{
    struct hashobjtoid he;

    if (!ht || !o)
        return ERROR;

    /* add to hash table for quick lookup */
    he.o = o;
    he.id = id;
    he.literal = literal;

    if (!DXInsertHashElement(ht, (Pointer)&he))
	return ERROR;

    if (DXQueryDebug("O")) {
	DXDebug("O", "added object 0x%08x, %s %d", 
	      o, literal ? "number" : "dictionary string", id);
    }
    return OK;
}

/* return the id associated with an object if it is already in the table.
 *  if not found, return NULL id.  only return error if something bad happens.
 */
Error _dxflook_objident(HashTable ht, Object o, int *id, int *literal)
{
    struct hashobjtoid *he;

    if (!ht || !o || !id || !literal)
        return ERROR;

    /* check for object in hash table.
     */
    he = (struct hashobjtoid *)DXQueryHashElement(ht, (Key)&o);
    if (!he)
	*id = 0;
    else {
	*id = he->id;
	*literal = he->literal;
    }

    return OK;
}


