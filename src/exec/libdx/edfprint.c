/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "edf.h"

#include <stdlib.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

static struct ap_info {
    char *format;
    int stride;
} ap[] = { 
	/* strings */
	{ NULL,                  0 },   /* use pstring instead */
	/* unsigned bytes */
	{ "%3u",                16 },
	/* bytes */
	{ "%3d",                16 },
	/* unsigned shorts */
	{ "%6u",                 8 },
	/* shorts */
	{ "%6d",                 8 },
	/* unsigned ints */
	{ "%9u",                 8 },
	/* ints */
	{ "%9d",                 8 },
	/* floats */
	{ "%12.7g",              6 },
	/* doubles */
	{ "%15g",                4 },
};


struct fmt_info {
    HashTable ht;       /* hash on object addr to return id */
    struct dict *d;	/* string dictionary */
    FILE *fp;           /* header file pointer */
    FILE *dfp;          /* data file pointer, if not NULL, binary data */
    char *dfile;        /* data file name, if not NULL, separate file */
    int pipe;		/* set if writing data to an external processor */
    int dformat;        /* data formats if not text: msb/lsb, xdr/ieee */
    int lastid;         /* last object number to be used */
    int recurse;        /* if defining attributes, don't recurse */
    int toplevel;       /* set for top level object */
};

static void pinfo(FILE *fp, int items, Type t, Category c, int rank, 
		  int *shape);
static void pvalue(FILE *fp, Type t, Category c, int rank, int *shape, 
	    Pointer value);
static void ptype(FILE *fp, Type type);
static void pstring(FILE *fp, char *cp);
static void pattrib(Object o, struct fmt_info *p);
static Error dattrib(Object o, struct fmt_info *p);
static Error swapwrite(FILE *datafile, Array array);
static Error nativewrite(FILE *datafile, int itemcount, int itemsize, 
			 char *dataptr);


Object DXExportDX (Object o, char *filename, char *format);
Object _dxfExportDX (Object o, char *filename, char *format);
Object _dxfExportDX_FD (Object o, FILE *fptr, char *format);
static Error _dxfObject_Format (Object o, struct fmt_info *p);

static Error Camera_Format (Camera camera, struct fmt_info *p);
static Error Light_Format (Light light, struct fmt_info *p);
static Error Array_Format (Array array, struct fmt_info *p);
static Error Clipped_Format (Clipped clipped, struct fmt_info *p);
static Error Field_Format (Field field, struct fmt_info *p);
static Error String_Format (String string, struct fmt_info *p);
static Error Xform_Format (Xform xform, struct fmt_info *p);
static Error Group_Format (Group group, struct fmt_info *p);
static Error Screen_Format (Screen screen, struct fmt_info *p);

#if DXD_POPEN_OK && DXD_HAS_LIBIOP
#define popen popen_host
#define pclose pclose_host
#endif


static 
char *objid(struct fmt_info *p, Object o)
{
    int id, literal;
    static char fred[256];

    if (!_dxflook_objident(p->ht, o, &id, &literal))
	sprintf(fred, "unknown");
    else if (literal)
	sprintf(fred, "%d", id);
    else
	sprintf(fred, "\"%s\"", _dxfdictname(p->d, id));
	
	
    return fred;
}

/* old cover function for compatibility.
 */
Object _dxfExportDX(Object o, char *filename, char *format)
{
    return DXExportDX(o, filename, format);
}

/* external entry point.
 */
Object DXExportDX(Object o, char *filename, char *format)
{
    int rc = OK;
    struct fmt_info p;
    char *cp, *cp2, *fname = NULL, *fname2 = NULL;
    char lcasefmt[32], *lcp;
    struct dict d;

    p.ht = NULL;
    p.d = &d;
    p.dfp = NULL;
    p.dfile = NULL;
    p.pipe = 0;
    p.dformat = 0;
    p.lastid = 0;
    p.recurse = 1;
    p.toplevel = 1;

    if (!o) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return NULL;
    }

    if (!filename) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "filename");
	return NULL;
    }

    fname = (char *)DXAllocateLocal(strlen(filename) + 20);
    if (!fname)
	return NULL;
    fname2 = (char *)DXAllocateLocal(strlen(filename) + 20);
    if (!fname2)
	return NULL;

#if DXD_POPEN_OK

    /* check for a leading !
     */
    if (filename[0] == '!') {
	p.pipe++;
	p.fp = popen(filename+1, "w");
	if (!p.fp) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10720", filename+1);
	    rc = ERROR;
	    goto done;
	}
	p.dformat = D_FOLLOWS | D_TEXT;
	
    } else {
#endif
	/* look for an extension in the last part of the pathname.  if it
	 *  doesn't exist, add ".dx".  if it does and is already .dx, leave
	 *  it alone - otherwise add .dx anyway.
	 */
	strcpy(fname, filename);
	if ((cp = strrchr(fname, '/')) == NULL)
	    cp = fname;
	if ((cp2 = strrchr(cp, '.')) == NULL || cp2 < cp)
	    strcat(fname, ".dx");
	else if (strcmp(cp2, ".dx"))
	    strcat(fname, ".dx");
	
	p.fp = fopen(fname, "w+");
	if (!p.fp) {
	    DXSetError(ERROR_BAD_PARAMETER, "#12248", fname);
	    rc = ERROR;
	    goto done;
	}

#if DXD_POPEN_OK
    }
