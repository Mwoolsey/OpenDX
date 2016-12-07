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
#include <stdarg.h>
#include <stdlib.h>

#if defined(HAVE_SYS_ERRNO_H)
#include <sys/errno.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
 
#if ! DXD_HAS_UNIX_SYS_INCLUDES
typedef unsigned long mode_t;
#endif

#include "edf.h"

#include <stdlib.h>
 
#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
#define DX_DIR_SEPARATOR ';'
#else
#define DX_DIR_SEPARATOR ':'
#endif
/* prototypes
 */
 
static Error check_parms(struct getby *gp, char *fname, char **namelist, 
			 int *start, int *end, int *delta);
static Error is_dir(FILE *fp, char *name);

#if DXD_POPEN_OK && DXD_HAS_LIBIOP
#define popen popen_host
#define pclose pclose_host
#endif


/* messages about components and their consistency with each other
 */ 
#define Err_MustBeString      "#10700" 
#define Err_MustBeStringList  "#10701" 
#define Err_MissingComp       "#10702" 
#define Wrn_NotPosConnFac     "#10704" 
#define Err_NotArray          "#10706" 
#define Err_DiffCount         "#10708" 
#define Err_RefNotInt         "#10710" 
#define Err_BadElemType       "#10712" 
#define Err_OutOfRange        "#10714"


static char *ending(int num)
{
    switch(num % 10) {
      case 1:
	return (num%100)==11 ? "th" : "st";
	
      case 2:
	return (num%100)==12 ? "th" : "nd";

      case 3:
	return (num%100)==13 ? "th" : "rd";
    }

    return "th";
}

static Error elemtypecheck(int num, char *str)
{
    char rightstr[20];

    switch(num) {
      case 0:
	strcpy(rightstr, "points");
	break;
      case 1:
	strcpy(rightstr, "lines");
	break;
      case 2:
	strcpy(rightstr, "quads");
	break;
      case 3:
	strcpy(rightstr, "cubes");
	break;
      default:
	sprintf(rightstr, "cubes%dD", num);
	break;
    }
    
    if (strcmp(rightstr, str)) {
	DXSetError(ERROR_DATA_INVALID, Err_BadElemType, str, rightstr);
	return ERROR;
    }

    return OK;
}

