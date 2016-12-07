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
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
 
#include "edf.h"
 
 
/* prototypes
 */
static Error parse_object(struct finfo *f);
static Error parse_string(struct finfo *f);
static Error parse_series(struct finfo *f);
static Error parse_smember(struct finfo *f, int gmember);
static Error parse_group(struct finfo *f);
static Error parse_composite(struct finfo *f);
static Error parse_multigrid(struct finfo *f);
static Error make_group(struct finfo *f, Object o, char *thing);
static Error parse_member(struct finfo *f);
static Error parse_field(struct finfo *f);
static Error parse_component(struct finfo *f);
static Error parse_array(struct finfo *f, int kind);
static Error parse_sarray(struct finfo *f, int kind);
static Error parse_light(struct finfo *f);
static Error parse_camera(struct finfo *f);
static Error parse_xform(struct finfo *f);
static Error parse_screen(struct finfo *f);
static Error parse_clipped(struct finfo *f);
static Error parse_attribute(struct finfo *f, Object o);

static Error local_objectid(struct finfo *f, int *objnum, char *what);
static Error remote_objectid(struct finfo *f, int *objnum, char *what);
static Error parse_objectid(struct finfo *f, int *objnum, int *objtype, 
			    char *what);
static Error object_or_id(struct finfo *f, Object *o, char *what, int *which);
static Error seriesobject_or_id(struct finfo *f, Object *o, char *what, 
				int *which, int enumval, int *skip);
static Error remote_object(struct finfo *f, int value, Object *o);
static Error file_lists(struct finfo *f, int value);

static Error parse_include(struct finfo *f);
static Error parse_datadefs(struct finfo *f);
static Error parse_default(struct finfo *f);
static Object make_string(struct finfo *f);
static Error make_obj(struct finfo *f);

 
/* fill in the file info block, allocate space for input lines, and call
 *  the file parser on the input stream.  input parms are an open file
 *  stream pointer, a struct to specify how to select the return value and
 *  to specify the series limits if needed, the level of recursion,
 *  and the filename for error messages.
 */
Error _dxfparse_file(struct finfo *fp, Object *returnobj)
{
    int state, keywd;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "entering _dxfparse_file");
    DXDebug("F", "opening file '%s'", fp->fname);
#endif
    
    *returnobj = NULL;

    if (fp->recurse >= MAXNEST) {
	DXSetError(ERROR_DATA_INVALID, 
		   "can't nest included files more than %d deep", MAXNEST);
	return ERROR;
    }


    /*
     * start of parse section
     */

    /* first pass.  look for gross syntax errors, find out which objects
     *  match the ones requested and which objects they refer to, fill
     *  byte offset to input line table, and find the byte offset of 
     *  header 'end' keyword.
     */
    fp->activity = IDENTIFY_OBJS;
 
    rc = next_class(fp, &state);
    if (!rc) {
	if (state == ENDOFHEADER)
	    DXSetError(ERROR_DATA_INVALID, "file '%s' is empty", fp->fname);
	else if (state == NOTASCII)
	    DXSetError(ERROR_DATA_INVALID, 
		   "file '%s' contains binary data in header, not DX format", 
		       fp->fname);
	else
	    DXSetError(ERROR_DATA_INVALID, "file '%s' is not DX format", 
		       fp->fname);
	goto done;
    }

    if (fp->onepass) {
	fp->activity = MAKE_ALL_OBJS;
	goto singlepass;
    }

    while(1) {

	if (!rc)
	    goto geterror;
        
	rc = next_class(fp, &state);
	switch (state) {
	  case ENDOFHEADER:
	    goto secondpass;
	    
	  case NOTASCII:
	    DXSetError(ERROR_DATA_INVALID, 
		       "file not DX format; non-ascii data in header");
	    goto done;
	    
	  case KEYWORD:
	    rc = get_keyword(fp, &keywd);
	    switch (keywd) {
              case KW_OBJECT:
		/* this was skip_object */
		rc = parse_object(fp);
		break;
		
              case KW_DATA:
		rc = parse_datadefs(fp);
		break;
		
              case KW_DEFAULT:
		rc = parse_default(fp);
		break;
		
              case KW_ATTRIBUTE:
		rc = parse_attribute(fp, fp->curobj);
		break;
		
              case KW_END:
		rc = _dxfset_headerend(fp);
                goto secondpass;
		
              default:
                rc = ERROR;
                goto geterror;
	    }
	    break;
	    
          case LEXERROR:
	  default:
	    rc = ERROR;
	    goto geterror;
 
	}
	if (rc == ERROR)
	    goto geterror;

     }

    /* start of second pass.  actually build objects.
     */
  secondpass:
    rc = _dxfresetparse(fp);
    if (rc == ERROR)
	goto geterror;

    /* mark the parts used.
     */
    switch (fp->gb.which) {
      case GETBY_NAME:
	_dxfmarknamedobjlist(fp, fp->gb.namelist);
	break;
      case GETBY_NUMLIST:
	_dxfmarknumberedobjlist(fp, fp->gb.numlist, 0);
	break;
      case GETBY_ID:
	_dxfmarknumberedobjlist(fp, fp->gb.numlist, 1);
	break;
      case GETBY_NONE:
	_dxfmarknumberedobjlist(fp, fp->gb.numlist, 2); 
	break;
      default:
	DXSetError(ERROR_INTERNAL, "bad getby structure");
	break;
    }

    fp->activity = MAKE_USED_OBJS;

  singlepass:

    while (1) {

      geterror:
	if (!rc) {
	    if (DXGetError() == ERROR_NONE)
		DXSetError(ERROR_DATA_INVALID, "Error reading DX data format");

	    DXAddMessage("file '%s' line %d", 
			 fp->fname, _dxfgetprevline(fp));
	    
	    if (fp->t.class == LEXERROR)
		DXAddMessage(" %s", _dxfprtoken(&fp->t, fp->d));
	    
	    goto done;
	}
        
	rc = next_class(fp, &state);
	switch (state) {
	  case ENDOFHEADER:
	    goto done;
	    
	  case KEYWORD:
            rc = get_keyword(fp, &keywd);
	    if (rc == ERROR)
		goto geterror;

	    switch (keywd) {
              case KW_OBJECT:
		rc = parse_object(fp);
		break;
		
              case KW_DATA:
		rc = parse_datadefs(fp);
		break;
		
              case KW_DEFAULT:
		rc = parse_default(fp);
		break;
		
              case KW_ATTRIBUTE:
		rc = parse_attribute(fp, fp->curobj);
		break;
		
              case KW_END:
                goto done;
		
              default:
                rc = ERROR;
                goto geterror;
	    }
	    break;
	    
          case LEXERROR:
	  default:
	    rc = ERROR;
	    goto geterror;
 
	}
 
    }

    /* 
     * end of parse section
     */
 
 
    /* 
     * start of cleanup section 
     */
 
  done:
 
    /* if not error, build return object 
     */
    if (rc != ERROR) {
	switch (fp->gb.which) {
	  case GETBY_NAME:
            *returnobj = _dxfnamedobjlist(fp, fp->gb.namelist);
	    break;
	  case GETBY_NUMLIST:
            *returnobj = _dxfnumberedobjlist(fp, fp->gb.numlist, 0);
	    break;
	  case GETBY_ID:
            *returnobj = _dxfnumberedobjlist(fp, fp->gb.numlist, 1);
	    break;
	  case GETBY_NONE:
            *returnobj = _dxfnumberedobjlist(fp, fp->gb.numlist, 2); 
	    break;
	  default:
	    DXSetError(ERROR_INTERNAL, "bad getby structure");
	    break;
	}
 
#if !defined(DXD_STANDARD_IEEE)
	/* bad binary floating point numbers */
	if (fp->denorm_fp > 0)
	    DXMessage("#1010", fp->denorm_fp, fp->fname);
	if (fp->bad_fp > 0)
	    DXMessage("#1020", fp->bad_fp, fp->fname);
#endif

    } else
	*returnobj = NULL;
 
    return rc;
}

#define SKIP_TO_NEXT_OBJ    1
#define SKIP_TO_OBJ_OR_ATTR 2 

/*
 * syntax:  object [ number | name ] [ class ] type ...
 *  
 *  start definition of new object
 */
static Error parse_object(struct finfo *f)
{
    int kind;
    int skip = 0;
    int dictid;
    int objnum;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_object");
#endif
 
    /* save the input line number for later error messages */
    _dxfsetstartline(f);

    /* parse the object number, only locals allowed here */
    rc = local_objectid(f, &objnum, "new object");
    if (!rc || objnum < 0)
	return ERROR;
	
#if DEBUG_MSG
    switch (f->activity) {
      case IDENTIFY_OBJS:
	DXDebug("O", "identifying object %d", objnum);  break;
      case MAKE_USED_OBJS:
	DXDebug("O", "defining object %d if used", objnum);  break;
      case MAKE_ALL_OBJS:
        DXDebug("O", "defining object %d", objnum);  break;
      default:
	DXDebug("O", "bad activity flag value, object %d", objnum); break;
    }
#endif

    /* MAKE_ALL is set during single pass, and IDENTIFY is set for the
     *  first of two passes.  in either case, check for dups.  if this
     *  is the second pass (MAKE_USED), we've already done this.
     */
    if (f->activity != MAKE_USED_OBJS) {
	
	/* look for dup object id's - at this point the object can't already
	 *  be in the table.
	 */
	if (_dxflookobjlist(f, objnum, NULL, NULL) != ERROR)
	    DXErrorReturn(ERROR_DATA_INVALID, 
			  "duplicate object name or number");
	
	/* put id into object list so we can start adding references 
	 *  to it and/or actually create it.
	 * getdictalias is going to be SLOW and is this going to happen for 
	 *  every object we define.  this is bad.
	 */
	if (_dxfgetdictalias(f->d, objnum, &dictid))
	    rc = _dxfaddobjlist(f, objnum, NULL, _dxfdictname(f->d, dictid), 1);
	else
	    rc = _dxfaddobjlist(f, objnum, NULL, NULL, 1);
    }
    
    f->curid = objnum;
    
    rc = get_keyword(f, &kind);
    if (!rc)
	DXErrorReturn(ERROR_DATA_INVALID, "bad or missing object type");
    
    /* optional keyword - ignore and parse again */
    if (kind == KW_CLASS) {
	rc = get_keyword(f, &kind);
	if (!rc)
	    DXErrorReturn(ERROR_DATA_INVALID, "bad or missing object type");
	
    }

    /* on the second pass, if this object isn't used, skip it.
     *  arrays and constanarrays can't be skipped, because they
     *  may contain binary data.
     * everything else can be skipped until the start of the next
     *  object definition.
     */
    if (f->activity == MAKE_USED_OBJS && !_dxfisobjused(f, objnum)) {
	if (kind != KW_ARRAY && kind != KW_CONSTANTARRAY) {
	    skip = SKIP_TO_NEXT_OBJ;
	    goto done;
	}
    }
    
    /* we get here if we are either identifying objects which use
     *  other objects, or if we are really making objects.
     *  switch on object type and parse rest of object.
     */
    switch (kind) {
 
      /* these have members, and should set current object */
      case KW_GROUP:
        rc = parse_group(f);
        break;
      case KW_SERIES:
        rc = parse_series(f);
        break;
      case KW_COMPOSITE:
        rc = parse_composite(f);
        break;
      case KW_MULTIGRID:
        rc = parse_multigrid(f);
        break;
 
      /* this has components, and should set current object */
      case KW_FIELD:
        rc = parse_field(f);
        break;
 
      /* these have parms and data lists, including binary data follows */
      case KW_ARRAY:
      case KW_CONSTANTARRAY:
        rc = parse_array(f, kind);
	/* skip any trailing attributes if you aren't making the object */
	if (f->activity == MAKE_USED_OBJS && !_dxfisobjused(f, objnum))
	    skip = SKIP_TO_NEXT_OBJ;
        break;

      /* these have terms which can be other objects */
      case KW_PRODUCTARRAY:
      case KW_MESHARRAY:
        rc = parse_sarray(f, kind);
        break;

      /* these just have parms which can't be other objects */
      case KW_REGULARARRAY:
      case KW_PATHARRAY:
      case KW_GRIDPOSITIONS:
      case KW_GRIDCONNECTIONS:
	if (f->activity == IDENTIFY_OBJS) {
	    skip = SKIP_TO_OBJ_OR_ATTR;
	    break;
	}
        rc = parse_sarray(f, kind);
        break;

      /* these just have parms which can't be other objects */
      case KW_STRING:
	if (f->activity == IDENTIFY_OBJS) {
	    skip = SKIP_TO_OBJ_OR_ATTR;
	    break;
	}
        rc = parse_string(f);
        break;
      case KW_LIGHT:
	if (f->activity == IDENTIFY_OBJS) {
	    skip = SKIP_TO_OBJ_OR_ATTR;
	    break;
	}
        rc = parse_light(f);
        break;
      case KW_CAMERA:
	if (f->activity == IDENTIFY_OBJS) {
	    skip = SKIP_TO_OBJ_OR_ATTR;
	    break;
	}
        rc = parse_camera(f);
        break;
 
      /* these have subobjects */
      case KW_TRANSFORM:
	rc = parse_xform(f);
	break;
      case KW_CLIPPED:
	rc = parse_clipped(f);
	break;
      case KW_SCREEN:
	rc = parse_screen(f);
	break;
 
      /* this reads an object from another file */
      case KW_FILE:
	if (f->activity == IDENTIFY_OBJS) {
	    skip = SKIP_TO_OBJ_OR_ATTR;
	    break;
	}
	rc = parse_include(f);
	break;

#if 0
      /* you can't have this at this level!  you would get here if your
       *  file was:   object 13 attribute ...
       */
      /* this applies to the last object defined */
      case KW_ATTRIBUTE:
        rc = parse_attribute(f, f->curobj);
        break;
#endif
 
      default:
        DXErrorReturn(ERROR_DATA_INVALID, "unrecognized object type");
    }
    
  done:
    
    /* skip to start of next object definition, or next attribute
     *  since the target of an attribute can be another object.
     */
    if (skip) {
	if (skip == SKIP_TO_NEXT_OBJ)
	    rc = _dxfskip_object(f);
	else
	    rc = _dxfskip_object_or_attr(f);
    }
    
#if DEBUG_MSG
    if (skip)
	DXDebug("O", "skip of rest of object %d complete", objnum);
    else
	DXDebug("O", "if used, creation of object %d complete", objnum);
#endif
    return rc;
}