#endif
    
    /* turn format string into lower case, and look for keywords.
     */
    if (format) {
	for (cp=format, lcp=lcasefmt; *cp && lcp < &lcasefmt[32]; cp++, lcp++)
	    *lcp = isupper(*cp) ? tolower(*cp) : *cp;
	*lcp = '\0';

	for (cp = lcasefmt; *cp; ) {
	    if (isspace(*cp)) {
		cp++;
		continue;
	    }
	    if (!strncmp(cp, "dx", 2)) {
		cp += 2;
		continue;
	    }
	    if (!strncmp(cp, "msb", 3)) {
		p.dformat |= D_MSB;
		cp += 3;
		continue;
	    }
	    if (!strncmp(cp, "lsb", 3)) {
		p.dformat |= D_LSB;
		cp += 3;
		continue;
	    }
	    if (!strncmp(cp, "binary", 6)) {
		p.dformat |= D_IEEE;
		cp += 6;
		continue;
	    }
	    if (!strncmp(cp, "ieee", 4)) {
		p.dformat |= D_IEEE;
		cp += 4;
		continue;
	    }
	    if (!strncmp(cp, "xdr", 3)) {
		p.dformat |= D_XDR;
		cp += 3;
		continue;
	    }
	    if (!strncmp(cp, "text", 4)) {
		p.dformat |= D_TEXT;
		cp += 4;
		continue;
	    }
	    if (!strncmp(cp, "ascii", 5)) {
		p.dformat |= D_TEXT;
		cp += 5;
		continue;
	    }
	    if (!strncmp(cp, "follows", 7)) {
		p.dformat |= D_FOLLOWS;
		cp += 7;
		continue;
	    }
	    if (*cp == '1') {
		p.dformat |= D_ONE;
		cp ++;
		continue;
	    }
	    if (*cp == '2') {
		p.dformat |= D_TWO;
		cp ++;
		continue;
	    }
	    
	    DXSetError(ERROR_BAD_PARAMETER, "#12238", cp);
	    rc = ERROR;
            goto done;
	}
    }

    /* error checks */

#if DXD_POPEN_OK
    if (p.pipe && (p.dformat != (D_FOLLOWS|D_TEXT))) {
	DXSetError(ERROR_BAD_PARAMETER, 
		 "external filter format must be 'follows' and 'text'");
	rc = ERROR;
        goto done;
    }
#endif

    switch (p.dformat & D_BYTES) {
      case D_MSB:
      case D_LSB:
	break;
      case 0:
	p.dformat |= D_NATIVE;
	break;
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#12160", "`msb' or `lsb'");
	rc = ERROR;
	goto done;
    }
    
    switch (p.dformat & D_FORMATS) {
      case D_IEEE:
      case D_XDR:
      case D_TEXT:
	break;
      case 0:
	p.dformat |= D_IEEE;
	break;
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#12160", 
		   "`binary', `ieee', `xdr', `text' or `ascii'");
	rc = ERROR;
	goto done;
    }
    
    switch(p.dformat & D_FILES) {
      case D_FOLLOWS:
      case D_ONE:
      case D_TWO:
	break;
      case 0:
	p.dformat |= D_ONE;
	break;
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#12160", "`follows', `1' or `2'");
        rc = ERROR;
	goto done;
    }
    
    /* if two files, make the second basename.bin, or if text, .data */
    if (p.dformat & (D_ONE | D_TWO)) {
	char *ep = ".bin";

	if (p.dformat & D_TEXT)
	    ep = ".data";

	strcpy(fname2, filename);
	if ((cp = strrchr(fname2, '/')) == NULL)
	    cp = fname2;
	if ((cp2 = strrchr(cp, '.')) == NULL || cp2 < cp) {
	    strcat(fname2, ep);
	} else {
	    if (!strcmp(cp2, ".dx"))
		strcpy(cp2, ep);
	    else
		strcat(fname2, ep);
	}
	/* hack for single files - add a proc id */
	if (p.dformat & D_ONE)
	    sprintf(fname2 + strlen(fname2), "%d", getpid());

	p.dfp = fopen(fname2, "w+");
	if (!p.dfp) {
 	    DXSetError(ERROR_BAD_PARAMETER, "#12248", fname2);
	    rc = ERROR;
	    goto done;
	}
	/* one is just a hack here for now */
	if (p.dformat & (D_TWO|D_ONE))
	    p.dfile = fname2;
    }
    
    
    /* set up dictionary, object ident hash tables, and import object.
     */
    
    _dxfinitdict(p.d);
    _dxfinit_objident(&p.ht);

    rc = _dxfObject_Format(o, &p);
    fprintf(p.fp, "end\n");

    _dxfdeletedict(p.d);
    _dxfdelete_objident(p.ht);



    /* hack for single files */
    if ((p.dformat & D_ONE) && p.dfp) {
	char *iob = NULL;
	int nbytes;

#define IOSIZE  (1024*16)
	iob = (char *)DXAllocate(IOSIZE);
	if (!iob) {
	    rc = ERROR;
	    goto done;
	}

	if (fclose(p.dfp) < 0) {
	    DXFree((Pointer)iob);
	    p.dfp = NULL;
	    DXSetError(ERROR_DATA_INVALID, "#12249", fname2);
	    rc = ERROR;
	    goto done;
	}
	p.dfp = fopen(fname2, "r");
	if (!p.dfp) {
	    DXFree((Pointer)iob);
	    DXSetError(ERROR_UNEXPECTED, "#12240", fname2);
	    rc = ERROR;
	    goto done;
	}

	while((nbytes = fread(iob, 1, IOSIZE, p.dfp)) > 0) {
	    if( fwrite(iob, 1, nbytes, p.fp) != nbytes) {
                DXSetError(ERROR_INTERNAL, "#12249", fname2);
                rc = ERROR;
            }
        }

	DXFree((Pointer)iob);

	/* the wintel version gets unhappy with unlinking
	 * a file which is still open.  (unix deletes the dir
	 * entry but does not release the inode until the process
	 * exits or the last open descriptor is closed).  do an
	 * explicit close first before the unlink to make wintel happy.
	 * nsc01mar97
	 */
	if (fclose(p.dfp) < 0) {
	    DXSetError(ERROR_DATA_INVALID, "#12249", fname2);
	    p.dfp = NULL;
            rc = ERROR;
	    goto done;
        }
	p.dfp = NULL;
	unlink(fname2);
	DXFree((Pointer)fname2);
        fname2 = NULL;
    }

  done:
    if (p.fp != NULL) {
#if DXD_POPEN_OK
	if (p.pipe)
	    pclose(p.fp);
	else
#endif
	if (fclose(p.fp) < 0) {
	    DXSetError(ERROR_DATA_INVALID, "#12249", fname);
            rc = ERROR;
        }
    }
	    
    if (p.dfp != NULL) {
	if (fclose(p.dfp) < 0) {
	    DXSetError(ERROR_DATA_INVALID, "#12249", fname2);
            rc = ERROR;
        }
    }

    if (rc != OK) {
        if (fname)
            unlink(fname);
        if (fname2)
            unlink(fname2);
    }

    DXFree((Pointer)fname);
    DXFree((Pointer)fname2);
    return (rc == OK ? o : NULL);
}



