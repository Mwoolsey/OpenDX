/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#include <dx/dx.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "config.h"
#include "pmodflags.h"
#include "utils.h"
#include "distp.h"
#include "parse.h"
#include "parsemdf.h"
#include "_macro.h"
#include "remote.h"
#include "loader.h"
#include "graph.h"
#include "function.h"

static Error do_mdf_load(struct moddef *mp);

/* this has to be in global memory for MP systems
 * since adding run-time modules has to happen on 
 * all processors 
 */
static struct moddef *newmod()
{
    struct moddef *mp;

    mp = DXAllocateZero (sizeof(struct moddef));
    if (!mp)
	return NULL;

    return mp;
}


static Error delmod(struct moddef *mp)
{
    struct modargs *map, *next;
    int i;

    if (!mp)
	return OK;

    if (mp->m_name)
	DXFree ((Pointer)mp->m_name);
    if (mp->m_exec)
	DXFree ((Pointer)mp->m_exec);
    if (mp->m_host)
	DXFree ((Pointer)mp->m_host);
    if (mp->m_loadfile)
	DXFree ((Pointer)mp->m_loadfile);

    /* follow the chains here and delete the char strings */
    for (i=0, map = mp->m_innames; i<mp->m_nin; i++, map=next) {
	if (!map)
	    break;

	next = map->nextarg;
	DXFree ((Pointer)map->thisarg);
	DXFree ((Pointer)map->deflt);
	DXFree ((Pointer)map);
    }
    for (i=0, map = mp->m_outnames; i<mp->m_nout; i++, map=next) {
	if (!map)
	    break;

	next = map->nextarg;
	DXFree ((Pointer)map->thisarg);
	DXFree ((Pointer)map->deflt);
	DXFree ((Pointer)map);
    }

    DXFree ((Pointer)mp);
    return OK;
}

#define INPUTARG 1
#define OUTPUTARG 2

static Error addarg(struct moddef *mp, int type, char *name, char *def)
{
    struct modargs *map, *newmap;

    if (!mp)
	return OK;

    newmap = (struct modargs *)DXAllocateZero (sizeof(struct modargs));
    if (!newmap)
	return ERROR;

    newmap->thisarg = name;
    newmap->deflt = def;

    if (type == INPUTARG) 
    {
	mp->m_nin++;
	if (!mp->m_innames) 
	{
	    mp->m_innames = newmap;
	    return OK;
	} 
	map = mp->m_innames;
    } 
    else 
    {
	mp->m_nout++;
	if (!mp->m_outnames) 
	{
	    mp->m_outnames = newmap;
	    return OK;
	}
	map = mp->m_outnames;
    }
	
    while (map->nextarg)
	map = map->nextarg;

    map->nextarg = newmap;
    return OK;
}


static Error argdup(struct moddef *mp, int type, int repcount)
{
    int i;
    int startrep;
    struct modargs *map;
    char *name, *value;

    if (type == INPUTARG) {
	startrep = mp->m_nin - repcount;
	map = mp->m_innames;
    } else {
	startrep = mp->m_nout - repcount;
	map = mp->m_outnames;
    } 
    if (startrep < 0) {
	DXSetError(ERROR_DATA_INVALID, "invalid repeat count");
	return ERROR;
    }
    
    for (i=0; i<startrep; i++, map=map->nextarg)
	;
    
    for (i=0; i<repcount*REPCOUNT; i++) {
	name = NULL;
	if (map->thisarg) {
	    name = DXAllocateZero (strlen(map->thisarg) + 4);
	    if (!name)
		return ERROR;
	    sprintf(name, "%s%d", map->thisarg, i);
	}
	
	value = NULL;
	if (map->deflt) {
	    value = DXAllocateZero (strlen(map->deflt) + 1);
	    if (!value)
		return ERROR;
	}

	if (!addarg(mp, type, name, value))
	    return ERROR;
    }

    return OK;
}



/*
 *  DXAddModuleV (name, func, flags, nin, inlist, nout, outlist, exec, host)
 *
 * struct moddef {
 *    char *		m_name;
 *    PFI		m_func;  
 *    int		m_flags;
 *    int		m_nin;
 *    struct modargs *	m_innames;
 *    int		m_nout;
 *    struct modargs *	m_outnames;
 *    char *		m_exec;
 *    char *		m_host;
 *    int		m_pflags;
 *    char *		m_loadfile;
 * }
 */