/*
 * syntax:  file [ filename | filename,numlist | filename,namelist ]
 *  
 *  the contents of that file are read in and that object is returned.
 */
static Error parse_include(struct finfo *f)
{
    int rc;
    int remoteid;
    Object o;
 
#if DEBUG_MSG
    DXDebug("EP", "in parse_include");
#endif
 
    /* import the remote object, add it to the dictionary and return 
     *  the id.
     */
    rc = remote_objectid(f, &remoteid, "include object");
    
    if (!rc) {
	DXSetError(ERROR_DATA_INVALID, "bad or missing file definition");
	return ERROR;
    }

    rc = remote_object(f, remoteid, &o);
    if (!rc) {
	DXSetError(ERROR_DATA_INVALID, "bad or missing file object");
	return ERROR;
    }
    
    /* now assign it to the local id it should have.
     *  is this going to be a problem?
     */
    if (_dxflookobjlist(f, remoteid, &o, NULL) == ERROR)
	return ERROR;
    
    if (!_dxfsetobjptr(f, f->curid, o))
	return ERROR;
    
    return OK;
}
 
/*
 * syntax:  string "contents" [ "more_contents" ... ]
 *  
 *  a string or stringlist object is created.
 */
static Error parse_string(struct finfo *f)
{
    Object o;
    Error rc = OK;
    
#if DEBUG_MSG
    DXDebug("E", "in parse_string");
#endif
    
    o = make_string(f);
    
    if (!o)
	return ERROR;
    
    if (!_dxfsetobjptr(f, f->curid, o)) {
	DXDelete(o);
	return ERROR;
    }
    
    return rc;
}

/* syntax: member [ number ] [ [ position ] number ] [ value ] seriesmember
 *
 */
static Error parse_series(struct finfo *f)
{ 
    Object o;
    Error rc = OK;
    int id;
    int member = 0;
    int done = 0;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_series");
#endif
 
    if (make_obj(f)) {
	o = (Object)DXNewSeries();
	if (!o)
	    return ERROR;
	
	if (!_dxfsetobjptr(f, f->curid, o)) {
	    DXDelete(o);
	    return ERROR;
	}

	f->curobj = o;
    }
    
    
    /* now until the next keyword isn't member or attribute, process.
     */
    while (!done) {
	rc = next_class(f, &id);
        if (!rc || (id != KEYWORD))
            break;
 
	next_id(f, &id);
        switch (id) {
          case KW_MEMBER:
            rc = skipkeyword(f);
            if (!rc)
                DXErrorReturn(ERROR_DATA_INVALID, 
                            "bad or missing series member");
 
            rc = parse_smember(f, member);
	    member++;
            break;
 
          case KW_ATTRIBUTE:
            rc = skipkeyword(f);
            if (!rc)
                DXErrorReturn(ERROR_DATA_INVALID, "bad or missing attribute");
 
            rc = parse_attribute(f, f->curobj);
            break;
 
          default:
            done++;
            break;
        }
 
	if (!rc)
	    done++;
    }
 
    return rc; 
}


 
/* copy the array contents */
extern Array _dxfReallyCopyArray(Array a);

static Error parse_smember(struct finfo *f, int gmember) 
{
    int num, enumval;
    int state = 0;
    int skip = 0;
    int done = 0;
    int which, id;
    Object o = NULL;
    Object newo = NULL;
    float pos;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_smember");
#endif

    /* next available member number.  this is the default series member
     *  number unless it is specified.
     */
    if (make_obj(f))
	if (!DXGetMemberCount((Group)f->curobj, &num))
	    return ERROR;
    
    /* series position defaults to series member unless explicitly specified,
     * member number defaults to next available unless explicitly specified.
     */
    enumval = gmember;
    pos = (float)gmember;

    /* default is to add the given object as an unnamed enumerated member
     *  with the same seriesposition as the enum member number.  an explicit
     *  enumeration and seriesposition can be specified.
     *  (but enumerations must be in order - no skipped members allowed.)
     *  the POSITION and VALUE keywords are in many cases optional, as this
     *  tries to identify from the context what is being specified.
     *
     * states:  0 = seen nothing
     *          1 = seen enumeration number
     *          2 = seen 'position' keyword
     *          3 = seen seriesposition value
     *          4 = seen 'value' keyword
     *          5 = seen member object id
     */
    while (!done) {
	rc = next_class(f, &id);
	switch (id) {
	  case KEYWORD:
	    next_id(f, &id);
	    switch (id) {
	      case KW_POSITION:
		skipkeyword(f);
		rc = _dxfmatchfloat(f, &pos);
		if (!rc)
		    DXErrorReturn(ERROR_DATA_INVALID, "bad series position");
		
		state = 3;
		break;
		
	      case KW_VALUE:
		skipkeyword(f);
		if (state >= 4)
		    DXErrorReturn(ERROR_DATA_INVALID, 
				  "unexpected keyword 'value'");
		state = 4;
		break;

	      case KW_FILE:
		rc = seriesobject_or_id(f, &o, "series member", &which,
					enumval, &skip);
		if (!rc)
		    return ERROR;

		state = 5;
		break;

	      default:
		done++;
		break;

	    }
	    break;
	    
	  case STRING: 
	    rc = seriesobject_or_id(f, &o, "series member", &which,
				    enumval, &skip);
	    if (!rc)
		return ERROR;
	    
	    state = 5;
	    break;

	  case NUMBER:
	    switch (state) {
	      case 0:
		rc = _dxfmatchint(f, &enumval);
		if (!rc)
		    DXErrorReturn(ERROR_DATA_INVALID, 
				"bad series member number");
		if (enumval != gmember)
		    DXErrorReturn(ERROR_DATA_INVALID, 
	 "if specified, series member numbers must be 0-based and contiguous");
		state = 1;
		break;

	      case 1: case 2:
		rc = _dxfmatchfloat(f, &pos);
		if (!rc)
		    DXErrorReturn(ERROR_DATA_INVALID, "bad series position");

		state = 3;
		break;

	      case 3: case 4:
		rc = seriesobject_or_id(f, &o, "series member", &which,
					enumval, &skip);
		if (!rc)
		    return ERROR;
		
		state = 5;
		break;

	      default:
		done++;
		break;
	

	    } /* switch(state) */
	    break;

	  default:
	    done++;
	    break;

	} /* switch(class) */

	if (!rc)
	    return rc;

    }  /* while (!done) */

#if DEBUG_MSG
    DXDebug("O", 
	    "setting series member %g of group 0x%08x to object 0x%08x",
	    pos, f->curobj, o);
#endif
    
    if (skip || (!make_obj(f)))
	return OK;
		

    newo = DXCopy(o, COPY_STRUCTURE);
    if ((newo == o) && (DXGetObjectClass(o) == CLASS_ARRAY))
	newo = (Object)_dxfReallyCopyArray((Array)o);

    /* DXSetSeriesMember adds a 'series position' attribute to the
     * object.  arrays don't copy, so if this is a series of arrays,
     * the one shared array will keep getting its series position
     * attribute overwritten unless you really copy the array.
     */

    /* check if enumval > num+1? here */
    if (!newo || !DXSetSeriesMember((Series)f->curobj, num, pos, newo))
	return ERROR;
 
    return rc; 
}

 
static Error parse_group(struct finfo *f)
{
    Object o = NULL;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_group");
#endif
 
    if (make_obj(f)) {
	o = (Object)DXNewGroup();
	if(!o)
	    return ERROR;
    }

    return make_group(f, o, "group");
}
 
 
static Error parse_composite(struct finfo *f)
{ 
    Object o = NULL;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_composite");
#endif
 
    if (make_obj(f)) {
	o = (Object)DXNewCompositeField();
	if(!o)
	    return ERROR;
    }

    return make_group(f, o, "composite field");
}
 
static Error parse_multigrid(struct finfo *f)
{ 
    Object o = NULL;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_multigrid");
#endif
 
    if (make_obj(f)) {
	o = (Object)DXNewMultiGrid();
	if(!o)
	    return ERROR;
    }

    return make_group(f, o, "multigrid group");
}
 
