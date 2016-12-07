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
#include <string.h>
#include "import.h"
#include "verify.h"

#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
#define DX_DIR_SEPARATOR ';'
#else
#define DX_DIR_SEPARATOR ':'
#endif


/* if this ifdef is enabled, then the filename parameter can be a list of
 *  files instead of just single filename.  you can't specify a variable name.
 *  it imports the default object from each file and adds it to a group and 
 *  returns the group at the end of import.  
 * one of the reasons this code isn't compiled in is that the original
 *  requestors asked that we allow empty files or invalid files to be skipped
 *  without ending the import.  if we do decide to enable this feature in the
 *  future, a bad file should set the error code and return.
 */
#define MULTIFILE 0
 

/* prototypes */
static Error  check_extension(char *fname, int *dt);
extern Object _dxfImportBin(char *dataset);   /* from libdx/rwobject.c */ 

 
/* 0 = filename         mdf:   name
 * 1 = fieldname               variable
 * 2 = format string           format
 * 3 = startframe              start
 * 4 = endframe                end
 * 5 = deltaframe              delta
 */
int
m_Import(Object *in, Object *out)
{
    char **fp = NULL, *format;
    char *ctmp, **ftmp;
    int i, nfields;
    int entrynum;
    int startframe, endframe, deltaframe;
    int match = 0;
#if MULTIFILE
    int multifile = 0;
#endif
    struct parmlist p;
    struct import_table *it;
    
    out[0] = NULL;
    entrynum = -1;
 
    /* check filename 
     */
    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "file name");
	return ERROR;
    }

    /* unless multifile is enabled, the filename is required to be
     *  a single a string.
     */
    if (!DXExtractString(in[0], &p.filename))
#if MULTIFILE
    {
	if (!DXExtractNthString(in[0], 0, &p.filename)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "file name");
	    return ERROR;
	} 
	else
	    multifile++;
    }
	    
#else
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "file name");
	return ERROR;
    }
#endif

 
    /* check format string. 
     */
    if (in[2]) {
	char *cp, *firstword;

	if(!DXExtractString(in[2], &format)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "format");
	    return ERROR;
	}

	/* stop parsing the format at the end of string or at the first blank
	 */
	firstword = format;
	if ((cp = strchr(format, ' ')) != NULL) {
	    firstword = (char *)DXAllocateLocal(cp-format+1);
	    if (!firstword)
		return ERROR;
	    strncpy(firstword, format, cp-format);
	    firstword[cp-format] = '\0';
	}
	
	for (it=_dxdImportTable, entrynum=0, match=0; 
	                                  it->readin; 
	                                  it++, entrynum++) {
	    ftmp = it->formatlist;
	    while(ftmp && *ftmp && !match) {
		if (!strncmp(firstword, *ftmp, strlen(*ftmp))) {
		    match++;
		    break;
		}
		ftmp++;
	    }
	    if (match)
		break;
	}
	
	if (!match) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10210", format, "file format");
	    return ERROR;
	}

	if (firstword != format)
	    DXFree((Pointer)firstword);

	p.format = format;
    } else
	p.format = NULL;
    
    
    /* fieldname(s) - build null terminated list
     */
    if(in[1]) {
	/* count the number of strings */
	nfields = 0;
	while (DXExtractNthString(in[1], nfields, &ctmp))
	    nfields++;
 
	if(nfields == 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "variable");
	    return ERROR;
	}
	if(!(fp = (char **)DXAllocate(sizeof(char *) * (nfields + 1))))
	    return ERROR;
	
	for(i = 0; i < nfields; i++)
	    if(!DXExtractNthString(in[1], i, &fp[i])) {
		DXSetError(ERROR_BAD_PARAMETER, "#10200", "variable");
		goto done;
	    }
 
	fp[i] = NULL;
    }
    p.fieldlist = fp;
 
 
    /* start, end and delta for series groups.  you need to know if the
     *  values were set, and what they were set to.  the defaults if the
     *  values aren't specified is the first series member, the last series
     *  member, and 1 respectively.
     */
    if(in[3]) {
	if(!DXExtractInteger(in[3], &startframe)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10010", "start");
	    goto done;
	}
	p.startframe = &startframe;
    } else
	p.startframe = NULL;
 
    if(in[4]) {
	if(!DXExtractInteger(in[4], &endframe)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10010", "end");
	    goto done;
	}
	p.endframe = &endframe;
    } else
	p.endframe = NULL;
 
    if(in[5]) {
	if(!DXExtractInteger(in[5], &deltaframe)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10010", "delta");
	    goto done;
	}
	p.deltaframe = &deltaframe;
    } else
	p.deltaframe = NULL;
 

    /* if format type was not specified, try to determine the type, first
     * by checking the file extension, and then trying the autodetermination
     * routines in the import table.  at the end, if there still isn't a 
     * match, the data type remains unknown.  (the autodetermination routines
     * usually work by opening the file and trying to read the first few
     * bytes to see if their signature or format is there).
     */
    if (!in[2] && !match) {

	/* is it fred.XXX, where xxx matches something we know?
         *  if not, try each of the autodetermination routines for a match.
         */
	if (!check_extension(p.filename, &entrynum)) {
	    for (it=_dxdImportTable, entrynum = 0, match = 0; 
		                                it->readin; 
		                                it++, entrynum++) {
		if (it->autotype != NULL && ((it->autotype)(&p) == IMPORT_STAT_FOUND)) {
		    match++;
		    break;
		}
	    }
	    if (!match)
		entrynum = -1;
	}
	
    }


    /* now call the actual import routine.  either set an error if the
     *  data type still isn't known, or default to using the 5 (dx)
     *  in the table.  for now, try the latter and see if it is helpful
     *  or just confusing.
     */