static Error
callmdf_remote(char *fname)
{
    if (! DXLoadAndRunObjFile(fname, "MODULES"))
	return ERROR;
    else
	return OK;
}

static Error callmdf(struct moddef *mp, int doremote)
{
    int rc;
    
    
    /* check the parms against the already defined module */
    if (mp->m_pflags == PF_RELOAD) {
	/* add code here */
	if (!(mp->m_pflags & PF_LOADABLE) && !(mp->m_pflags & PF_OUTBOARD)) {
	    /* set error code here?  we are going to ignore this defn */
	    DXMessage("ignoring redefinition of module %s", mp->m_name);
	    return OK;
	}
	/* fall through - it's ok to reload an outboard or runtime loadable */
    }
    
    if (mp->m_pflags & PF_LOADABLE) {
	/* on an MP machine, we have to load the code into the
         * running exec for each process.
	 */
	
        if (DXProcessors(0) > 1 && _dxd_exGoneMP) {
	    
#ifdef DXD_NO_MP_RUNTIME
	    if (_dxd_exGoneMP) {
		/* can't do runtime loadable after forking on sgi or solaris */
#define LONGHELP "Runtime-loadable modules cannot be added after startup when running with more than 1 processor.  Either specify -mdf on the startup command line, or run -processors 1"
		DXSetError(ERROR_DATA_INVALID, LONGHELP);
		return ERROR;
	    }
#endif
	    
	    if (_dxf_ExRunOnAll (callmdf_remote, mp->m_loadfile,
				 strlen(mp->m_loadfile)+1) == ERROR) {
		DXAddMessage("module %s", mp->m_name); 
		return ERROR;
	    }
	} else {
	    /* do the load for a single process machine, or MP machine
	     * before the fork.
	     */
	    if (!DXLoadAndRunObjFile(mp->m_loadfile, "DXMODULES")) {
		DXAddMessage("module %s", mp->m_name);
		return ERROR;
	    }
	}
	return OK;
    }
    
    if (mp->m_flags & MODULE_OUTBOARD)
	mp->m_func = DXOutboard;
    
    if (mp->m_func == NULL) {
	DXSetError(ERROR_DATA_INVALID, 
	      "module %s must have an OUTBOARD or LOADABLE entry to be added at run time",
	       mp->m_name);
	return ERROR;
    }
    
    /* mark this module as being loaded at run time instead of being
     * known at compile time.  i don't think any one cares, but just
     * in case it turns out to be useful...
     */
    mp->m_flags |= RUNTIMELOAD;
    
    
    /* the work happens here.
     */
    if (DXProcessors(0) == 1 || _dxd_exGoneMP == 0)
	rc = do_mdf_load(mp);
    else
	rc = _dxf_ExRunOn (1, do_mdf_load, (Pointer)mp, 0);

    
    /* if running distributed, tell the others about the new module.
     */ 
    if (doremote)
	_dxf_ExDistributeMsg(DM_LOADMDF, (Pointer)mp, 0, TOSLAVES);
    
    return rc;
}


static Error do_mdf_load(struct moddef *mp)
{
    int i;
    char **inlist = NULL, **outlist = NULL;
    struct modargs *map;
    Error rc;

    if (mp->m_nin) {
	inlist = (char **)DXAllocateZero(mp->m_nin * sizeof (char *));
	if (!inlist)
	    return ERROR;
    }
    if (mp->m_nout) {
	outlist = (char **)DXAllocateZero(mp->m_nout * sizeof (char *));
	if (!outlist) {
	    DXFree((Pointer)inlist);
	    return ERROR;
	}
    }

    for (i=0, map=mp->m_innames; i<mp->m_nin; i++, map=map->nextarg)
	inlist[i] = map->thisarg;

    for (i=0, map=mp->m_outnames; i<mp->m_nout; i++, map=map->nextarg)
	outlist[i] = map->thisarg;


    /* this loads the executive jump table.
     */
    rc = DXAddModuleV(mp->m_name, mp->m_func, mp->m_flags,
		      mp->m_nin, inlist,
		      mp->m_nout, outlist,
		      mp->m_exec, mp->m_host);


    /* we may have some graph with these functions built in already.
     * this forces us to rebuild all graphs with the new mdf defintion.
     */
    if ((node *)_dxf_ExMacroSearch (mp->m_name) != NULL)
        _dxf_ExDictionaryPurge (_dxd_exGraphCache);


    DXFree((Pointer)inlist);
    DXFree((Pointer)outlist);
    
    return rc;
}