Error 
_dxfValidate(Field f)
{
    Array current = NULL;
    Array target = NULL;
    char *tname, *cname;
    int ncurrent, ntarget, *ip;
    int index;
    unsigned char *cp;
    int i, j, lim, nitems;
    Type type, ref_type;
    Category cat;
    String s = NULL;
    Object o = NULL;
    int rank, shape[MAXRANK];
    int counts[MAXRANK];

    for (i=0; (current=(Array)DXGetEnumeratedComponentValue(f, i, &cname)); i++) {

	/* dep */
	if ((s = (String)DXGetAttribute((Object)current, "dep")) != NULL) {
	    /* make sure number of items matches number of items in dep */
	    if ((DXGetObjectClass((Object)s) != CLASS_STRING) ||
	        ((tname = DXGetString(s)) == NULL)) {
		DXSetError(ERROR_DATA_INVALID, 
			 Err_MustBeString, "dep", cname);
		return ERROR;
	    }
	    
	    if ((target = (Array)DXGetComponentValue(f, tname)) == NULL) {
		DXSetError(ERROR_DATA_INVALID, 
			 Err_MissingComp, tname, "dep", cname);
		return ERROR;
	    }
	    
	    if (DXGetObjectClass((Object)target) != CLASS_ARRAY) {
		DXSetError(ERROR_DATA_INVALID,
			 Err_NotArray, tname, "dep", cname);
		return ERROR;
	    }
	    
	    if (!DXGetArrayInfo(current, &nitems, &type, &cat, &rank, shape))
		return ERROR;
	    
	    ncurrent = nitems;

	    if (!DXGetArrayInfo(target, &nitems, &type, &cat, &rank, shape))
		return ERROR;
	    
	    ntarget = nitems;
	    
	    if (ncurrent != ntarget) {
		DXSetError(ERROR_DATA_INVALID,
			 Err_DiffCount, "dep",
			 ncurrent, cname, ntarget, tname);
		return ERROR;
	    }

	} /* end of if (has dep) */


	/* ref */
	if ((s = (String)DXGetAttribute((Object)current, "ref")) != NULL) {
	    if ((DXGetObjectClass((Object)s) != CLASS_STRING) ||
	        ((tname = DXGetString(s)) == NULL)) {
		DXSetError(ERROR_DATA_INVALID, 
			   Err_MustBeString, "ref", cname);
		return ERROR;
	    }
	    
	    if ((target = (Array)DXGetComponentValue(f, tname)) == NULL) {
		DXSetError(ERROR_DATA_INVALID,
			   Err_MissingComp, tname, "ref", cname);
		return ERROR;
	    }
	    
	    if (DXGetObjectClass((Object)target) != CLASS_ARRAY) {
		DXSetError(ERROR_DATA_INVALID, 
			   Err_NotArray, tname, "ref", cname);
		return ERROR;
	    }
	    
	    if (!DXGetArrayInfo(current, &nitems, &type, &cat, &rank, shape))
		return ERROR;
	    
	    if ( !( type == TYPE_INT || type == TYPE_UBYTE ) ) {
		DXSetError(ERROR_DATA_INVALID, Err_RefNotInt, cname);
		return ERROR;
	    }
	    ref_type = type;
	    
	    ncurrent = nitems;
	    for (j=0; j<rank; j++)
	        ncurrent *= shape[j];
	    
	    if (ncurrent > 0) {
                
		if (!DXGetArrayInfo(target, &nitems, &type, &cat, &rank, shape))
		    return ERROR;
		
		ntarget = nitems;
                
                /* only do this if they are already irregular */
                if (DXGetArrayClass(current) == CLASS_ARRAY) {
                    if ((ip = (int *)DXGetArrayData(current)) == NULL)
                        return ERROR;
		    cp = (unsigned char *) ip;
                    
		    /* neighbors can have -1's as indicies */
                    lim = strcmp(cname, "neighbors") ? 0 : -1;

		    for (j=0; j < ncurrent; j++) {
			if (ref_type == TYPE_INT) index = *(ip++);
			else                      index = *(cp++);

			if (index < lim || index >= ntarget) {
			    DXSetError(ERROR_DATA_INVALID, Err_OutOfRange,
				j+1, ending(j+1), cname, index, lim, ntarget-1);
			    return ERROR;
			}
		    }

                } else if (DXQueryGridConnections(current, &rank, counts)) {
		    for (j=0, ncurrent=1; j<rank; j++)
			ncurrent *= counts[j];
                    if (ncurrent > ntarget) {
                        DXSetError(ERROR_DATA_INVALID,
                                 Err_DiffCount, "ref",
                                 ncurrent, cname, ntarget, tname);
                        return ERROR;
                    }
                } else {
                    /* mesh array of mixed path and irregular arrays. */
                    /* have to handle the terms separately */
                }
	    }
	} /* end of if (has ref) */


        /* der - can be string lists here */
	if ((o = DXGetAttribute((Object)current, "der")) != NULL) {

	    if (DXExtractString(o, &tname)) {    /* simple string? */
		if ((target = (Array)DXGetComponentValue(f, tname)) == NULL) {
		    DXSetError(ERROR_DATA_INVALID,
			       Err_MissingComp, tname, "der", cname);
		    return ERROR;
		}
		
		if (DXGetObjectClass((Object)target) != CLASS_ARRAY) {
		    DXSetError(ERROR_DATA_INVALID,
			       Err_NotArray, tname, "der", cname);
		    return ERROR;
		}
	    } else if (DXExtractNthString(o, 0, &tname)) {  /* string list? */
		for (j=0; DXExtractNthString(o, j, &tname); j++) {
		    
		    if ((target = (Array)DXGetComponentValue(f, tname)) == NULL) {
			DXSetError(ERROR_DATA_INVALID,
				   Err_MissingComp, tname, "der", cname);
			return ERROR;
		    }
		    
		    if (DXGetObjectClass((Object)target) != CLASS_ARRAY) {
			DXSetError(ERROR_DATA_INVALID,
				   Err_NotArray, tname, "der", cname);
			return ERROR;
		    }
		}
	    } else {  /* neither string or string list */
		DXSetError(ERROR_DATA_INVALID, 
			   Err_MustBeStringList, "der", cname);
		return ERROR;
	    }
	    
	} /* end of if (has der) */

        /* element type */
	if ((s = (String)DXGetAttribute((Object)current, "element type")) != NULL) {
	    if ((DXGetObjectClass((Object)s) != CLASS_STRING) ||
	        ((tname = DXGetString(s)) == NULL)) {
		DXSetError(ERROR_DATA_INVALID, 
			 Err_MustBeString, "element type", cname);
		return ERROR;
	    }
	    
	    if (!strcmp(cname, "connections") && 
		DXQueryGridConnections(current, &rank, counts)) {

		if (!elemtypecheck(rank, tname))
		    return ERROR;
	    }

	} /* end of if (has element type) */

    }  /* for each component in the field */

    /* check for missing positions component if the field has more than
     *  one component.
     */
    if (i > 1 && !DXGetComponentValue(f, "positions"))
	DXWarning("importing a field with no `positions' component");

    return OK;
}

