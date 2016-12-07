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

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_SYS_SIGNAL_H)
#include <sys/signal.h>
#endif

#if defined(HAVE_TYPES_H)
#include <types.h>
#endif

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#include <setjmp.h>
#include "diskio.h"

extern Error _dxfByteSwap(void *, void*, int, Type);


#if !defined(DXD_STANDARD_IEEE)
extern void _dxffloat_denorm(int *, int, int *, int *);
extern void _dxfdouble_denorm(int *, int, int *, int *);
#endif

/* for debugging */
static void badaddr(void)
{
}

#define MARKTIME  0  /* turn off if timing not needed */

#if defined(WORDS_BIGENDIAN)
#define MSB_MACHINE 1
#else
#define MSB_MACHINE 0
#endif

/* label section */

/* header which i expect to find at the beginning of the file
 */
struct f_label {
    int version;	/* change this whenever the format changes */
    int cutoff;		/* threshold for arrays: in header or separate blk */
    int blocksize;	/* number of bytes per data block */
    int headerbytes;	/* number of bytes in header */
    int type;		/* for series group support (append series) */
    int format;		/* binary or dx format */
    int leftover;	/* number of bytes unused at end of last block */
    int byteorder;	/* msb or lsb value from WORD_BIGENDIAN determined at configure */
};

#define DISKVERSION_A	 (int)0x010100aa  /* original format */
#define DISKVERSION_B	 (int)0x010100bb  /* new label block; mesh offsets */
#define DISKVERSION_C	 (int)0x010100cc  /* variable threshold: hdr vs body */
#define DISKVERSION_D	 (int)0x010100dd  /* new types and constant arrays */
#define DISKVERSION_E	 (int)0x010100ee  /* mark the byte order */
#define DISKVERSION_IE	 (int)0xee000101  /* inverted byte order version E */

#define DISKTYPE_NORMAL  1	/* normal object file */
#define DISKTYPE_SERIES  2	/* extra series pointers; can be appended to */
#define DISKTYPE_MOVIE   3	/* raw rgb file - no object structure */

#define DISKFORMAT_BIN	 1	/* completely binary */
#define DISKFORMAT_DX	 2	/* dx external data format */

static int version;             /* shouldn't be global, but this code */
static int cutoff;              /* won't be running in parallel, so */
static int needswab;            /* i can get away with it. */
static int oneblock;            /* 64k for array disk only, 512 all else */ 


/* hash stuff - used to be sure that shared objects are only written 
 *  to the file once, and that they are shared when being read back in.
 */

struct hashElement
{
    PseudoKey   key;		/* either an object or an offset */
    PseudoKey   data;		/* the other thing */
};

typedef struct hashElement *HashElement;

#define HASH_EMPTY      (uint)0
#define HASH_INPROGRESS (uint)1

/* used during write-object-to-file.
 * FindOffest looks for entry keyed by object memory address.  if found, 
 *  returns byte offset in file where object starts.  if not found, returns
 *  with Offset == EMPTY (0).  
 * AddOffset is called at the top of the function to set either a temp flag
 *  to indicate the object is in-progress, or else called with the actual
 *  byteoffset and object address.
 * ReplaceOffset is called to change the in-progress flag to the real
 *  byte offset/object address at the end.
 */
static HashTable
FindOffset(HashTable ht, Object object, uint *offset)
{
    HashElement eltPtr;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&object);
    if (eltPtr)
	*offset = (uint)eltPtr->data;
    else
	*offset = HASH_EMPTY;	/* 0 -- an illegal offset for objects */

    return ht;
}


static HashTable
AddOffset(HashTable ht, Object object, uint offset)
{
    struct hashElement elt;
    HashElement        eltPtr;

    elt.key = (PseudoKey)object;
    elt.data = (PseudoKey)offset;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&object);
    if (eltPtr)
	return NULL;   /* shouldn't already be there */

    if (! DXInsertHashElement(ht, (Element)&elt))
	return NULL;
    
    return ht;
}

static HashTable
ReplaceOffset(HashTable ht, Object object, uint offset)
{
    HashElement        eltPtr;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&object);
    if (!eltPtr)
	return NULL;   /* should be there already */

    eltPtr->data = offset;

    return ht;
}

/* used during read-from-file.
 * FindObject key is byte offset, and if found returns object memory address.
 *  if not found, returns outObject == NULL.
 * AddObject adds the object to the hash table after it has been read 
 *  in completely.
 */
static HashTable
FindObject(HashTable ht, uint offset, Object *outObject)
{
    HashElement eltPtr;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&offset);
    if (eltPtr)
	*outObject = (Object)eltPtr->data;
    else
	*outObject = NULL;

    return ht;
}

static HashTable
AddObject(HashTable ht, uint offset, Object object)
{
    struct hashElement elt;
    HashElement        eltPtr;

    elt.data = (PseudoKey)object;
    elt.key = (PseudoKey)offset;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&offset);
    if (eltPtr)
	return NULL;  /* shouldn't already be there */

    if (! DXInsertHashElement(ht, (Element)&elt))
	return NULL;
    
    return ht;
}

/* my own id's for object classes, since they are subject to change
 */
#define OBJ_MIN			0x0E01	/* illegal */
#define OBJ_OBJECT 		0x0E02	/* generic? */
#define OBJ_PRIVATE		0x0E03	/* can't do much about saving these */
#define OBJ_STRING		0x0E04	/* */
#define OBJ_FIELD		0x0E05  /* */
#define OBJ_GROUP		0x0E06	/* */
#define OBJ_SERIES		0x0E07	/* */
#define OBJ_COMPOSITEFIELD	0x0E08	/* */
#define OBJ_PAD                 0x0E09  /* obsolete */
#define OBJ_ARRAY		0x0E0A	/* */
#define OBJ_REGULARARRAY	0x0E0B	/* */
#define OBJ_PATHARRAY           0x0E0C	/* */
#define OBJ_PRODUCTARRAY	0x0E0D	/* */
#define OBJ_MESHARRAY		0x0E0E	/* */
#define OBJ_XFORM		0x0E0F	/* */
#define OBJ_SCREEN		0x0E10	/* */
#define OBJ_CLIPPED		0x0E11	/* */
#define OBJ_CAMERA		0x0E12	/* */
#define OBJ_LIGHT		0x0E13	/* */
#define OBJ_MAX			0x0E14	/* illegal */
#define OBJ_DELETED		0x0E15	/* illegal */
#define OBJ_CONSTANTARRAY	0x0E16	/* new 16feb93 */
#define OBJ_MULTIGRID		0x0E17	/* new 29mar93 */

/* ditto for array types and array categories (basically anything which
 *  is implemented as an enum has to worry about this).
 */
#define ARRAY_UBYTE             0x0D01  /* unsigned byte */
#define ARRAY_BYTE              0x0D02  /* signed byte */
#define ARRAY_USHORT            0x0D03  /* unsigned short */
#define ARRAY_SHORT             0x0D04  /* signed short */
#define ARRAY_UINT              0x0D05  /* unsigned int */
#define ARRAY_INT               0x0D06  /* signed int */
#define ARRAY_UHYPER            0x0D07  /* unsigned 64bit int */
#define ARRAY_HYPER             0x0D08  /* signed 64bit int */
#define ARRAY_FLOAT             0x0D09  /* ieee float */
#define ARRAY_DOUBLE            0x0D0A  /* ieee double */
#define ARRAY_STRING            0x0D0B  /* string arrays */

#define ARRAY_REAL              0x0D81  /* real */
#define ARRAY_COMPLEX           0x0D82  /* complex */
#define ARRAY_QUATERNION        0x0D83  /* quaternion */


#define INC_VOID(p, n)	p = (Pointer)((ulong)p + ((n+3) & ~3))
#define INC_BYTE(p, n)	p += (n+3) & ~3

#define MAXRANK		16       /* max number of dims */

#define F_HEADER	0x01
#define F_BODY		0x02
#define F_DUPLICATE	0x04

#define CAM_ORTHO	1
#define CAM_PERSPECT	2

#define LITE_AMBIENT     1
#define LITE_DISTANT     2
#define LITE_CAM_DISTANT 3
#define LITE_LOCAL       4
#define LITE_CAM_LOCAL   5


typedef unsigned int Offset;              /* byte offset in file */

/* per-object info */
struct i_object {
    int flags;                            /* header/body/duplicate */
    Class class;
    int attrcount;
    /* attribute list follows, then object body follows */
};

struct i_named_obj {
    int namelength;
    /* name body follows, then object follows */
};

struct i_numbered_obj {
    int number;
    /* object follows */
};


struct i_string {
    int strlength;
    /* string body follows */
};

struct i_attributes {
    int namelength;
    /* name body follows, then object follows */
};

struct i_array {
    Type type;
    Category category;
    int rank;
    int shape[MAXRANK];
    int count;
    Class subclass;
    /* subclass array follows */
};

struct i_sharedarray {
    int id;
    long offset;
};

struct i_arrayarray {
    int datalength;
    /* data follows if inline */
    /* offset into body follows if not */
};

struct i_productarray {
    int listlength;
    /* list of array objects follow */
};

struct i_mesharray {
    int listlength;
    /* list of array objects follow */
    /* list of offsets follow */
};

struct i_field {
    int componentcount;
    /* list of names, objects follows */
};

struct i_transform {
    Matrix transform;
    /* object to be transformed follows */
};

struct i_light {
    int type;
    Point location;
    Vector direction;
    RGBColor color;
};

struct i_screen {
    int fixed;
    int position;
    /* screen object follows */
};

struct i_camera {
    int type;
    float width;
    float aspect;
    float angle;
    int resolution;
    Point from;
    Point to;
    Point up;
    RGBColor color;
};

struct i_group {
    Class subclass;
    int membercount;
    /* list of members follows */
};

struct i_groupgroup {
    int namelength;
    /* list of (name body, object) follows */
};

struct i_compositegroup {
    /* perhaps some additional info eventually here */
    int namelength;
    /* list of (name body, object) follows  */
};

struct i_seriesgroup {
    float position;
    /* object body follows */
};

static int
arrayitemsize(Type t, Category c, int rank, int *shape)
{
    int i, n;

    n = DXTypeSize(t) * DXCategorySize(c);
    for (i=0; i<rank; i++)
	n *= shape[i];

    return n;
}

#define GetArraySize(a, len)	{ DXGetArrayInfo((Array)a, &len, NULL, NULL, \
					   NULL, NULL); \
                                  len *= DXGetItemSize((Array)a); \
				  len = (len + 3) & ~3; }
#define GetStrLen(name)		(name ? strlen(name) : 0)
#define GetStrLenX(name)	(name ? (strlen(name)+1 + 3) & ~3 : 0)