/* private entry point.  parms are object, already open file stream,
 *  and a restricted set of formats.  the output WILL be follows,
 *  but text or binary is settable.
 */
Object _dxfExportDX_FD(Object o, FILE *fptr, char *format)
{
    int rc = OK;
    struct fmt_info p;
    char *cp;
    char lcasefmt[32], *lcp;
    struct dict d;

    p.ht = NULL;
    p.d = &d;
    p.fp = fptr;
    p.dfp = NULL;
    p.dfile = NULL;
    p.pipe = 1;
    p.dformat = 0;
    p.lastid = 0;
    p.recurse = 1;
    p.toplevel = 1;

    if (!o) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return NULL;
    }

    if (!fptr) {
	DXSetError(ERROR_INTERNAL, "#10000", "file stream pointer");
	return NULL;
    }

    
    /* turn format string into lower case, and look for keywords.
     */
    if (format) {
	for (cp=format, lcp=lcasefmt; *cp && lcp < &lcasefmt[32]; cp++, lcp++)
	    *lcp = isupper(*cp) ? tolower(*cp) : *cp;
	*lcp = '\0';

	for (cp = lcasefmt; *cp; ) {
	    if (isspace(*cp)) {
		cp++;
		continue;
	    }
	    if (!strncmp(cp, "dx", 2)) {
		cp += 2;
		continue;
	    }
	    if (!strncmp(cp, "msb", 3)) {
		p.dformat |= D_MSB;
		cp += 3;
		continue;
	    }
	    if (!strncmp(cp, "lsb", 3)) {
		p.dformat |= D_LSB;
		cp += 3;
		continue;
	    }
	    if (!strncmp(cp, "binary", 6)) {
		p.dformat |= D_IEEE;
		cp += 6;
		continue;
	    }
	    if (!strncmp(cp, "ieee", 4)) {
		p.dformat |= D_IEEE;
		cp += 4;
		continue;
	    }
	    if (!strncmp(cp, "xdr", 3)) {
		p.dformat |= D_XDR;
		cp += 3;
		continue;
	    }
	    if (!strncmp(cp, "text", 4)) {
		p.dformat |= D_TEXT;
		cp += 4;
		continue;
	    }
	    if (!strncmp(cp, "ascii", 5)) {
		p.dformat |= D_TEXT;
		cp += 5;
		continue;
	    }
	    if (!strncmp(cp, "follows", 7)) {
		p.dformat |= D_FOLLOWS;
		cp += 7;
		continue;
	    }
	    if (*cp == '1') {
		p.dformat |= D_ONE;
		cp ++;
		continue;
	    }
	    if (*cp == '2') {
		p.dformat |= D_TWO;
		cp ++;
		continue;
	    }
	    
	    DXSetError(ERROR_BAD_PARAMETER, "#12238", cp);
	    rc = ERROR;
            goto done;
	}
    }

    /* error checks - note defaults are DIFFERENT than normal */
    switch (p.dformat & D_BYTES) {
      case D_MSB:
      case D_LSB:
	break;
      case 0:
	p.dformat |= D_NATIVE;
	break;
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#12160", "`msb' or `lsb'");
	rc = ERROR;
	goto done;
    }
    
    /* used to be TEXT - now BINARY because it works now */
    switch (p.dformat & D_FORMATS) {
      case D_IEEE:
      case D_XDR:
      case D_TEXT:
	break;
      case 0:
	p.dformat |= D_IEEE;
	break;
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#12160", 
		   "`binary', `ieee', `xdr', `text' or `ascii'");
	rc = ERROR;
	goto done;
    }
    
    switch(p.dformat & D_FILES) {
      case D_FOLLOWS:
	break;
      case 0:
	p.dformat |= D_FOLLOWS;
      default:
	DXSetError(ERROR_BAD_PARAMETER, 
		 "in this mode, '1' or '2' not allowed");
	rc = ERROR;
        goto done;
    }
    
    
    /* set up dictionary, object ident hash tables, and import object.
     */
    
    _dxfinitdict(p.d);
    _dxfinit_objident(&p.ht);

    rc = _dxfObject_Format(o, &p);
    fprintf(p.fp, "end\n");

    _dxfdeletedict(p.d);
    _dxfdelete_objident(p.ht);


    /* don't close anything here, but flush it.
     */

  done:
    if (p.fp) fflush(p.fp);
    return (rc == OK ? o : NULL); 
}

/* see if the name attr is there and make sure it isn't null
 */
static char *nameattr(Object o)
{
    char *cp;

    if (!DXGetStringAttribute(o, "name", &cp))
	return NULL;

    if (!cp || !cp[0])
	return NULL;

    return cp;
}
    

static Error _dxfObject_Format(Object o, struct fmt_info *p)
{
    int rc = OK;
    int id, literal;
    char *name;

    if(!o) {
	DXSetError(ERROR_DATA_INVALID, "null object");
	return ERROR;
    }

    /* check the hash table for an entry with this address.
     * if found:
     *  you've already formatted this object, there isn't anything to do.
     * if not found:
     *  if there is a name attribute, put it in the dictionary and use that id;
     *  else increment a 'next-object' id and mark it literal.  special case
     *  the top level object?  it should be by name even if there isn't one.
     *  then put the id into the hash table.  then write out the object.
     */
    _dxflook_objident(p->ht, o, &id, &literal);
    if (id != 0)
	return OK;

    if (p->toplevel) {
	if ((name = nameattr(o)) != NULL)
	    id = _dxfputdict(p->d, name);
	else
	    id = _dxfputdict(p->d, "default");
	literal = 0;
	p->toplevel = 0;
    } else if ((name = nameattr(o)) != NULL) {
	/* if it's already in the dictionary - make the name unique? */
	if (_dxflookdict(p->d, name) != BADINDEX) {
	    /* the name already exists for another object - for now,
             *  just use a number.
	     */
	    id = ++p->lastid;
	    literal = 1;
	} else {
	    id = _dxfputdict(p->d, name);
	    literal = 0;
	}
    } else {
	id = ++p->lastid;
	literal = 1;
    }

    _dxfadd_objident(p->ht, o, id, literal);


    /* define any attribute objects before we start this object definition */
    p->recurse = 0;
    rc = dattrib(o, p);
    if (rc != OK)
	return ERROR;
    p->recurse = 1;

    switch(DXGetObjectClass(o)) {

      case CLASS_GROUP:
        rc = Group_Format((Group)o, p);
        break;

      case CLASS_FIELD:
        rc = Field_Format((Field)o, p);
        break;

      case CLASS_ARRAY:
        rc = Array_Format((Array)o, p);
        break;

      case CLASS_STRING:
	rc = String_Format((String)o, p);
	break;
	
      case CLASS_CAMERA:
	rc = Camera_Format((Camera)o, p);
	break;

      case CLASS_XFORM:
	rc = Xform_Format((Xform)o, p);
	break;

      case CLASS_LIGHT:
	rc = Light_Format((Light)o, p);
	break;
	
      case CLASS_CLIPPED:
	rc = Clipped_Format((Clipped)o, p);
	break;
	
      case CLASS_SCREEN:
	rc = Screen_Format((Screen)o, p);
	break;
	
      case CLASS_INTERPOLATOR:
	DXWarning("interpolator objects cannot be saved");
	break;
	
      case CLASS_PRIVATE:
	DXWarning("private objects cannot be saved");
	break;
	
      case CLASS_OBJECT:
      case CLASS_MAX:
      case CLASS_MIN:
      case CLASS_DELETED:
      default:
	DXSetError(ERROR_DATA_INVALID, "bad object");
	rc = ERROR;

    }

    if (rc != OK)
        return ERROR;

    /* now add attributes (but don't add attributes of attributes) */
    if (p->recurse)
	pattrib(o, p);

    /* end of object definition */
    fprintf(p->fp, "#\n");
    fflush(p->fp);
    return OK;
}


