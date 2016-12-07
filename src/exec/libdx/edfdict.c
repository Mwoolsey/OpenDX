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

/* keywords and dictionary code */

/* so what was faster?
 */    
#define TIMING 1

/* prototypes
 */

static char *dictplace(struct dict *d, int len);

static Error initkeywords(struct dict *d);
static Error putkeyinfo(struct dict *d, char *word, int value);
static char *strcpy_lc(char *a, char *b);

#if TIMING
#else
static int strcmp_lc(char *a, char *b);
#endif

static PseudoKey hashFunc(char *str);
static int hashCompare(Key search, Element matched);



#define LONGESTKEY  20	/* set this to longer than the longest keyword */

/* keyword to internal id map 
 */
static struct keywords {
    char *word;
    int id;
} keywords[] = {
    {    "object",		KW_OBJECT 	},
    {    "group",		KW_GROUP 	},
    {    "array",		KW_ARRAY 	},
    {    "field",		KW_FIELD 	},
    {    "string",		KW_STRING 	},
    {    "transform",           KW_TRANSFORM 	},
    {    "xform",               KW_TRANSFORM 	},
    {    "matrix",              KW_MATRIX 	},
    {    "light",               KW_LIGHT 	},
    {    "clipped",             KW_CLIPPED 	},
    {    "screen",              KW_SCREEN 	},
    {    "file",		KW_FILE 	},
    {    "alias",		KW_ALIAS 	},
    {    "series",		KW_SERIES 	},
    {    "attribute", 		KW_ATTRIBUTE 	},
    {    "value",		KW_VALUE 	}, 
    {    "component",		KW_COMPONENT 	},
    {    "member",		KW_MEMBER 	},
    {    "class",		KW_CLASS 	},
    {    "type",		KW_TYPE 	},
    {    "char",		KW_CHAR 	},
    {    "byte",		KW_CHAR 	},
    {    "short",		KW_SHORT 	},
    {    "int",			KW_INTEGER 	},
    {    "integer",		KW_INTEGER 	},
    {    "hyper",		KW_HYPER 	},
    {    "float",            	KW_FLOAT 	},
    {    "double",           	KW_DOUBLE 	},
    {    "category",         	KW_CATEGORY 	},
    {    "real",             	KW_REAL 	},
    {    "complex",          	KW_COMPLEX 	},
    {    "quaternion",       	KW_QUATERNION 	},
    {    "rank",             	KW_RANK 	},
    {    "shape",            	KW_SHAPE 	},
    {    "item",           	KW_COUNT 	},
    {    "items",           	KW_COUNT 	},
    {    "count",            	KW_COUNT 	},
    {    "counts",           	KW_COUNT 	},
    {    "data",             	KW_DATA 	},
    {    "datum",             	KW_DATA 	},
    {    "xdr",              	KW_XDR 		},
    {    "text",             	KW_ASCII 	},
    {    "ieee",             	KW_IEEE 	},
    {    "ascii",            	KW_ASCII 	},
    {    "follows",          	KW_FOLLOWS 	},
    {    "follow",          	KW_FOLLOWS 	},
    {    "regulararray",     	KW_REGULARARRAY },
    {    "productarray",     	KW_PRODUCTARRAY },
    {    "mesharray",        	KW_MESHARRAY 	},
    {    "path",         	KW_PATHARRAY 	},
    {    "patharray",        	KW_PATHARRAY 	},
    {    "camera",           	KW_CAMERA 	},
    {    "direction",        	KW_DIRECTION 	},
    {    "location",         	KW_LOCATION 	},
    {    "position",         	KW_POSITION 	},
    {    "size",             	KW_SIZE 	},
    {    "color",            	KW_COLOR 	},
    {    "compositefield",   	KW_COMPOSITE 	},
    {    "composite",        	KW_COMPOSITE 	},
    {    "name",             	KW_NAME 	},
    {    "of",			KW_OF 		},
    {    "by",			KW_BY 		},
    {    "times", 		KW_TIMES 	},
    {    "plus",		KW_PLUS 	},
    {    "term", 		KW_TERM 	},
    {    "origin",		KW_ORIGIN 	},
    {    "origins",		KW_ORIGIN 	},
    {    "delta",		KW_DELTAS 	},
    {    "deltas",		KW_DELTAS 	},
    {    "grid",		KW_GRID 	},
    {    "gridpositions",	KW_GRIDPOSITIONS },
    {    "gridconnections",	KW_GRIDCONNECTIONS },
    {    "distant",		KW_DISTANT 	},
    {    "local", 		KW_LOCAL 	},
    {    "from",		KW_FROM 	},
    {    "to",			KW_TO 		},
    {    "width",		KW_WIDTH 	},
    {    "height",		KW_HEIGHT 	},
    {    "resolution",		KW_RESOLUTION 	},
    {    "aspect",		KW_ASPECT 	},
    {    "up",			KW_UP 		},
    {    "number",		KW_NUMBER 	},
    {    "msb",			KW_MSB 		},
    {    "lsb", 		KW_LSB 		},
    {    "mode", 		KW_MODE 	},
    {    "binary", 		KW_BINARY 	},
    {    "meshoffsets",        	KW_MESHOFFSETS	},
    {    "default",        	KW_DEFAULT	},
    {    "line",        	KW_LINES	},
    {    "lines",        	KW_LINES	},
    {    "angle",        	KW_ANGLE	},
    {    "ambient",        	KW_AMBIENT	},
    {    "world",        	KW_WORLD	},
    {    "viewport",        	KW_VIEWPORT	},
    {    "pixel",        	KW_PIXEL	},
    {    "behind",        	KW_BEHIND	},
    {    "inside",        	KW_INSIDE	},
    {    "infront",        	KW_INFRONT	},
    {    "stationary",        	KW_STATIONARY	},
    {    "orthographic",       	KW_ORTHOGRAPHIC	},
    {    "perspective",        	KW_PERSPECTIVE	},
    {    "named",        	KW_NAMED	},
    {    "constantarray",       KW_CONSTANTARRAY },
    {    "signed",              KW_SIGNED       },
    {    "unsigned",            KW_UNSIGNED     },
    {    "multigrid",           KW_MULTIGRID    },
    /* add more here, and add #defines for KW_xxx in edf.h */
    {    "end",              	KW_END 		},
    {     NULL,			KW_NULL 	}
};