#if DEBUGOBJECT
static char *
ClassNameString(Class c)
{
    switch(c) {    
      case CLASS_MIN:            return "min";
      case CLASS_OBJECT:         return "object";
      case CLASS_PRIVATE:        return "private";
      case CLASS_STRING:	 return "string";	
      case CLASS_FIELD:	       	 return "field";	
      case CLASS_GROUP:	       	 return "group";	
      case CLASS_SERIES:         return "series";
      case CLASS_COMPOSITEFIELD: return "compositefield";
      case CLASS_ARRAY:	       	 return "array";		
      case CLASS_REGULARARRAY:   return "regulararray";
      case CLASS_PATHARRAY:      return "patharray";    
      case CLASS_PRODUCTARRAY:   return "productarray";
      case CLASS_MESHARRAY:      return "mesharray";
      case CLASS_XFORM:	       	 return "xform";		
      case CLASS_SCREEN:	 return "screen";		
      case CLASS_CLIPPED:	 return "clipped";	
      case CLASS_CAMERA:	 return "camera";	
      case CLASS_LIGHT:	       	 return "light";	
      case CLASS_MAX:            return "max";
      case CLASS_DELETED:        return "deleted";        
      case CLASS_CONSTANTARRAY:  return "constantarray";        
      case CLASS_MULTIGRID:	 return "multigrid";        
      default: 			 return "unknown";
    }
    /* notreached */
}
#endif


static
int ConvertClassOut(Class c)
{
#if DEBUGOBJECT
    DXDebug("O", "object class %lu (0x%lx), name %s", 
	    (unsigned long)c, (unsigned long)c, ClassNameString(c));
#endif
    
    switch(c) {    
      case CLASS_MIN:            return OBJ_MIN;
      case CLASS_OBJECT:         return OBJ_OBJECT;
      case CLASS_PRIVATE:        return OBJ_PRIVATE;
      case CLASS_STRING:	 return OBJ_STRING;	
      case CLASS_FIELD:	       	 return OBJ_FIELD;	
      case CLASS_GROUP:	       	 return OBJ_GROUP;	
      case CLASS_SERIES:         return OBJ_SERIES;
      case CLASS_COMPOSITEFIELD: return OBJ_COMPOSITEFIELD;
      case CLASS_ARRAY:	       	 return OBJ_ARRAY;		
      case CLASS_REGULARARRAY:   return OBJ_REGULARARRAY;
      case CLASS_PATHARRAY:      return OBJ_PATHARRAY;    
      case CLASS_PRODUCTARRAY:   return OBJ_PRODUCTARRAY;
      case CLASS_MESHARRAY:      return OBJ_MESHARRAY;
      case CLASS_XFORM:	       	 return OBJ_XFORM;		
      case CLASS_SCREEN:	 return OBJ_SCREEN;		
      case CLASS_CLIPPED:	 return OBJ_CLIPPED;	
      case CLASS_CAMERA:	 return OBJ_CAMERA;	
      case CLASS_LIGHT:	       	 return OBJ_LIGHT;	
      case CLASS_MAX:            return OBJ_MAX;
      case CLASS_DELETED:        return OBJ_DELETED;        
      case CLASS_CONSTANTARRAY:  return OBJ_CONSTANTARRAY;        
      case CLASS_MULTIGRID:	 return OBJ_MULTIGRID;        
      default: 			 return -1;
    }

    /* not reached */
}


#if DEBUGOBJECT
static char *
TypeNameString(Type t)
{
    switch(t) {    
      case TYPE_UBYTE:		return "ubyte";
      case TYPE_BYTE:		return "byte";
      case TYPE_USHORT:		return "ushort";
      case TYPE_SHORT:		return "short";
      case TYPE_UINT:		return "uint";
      case TYPE_INT:		return "int";
   /* case TYPE_UHYPER:		return "uhyper"; */
      case TYPE_HYPER:		return "hyper";
      case TYPE_FLOAT:		return "float";
      case TYPE_DOUBLE:		return "double";
      case TYPE_STRING:		return "string";
      default:			return "unknown";
    }
    /* notreached */
}
#endif

static
int ConvertTypeOut(Type t)
{
#if DEBUGOBJECT
    DXDebug("O", "array type %lu, (0x%lx), name %s", 
	    (unsigned long)t, (unsigned long)t, TypeNameString(t) );
#endif
    
    switch(t) {    
      case TYPE_UBYTE:		return ARRAY_UBYTE;
      case TYPE_BYTE:		return ARRAY_BYTE;
      case TYPE_USHORT:		return ARRAY_USHORT;
      case TYPE_SHORT:		return ARRAY_SHORT;
      case TYPE_UINT:		return ARRAY_UINT;
      case TYPE_INT:		return ARRAY_INT;
   /* case TYPE_UHYPER:		return ARRAY_UHYPER; */
      case TYPE_HYPER:		return ARRAY_HYPER;
      case TYPE_FLOAT:		return ARRAY_FLOAT;
      case TYPE_DOUBLE:		return ARRAY_DOUBLE;
      case TYPE_STRING:		return ARRAY_STRING;
      default:			return -1;
    }

    /* notreached */
}



#if DEBUGOBJECT
static char *
CatNameString(Category c)
{
    switch(c) {    
      case CATEGORY_REAL:	return "real";
      case CATEGORY_COMPLEX:	return "complex";
      case CATEGORY_QUATERNION: return "quaternion";
      default:			return "unknown";
    }
    /* notreached */
}
#endif


static
int ConvertCatOut(Category c)
{
#if DEBUGOBJECT
    DXDebug("O", "array category %lu, (0x%lx), name %s", 
	    (unsigned long)c, (unsigned long)c, CatNameString(c) );
#endif
    
    switch(c) {    
      case CATEGORY_REAL:	return ARRAY_REAL;
      case CATEGORY_COMPLEX:	return ARRAY_COMPLEX;
      case CATEGORY_QUATERNION: return ARRAY_QUATERNION;
      default:			return -1;
    }

    /* notreached */
}

static
Class ConvertClassIn(int i, int version)
{
#if DEBUGOBJECT
    DXDebug("O", "disk object type %x", i);
#endif

    switch(version) {

      case DISKVERSION_A:
	switch(i) {
	  case 0:		return CLASS_MIN;
	  case 1:		return CLASS_OBJECT;
	  case 2:		return CLASS_STRING;
	  case 3:		return CLASS_FIELD;
	  case 4:		return CLASS_GROUP;
	  case 5:		return CLASS_SERIES;
	  case 6:		return CLASS_COMPOSITEFIELD;
/*	  case 7:		return CLASS_PYRAMID; */
/*	  case 8:		return CLASS_MIXEDFIELD; */
	  case 9:		return CLASS_ARRAY;
	  case 10:		return CLASS_REGULARARRAY;
	  case 11:		return CLASS_PATHARRAY;
	  case 12:		return CLASS_PRODUCTARRAY;
	  case 13:		return CLASS_MESHARRAY;
/*	  case 14:		return CLASS_INTERPOLATOR; */
/*	  case 15:		return CLASS_FIELDINTERPOLATOR; */
/*	  case 16:		return CLASS_GROUPINTERPOLATOR; */
/*	  case 17:		return CLASS_LINESRR1DINTERPOLATOR; */
/*	  case 18:		return CLASS_LINESRI1DINTERPOLATOR; */
/*	  case 19:		return CLASS_QUADSRR2DINTERPOLATOR; */
/*	  case 20:		return CLASS_QUADSII2DINTERPOLATOR; */
/*	  case 21:		return CLASS_TRISRI2DINTERPOLATOR; */
/*	  case 22:		return CLASS_CUBESRRINTERPOLATOR; */
/*	  case 23:		return CLASS_CUBESIIINTERPOLATOR; */
/*	  case 24:		return CLASS_TETRASINTERPOLATOR; */
/*	  case 25:		return CLASS_GROUPITERATOR; */
/*	  case 26:		return CLASS_ITEMITERATOR; */
	  case 27:		return CLASS_XFORM;
	  case 28:		return CLASS_SCREEN;
	  case 29:		return CLASS_CLIPPED;
	  case 30:		return CLASS_CAMERA;
	  case 31:		return CLASS_LIGHT;
	  case 32:		return CLASS_MAX;
	  case 33:		return CLASS_DELETED;
	}
	break;

      case DISKVERSION_B:
      case DISKVERSION_C:
      case DISKVERSION_D:
      case DISKVERSION_E:
	switch(i) {
	  case OBJ_OBJECT: 		return CLASS_OBJECT;
	  case OBJ_PRIVATE:		return CLASS_PRIVATE;       
	  case OBJ_STRING:		return CLASS_STRING;        
	  case OBJ_FIELD:		return CLASS_FIELD;         
	  case OBJ_GROUP:		return CLASS_GROUP;         
	  case OBJ_SERIES:		return CLASS_SERIES;        
	  case OBJ_COMPOSITEFIELD:	return CLASS_COMPOSITEFIELD;
	  case OBJ_ARRAY:		return CLASS_ARRAY;         
	  case OBJ_REGULARARRAY:	return CLASS_REGULARARRAY;  
	  case OBJ_PATHARRAY:		return CLASS_PATHARRAY;     
	  case OBJ_PRODUCTARRAY:	return CLASS_PRODUCTARRAY;  
	  case OBJ_MESHARRAY:		return CLASS_MESHARRAY;     
	  case OBJ_XFORM:		return CLASS_XFORM;         
	  case OBJ_SCREEN:		return CLASS_SCREEN;        
	  case OBJ_CLIPPED:		return CLASS_CLIPPED;       
	  case OBJ_CAMERA:		return CLASS_CAMERA;        
	  case OBJ_LIGHT:		return CLASS_LIGHT;         
	  case OBJ_CONSTANTARRAY: 	return CLASS_CONSTANTARRAY; 
	  case OBJ_MULTIGRID:	 	return CLASS_MULTIGRID; 
	}
	break;
	
      default:
	break;
    }
    
    DXSetError(ERROR_DATA_INVALID, "Unrecognized Object Class");
    return (Class) -1;
}