/* communicating with remote execs for distributed.
 */

void _dxf_ExSendMdfPkg(Pointer *data, int tofd)
{
    int i, len;
    struct moddef *mp;
    struct modargs *map;

    mp = (struct moddef *)data;

    len = (mp->m_name) ? strlen(mp->m_name) : 0;
    _dxf_ExWriteSock(tofd, &len, sizeof(int));
    if (len > 0)
	_dxf_ExWriteSock(tofd, mp->m_name, len);
    _dxf_ExWriteSock(tofd, &mp->m_flags, sizeof(int));

    _dxf_ExWriteSock(tofd, &mp->m_nin, sizeof(int));
    map = mp->m_innames;
    for (i=0, map=mp->m_innames; i<mp->m_nin; i++, map=map->nextarg) {
	len = (map->thisarg) ? strlen(map->thisarg) : 0;
	_dxf_ExWriteSock(tofd, &len, sizeof(int));
	if (len > 0)
	    _dxf_ExWriteSock(tofd, map->thisarg, len);
	/* i'm NOT writing the default value here because they are not
	 * supported yet.  this will have to be added sometime.
	 */
    }

    _dxf_ExWriteSock(tofd, &mp->m_nout, sizeof(int));
    map = mp->m_outnames;
    for (i=0, map=mp->m_outnames; i<mp->m_nout; i++, map=map->nextarg) {
	len = (map->thisarg) ? strlen(map->thisarg) : 0;
	_dxf_ExWriteSock(tofd, &len, sizeof(int));
	if (len > 0)
	    _dxf_ExWriteSock(tofd, map->thisarg, len);
	/* i'm NOT writing the default value here because they are not
	 * supported yet.  this will have to be added sometime.
	 */
    }

    len = (mp->m_exec) ? strlen(mp->m_exec) : 0;
    _dxf_ExWriteSock(tofd, &len, sizeof(int));
    if (len > 0)
	_dxf_ExWriteSock(tofd, mp->m_exec, len);
    len = (mp->m_host) ? strlen(mp->m_host) : 0;
    _dxf_ExWriteSock(tofd, &len, sizeof(int));
    if (len > 0)
	_dxf_ExWriteSock(tofd, mp->m_host, len);

    _dxf_ExWriteSock(tofd, &mp->m_pflags, sizeof(int));
    len = (mp->m_loadfile) ? strlen(mp->m_loadfile) : 0;
    _dxf_ExWriteSock(tofd, &len, sizeof(int));
    if (len > 0)
	_dxf_ExWriteSock(tofd, mp->m_loadfile, len);

    return;
}


Error _dxf_ExRecvMdfPkg(int fromfd, int swap)
{
    int i, len, num;
    struct moddef *mp;
    char *argname;


    if ((mp = newmod()) == NULL)
	return ERROR;

    _dxf_ExReceiveBuffer(fromfd, &len, 1, TYPE_INT, swap);
    if (len > 0) {
	mp->m_name = DXAllocateZero(len+1);
	_dxf_ExReceiveBuffer(fromfd, mp->m_name, len, TYPE_UBYTE, swap);
    }
    _dxf_ExReceiveBuffer(fromfd, &mp->m_flags, 1, TYPE_INT, swap);

    _dxf_ExReceiveBuffer(fromfd, &num, 1, TYPE_INT, swap);
    for (i=0; i<num; i++) {
	_dxf_ExReceiveBuffer(fromfd, &len, 1, TYPE_INT, swap);
	if (len > 0) {
	    argname = DXAllocateZero(len+1);
	    _dxf_ExReceiveBuffer(fromfd, argname, len, TYPE_UBYTE, swap);
	    addarg(mp, INPUTARG, argname, NULL);
	}
    }

    _dxf_ExReceiveBuffer(fromfd, &num, 1, TYPE_INT, swap);
    for (i=0; i<num; i++) {
	_dxf_ExReceiveBuffer(fromfd, &len, 1, TYPE_INT, swap);
	if (len > 0) {
	    argname = DXAllocateZero(len+1);
	    _dxf_ExReceiveBuffer(fromfd, argname, len, TYPE_UBYTE, swap);
	    addarg(mp, OUTPUTARG, argname, NULL);
	}
    }

    _dxf_ExReceiveBuffer(fromfd, &len, 1, TYPE_INT, swap);
    if (len > 0) {
	mp->m_exec = DXAllocateZero(len+1);
	_dxf_ExReceiveBuffer(fromfd, mp->m_exec, len, TYPE_UBYTE, swap);
    }

    _dxf_ExReceiveBuffer(fromfd, &len, 1, TYPE_INT, swap);
    if (len > 0) {
	mp->m_host = DXAllocateZero(len+1);
	_dxf_ExReceiveBuffer(fromfd, mp->m_host, len, TYPE_UBYTE, swap);
    }

    _dxf_ExReceiveBuffer(fromfd, &mp->m_pflags, 1, TYPE_INT, swap);
    _dxf_ExReceiveBuffer(fromfd, &len, 1, TYPE_INT, swap);
    if (len > 0) {
	mp->m_loadfile = DXAllocateZero(len+1);
	_dxf_ExReceiveBuffer(fromfd, mp->m_loadfile, len, TYPE_UBYTE, swap);
    }


    if (!callmdf(mp, 0))
	goto error;

    delmod(mp);
    return OK;

  error:
    if (mp)
	delmod(mp);
    return ERROR;
}