static Error Array_Format (Array array, struct fmt_info *p)
{
    Type type;
    Category category;
    Class aclass;
    int items, rank, shape[MAXRANK];
    Array terms[MAXRANK];
    int stride;
    Pointer origin, delta;
    FILE *datafile = NULL;
    int i, j, rc;
    int numtype;
    char *format;
    char *cp;
    
    
    switch((aclass = DXGetArrayClass(array))) {
	
      case CLASS_ARRAY:
      case CLASS_CONSTANTARRAY:
	DXGetArrayInfo(array, &items, &type, &category, &rank, shape);
	fprintf(p->fp, "object %s class %s ", 
		objid(p, (Object)array), 
		(aclass == CLASS_ARRAY) ? "array" : "constantarray");
	pinfo(p->fp, items, type, category, rank, shape);
	
        if (items == 0) {
            fprintf(p->fp, "\n");
            break;
        }

	/* this takes into account category, rank and shape */
	if (aclass == CLASS_CONSTANTARRAY)
	    items = 1;

	items *= (DXGetItemSize(array) / DXTypeSize(type));
	

	switch(type) {
	  case TYPE_STRING:	    numtype = 0;	    break;
	  case TYPE_UBYTE:	    numtype = 1;	    break;
	  case TYPE_BYTE:	    numtype = 2;	    break;
	  case TYPE_USHORT:	    numtype = 3;	    break;
	  case TYPE_SHORT:	    numtype = 4;	    break;
	  case TYPE_UINT:      	    numtype = 5;	    break;
	  case TYPE_INT:      	    numtype = 6;	    break;
	  case TYPE_FLOAT:	    numtype = 7;	    break;
	  case TYPE_DOUBLE:	    numtype = 8;	    break;
	  case TYPE_HYPER:     	    numtype = 9;	    break;
	  default:
            DXSetError(ERROR_DATA_INVALID, "bad type");
	    return ERROR;
	}
	
	
	if ((p->dformat & D_FORMATS) != D_TEXT) {
	    switch(p->dformat & D_BYTES) {
	      case D_NATIVE:
#if defined(WORDS_BIGENDIAN)
		fprintf(p->fp, "msb ");
#else
		fprintf(p->fp, "lsb ");
#endif
		break;
	      case D_MSB:
		fprintf(p->fp, "msb ");
		break;
	      case D_LSB:
		fprintf(p->fp, "lsb ");
		break;
	      default:
                DXSetError(ERROR_DATA_INVALID, "unrecognized byte order");
		return ERROR;
	    }
	}

	switch(p->dformat & D_FORMATS) {
	  case D_TEXT:
	    break;
	  case D_IEEE:
	    fprintf(p->fp, "ieee ");
	    break;	
	  case D_XDR:
	    fprintf(p->fp, "xdr ");
	    break;
	  default:
            DXSetError(ERROR_DATA_INVALID, "unrecognized data format");
	    return ERROR;
	}

        fprintf(p->fp, "data ");
	switch(p->dformat & D_FILES) {
	  case D_FOLLOWS:
	    fprintf(p->fp, "follows\n");
	    fflush(p->fp);
	    datafile = p->fp;
	    break;
	  case D_ONE:
	    fprintf(p->fp, "%ld\n",  ftell(p->dfp));
	    datafile = p->dfp;
	    break;
	  case D_TWO:
	    fprintf(p->fp, "file %s,%ld\n", p->dfile, ftell(p->dfp));
	    datafile = p->dfp;
	    break;
	  default:
            DXSetError(ERROR_DATA_INVALID, "unrecognized number of files");
	    return ERROR;
	}


#define PRDATA(type) { \
\
    type *tp; \
\
    if (aclass == CLASS_CONSTANTARRAY) \
	tp = (type *)DXGetConstantArrayData(array); \
    else \
        tp = (type *)DXGetArrayData(array); \
    if (!tp) \
	return ERROR; \
\
    for(i=0; i<items; i++, tp++) { \
	fprintf(datafile, format, *tp); \
	if(!((i+1)%stride)) { \
	    fprintf(datafile, "\n"); \
	} else \
	    fprintf(datafile, " "); \
    } \
}
	
	if (p->dformat & D_TEXT) {
	
	    format = ap[numtype].format;
	    stride = (rank > 0 && shape[rank-1] <= ap[numtype].stride)
		                 ? shape[rank-1] : ap[numtype].stride;

	    switch(numtype) {
	      case 0:    /* ascii strings with special chars escaped */
		cp = (char *)DXGetArrayData(array);
		if (!cp)
		    return ERROR;
		if (rank == 0) {
		    rank = 1;
		    shape[0] = 1;
		}
		for(i=0; i<items/shape[rank-1]; i++, cp+=shape[rank-1])
		    pstring(datafile, cp);

		break;
		
	      case 1:
		PRDATA(ubyte); break;
	      case 2:
		PRDATA(byte); break;
	      case 3:
		PRDATA(ushort); break;
	      case 4:
		PRDATA(short); break;
	      case 5:
		PRDATA(uint); break;
	      case 6:
		PRDATA(int); break;
	      case 7:
		PRDATA(float); break;
	      case 8:
		PRDATA(double); break;
	      case 9:
		DXSetError(ERROR_NOT_IMPLEMENTED, 
			   "can't export 64bit integers");
		break;
	      default:
		DXSetError(ERROR_DATA_INVALID, "unrecognized data type");
		break;
	    }
	    fprintf(datafile, "\n");
	    break;

	} else if (p->dformat & D_IEEE) {

#if defined(WORDS_BIGENDIAN)
	    if ((p->dformat & D_BYTES) == D_LSB) {
#else
	    if ((p->dformat & D_BYTES) == D_MSB) {
#endif
		/* if not native format, swap bytes */
		swapwrite(datafile, array);

	    } else {
		/* native format */
		nativewrite(datafile, items, DXTypeSize(type), 
			    DXGetArrayData(array));
	    }

	    if ((p->dformat & D_FILES) == D_FOLLOWS) {
		fseek(datafile, 0, 1);
		fprintf(datafile, "\n");
	    }
	    break;
	} else {
	    DXSetError(ERROR_NOT_IMPLEMENTED, "xdr format not supported");
	    return ERROR;
	}
	break;
	    
      case CLASS_REGULARARRAY:
	DXGetArrayInfo(array, &items, &type, &category, &rank, shape);
	fprintf(p->fp, "object %s class regulararray ", 
		objid(p, (Object)array));
	fprintf(p->fp, "count %d\n", items);
	ptype(p->fp, type);

	origin = delta = NULL;
	origin = DXAllocateLocal(DXGetItemSize(array));
	delta = DXAllocateLocal(DXGetItemSize(array));
	if(!delta || !origin) {
	    if(delta) DXFree(delta);
	    if(origin) DXFree(origin);
	    break;
	}
	
	DXGetRegularArrayInfo((RegularArray)array, &items, origin, delta);
	fprintf(p->fp, " origin ");
	pvalue(p->fp, type, category, rank, shape, origin);
	
	fprintf(p->fp, " delta ");
	pvalue(p->fp, type, category, rank, shape, delta);
	fprintf(p->fp, "\n");

	DXFree(origin);
	DXFree(delta);
	break;
	
      case CLASS_PRODUCTARRAY:
	DXGetArrayInfo(array, &items, &type, &category, &rank, shape);
	origin = delta = NULL;
	origin = DXAllocateLocal(DXGetItemSize(array));
	delta = DXAllocateLocal(DXGetItemSize(array) * shape[0]);
	if(!delta || !origin) {
	    if(delta) DXFree(delta);
	    if(origin) DXFree(origin);
	    break;
	}
	
	if (DXQueryGridPositions(array, &rank, shape, 
			       (float *)origin, (float *)delta)) {
	    fprintf(p->fp, "object %s class gridpositions counts ", 
		    objid(p, (Object)array));
	    for (i=0; i<rank; i++)
		fprintf(p->fp, "%d ", shape[i]);
	    fprintf(p->fp, "\n");
	    
	    fprintf(p->fp, " origin ");
	    for (i=0; i<rank; i++)
		fprintf(p->fp, "%13.7g ", ((float *)origin)[i]);
	    fprintf(p->fp, "\n");
	    
	    for (i=0; i<rank; i++) {
		fprintf(p->fp, " delta  ");
		for (j=0; j<rank; j++)
		    fprintf(p->fp, "%13.7g ", ((float *)delta)[i*rank + j]);
		fprintf(p->fp, "\n");
	    }
	    
	    
	} else {
	    DXGetProductArrayInfo((ProductArray)array, &items, terms);
	    for(i=0; i<items; i++) { 
		rc = _dxfObject_Format((Object)terms[i], p);
		if (rc != OK)
		    return ERROR;
	    }
	    
	    fprintf(p->fp, "object %s class productarray\n", 
		    objid(p, (Object)array));
	    for(i=0; i<items; i++)
		fprintf(p->fp, " term %s\n", objid(p, (Object)terms[i]));
	}
	DXFree(delta);  
	DXFree(origin);
	break;
	
      case CLASS_PATHARRAY:
	DXGetPathArrayInfo((PathArray)array, &items);
	fprintf(p->fp, "object %s class path ", objid(p, (Object)array));
	fprintf(p->fp, "count %d\n", items);
	break;
	
      case CLASS_MESHARRAY:
	if (DXQueryGridConnections(array, &items, shape)) {
	    fprintf(p->fp, "object %s class gridconnections counts ", 
		    objid(p, (Object)array));
	    for (i=0; i<items-1; i++)
		fprintf(p->fp, "%d ", shape[i]);
	    fprintf(p->fp, "%d\n", shape[items-1]);
	    if(items > 0 && DXGetMeshOffsets((MeshArray)array, shape)) {
		for(i=0; i<items; i++)
		    if(shape[i] > 0)
			break;
		
		if(i != items) {
		    fprintf(p->fp, "meshoffsets ");
		    for(i=0; i<items-1; i++)
			fprintf(p->fp, "%d ", shape[i]);
		    fprintf(p->fp, "%d\n", shape[items-1]);
		}
	    }
	} else {
	    DXGetMeshArrayInfo((MeshArray)array, &items, terms);
	    for(i=0; i<items; i++) {
		rc = _dxfObject_Format((Object)terms[i], p);
		if (rc != OK)
		    return ERROR;
	    }

	    fprintf(p->fp, "object %s class mesharray ", 
		    objid(p, (Object)array));
	    for(i=0; i<items; i++)
		fprintf(p->fp, "term %s\n", objid(p, (Object)terms[i]));

	}
	break;
	
      default:
	DXSetError(ERROR_DATA_INVALID, "unknown array type");
	return ERROR;
    }

    return OK;
}