static
Type ConvertTypeIn(int i, int version)
{
#if DEBUGOBJECT
    DXDebug("O", "disk array type %x", i);
#endif

    switch(version) {

      case DISKVERSION_A:
      case DISKVERSION_B:
      case DISKVERSION_C:
	switch(i) {
	  case 0:			return TYPE_UBYTE;
	  case 1:			return TYPE_SHORT;
	  case 2:			return TYPE_INT;
	  case 3:			return TYPE_HYPER;
	  case 4:			return TYPE_FLOAT;
	  case 5:			return TYPE_DOUBLE;
	}
	break;

      case DISKVERSION_D:
      case DISKVERSION_E:
	switch(i) {                
	  case ARRAY_UBYTE:   	        return TYPE_UBYTE;         
	  case ARRAY_BYTE:	        return TYPE_BYTE;          
	  case ARRAY_USHORT:	        return TYPE_USHORT;        
	  case ARRAY_SHORT:	        return TYPE_SHORT;         
	  case ARRAY_UINT:	        return TYPE_UINT;          
	  case ARRAY_INT:	        return TYPE_INT;           
      /*  case ARRAY_UHYPER:		return TYPE_UHYPER;  */
	  case ARRAY_HYPER:	        return TYPE_HYPER;         
	  case ARRAY_FLOAT:	        return TYPE_FLOAT;         
	  case ARRAY_DOUBLE:	        return TYPE_DOUBLE;        
	  case ARRAY_STRING:	        return TYPE_STRING;        
	}
	break;
	
      default:
	break;
    }

    DXSetError(ERROR_DATA_INVALID, "Unrecognized Array Type");
    return (Type) -1;
}


static
Category ConvertCatIn(int i, int version)
{
#if DEBUGOBJECT
    DXDebug("O", "disk array category %x", i);
#endif

    switch(version) {

      case DISKVERSION_A:
      case DISKVERSION_B:
      case DISKVERSION_C:
	switch(i) {
	  case 0:		return CATEGORY_REAL;
	  case 1:		return CATEGORY_COMPLEX;
	  case 2:		return CATEGORY_QUATERNION;
	}
	break;

      case DISKVERSION_D:
      case DISKVERSION_E:
	switch(i) {
	  case ARRAY_REAL:		return CATEGORY_REAL;
	  case ARRAY_COMPLEX:		return CATEGORY_COMPLEX;
	  case ARRAY_QUATERNION:	return CATEGORY_QUATERNION;
	}
	break;

      default:
	break;
    }

    DXSetError(ERROR_DATA_INVALID, "Unrecognized Array Category");
    return (Category) -1;
}




/* sizeof header, sizeof body (where body is sizeof all arrays > cutoff)
 *
 * header is in bytes, body is in blocks.
 *
 * put test for recursive objects here because the code calls this
 * first before doing the actual read/write.  it has code to catch
 * loops at any level (e.g. A->B->C->D-> ... ->A will be flagged as bad).
 * at the top, new objects are added to the hash table with an offset
 * of 1, and aren't change to the real byte offset until the end.  if
 * at any time during the recursion of the object if a reference to the
 * current object is found, it will be in the hash table as in-progress
 * and can be caught.
 * 
 */
static
Error size_estimate(Object o, int *header, int *body, int doattr, 
		    int cutoff, HashTable ht)
{
    int count, i, len;
    Type type;
    float pos;
    Array terms[MAXRANK];
    Object subo, subo1, subo2;
    char *name;
    uint dup;


    *header += sizeof(struct i_object);

    /* check for duplicates in hash table here, and if dup, only count once.
     */
#if DEBUGHASH
    DXDebug("H", "looking for obj %08x, current offset is %5d", o, *header);
#endif
    if(!FindOffset(ht, o, &dup)) {
	DXSetError(ERROR_INTERNAL, "FindOffset: error, object %08x, offset %5d",
		 o, *header);
	return ERROR;
    }
    
    if (dup != HASH_EMPTY) {
#if DEBUGHASH
	if (dup == HASH_INPROGRESS)
	    DXDebug("H", "found circular reference");
	else
	    DXDebug("H", "found duplicate at %5d", dup);
#endif
	if (dup == HASH_INPROGRESS) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "a container object has itself as a child object causing a recursive loop while trying to do a traversal");
	    return ERROR;
	}

	/* a reference to an object we've already computed the size of.
	 */
	return OK;
    }

    /* make sure it's ok before trying to traverse it */
    switch (DXGetObjectClass(o)) {
      case CLASS_MIN:
      case CLASS_MAX:
      case CLASS_DELETED:
	DXSetError(ERROR_DATA_INVALID, "bad object class");
	return ERROR;
      default: /* Lots of other classes */
	break;
    }


    /* ok, object seems ok to traverse.
     * before starting, mark the object as being currently being
     * traversed.  if we run into a pointer to it again before
     * completing it, we have a recursive reference.
     */
    if (!AddOffset(ht, o, HASH_INPROGRESS))
	return ERROR;


    
    if (doattr) {
	/* if object has attributes, add them first */
	for(i=0; (subo = DXGetEnumeratedAttribute(o, i, &name)); i++) {
	    *header += sizeof(struct i_attributes) + GetStrLenX(name);
	    if (size_estimate(subo, header, body, 0, cutoff, ht) == ERROR)
		return ERROR;
	}
    }

    switch (DXGetObjectClass(o)) {
      case CLASS_STRING:
	*header += sizeof(struct i_string) + 
	    GetStrLenX((char *)DXGetString((String)o));
	break;

      case CLASS_ARRAY:
	*header += sizeof(struct i_array);
	switch(DXGetArrayClass((Array)o)) {
	  case CLASS_ARRAY:
	    *header += sizeof(struct i_arrayarray);
	    GetArraySize(o, len);
	    if (cutoff > 0 && len > cutoff)
		*body += PARTIAL_BLOCKS(len, oneblock);
	    else
		*header += len;
	    break;

	  case CLASS_SHAREDARRAY:
	    *header += sizeof(struct i_sharedarray);
	    break;
	    
	  case CLASS_PATHARRAY:
	    break;

	  case CLASS_CONSTANTARRAY:
	    DXGetArrayInfo((Array)o, NULL, &type, NULL, NULL, NULL);
#if DEBUGOBJECT
	    DXDebug("O", "constant array item size = %d", DXGetItemSize((Array)o));
#endif

	    *header += DXGetItemSize((Array)o);
	    break;

	  case CLASS_REGULARARRAY:
	    DXGetArrayInfo((Array)o, NULL, &type, NULL, NULL, NULL);
#if DEBUGOBJECT
	    DXDebug("O", "regular array item size = %d", DXGetItemSize((Array)o));
#endif
	    *header += 2 * DXGetItemSize((Array)o);
	    break;

	  case CLASS_PRODUCTARRAY:
	    *header += sizeof(struct i_productarray);
	    DXGetProductArrayInfo((ProductArray)o, &count, terms);
	    for(i=0; i<count; i++)
		if (size_estimate((Object)terms[i], header, body, 1, cutoff, ht) == ERROR)
		    return ERROR;

	    break;

	  case CLASS_MESHARRAY:
	    *header += sizeof(struct i_mesharray);
	    DXGetMeshArrayInfo((MeshArray)o, &count, terms);
	    for(i=0; i<count; i++)
		if (size_estimate((Object)terms[i], header, body, 1, cutoff, ht) == ERROR)
		    return ERROR;
	    *header += sizeof(int) * count;

	    break;

	  default:
	    DXSetError(ERROR_DATA_INVALID, "unrecognized array subclass");
	    return ERROR;
	}
	break;

      case CLASS_FIELD:
	*header += sizeof(struct i_field);
	for(i=0; (subo=DXGetEnumeratedComponentValue((Field)o, i, &name)); i++) {
	    *header += sizeof(struct i_field) + GetStrLenX(name);
	    if (size_estimate(subo, header, body, 1, cutoff, ht) == ERROR)
		return ERROR;
	}

	break;
		
      case CLASS_LIGHT:
	*header += sizeof(struct i_light);
	break;

      case CLASS_CLIPPED:
	DXGetClippedInfo((Clipped)o, &subo1, &subo2);
	if (size_estimate(subo1, header, body, 1, cutoff, ht) == ERROR)
	    return ERROR;
	if (size_estimate(subo2, header, body, 1, cutoff, ht) == ERROR)
	    return ERROR;
	break;

      case CLASS_SCREEN:
	*header += sizeof(struct i_screen);
	DXGetScreenInfo((Screen)o, &subo, NULL, NULL);
	if (size_estimate(subo, header, body, 1, cutoff, ht) == ERROR)
	    return ERROR;
	break;

      case CLASS_XFORM:
	*header += sizeof(struct i_transform);
	DXGetXformInfo((Xform)o, &subo1, NULL);
	if (size_estimate(subo1, header, body, 1, cutoff, ht) == ERROR)
	    return ERROR;
	break;

      case CLASS_CAMERA:
	*header += sizeof(struct i_camera);
	break;

      case CLASS_GROUP:
	*header += sizeof(struct i_group);
	switch(DXGetGroupClass((Group)o)) {
	  case CLASS_GROUP:
	    for(i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
		*header += sizeof(struct i_groupgroup) + GetStrLenX(name);
		if (size_estimate(subo, header, body, 1, cutoff, ht) == ERROR)
		    return ERROR;
	    }
	    break;
		
	  case CLASS_MULTIGRID:
	  case CLASS_COMPOSITEFIELD:
	    for(i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
		*header += sizeof(struct i_compositegroup) + GetStrLenX(name);
		if (size_estimate(subo, header, body, 1, cutoff, ht) == ERROR)
		    return ERROR;
	    }
	    break;
		
	  case CLASS_SERIES:
	    for(i=0; (subo=DXGetSeriesMember((Series)o, i, &pos)); i++) {
		*header += sizeof(struct i_seriesgroup);
		if (size_estimate(subo, header, body, 1, cutoff, ht) == ERROR)
		    return ERROR;
	    }
	    break;
		
	  default:
	    DXSetError(ERROR_DATA_INVALID, "unrecognized group subclass");
	    return ERROR;
	}
	break;

      default:
	DXSetError(ERROR_DATA_INVALID, "unrecognized object class");
	return ERROR;
    }

#if DEBUGHASH
    DXDebug("H", "adding offset %5d for object %08x", *header, o);
#endif
    /* turn it from in-progress to completed */
    if (!ReplaceOffset(ht, o, *header))
	return ERROR;
    

    return OK;
}



#define SetInt(value)	{ (*(int *)*header) = value; \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define SetFloat(value)	{ (*(float *)*header) = value; \
			     INC_VOID(*header, sizeof(float)); \
			     INC_BYTE(*byteoffset, sizeof(float)); }
#define SetClass(value)	{ (*(int *)*header) = ConvertClassOut(value); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define SetType(value)	{ (*(int *)*header) = ConvertTypeOut(value); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define SetCatgry(value) { (*(int *)*header) = ConvertCatOut(value); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define SetString(value) { strcpy((char *)*header, value); \
			     INC_VOID(*header, strlen(value)+1); \
			     INC_BYTE(*byteoffset, strlen(value)+1); }
#define SetBytes(value, len) { memcpy((char *)*header, (char *)value, len); \
			     INC_VOID(*header, len); \
			     INC_BYTE(*byteoffset, len); }

 
extern Error _dxfGetSharedArrayInfo(SharedArray a, int *id, long *offset);