/* stolen from system.m (cpv's code)
 */
#if !defined(DXD_POPEN_OK)
static Error mkfifo(char *path, mode_t mode)
{
    int rc;
    char *cmd = (char *)DXAllocateLocal(strlen("mknod ") +
				      strlen(path) +
				      strlen(" p; chmod ") + 
				      12 + 
				      strlen(path) + 1);
    sprintf(cmd, "mknod %s p; chmod 0%o %s", path, mode, path);
    rc = system(cmd);

    DXFree((Pointer)cmd);
    return (rc==0) ? OK : ERROR;
}
static Error Unlink(char *path)
{
    int rc;
    char *cmd = (char *)DXAllocateLocal(strlen("rm -f") +
				      strlen(path) + 1);
    sprintf(cmd, "rm -f %s", path);
    rc = system(cmd);

    DXFree((Pointer)cmd);
    return (rc==0) ? OK : ERROR;
}
#endif



/*
 *
 * entry point for processing a file
 *
 */
 
Object DXImportDX(char *filename, char **namelist, 
		  int *start, int *end, int *delta)
{
    struct finfo f;
    Object o = NULL;
    char needfree = 1;
    int rc;
    
    /* setup finfo struct 
     */

    /* clear struct to be sure all pointers are NULL, and set stuff which
     *  we know the value of now.  then call the init subroutine to finish
     *  the rest of the initialization.
     */
    memset(&f, '\0', sizeof(f));
 
    f.fd = _dxfopen_dxfile(filename, NULL, &f.fname,".dx");
    if (!f.fd)
	goto done;
    
    rc = check_parms(&f.gb, NULL, namelist, start, end, delta);
    if (!rc)
	goto done;

    if (!f.fname) {
	f.fname = "<NULL>";
	needfree = 0;
    }

    f.recurse = 0;
    f.onepass = filename[0]=='!' ? 1 : 0;

    rc = _dxfinitfinfo(&f);
    if (!rc) {
	if (needfree)
	    DXFree((Pointer)f.fname);
	DXFree((Pointer)f.gb.gbuf);
	return NULL;
    }


    /* read the file and construct the requested object
     */
    rc = _dxfparse_file(&f, &o);
    if (!rc)
	goto done;

 
  done:
#if DXD_POPEN_OK
    _dxfclose_dxfile(f.fd, filename);   /* original filename possibly w/ ! */
#else
    _dxfclose_dxfile(f.fd, filename);    
#endif

    if (needfree)
	DXFree((Pointer)f.fname);

    DXFree((Pointer)f.gb.gbuf);

    _dxffreefinfo(&f);

    /* special routine which decrements ref count without deleting the object.
     *  the object was referenced when added to the dictionary, so that 
     *  cleanup could delete all things not being returned.
     */
    if (o)
	DXUnreference(o);

    return o;
}

 
/* special internal entry point.  input is a single open stream pointer,
 *  which can't be closed.  the caller has to ensure the file stream
 *  is a valid open file or pipe, and the default object will be imported
 *  and the format MUST be follows (can't seek on pipes, so can't look
 *  for forward references to data).
 */ 