static Error make_group(struct finfo *f, Object o, char *thing)
{
    Error rc = OK;
    int n, keyword;
    int done = 0;
 
#if DEBUG_MSG
    DXDebug("E", "in make_group");
#endif

    if (make_obj(f)) {
	if (!_dxfsetobjptr(f, f->curid, o)) {
	    DXDelete(o);
	    return ERROR;
	}
	
	f->curobj = o;
    }
 
    /* now until the next keyword isn't member or attribute, process
     */
    while (!done) {
        if (!next_class(f, &n))
            break;
	
	if (n != KEYWORD)
	    break;

	if (!next_id(f, &keyword))
	    break;

	if ((keyword != KW_MEMBER) && (keyword != KW_ATTRIBUTE))
	    break;
	
        switch (keyword) {
          case KW_MEMBER:
            rc = skipkeyword(f);
            if(!rc) {
                DXSetError(ERROR_DATA_INVALID, "bad or missing %s member", thing);
		return ERROR;
	    }
 
            rc = parse_member(f);
            break;
 
          case KW_ATTRIBUTE:
            rc = skipkeyword(f);
            if(!rc)
                DXErrorReturn(ERROR_DATA_INVALID, "bad or missing attribute");
 
            rc = parse_attribute(f, f->curobj);
            break;
 
          default:
            done++;
            break;
        }
	
	if(!rc)
	    done++;
 
    }
 
    return rc; 
}

 
static Error parse_member(struct finfo *f) 
{
    int name, enumval;
    int byname;
    int which;
    Object o;
    char *cp = NULL;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_member");
#endif
 
    /* valid things here are: "name", enumerated_number, or VALUE kw.
     */
    if (match_keyword(f, KW_VALUE)) {
	/* if VALUE first, no name or number */
	cp = NULL;
	byname = 1;
	goto seenvalue;
    }
 
    rc = get_string(f, &name);
    if (rc) {
	cp = _dxfdictname(f->d, name);
	byname = 1;
    } else {
	rc = _dxfmatchint(f, &enumval);
	if (rc)
	    byname = 0;
        else
	    DXErrorReturn(ERROR_DATA_INVALID, "bad member clause");
    }
    
    /* skip optional keyword VALUE */
    match_keyword(f, KW_VALUE);
    
  seenvalue:
 
    rc = object_or_id(f, &o, "member", &which);
    if (!rc)
	return ERROR;
 
    if (which == IDENTIFY_OBJS)
	return rc;

#if DEBUG_MSG
    DXDebug("O", "setting member %s of group 0x%08x to object 0x%08x",
	        cp,  f->curobj, o);
#endif
 
    /* this comment seems to be out of date, since ocp doesn't seem to be
     *  used anymore, but the idea is still a good one - if the member has
     *  a name, it seems we should use it when setting the group member.
     *
     * if cp == NULL and ocp != NULL, should this set by ocp?  
     *  (cp == NULL implies the clause was "member value <object>" so there
     *   explicitly was no object name, but ocp != NULL implies that the
     *   object itself was named, and we could use that name for the member).
     */
    if (byname) {
	if (!DXSetMember((Group)f->curobj, cp, o))
	    return ERROR;
    } else
	if (!DXSetEnumeratedMember((Group)f->curobj, enumval, o))
	    return ERROR;
    
    return rc; 
}

 

static Error parse_field(struct finfo *f)
{
    Object o = NULL;
    Error rc = OK;
    int done = 0;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_field");
#endif
 
    if (make_obj(f)) {
	o = (Object)DXNewField();
	if(!o)
	    return ERROR;
 
	if (!_dxfsetobjptr(f, f->curid, o)) {
	    DXDelete(o);
	    return ERROR;
	}
 
	f->curobj = o;
    }
 
    /* now until the next keyword isn't component or attribute, process
     */
    while(!done) {
        if(f->t.class != KEYWORD)
            break;
 
        switch(f->t.token.id) {
          case KW_COMPONENT:
            skipkeyword(f);
            rc = parse_component(f);
            break;
 
          case KW_ATTRIBUTE:
            skipkeyword(f);
            rc = parse_attribute(f, f->curobj);
            break;
 
          default:
            done++;
            break;
        }
 
	if(rc == ERROR)
	    return ERROR;
    }

    if (make_obj(f)) {

	/* private check for ranges, etc */
	if (!_dxfValidate((Field)o))
	    return ERROR;
	
#ifdef DO_ENDFIELD
	if (!DXEndField((Field)o))
	    return ERROR;
#endif

    }

    return rc; 
}


static Error parse_component(struct finfo *f)
{ 
    int name, which;
    Object o;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_component");
#endif
 
    rc = get_string(f, &name);
    if(!rc)
	DXErrorReturn(ERROR_DATA_INVALID, "bad or missing component name");
 
    /* skip VALUE if present */
    match_keyword(f, KW_VALUE);

    rc = object_or_id(f, &o, "component", &which);
    if (!rc)
	return ERROR;
    
#if DEBUG_MSG
    if (which != IDENTIFY_OBJS)
	DXDebug("O", "setting component %s of field 0x%08x to object 0x%08x",
		_dxfdictname(f->d, name), f->curobj, o);
    else
	DXDebug("O", "marked next object as member of field");
#endif

    if (which == IDENTIFY_OBJS)
	return rc;

    if (!DXSetComponentValue((Field)f->curobj, _dxfdictname(f->d, name), o))
	return ERROR;
 
    return rc; 
}
 

 
#define SIGN_UNSET 0
#define SIGN_YES   1
#define SIGN_NO    2