/* fill buffers for header.  body blocks are written out directly.
 *  if attrflag set, traverse attributes.  if 0, skip them.
 */
static
Error writeout_object(Object o, char *dataset, HashTable ht, Pointer *header, 
		      uint *byteoffset, int *bodyblk, int attrflag)
{
    int count, i, len, *listcount;
    Type type;
    Category category;
    int rank, shape[MAXRANK];
    float origin[MAXRANK], delta[MAXRANK];
    Pointer *dp;
    Vector direction;
    RGBColor color;
    Point from, to;
    Matrix m;
    int fixed, z;
    int w;
    float pos, width, aspect;
    Array terms[MAXRANK];
    Object subo, subo1, subo2;
    char *name, *cp;
    uint dupoffset;
    uint offset;


    /* check for errors
     */
    if (o == NULL || (long)o == -1L || ht == NULL || (long)ht == -1L)
	badaddr();

    /* check for duplicates in hash table here, and mark it as a dup, and
     *  add the byteoffset of the first occurance of the object.
     */
#if DEBUGHASH
    DXDebug("H", "looking for obj %08x, current offset is %5d", o, *byteoffset);
#endif
    if(!FindOffset(ht, o, &dupoffset)) {
	DXSetError(ERROR_INTERNAL, "FindOffset: error, object %08x, offset %5d",
		 o, byteoffset);
	return ERROR;
    }
    
    if(dupoffset != 0) {
#if DEBUGHASH
	DXDebug("H", "found duplicate at %5d", dupoffset);
#endif
	SetInt(F_DUPLICATE);
	SetInt(dupoffset);
	return OK;
    }

    /* unique object, add to output */
    offset = *byteoffset;

    /* add info common to any object */
    SetInt(F_HEADER);
    SetClass(DXGetObjectClass(o));

    switch(DXGetObjectClass(o)) {
      case CLASS_STRING:
	cp = DXGetString((String)o);
	SetInt(GetStrLen(cp));
	if(GetStrLen(cp) > 0)
	    SetString(cp);
	break;
	
      case CLASS_ARRAY:
	DXGetArrayInfo((Array)o, &count, &type, &category, &rank, shape);
	SetType(type);
	SetCatgry(category);
	SetInt(rank);
	if (version == DISKVERSION_A || version == DISKVERSION_B) {
	    for(i=0; i<MAXRANK; i++)
		SetInt(shape[i]);
	} else {
	    for(i=0; i<rank; i++)
		SetInt(shape[i]);
	}
	SetInt(count);
	SetClass(DXGetArrayClass((Array)o));
	switch(DXGetArrayClass((Array)o)) {
	  case CLASS_ARRAY:
	    len = count * DXGetItemSize((Array)o);	
	    if (cutoff > 0 && len > cutoff) {
		SetInt(*bodyblk);
		count = PARTIAL_BLOCKS(len, oneblock);
		if (!_dxffile_write(dataset, *bodyblk, count, 
			DXGetArrayData((Array)o), LEFTOVER_BYTES(len, oneblock)))
		    return ERROR;

		*bodyblk += count;
	    } else {
		SetInt(len);
		SetBytes(DXGetArrayData((Array)o), len);
	    }

	    break;
	    
	  case CLASS_SHAREDARRAY:
	  {
	    struct i_sharedarray isa;
	    _dxfGetSharedArrayInfo((SharedArray)o, &isa.id, &isa.offset);
	    SetBytes((Pointer)&isa, sizeof(isa));
	    break;
	  }

	  case CLASS_PATHARRAY:
	    break;
	    
	  case CLASS_REGULARARRAY:
	    DXGetRegularArrayInfo((RegularArray)o, NULL, 
				(Pointer)origin, (Pointer)delta);
#if DEBUGOBJECT
	    DXDebug("O", "regular array item size = %d", DXGetItemSize((Array)o));
#endif
	    SetBytes(origin, DXGetItemSize((Array)o));
	    SetBytes(delta, DXGetItemSize((Array)o));
	    break;

	  case CLASS_CONSTANTARRAY:
	    dp = DXGetConstantArrayData((Array)o);
#if DEBUGOBJECT
	    DXDebug("O", "contants array item size = %d", DXGetItemSize((Array)o));
#endif
	    SetBytes(dp, DXGetItemSize((Array)o));
	    break;

	  case CLASS_PRODUCTARRAY:
	    DXGetProductArrayInfo((ProductArray)o, &count, terms);
	    SetInt(count);
	    for(i=0; i<count; i++)
		if (!writeout_object((Object)terms[i], dataset, ht, header, 
				byteoffset, bodyblk, 1))
		    return ERROR;

	    break;

	  case CLASS_MESHARRAY:
	    DXGetMeshArrayInfo((MeshArray)o, &count, terms);
	    SetInt(count);
	    for(i=0; i<count; i++)
		if (!writeout_object((Object)terms[i], dataset, ht, header, 
				byteoffset, bodyblk, 1))
		    return ERROR;

	    DXGetMeshOffsets((MeshArray)o, shape);
	    for(i=0; i<count; i++)
		SetInt(shape[i]);

	    break;

	  default:
	    DXSetError(ERROR_INTERNAL, "unrecognized array subclass");
	    break;
	}
	break;

      case CLASS_FIELD:
	listcount = (int *)*header;
	SetInt(0);
	for(i=0; (subo=DXGetEnumeratedComponentValue((Field)o, i, &name)); i++) {
	    (*listcount)++;
	    SetInt(GetStrLen(name));
	    if(GetStrLen(name) > 0)
		SetString(name);
	    if (!writeout_object(subo, dataset, ht, header, byteoffset, 
				 bodyblk, 1))
		return ERROR;
	}
	break;
		
      case CLASS_LIGHT:
	if (DXQueryAmbientLight((Light)o, &color)) {
	    SetInt(LITE_AMBIENT);
	    SetBytes(&color, sizeof(RGBColor));
	} else if (DXQueryDistantLight((Light)o, &direction, &color)) {
	    SetInt(LITE_DISTANT);
	    SetBytes(&direction, sizeof(Vector));
	    SetBytes(&color, sizeof(RGBColor));
	} else if (DXQueryCameraDistantLight((Light)o, &direction, &color)) {
	    SetInt(LITE_CAM_DISTANT);
	    SetBytes(&direction, sizeof(Vector));
	    SetBytes(&color, sizeof(RGBColor));
	} else 
	    DXWarning("unsupported light type ignored.");
	break;

      case CLASS_CLIPPED:
	DXGetClippedInfo((Clipped)o, &subo1, &subo2);
	if (!writeout_object(subo1, dataset, ht, header, byteoffset, 
			     bodyblk, 1))
	    return ERROR;

	if (!writeout_object(subo2, dataset, ht, header, byteoffset, 
			     bodyblk, 1))
	    return ERROR;

	break;

      case CLASS_SCREEN:
	DXGetScreenInfo((Screen)o, &subo, &fixed, &z);
	SetInt(fixed);
	SetInt(z);
	if (!writeout_object(subo, dataset, ht, header, byteoffset, 
			     bodyblk, 1))
	    return ERROR;

	break;

      case CLASS_XFORM:
	DXGetXformInfo((Xform)o, &subo, &m);
	SetBytes(&m, sizeof(m));
	if (!writeout_object(subo, dataset, ht, header, byteoffset, 
			     bodyblk, 1))
	    return ERROR;

	break;

      case CLASS_CAMERA:
	if (DXGetOrthographic((Camera)o, &width, &aspect)) {
	    SetInt(CAM_ORTHO);
	} else if (DXGetPerspective((Camera)o, &width, &aspect)) {
	    SetInt(CAM_PERSPECT);
	} else {
	    DXWarning("ignoring unsupported camera type");
	    break;
	}
	
	SetFloat(width);
	SetFloat(aspect);
	DXGetCameraResolution((Camera)o, &w, NULL);
	SetInt(w);
	DXGetView((Camera)o, &from, &to, &direction);
	SetBytes(&from, sizeof(Point));
	SetBytes(&to, sizeof(Point));
	SetBytes(&direction, sizeof(Vector));
	DXGetBackgroundColor((Camera)o, &color);
	SetBytes(&color, sizeof(RGBColor));
	break;

      case CLASS_GROUP:
	SetClass(DXGetGroupClass((Group)o));
	switch(DXGetGroupClass((Group)o)) {
	  case CLASS_GROUP:
	  case CLASS_COMPOSITEFIELD:
	  case CLASS_MULTIGRID:
	    listcount = (int *)*header;
	    SetInt(0);
	    for(i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
		(*listcount)++;
		SetInt(GetStrLen(name));
		if(GetStrLen(name) > 0)
		    SetString(name);
		if (!writeout_object(subo, dataset, ht, header, byteoffset, 
				bodyblk, 1))
		    return ERROR;
	    }
	    break;
		
	  case CLASS_SERIES:
	    listcount = (int *)*header;
	    SetInt(0);
	    for(i=0; (subo=DXGetSeriesMember((Series)o, i, &pos)); i++) {
		(*listcount)++;
		SetFloat(pos);
		if (!writeout_object(subo, dataset, ht, header, byteoffset, 
				bodyblk, 1))
		    return ERROR;
	    }
	    break;
		
	  default:
	    DXSetError(ERROR_INTERNAL, "unrecognized group subclass");
	    break;
	}
	break;

      default:
	DXSetError(ERROR_INTERNAL, "unrecognized object class");
	return ERROR;
    }

    /* now do attributes, if necessary */
    listcount = (int *)*header;
    SetInt(0);

    if(attrflag) {
	/* attrflag makes sure we don't add attributes of attributes.
	 */
	for(i=0; (subo=DXGetEnumeratedAttribute(o, i, &name)); i++) {
	    (*listcount)++;
	    SetInt(GetStrLen(name));
	    if(GetStrLen(name) > 0)
		SetString(name);
	    if (!writeout_object(subo, dataset, ht, header, byteoffset, 
				 bodyblk, 0))
		return ERROR;
	}
    }

#if DEBUGHASH
    DXDebug("H", "adding offset %5d for object %08x", offset, o);
#endif
    if(!AddOffset(ht, o, offset))
       return ERROR;

    return OK;
}



/* 
 * macros to convert the next bytes in the buffer to the right values, 
 *  and swap the bytes if necessary.
 */
#define SWABS(x)  _dxfByteSwap((void *)&x, (void*)&x, 1, TYPE_SHORT) 
#define SWABI(x)  _dxfByteSwap((void *)&x, (void*)&x, 1, TYPE_INT) 
#define SWABF(x)  _dxfByteSwap((void *)&x, (void*)&x, 1, TYPE_FLOAT) 
#define SWABD(x)  _dxfByteSwap((void *)&x, (void*)&x, 1, TYPE_DOUBLE) 