/* compare input word to a list of known keywords.  return the keyword value
 *  if it matches, else return KW_NULL.
 */
int _dxfiskeyword(struct dict *d, char *word)
{
#if TIMING
    struct kinfo *ki, search;
    char lcase[LONGESTKEY];

    if (!d || !word)
        return BADINDEX;

#if DEBUG_MSG
    DXDebug("D", "looking for %s in keyword dictionary", word);
#endif

    if (strlen(word) >= LONGESTKEY)
	return KW_NULL;

    strcpy_lc(lcase, word);

    search.key = hashFunc(lcase);
    search.contents = lcase;

    if ((ki = (struct kinfo *)DXQueryHashElement(d->keytab, 
					       (Pointer)&search)) == NULL)
	return KW_NULL;

    return ki->value;
#else
    struct keywords *kp = keywords;

    while(kp->word) {
	if (!strcmp_lc(word, kp->word))
	    return kp->id;
	kp++;
    }
    return KW_NULL;
#endif
}


/* return the string associated with the keyword ID.
 *  used for error messages.
 */
char *_dxflookkeyword(int id)
{
    struct keywords *kp = keywords;

    /* linear search should be good enough since this is only 
     *  used for error messages.
     */
    while(kp->id != KW_NULL) {
	if (id == kp->id)
	    return kp->word;

	kp++;
    }

    /* not found */
    return NULL;
}

/* add a new keyword to dictionary and set value.  
 */
static Error putkeyinfo(struct dict *d, char *word, int value)
{
    struct kinfo ki;

    if (!d || !word)
        return ERROR;

#if DEBUG_MSG
    DXDebug("D", "adding %s to dictionary", word);
#endif

    ki.key = hashFunc(word);
    ki.contents = dictplace(d, strlen(word)+1);
    if (!ki.contents)
	return ERROR;
    strcpy(ki.contents, word);
    ki.value = value;

    if (!DXInsertHashElement(d->keytab, (Pointer)&ki))
	return ERROR;

#if DEBUG_MSG
    DXDebug("D", "setting %s to value %d", word, value);
#endif

    return OK;
}