/* normal arrays and constant arrays */
static Error parse_array(struct finfo *f, int kind)
{
    int value;
    Object o = NULL;
    Object tmp = NULL, tmp2 = NULL;
    int i, id, done = 0;
    int conitems=0;
    int makeit;
    int issigned = SIGN_UNSET;
    int signok = 0;
    int rankset = 0;
    int shapeset = 0;
    Error rc = OK;

    int datatype = f->dformat;
 
    Type type = TYPE_FLOAT;		/* defaults for array parms */
    Category cat = CATEGORY_REAL;
    int items = -1;
    int rank = 0;
    int *shape = NULL;
 
 
#if DEBUG_MSG
    DXDebug("E", "in parse_array");
#endif
 
    /* initialize this to 0's */
    shape = (int *)DXAllocateLocalZero(sizeof(int) * MAXRANK);

    /* are we making or skipping this object?
     */
    makeit = make_obj(f);

    while (!done) {
	rc = next_class(f, &id);
	if (!rc || (id != KEYWORD))
	    break;

	next_id(f, &id);
        switch (id) {
          case KW_TYPE:
            rc = skipkeyword(f);
 
          moretype:
            rc = get_keyword(f, &value);
            if (!rc) {
                DXSetError(ERROR_DATA_INVALID, "bad or missing array type");
		goto error;
	    }
            
            switch (value) {
              case KW_SIGNED:
                issigned = SIGN_YES;
                goto moretype;
              case KW_UNSIGNED:
                issigned = SIGN_NO;
                goto moretype;
              case KW_CHAR:
                signok = 1;
		switch (issigned) {
		  case SIGN_YES:
		    type = TYPE_BYTE;
		    break;
		  case SIGN_NO:
		  case SIGN_UNSET:
		    type = TYPE_UBYTE;
		    break;
		}
                break;
              case KW_SHORT:
                signok = 1;
		switch (issigned) {
		  case SIGN_YES:
		  case SIGN_UNSET:
		    type = TYPE_SHORT;
		    break;
		  case SIGN_NO:
		    type = TYPE_USHORT;
		    break;
		}
                break;
              case KW_INTEGER:
                signok = 1;
		switch (issigned) {
		  case SIGN_YES:
		  case SIGN_UNSET:
		    type = TYPE_INT;
		    break;
		  case SIGN_NO:
		    type = TYPE_UINT;
		    break;
		}
                break;
              case KW_HYPER:
                signok = 1;
		switch (issigned) {
		  case SIGN_YES:
		  case SIGN_UNSET:
		    type = TYPE_HYPER;
		    break;
		  case SIGN_NO:
		    DXSetError(ERROR_NOT_IMPLEMENTED, 
			       "unsigned hyper not supported yet");
		    goto error;
		}
                break;
              case KW_FLOAT:
                type = TYPE_FLOAT;
                break;
              case KW_DOUBLE:
                type = TYPE_DOUBLE;
                break;
              case KW_STRING:
                type = TYPE_STRING;
                break;
              default:
                DXSetError(ERROR_DATA_INVALID, "bad array type");
		goto error;
            }
            if ((issigned != SIGN_UNSET) && !signok) {
                DXSetError(ERROR_DATA_INVALID, 
                           "signed or unsigned not valid for this array type");
                goto error;
            }
            break;
            
          case KW_CATEGORY:
            rc = skipkeyword(f);
 
            rc = get_keyword(f, &value);
            if (!rc) {
                DXSetError(ERROR_DATA_INVALID, "bad or missing array category");
		goto error;
	    }
            
            switch (value) {
              case KW_REAL:
                cat = CATEGORY_REAL;
                break;
              case KW_COMPLEX:
                cat = CATEGORY_COMPLEX;
                break;
              case KW_QUATERNION:
                cat = CATEGORY_QUATERNION;
                break;
              default:
                DXSetError(ERROR_DATA_INVALID, "bad array category");
		goto error;
            }
            break;
            
          case KW_COUNT:
            rc = skipkeyword(f);
 
            rc = _dxfmatchint(f, &value);
            if (!rc || value < 0) {
                DXSetError(ERROR_DATA_INVALID, 
			   "bad or missing array item count");
		goto error;
	    }
            
            items = value;
            break;
            
          case KW_RANK:
            rc = skipkeyword(f);
 
            rc = _dxfmatchint(f, &value);
            if (!rc || value < 0 || value >= MAXRANK) {
		if (!rc || value < 0)
		    DXSetError(ERROR_DATA_INVALID, 
			       "bad or missing array rank");
		else
		    DXSetError(ERROR_DATA_INVALID, 
			       "array rank cannot be larger than %d", MAXRANK);
		goto error;
	    }
	    
	    if (rankset && rank != value) {
		DXSetError(ERROR_DATA_INVALID, "rank doesn't match shape");
		goto error;
	    }
 
            rank = value;
	    rankset++;
 
            break;
            
          case KW_SHAPE:
            rc = skipkeyword(f);
 
	    /* if they've already set the rank, number of shape parms must
	     *  match.  otherwise, set rank based on number of integers
	     *  following the shape keyword.
	     */
	    if (rankset) {
		if (rank == 0) {
		    DXSetError(ERROR_DATA_INVALID, 
			     "rank 0 data is scalar; it cannot have shape");
		    goto error;
		}
		
		for (i=0; i<rank; i++) {
		    rc = _dxfmatchint(f, &value);
		    if (!rc) {
			DXSetError(ERROR_DATA_INVALID,
				 "array shape list does not match rank");
			goto error;
		    }
		    if (value <= 0) {
			DXSetError(ERROR_DATA_INVALID, 
				 "non-positive array shape value");
			goto error;
		    }
		    
		    shape[i] = value;
		    /* add more checks to shape here for 0? for very large? */
		}
		
	    } else {
 
		rc = _dxfmatchint(f, &value);
		if (!rc) {
		    DXSetError(ERROR_DATA_INVALID, "missing or bad array shape");
		    goto error;
		}
		if (value <= 0) {
		    DXSetError(ERROR_DATA_INVALID, 
			     "non-positive array shape value");
		    goto error;
		}
		
		i = 0;
		shape[i++] = value;
		
		do {
		    rc = _dxfmatchint(f, &value);
		    if (!rc) {
			rc = OK;
			break;
		    }
		    shape[i++] = value;
		    
		} while(i < MAXRANK);
		
		if (i == MAXRANK) {
		    DXSetError(ERROR_DATA_INVALID, 
			       "shape cannot have more than %d terms", MAXRANK);
		    goto error;
		}
		
		rank = i;
		rankset++;
		
	    }
	    shapeset++;
	    break;

	  case KW_BINARY:
	  case KW_IEEE:
	    rc = skipkeyword(f);
	    datatype = (datatype & ~D_FORMATS) | D_IEEE;
	    break;
 
	  case KW_XDR:
	    rc = skipkeyword(f);
	    datatype = (datatype & ~D_FORMATS) | D_XDR;
	    break;
 
	  case KW_ASCII:
	    rc = skipkeyword(f);
	    datatype = (datatype & ~D_FORMATS) | D_TEXT;
	    break;
 
	  case KW_MSB:
	    rc = skipkeyword(f);
	    datatype = (datatype & ~D_BYTES) | D_MSB;
	    break;
 
	  case KW_LSB:
	    rc = skipkeyword(f);
	    datatype = (datatype & ~D_BYTES) | D_LSB;
	    break;
 
	  case KW_DATA:
	    /* start of data section.  must be followed by one of:
	     *  byte_offset (in this file, relative to 'end' keyword)
	     *  filename,byte_offset
	     *  FILE filename,byteoffset
	     *  FOLLOWS keyword
	     *  MODE keyword (which sets the defaults)
	     */
	    rc = skipkeyword(f);
 
	    /* set future file defaults */
	    if (match_keyword(f, KW_MODE))
		f->dformat = datatype;

	    if ((rankset && !shapeset) && (rank > 0)) {
		DXSetError(ERROR_DATA_INVALID,
			   "shape must be set if rank is larger than 0");
		goto error;
	    }
            if (items <= 0) {
                DXSetError(ERROR_DATA_INVALID,
                         "data item count must be set before data value list");
                goto error;
            }
            
	    /* make space for the data */
            if (kind == KW_CONSTANTARRAY) {
                conitems = items;
                items = 1;
            }

	    if (makeit) {
#if DEBUG_MSG
		DXDebug("O", "creating array object");
#endif
		
		o = (Object)DXNewArrayV(type, cat, rank, shape);
		if (!o)
		    goto error;
            
		if (!DXAddArrayData((Array)o, 0, items, NULL))
		    goto error;
	    }

            /* does the data follow immediately or is it at the end of this
	     *  file, or is it in another file?
             */
	    if (nextkeywordis(f, KW_FOLLOWS)) {
		
		switch(datatype & D_FORMATS) {
		  case D_TEXT:
		  default:
		    rc = skipkeyword(f);
		    if (makeit)
			rc = _dxfreadarray_text(f, (Array)o);
		    else {
			for (i=0; i<rank; i++)
			    items *= shape[i];
			items *= DXCategorySize(cat);
			if (type == TYPE_STRING)
			    items /= shape[rank==0 ? 1 : rank-1];
			rc = _dxfskiparray_text(f, items, type);
		    }
		    if (!rc)
			goto error;

		    break;
		    
		  case D_XDR:
		    _dxfnextline(f);
		    if (makeit)
			rc = _dxfreadarray_xdr(f->fd, (Array)o);
		    else {
			for (i=0; i<rank; i++)
			    items *= shape[i];
			items *= DXCategorySize(cat);
			rc = _dxfskiparray_xdr(f->fd, items, DXTypeSize(type));
		    }
		    if (!rc)
			goto error;
		    
		    _dxfendnotascii(f);
		    break;
		    
		  case D_IEEE:
		    _dxfnextline(f);
		    if (makeit)
			rc = _dxfreadarray_binary(f, (Array)o, datatype);
		    else {
			for (i=0; i<rank; i++)
			    items *= shape[i];
			items *= DXCategorySize(cat);
			rc = _dxfskiparray_binary(f, items, DXTypeSize(type));
		    }
		    if (!rc)
			goto error;

		    _dxfendnotascii(f);
		    break;
		    
		}
		
	    } else if (_dxfmatchint(f, &value))  {
		
		if (makeit) {
		    rc = _dxfreadoffset(f, (Array)o, value, datatype);
		    if (!rc)
			goto error;
		}
		
	    } else if (match_keyword(f, KW_FILE)) {
		
		rc = remote_objectid(f, &value, "data file");
		if (!rc)
		    goto error;
		
		if (makeit) {
		    rc = _dxfreadremote(f, (Array)o, value, datatype);
		    if (!rc)
			goto error;
		}
		
	    } else {
		DXSetError(ERROR_DATA_INVALID, "bad array data specification");
		goto error;
	    }
	    
            /* if constantarray, use real number of items to make array */
            if (makeit && (kind == KW_CONSTANTARRAY)) {
                tmp2 = (Object)DXNewConstantArrayV(conitems, 
					   DXGetArrayData((Array)o),
                                           type, cat, rank, shape);
                                           
                DXDelete(o);
                o = tmp2;
		tmp2 = NULL;
            }
	    done++;
	    break;
            
	  case KW_ATTRIBUTE:
	    rc = skipkeyword(f);
 
	    /* make a temporary object to hang the attributes onto until
	     * the array is constructed.
	     */
	    if(!tmp && makeit) {
		tmp = (Object)DXNewString("tmp");
		if(!tmp)
		    goto error;
	    }
 
	    rc = parse_attribute(f, tmp);
	    break;
 
          default:
	    /* any other keyword ends this section */
            done++;
            break;
        }
	
	/* any error ends this section */
	if (!rc)
	    done++;
    }
 
    if (!rc)
        goto error;
 

    if (!makeit)
	goto done;

    /* if not created yet (by a DATA keyword), make object */

    if(!o) {
#if DEBUG_MSG
	DXDebug("O", "creating array object");
#endif
	
	o = (Object)DXNewArrayV(type, cat, rank, shape);
	if(!o)
	    goto error;

	if (items > 0) {
	    o = (Object)DXAddArrayData((Array)o, 0, items, NULL);
	    if(!o)
		goto error;

	    DXWarning("creating an array with uninitialized data");
	}
    }
 
    if (tmp) {
	if (!DXCopyAttributes(o, tmp))
	    goto error;
	DXDelete(tmp);
	tmp = NULL;
    }
 
    if (!_dxfsetobjptr(f, f->curid, o))
	goto error;
	   
    f->curobj = o;

 done: 
    DXFree((Pointer)shape);
    return rc;
 
 error:
    DXFree((Pointer)shape);
    DXDelete(tmp);
    DXDelete(tmp2);
    DXDelete(o);
 
    return ERROR;
}


 
static Error parse_sarray(struct finfo *f, int kind)
{
    int value;
    Object o = NULL;
    int i, done = 0;
    int drank = 0;
    int which = MAKE_ALL_OBJS;
    Error rc = OK;

    Type type = TYPE_FLOAT;                
    int issigned = SIGN_UNSET;
    int signok = 0;
    int items = 0;
    int rank = 0;
    int shape = 0;
    Pointer origin = NULL;
    Pointer deltas = NULL;
    int *counts = NULL;
    int *offsets = NULL;
    Array *terms = NULL;
    Object tmp = NULL;
 
 
#if DEBUG_MSG
    DXDebug("E", "in parse_sarray");
#endif
 
 
    switch(kind) {
      case KW_REGULARARRAY:
	/* initialize these */
	origin = DXAllocateLocalZero(sizeof(double) * MAXRANK);
	deltas = DXAllocateLocalZero(sizeof(double) * MAXRANK);
	counts = (int *)DXAllocateLocalZero(sizeof(int) * MAXRANK);
	rank = 1;
 
	done = 0;
	while(!done) {
	    if(f->t.class != KEYWORD) {
		done++;
		break;
	    }
	    
	    switch(f->t.token.id) {
	      case KW_ORIGIN:
		skipkeyword(f);

#define READORIGIN(type) \
		rc = _dxfmatch##type(f, &((type *)origin)[i]); \
		if(!rc) { \
		    DXSetError(ERROR_DATA_INVALID, "missing or bad origin"); \
		    goto error; \
		} \
		do { \
		    i++; \
		    rc = _dxfmatch##type(f, &((type *)origin)[i]); \
		    if(!rc) { \
			rc = OK; \
			break; \
		    } \
		} while(i < MAXRANK); \
		if (i == MAXRANK) { \
		    DXSetError(ERROR_DATA_INVALID, \
			"origin cannot have more than %d terms", MAXRANK); \
		    goto error; \
		}
		

		i = 0;
		switch(type) {
		  case TYPE_UBYTE: READORIGIN(ubyte); break;
		  case TYPE_BYTE: READORIGIN(byte); break;
		  case TYPE_USHORT: READORIGIN(ushort); break;
		  case TYPE_SHORT: READORIGIN(short); break;
		  case TYPE_UINT: READORIGIN(uint); break;
		  case TYPE_INT: READORIGIN(int); break;
		  case TYPE_FLOAT: READORIGIN(float); break;
		  case TYPE_DOUBLE: READORIGIN(double); break;
		  case TYPE_HYPER:
		    DXSetError(ERROR_NOT_IMPLEMENTED, "hyper not supported");
		    goto error;
		  default:
		    DXSetError(ERROR_DATA_INVALID, "unrecognized type");
		    goto error;
		}
		
		shape = i;
		break;
		
	      case KW_DELTAS:
		skipkeyword(f);
		
		if(shape == 0) {
		    DXSetError(ERROR_DATA_INVALID, 
			     "must set origin before deltas");
		    goto error;
		}

#define READDELTAS(type) \
		for (i=0; i<shape; i++) { \
		    rc = _dxfmatch##type(f, &((type *)deltas)[i]); \
		    if(!rc) { \
			DXSetError(ERROR_DATA_INVALID, \
				   "missing or bad delta"); \
			goto error; \
		    } \
		} 
		
		switch(type) {
		  case TYPE_UBYTE: READDELTAS(ubyte); break;
		  case TYPE_BYTE: READDELTAS(byte); break;
		  case TYPE_USHORT: READDELTAS(ushort); break;
		  case TYPE_SHORT: READDELTAS(short); break;
		  case TYPE_UINT: READDELTAS(uint); break;
		  case TYPE_INT: READDELTAS(int); break;
		  case TYPE_FLOAT: READDELTAS(float); break;
		  case TYPE_DOUBLE: READDELTAS(double); break;
		  case TYPE_HYPER:
		    DXSetError(ERROR_NOT_IMPLEMENTED, "hyper not supported");
		    goto error;
		  default:
		    DXSetError(ERROR_DATA_INVALID, "unrecognized type");
		    goto error;
		}
		break;
		
	      case KW_COUNT:
		skipkeyword(f);
		
		rc = _dxfmatchint(f, &value);
		if(!rc) {
		    DXSetError(ERROR_DATA_INVALID, "missing or bad item count");
		    goto error;
		}
		
		items = value;
		break;
		
	      case KW_TYPE:
		rc = skipkeyword(f);
		
	      moretype:
		value = f->t.token.id;
		rc = skipkeyword(f);
		if(!rc) {
		    DXSetError(ERROR_DATA_INVALID, 
			       "bad or missing regulararray type");
		    goto error;
		}
		
		switch(value) {
		  case KW_SIGNED:
		    issigned = SIGN_YES;
		    goto moretype;
		  case KW_UNSIGNED:
		    issigned = SIGN_NO;
		    goto moretype;
		  case KW_CHAR:
		    signok = 1;
		    switch(issigned) {
		      case SIGN_YES:
			type = TYPE_BYTE;
			break;
		      case SIGN_NO:
		      case SIGN_UNSET:
			type = TYPE_UBYTE;
			break;
		    }
		    break;
		  case KW_SHORT:
		    signok = 1;
		    switch(issigned) {
		      case SIGN_YES:
		      case SIGN_UNSET:
			type = TYPE_SHORT;
			break;
		      case SIGN_NO:
			type = TYPE_USHORT;
			break;
		    }
		    break;
		  case KW_INTEGER:
		    signok = 1;
		    switch(issigned) {
		      case SIGN_YES:
		      case SIGN_UNSET:
			type = TYPE_INT;
			break;
		      case SIGN_NO:
			type = TYPE_UINT;
			break;
		    }
		    break;
		  case KW_HYPER:
		    signok = 1;
		    switch(issigned) {
		      case SIGN_YES:
		      case SIGN_UNSET:
			type = TYPE_HYPER;
			break;
		      case SIGN_NO:
			DXSetError(ERROR_NOT_IMPLEMENTED, 
				   "unsigned hyper not supported yet");
			goto error;
		    }
		    break;
		  case KW_FLOAT:
		    type = TYPE_FLOAT;
		    break;
		  case KW_DOUBLE:
		    type = TYPE_DOUBLE;
		    break;
		  default:
		    DXSetError(ERROR_DATA_INVALID, "bad array type");
		    goto error;
		}
		if ((issigned != SIGN_UNSET) && !signok) {
		    DXSetError(ERROR_DATA_INVALID, 
			   "signed or unsigned not valid for this array type");
		    goto error;
		}
		break;

	      case KW_ATTRIBUTE:
		rc = skipkeyword(f);
		
		/* make a temporary object to hang the attributes onto until
		 * the array is constructed.
		 */
		if(!tmp) {
		    tmp = (Object)DXNewString("tmp");
		    if(!tmp)
			goto error;
		}
		
		rc = parse_attribute(f, tmp);
		break;
		
	      case KW_RANK:
		rc = skipkeyword(f);
		
		rc = _dxfmatchint(f, &value);
		if(!rc) {
		    DXSetError(ERROR_DATA_INVALID, "bad or missing array rank");
		    goto error;
		}
		
		if(value != 1) {
		    DXSetError(ERROR_NOT_IMPLEMENTED, 
			     "only rank = 1 supported for regular arrays");
		    goto error;
		}
 
		rank = value;
		break;
            
	      case KW_SHAPE:
		rc = skipkeyword(f);
		
		rc = _dxfmatchint(f, &value);
		if(!rc) {
		    DXSetError(ERROR_DATA_INVALID,
			     "missing regular array shape");
		    goto error;
		}
		shape = value;
		break;
 
	      default:
		done++;
		break;
	    }
	}
 
        if (!rc)
            goto error;
 
	if(shape == 0) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing origin clause");
	    goto error;
	}
	    
	/* make array here */
	o = (Object)DXNewRegularArray(type, shape, items,
				    (Pointer)origin, (Pointer)deltas);
	if(!o)
	    goto error;
 
	break;
 
      case KW_PATHARRAY:
 
	/* optional */
	match_keyword(f, KW_COUNT);
 
	rc = _dxfmatchint(f, &value);
	if (!rc) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing path array length");
	    goto error;
	}
		
	/* make array here */
	o = (Object)DXNewPathArray(value);
	if (!o)
	    goto error;
 
	if (match_keyword(f, KW_ATTRIBUTE))
	    rc = parse_attribute(f, o);
 
	break;
 
      case KW_MESHARRAY:
	/* initialize these */
	offsets = (int *)DXAllocateLocalZero(sizeof(int) * MAXRANK);
	terms = (Array *)DXAllocateLocalZero(sizeof(Array) * MAXTERMS);
	items = 0;
 
	done = 0;
	while(!done) {
	    if(f->t.class != KEYWORD) {
		done++;
		break;
	    }
	    
	    switch(f->t.token.id) {
	      case KW_ATTRIBUTE:
		skipkeyword(f);
		
		/* make a temporary object to hang the attributes onto until
		 * the array is constructed.
		 */
		if(!tmp) {
		    tmp = (Object)DXNewString("tmp");
		    if(!tmp)
			goto error;
		}
		
		rc = parse_attribute(f, tmp);
		if (!rc)
		    goto error;

		break;

	      case KW_TERM:
		skipkeyword(f);
 
		/* arrays */
		rc = object_or_id(f, (Object *)&terms[items], "mesh term",
				  &which);
		if (!rc)
		    goto badterm;

		items++;
		break;
 
	      badterm:
		DXSetError(ERROR_DATA_INVALID, 
			 "bad or missing mesh array term %d", items);
		goto error;
 
	      case KW_MESHOFFSETS:
		skipkeyword(f);

		/* parse 'items' numbers */
		for (i=0; i<items; i++) {
		    rc = _dxfmatchint(f, &value);
		    if (!rc) {
			DXSetError(ERROR_DATA_INVALID, 
				   "missing or bad mesh offset");
			goto error;
		    }
		    
		    offsets[i] = value;
		}
		break;
		

	      default:
		done++;
		break;
	    }
	}
	
	if (items == 0) {
	    DXSetError(ERROR_DATA_INVALID, "missing mesh array terms");
	    goto error;
	}

	if (which != IDENTIFY_OBJS) {
	    o = (Object)DXNewMeshArrayV(items, terms);
	    if(!o)
		goto error;
	    
	    if (!DXSetMeshOffsets((MeshArray)o, offsets))
		goto error;
	}

	/* this shouldn't need to be here anymore */
	if (match_keyword(f, KW_ATTRIBUTE))
	    rc = parse_attribute(f, o);
 
	break;
 
      case KW_PRODUCTARRAY:
	/* initialize these */
	terms = (Array *)DXAllocateLocalZero(sizeof(Array) * MAXTERMS);
 
	if(match_keyword(f, KW_ATTRIBUTE)) {
 
	    /* make a temporary object to hang the attributes onto until
	     * the array is constructed.
	     */
	    if(!tmp) {
		tmp = (Object)DXNewString("tmp");
		if(!tmp)
		    goto error;
	    }
 
	    rc = parse_attribute(f, tmp);
	    if (!rc)
		goto error;
	}
 
	i = 0;
	while (match_keyword(f, KW_TERM)) {
 
	    /* arrays */
	    rc = object_or_id(f, (Object *)&terms[i], "product term", &which);
	    if (!rc)
		goto badterm2;

	    i++;
	    if (i >= MAXRANK) {
		DXSetError(ERROR_DATA_INVALID, 
			   "product array cannot have more than %d terms",
			   MAXRANK);
		goto error;
	    }
	    continue;
 
	  badterm2:
	    DXSetError(ERROR_DATA_INVALID, 
		     "bad or missing product array term %d", i);
	    goto error;
 
	}
 
	if(i == 0) {
	    DXSetError(ERROR_DATA_INVALID, "missing product array terms");
	    goto error;
	}
 
	if (which != IDENTIFY_OBJS) {
	    o = (Object)DXNewProductArrayV(i, terms);
	    if(!o)
		goto error;
	}
	/* this doesn't need to be here anymore */
	if (match_keyword(f, KW_ATTRIBUTE))
	    rc = parse_attribute(f, o);
 
	break;
 
      case KW_GRIDPOSITIONS:
	/* initialize these */
	origin = DXAllocateLocalZero(sizeof(float) * MAXRANK);
	deltas = DXAllocateLocalZero(sizeof(float) * MAXRANK * MAXRANK);
	counts = (int *)DXAllocateLocalZero(sizeof(int) * MAXRANK);
 
	match_keyword(f, KW_COUNT);
 
	rc = _dxfmatchint(f, &value);
	if(!rc) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing counts");
	    goto error;
	}
 
	i = 0;
	counts[i++] = value;
	
	do {
	    rc = _dxfmatchint(f, &value);
	    if(!rc) {
		rc = OK;
		break;
	    }
	    
	    counts[i++] = value;
 
	} while(i < MAXRANK);
	
	if (i == MAXRANK) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "counts cannot have more than %d terms", MAXRANK);
	    goto error;
	}
 
	rank = i;
	for (i=0; i<rank; i++)
	    ((float *)deltas)[i * rank + i] = 1.0;
            
	done = 0;
	while(!done) {
	    if(f->t.class != KEYWORD) {
		done++;
		break;
	    }
	    
	    switch(f->t.token.id) {
	      case KW_ORIGIN:
		rc = skipkeyword(f);
		
		for (i=0; i<rank; i++) {
		    rc = _dxfmatchfloat(f, &((float *)origin)[i]);
		    if(!rc) {
			DXSetError(ERROR_DATA_INVALID, "missing or bad origin");
			goto error;
		    }
		}
		break;
		
	      case KW_DELTAS:
		rc = skipkeyword(f);
		
		for (i=0; i<rank; i++) {
		    rc = _dxfmatchfloat(f, &((float *)deltas)[drank * rank + i]);
		    if(!rc) {
			DXSetError(ERROR_DATA_INVALID, 
				 "missing or bad delta vectors");
			goto error;
		    }
		}
		drank++;
		break;
		
	      case KW_ATTRIBUTE:
		rc = skipkeyword(f);
		
		/* make a temporary object to hang the attributes onto until
		 * the array is constructed.
		 */
		if(!tmp) {
		    tmp = (Object)DXNewString("tmp");
		    if(!tmp)
			goto error;
		}
		
		rc = parse_attribute(f, tmp);
		break;
 
	      default:
		done++;
		break;
	    }
	}
	
	/* make grid here */
	o = (Object)DXMakeGridPositionsV(rank, counts, 
                                       (float *)origin, (float *)deltas);
	if(!o)
	    goto error;
 
	break;
 
      case KW_GRIDCONNECTIONS:
	/* initialize these */
	counts = (int *)DXAllocateLocalZero(sizeof(int) * MAXRANK);
	offsets = (int *)DXAllocateLocalZero(sizeof(int) * MAXRANK);
 
	match_keyword(f, KW_COUNT);
 
	rc = _dxfmatchint(f, &value);
	if(!rc) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing counts");
	    goto error;
	}
 
	i = 0;
	counts[i++] = value;
	
	do {
	    rc = _dxfmatchint(f, &value);
	    if(!rc) {
		rc = OK;
		break;
	    }
	    
	    counts[i++] = value;
 
	} while(i < MAXRANK);
	
	if (i == MAXRANK) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "counts cannot have more than %d terms", MAXRANK);
	    goto error;
	}
 
	rank = i;

	/* make grid here - this sets element type now */
	o = (Object)DXMakeGridConnectionsV(rank, counts);
	if (!o)
	    goto error;

	if (match_keyword(f, KW_ATTRIBUTE))
	    rc = parse_attribute(f, o);
 
	if (match_keyword(f, KW_MESHOFFSETS)) {

	    for (i=0; i<rank; i++) {
		rc = _dxfmatchint(f, &value);
		if(!rc) {
		    DXSetError(ERROR_DATA_INVALID, "missing or bad meshoffsets");
		    goto error;
		}
		
		offsets[i] = value;
	    }
	}
 
	if (!DXSetMeshOffsets((MeshArray)o, offsets))
	    goto error;

	if (match_keyword(f, KW_ATTRIBUTE))
	    rc = parse_attribute(f, o);
 
	break;
    }
 
    if (!rc)
        goto error;

    if (which == IDENTIFY_OBJS)
	goto done;
    
    if(tmp) {
	DXCopyAttributes(o, tmp);
	DXDelete(tmp);
	tmp = NULL;
    }
    
    /* add new array to object list */
    if (!_dxfsetobjptr(f, f->curid, o))
	goto error;
    
    
    f->curobj = o;

  done:
    DXFree((Pointer)origin);
    DXFree((Pointer)deltas);
    DXFree((Pointer)counts);
    DXFree((Pointer)offsets);
    DXFree((Pointer)terms);
    DXDelete(tmp);
 
    return rc;
 
 error:
 
    DXDelete(o);
    rc = ERROR;
    goto done;

}
 
 
static Error parse_light(struct finfo *f)
{
    int type = KW_DISTANT;
    int relative = KW_LIGHT;
    float fvalue;
    int value;
    Object o = NULL;
    Vector d;
    Point p;
    float *fp;
    RGBColor dc, ac;
    char *colorname = NULL;
    Error rc = OK;
    int i;
    int specific = 0;
    int done = 0;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_light");
#endif
 
    /* set defaults */
    dc = DXRGB(1, 1, 1);		/* distant light color default */
    ac = DXRGB(.25, .25, .25);	/* ambient light color default */
    d = DXVec(0, 0, 1);
    p = DXPt(0, 0, 0);

    /* do until the next keyword isn't type, color or direction
     */
    while (!done) {
        if (f->t.class != KEYWORD)
            break;
 
        switch(f->t.token.id) {
          case KW_TYPE:
            rc = skipkeyword(f);
 
            type = f->t.token.id;
            rc = skipkeyword(f);
            if(!rc)
                DXErrorReturn(ERROR_DATA_INVALID, "bad or missing light type");
            
            if (type != KW_DISTANT && type != KW_AMBIENT) 
                DXMessage("only distant or ambient lights supported");
	    
            break;
 
          case KW_DISTANT:
	  case KW_AMBIENT:
	    type = f->t.token.id;
            rc = skipkeyword(f);
            
            if (type != KW_DISTANT && type != KW_AMBIENT) 
                DXMessage("only distant or ambient lights supported");
	    
            break;
 
          case KW_DIRECTION:
            rc = skipkeyword(f);
 
            fp = (float *)&d;
            for (i=0; i<3; i++) {
		rc = _dxfmatchfloat(f, &fvalue);
                if(!rc) {
		    DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing light direction");
		}
                *fp++ = fvalue;
            }
	    specific++;
            break;
 
          case KW_POSITION:
            rc = skipkeyword(f);
 
            fp = (float *)&p;
            for (i=0; i<3; i++) {
		rc = _dxfmatchfloat(f, &fvalue);
                if(!rc) {
		    DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing light position");
		}
                *fp++ = fvalue;
            }
	    specific++;
	    DXWarning("local lights not supported, position keyword ignored");
            break;
 
          case KW_COLOR:
            rc = skipkeyword(f);

	    if (get_string(f, &value)) {
		colorname = _dxfdictname(f->d, value);
		if (!DXColorNameToRGB(colorname, &dc)) {
		    DXErrorReturn(ERROR_DATA_INVALID,
				"unrecognized light color name");
		}
	    } else {
		fp = (float *)&dc;
		for (i=0; i<3; i++) {
		    rc = _dxfmatchfloat(f, &fvalue);
		    if(!rc) {
			DXErrorReturn(ERROR_DATA_INVALID, 
				    "bad or missing light color");
		    }
		    *fp++ = fvalue;
		}
	    }
	    ac = dc;
	    break;

          case KW_FROM:
            rc = skipkeyword(f);

            /* FALL THRU */

          case KW_CAMERA:
            if (!match_keyword(f, KW_CAMERA)) {
                DXErrorReturn(ERROR_DATA_INVALID, 
                      "distant light direction may be given as 'from camera'");
            }
            relative = KW_CAMERA;
            break;

          default:
            done++;
            break;
        }
 
    }

    if (!make_obj(f))
	return rc;

    switch(type) {
      case KW_AMBIENT:
	if (specific) {
	    DXSetError(ERROR_DATA_INVALID, 
		     "ambient lights cannot have position or direction");
	    return ERROR;
	}
	o = (Object)DXNewAmbientLight(ac);
	break;

      case KW_DISTANT:
	switch(relative) {
	  case KW_LIGHT:
	    o = (Object)DXNewDistantLight(d, dc);
	    break;
	  case KW_CAMERA:
	    o = (Object)DXNewCameraDistantLight(d, dc);
	    break;
	}
	break;
    }

    if (!o)
        return ERROR;
    
    if (!_dxfsetobjptr(f, f->curid, o)) {
	DXDelete(o);
	return ERROR;
    }
    
    f->curobj = o;
 
    return rc; 
}