#define SWABSN(x,n)  _dxfByteSwap((void *)x, (void*)x, n/2, TYPE_SHORT) 
#define SWABIN(x,n)  _dxfByteSwap((void *)x, (void*)x, n/4, TYPE_INT) 
#define SWABFN(x,n)  _dxfByteSwap((void *)x, (void*)x, n/4, TYPE_FLOAT)
#define SWABDN(x,n)  _dxfByteSwap((void *)x, (void*)x, n/8, TYPE_DOUBLE) 
 
#define GetInt(value)	{ value = (*(int *)*header); \
			     if (needswab) SWABI(value); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define GetFloat(value)	{ value = (*(float *)*header); \
			     if (needswab) SWABF(value); \
			     INC_VOID(*header, sizeof(float)); \
			     INC_BYTE(*byteoffset, sizeof(float)); }
#define GetClass(value, version)  \
                        { value = (Class) (*(int *)*header); \
			     if (needswab) SWABI(value); \
			     value = ConvertClassIn((Class)value, version); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define GetType(value, version)   \
                        { value = (Type) (*(int *)*header); \
			     if (needswab) SWABI(value); \
			     value = ConvertTypeIn((Type)value, version); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define GetCatgry(value, version) \
                        { value = (Category) (*(int *)*header); \
			     if (needswab) SWABI(value); \
			     value = ConvertCatIn((Category)value, version); \
			     INC_VOID(*header, sizeof(int)); \
			     INC_BYTE(*byteoffset, sizeof(int)); }
#define GetStringX(value) { value = (char *)*header; \
			     INC_VOID(*header, strlen(value)+1); \
			     INC_BYTE(*byteoffset, strlen(value)+1); }
#define GetBytes(value, len) { value = (Pointer)*header; \
			     INC_VOID(*header, len); \
			     INC_BYTE(*byteoffset, len); }
#define GetSBytes(value, len) { value = (Pointer)*header; \
			     if (needswab) SWABSN(value, len); \
			     INC_VOID(*header, len); \
			     INC_BYTE(*byteoffset, len); }
#define GetIBytes(value, len) { value = (Pointer)*header; \
			     if (needswab) SWABIN(value, len); \
			     INC_VOID(*header, len); \
			     INC_BYTE(*byteoffset, len); }
#define GetFBytes(value, len) { value = (Pointer)*header; \
			     if (needswab) SWABFN(value, len); \
			     INC_VOID(*header, len); \
			     INC_BYTE(*byteoffset, len); }
#define GetDBytes(value, len) { value = (Pointer)*header; \
			     if (needswab) SWABDN(value, len); \
			     INC_VOID(*header, len); \
			     INC_BYTE(*byteoffset, len); }

 
/* fill buffers from header.  body blocks are read in directly.
 *  if attrflag set, traverse attributes.  if 0, skip them.
 */
SharedArray _dxfNewSharedArrayFromOffsetV(int id, long offset, Type t, Category c, int r, int *s);

static
Object readin_object(char *dataset, Pointer *header, HashTable ht,
		      uint *byteoffset, int *bodyblk, int attrflag)
{
    Object o = NULL;
    int count, i, len, listcount;
    Type type;
    Category category;
    int rank, shape[MAXRANK];
    Pointer origin = NULL, delta = NULL;
    Pointer dp = NULL;
    Matrix *m;
    int fixed, z;
    int imagewidth;
    Point *from, *to;
    Vector *up, *direction;
    float width, aspect;
    RGBColor *color;
    float pos;
    Array terms[MAXRANK];
    Object subo, subo1, subo2;
    char *name;
    uint offset;
    Class class, subclass;


    /* look for errors */
    if (header == NULL || (ulong)header == -1L || ht == NULL || (ulong)ht == -1L)
	badaddr();

    /* check for duplicates in hash table here, and if duplicate, return
     *  existing object address.  if not there, save the object offset
     *  so we can put object in hash table at the end of the call.
     */

    offset = *byteoffset;   /* save for later */

    /* info common to any object */
    GetInt(i);

    if(i == F_DUPLICATE) {
	GetInt(offset);
	if(!FindObject(ht, offset, &o)) {
	    DXSetError(ERROR_INTERNAL, "didn't find duplicate at offset %5d",
			offset);
	    return ERROR;
	}
#if DEBUGHASH
	DXDebug("H", "duplicate at offset %5d, object %08x", offset, o);
#endif
	if (o == NULL || (ulong)o == -1L)
	    badaddr();

	return o;
    }

    GetClass(class, version);

#if DEBUGOBJECT
    DXDebug("F", "object type %x, byteoffset %d", (int) class, offset); 
#endif

    switch(class) {
      case CLASS_STRING:
	GetInt(len);
	if (len == 0) {
	    o = (Object)DXNewString("");
	    break;
	}
	GetStringX(name);
	o = (Object)DXNewString(name);
	break;
	
      case CLASS_ARRAY:
	GetType(type, version);
	GetCatgry(category, version);
	GetInt(rank);
	if (version == DISKVERSION_A || version == DISKVERSION_B) {
	    for(i=0; i<MAXRANK; i++)
		GetInt(shape[i]);
	} else {
	    for(i=0; i<rank; i++)
		GetInt(shape[i]);
	}
	GetInt(count);
	GetClass(subclass, version);
	switch(subclass) {
	  case CLASS_ARRAY:
	    o = (Object)DXNewArrayV(type, category, rank, shape);
	    if (!o)
		return NULL;
	    len = count * DXGetItemSize((Array)o);
	    if (cutoff > 0 && len > cutoff) {
		if (!DXAddArrayData((Array)o, 0, count, NULL)) {
		    DXDelete(o);
		    return NULL;
		}
		GetInt(*bodyblk);
		count = PARTIAL_BLOCKS(len, oneblock);
#if MARKTIME
		if (len > oneblock)
		    DXMarkTime("> multi b rd");
		else
		    DXMarkTime("> single b rd");
#endif
		if (!_dxffile_read(dataset, *bodyblk, count, 
				   DXGetArrayData((Array)o), 
				   LEFTOVER_BYTES(len, oneblock))) {
		    DXDelete(o);
		    return NULL;
		}
#if MARKTIME
		if (len > oneblock)
		    DXMarkTime("< multi b rd");
		else
		    DXMarkTime("< single b rd");
#endif
		
		*bodyblk += count;
	    } else {
		GetInt(len);
		switch(type) {
		  case TYPE_BYTE: case TYPE_UBYTE: case TYPE_STRING:
		    GetBytes(dp, len);
		    break;
		  case TYPE_SHORT: case TYPE_USHORT:	
		    GetSBytes(dp, len);
		    break;
		  case TYPE_INT: case TYPE_UINT:	
		    GetIBytes(dp, len);
		    break;
		  case TYPE_FLOAT:
		    GetFBytes(dp, len);
		    break;
		  case TYPE_DOUBLE:
		    GetDBytes(dp, len);
		    break;
		  default: /* Type Hyper not handled (bug?) */
		    break;
		}
		if (!DXAddArrayData((Array)o, 0, count, dp)) {
		    DXDelete(o);
		    return NULL;
		}
	    }
	    
#if !defined(DXD_STANDARD_IEEE)
	    if (type == TYPE_FLOAT) {
		int bad_fp = 0;
		int denorm_fp = 0;
		_dxffloat_denorm((int *)DXGetArrayData((Array)o), len/sizeof(float),
				 &denorm_fp, &bad_fp);
	        if (bad_fp > 0 || denorm_fp > 0)
		    DXWarning("%d bad floats, %d small floats truncated to zero",
			      bad_fp, denorm_fp);
	    } else if (type == TYPE_DOUBLE) {
		int bad_fp = 0;
		int denorm_fp = 0;
		_dxfdouble_denorm((int *)DXGetArrayData((Array)o), len/sizeof(double),
				&denorm_fp, &bad_fp);
	        if (bad_fp > 0 || denorm_fp > 0)
		    DXWarning("%d bad doubles, %d small doubles truncated to zero",
			      bad_fp, denorm_fp);
	    }
#endif
	    
	    break;
	    
#if 0
	  case CLASS_SHAREDARRAY:
	  {
	      struct i_sharedarray *isa;
	      GetBytes(isa, sizeof(struct i_sharedarray));
	      o = (Object)_dxfNewSharedArrayFromOffsetV(isa->id, isa->offset, type, category, rank, shape);
	      break;
	  }
#endif

	  case CLASS_PATHARRAY:
	    o = (Object)DXNewPathArray(count+1);
	    break;
	    
	  case CLASS_REGULARARRAY:
	    switch(type) {
	      case TYPE_BYTE: case TYPE_UBYTE: case TYPE_STRING:
		GetBytes(origin, arrayitemsize(type, category, rank, shape));
		GetBytes(delta, arrayitemsize(type, category, rank, shape));;
		break;
	      case TYPE_SHORT: case TYPE_USHORT:	
		GetSBytes(origin, arrayitemsize(type, category, rank, shape));
		GetSBytes(delta, arrayitemsize(type, category, rank, shape));;
		break;
	      case TYPE_INT: case TYPE_UINT:	
		GetIBytes(origin, arrayitemsize(type, category, rank, shape));
		GetIBytes(delta, arrayitemsize(type, category, rank, shape));;
		break;
	      case TYPE_FLOAT:
		GetFBytes(origin, arrayitemsize(type, category, rank, shape));
		GetFBytes(delta, arrayitemsize(type, category, rank, shape));;
		break;
	      case TYPE_DOUBLE:
		GetDBytes(origin, arrayitemsize(type, category, rank, shape));
		GetDBytes(delta, arrayitemsize(type, category, rank, shape));;
		break;
	      default: /* No TYPE_HYPER (bug?) */
		break;
	    }
	    o = (Object)DXNewRegularArray(type, shape[0], count, origin, delta);
	    break;
	    
	  case CLASS_CONSTANTARRAY:
	    switch (type) {
	      case TYPE_BYTE: case TYPE_UBYTE: case TYPE_STRING:
		GetBytes(origin, arrayitemsize(type, category, rank, shape));
		break;
	      case TYPE_SHORT: case TYPE_USHORT:	
		GetSBytes(origin, arrayitemsize(type, category, rank, shape));
		break;
	      case TYPE_INT: case TYPE_UINT:	
		GetIBytes(origin, arrayitemsize(type, category, rank, shape));
		break;
	      case TYPE_FLOAT:
		GetFBytes(origin, arrayitemsize(type, category, rank, shape));
		break;
	      case TYPE_DOUBLE:
		GetDBytes(origin, arrayitemsize(type, category, rank, shape));
		break;
	      default: /* TYPE_HYPER not handled (bug?) */
		break;
	    }
	    o = (Object)DXNewConstantArrayV(count, origin, type,
					    category, rank, shape);
	    break;
	    
	  case CLASS_PRODUCTARRAY:
	    GetInt(count);
	    for (i=0; i<count; i++) {
		terms[i] = (Array)readin_object(dataset, header, ht, 
						byteoffset, bodyblk, 1);
		if (!terms[i]) {
		    while(--i >= 0)
			DXDelete((Object)terms[i]);
		    
		    return NULL;
		}
	    }
	    
	    o = (Object)DXNewProductArrayV(count, terms);
	    break;
	    
	  case CLASS_MESHARRAY:
	    GetInt(count);
	    for (i=0; i<count; i++) {
		terms[i] = (Array)readin_object(dataset, header, ht, 
						byteoffset, bodyblk, 1);
		if (!terms[i]) {
		    while(--i >= 0)
			DXDelete((Object)terms[i]);
		    
		    return NULL;
		}
	    }

	    o = (Object)DXNewMeshArrayV(count, terms);
	    if (!o)
		break;
	    
	    if (version != DISKVERSION_A) {
		for (i=0; i<count; i++)
		    GetInt(shape[i]);
		
		if (!DXSetMeshOffsets((MeshArray)o, shape)) {
		    DXDelete(o);
		    return NULL;
		}
	    }

	    break;

	  default:
	    DXSetError(ERROR_INTERNAL, "unrecognized array subclass");
	    break;
	}
	break;

      case CLASS_FIELD:
	o = (Object)DXNewField();
	if (!o)
	    return NULL;
	
	GetInt(listcount);
	for (i=0; i<listcount; i++) {
	    GetInt(len);
	    if (len == 0)
		break;
	    GetStringX(name);
	    subo = readin_object(dataset, header, ht, byteoffset, bodyblk, 1);
	    if (!subo) {
	        DXDelete(o);
	        return NULL;
	    }
	    if (!DXSetComponentValue((Field)o, name, subo)) {
		DXDelete(o);
		return NULL;
	    }
	}
#if 0
	if (!DXEndField((Field)o)) {
	    DXDelete(o);
	    return NULL;
	}
#endif
	break;
	
      case CLASS_LIGHT:
	GetInt(z);
	switch(z) {
	  case LITE_AMBIENT:
	    GetFBytes(color, sizeof(RGBColor));
	    o = (Object)DXNewAmbientLight(*color);
	    break;
	  case LITE_DISTANT:
	    GetFBytes(direction, sizeof(Vector));
	    GetFBytes(color, sizeof(RGBColor));
	    o = (Object)DXNewDistantLight(*direction, *color);
	    break;
	  case LITE_CAM_DISTANT:
	    GetFBytes(direction, sizeof(Vector));
	    GetFBytes(color, sizeof(RGBColor));
	    o = (Object)DXNewCameraDistantLight(*direction, *color);
	    break;
	  default:
	    DXWarning("unsupported light type ignored.");
	    break;
	}
	break;
	
      case CLASS_CLIPPED:
	subo1 = readin_object(dataset, header, ht, byteoffset, bodyblk, 1);
	subo2 = readin_object(dataset, header, ht, byteoffset, bodyblk, 1);
	if (!subo1 || !subo2) {
	    DXDelete(subo1);
	    DXDelete(subo2);
	    return NULL;
	}
	o = (Object)DXNewClipped(subo1, subo2);
	break;

      case CLASS_SCREEN:
	GetInt(fixed);
	GetInt(z);
	subo = readin_object(dataset, header, ht, byteoffset, bodyblk, 1);
	if (!subo)
	    return NULL;

	o = (Object)DXNewScreen(subo, fixed, z);
	break;

      case CLASS_XFORM:
	GetFBytes(m, sizeof(Matrix));
	subo = readin_object(dataset, header, ht, byteoffset, bodyblk, 1);
	if (!subo)
	    return NULL;
	
	o = (Object)DXNewXform(subo, *m);
	break;
	
      case CLASS_CAMERA:
	GetInt(z);
	switch(z) {
	  case CAM_ORTHO:
	  case CAM_PERSPECT:
	    break;
	  default:
	    DXWarning("unsupported camera type ignored.");
	    break;
	}
	
	GetFloat(width);  /* fov for perspect */
	GetFloat(aspect);
	GetInt(imagewidth);
	GetFBytes(from, sizeof(Point));
	GetFBytes(to, sizeof(Point));  
	GetFBytes(up, sizeof(Vector));
	GetFBytes(color, sizeof(RGBColor));
	
	o = (Object)DXNewCamera();
	if(z == CAM_ORTHO) 
	    DXSetOrthographic((Camera)o, width, aspect);
	else
	    DXSetPerspective((Camera)o, width, aspect);
	
	DXSetResolution((Camera)o, imagewidth, 1);
	DXSetView((Camera)o, *from, *to, *up);
	DXSetBackgroundColor((Camera)o, *color);
	break;


      case CLASS_GROUP:
	GetClass(subclass, version);
	switch(subclass) {
	  case CLASS_GROUP:
	    o = (Object)DXNewGroup();
	    if (!o)
	  	return NULL;

	    goto buildgroup;

	  case CLASS_MULTIGRID:
	    o = (Object)DXNewMultiGrid();
	    if (!o)
	  	return NULL;

	    goto buildgroup;

	  case CLASS_COMPOSITEFIELD:
	    o = (Object)DXNewCompositeField();
	    if (!o)
	  	return NULL;

	  buildgroup:
	    GetInt(listcount);
	    for(i=0; i<listcount; i++) {
		GetInt(len);
		if(len > 0) {
		    GetStringX(name);
		} else
		    name = NULL;
		subo = readin_object(dataset, header, ht, byteoffset, 
				     bodyblk, 1);
		if (!subo) {
		    DXDelete(o);
		    return NULL;
		}
		if (!DXSetMember((Group)o, name, subo)) {
		    DXDelete(o);
		    return NULL;
		}
	    }
	    break;
		
	  case CLASS_SERIES:
	    o = (Object)DXNewSeries();
	    if (!o)
	  	return NULL;
	    
	    GetInt(listcount);
	    for(i=0; i<listcount; i++) {
		GetFloat(pos);
		subo = readin_object(dataset, header, ht, byteoffset, 
				     bodyblk, 1);
		if (!subo) {
		    DXDelete(o);
		    return NULL;
		}
		if (!DXSetSeriesMember((Series)o, i, pos, subo)) {
		    DXDelete(o);
		    return NULL;
		}
	    }
	    break;
		
	  default:
	    DXSetError(ERROR_INTERNAL, "unrecognized group subclass");
	    break;
	}
	break;
	
      default:
	DXSetError(ERROR_INTERNAL, "unrecognized object class");
	break;
    }
    
    /* bad object */
    if (o == NULL || (ulong)o == -1L)
	badaddr();

    /* error creating object? */
    if (!o)
	return NULL;

    /* now handle attributes, if present */
    GetInt(listcount);

    if(attrflag) {
	/* if object has attributes, get them here.
	 *  attrflag makes sure we don't add attributes of attributes.
	 */
	for(i=0; i<listcount; i++) {
	    GetInt(len);
	    if(len == 0)
		break;
	    GetStringX(name);
	    subo = readin_object(dataset, header, ht, byteoffset, bodyblk, 0);
	    if (!subo) {
		DXDelete(o);
		return NULL;
	    }
	    if (!DXSetAttribute(o, name, subo)) {
		DXDelete(o);
		return NULL;
	    }
	}
    }

#if DEBUGHASH
    DXDebug("H", "adding object %08x at byteoffset %5d", o, offset);
#endif
    if (o && !AddObject(ht, offset, o))
	return NULL;

    return o;
}