#if 0
    if (entrynum < 0) {
	DXSetError(ERROR_BAD_PARAMETER, 
		 "file not found or unrecognized file type");
	return NULL;
    }
#else
    if (entrynum < 0)
	entrynum = 5;
#endif

#if MULTIFILE
    if (entrynum == 3 && multifile) {
	Object o = NULL;
	
	multifile = 0;
	if (!(out[0] = (Object)DXNewGroup()))
	    goto done;

	while (DXExtractNthString(in[0], multifile, &p.filename)) {
	    o = (_dxdImportTable[entrynum].readin)(&p);
	    if (!o) {
		multifile++;
		DXPrintError("0::Import");
		DXResetError();
		continue;
	    }

	    if (!DXSetMember((Group)out[0], NULL, o)) {
		DXDelete(out[0]);
		out[0] = NULL;
		goto done;
	    }
	    multifile++;
	}

    } else
	out[0] = (_dxdImportTable[entrynum].readin)(&p);
#else
    /* import happens here */
    out[0] = (_dxdImportTable[entrynum].readin)(&p);

    /* call code from Verify() to look for loops - like a group member
     *  which contains a pointer to the parent group.  modules further
     *  down the line are likely to loop forever trying to recursivly
     *  traverse the object.
     */
    if (_dxfIsCircular(out[0]) == ERROR) {
	out[0] = NULL;
	goto done;
    }

#ifndef DO_ENDFIELD
    /*  if you don't call endfield on each field as you build it but call
     *   endobject on the entire object, there is code in endobject which
     *   detects which fields share positions and connections arrays, and
     *   builds the neighbors and bounding box only once and makes them
     *   shared as well.  for big irregular fields which are part of a long
     *   series, this is a big time and space win.
     */
    if (out[0])
	if (! DXEndObject(out[0])) {
	    DXDelete(out[0]);
	    out[0] = NULL;
	}
#endif

#endif

  done: 
    DXFree((Pointer)fp);
    if (out[0] == NULL)
    {
		if (DXGetError() == ERROR_NONE)
			DXSetError(ERROR_INTERNAL, "unspecified error in Import");

 		return ERROR;
    }
    else
		return OK;
}



static Error check_extension(char *fname, int *entrynum)
{
    char *cp, **ftmp;
    struct import_table *it;


    /* if no extension, return error */
    if ((cp = strrchr(fname, '.')) == NULL)
	return ERROR;
   
    if (*(++cp) == '\0')
	return ERROR;

    for (it = _dxdImportTable, *entrynum = 0; it->readin; it++, (*entrynum)++) {
	ftmp = it->formatlist;
	while(ftmp && *ftmp) {
	    if(!strcmp(cp, *ftmp))
		return OK;
	    ftmp++;
	}
    }
	
    return ERROR;
}    



/* built in importers */
 
ImportStatReturn 
_dxftry_ncdf(struct parmlist *p)
{
    char *tbuf;
    int frame;
    ImportStatReturn rc;

    /* if extension == .cdf or .ncdf, or */
    /* file or file.(n)cdf exists somewhere in curdir or DXDATA path, and */
    /* ncopen() works, then OK */
 
    if (_dxfstat_netcdf_file(p->filename) == IMPORT_STAT_FOUND)
	return IMPORT_STAT_FOUND;

    if (p->startframe == NULL && p->endframe == NULL && p->deltaframe == NULL)
	return IMPORT_STAT_NOT_FOUND;

    tbuf = (char *)DXAllocate(strlen(p->filename) + 20);
    if (!tbuf)
	return IMPORT_STAT_ERROR;

    frame = p->startframe ? *p->startframe : 1;
    sprintf(tbuf, "%s.%d", p->filename, frame);

    rc = _dxfstat_netcdf_file(tbuf);
    DXFree(tbuf);
    return rc;
}
 