#define DEG2RAD(x)      (x/180.0*M_PI)   /* degrees to radians */
 
static Error parse_camera(struct finfo *f)
{ 
    Object o=NULL;
    Error rc = OK;
    float width, aspect, angle, fov;
    float *fp, fvalue;
    int value;
    float xxx;
    int isortho = -1;
    int type;
    int resolution;
    Point from, to;
    Vector up;
    char *colorname = NULL;
    RGBColor background;
    int i;
    int done = 0;

#if DEBUG_MSG
    DXDebug("E", "in parse_camera");
#endif


    /* default parms, in case user specifies only one parm which gets
     *  set in a call which requires 2 or 3 parms (like to set the
     *  width, you also have to set aspect at the same time).
     */
    from = DXPt(0, 0, 1);
    to = DXPt(0, 0, 0);
    up = DXVec(0, 1, 0);
    width = 100.0;
    aspect = 0.75;
    angle = 30.0;
    resolution = 640;
    background = DXRGB(0.0, 0.0, 0.0);
 

    if (make_obj(f)) {
	o = (Object)DXNewCamera();
	if(!o)
	    return ERROR;
    }
 
    while(!done) {
        if(f->t.class != KEYWORD)
            break;
	
        switch(f->t.token.id) {
          case KW_TYPE:
            rc = skipkeyword(f);
  
            type = f->t.token.id;
            rc = skipkeyword(f);
            if(!rc)
                DXErrorReturn(ERROR_DATA_INVALID, "bad or missing camera type");
            
            if (type != KW_ORTHOGRAPHIC && type != KW_PERSPECTIVE) {
                DXErrorReturn(ERROR_DATA_INVALID,
                            "camera type must be orthographic or perspective");
            }
	    
            switch(type) {
              case KW_PERSPECTIVE:
                isortho = 0;
                break;
              case KW_ORTHOGRAPHIC:
                isortho = 1;
                break;
            }
            break;
 
          case KW_ORTHOGRAPHIC:
            rc = skipkeyword(f);

            isortho = 1;
            break;

          case KW_PERSPECTIVE:
            rc = skipkeyword(f);

            isortho = 0;
            break;

          case KW_FROM:
            rc = skipkeyword(f);
 
            fp = (float *)&from;
            for (i=0; i<3; i++) {
		rc = _dxfmatchfloat(f, &fvalue);
                if(!rc) {
		    DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing camera from point");
		}
                *fp++ = fvalue;
            }
 
            break;
 
          case KW_TO:
            rc = skipkeyword(f);
 
            fp = (float *)&to;
            for (i=0; i<3; i++) {
		rc = _dxfmatchfloat(f, &fvalue);
                if(!rc) {
		    DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing camera to point");
		}
                *fp++ = fvalue;
            }
 
            break;
 
          case KW_UP:
            rc = skipkeyword(f);
 
            fp = (float *)&up;
            for (i=0; i<3; i++) {
		rc = _dxfmatchfloat(f, &fvalue);
                if(!rc) {
		    DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing camera up vector");
		}
                *fp++ = fvalue;
            }
 
            break;
 
          case KW_WIDTH:
            rc = skipkeyword(f);
 
	    rc = _dxfmatchfloat(f, &width);
	    if(!rc || width <= 0.0) {
		DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing camera width");
	    }
	    
	    break;


          case KW_HEIGHT:
	    /* not supported anymore.  if they have a good value, just ignore
             *  it and give a warning.  if they have a bad value, error out.
	     */
	    rc = skipkeyword(f);

	    rc = _dxfmatchfloat(f, &xxx);
	    if(!rc) {
		DXErrorReturn(ERROR_DATA_INVALID, 
				"camera 'height' parameter obsolete");
	    }

	    DXWarning("'height' keyword obsolete; ignored");
	    break;

          case KW_ASPECT:
            rc = skipkeyword(f);
 
	    rc = _dxfmatchfloat(f, &aspect);
	    if(!rc) {
		DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing camera aspect ratio");
	    }
	    
	    break;

          case KW_RESOLUTION:
            rc = skipkeyword(f);
 
	    rc = _dxfmatchint(f, &resolution);
	    if(!rc) {
		DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing camera resolution");
	    }
	    
	    break;

          case KW_ANGLE:
            rc = skipkeyword(f);
 
	    rc = _dxfmatchfloat(f, &angle);
	    if(!rc) {
		DXErrorReturn(ERROR_DATA_INVALID, 
				"bad or missing angle for perspective");
	    }
	    
	    if (angle < 0.0 || angle >= 180.0) {
		DXErrorReturn(ERROR_DATA_INVALID,
		"angle must be greater than or equal to 0.0 but less than 180");
	    }

            if (angle > 0.0) {
                switch(isortho) {
                  case -1:   /* don't know yet */
                    isortho = 0;
                    break;
                  case 0:    /* is perspective */
                    break;
                  case 1:    /* is orthographic */
                    DXErrorReturn(ERROR_DATA_INVALID,
                        "angle parameter only valid for perspective camera");
                }
            } else
                isortho = 1;
                    
	    break;

          case KW_COLOR:
            rc = skipkeyword(f);
 
	    if (get_string(f, &value)) {
		colorname = _dxfdictname(f->d, value);
		if (!DXColorNameToRGB(colorname, &background)) {
		    DXAddMessage("camera background");
		    goto error;
		}
	    } else {
		fp = (float *)&background;
		for (i=0; i<3; i++) {
		    rc = _dxfmatchfloat(f, &fvalue);
		    if(!rc) {
			DXErrorReturn(ERROR_DATA_INVALID, 
				    "bad or missing background color");
		    }
		    *fp++ = fvalue;
		}
	    }
 
            break;
 
	  default:
	    done++;
	}
    }

    if (!make_obj(f))
	return OK;

    /* now set the camera parms.
     */

    if (!isortho) {
	fov = 2 * tan(DEG2RAD(angle/2));
	if (!DXSetPerspective((Camera)o, fov, aspect))
	    goto error;
    } else {
	if (!DXSetOrthographic((Camera)o, width, aspect))
	    goto error;
    }

    if (!DXSetResolution((Camera)o, resolution, 1))
	goto error;

    if (!DXSetView((Camera)o, from, to, up))
	goto error;

    if (!DXSetBackgroundColor((Camera)o, background))
	goto error;


    if (!_dxfsetobjptr(f, f->curid, o))
	goto error;

 
    f->curobj = o;
    return rc; 

  error:
    DXDelete(o);
    return ERROR;
}
 
 
static Error parse_xform(struct finfo *f)
{ 
    int i, j, done;
    float fvalue;
    Matrix m;
    Object o = NULL;
    Object subo = NULL;
    int which;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_xform");
#endif
 
    /* optional */
    match_keyword(f, KW_OF);
 
 
    /* object to be transformed */
    rc = object_or_id(f, &subo, "transform", &which);
    if (!rc)
	return ERROR;

    done = 0;
    while(f->t.class == KEYWORD) {
	switch(f->t.token.id) {
	  case KW_BY:
	  case KW_MATRIX:
	    rc = skipkeyword(f);
	    break;

	  case KW_TIMES:
	    done++;
	    break;

	  default:
	    DXErrorReturn(ERROR_DATA_INVALID, "bad or missing transform matrix");
 
	}
	if (done)
	    break;
    }
 
    /* 12 floats */
 
    match_keyword(f, KW_TIMES);
    for (i=0; i<3; i++)
	for (j=0; j<3; j++) {
	    rc = _dxfmatchfloat(f, &fvalue);
	    if (!rc) {
		DXErrorReturn(ERROR_DATA_INVALID, 
			    "bad or missing transform matrix");
	    }
    
	    m.A[i][j] = fvalue;
	}
 
    match_keyword(f, KW_PLUS);
    for (i=0; i<3; i++) {
	rc = _dxfmatchfloat(f, &fvalue);
	if (!rc) {
	    DXErrorReturn(ERROR_DATA_INVALID, "bad or missing transform matrix");
	}
	
	m.b[i] = fvalue;
    }
 
 
    if (which == IDENTIFY_OBJS)
	return rc;

    o = (Object)DXNewXform(subo, m);
    if(!o)
	return ERROR;
    
    if (!_dxfsetobjptr(f, f->curid, o)) {
	DXDelete(o);
	return ERROR;
    }
 
    f->curobj = o;
    
    return rc; 
}