static Error initkeywords(struct dict *d)
{
    struct keywords *kp = keywords;

    /* add each keyword to the hash table.
     */
    while (kp->word) {

	if (!putkeyinfo(d, kp->word, kp->id))
	    return ERROR;

	kp++;
    }

    return OK;
}


/* given a string and a lower case string, do a strcmp ignoring the
 *  case of the first string.  return 1 if not equal, 0 if equal.
 */
#if TIMING
#else
/* Only used if TIMING is 0 */
static int strcmp_lc(char *a, char *b)
{
    if (!a || !b)
	return 1;

    while(*a && *b) {
	if (*a != *b && (!isupper(*a) || tolower(*a) != *b))
	    return 1;

	a++, b++;
    }
    
    if (!*a && !*b)
	return 0;

    return 1;
}
#endif

/* copy b to a, changing all chars to lower case.  return NULL on error.
 */
static char *strcpy_lc(char *a, char *b)
{
    char *c = a;

    if (!a || !b)
	return NULL;

    while(*b) {
	*a = (isupper(*b) ? tolower(*b) : *b);

	a++, b++;
    }
    
    *a = '\0';

    return c;
}




/* 
 * dictionary section.  
 */

static PseudoKey hashFunc(char *str)
{
    unsigned long h = 1;
    while(*str)
	h = (17*h + *str++);
    return (PseudoKey)h;
 
}

static int hashCompare(Key search, Element matched)
{
    return (strcmp(((struct dinfo *)search)->contents,
		   ((struct dinfo *)matched)->contents));
}

static char *dictplace(struct dict *d, int len)
{
    char *place;
    struct localstor *ls, *newls;

    if (len >= LOCALSTORBLOCK)
	return NULL;

    ls = d->localdict;
    if (!ls)
	return NULL;

    /* if not enough space is allocated, insert new block at head of list
     *  so d->localstor always contains the block from which allocation
     *  is happening.
     */
    if (ls->bytesleft < len) {
	newls = (struct localstor *)
	        DXAllocateLocalZero(sizeof(struct localstor));
	if (!newls)
	    return NULL;
    
	newls->bytesleft = LOCALSTORBLOCK;
	newls->nextbyte = 0;
	newls->next = ls;
	d->localdict = newls;
	ls = newls;
    }

    place = ls->contents + ls->nextbyte;

    ls->bytesleft -= len;
    ls->nextbyte += len;
    
    return place;
}

/* allocate dictionary and add known keywords.
 */
Error _dxfinitdict(struct dict *d)
{
    if (!d)
        return ERROR;

    d->dicttab = DXCreateHash(sizeof(struct dinfo), NULL, hashCompare);
    if (!d->dicttab)
	goto error;
    
    d->keytab = DXCreateHash(sizeof(struct kinfo), NULL, hashCompare);
    if (!d->keytab)
	goto error;
    
    d->localdict = (struct localstor *)
	           DXAllocateLocalZero(sizeof(struct localstor));
    if (!d->localdict)
	goto error;
    
    d->localdict->nextbyte = 0;
    d->localdict->bytesleft = LOCALSTORBLOCK;
    d->localdict->next = NULL;

    d->nextid = 1;

    if (!initkeywords(d))
	goto error;

    return OK;

  error:
    _dxfdeletedict(d);
    return ERROR;
}

/* free dictionary
 */
Error _dxfdeletedict(struct dict *d)
{
    struct localstor *ls, *next;

    if (!d)
	return ERROR;
    
    if (d->dicttab) {
	DXDestroyHash(d->dicttab);
	d->dicttab = NULL;
    }

    if (d->keytab) {
	DXDestroyHash(d->keytab);
	d->keytab = NULL;
    }

    ls = d->localdict;
    while(ls) {
	next = ls->next;
	DXFree((Pointer)ls);
	ls = next;
    }
	
    d->localdict = NULL;
    d->nextid = BADINDEX;

    return OK;
}


/* add a string to a lookup table.  used to support named objects
 *  and aliases.  if string is already in table, return id.  if not,
 *  create new slot and return id.
 */
int _dxfputdict(struct dict *d, char *word)
{
    struct dinfo di;
    int id;

    if (!d || !word)
        return BADINDEX;

    if ((id = _dxflookdict(d, word)) != BADINDEX)
	return id;

#if DEBUG_MSG
    DXDebug("D", "adding %s to dictionary", word);
#endif

    di.key = hashFunc(word);
    di.contents = dictplace(d, strlen(word)+1);
    if (!di.contents)
	return BADINDEX;
    strcpy(di.contents, word);
    di.type = STRING;
    di.value = 0;

    if (!DXInsertHashElement(d->dicttab, (Pointer)&di))
	return BADINDEX;

    return _dxflookdict(d, word);
}