Object _dxfget_ncdf(struct parmlist *p)
{
    int i, j, frno;
    int startframe, endframe, deltaframe;
    char dataset_name[512], numbuf[16];
    int overwrite = 0;
    ErrorCode firstrc = ERROR_NONE;
    char *firsterr = NULL;
    Series tsg = NULL;   /* time series group */
    Group grp = NULL;    /* generic group */
    Field f = NULL;
    Object o = NULL, retobj = NULL;
#if 0
    int incframe = 0;
#endif

 
    /* the start/end frame aren't specified.
     */
    if(!p->startframe && !p->endframe && !p->deltaframe) {
	retobj = DXImportNetCDF(p->filename, p->fieldlist, NULL, NULL, NULL);
	goto done;
    }
 

#if 0 
    /* workaround a bug in DXImportNetCDF - if startframe == endframe, it
     * doesn't import any frames.
     */
    if (p->endframe && p->startframe && (*p->startframe == *p->endframe)) {
	incframe = 1;
        (*p->endframe)++;
    }
#endif
    
    retobj = DXImportNetCDF(p->filename, p->fieldlist, p->startframe, 
			   p->endframe, p->deltaframe);
    if(retobj)
	goto done;
    
    
    /* if that failed, try the old way for backward compability.
     *  the old way required a field name.
     */
    if (!p->fieldlist || !p->fieldlist[0])
	goto done;

    firstrc = DXGetError();
    firsterr = (char *)DXAllocateLocal(strlen(DXGetErrorMessage()) + 1);
    if (!firsterr)
	goto cleanup;

    strcpy(firsterr, DXGetErrorMessage());
    DXResetError();

#if 0
    if (incframe)
        (*p->endframe)--;
#endif
   
    startframe = (p->startframe ? *p->startframe : 1);
    endframe = (p->endframe ? *p->endframe : startframe);
    deltaframe = (p->deltaframe ? *p->deltaframe : 1);
 
    /* for each import field... */
    for(j=0; p->fieldlist[j]; j++) {
	
	/* if more than one member, make a group and put the field you
         *  already have into it as the first member.
	 */
	if(j > 0 && !grp) {
	    grp = DXNewGroup();
	    if(!grp)
		goto cleanup;
	    
	    if(!DXSetMember(grp, p->fieldlist[0], 
			      		(tsg ? (Object)tsg : (Object)f)))
		goto cleanup;
	}
 
	/* only create a series group if there is more than one frame */
	if(endframe > startframe) {
	    tsg = DXNewSeries();  
	    if(!tsg)
		goto cleanup;
	}
	
	for(i=0, frno=startframe; frno<=endframe; frno+=deltaframe, i++) {
	    char *fieldptr[2];
	    
	    strcpy(dataset_name, p->filename);
	    sprintf(numbuf, ".%d", frno);
	    strcat(dataset_name, numbuf);
	    
	    fieldptr[0] = p->fieldlist[j];
	    fieldptr[1] = NULL;
 
	    o = DXImportNetCDF(dataset_name, fieldptr, NULL, NULL, NULL);
	    if(!o) {
		overwrite = 1;
		goto cleanup;
	    }
	    
	    if(tsg)    
		tsg = DXSetSeriesMember(tsg, i, (double)frno, o);
	}
	
	/* add to group if there already is one */
	if(grp && !DXSetMember(grp, p->fieldlist[j], 
					(tsg ? (Object)tsg : (Object)f)))
	    goto cleanup;
    }
    
    /* if there is a group, return that.  if there is a series, return
     *  that.  else it must be a simple field.
     */
    if(grp)
	retobj = (Object)grp;
    else if(tsg)
	retobj = (Object)tsg;
    else
	retobj = (Object)o;
    
    goto done;
 
  cleanup:
    DXDelete((Object)grp);
    DXDelete((Object)tsg);
    DXDelete((Object)f);
    DXDelete((Object)o);

    /* if you failed a second time, restore the first error message 
     */
    if (overwrite == 1)
	DXSetError(firstrc, firsterr);

    /* fall thru */

  done:
    return retobj;
}
 
 
ImportStatReturn
_dxftry_hdf(struct parmlist *p)
{
    char *tbuf;
    int frame;
    ImportStatReturn rc;

    /* if extension == .hdf, or 
     * file or file.hdf exists somewhere in curdir or DXDATA path, and
     * dfopen() works, then OK 
     */

    if (_dxfstat_hdf(p->filename) == IMPORT_STAT_FOUND)
	return IMPORT_STAT_FOUND;

    tbuf = (char *)DXAllocate(strlen(p->filename) + 20);
    if (!tbuf)
	return IMPORT_STAT_ERROR;

    frame = p->startframe ? *p->startframe : 1;
    sprintf(tbuf, "%s.%d", p->filename, frame);

    rc = _dxfstat_hdf(tbuf);
    DXFree(tbuf);
    return rc;
}
 