static Error parse_clipped(struct finfo *f)
{ 
    Object o = NULL;
    Object clipper = NULL;
    Object clippie = NULL;
    int which;
    Error rc = ERROR;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_clipped");
#endif

    /* skip these 
     */
    match_keyword(f, KW_CLIPPED);
    match_keyword(f, KW_BY);

    /* clipper
     */
    rc = object_or_id(f, &clipper, "clipped by", &which);
    if (!rc)
	return ERROR;
 
    /* skip this
     */
    match_keyword(f, KW_OF);

    /* clippie
     */
    rc = object_or_id(f, &clippie, "clipped of", &which);
    if (!rc)
	return ERROR;
 
    
    if (which == IDENTIFY_OBJS)
	return rc;

    /* libdx call to create clipped object
     */
    o = (Object)DXNewClipped(clippie, clipper);
    if (!o)
        return ERROR;
    
    if (!_dxfsetobjptr(f, f->curid, o)) {
	DXDelete(o);
	return ERROR;
    }
    
    f->curobj = o;
 
    return OK; 
}

#define BEHIND -1
#define INSIDE  0
#define INFRONT 1

#define OneKind  "only one of 'world', 'viewport', 'pixel' allowed"
#define OnePlace "only one of 'behind', 'inside' or 'infront' allowed"
 