/* look up a string in the lookup table.  if not found, return error.
 */
int _dxflookdict(struct dict *d, char *word)
{
    struct dinfo *di, search;

    if (!d || !word)
        return BADINDEX;

#if DEBUG_MSG
    DXDebug("D", "looking for %s in dictionary", word);
#endif

    search.key = hashFunc(word);
    search.contents = word;

    if ((di = (struct dinfo *)DXQueryHashElement(d->dicttab, 
					       (Pointer)&search)) == NULL)
	return BADINDEX;

    return (int)((byte *)di - (byte *)d->dicttab);

}



/* return the string associated with a dictionary slot.
 */
char *_dxfdictname(struct dict *d, int slot)
{
    struct dinfo *di;

    if (!d || !slot)
        return NULL;

    di = (struct dinfo *)((byte *)d->dicttab + slot);
    return di->contents;
}


/* mark an existing string in the dictionary as an alias for an object.
 *  generate a new unique object id and return it.  the SYS_ID() macro
 *  makes sure internally generated numbers don't collide with numbers
 *  assigned to objects by the user.
 */
Error _dxfsetdictalias(struct dict *d, int slot, int *value)
{
    struct dinfo *di;

    if (!d || !slot)
        return ERROR;

    di = (struct dinfo *)((byte *)d->dicttab + slot);

    di->type = ALIAS;
    di->value = SYS_ID(d->nextid);
    *value = SYS_ID(d->nextid);
    d->nextid++;

#if DEBUG_MSG
    DXDebug("D", "setting %s to alias value %d", di->contents, di->value);
#endif

    return OK;
}

/* return the type and value associated with a dictionary slot
 */
Error _dxfgetdictinfo(struct dict *d, int slot, int *type, int *value)
{
    struct dinfo *di;

    if (!d || !slot)
        return ERROR;

    di = (struct dinfo *)((byte *)d->dicttab + slot);

    if (type)
	*type = di->type;
    if (value)
	*value = di->value;

    return OK;

}

/* return the type and value associated with a dictionary slot
 * it would be nice if this could be fast somehow.
 */
Error _dxfgetdictalias(struct dict *d, int alias, int *value)
{
    struct dinfo *di;

    /* this is hashed on string value, so to look up an alias number
     *  it has to be a linear search.
     */
    
    if (!DXInitGetNextHashElement(d->dicttab))
	return ERROR;
    
    while ((di = (struct dinfo *)DXGetNextHashElement(d->dicttab)) != NULL) {
	if (di->type != ALIAS)
	    continue;
	if (di->value == alias) {
	    *value = (int)((byte *)di - (byte *)d->dicttab);
	    return OK;
	}
    }

    return ERROR;
}

/* allocate just a unique id from the dictionary list so we are 
 *  guarenteed not to collide with other valid object ids.
 */
int _dxfgetuniqueid(struct dict *d)
{
    int value;

    if (!d)
	return -1;
    
    value = SYS_ID(d->nextid);
    d->nextid++;

    return value;
}

/* for debug, print contents of dictionary
 */
#ifdef DEBUG
static void prdictinfo(struct dict *d)
{
    struct dinfo *di;
    struct kinfo *ki;
    int i;

    /* dictionary */
    i = 0;

    if (!DXInitGetNextHashElement(d->dicttab))
	return;

    while((di = (struct dinfo *)DXGetNextHashElement(d->dicttab)) != NULL) {
	DXMessage("%d: %08x %d %d %08x %s", i, di->key, di->type, di->value, 
		(unsigned int)&di->contents, di->contents);
	  
	i++;
    }

    /* keyword table */
    i = 0;
    if (!DXInitGetNextHashElement(d->keytab))
	return;

    while((ki = (struct kinfo *)DXGetNextHashElement(d->keytab)) != NULL) {
	DXMessage("%d: %08x %d %d %08x %s", i, ki->key, ki->value, 
		(unsigned int)&ki->contents, ki->contents);
	  
	i++;
    }

    return;
}
#endif