Object _dxfImportDX_FD(FILE *fptr)
{
    struct finfo f;
    Object o = NULL;
    int rc;
    
    /* setup finfo struct 
     */

    /* clear struct to be sure all pointers are NULL, and set stuff which
     *  we know the value of now.   then call the init subroutine to finish
     *  the rest of the initialization.
     */
    memset(&f, '\0', sizeof(f));
 
    rc = check_parms(&f.gb, NULL, NULL, NULL, NULL, NULL);
    if (!rc)
	goto done;

    f.fd = fptr;
    f.fname = "<NULL>";
    f.recurse = 0;
    f.onepass = 1;

    rc = _dxfinitfinfo(&f);
    if (!rc) {
	DXFree((Pointer)f.gb.gbuf);
	return NULL;
    }


    /* read the file and construct the requested object
     */
    rc = _dxfparse_file(&f, &o);
    if (!rc)
	goto done;

  done:

    DXFree((Pointer)f.gb.gbuf);
    _dxffreefinfo(&f);

    /* special routine which decrements ref count without deleting the object.
     *  the object was referenced when added to the dictionary, so that 
     *  cleanup could delete all things not being returned.
     */
    if (o)
	DXUnreference(o);

    return o;
}

 
 
/* 
 * open a file, trying to append 'ext' and using each part of the DXDATA path
 *  if the environment variable is defined.  if auxname is defined, use
 *  the basename of that directory first before using the search path.
 */
FILE *_dxfopen_dxfile(char *inname, char *auxname, char **outname,char *ext)
{
    FILE *fd = NULL;
    char *datadir = NULL, *cp;
    char *basename = NULL;
    int bytes = 0;
    int rc;

    *outname = NULL;

    /* support for external filters
     */
    if (inname[0] == '!') {
	if (inname[1] == '\0') {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "external filter name");
	    return NULL;
	}
        
#if DXD_POPEN_OK
	
	if ((fd = popen(inname+1, "r")) == NULL) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10720", inname+1);
	    return NULL;
	}
	/* popen appears to succeed even if the program trying to
	 *  be run doesn't get started.  this is trying to test for
	 *  that case and give a more reasonable error message.
	 */
	if ((rc=fgetc(fd)) == EOF) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10722", inname+1);
	    return NULL;
	} else
	    ungetc(rc, fd);

	*outname = (char *)DXAllocateLocalZero(strlen(inname)+1);
	if (!*outname) {
	    pclose(fd);
	    return NULL;
	}

	strcpy(*outname, inname+1);

        return fd;

#else
#if os2
        DXSetError(ERROR_BAD_PARAMETER, "'!' is not supported by os2");
        return ERROR;
#else

        /* popen() is currently not supported on ibmpvs, so for now, use
         *  a named pipe as a substitute.
         */
#define TEMPLATE  "!/tmp/DXI%08d.0"

        bytes = strlen("(%s) > %s &") + 
                strlen(inname) + 1 +
                strlen(TEMPLATE) + 6;
        
	if ((cmd = (char *)DXAllocateLocalZero(bytes)) == NULL)
	    return NULL;
        
	if ((*outname = (char *)DXAllocateLocalZero(strlen(TEMPLATE) + 6)) == NULL)
	    return NULL;

        pid = getpid();
        sprintf(*outname, TEMPLATE, pid);
    
        if (mkfifo(*outname+1, 0600) == ERROR)
            goto npipe_error;

        sprintf(cmd, "(%s) > %s &", inname+1, *outname+1);

        rc = system(cmd);
        DXFree((Pointer) cmd);

        if (rc != 0)
            goto npipe_error2;

        /* open for reading */
	DXDebug("F", "trying at (0) to open pipe");
        fd = fopen(*outname+1, "r");
        if (fd == NULL) 
            goto npipe_error;
	

        return fd;

      npipe_error:
        DXSetError(ERROR_DATA_INVALID, "%s: %s", *outname+1, strerror(errno));
        Unlink(*outname+1);
        return ERROR;

      npipe_error2:
        DXSetError(ERROR_BAD_PARAMETER, "#10720", inname+1);
        Unlink(*outname+1);
        return ERROR;