/*
 * utility type routines.
 */

/* return pointer to start of next line.  if flag is set, assume you are
 *  in the middle of a line and always search forward.  if flag is zero,
 *  assume you could already be at the start of the line and only space
 *  forward if the line is empty or a comment.
 */
static char *nextline(char *str, int forward, int *lineno)
{
    char *cp = str;

    if (!cp)
	return NULL;

  again:
    (*lineno)++;
    
    if (!forward) {
	if (*cp != '#' && *cp != '\n' && *cp != '\r')
	    return cp;
	
	forward = 1;
    }

    while (*cp != '\0' && *cp != '\n' && *cp != '\r')
	cp++;
    
    if (*cp == '\0')
	return NULL;
	
    if (*cp == '\n' || *cp == '\r')
	{
	    while (*cp == '\n' || *cp == '\r')
	        cp ++;
	
	    forward = 0;
	    goto again;
    }

    if (*cp == '#')
	goto again;

    return cp;
}


/* return a pointer to the next char which matches.  this is different
 *  than strindex or strstr because it won't space past a newline.
 *  return NULL if there isn't a match before the next newline or 
 *  end of file (actually, end of string).
 */
static char *find_next(char *str, char find)
{
    char *cp = str;

    if (cp == NULL)
	return NULL;

    while (*cp != '\0' && *cp != '\n' && *cp != find)
	cp++;

    return (*cp == find) ? cp : NULL;
}

static char *end_of_line(char *str)
{
    return find_next(str, '\n');
}

/*
 * return start of next whitespace separated token.  return NULL if
 * end-of-line or end-of-string encountered first.  if semi is set,
 * allow semicolons to be considered as whitespace.
 */
static char *find_next_token(char *str, int semi)
{
    char *cp = str;

    if (cp == NULL)
	return NULL;

    /* find whitespace (or semi)
     */
    if (semi)
	while (*cp != '\0' && *cp != '\n' &&  
	       *cp != ' '  && *cp != '\t' &&
	       *cp != '\r' && *cp != ';')
	    cp++;
    else
	while (*cp != '\0' && *cp != '\n' &&  
	       *cp != ' ' && *cp != '\t' &&
	       *cp != '\r')
	    cp++;

	if (*cp == '\r')
		cp++;
    
    if (*cp == '\0' || *cp == '\n')
	return NULL;
    
    /* now find non-whitespace 
     */
    if (semi && *cp == ';')
	cp++;
    
    while (*cp != '\0' && *cp != '\n' && 
		*cp != '\r' && (*cp == ' ' || *cp == '\t'))
	cp++;

    if (*cp == '\r')
		cp++;
    
    if (*cp == '\0' || *cp == '\n')
	return NULL;

    return cp;
}

/* return the end of the current string of chars, delimited by either
 * end-of-line, end-of-string, whitespace or semicolon.
 */
static char *find_token_end(char *str)
{
    char *cp = str;

    if (cp == NULL)
	return NULL;

    while (*cp != '\0' && *cp != '\n' && 
	   *cp != ' '  && *cp != '\t' && 
	   *cp != '\r' && *cp != ';')
	cp++;

    return cp;
}

/* names have to start with an alpha, and then
 * contain only alphas or digits
 */