static Error Camera_Format (Camera camera, struct fmt_info *p)
{
    float world, aspect;
    Vector up;
    Point from, to;
    int height, width;
    RGBColor backgrnd;
    int isortho = 1;

    if (DXGetOrthographic(camera, &world, &aspect))
        fprintf(p->fp, "object %s class camera type orthographic\n", 
                objid(p, (Object)camera));
        
    else if (DXGetPerspective(camera, &world, &aspect)) {
        fprintf(p->fp, "object %s class camera type perspective\n", 
                objid(p, (Object)camera));
	world = atan(world/2) * 360.0 / M_PI;
        isortho = 0;
    } else {
	DXSetError(ERROR_INTERNAL, "invalid type of camera");
	return ERROR;
    }

    DXGetView(camera, &from, &to, &up);
    fprintf(p->fp, " from %13.7g %13.7g %13.7g\n",
	    (double)from.x, (double)from.y, (double)from.z);
    fprintf(p->fp, " to   %13.7g %13.7g %13.7g\n",
	    (double)to.x, (double)to.y, (double)to.z);
    fprintf(p->fp, " up   %13.7g %13.7g %13.7g\n", 
	    (double)up.x, (double)up.y, (double)up.z);
    
    DXGetCameraResolution(camera, &width, &height);

    if (isortho)
	fprintf(p->fp, " resolution %d  width %g  aspect %g\n", 
		width, (double)world, (double)aspect);
    else
	fprintf(p->fp, " resolution %d  angle %g  aspect %g\n", 
		width, (double)world, (double)aspect);

    DXGetBackgroundColor(camera, &backgrnd);
    fprintf(p->fp, " color %g %g %g\n", backgrnd.r, backgrnd.g, backgrnd.b);


    return OK;
}