#endif
#endif
    }


    /* try to open the filename as given, and save the name 
     *  for error messages later.
     */
    DXDebug("F", "trying at (1) to open `%s'", inname);
    fd = fopen(inname, "r");
    if (fd) {
	if (is_dir(fd, inname) != OK)
	    goto error;

	*outname = (char *)DXAllocateLocalZero(strlen(inname)+1);
	if (!*outname) 
	    goto error;
	
	strcpy(*outname, inname);
	return fd;
    }


#define XXTRA 12   /* space for the null, the / and .general - plus some extra */

    /* allocate space to construct variations of the given filename.
     *  get enough space the first time so we can construct any variation
     *  without having to reallocate.
     */
    bytes = strlen(inname) + XXTRA;
    if (auxname)
	bytes += strlen(auxname);

    datadir = (char *)getenv("DXDATA");
    if (datadir)
	bytes += strlen(datadir);

    *outname = (char *)DXAllocateLocalZero(bytes);
    if (!*outname)
	goto error;

    /* name as given, with extension appended to the end
     */
    strcpy(*outname, inname);
    strcat(*outname, ext);
    DXDebug("F", "trying at (2) to open `%s'", *outname);
    if ((fd=fopen(*outname, "r"))!=NULL) {
	if (is_dir(fd, *outname) != OK)
	    goto error;

	DXMessage("opening file '%s'", *outname);
	return fd;
    }

    
    /* if absolute pathname, it's not found.
     */
#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
    if (inname[0] == '/' || inname[0] == '\\' || inname[1] == ':') {
#else
    if (inname[0] == '/') {
#endif
	DXSetError(ERROR_BAD_PARAMETER, "#12240", inname);
	goto error;
    }
    
    
    /* on a recursive open, try the same directory as the parent file.
     */
    if (auxname != NULL) {
	
	/* find basename, keeping the last '/' */
#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
	basename = strrchr(auxname, '/');
	if(!basename)  
	    basename = strrchr(auxname, '\\');
	if(!basename)  
	    basename = strrchr(auxname, ':');
#else
	basename = strrchr(auxname, '/');
#endif
	if (basename) { 
	    
	    /* try basename + file */
	    strcpy(*outname, auxname);
	    (*outname)[basename-auxname+1] = '\0';
	    strcat(*outname, inname);
	    DXDebug("F", "trying at (3) to open `%s'", *outname);
	    if ((fd=fopen(*outname, "r"))!=NULL) {
		if (is_dir(fd, *outname) != OK)
		    goto error;
		
		DXMessage("opening file '%s'", *outname);
		return fd;
	    }
	    
	    /* basename + file + extension */
	    strcat(*outname, ext);
	    DXDebug("F", "trying at (4) to open `%s'", *outname);
	    if ((fd=fopen(*outname, "r"))!=NULL) {
		if (is_dir(fd, *outname) != OK)
		    goto error;
		
		DXMessage("opening file '%s'", *outname);
		return fd;
	    }
	}
    }
    
    
    /* if the DXDATA environment variable existed, try the list of
     *  directory names.
     */
    while (datadir) {
	
	strcpy(*outname, datadir);
	if ((cp = strchr(*outname, DX_DIR_SEPARATOR)) != NULL)
	    *cp = '\0';
	strcat(*outname, "/");
	strcat(*outname, inname);
	DXDebug("F", "trying at (5) to open `%s'", *outname);
	if ((fd=fopen(*outname, "r"))!=NULL) {
	    if (is_dir(fd, *outname) != OK)
		goto error;
	    
	    DXMessage("opening file '%s'", *outname);
	    return fd;
	}
	
	strcat(*outname, ext);
	DXDebug("F", "trying at (6) to open `%s'", *outname);
	if ((fd=fopen(*outname, "r"))!=NULL) {
	    if (is_dir(fd, *outname) != OK)
		goto error;
	    
	    DXMessage("opening file '%s'", *outname);
	    return fd;
	}
	
	datadir = strchr(datadir, DX_DIR_SEPARATOR);
	if (datadir)
	    datadir++;
    }
    
    /* if you get here, you didn't find the file.  fall thru
     *  into error section.
     */
    DXSetError(ERROR_BAD_PARAMETER, "#12240", inname);
    
  error:
    if (*outname) {
	DXFree((Pointer)*outname);
	*outname = NULL;
    }
    
    if (fd)
	fclose(fd);
    
    return NULL;
 
}