static int IsGoodIdentifier(char *name)
{
    char *cp = name;

    if (!isalpha(*cp++))
	return FALSE;
    
    while (*cp) {
	if (!isalnum(*cp))
	    return FALSE;
	cp++;
    }
    
    return TRUE;
}

/* allocate space for and make a copy of the next string up to the
 * next ';', ' ' or end of line
 */
static char *AllocToken(char *str)
{
    char *newbuf;
    char *strend;
    
    strend = find_token_end(str);

    newbuf = DXAllocate(strend - str + 1);
    if (!newbuf)
	return NULL;

    strncpy(newbuf, str, strend-str);
    newbuf[strend-str] = '\0';

    return newbuf;
}

/* allocate space for and make a copy of the next string up to the
 * next '"'
 */
static char *AllocQuote(char *str)
{
    char *newbuf;
    char *strend;
    char *last;
    
    last = end_of_line(str);
    strend = find_next(str, '"');
    if (!strend || (last && strend > last)) {
	strend = last;
	if (!strend) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "missing name or mismatched quotes");
	    return NULL;
	}
    }
    
    newbuf = DXAllocate(strend - str + 1);
    if (!newbuf)
	return NULL;

    strncpy(newbuf, str, strend-str);
    newbuf[strend-str] = '\0';

    return newbuf;
}

/* allocate space for and make a copy of the next string up to the
 * next ';' or end of line, including looking for attributes and squeezing
 * any intervening spaces from them.  (should this be necessary?
 * why doesn't the exec code handle spaces?)
 */
static char *AllocVariable(char *str)
{
    char *newbuf;
    char *strend;
    char *cp, *dest;
    
    strend = find_token_end(str);
    if (!strend)
	return NULL;
    
    cp = strend;
    while (*cp == ' ')
	cp++;
    if(*cp  == '[') {
        strend = strchr(cp, ']');
        if(!strend)
            return NULL;
        strend++;
    }

    newbuf = DXAllocate(strend - str + 1);
    if (!newbuf)
	return NULL;
    
    for (cp = str, dest = newbuf; cp < strend; cp++)
	if (!isspace(*cp))
	    *dest++ = *cp;
    
    *dest = '\0';

    return newbuf;
}

static int IDKeyword(char *str)
{
    if (!str || str[0] == '\0')
	return T_NONE;
    
    if (!strncmp(str, "MODULE", sizeof("MODULE")-1))
	return T_MODULE;
    
    if (!strncmp(str, "CATEGORY", sizeof("CATEGORY")-1))
	return T_CATEGORY;
    
    if (!strncmp(str, "DESCRIPTION", sizeof("DESCRIPTION")-1))
	return T_DESCRIPTION;
    
    if (!strncmp(str, "INPUT", sizeof("INPUT")-1))
	return T_INPUT;
    
    if (!strncmp(str, "REPEAT", sizeof("REPEAT")-1))
	return T_REPEAT;
    
    if (!strncmp(str, "OUTPUT", sizeof("OUTPUT")-1))
	return T_OUTPUT;
    
    if (!strncmp(str, "OPTIONS", sizeof("OPTIONS")-1))
	return T_OPTIONS;
    
    if (!strncmp(str, "FLAGS", sizeof("FLAGS")-1))
	return T_FLAGS;
    
    if (!strncmp(str, "OUTBOARD", sizeof("OUTBOARD")-1))
	return T_OUTBOARD;
    
    if (!strncmp(str, "LOADABLE", sizeof("LOADABLE")-1))
	return T_LOADABLE;
    
    return T_ERROR;
}