Object _dxfget_hdf(struct parmlist *p)
{
    int i, j, frno;
    int startframe, endframe, deltaframe;
    int setseries = 0, numflds=0, digit=0;
    char dataset_name[512], numbuf[16];
    char skip[16];
    Series tsg = NULL;   /* time series group */
    Group grp = NULL;    /* generic group */
    Field f = NULL;
    Object o = NULL, retobj = NULL;

#if 0  /* old code - try the first set in the file if no name given */ 
    /* HDF - no default field names because i don't know how to query
     *  the file for the fields in it.
     */
    
    /* field name is required */
    if(!p->fieldlist) {
	DXSetError(ERROR_BAD_PARAMETER, "variable required for HDF");
	goto done;
    }
#endif

    if (p->startframe || p->endframe || p->deltaframe)
	setseries++;

    startframe = (p->startframe ? *p->startframe : 1);
    endframe = (p->endframe ? *p->endframe : startframe);
    deltaframe = (p->deltaframe ? *p->deltaframe : 1);

    /* how many fields are there */
    if (p->fieldlist){
       for (i=0; p->fieldlist[i]; i++)
          ;
       numflds=i;
       /* is it a number */
       for (j=0; j<strlen(p->fieldlist[0]); j++){
          if (!isdigit((p->fieldlist[0])[j]))
	     break;
       }
       if (j==strlen(p->fieldlist[0])) digit=1;
       /* all members of fieldlist must be either number or not */
       for (i=0; i <numflds; i++){
	  for (j=0; j<strlen(p->fieldlist[i]); j++){
	     if (!isdigit((p->fieldlist[0])[j]))
	     break;
          }
	  if (j==strlen(p->fieldlist[0]) && !digit){
	     DXSetError(ERROR_BAD_PARAMETER,"variable list must be either number or name, not a mix");
	     return  NULL;
	  }
       }

    }
    else{
       numflds = _dxfget_hdfcount(p->filename);
       if (numflds==0)
          return NULL;
    }
    if (numflds>1) grp = DXNewGroup();
 
    /* for each input fieldname */
    for(j=0; j<numflds; j++) {
        if (p->fieldlist && digit)
	   strcpy(skip ,p->fieldlist[j]);
	else if (p->fieldlist){
	   i = _dxfwhich_hdf(p->filename,p->fieldlist[j]);
	   if (i<0)
	      return ERROR;
	   sprintf(skip,"%d",i);
	}
	else
	   sprintf(skip,"%d",j);
	
	/* only create a series group if there is more than one frame */
	if(endframe > startframe) {
	    tsg = DXNewSeries();  
	    if(!tsg)
		goto cleanup;
	} 
	
	/* for each frame of the series, import a field */
	for(i=0, frno=startframe; frno<=endframe; frno+=deltaframe, i++) {
	    
	    strcpy(dataset_name, p->filename);

	    if (setseries) {
		sprintf(numbuf, ".%d", frno);
		strcat(dataset_name, numbuf);
	    }
		
	    f = DXImportHDF(dataset_name, skip);
            if (!f)
              goto cleanup;
	     
	    if(tsg) {   
		tsg = DXSetSeriesMember(tsg, i, (double)frno, (Object)f);
		if(!tsg)
		    goto cleanup;
	    }
	    
	}
	
	/* if there is a group, add this as a member */
	if(grp && !DXSetMember(grp, skip, 
					(tsg ? (Object)tsg : (Object)f)))
	    goto cleanup;
	
    }

    /* if there is a group, return that.  if there is a series, return
     *  that.  else it must be a simple field.
     */
    if(grp)
	retobj = (Object)grp;
    else if(tsg)
	retobj = (Object)tsg;
    else
	retobj = (Object)f;
    
    goto done;
    
    
  cleanup:
    DXDelete((Object)grp);
    DXDelete((Object)tsg);
    DXDelete((Object)f);
    DXDelete((Object)o);
    /* fall thru */
    
  done:
    return retobj;
    
}
 