/* take care of pipes, if supported on this architecture
 */
Error _dxfclose_dxfile(FILE *fptr, char *filename) 
{
#if DXD_POPEN_OK
    if (fptr) {
		if (filename[0] == '!') {
	    	pclose(fptr);
		} else {
            fclose(fptr);
        }
    }
#else
    if (fptr) {
	    fclose(fptr);
	    if (filename[0] == '!')
	        Unlink(filename+1);
    }
    
#endif

    return OK;
}


/*
 * make sure the file pointer doesn't point to a directory.  on random
 *  occasions we're gotten segfaults (which don't reproduce later) when
 *  the file opened is a directory.
 */
static Error is_dir(FILE *fp, char *fname)
{
    struct stat sbuf;

    if (fstat(fileno(fp), &sbuf) < 0) {
        DXSetError(ERROR_BAD_PARAMETER, "%s: %s", fname, strerror(errno));
	return ERROR;
    }

    if (S_ISDIR(sbuf.st_mode)) {
	DXSetError(ERROR_BAD_PARAMETER, "%s is a directory", fname);
	return ERROR;
    }

    return OK;
}
 
/*
 *  rangecheck input parms, and fill the 'getby' struct.
 */
static Error check_parms(struct getby *gp, char *fname, char **namelist, 
		  int *start, int *end, int *delta)
{
    gp->which = GETBY_NONE;
    gp->numlist = NULL;
    gp->gbuf = NULL;

    gp->fname = fname;

    if (namelist && *namelist) {
	gp->which = GETBY_NAME;
	gp->namelist = namelist;
    }

    gp->seriesflag = 0;

    if (start) {
	if (*start < 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10030", "start");
	    return ERROR;
	}
	gp->seriesflag |= SL_START;
	gp->serieslim[0] = *start;
    }
    if (end) {
	if (*end < 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10030", "end");
	    return ERROR;
	}
	gp->seriesflag |= SL_END;
	gp->serieslim[1] = *end;
    }
    if (delta) {
	if (*delta <= 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10020", "delta");
	    return ERROR;
	}
	gp->seriesflag |= SL_DELTA;
	gp->serieslim[2] = *delta;
    }

    if (start && end) {
	if (*start > *end) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10185", "start", "end");
	    return ERROR;
	}
    }
   
    return OK;
}

/* initialize the finfo struct
 */ 
Error _dxfinitfinfo(struct finfo *fp)
{

    /* initialize the dictionary
     */
    fp->d = (struct dict *)DXAllocateLocal(sizeof(struct dict));
    if(!fp->d)
        goto done;

    if (!_dxfinitdict(fp->d))
        goto done;
 
    /* initialize the object list 
     */
    if (!_dxfinitobjlist(fp))
	goto done;
 

    /* initialize the parsing code
     */
    if (!_dxfinitparse(fp)) {
	DXSetError(ERROR_DATA_INVALID, "#10730", fp->fname);
	goto done;
    }


    return OK;


  done:
    _dxffreefinfo(fp);
    return ERROR;
}
 

void _dxffreefinfo(struct finfo *fp)
{
    _dxfdeleteparse(fp);

    _dxfdeletedict(fp->d);
    DXFree((Pointer)fp->d);

    _dxfdeleteobjlist(fp);
}

/* copy contents of finfo struct.
 *  source, destination.
 */
void _dxfdupfinfo(struct finfo *fp1, struct finfo *fp2)
{
    memcpy((void *)fp2, (void *)fp1, sizeof(struct finfo));
}
 