static int IDFlag(char *str)
{
    if (!str || str[0] == '\0')
	return 0;

#if 0
    if (!strncmp(str, "SERIAL", sizeof("SERIAL")-1))
	return MODULE_SERIAL;

    if (!strncmp(str, "SEQUENCED", sizeof("SEQUENCED")-1))
	return MODULE_SEQUENCED;
#endif

    if (!strncmp(str, "PIN", sizeof("PIN")-1))
	return MODULE_PIN;

#if 0
    if (!strncmp(str, "ASSIGN", sizeof("ASSIGN")-1))
	return MODULE_ASSIGN;
#endif

    if (!strncmp(str, "SIDE_EFFECT", sizeof("SIDE_EFFECT")-1))
	return MODULE_SIDE_EFFECT;

    if (!strncmp(str, "LOOP", sizeof("LOOP")-1))
        return MODULE_LOOP;

#if 0
    if (!strncmp(str, "JOIN", sizeof("JOIN")-1))
	return MODULE_JOIN;
#endif

    if (!strncmp(str, "ERR_CONT", sizeof("ERR_CONT")-1))
	return MODULE_ERR_CONT;

    if (!strncmp(str, "REROUTABLE", sizeof("REROUTABLE")-1))
	return MODULE_REROUTABLE;

    if (!strncmp(str, "REACH", sizeof("REACH")-1))
	return MODULE_REACH;

    if (!strncmp(str, "OUTBOARD", sizeof("OUTBOARD")-1))
	return MODULE_OUTBOARD;

    if (!strncmp(str, "PERSISTENT", sizeof("PERSISTENT")-1))
	return MODULE_PERSISTENT;

    if (!strncmp(str, "ASYNC", sizeof("ASYNC")-1))
	return MODULE_ASYNC;
    
    if (!strncmp(str, "ASYNCHRONOUS", sizeof("ASYNCHRONOUS")-1))
	return MODULE_ASYNC;
    
    if (!strncmp(str, "ASYNCLOCAL", sizeof("ASYNCLOCAL")-1))
	return MODULE_ASYNCLOCAL;
    
    return 0;
}

/*
 * parse the string into lines, and call DXAddModuleV for each new
 * module definition.
 */
static Error ExParseMDF(char *str)
{
    int lineno = 0;
    int modflag;
    int repcount;
    int id;
    int argtype = -1;
    char *tempc;
    char *nextc;
    struct moddef *mp = NULL;


    /* start at the first non-comment line in the string */
    nextc = nextline(str, 0, &lineno);
    
    while ((id = IDKeyword(nextc)) != T_NONE) {
	switch (id) {
	  case T_MODULE:
	    /* output previous module definition */
	    if (mp) {
		
		if (!callmdf(mp, 1))
		    goto error;

		delmod(mp);
	    }
	    /* start new definition */
	    mp = newmod();
	    if (!mp)
		goto error;
	    
	    nextc = find_next_token(nextc, 0);
	    if (!nextc) {
		DXSetError(ERROR_DATA_INVALID, 
			   "missing module name, line %d", lineno);
		goto error;
	    }
	    mp->m_name = AllocToken(nextc);
	    if (!IsGoodIdentifier(mp->m_name)) {
		DXSetError(ERROR_DATA_INVALID, 
			   "module names must start with a letter and contain "
			   "only letters and numbers, line %d", lineno);
		goto error;
	    }
	    
#if 0  /* reload == 0, so that's the default */
	    /* does it already exist? */
            if ((node *)_dxf_ExMacroSearch (mp->m_name) != NULL)
		mp->m_pflags |= PF_RELOAD;
#endif

	    break;
	    
	  /* get name only; ignore type, default & description. */
	  case T_INPUT:
	  case T_OUTPUT:
	    nextc = find_next_token(nextc, 1);
	    if (!nextc) {
		DXSetError(ERROR_DATA_INVALID, 
			   "missing %s parameter name, line %d", 
			   (id==T_INPUT) ? "input" : "output", lineno);
		goto error;
	    }
	    
	    tempc = AllocVariable(nextc);
	    if (!tempc)
		goto error;
	    
	    if (!addarg(mp, (id==T_INPUT) ? INPUTARG : OUTPUTARG, tempc, NULL))
		goto error;

	    /* save this for later, if handling repeats */
	    argtype = id;
	    break;
	    
	  case T_ERROR:
	    /* could this be more helpful? */
	    DXSetError(ERROR_DATA_INVALID, 
		       "unrecognized keyword on line %d", lineno);
	    return ERROR;
	    
	  case T_OUTBOARD:
	    /* get the exec and host name */
	    tempc = find_next(nextc, '"');
	    if (!tempc) {
		nextc = find_next_token(nextc, 1);
		if (!nextc) {
		    DXSetError(ERROR_DATA_INVALID, 
			       "missing outboard execution name, line %d", 
			       lineno);
		    goto error;
		}
		mp->m_exec = AllocToken(nextc);
	    } 
	    else
		mp->m_exec = AllocQuote(++tempc);

	    if (!mp->m_exec) {
		DXSetError(ERROR_DATA_INVALID, 
			   "missing outboard execution name, line %d", 
			   lineno);
		goto error;
	    }

	    tempc = find_next(nextc, ';');
	    if (tempc) {
		nextc = tempc;
		tempc = find_next_token(nextc, 1);
		if (tempc) {
		    mp->m_host = AllocToken(tempc);
		    nextc = tempc;
		}
	    }

	    mp->m_flags |= MODULE_OUTBOARD;
	    mp->m_pflags |= PF_OUTBOARD;
	    break;
	    
	  case T_LOADABLE:
	    /* get the executable filename */
	    tempc = find_next(nextc, '"');
	    if (!tempc) {
		nextc = find_next_token(nextc, 1);
		if (!nextc) {
		    DXSetError(ERROR_DATA_INVALID, 
			       "missing loadable module filename, line %d", 
			       lineno);
		    goto error;
		}
		mp->m_loadfile = AllocToken(nextc);
	    } 
	    else
		mp->m_loadfile = AllocQuote(++tempc);

	    if (!mp->m_loadfile) {
		DXSetError(ERROR_DATA_INVALID, 
			   "missing loadable module filename, line %d", 
			   lineno);
		goto error;
	    }

	    mp->m_pflags |= PF_LOADABLE;
	    break;
	    
	  case T_FLAGS:
	    tempc = find_next_token(nextc, 0);
	    while ((modflag = IDFlag(tempc)) != F_NONE) {
		mp->m_flags |= modflag;
		nextc = tempc;
		tempc = find_next_token(tempc, 0);
	    }
	    break;

	  case T_REPEAT:
	    nextc = find_next_token(nextc, 0);
	    if (!nextc) {
		DXSetError(ERROR_DATA_INVALID, 
			   "missing repeat count, line %d", lineno);
		goto error;
	    }
	    repcount = atoi(nextc);
	    if (argtype == T_INPUT) {
		if (repcount <= 0 || repcount > mp->m_nin) {
		    DXSetError(ERROR_DATA_INVALID,
			       "invalid input repeat count, line %d", lineno);
		    goto error;
		}
		if (!argdup(mp, INPUTARG, repcount)) {
		    DXAddMessage("line %d", lineno);
		    goto error;
		}

	    } else if (argtype == T_OUTPUT) {
		if (repcount <= 0 || repcount > mp->m_nout) {
		    DXSetError(ERROR_DATA_INVALID,
			       "invalid output repeat count, line %d", lineno);
		    goto error;
		}
		if (!argdup(mp, OUTPUTARG, repcount)) {
		    DXAddMessage("line %d", lineno);
		    goto error;
		}
		
	    } else {
		DXSetError(ERROR_DATA_INVALID,
			   "misplaced REPEAT line, line %d", lineno);
	    }
	    break;

	  case T_CATEGORY:
	  case T_DESCRIPTION:
	  case T_OPTIONS:
	    /* ignore rest of line */
	    break;
	}

	nextc = nextline(nextc, 1, &lineno);
    }

    /* at end-of-string, finish current definition if there is one */
    if (mp) {
	if (!callmdf(mp, 1))
	    goto error;
	
	delmod(mp);
    }
    
    return OK;

  error:
    if (mp)
	delmod(mp);

    return ERROR;
}

