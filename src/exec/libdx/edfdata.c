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
#include <memory.h>     /* For memcpy() in _dxfByteSwap() */
 
#include "edf.h"
 
#if !defined(DXD_STANDARD_IEEE)
/* when they are ready, include whichever file defines swab, swaw and swad */
/* they will be part of a library, and written in assembler */

#define FLOAT_SIGN	0x80000000
#define FLOAT_EXP	0x7F800000
#define FLOAT_MANT	0x007FFFFF
#define DOUBLE_SIGN	0x80000000
#define DOUBLE_EXP	0x7FF00000
#define DOUBLE_MANT1	0x000FFFFF
#define DOUBLE_MANT2	0xFFFFFFFF

void _dxffloat_denorm(int *from, int count, int *denorm_fp, int *bad_fp)
{

    for ( ; count > 0; from++, count--) {
	
	switch(*from & FLOAT_EXP) {
	  case FLOAT_EXP:	/* NaN and Infinity */
	    *from = 0;
	    (*bad_fp)++;
	    break;

	  case 0:		/* check for real zero */
	    if ((*from & FLOAT_MANT) == 0)
		break;

	    *from = 0;
	    (*denorm_fp)++;

	  default:
	    break;
	}
    }

    return;

}

void _dxfdouble_denorm(int *from, int count, int *denorm_fp, int *bad_fp)
{

    for ( ; count > 0; from += 2, count--) {
	
	switch(*(from+1) & DOUBLE_EXP) {
	  case DOUBLE_EXP:	/* NaN and Infinity */
	    *from = 0;
	    *(from+1) = 0;
	    (*bad_fp)++;
	    break;

	  case 0:		/* check for real zero */
	    if (((*(from+1) & DOUBLE_MANT1)==0) && ((*from & DOUBLE_MANT2)==0))
		break;

	    *from = 0;
	    *(from+1) = 0;
	    (*denorm_fp)++;

	  default:
	    break;
	}
    }

    return;
}
#endif

 
/* 
 * read data array routines
 */

/* Not needed if linking with BINOBJ
extern int _fmode;
*/ 
 
/* binary format.  reverse byte order if necessary.  the array space should
 *  have already been allocated before getting here.
 */