static Error Clipped_Format (Clipped clipped, struct fmt_info *p)
{
    Object render, clipping;
    int rc;

    DXGetClippedInfo(clipped, &render, &clipping);
    rc = _dxfObject_Format(clipping, p);
    if (rc != OK)
	return ERROR;
    rc = _dxfObject_Format(render, p);
    if (rc != OK)
	return ERROR;

    fprintf(p->fp, "object %s class clipped ", objid(p, (Object)clipped));
    fprintf(p->fp, "by %s ", objid(p, clipping));
    fprintf(p->fp, "of %s\n", objid(p, render));
    return OK;

}

static Error Screen_Format (Screen screen, struct fmt_info *p)
{
    Object o;
    int position, z, rc;

    DXGetScreenInfo(screen, &o, &position, &z);
    rc = _dxfObject_Format(o, p);
    if (rc != OK)
	return ERROR;

    fprintf(p->fp, "object %s class screen ", objid(p, (Object)screen));
    switch(position) {
      case 0:
	fprintf(p->fp, "world "); break;
      case 1:
	fprintf(p->fp, "viewport "); break;
      case 2:
	fprintf(p->fp, "pixel "); break;
      case 3:
	fprintf(p->fp, "stationary "); break;
    }

    switch(z) {
      case -1:
	fprintf(p->fp, "behind "); break;
      case 0:
	fprintf(p->fp, "inside "); break;
      case 1:
	fprintf(p->fp, "infront "); break;
    }
    
    fprintf(p->fp, "of %s\n", objid(p, o));
    return OK;
}


static Error Field_Format (Field field, struct fmt_info *p)
{

    Object subo;
    int i, rc;
    char *name;

    /* add code here to remove any derived components before 
     *  the components get written out.
     */

    for(i=0; (subo=DXGetEnumeratedComponentValue(field, i, &name)); i++) {
	if (!DXGetAttribute(subo, "der")) {
	    rc = _dxfObject_Format(subo, p);
	    if (rc != OK)
		return ERROR;
	}
    }
    
    fprintf(p->fp, "object %s class field\n", objid(p, (Object)field));

    for(i=0; (subo=DXGetEnumeratedComponentValue(field, i, &name)); i++) {
	if (!DXGetAttribute(subo, "der"))
	    fprintf(p->fp, "component \"%s\" value %s\n", 
		    name, objid(p, subo));
    }

    return OK;
}


static Error Light_Format (Light light, struct fmt_info *p)
{
    Vector v;
    RGBColor c;

    fprintf(p->fp, "object %s class light ", objid(p, (Object)light)); 
    if (DXQueryAmbientLight(light, &c))  {
	fprintf(p->fp, " type ambient\n color %13.7g %13.7g %13.7g\n", 
		c.r, c.g, c.b);
        return OK;

    } else if (DXQueryDistantLight(light, &v, &c)) {
	fprintf(p->fp, " type distant\n direction %13.7g %13.7g %13.7g\n color     %13.7g %13.7g %13.7g\n",
		v.x, v.y, v.z, c.r, c.g, c.b);
        return OK;
    
    } else if (DXQueryCameraDistantLight(light, &v, &c)) {
	fprintf(p->fp, " type distant\n direction %13.7g %13.7g %13.7g from camera\n color     %13.7g %13.7g %13.7g\n",
		v.x, v.y, v.z, c.r, c.g, c.b);
        return OK;
    } 

    DXSetError(ERROR_DATA_INVALID, "unrecognized light type");
    return ERROR;
}

static Error String_Format (String string, struct fmt_info *p)
{
    char *cp;

    fprintf(p->fp, "object %s class string ", objid(p, (Object)string));
    cp = (char *)DXGetString(string);
    pstring(p->fp, cp);

    return OK;

}

static Error Xform_Format (Xform xform, struct fmt_info *p)
{
    Matrix tm;
    Object subo;
    int rc;
    
    DXGetXformInfo(xform, &subo, &tm);
    rc = _dxfObject_Format(subo, p);
    if (rc != OK)
	return ERROR;

    fprintf(p->fp, "object %s class transform ", objid(p, (Object)xform));
    fprintf(p->fp, "of %s\n", objid(p, subo));
    fprintf(p->fp, " times %13.7g %13.7g %13.7g\n       %13.7g %13.7g %13.7g\n       %13.7g %13.7g %13.7g\n", 
	    (double)tm.A[0][0], (double)tm.A[0][1], (double)tm.A[0][2],
	    (double)tm.A[1][0], (double)tm.A[1][1], (double)tm.A[1][2],
	    (double)tm.A[2][0], (double)tm.A[2][1], (double)tm.A[2][2]);
    /* same line as last */    
    fprintf(p->fp, " plus  %13.7g %13.7g %13.7g\n", 
	    (double)tm.b[0], (double)tm.b[1], (double)tm.b[2]);

    return OK;
}