/* if file, read into memory and then parse the same way a string obj
 * would be parsed.
 */
Error DXLoadMDFFile(char *filename)
{
    int fd;
    int len, rlen;
    int rc = ERROR;
    char *foundname = NULL;
    char *cp = NULL;
    
    if (_dxf_fileSearch(filename, &foundname, "mdf", "DXMODULES") == ERROR)
	return ERROR;

    fd = open(foundname, O_RDONLY);
    if (fd < 0) {
	DXSetError(ERROR_DATA_INVALID, "cannot open %s as MDF file", foundname);
	goto error;
    }

    len = lseek(fd, 0, 2);   /* find length of file */
    if (len <= 0) {
	DXSetError(ERROR_DATA_INVALID, "error reading from %s", foundname);
	goto error;
    }
    lseek(fd, 0, 0);

    cp = (char *)DXAllocate(len+1);
    if (!cp)
	goto error;

    rlen = read(fd, cp, len);
    if (rlen != len) {
	DXSetError(ERROR_DATA_INVALID, "error reading from %s", foundname);
	goto error;
    }
    cp[len] = '\0';

    close (fd);

    rc = ExParseMDF(cp);

  error:
    DXFree((Pointer)cp);
    DXFree((Pointer)foundname);
    return rc;
}

Error DXLoadMDFString(char *cp)
{
    return ExParseMDF(cp);
}