Error _dxfreadarray_binary(struct finfo *f, Array a, int format)
{
    FILE *fd;
    char *cp;

    int bytes;
    Error rc = OK;
    
    Type type;
    int items, actual;

#if defined(WORDS_BIGENDIAN)
    int nonnative = D_LSB;
#else
    int nonnative = D_MSB;
#endif

    fd = f->fd;

    if (!DXGetArrayInfo(a, &items, &type, NULL, NULL, NULL))
	return ERROR;
    
    if (items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad item count, rank or shape");

    cp = (char *)DXGetArrayData(a);

    /* read the data 
     */
#if 0
	{
		// This was in there testing _fmode for the Windos version ... GDA
		int start, sz0, sz1, fsize = DXGetItemSize(a) * items;
		for (start = 0; start < fsize; start += sz1)
		{
			sz0 = 4096; 
			if (sz0 > (fsize - start))
				sz0 = fsize - start;
			sz1 = fread((void *)(cp+start), 1, sz0, fd);
			if (sz1 != sz0)
			{
				int err = ferror(fd);
				DXSetError(ERROR_DATA_INVALID, 
					"error %d reading binary data; %d bytes successfully read (%d expected)",
					err, sz1, sz0);
				return ERROR;
			}
			start += sz1;
		}
	}
#else
    if ((actual = fread(cp, DXGetItemSize(a), items, fd)) != items) {
		int err = ferror(fd);
	DXSetError(ERROR_DATA_INVALID, 
       "error %d reading binary data; %d items successfully read (%d expected)",
		   err, actual, items);
	return ERROR;
    }
#endif
 
    /* if user specifically requested the non-native format, 
     *  swab bytes within the right size word.
     */
    if ((format & D_BYTES) == nonnative) {

	bytes = DXGetItemSize(a) * items;

	/* what you swap depends on the datatype */
	switch(type) {
	  case TYPE_SHORT:
	  case TYPE_USHORT:
	    if (!SWAB(cp, cp, bytes))
		return ERROR;
	    break;
	
	  case TYPE_INT:
	  case TYPE_UINT:
	    if  (!SWAW(cp, cp, bytes))
		return ERROR;
	    break;

	  case TYPE_FLOAT:
	    if  (!SWAW(cp, cp, bytes))
		return ERROR;
#if !defined(DXD_STANDARD_IEEE)
	    _dxffloat_denorm((int *)cp, bytes/sizeof(float), 
			     &f->denorm_fp, &f->bad_fp);
#endif
	    break;
	    
	  case TYPE_DOUBLE:
	    if (!SWAD(cp, cp, bytes))
		return ERROR;
	
#if !defined(DXD_STANDARD_IEEE)
	    _dxfdouble_denorm((int *)cp, bytes/sizeof(double), 
			      &f->denorm_fp, &f->bad_fp);
#endif
	    break;
 
	  default:
	    break;
 
	}
    } 
#if !defined(DXD_STANDARD_IEEE)
    else {  
	/* don't have to swab data, but still have to check for denorms 
	 *  if the data is float or double and the processor is i860
	 */

	/* only for floats or doubles */
	switch(type) {
	  case TYPE_FLOAT:
	    bytes = DXGetItemSize(a) * items;
	    _dxffloat_denorm((int *)cp, bytes/sizeof(float), 
			     &f->denorm_fp, &f->bad_fp);
	    break;
	    
	  case TYPE_DOUBLE:
	    bytes = DXGetItemSize(a) * items;
	    _dxfdouble_denorm((int *)cp, bytes/sizeof(double), 
			      &f->denorm_fp, &f->bad_fp);
	    break;
	}
    }
#endif
 
    return rc;
}
 
 
/* binary format.  skip over the right number of bytes.
 */
Error _dxfskiparray_binary(struct finfo *f, int items, int itemsize)
{
    int rc;

    rc = fseek(f->fd, (long)items * itemsize, 1);
    if (rc == 0)
	return OK;

    return ERROR;
}
 
 
/* text format
 */
Error _dxfreadarray_text(struct finfo *f, Array a)
{
    Error rc = OK;
 
    Type type;
    Category cat;
    int items, i;
    int rank;
    int shape[MAXRANK];
 
    if(!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
	return ERROR;
 
    if(items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad item count");

    for (i=0; i<rank; i++)
	items *= shape[i];

    if(items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad shape value");


    items *= DXCategorySize(cat);
    if (items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad category value");

/* the seterror message below looks strange because dname is a string.
 *  it will say "... type" "float" etc. after macro expansion.
 */ 
#define GETITEMS(dtype,dname) \
{ \
    dtype *dp; \
    int i; \
\
    dp = (dtype *)DXGetArrayData(a); \
    for(i=0; i<items; i++, dp++) { \
        rc = _dxfmatch##dtype(f, dp); \
        if(!rc) { \
            DXSetError(ERROR_DATA_INVALID,  \
                       "error reading text data; item %d, type " dname, i+1); \
            return ERROR; \
        } \
    } \
}      

#if 0
    /* at this point we haven't parsed the next thing after FOLLOWS yet.
     *  for strings, we want to set nodict first.  for all the others,
     *  go ahead and read in the first number.
     */
    if (type != TYPE_STRING) {
	rc = skipkeyword(f);
	if (!rc)
	    return rc;
    }
#endif

    /* datatype */
    switch(type) {
      case TYPE_STRING:
	{ 
	    char *cp = (char *)DXGetArrayData(a);
	    char *dp;
	    int j, len;

	    /* make sure rank-1 means something down below 
	     * for strings, last rank is max length of each string
	     */
	    if (rank == 0) {
		rank = 1;
		shape[0] = 1;
	    }

	    /* for the following strings - don't put them in the
             *  dictionary since they are likely not to be used again.
	     */
	    _dxfsetdictstringmode(f, 1);

	    for (j=0; j<items/shape[rank-1]; j++, cp+= shape[rank-1]) {
		if (j != 0)
		    rc = _dxflexinput(f);
		dp = _dxfgetstringinfo(f, &len);
		if (!rc || !dp) {
		    DXSetError(ERROR_DATA_INVALID,
		       "error reading text data; item %d, type string",  j+1);
		    return ERROR;
		}
		if (len >= shape[rank-1]) {
		    DXSetError(ERROR_DATA_INVALID,
	   "error reading text data; string item %d longer than array shape",
			       j+1);
		    return ERROR;
		}
		strcpy(cp, dp);
	    }

	    /* start putting strings back into the dictionary */
	    _dxfsetdictstringmode(f, 0);
	    _dxflexinput(f);

        }
	break;

      case TYPE_UBYTE:
        GETITEMS(ubyte, "unsigned byte")
	break;
 
      case TYPE_BYTE:
        GETITEMS(byte, "signed byte")
	break;
 
      case TYPE_SHORT:
        GETITEMS(short, "short");
        break;
	
      case TYPE_USHORT:
        GETITEMS(ushort, "unsigned short");
        break;
	
      case TYPE_INT:
        GETITEMS(int, "integer");
        break;
	
      case TYPE_UINT:
        GETITEMS(uint, "unsigned integer");
        break;

      case TYPE_FLOAT:
        GETITEMS(float, "float");
        break;
 
      case TYPE_DOUBLE:
        GETITEMS(double, "double");
        break;

      default:
	DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
 
    }
 
    return rc;
}
 
 
/* text format
 */
Error _dxfskiparray_text(struct finfo *f, int items, Type type)
{
    Error rc = OK;
 
    if(items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad item count");

    /* set it to NOT construct numbers but still return ok and to
     *  not construct strings.
     */
    _dxfsetskipnum(f, 1);
    _dxfsetdictstringmode(f, 1);

/* the seterror message below looks strange because dname is a string.
 *  it will say "... type" "float" etc. after macro expansion.
 */ 
#define GETITEMSX(dtype,dname) \
{ \
    dtype d; \
    int i; \
\
    for(i=0; i<items; i++) { \
        rc = _dxfmatch##dtype(f, &d); \
        if(!rc) { \
            DXSetError(ERROR_DATA_INVALID,  \
                       "error reading text data; item %d, type " dname, i+1); \
            goto error; \
        } \
    } \
}      

    /* datatype */
    switch(type) {
      case TYPE_STRING:
	{ 
	    int rc, j;

	    for (j=0; j<items; j++) {
		rc = _dxfgetstringnodict(f);
		if (!rc) {
		    DXSetError(ERROR_DATA_INVALID,
		       "error reading text data; item %d, type string",  j+1);
		    goto error;
		}
	    }

        }
	break;

      case TYPE_UBYTE:
        GETITEMSX(ubyte, "unsigned byte")
	break;
 
      case TYPE_BYTE:
        GETITEMSX(byte, "signed byte")
	break;
 
      case TYPE_SHORT:
        GETITEMSX(short, "short");
        break;
	
      case TYPE_USHORT:
        GETITEMSX(ushort, "unsigned short");
        break;
	
      case TYPE_INT:
        GETITEMSX(int, "integer");
        break;
	
      case TYPE_UINT:
        GETITEMSX(uint, "unsigned integer");
        break;

      case TYPE_FLOAT:
        GETITEMSX(float, "float");
        break;
 
      case TYPE_DOUBLE:
        GETITEMSX(double, "double");
        break;

      default:
	DXSetError(ERROR_NOT_IMPLEMENTED, "unsupported data type");
	break;
 
    }

 error: 
    _dxfsetskipnum(f, 0);
    _dxfsetdictstringmode(f, 0);

    return rc;
}
 
 
 
/* xdr format
 */
Error _dxfreadarray_xdr(FILE *fd, Array a)
{
#if 1
    DXErrorReturn(ERROR_NOT_IMPLEMENTED, "no xdr support yet");
#else
    unsigned char cvalue, *cp;
    int *ip;
    float *fp;
 
    Object o;
    int i, done = 0;
    Error rc = OK; 
 
    Type type;
    Category cat;
    int items;
    int rank;
    int shape[MAXRANK];


    if(!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
	return ERROR;
 
    if(items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad item count");
 
    while(rank > 0) {
	items *= shape[rank-1];
	rank--;
    }
    if(items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad shape value");
 
    items *= DXCategorySize(cat);
    if (items <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "bad category value");
 
 
    /* switch on datatype */
    switch(type) {
      case TYPE_UBYTE:
	cp = (ubyte *)DXGetArrayData(a);

	for(i=0; i<items; i++, cp++) {
	    cvalue = 0;  /* call XDR routine here */  
            *cp = cvalue;
	}
	break;
 
      case TYPE_INT:
	ip = (int *)DXGetArrayData(a);
	
	for(i=0; i<items; i++, ip++) {
	    *ip = 0;   /* call XDR */
	}
	break;
	
      case TYPE_FLOAT:
	fp = (float *)DXGetArrayData(a);
 
	for(i=0; i<items; i++, fp++) {
	    *fp = 0.0;   /* call XDR */
	}
	break;
 
      default:
	DXErrorReturn(ERROR_NOT_IMPLEMENTED, 
		    "only byte, int or float data types");
 
    }
 
    return rc;

#endif

}
 
/* xdr format.  skip the right number of bytes.
 */
Error _dxfskiparray_xdr(FILE *fd, int items, int itemsize)
{
#if 1
    DXErrorReturn(ERROR_NOT_IMPLEMENTED, "no xdr support yet");
#else
    int rc;

    rc = fseek(fd->fd, (long)items * itemsize, 1);
    if (rc == 0)
	return OK;

 
    return rc;

#endif

}
 
 
/* read at another offset in this file
 */
Error _dxfreadoffset(struct finfo *f, Array a, int offset, int type)
{
    int rc = OK;
    int oldoffset;
    struct finfo nf;
    
    if (f->onepass) {
	DXSetError(ERROR_DATA_INVALID, 
		   "can't use forward offsets in this file in this mode");
	return ERROR;
    }
	
    _dxfdupfinfo(f, &nf);

    if ((oldoffset = ftell(f->fd)) < 0) {
	DXSetError(ERROR_INTERNAL, "ftell() returned bad value");
	return ERROR;
    }
    
    if (fseek(f->fd, offset + f->headerend, 0) != 0) {
	DXSetError(ERROR_DATA_INVALID, "can't seek to data offset %d", offset);
	return ERROR;
    }
    
    switch(type & D_FORMATS) {
      case D_TEXT:
      default:
	_dxfsetparse(&nf, (long)(offset + f->headerend), 0, 0);
	
	if (_dxfreadarray_text(&nf, a) != OK) {
	    DXAddMessage("array starting at data offset %d", offset);
	    rc = ERROR;
	    goto done;
	}
	break;

      case D_IEEE:
	if (_dxfreadarray_binary(&nf, a, type) != OK) {
	    DXAddMessage("array starting at data offset %d", offset); 
	    rc = ERROR;
	    goto done;
	}
	/* bad float warnings will be printed when this file is closed */
	break;
	
      case D_XDR:
	if (_dxfreadarray_xdr(f->fd, a) != OK) {
	    DXAddMessage("array starting at data offset %d", offset);
	    rc = ERROR;
	    goto done;
	}
	break;
    }

  done:
    if (fseek(f->fd, oldoffset, 0) != 0) {
	DXSetError(ERROR_INTERNAL, "fseek() returned error");
	rc = ERROR;
    }
 
    return rc;
}
 
 
/* read at an absolute byte offset in another file
 */
Error _dxfreadremote(struct finfo *f, Array a, int id, int type)
{
    int rc = OK;
    struct finfo nf;
    struct getby **gpp;
 
    /* getby struct is setup by file_lists() */
    gpp = _dxfgetobjgb(f, id);
    if (!gpp)
	return ERROR;

    /* this is going to screw up the dictionary and line counts unless
     *  this is a separate structure...  does it need a separate getby?
     */
    _dxfdupfinfo(f, &nf);

    nf.fd = _dxfopen_dxfile((*gpp)->fname, f->fname, &nf.fname,".dx");
    if (!nf.fd) {
	rc = ERROR;
	goto done;
    }
    
    if (fseek(nf.fd, (*gpp)->num, 0) != 0)  {
	DXSetError(ERROR_DATA_INVALID, "can't seek to offset %d in file '%s'",
		   (*gpp)->num, nf.fname);
	rc = ERROR;
	goto done;
    }

    switch(type & D_FORMATS) {
      case D_TEXT:
      default:
	rc = _dxfsetparse(&nf, (long)(*gpp)->num, 0, 0);
	if (!rc)
	    goto done;
	
	if(_dxfreadarray_text(&nf, a) != OK) {
	    DXAddMessage("data file '%s'", nf.fname);
	    rc = ERROR;
	    goto done;
	}
	break;

      case D_IEEE:
	if (_dxfreadarray_binary(&nf, a, type) != OK) {
	    DXAddMessage("data file '%s'", nf.fname);
	    rc = ERROR;
	    goto done;
	}

	/* have to do this here since we aren't going to get to the
	 *  code which normally prints this message out 
	 */
#if !defined(DXD_STANDARD_IEEE)
	/* bad binary floating point numbers */
	if (nf.denorm_fp > 0)
	    DXMessage("#1010", nf.denorm_fp, nf.fname);
	if (nf.bad_fp > 0)
	    DXMessage("#1020", nf.bad_fp, nf.fname);
#endif
	
	break;
	
      case D_XDR:
	if (_dxfreadarray_xdr(nf.fd, a) != OK) {
	    DXAddMessage("data file '%s'", nf.fname);
	    rc = ERROR;
	    goto done;
	}
	break;
    }
    
  done: 
    if (nf.fd)
	fclose(nf.fd);
    DXFree((Pointer)nf.fname);
    return rc;
    
}


/*** Everthing below here is byte order stuff and belongs in library ********/

/*
 * LSB   - bytes within doubles are ordered in memory 76543210
 * MSB   - bytes within doubles are ordered in memory 01234567
 */
ByteOrder
_dxfLocalByteOrder(void)
{
    union {
	long l;
	char c[4];
    } u;
    
    u.l = 0x00112233;
    if (u.c[0] == 0)
	return BO_MSB;
    else if (u.c[0] == 0x33)
	return BO_LSB;
    else
	return BO_UNKNOWN;
}


#define WORD_SWAP	1
#define HWORD_SWAP	2
#define BYTE_SWAP	4
/* 
 * DXSwap the half words (16 bits) within a word (32 bits) 
 * abcd -> cdab
 */
#define SWAP_HWORDS(A) 	(((A) >> 16) & 0x0000ffff) | (((A) << 16) & 0xffff0000)
/* 
 * DXSwap the each pair of bytes (8 bits) within a word (32 bits) 
 * abcd -> badc
 */
#define SWAP_BYTES(A) 	(((A) >>  8) & 0x00ff00ff) | (((A) <<  8) & 0xff00ff00)

/*
 * Byte swap n 64-bit quantities  according to swaps: 
 * If (swaps & WORD_SWAP)	swap words
 * If (swaps & HWORD_SWAP)	swap half words within words
 * If (swaps & BYTE_SWAP)	swap bytes within half words 
 *
 * NOTE: this works when src=dest.
 8
 * FIXME: This can be improved if there is a 64-bit int with double word loads.
 */
static Error
swap_64(void *src, void *dest, int n, int swaps) 
{
    unsigned int *s = (unsigned int*)src;
    unsigned int *d = (unsigned int*)dest;
    unsigned int x,y,i;
    
    if (((long)s & 3) || ((long)d & 3)) 
	DXErrorGoto(ERROR_INTERNAL,"Tried to swap non-word aligned data");
    
    switch (swaps) {
      case (WORD_SWAP|HWORD_SWAP|BYTE_SWAP):
	  for (i=0 ; i<n ; i++) {
	      x = *s; 
	      y = *(++s);  s++;
	      x = SWAP_HWORDS(x); x = SWAP_BYTES(x);	
	      y = SWAP_HWORDS(y); y = SWAP_BYTES(y);	
	      *d = y;
	      *(++d) = x; d++;
	  }
	  break;
	case (HWORD_SWAP|BYTE_SWAP):
	  n *= 2;	/* Do swaps a word at a time instead of double words */
	  for (i=0 ; i<n ; i++)   {
	      x = *(s++); 
	      x = SWAP_HWORDS(x); x = SWAP_BYTES(x);	
	      *(d++) = x;
	  }
	  break;
	case (BYTE_SWAP):
	  n *= 2;	/* Do swaps a word at a time instead of double words */
	  for (i=0 ; i<n ; i++)  {
	      x = *(s++); 
	      x = SWAP_BYTES(x);	
	      *(d++) = x;
	  }
	  break;
	default:
	  DXErrorGoto(ERROR_INTERNAL,"Unexpected byte swapping");
      }
    return OK;
  error:
    return ERROR;
}

/*
 * Given a pointer src to n number of items of type t, perform
 * byte_swap on the data.  That is change from msb (big-endian) 
 * to lsb (little-endian) order (or vice versa).  Swapping is done 
 * in place and it is ok if dest=src;
 *
 *  	MSB stores double words in 0x0011223344556677 order 
 *  	LSB stores double words in 0x7766554433221100 order 
 * 
 */

Error
_dxfByteSwap(void *dest, void *src, int ndata, Type t) 
{
    unsigned int swaps, doubles, x, tsize;
    
    tsize = DXTypeSize(t);		/* The number of bytes in an item */
    
    if (t == TYPE_UBYTE) {
	if (dest != src) 	/* DXCopy the buffers */
	    if (!memcpy(dest, src, tsize * ndata))
		DXErrorGoto(ERROR_INTERNAL,"Can not copy byte swap buffers");
	return OK;
    }
    
    ASSERT(sizeof(double) == 8);
    ASSERT(sizeof(int) == 4);
    ASSERT(sizeof(short) == 2);
    
    tsize *= 8;			/* The number of bits in an item */
    
    /* 
     * Determine which swaps we need to make, based on the size of the
     * item and assuming that we are swapping between msb and lsb.
     */
    swaps = 0;
    switch (tsize) {
      case 64: swaps |= WORD_SWAP;   /* Assuming msb <->lsb */
	/* Fall through */
      case 32: swaps |= HWORD_SWAP;  /* Assuming msb <->lsb */
	/* Fall through */
      case 16: swaps |= BYTE_SWAP;   /* Assuming msb <->lsb */
	break;
      case 8: return OK;             /* bytes are bytes are bytes */
      default:
	DXErrorGoto(ERROR_INTERNAL,"Don't know how to swap data type");
    }
    
    /* 
     * Set up and do the swapping. 
     */
    doubles = ndata;
    switch (tsize) {
      case 16:
	doubles /= 4;
	if ((ndata & 3) && (swaps & BYTE_SWAP)) {
	    /* Handle last odd shorts */
	    int shorts = doubles * 4;
	    unsigned short *s = (unsigned short*)src  + shorts;
	    unsigned short *d = (unsigned short*)dest + shorts;
	    shorts = ndata & 3;
	    while (shorts--) {
		x = *s;  s++;
		x = SWAP_BYTES(x); 
		*d = x;  d++;
	    }
	}
	break;
      case 32:
	doubles /= 2;
	if (ndata & 1)  {
	    /* Handle last odd word */
	    int words = doubles * 2;
	    unsigned int *s = (unsigned int*)src  + words;
	    unsigned int *d = (unsigned int*)dest + words;
	    x = *s; 
	    if (swaps & HWORD_SWAP)  x = SWAP_HWORDS(x); 
	    if (swaps & BYTE_SWAP)   x = SWAP_BYTES(x); 
	    *d = x;
	    
	}
	/* Fall through */
	break;
      case 64:
	break;
    }
    if (!swap_64(src,dest,doubles,swaps))
	goto error;
    
    return OK;
  error:
    return ERROR;
}