/* external entry points
 */
Object _dxfImportBin(char *dataset)
{
    Object o = NULL;
    int nh = 0;			/* number of header blocks */
    HashTable hashTable;
    Pointer header_base, header;
    struct f_label *flp;
    uint byteoffset = 0;
    int bodyblk = 0;

    /* assume one block.  after reading in, if more, free and read
     *  in again.
     */

#if MARKTIME
    DXMarkTime("> ImportBin");
#endif

    oneblock = ONEBLK;

    if(_dxffile_open(dataset, 0) == ERROR)
	return NULL;

    /* allocate 1 block for header */
    header_base = DXAllocate(oneblock + ONEK);
    if(!header_base)
	return NULL;

    header = (Pointer)ROUNDUP_BYTES(header_base, ONEK);
    byteoffset = 0;

    if(_dxffile_read(dataset, 0, 1, header, 0) == ERROR) {
	DXFree(header_base);
	return NULL;
    }

    /* check label to see if it matches */
    flp = (struct f_label *)header;

    version = flp->version;   /* temp global */
    cutoff = flp->cutoff;     /* temp global */


    /* version number */
    /* byte order is marked in version E and beyond */

    switch(flp->version) {
      case DISKVERSION_A:
	/* handle old version read only */
	DXMessage("old format file, please resave");
	cutoff = 4 * ONEK;
	break;

      case DISKVERSION_B:
	cutoff = 4 * ONEK;
	break;

      case DISKVERSION_C:
      case DISKVERSION_D:
	break;

      case DISKVERSION_E:
	if (flp->byteorder != MSB_MACHINE) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "byte order mismatch, can't read binary data file");
	    DXFree(header_base);
	    return NULL;
	}
	break;

      default:
	DXSetError(ERROR_DATA_INVALID, 
		 "unsupported version number, can't read file");
	DXFree(header_base);
	return NULL;
    }

    /* blocking size, in bytes */
    if(flp->blocksize != oneblock) {
	DXSetError(ERROR_DATA_INVALID, 
		 "blocksize doesn't match, can't read file");
	DXFree(header_base);
	return NULL;
    }

    /* old format was blocks */
    if (version == DISKVERSION_A)
	nh = flp->headerbytes;
    else
	nh = PARTIAL_BLOCKS(flp->headerbytes, oneblock);

    DXDebug("S", "import %s: %d header blocks, %d leftover bytes", 
	  dataset, nh, flp->leftover);

    /* number of header blocks */
    if(nh > 1) {
	DXFree(header_base);

#if MARKTIME
	DXMarkTime("> hdr alloc");
#endif

	/* allocate right number of blocks for header */
	header_base = DXAllocate(nh * oneblock + ONEK);
	if(!header_base)
	    return NULL;
	
	header = (Pointer)ROUNDUP_BYTES(header_base, ONEK);
	byteoffset = 0;
	
#if MARKTIME
	DXMarkTime("< hdr alloc");
#endif
#if MARKTIME
	DXMarkTime("> hdr rd");
#endif

	if(_dxffile_read(dataset, 0, nh, header, 0) == ERROR) {
	    DXFree(header_base);
	    return NULL;
	}

#if MARKTIME
	DXMarkTime("< hdr rd");
#endif

	/* reset header */
	flp = (struct f_label *)header;

    }

    /* use defaults: first element in struct is the key; and no extra
     *  compare function is needed.
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if(!hashTable) {
	DXFree(header_base);
	return NULL;
    }

    /* start of body */
    bodyblk = nh;
    if (version == DISKVERSION_A)
	byteoffset += sizeof(int) * 4;
    else
	byteoffset += sizeof(struct f_label);

    header = (Pointer)((char *)header + byteoffset);