ImportStatReturn
_dxftry_bin(struct parmlist *p)
{
    char *cp;

    /* if extension == .bin, or arraydisk:filename */
    
#ifndef DXD_OS_NON_UNIX
    if (strchr(p->filename, ':') != NULL)
	return IMPORT_STAT_FOUND;
#endif

    if ((cp = strrchr(p->filename, '.')) != NULL) {
	if (!strcmp(".bin", cp))
	    return IMPORT_STAT_FOUND;
    }

    return IMPORT_STAT_NOT_FOUND;
}
 
Object _dxfget_bin(struct parmlist *p)
{
    return _dxfImportBin(p->filename);
}
 
ImportStatReturn
_dxftry_dxfile(char *inname);

ImportStatReturn
_dxftry_dx(struct parmlist *p)
{
    /* if extension == .dx, or
     * file or file.dx exists somewhere in curdir or DXDATA path, and
     * DXImportDX() works, then OK 
     */

    return _dxftry_dxfile(p->filename);
}
 

/* 
 * see if the filename exists, trying to append .dx and using each part 
 *  of the DXDATA path if the environment variable is defined.
 */
#include <sys/stat.h>

ImportStatReturn
_dxftry_dxfile(char *inname)
{
    ImportStatReturn rc = IMPORT_STAT_FOUND;
    struct stat sbuf;
    char *tryname = NULL;
    char *datadir = NULL, *cp;
 
    /* see if the file exists with the given name, and make sure it isn't
     *  a directory.  we've gotten random segfaults from trying to read a
     *  directory, even tho it should just look like garbage.
     */
    if ((stat(inname, &sbuf) >= 0) && (!S_ISDIR(sbuf.st_mode)))
	return IMPORT_STAT_FOUND;
    
	
#define XTRA 8   /* space for the null, the / and .dx - plus some extra */
    
    datadir = (char *)getenv("DXDATA");
    tryname = (char *)DXAllocateLocalZero((datadir ? strlen(datadir) : 0)
					  + strlen(inname) + XTRA);
    if (!tryname)
	return IMPORT_STAT_ERROR;
    
    strcpy(tryname, inname);
    strcat(tryname, ".dx");
    if ((stat(tryname, &sbuf) >= 0) && (!S_ISDIR(sbuf.st_mode)))
	goto done;
    
    while (datadir) {
	
	strcpy(tryname, datadir);
	if((cp = strchr(tryname, DX_DIR_SEPARATOR)) != NULL)
	    *cp = '\0';
	strcat(tryname, "/");
	strcat(tryname, inname);
	if ((stat(tryname, &sbuf) >= 0) && (!S_ISDIR(sbuf.st_mode)))
	    goto done;
	
	strcat(tryname, ".dx");
	if ((stat(tryname, &sbuf) >= 0) && (!S_ISDIR(sbuf.st_mode)))
	    goto done;

	datadir = strchr(datadir, DX_DIR_SEPARATOR);
	if (datadir)
	    datadir++;
    }
    rc = IMPORT_STAT_ERROR;
    
  done:
    DXFree (tryname);
    return rc;
}


Object _dxfget_dx(struct parmlist *p)
{
    /* Data Explorer external data format */
 
    return DXImportDX(p->filename, p->fieldlist, p->startframe,
		   p->endframe, p->deltaframe);
}

ImportStatReturn
_dxftry_cdf(struct parmlist *p)
{
    /* if extension == .cdf */
    /* file or file.cdf exists somewhere in curdir or DXDATA path, and */
    /* cdfopen() works, then OK */
    return _dxfstat_cdf(p->filename); 
}
 
Object _dxfget_cdf(struct parmlist *p)
{
    /* CDF importer */

    return DXImportCDF(p->filename,p->fieldlist,p->startframe,
		p->endframe,p->deltaframe);
}

Object _dxfget_cm(struct parmlist *p)
{
   return DXImportCM(p->filename,p->fieldlist);
}

ImportStatReturn 
_dxftry_wv(struct parmlist *p) 
{ 
    return IMPORT_STAT_NOT_FOUND; 
}

Object _dxfget_wv(struct parmlist *p) 
{ 
    DXSetError(ERROR_NOT_IMPLEMENTED, "winvis90 not supported");
    return NULL; 
}