static Error parse_screen(struct finfo *f)
{
    Object so = NULL;
    Object o = NULL;
    Error rc = ERROR;
    int setpos = 0;
    int position = SCREEN_WORLD;
    int setwhere = 0;
    int where = INSIDE;
    int which;
    int class, keyword;
    int done = 0;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_screen");
#endif

    /* do until the next keyword isn't valid
     */
    while (!done) {
	rc = next_class(f, &class);
        if (!rc || ((class != KEYWORD)))
            break;

	next_id(f, &keyword);
        switch (keyword) {
          case KW_WORLD:
            rc = skipkeyword(f);
 
            if (setpos)
                DXErrorReturn(ERROR_DATA_INVALID, OneKind);
            
            position = SCREEN_WORLD;
	    setpos++;
            break;
 
          case KW_VIEWPORT:
            rc = skipkeyword(f);
 
            if (setpos)
                DXErrorReturn(ERROR_DATA_INVALID, OneKind);
            
            position = SCREEN_VIEWPORT;
	    setpos++;
            break;
 
          case KW_PIXEL:
            rc = skipkeyword(f);
 
            if (setpos)
                DXErrorReturn(ERROR_DATA_INVALID, OneKind);
            
            position = SCREEN_PIXEL;
	    setpos++;
            break;
 
          case KW_STATIONARY:
            rc = skipkeyword(f);
 
            if (setpos)
                DXErrorReturn(ERROR_DATA_INVALID, OneKind);
            
            position = SCREEN_STATIONARY;
	    setpos++;
            break;
 
          case KW_BEHIND:
            rc = skipkeyword(f);
 
            if (setwhere)
                DXErrorReturn(ERROR_DATA_INVALID, OnePlace);
            
            where = BEHIND;
	    setwhere++;
            break;
 
          case KW_INSIDE:
            rc = skipkeyword(f);
 
            if (setwhere)
                DXErrorReturn(ERROR_DATA_INVALID, OnePlace);
            
            where = INSIDE;
	    setwhere++;
            break;
 
          case KW_INFRONT:
            rc = skipkeyword(f);
 
            if (setwhere)
                DXErrorReturn(ERROR_DATA_INVALID, OnePlace);
            
            where = INFRONT;
	    setwhere++;
            break;
 
          case KW_OF:
            rc = skipkeyword(f);
	    done++;
            break;
 
          default:
            done++;
            break;
        }
 
    }
 
    /* object to make into screen object 
     */
    rc = object_or_id(f, &so, "screen", &which);
    if (!rc)
	return ERROR;

    if (which == IDENTIFY_OBJS)
	return OK;
    
    /* libdx call to create new screen object
     */
    o = (Object)DXNewScreen(so, position, where);
    if (!o)
        return ERROR;
    
    if (!_dxfsetobjptr(f, f->curid, o)) {
	DXDelete(o);
	return ERROR;
    }
    
    f->curobj = o;
 
    return OK; 
}


static Error parse_attribute(struct finfo *f, Object o)
{
    int name;
    Token value;
    int ntype;
    int makeit, which;
    Object newo = NULL;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in parse_attribute");
#endif
 
    /* here we either need to set the uses field or actually attach
     *  the attribute to the object.
     */
    makeit = make_obj(f);

    /* attribute name */
    rc = get_string(f, &name);
    if (!rc)
	DXErrorReturn(ERROR_DATA_INVALID, "bad or missing attribute name");
 
    /* optional */
    match_keyword(f, KW_VALUE);
 
    /* string "string", number N, or objectid (number, name or file) */
    if (match_keyword(f, KW_STRING)) {

	newo = make_string(f);
        if (!newo)
            return ERROR;
	
    } else if (match_keyword(f, KW_NUMBER)) {
	if (!get_number(f, &ntype, &value))
	    DXErrorReturn(ERROR_DATA_INVALID, "bad numeric attribute value");
	    
	if (!makeit)
	    goto done;

	switch (ntype) {
	  case FLOAT:
	    newo = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	    if (!DXAddArrayData((Array)newo, 0, 1, (Pointer *)&value))
		return ERROR;
	    break;
	    
	  case INTEGER:
	    newo = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	    if (!DXAddArrayData((Array)newo, 0, 1, (Pointer *)&value))
		return ERROR;
	    break;
	    
	  default:
	    DXErrorReturn(ERROR_DATA_INVALID, 
			  "bad number for attribute value");
	}
	
    } else {
	
	rc = object_or_id(f, &newo, "attribute", &which);
	if (!rc)
	    return ERROR;
    }
    
#if DEBUG_MSG
    if (makeit)
	DXDebug("O", "setting attribute %s to object 0x%08x for object 0x%08x",
		_dxfdictname(f->d, name), newo, o);
#endif
    
    if (!makeit) {
	DXDelete((Object)newo);
	return rc;
    }

    if (!DXSetAttribute(o, _dxfdictname(f->d, name), newo))
        return ERROR;
    
  done:
    return rc;
}
 
 

/* things we seem to need here:
 *
 *  return the next object id
 *  return the next object id, maybe adding to the current obj ref list
 *  return the next object
 *
 */

/* return the next object id - either number or dict id of "name"
 */
static Error local_objectid(struct finfo *f, int *objnum, char *what)
{
    int slot, type, value;
    
#if DEBUG_MSG
    DXDebug("E", "in local_objectid");
#endif

    /* remote objects not allowed here */
    if (match_keyword(f, KW_FILE)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "`file' keyword not allowed for identifying %s", what);
	return ERROR;
    }

    /* is it a simple number? */
    if (_dxfmatchint(f, &value)) {
	*objnum = USER_ID(value);
	return OK;
    }
    
    /* object by name? */
    if (get_string(f, &slot)) {
	if (!_dxfgetdictinfo(f->d, slot, &type, &value))
	    return ERROR;
	
	/* a name we've seen before? */
	if (type == ALIAS) {
	    *objnum = value;
	    return OK;
	}
	
	/* have the dict assign a new value for it and get it back */
	if (!_dxfsetdictalias(f->d, slot, &value))
	    return ERROR;
	
	*objnum = value;
	return OK;
    }

    DXSetError(ERROR_DATA_INVALID, "bad object number or name for %s", what);
    return ERROR;
}
 
/* FILE needs to have already been parsed off.
 * 
 * <<<THIS HAS TO SET *objnum TO SOMETHING BEFORE IT RETURNS>>>
 */
static Error 
remote_objectid(struct finfo *f, int *objnum, char *what)
{
    Error rc = OK;
    int value;
    
#if DEBUG_MSG
    DXDebug("E", "in remote_objectid");
#endif

    /* get a unique id here and assign an objlist id struct to it.
     */
    value = _dxfgetuniqueid(f->d);
    rc = _dxfaddobjlist(f, value, NULL, NULL, 0);
    if (!rc)
	return ERROR;

    /* call file_lists here with that new id? 
     * when it returns, the getby struct should be filled in.
     */
    rc = file_lists(f, value);
    if (!rc)
	goto error;

    *objnum = value;
    return OK;

    
  error:
    DXSetError(ERROR_DATA_INVALID, 
	       "bad filename, or object number or name for %s", what);
    return ERROR;
}

/* either local or remote is ok.
 *  'what' is a string for the error message saying what the object id
 *  is supposed to be identifying.
 */ 
static Error 
parse_objectid(struct finfo *f, int *objnum, int *objtype, char *what)
{
    if (match_keyword(f, KW_FILE)) {
	*objtype = ID_REMOTE;
	return remote_objectid(f, objnum, what);
    }

    *objtype = ID_LOCAL;
    return local_objectid(f, objnum, what);
}


/* if next token is a valid object id, either look it up and return it,
 *  or mark it as used by the current object.  the which flag says what
 *  it did.
 */
static Error 
object_or_id(struct finfo *f, Object *o, char *what, int *which)
{
    int rc;
    int value;
    int objtype;

    *which = f->activity;

    rc = parse_objectid(f, &value, &objtype, what);
    if (!rc)
	return ERROR;

    if (*which == IDENTIFY_OBJS) {
	rc = _dxfobjusesobj(f, f->curid, value);
	if (!rc) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing %s object", what);
	    return ERROR;
	}
    }
    else {
	if (objtype == ID_LOCAL)
	    rc = _dxflookobjlist(f, value, o, NULL);
	else
	    rc = remote_object(f, value, o);
 
	if (!rc || !*o) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing %s object", what);
	    return ERROR;
	}
    }
 
    return rc;
}


/* if next token is a valid object id, either look it up and return it,
 *  or mark it as used by the current object.  the which flag says what
 *  it did.
 */
static Error 
seriesobject_or_id(struct finfo *f, Object *o, char *what, int *which, 
		   int enumval, int *skip)
{
    int rc;
    int value;
    int objtype;
    int start;
    struct getby *gp;

    *which = f->activity;

    rc = parse_objectid(f, &value, &objtype, what);
    if (!rc)
	return ERROR;
    
    gp = &(f->gb);
    
    /* if out of range, don't mark the object used or construct it.
     */
    if (gp->seriesflag != 0) {
	if (gp->seriesflag & SL_START) {
	    start = gp->serieslim[0];
	    if (start > enumval) {
		*skip = 1;
		return OK;
	    }
	} else
	    start = 0;

	if (gp->seriesflag & SL_END) {
	    if (gp->serieslim[1] < enumval) {
		*skip = 1;
		return OK;
	    }
	}

	if (gp->seriesflag & SL_DELTA) {
	    if ((enumval - start) % gp->serieslim[2]) {
		*skip = 1;
		return OK;
	    }
	}
    }
 
    *skip = 0;

    if (*which == IDENTIFY_OBJS) {
	rc =  _dxfobjusesobj(f, f->curid, value);
	if (!rc) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing %s object", what);
	    return ERROR;
	}
    }
    else {
	if (objtype == ID_LOCAL)
	    rc = _dxflookobjlist(f, value, o, NULL);
	else
	    rc = remote_object(f, value, o);
 
	if (!rc || !*o) {
	    DXSetError(ERROR_DATA_INVALID, "bad or missing %s object", what);
	    return ERROR;
	}
    }
 
    return rc;
}

/*
 *  return an object from another file
 *  
 *  input is dict id of filename,number or filename,name, output is that object
 *  
 */
static Error remote_object(struct finfo *f, int value, Object *o)
{
    struct finfo newf;
    struct getby **gpp;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in remote_object");
#endif
 
    *o = NULL;

    /* setup finfo struct 
     */

    /* clear struct to be sure all pointers are NULL, and set stuff which
     *  we know the value of now.  then call the init subroutine to finish
     *  the rest of the initialization.
     */
    memset(&newf, '\0', sizeof(newf));
 
    gpp = _dxfgetobjgb(f, value);
    if (!gpp)
	goto done;

    newf.fd = _dxfopen_dxfile((*gpp)->fname, f->fname, &newf.fname,".dx");
    if (!newf.fd)
	goto done;
    
    /* really copy struct contents to new file struct */
    newf.gb = **gpp;

    newf.recurse = f->recurse + 1;
    newf.onepass = f->onepass;

    rc = _dxfinitfinfo(&newf);
    if (!rc) {
	if (newf.fd)
	    _dxfclose_dxfile(newf.fd, newf.fname);
	DXFree((Pointer)newf.fname);
       	return rc;
    }


    /* read the file and construct the requested object
     */
    rc = _dxfparse_file(&newf, o);
    if (!rc)
	goto done;

    
#if DEBUG_MSG
    if (newf.gb.numlist)
	DXDebug("I", "end of remote file object '%s':%d", 
		newf.fname, newf.gb.numlist[0]);
    else if (newf.gb.namelist)
	DXDebug("I", "end of remote file object '%s':'%s'", 
		newf.fname, newf.gb.namelist[0]);
    else
	DXDebug("I", "end of remote file '%s':", newf.fname);
#endif
 
    if (rc != OK) {
	DXDelete(*o);
        goto done;
    }

    /* add it to this files object list */
    if (!_dxfsetobjptr(f, value, *o))
	goto done;
    
    DXDelete(*o);    /* get rid of extra reference added by setobjptr */
 
  done:
	DXDebug("F", "trying at (1) to close remote `%s'", newf.fname);
    if (newf.fd)
	_dxfclose_dxfile(newf.fd, newf.fname);
    DXFree((Pointer)newf.fname);

    _dxffreefinfo(&newf);
    return rc; 
}
 