static Error Group_Format (Group group, struct fmt_info *p)
{
    int i, rc;
    float position;
    char *name;
    Object subo;


    switch(DXGetGroupClass(group)) {
	
      case CLASS_GROUP:
	for(i=0; (subo=DXGetEnumeratedMember(group, i, &name)); i++) {
	    rc = _dxfObject_Format(subo, p);
	    if (rc != OK)
		return ERROR;
	}

	fprintf(p->fp, "object %s class group\n", objid(p, (Object)group));
	
	goto group_common;

      case CLASS_COMPOSITEFIELD:
	for(i=0; (subo=DXGetEnumeratedMember(group, i, &name)); i++) {
	    rc = _dxfObject_Format(subo, p);
	    if (rc != OK)
		return ERROR;
	}

	fprintf(p->fp, "object %s class compositefield\n", 
		objid(p, (Object)group));
	
	goto group_common;

      case CLASS_MULTIGRID:
	for(i=0; (subo=DXGetEnumeratedMember(group, i, &name)); i++) {
	    rc = _dxfObject_Format(subo, p);
	    if (rc != OK)
		return ERROR;
	}
	
	fprintf(p->fp, "object %s class multigrid\n", 
		objid(p, (Object)group));
	
	goto group_common;

      group_common:
	for(i=0; (subo=DXGetEnumeratedMember(group, i, &name)); i++) {
	    if (name)
		fprintf(p->fp, "member \"%s\" value %s\n", 
			name, objid(p, subo));
	    else
		fprintf(p->fp, "member %d value %s\n", i, objid(p, subo));
	}
	break;
	
      case CLASS_SERIES:
	for(i=0; (subo=DXGetSeriesMember((Series)group, i, &position)); i++) { 
	    rc = _dxfObject_Format(subo, p);
	    if (rc != OK)
		return ERROR;
	}
	
	fprintf(p->fp, "object %s class series\n", objid(p, (Object)group));
	
	for(i=0; (subo=DXGetSeriesMember((Series)group, i, &position)); i++)
	    fprintf(p->fp, "member %d position %13.7g value %s\n", i, position,
		    objid(p, subo));
	
	break;
	
      default:
	DXSetError(ERROR_DATA_INVALID, "bad group type");
        return ERROR;
    }
    
    return OK;
}

static void ptype(FILE *fp, Type type)
{
    fprintf(fp, "type ");
    switch(type) {
      case TYPE_STRING:
	fprintf(fp, "string "); break;
      case TYPE_BYTE:
	fprintf(fp, "signed byte "); break;
      case TYPE_UBYTE:
	fprintf(fp, "unsigned byte "); break;
      case TYPE_SHORT:
	fprintf(fp, "short "); break;
      case TYPE_USHORT:
	fprintf(fp, "unsigned short "); break;
      case TYPE_INT:
	fprintf(fp, "int "); break;
      case TYPE_UINT:
	fprintf(fp, "unsigned int "); break;
      case TYPE_HYPER:
	fprintf(fp, "hyper "); break;
      case TYPE_FLOAT:
	fprintf(fp, "float "); break;
      case TYPE_DOUBLE:
	fprintf(fp, "double "); break;
      default:
	break;
    }
}

static void pinfo(FILE *fp, int items, Type type, Category category, 
	   int rank, int *shape)
{
    int i;
    
    ptype(fp, type);

    /* only put the category keyword out if it's not real */
    switch(category) {
      case CATEGORY_COMPLEX:
	fprintf(fp, "category complex "); break;
      case CATEGORY_QUATERNION:
	fprintf(fp, "category quaternion "); break;
      case CATEGORY_REAL:
      default:
	break;
    }

    fprintf(fp, "rank %d ", rank);
    if (rank > 0) {
	fprintf(fp, "shape ");
	for(i=0; i < rank; i++)
	    fprintf(fp, "%d ", shape[i]);
    }

    fprintf(fp, "items %d ", items);
    return;
}

static void pv(FILE *fp, Type t, Pointer value, int offset)
{
    switch(t) {
      case TYPE_STRING:
	pstring(fp, ((char *)value)+offset); break;
      case TYPE_SHORT:
        fprintf(fp, "%d ", ((short *)value)[offset]); break;
      case TYPE_INT:
        fprintf(fp, "%d ", ((int *)value)[offset]); break;
      case TYPE_FLOAT:
        fprintf(fp, "%g ", ((float *)value)[offset]); break;
      case TYPE_DOUBLE:
        fprintf(fp, "%g ", ((double *)value)[offset]); break;
      case TYPE_UINT:
        fprintf(fp, "%u ", ((uint *)value)[offset]); break;
      case TYPE_USHORT:
        fprintf(fp, "%u ", ((ushort *)value)[offset]); break;
      case TYPE_UBYTE:
        fprintf(fp, "%u ", ((ubyte *)value)[offset]); break;
      case TYPE_BYTE:
        fprintf(fp, "%d ", ((byte *)value)[offset]); break;
      case TYPE_HYPER:
      default:
        fprintf(fp, "? "); break;
    }
}


static void pvalue(FILE *fp, Type type, Category category, int rank, 
                   int *shape, Pointer value)
{
    int i, j;

    j = DXCategorySize(category);
    for (i=0; i<rank; i++)
	j *= shape[i];
    
    for (i=0; i<j; i++)
	pv(fp, type, value, i);

    fprintf(fp, " ");
}


static void pstring(FILE *fp, char *cp)
{
    
    if (!cp) {
	fprintf(fp, "\"\"\n");
	return;
    }

    putc('"', fp);
    while(*cp) {
	switch(*cp) {
#ifndef DXD_NO_ANSI_ALERT
	  case '\a': putc('\\', fp); putc('a', fp);  break;
#endif
	  case '\b': putc('\\', fp); putc('b', fp);  break;
	  case '\f': putc('\\', fp); putc('f', fp);  break;
	  case '\n': putc('\\', fp); putc('n', fp);  break;
	  case '\r': putc('\\', fp); putc('r', fp);  break;
	  case '\t': putc('\\', fp); putc('t', fp);  break;
	  case '\v': putc('\\', fp); putc('v', fp);  break;
	  case '\\': putc('\\', fp); putc('\\', fp); break;
	  case '"':  putc('\\', fp); putc('"', fp);  break;
	  default:
	    if (isprint(*cp))
		putc(*cp, fp);
	    else
		fprintf(fp, "\\%03o", (uint)*cp);  /* octal code */
	}
	++cp;
    }
    fprintf(fp, "\"\n");
}