#if MARKTIME
    DXMarkTime("> obj rd");
#endif

    /* this routine calls _dxffile_read for array bodies > cutoff directly */
    o = readin_object(dataset, &header, hashTable, &byteoffset, 
			                             &bodyblk, 1);

#if MARKTIME
    DXMarkTime("< obj rd");
#endif

    _dxffile_close(dataset);
    DXFree(header_base);
    DXDestroyHash(hashTable);

#if MARKTIME
    DXMarkTime("< ImportBin");
#endif

    return o;

}



Object _dxfExportBin(Object o, char *dataset)
{
    int nh = 0;			/* number of header bytes */
    int nb = 0;			/* number of body blocks */
    HashTable hashTable;
    Pointer header_base = NULL, header, hp;
    struct f_label *flp;
    uint byteoffset = 0;
    int bodyblk;

#if 0
    /* experiment with default cutoff */
    if (DXExtractInteger(DXGetAttribute(o, "cutoff"), &cutoff)) {
	if (cutoff <= 0 || cutoff > 128) {
	    DXSetError(ERROR_BAD_PARAMETER, 
		     "bad import cutoff attribute %d Kbytes", cutoff);
	    return NULL;
	}
	cutoff *= ONEK;
    } else
#endif
	cutoff = 64 * ONEK; 


    oneblock = ONEBLK;

    /* 
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if (!hashTable) {
	DXFree(header_base);
	return NULL;
    }

    if (size_estimate(o, &nh, &nb, 1, cutoff, hashTable) == ERROR) {
	DXDestroyHash(hashTable);
	return NULL;
    }

    DXDestroyHash(hashTable);


    DXDebug("S", "export %s: %d headerbyte, %d bodyblocks, cutoff %d Kb", 
	  dataset, nh, nb, cutoff);

    if (_dxffile_open(dataset, 2) == ERROR)
	return NULL;

    nh += sizeof(struct f_label);

    if (_dxffile_add(dataset, PARTIAL_BLOCKS(nh, oneblock) + nb) == ERROR)
	return NULL;
      
    header_base = DXAllocate(ROUNDUP_BYTES(nh, oneblock) + ONEK);
    if (!header_base)
	return NULL;

    header = (Pointer)ROUNDUP_BYTES(header_base, ONEK);

    /* use defaults: first element in struct is the key; and no extra
     *  compare function is needed.
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if (!hashTable) {
	DXFree(header_base);
	return NULL;
    }

    hp = header;
    byteoffset = 0;

    /* fill in label */
    flp = (struct f_label *)header;
    header = (Pointer)((byte *)header + sizeof(struct f_label));
    byteoffset += sizeof(struct f_label);


    /* version number */
    flp->version = DISKVERSION_E;

    /* cutoff between header and body */
    flp->cutoff = cutoff;

    /* blocksize */
    flp->blocksize = oneblock;

    /* number of header bytes */
    flp->headerbytes = nh;

    /* file type */
    flp->type = DISKTYPE_NORMAL;

    /* file format */
    flp->format = DISKFORMAT_BIN;

    /* number of valid bytes in the last block of ?? header or body? */
    /* header for now - although i'm not sure what i'm going to do with it */
    flp->leftover = LEFTOVER_BYTES(nh, oneblock);

    /* mark the native byteorder so we can at least recognize if we are
     * sending to an incompatible machine.  to be really useful, i need
     * to add code to swap only the correct bytes.  maybe next release?
     */
    flp->byteorder = MSB_MACHINE;

    /* write out object */
    bodyblk = PARTIAL_BLOCKS(nh, oneblock);

    /* calls _dxffile_write(array bodies > cutoff) directly */
    if (writeout_object(o, dataset, hashTable, &header, &byteoffset, 
		                                      &bodyblk, 1) == ERROR) {
	_dxffile_close(dataset);
	DXFree(header_base);
	DXDestroyHash(hashTable);
	return NULL;
    }

    DXDebug("S", "actual bytes: %d, estimated bytes: %d", byteoffset, nh);

    /* make sure the actual header isn't longer than the estimate */
    if (PARTIAL_BLOCKS(byteoffset, oneblock) > PARTIAL_BLOCKS(nh, oneblock)) {
	DXSetError(ERROR_INTERNAL, "import header estimate too small");
	return NULL;
    }

    /* hp has original header start */
    _dxffile_write(dataset, 0, PARTIAL_BLOCKS(nh, oneblock), hp, 0);
    _dxffile_close(dataset);
    DXFree(header_base);
    DXDestroyHash(hashTable);

    return o;
}


#if defined(DXD_HAS_OS2_CP) || defined(intelnt) || defined(WIN32)

#define savecontext() 0
#define restorecontext() 0

#else
/* set up signal handler for broken pipes 
 */

/* badsock0 and badsock1 were added because there seems to be a bug with */
/* setjump in solaris seems broken. Probably type jmp_buf is not big enough */
/* remove badsock0 and badsock1 when this is resolved. */

#ifdef DEBUG

static jmp_buf badsock0;
static jmp_buf badsock1;

static void handlesock(void) {
    signal(SIGPIPE, oldsignal);
    longjmp(badsock, 1);
}
#endif

static jmp_buf badsock;
void (*oldsignal)(int) = NULL;

static int savecontext()
{
    int i;

    oldsignal = signal(SIGPIPE, NULL);

    i = setjmp(badsock);
    if (i != 0) {
	signal(SIGPIPE, oldsignal);
	DXSetError(ERROR_DATA_INVALID, "connection to remote process closed");
    }

    return i;
}

static void restorecontext()
{
    if (oldsignal != NULL)
	signal(SIGPIPE, oldsignal);
}

#endif 


/* import/export in binary across an already open file descriptor
 *
 * this differs from the normal import/export routines in that:
 *  there is no difference between header and body - everything in inline
 *  there is no need to do a size estimate (?? see below)
 *  an artificial filename is constructed from the file descriptor because
 *   all the lower level routines for the array disk (where this code is
 *   normally used) are based on filenames.
 *
 * problems:
 *  without size estimate, how do i alloc enough blocks?  since the array
 *  disk wants large single reads/writes, this doesn't write out blocks
 *  as they fill.  this means i have to have enough space for the entire
 *  object in memory before i start.  can this be changed to write out
 *  blocks as they fill?
 *
 *  the array disk can only be written in whole block chunks, so there is
 *  a routine to read/write partial blocks.  is this going to be a problem?
 *  
 */

Object _dxfImportBin_FP(int fd)
{
    Object o = NULL;
    int nh = 0;			/* number of header blocks */
    HashTable hashTable = NULL;
    Pointer header_base = NULL, header;
    struct f_label *flp;
    uint byteoffset = 0;
    int trying_inverted = 0;
    int bodyblk = 0;
    char dataset[64];

#if 0 /* broken */
    oneblock = ONEBLK;
#else
    oneblock = HALFK;
#endif

    /* construct a fake name.  (the lower level read/write routines have
     *  a filename as the first parm, not a file descriptor.)
     */
    sprintf(dataset, "socket %x import", fd);


    /* read first block, get total size from header and realloc the right
     * buffer size.
     */
    if(_dxfsock_open(dataset, fd) == ERROR)
	return NULL;

    if (savecontext() != 0)
	goto error;

    /* allocate 1 block for header */
    header_base = DXAllocate(oneblock + ONEK);
    if(!header_base)
	goto error;

    header = header_base;
    byteoffset = 0;

    if(_dxffile_read(dataset, 0, 1, header, 0) == ERROR)
	goto error;
    
  again:
    /* check label to see if it matches */
    flp = (struct f_label *)header;
    
    version = flp->version;   /* these three are static globals */
    cutoff = flp->cutoff;
    needswab = 0;

    /* on a socket, don't support anything but the current version number */
    switch(flp->version) {
      case DISKVERSION_IE:
	/* might be a version we recognize but with an inverted byte order.
	 *  invert the header, set a flag and try again.
	 */
	trying_inverted++;
	_dxfByteSwap((void *)flp, (void*)flp, 
		     (int)(sizeof(struct f_label)/sizeof(int)), TYPE_INT);
	goto again;

      case DISKVERSION_E:
	if (flp->byteorder != MSB_MACHINE) {
	    /* if we've already inverted the header, then this is where we
	     *  know for sure this is right.  set the 'have-to-swap-bytes'
	     *  flag and get ready to swab bytes in shorts/ints/floats.
	     */
	    if (trying_inverted)
		needswab++;
	    else {
		DXSetError(ERROR_DATA_INVALID, 
			   "byte order mismatch, can't read binary data");
		goto error;
	    }
	}
	break;

      case DISKVERSION_D:
      case DISKVERSION_C:
      case DISKVERSION_B:
      case DISKVERSION_A:
      default:
	DXSetError(ERROR_DATA_INVALID, 
		   "unsupported version number, can't read file");
	goto error;
    }

    /* blocking size, in bytes */
    if(flp->blocksize != oneblock) {
	DXSetError(ERROR_DATA_INVALID, 
		 "blocksize doesn't match, can't read file");
	goto error;
    }
    
    nh = PARTIAL_BLOCKS(flp->headerbytes, oneblock);

    DXDebug("S", "import %s: %d header blocks, %d leftover bytes", 
	  dataset, nh, flp->leftover);

    /* number of header blocks */
    if (nh > 1) {

	/* reallocate right number of blocks for header 
	 *  used to be alloc but on a socket we can't go back and reread
	 *  the first block, so this copies the first block over to the
	 *  new space and later, reads nblocks-1
	 */
	header_base = DXReAllocate(header_base, nh * oneblock + ONEK);
	if(!header_base)
	    goto error;
	
	header = header_base;
	byteoffset = 0;
	flp = (struct f_label *)header;
	
	/* read the rest of the object here */
	if (_dxffile_read(dataset, 0, nh-1, 
			 (Pointer)((char *)header+oneblock), 0) == ERROR)
	    goto error;

    }

    /* use defaults: first element in struct is the key; and no extra
     *  compare function is needed.
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if(!hashTable)
	goto error;

    /* start of body */
    bodyblk = nh;
    byteoffset += sizeof(struct f_label);

    header = (Pointer)((char *)header + byteoffset);

    /* object is already completely read in.  unpack from buffer and actually
     * create objects here.
     */
    o = readin_object(dataset, &header, hashTable, &byteoffset, 
			                             &bodyblk, 1);

    _dxfsock_close(dataset);
    restorecontext();
    DXFree(header_base);
    DXDestroyHash(hashTable);
    return o;

  error:
    restorecontext();
    DXFree(header_base);
    DXDestroyHash(hashTable);
    return NULL;
}