/* extract the parts of a filename,number or filename,name token
 *  allocates and sets up local getby struct in the object structure.
 *  the getby struct gets deleted when objectlist is deleted.
 */
static Error 
file_lists(struct finfo *f, int value)
{
    struct getby **gpp;
    char *cp;
    int fnamewas = 0;
 
#if DEBUG_MSG
    DXDebug("E", "in file_lists");
#endif
 
    gpp = _dxfgetobjgb(f, value);
    if (!gpp)
	return ERROR;

    *gpp = (struct getby *)DXAllocateLocalZero(sizeof(struct getby));
    if (!(*gpp))
	return ERROR;

    /* set the default to be the last object in the file */
    (*gpp)->which = GETBY_NONE;

    /* first thing must be a filename.  i can set the mustmatch flag here.
     */
    _dxfsetnexttype(f, STRING);
    if (!get_string(f, &value)) {
	if (!get_ident(f, &value)) {
	    DXSetError(ERROR_DATA_INVALID, "bad filename");
	    return ERROR;
	} else
	    fnamewas = IDENTIFIER;
    } else
	fnamewas = STRING;
    _dxfsetnexttype(f, 0);

    cp = _dxfdictname(f->d, value);
    (*gpp)->fname = DXAllocateLocal(strlen(cp) + 1);
    if (!(*gpp)->fname)
	return ERROR;
    strcpy((*gpp)->fname, cp);

    /* the next thing can be a comma and then another string or a number
     * or the keyword lines.
     */
    match_punct(f, COMMA);

    if (match_keyword(f, KW_LINES)) {
	(*gpp)->which = GETBY_LINES;
	/* line offset */
	if (!get_int(f, &(*gpp)->num)) {
	    DXSetError(ERROR_DATA_INVALID, "bad line offset");
	    return ERROR;
	}
	return OK;
	
    } else if (get_int(f, &(*gpp)->num)) {
	/* object number */
	(*gpp)->which = GETBY_NUM;  /* what it should be */
	(*gpp)->gbuf = DXAllocateLocal(2 * sizeof(int));
	(*gpp)->numlist = (int *)(*gpp)->gbuf;
	(*gpp)->numlist[0] = (*gpp)->num;
	(*gpp)->numlist[1] = -1;
	(*gpp)->which = GETBY_NUMLIST;
	return OK;

    } else if (get_ident(f, &value) || get_string(f, &value)) {
	/* object name */
	(*gpp)->which = GETBY_NAME;
	cp = _dxfdictname(f->d, value);
    /*  hasname: */
	(*gpp)->gbuf = DXAllocateLocal(2*sizeof(char *) + strlen(cp)+1);
	if (!(*gpp)->gbuf)
	    return ERROR;
	strcpy((char *)((*gpp)->gbuf)+(2*sizeof(char *)), cp);
	(*gpp)->namelist = (char **)(*gpp)->gbuf;
	(*gpp)->namelist[0] = (char *)((*gpp)->gbuf) + 2*sizeof(char *);
	(*gpp)->namelist[1] = NULL;
	return OK;
	
    }

    /* did we get here because there was just a filename?  see if it
     *  is part of the identifier, like:  filename,100
     */
    if (fnamewas == IDENTIFIER)
	;

    return OK;
    

#if 0  
    /* this is the code which parses a filename,offset out of a single
     *  double quoted string.  can i get rid of it now?
     */
    if (!input || (strlen(input) <= 0))
	goto error;
	
    *buf = (char *)DXAllocateLocal(strlen(input)+16);
    if (!*buf)
	goto error;

	
    cp = input;
    cp2 = *buf;
    while(*cp && *cp != ',')
	*cp2++ = *cp++;
    if (*cp == ',') {
	iscomma++;
	cp++;
    }
    *cp2 = '\0';
    cp2++;
 
    /* if this is the end of the string, there has been no comma and 
     * the next token isn't a comma, return here.
     */
    if ((*cp == NULL) && !iscomma && (get_punct(fp, NULL) != OK))
	return OK;

    /* this can be:   "file,num"     or   "file",num      or  "file", num
     *                "file","name"  or   "file", "name"  or  "file,name"
     *                 file,num      or    file, num      or   file , num
     *                "file"         or    file
     */

    /* all within one string? */
    if (*cp) {
	while (isspace(*cp))
	    cp++;
	
	if (isdigit(*cp)) {
	    gb->numlist[0] = atoi(cp);
	    gb->numlist[1] = -1;
            gb->which = GETBY_NUM;
	    goto lines;
	} else if (*cp == '"') {
	    gb->namelist[0] = cp2;
	    cp++;
	    while(*cp && *cp != '"')
		*cp2++ = *cp++;
	    *cp2 = '\0';
	    gb->namelist[1] = NULL;
            gb->which = GETBY_NAME;
	    goto lines;
	} else if (isalpha(*cp)) {
	    gb->namelist[0] = cp2;
            strcpy(gb->namelist[0], cp);
            gb->namelist[1] = NULL;
            gb->which = GETBY_NAME;
	    goto lines;
	}
    } 
    
    /* if there is something after the comma, parse it into a number
     *  or a string.  if there is a comma but nothing else, try parsing
     *  the next token to see if it's the offset.
     */
    if (get_punct(fp, NULL))
	iscomma++;

    /* if the string ended with a comma, try parsing the next token.
     *  if it's a number or a string, use that as the byte offset or
     *  object name to import from the remote file.
     */  
    if (_dxfmatchint(fp, &gb->numlist[0]) != ERROR) {
	gb->numlist[1] = -1;
	gb->which = GETBY_NUM;
	goto lines;
    }
    
    if (get_string(fp, &id) != ERROR) {
	gb->which = GETBY_NAME;
	gb->namelist[0] = _dxfdictname(fp->d, id);
	gb->namelist[1] = NULL;
	goto lines;
    }
    
    if (get_ident(fp, &id) != ERROR) {
	gb->which = GETBY_NAME;
	gb->namelist[0] = _dxfdictname(fp->d, id);
	gb->namelist[1] = NULL;
	goto lines;
    }
    
    DXSetError(ERROR_DATA_INVALID,
	     "no offset or name following comma");
    goto error;

#endif

}
 
 
/*
 * set data default modes 
 */
static Error parse_datadefs(struct finfo *f)
{
    int id;
    int rc = OK;
    int done = 0;

    while (!done) {
	rc = next_class(f, &id);
        if (!rc || (id != KEYWORD))
            break;
	
	next_id(f, &id);
        switch(id) {
	  case KW_MODE:
	    /* optional */
	    break;

	  case KW_BINARY:
	    /* default to ieee unless another format specified */
	    f->dformat = (f->dformat & ~D_FORMATS) | D_IEEE;
	    break;

	  case KW_MSB:
	    f->dformat = (f->dformat & ~D_BYTES) | D_MSB;
	    break;
	    
	  case KW_LSB:
	    f->dformat = (f->dformat & ~D_BYTES) | D_LSB;
	    break;
	    
	  case KW_IEEE:
	    f->dformat = (f->dformat & ~D_FORMATS) | D_IEEE;
	    break;
	    
	  case KW_XDR:
	    f->dformat = (f->dformat & ~D_FORMATS) | D_XDR;
	    break;
	    
	  case KW_ASCII:
	    f->dformat = (f->dformat & ~D_FORMATS) | D_TEXT;
	    break;
	    
	  default:
	    done++;
	    break;
	}

	if (!done)
	    skipkeyword(f);
    }

    return rc;
}


 
/*
 * set default objects to import
 */
static Error parse_default(struct finfo *f)
{
    struct getby *gp;
    int rc = OK;
    int objnum;
    int objcount = 0;

    gp = &(f->gb);

    /* if default already set, ignore rest of the "default xxx" line.
     * the object to import could have been passed in from the 
     * Import(file, default_object, "dx") call at the top level,
     * or the second way the object could already have been set is in
     * the two-pass section of the code where the default object will 
     * have been set on the first pass and should be ignored on the second.
     */
    if (gp->which == GETBY_NAME || gp->gbuf != NULL) {
	rc = _dxfskip_object(f);
	return rc;
    }

    /* parse the object id, only locals allowed here */
    rc = local_objectid(f, &objnum, "default object");
    if(!rc || objnum < 0)
	return ERROR;
    
#if DEBUG_MSG
    DXDebug("O", "setting default object %d", objnum);
#endif

    gp->gbuf = DXAllocateLocal(sizeof(int) * 2);
    if (!gp->gbuf)
	return ERROR;
    gp->numlist = (int *)gp->gbuf;

    gp->which = GETBY_ID;
    gp->numlist[objcount] = objnum;
    gp->numlist[++objcount] = -1;
    
    /* take a list here to match Import(file, list_of_objects, format...)
     *  module level interface.
     */
    while (1) {

	/* parse for more objects - not an error if no others */
	/* alloc commas here? */
	rc = local_objectid(f, &objnum, NULL);
	if(!rc || objnum < 0) {
	    DXResetError();
	    break;
	}
    
#if DEBUG_MSG
	DXDebug("O", "setting default object %d", objnum);
#endif
     
	gp->gbuf = DXReAllocate(gp->gbuf, sizeof(int) * (objcount+2));
	if (!gp->gbuf)
	    return ERROR;
	gp->numlist = (int *)gp->gbuf;

	gp->numlist[objcount] = objnum;
	gp->numlist[++objcount] = -1;

    }

    return OK;
}

 
 
#define CHUNK 16

/*
 * string or stringlist.
 *  a string is "abc"
 *  a stringlist is "xxx" "yyy" "zzz" ... until the next thing which 
 *    isn't a double quoted token.
 */
static Object make_string(struct finfo *f)
{
    int id, nid;
    Object o;
    char **clist = NULL;
    int nalloc = 0, nfilled = 0;
    Error rc = OK;
 
#if DEBUG_MSG
    DXDebug("E", "in make_string");
#endif
 
    rc = get_string(f, &id);
    if(!rc) 
	DXErrorReturn(ERROR_DATA_INVALID, "bad or missing string");
 
    rc = get_string(f, &nid);
    if (rc == OK) {
	while(rc == OK) {
	    if (!clist) {
		clist = (char **)DXAllocateLocal(CHUNK * sizeof(char *));
		if (!clist)
		    return ERROR;
 
		nalloc += CHUNK;
		clist[nfilled] = _dxfdictname(f->d, id);
		nfilled++;
	    }
	    
	    if (nfilled >= nalloc-1) {
		nalloc += CHUNK;
		clist = (char **)DXReAllocate((Pointer)clist, 
					    nalloc * sizeof(char *));
	    }
 
	    clist[nfilled] = _dxfdictname(f->d, nid);
	    nfilled++;
 
	    rc = get_string(f, &nid);
	}
 
	o = (Object)DXMakeStringListV(nfilled, clist);
    } else
	o = (Object)DXNewString(_dxfdictname(f->d, id));
 
    DXFree((Pointer)clist);
    return o;
}

 
/* make or skip this object?
 */
static Error make_obj(struct finfo *f)
{
    switch (f->activity) {
      case IDENTIFY_OBJS: 
	return ERROR;

      case MAKE_USED_OBJS:
	if (_dxfisobjused(f, f->curid))
	    return OK;

	return ERROR;

      case MAKE_ALL_OBJS:
	return OK;
    }

    return ERROR;
}