/* at import time these are the only distinctions that can be made.  
 * "xxx" = string,  X.Y = float, and N = integer, rank = 0, real.
 * any other array/object type must be explicitly exported 
 * to preserve that information.
 */
#define SH_NONE   0
#define SH_STRING 1
#define SH_INT    2
#define SH_FLOAT  3


/* is this object a simple string, int or float?
 *  if dp != NULL, return a pointer to the data
 */
static int has_shorthand(Object o, Pointer *dp)
{
    char *cp;
    int rank;
    Type type;
    Category cat;
    int items;
    
    if (DXExtractString(o, &cp)) {
	if (dp)
	    *dp = (Pointer)cp;
	
	return SH_STRING;
    }
    
    if (DXGetObjectClass(o) == CLASS_ARRAY) {
	
	if (!DXGetArrayInfo((Array)o, &items, &type, &cat, &rank, NULL))
	    return SH_NONE;
	
	if (items != 1 || cat != CATEGORY_REAL || rank != 0)
	    return SH_NONE;
	
	if (type == TYPE_INT) {
	    if (dp)
		*dp = DXGetArrayData((Array)o);
	    
	    return SH_INT;
	}
	
	if (type == TYPE_FLOAT) {
	    if (dp)
		*dp = DXGetArrayData((Array)o);
	    
	    return SH_FLOAT;
	}
    }
    
    return SH_NONE;
}

/* define attribute objects.
 *  skip ones which have a shorthand description.
 */
static Error dattrib(Object o, struct fmt_info *p)
{
    Object subo;
    char *name;
    int i, rc;
    
    for(i=0; (subo=DXGetEnumeratedAttribute(o, i, &name)); i++) 
	if (!has_shorthand(subo, NULL)) {
	    rc = _dxfObject_Format(subo, p);
	    if (rc != OK)
		return ERROR;
	}

    return OK;
}

/* output attribute lines, including shorthand representations
 *  for strings and simple numbers.
 */
static void pattrib(Object o, struct fmt_info *p)
{
    Object subo;
    char *name;
    int i;
    Pointer dp;
    
    for(i=0; (subo=DXGetEnumeratedAttribute(o, i, &name)); i++) {
	fprintf(p->fp, "attribute \"%s\" ", name);
	switch (has_shorthand(subo, &dp)) {
	  case SH_STRING:
	    fprintf(p->fp, "string ");
	    pstring(p->fp, (char *)dp);
	    break;
	    
	  case SH_FLOAT:
	    fprintf(p->fp, "number %f\n", *(float *)dp);
	    break;
	    
	  case SH_INT:
	    fprintf(p->fp, "number %d\n", *(int *)dp);
	    break;
	    
	  default:
	    fprintf(p->fp, "value %s\n", objid(p, subo));
	    break;
	}
    }

}

static Error swapwrite(FILE *datafile, Array array)
{
    char *cp;
    
    Error rc = OK;
    
    Type type;
    Category cat;
    int items;
    int rank;
    int shape[MAXRANK];

    char *iob = NULL;
    int nbytes, total;

#define IOSIZE  (1024*16)

    if(!DXGetArrayInfo(array, &items, &type, &cat, &rank, shape))
	return ERROR;

    if(items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad item count");

    if (DXGetArrayClass(array) == CLASS_CONSTANTARRAY) {
	total = DXGetItemSize(array);
	cp = (char *)DXGetConstantArrayData(array);
    } else {
	total = DXGetItemSize(array) * items;
	cp = (char *)DXGetArrayData(array);
    }
    if (total <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad type, category or shape value");
    if (!cp)
	return ERROR;
    
    
    /* switch on datatype */
    switch(type) {
      case TYPE_UBYTE:
      case TYPE_BYTE:
	if(fwrite(cp, 1, total, datafile) != total)
	    DXErrorReturn(ERROR_DATA_INVALID, "write error");
	
	break;
	
      case TYPE_SHORT:
      case TYPE_USHORT:
	iob = (char *)DXAllocate(IOSIZE);
	if (!iob)
	    return ERROR;
	
	while (total > 0) {
	    nbytes = MIN(IOSIZE, total);
	    if (!SWAB(iob, cp, nbytes)) {
		rc = ERROR;
		goto error;
	    }
	    if (fwrite(iob, 1, nbytes, datafile) != nbytes) {
		DXSetError(ERROR_MISSING_DATA, "write error");
		rc = ERROR;
		goto error;
	    }
	    total -= nbytes;
	    cp += nbytes;
	}
	break;
	
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_FLOAT:
	iob = (char *)DXAllocate(IOSIZE);
	if (!iob)
	    return ERROR;
		
	while (total > 0) {
	    nbytes = MIN(IOSIZE, total);
	    if (!SWAW(iob, cp, nbytes)) {
		rc = ERROR;
		goto error;
	    }
	    if (fwrite(iob, 1, nbytes, datafile) != nbytes) {
		DXSetError(ERROR_MISSING_DATA, "write error");
		rc = ERROR;
		goto error;
	    }
	    total -= nbytes;
	    cp += nbytes;
	}
	break;
	
      case TYPE_HYPER:
      case TYPE_DOUBLE:
	iob = (char *)DXAllocate(IOSIZE);
	if (!iob)
	    return ERROR;
		
	while (total > 0) {
	    nbytes = MIN(IOSIZE, total);
	    if  (!SWAD(iob, cp, nbytes)) {
		rc = ERROR;
		goto error;
	    }
	    if (fwrite(iob, 1, nbytes, datafile) != nbytes) {
		DXSetError(ERROR_MISSING_DATA, "write error");
		rc = ERROR;
		goto error;
	    }
	    total -= nbytes;
	    cp += nbytes;
	}
	break;
 
      default:
	DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unrecognized data type");

     }

  error:
    DXFree((Pointer)iob);
    return rc;
}

static Error nativewrite(FILE *datafile, int itemcount, int itemsize, 
			 char *dataptr)
{
    if (!dataptr || !datafile || itemcount < 0 || itemsize < 0)
	goto error;

    if (fwrite(dataptr, itemsize, itemcount, datafile) != itemcount)
	goto error;

    return OK;

  error:
    DXSetError(ERROR_DATA_INVALID, "error writing array");
    return ERROR;
}