Object _dxfExportBin_FP(Object o, int fd)
{
    int nh = 0;			/* number of header bytes */
    int nb = 0;			/* number of body blocks */
    HashTable hashTable = NULL;
    Pointer header_base = NULL, header, hp;
    struct f_label *flp;
    uint byteoffset = 0;
    int bodyblk;
    char dataset[64];


#if 0 /* broken */
    oneblock = ONEBLK;
#else
    oneblock = HALFK;
#endif


    /* construct a fake name.  (the lower level read/write routines have
     *  a filename as the first parm, not a file descriptor.)
     */
    sprintf(dataset, "socket %x export", fd);

    /* count number of blocks (on non-ibmpvs is this 512 bytes, not 64k?)
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if (!hashTable)
	goto error;
    
    cutoff = 0;   /* all in header, none in body */
    if (size_estimate(o, &nh, &nb, 1, cutoff, hashTable) == ERROR)
	goto error;

    DXDestroyHash(hashTable);
    hashTable = NULL;


    DXDebug("S", "export %s: %d headerbyte, %d bodyblocks, cutoff %d Kb", 
	  dataset, nh, nb, cutoff);

    if (_dxfsock_open(dataset, fd) == ERROR)
	goto error;

    if (savecontext() != 0)
	goto error;

    nh += sizeof(struct f_label);

    header_base = DXAllocate(ROUNDUP_BYTES(nh, oneblock) + ONEK);
    if (!header_base)
	goto error;

    header = (Pointer)ROUNDUP_BYTES(header_base, ONEK);

    /* use defaults: first element in struct is the key; and no extra
     *  compare function is needed.
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if (!hashTable)
	goto error;

    hp = header;
    byteoffset = 0;

    /* fill in label */
    flp = (struct f_label *)header;
    header = (Pointer)((byte *)header + sizeof(struct f_label));
    byteoffset += sizeof(struct f_label);


    /* version number */
    flp->version = DISKVERSION_E;

    /* cutoff between header and body */
    flp->cutoff = cutoff;

    /* blocksize */
    flp->blocksize = oneblock;

    /* number of header bytes */
    flp->headerbytes = nh;

    /* file type */
    flp->type = DISKTYPE_NORMAL;

    /* file format */
    flp->format = DISKFORMAT_BIN;

    /* number of valid bytes in the last block of ?? header or body? */
    /* header for now - although i'm not sure what i'm going to do with it */
    flp->leftover = LEFTOVER_BYTES(nh, oneblock);

    /* the native byteorder */
    flp->byteorder = MSB_MACHINE;


    /* write out object */
    bodyblk = PARTIAL_BLOCKS(nh, oneblock);

    /* calls _dxffile_write(array bodies > cutoff) directly */
    if (writeout_object(o, dataset, hashTable, &header, &byteoffset, 
		                                      &bodyblk, 1) == ERROR)
	goto error;

    DXDebug("S", "actual bytes: %d, estimated bytes: %d", byteoffset, nh);

    /* make sure the actual header isn't longer than the estimate */
    if (PARTIAL_BLOCKS(byteoffset, oneblock) > PARTIAL_BLOCKS(nh, oneblock)) {
	DXSetError(ERROR_INTERNAL, "import header estimate too small");
	goto error;
    }

    /* nothing has been written up to this point */
    if (_dxffile_write(dataset, 0, PARTIAL_BLOCKS(nh, oneblock), hp, 0)	== ERROR)
	goto error;


    _dxfsock_close(dataset);
    restorecontext();
    DXFree(header_base);
    DXDestroyHash(hashTable);
    return o;

  error:
    _dxfsock_close(dataset);
    restorecontext();
    DXFree(header_base);
    DXDestroyHash(hashTable);
    return NULL;
}

/*
 * In-memory serialization for MPI_based packetting
 */

Object _dxfImportBin_Buffer(void *buffer, int *size)
{
    Object o = NULL;
    int nh = 0;			/* number of header blocks */
    HashTable hashTable = NULL;
    Pointer header_base = NULL, header;
    struct f_label *flp;
    uint byteoffset = 0;
    int trying_inverted = 0;
    int bodyblk = 0;
    char dataset[64];

    oneblock = 1;

    /* construct a fake name.  (the lower level read/write routines have
     *  a filename as the first parm, not a file descriptor.)
     */
    sprintf(dataset, "socket import");

    if (!buffer)
	goto error;

    header_base = buffer;
    header      = header_base;
    byteoffset  = 0;

again:
    /*
     * check label to see if it matches
     */
    flp = (struct f_label *)header;
    
    version  = flp->version;   /* these three are static globals */
    cutoff   = flp->cutoff;
    needswab = 0;

    /*
     * don't support anything but the current version number
     */
    switch(flp->version)
    {
      case DISKVERSION_IE:
	/*
	 * might be a version we recognize but with an inverted byte order.
	 *  invert the header, set a flag and try again.
	 */
	trying_inverted++;
	_dxfByteSwap((void *)flp, (void*)flp, 
		     (int)(sizeof(struct f_label)/sizeof(int)), TYPE_INT);
	goto again;

      case DISKVERSION_E:
	if (flp->byteorder != MSB_MACHINE)
	{
	    /*
	     * if we've already inverted the header, then this is where we
	     *  know for sure this is right.  set the 'have-to-swap-bytes'
	     *  flag and get ready to swab bytes in shorts/ints/floats.
	     */
	    if (trying_inverted)
		needswab++;
	    else
	    {
		DXSetError(ERROR_DATA_INVALID, 
			   "byte order mismatch, can't read binary data");
		goto error;
	    }
	}
	break;

      case DISKVERSION_D:
      case DISKVERSION_C:
      case DISKVERSION_B:
      case DISKVERSION_A:
      default:
	DXSetError(ERROR_DATA_INVALID, 
		   "unsupported version number, can't read file");
	goto error;
    }

    /*
     * blocking size, in bytes
     */
    if (flp->blocksize != oneblock)
    {
	DXSetError(ERROR_DATA_INVALID, 
		 "blocksize doesn't match, can't read file");
	goto error;
    }
    
    nh = PARTIAL_BLOCKS(flp->headerbytes, oneblock);

    DXDebug("S", "import %s: %d header blocks, %d leftover bytes", 
	  dataset, nh, flp->leftover);

    /*
     * use defaults: first element in struct is the key; and no extra
     * compare function is needed.
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if(!hashTable)
	goto error;

    /* start of body */
    bodyblk = nh;
    byteoffset = sizeof(struct f_label);

    header = (Pointer)((char *)header + byteoffset);

    /* object is already completely read in.  unpack from buffer and actually
     * create objects here.
     */
    o = readin_object(dataset, &header, hashTable, &byteoffset, &bodyblk, 1);

    *size = byteoffset;

    DXDestroyHash(hashTable);
    return o;

  error:
    DXDestroyHash(hashTable);
    return NULL;
}

Object
_dxfExportBin_Buffer(Object o, int *size, void **buffer)
{
    int nh = 0;			/* number of header bytes */
    int nb = 0;			/* number of body blocks */
    HashTable hashTable = NULL;
    Pointer header_base = NULL, header;
    struct f_label *flp;
    uint byteoffset = 0;
    int bodyblk;
    char dataset[64];

    oneblock = 1; /* no blocking for in-memory serialization */

    /* construct a fake name.  (the lower level read/write routines have
     *  a filename as the first parm, not a file descriptor.)
     */
    sprintf(dataset, "socket export");

    /* count number of blocks (on non-ibmpvs is this 512 bytes, not 64k?)
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if (!hashTable)
	goto error;
    
    cutoff = 0;   /* all in header, none in body */
    if (size_estimate(o, &nh, &nb, 1, cutoff, hashTable) == ERROR)
	goto error;

    /*
     * nb better be 0!
     */

    DXDestroyHash(hashTable);
    hashTable = NULL;


    DXDebug("S", "export %s: %d headerbyte, %d bodyblocks, cutoff %d Kb", 
	  dataset, nh, nb, cutoff);

    nh += sizeof(struct f_label);

    header_base = DXAllocate(ROUNDUP_BYTES(nh, oneblock) + ONEK);
    if (!header_base)
	goto error;

    /* use defaults: first element in struct is the key; and no extra
     *  compare function is needed.
     */
    hashTable = DXCreateHash(sizeof(struct hashElement), NULL, NULL);
    if (!hashTable)
	goto error;

    header = header_base;
    byteoffset = 0;

    /* fill in label */
    flp = (struct f_label *)header;
    header = (Pointer)((byte *)header + sizeof(struct f_label));
    byteoffset += sizeof(struct f_label);

    /* version number */
    flp->version = DISKVERSION_E;

    /* cutoff between header and body */
    flp->cutoff = cutoff;

    /* blocksize */
    flp->blocksize = oneblock;

    /* number of header bytes */
    flp->headerbytes = nh;

    /* file type */
    flp->type = DISKTYPE_NORMAL;

    /* file format */
    flp->format = DISKFORMAT_BIN;

    /* number of valid bytes in the last block of ?? header or body? */
    /* header for now - although i'm not sure what i'm going to do with it */
    flp->leftover = LEFTOVER_BYTES(nh, oneblock);

    /* the native byteorder */
    flp->byteorder = MSB_MACHINE;

    /* write out object */
    bodyblk = PARTIAL_BLOCKS(nh, oneblock);

    /*
     * calls _dxffile_write(array bodies > cutoff) directly
     * ... but we have set cutoff to 0, so everything goes in header?  GDA
     */
    if (writeout_object(o, dataset, hashTable, &header, &byteoffset, 
		                                      &bodyblk, 1) == ERROR)
	goto error;

    DXDebug("S", "actual bytes: %d, estimated bytes: %d", byteoffset, nh);

    *buffer = header_base;
    *size   = byteoffset;

    /* make sure the actual header isn't longer than the estimate */
    if (PARTIAL_BLOCKS(byteoffset, oneblock) > PARTIAL_BLOCKS(nh, oneblock)) {
	DXSetError(ERROR_INTERNAL, "export header estimate too small");
	goto error;
    }

    DXDestroyHash(hashTable);
    return o;

  error:
    DXFree(header_base);
    DXDestroyHash(hashTable);
    return NULL;
}


