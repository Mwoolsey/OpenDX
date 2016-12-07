/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "loader.h"

/*
 * system dependent code:  dynamically load or unload an executable file.
 *   this is used for adding modules or functions to an already 
 *   running executive.  the new code is compiled into a separate executable
 *   with a single entry point.
 *
 * the three functions which are available in this file are:
 *   load a file and call the entry point, which must be the routine DXEnter
 *   load a file and return the entry point DXEnter without executing it
 *   unload a previously loaded file
 *
 * a fourth function may be necessary - reload an already loaded file
 *   which would imply unloading it first and then loading it again.
 *   this might be more useful for development than for normal running.
 *   where the alternative is to disconnect and reconnect to server.
 *
 * each system does this differently, and some don't support the function
 *   at all, so the default case is to return error.
 *
 * look carefully at this file - if you add an architecture you have
 *   to change two places.  the include files and handles you use to id
 *   the loaded segments vary by system, and then the actual code is
 *   also system dependent.
 */

#include <dx/dx.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(HAVE_SYS_ERRNO_H)
#include <sys/errno.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#include <sys/stat.h> 

#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
#define DX_DIR_SEPARATOR ';'
#else
#define DX_DIR_SEPARATOR ':'
#endif

/* include files and definition of the 'handle' used to identify
 * the loaded segment.
 */

#if defined(hp700)

# include <dl.h>
  typedef shl_t Handle;
# define __HANDLE_DEF

#endif /* hp700 */

#if !defined(hp700) && !defined(intelnt) && !defined(WIN32)

#include <dlfcn.h>
typedef void *Handle;
#define __HANDLE_DEF

/* for some reason, the prototypes in the header file are for _dlXXX and not
 * dlXXX when __STDC__ is defined.  i don't understand, but i'll play along.
 */
#if defined(aviion)
#define dlopen  _dlopen
#define dlsym   _dlsym
#define dlclose _dlclose
#define dlerror _dlerror
#endif


#endif /* sun4 or solaris or sgi or alpha or aviion of linux */


/* default for unsupported platforms */
#ifndef __HANDLE_DEF

typedef int Handle;

#endif


#ifdef DXD_WIN
#define EntryPt	FARPROC
#else
typedef int (*EntryPt)();
#endif

/* current load state */
typedef enum ls {
    L_UNKNOWN,
    L_LOADED,
    L_UNLOADED,
    L_RELOADED,
    L_ERROR
} Ls;

struct loadtable {
    Ls loadstate;
    char *filename;
    Handle h;
    EntryPt func;
    struct loadtable *next;
};

#define ALREADY_LOADED ((EntryPt)(-1))

static struct loadtable **ltp = NULL;
static lock_type *ltl;

Error _dxf_initloader()
{
    if (!ltp) {
	ltp = (struct loadtable **)DXAllocateZero(DXProcessors(0) * 
						  sizeof(struct loadtable *));
	if (!ltp)
	    return ERROR;

        ltl = (lock_type *)DXAllocate(sizeof(lock_type));
	if (!ltl)
	    return ERROR;

	if (DXcreate_lock(ltl, "loader") != OK)
	    return ERROR;

    }
    return OK;
}
    
/* put these in a header file - should they be public DXxxx() functions
 *  (then the prototypes go in <dx/dx.h>), or should they be semiprivate
 *  _dxfEx() function (then the prototypes go in one of the exec headers)?
 *
 * the basic question is would a user write something which used these
 * or are they much too low level?
 */

/*
 * loadobjfile should return a code saying whether it did anything or
 * whether the obj file was already loaded.  if it was loaded, do you
 * really want to call the run part again?  right now you can't tell.
 * that would mean making the function one of the parms set and returned
 * by the func, and yes/no/already done would be the return value.
 * or ok/error would be the return code, &func would be set and &already done
 * would also be a parm set and returned.
 */

static struct loadtable *get_tp_entry(char *filename);
static struct loadtable *set_tp_entry(Ls loadstate, 
					char *filename, Handle h, EntryPt func);
static Error findfile(char *inname, char **outname, char *envvar);


#if 0 /* ibm6000 */

AIX 4.2 and greater now supports the dl functions. All the specific ibm6000
loader code is no longer used. How many people are still running AIX 4.1 or 
lower? Pull an older version of this file from cvs if the older loader code
is needed. The notes on compilation below need updated.

/* 
 * to make a dynamically loaded module, you have to link with these flags:
 *
 *  -qmkshrobj       construct a shared object with C++
 *
 *  -bI:dxexec.imp   this lists the symbols in the exec which your module
 *                   will be able to call at runtime - like all the libdx
 *                   functions.  otherwise, they will be undefined when 
 *                   trying to link this module independently of the exec.
 *
 *  -G               same as calling -berok -brtl -bnortllib -bsymbolic 
 *                   -bnoautoexp -bM:SRE. -berok is definitely needed to
 *                   ignore the dxexec undefined symbols and -bM:SER for
 *                   a reentrant library.
 *
 * the exec doesn't have to be linked with any special flags, except to export
 *  its sybols (see below) because it isn't expecting to call any of the 
 *  subroutines defined in these modules directly - it is only going to call
 *  the single entry point, and it makes that call by address and not by name.
 *  It must export its symbol table so that loaded modules will be able to 
 *  access dx routines from within the exec.
 *
 *  -bE:dxexec.exp
 *
 * if we wanted the exec to be able to call directly into the dynamically
 *  loaded module code by subroutine names, then at link time the exec would
 *  need either an .exp file or the module object code itself on the link line,
 *  and the module would need to have been linked with -bE:xxx.exp which
 *  contained a list of symbol names to be made public.
 */

#endif  /* ibm6000 */


#if 0   /* comment */

ok the other other plan:
we expand the filename with DXMODULES as a search path and use
stat until we find a file (any file or an executable file?)

then we look thru the linked list and see if we have already loaded
that file.  if so, we say ok and return, unless it is a reload command
in which case we go ahead and first unload it and then load it.

if it was not originally loaded, we put it in the linked list with
the state and the full pathname we found it under and whatever we
need to save in order to be able to unload it (that last part is
system dependent).

if we are unloading a file, find it in the linked list, unload
it and then either leave the block in the list but marked unloaded
or remove it from the list.  i do not know if it would help for the
next time.  might if we are planning to unload and reload often.

are there limits to the number of loadable segments?  i cannot find
anything in the man pages about that.  run a test, i suppose.


ah, and now the MP questions.  if we load the segment before we
fork, does it stay with the process space on fork?  and afterwards,
how do we get it loaded in each segment?  does it load at exactly
the same place in each image?  or does this table have to be on a
per processor basis?  then that blows the single entry point in the
function table.  for MP systems and loadable modules, maybe they
all have to be specified on the command line, and the init routine
has to load them all before the first fork.  


apparently on the sgi you are going to have to load it into each
executable, at least after the fork.  i got errors running MP but
it worked fine UP.

#endif  /* comment */


#if hp700

#define __ROUTINES_DEF

/*
 * to make a dynamically loaded module, you have to link with these flags:
 *
 * these make it shareable, which seems to be what we want given the
 * comments in the ld(1) man page about the overhead with non-shared
 * but loadable files (see the comments in the DEPENDENCIES section
 * of the ld(1) man page, in the series 700/800 section about the -A option.
 *
 * -ldld  something about dynamic load library?
 *
 * +z     compile-time option to make the code position independent
 * -b     to make the object sharable
 *
 * -e entry_point  to set the entry point, which MUST be DXEntry for this
 *                 architecture, because it doesn't look like there is a
 *                 way to ask for the entry point by address, only by
 *                 symbol name.
 * 
 * -q      mark the file demand loadable
 *
 * generally useful option - should we add this to the default cflags
 * for all our builds?
 * -z       make null pointer references generate a seg fault
 *
 * generates extra overhead because the new text is loaded into
 * a read/write segment instead of the initial read-only text segement.
 * -A name  incremental loading (incompatible with -b?) 
 *
 * -B type  set the binding - load dynamic libs at startup time or at
 *          time of first reference
 *
 * -E       mark all symbols as exported to shared libs.  the man page
 *          says there is an option to only mark those which are used.
 *          (how does it know?)
 *
 * NOT SUPPORTED in the 700/800 version (300/400 only)
 * -V num   set a decimal version stamp for ident purposes
 *
 * -c filename read options from an indirect file
 *             each line contains options separated by white space
 *             # is the comment character, ## is a real hash char
 *
 * +e symbol  export this symbol
 *
 * +E symbol  set the elaborator function ??
 *
 * +I symbol  set the initializer function ??
 *
 *  
 */




PFI DXLoadObjFile(char *fname, char *envvar)
{
    Handle handle;
    int (*func)();
    char *foundname = NULL;
    struct loadtable *lp = NULL;

    /* look at DXMODULES here and if you can't find the file and it
     * doesn't start with '/', start prepending dirnames until you find it
     * or you get to the end of the path.
     */
    DXDebug("L", "trying to load `%s'", fname);
    if (!findfile(fname, &foundname, envvar))
        return NULL;

    DXDebug("L", "found at `%s'", foundname);
    if (((lp = get_tp_entry(foundname)) != NULL) && 
	 (lp->loadstate == L_LOADED)) {
	DXDebug("L", "`%s' was already loaded", foundname);
	DXFree((Pointer)foundname);
	return ALREADY_LOADED;
    }

    /* args are:
     *  filename, options, address.   they recommend 0 for the address
     *  (tells the system gets to pick a load address), and the flags 
     *  have to have one of BIND_IMMEDIATE or BIND_DEFERRED (what to do
     *  with symbols - resolve them all now, or resolve them when they
     *  are referenced).
     */
    handle = shl_load(foundname, BIND_IMMEDIATE, 0L);
    if (handle == NULL) {
	DXSetError(ERROR_DATA_INVALID, "cannot load file '%s'", foundname);
	DXFree((Pointer)foundname);
	return NULL;
    }

    if (shl_findsym(&handle, "DXEntry", TYPE_PROCEDURE, (long *)&func) < 0) {
	DXSetError(ERROR_DATA_INVALID, 
		   "cannot find entry point 'DXEntry' in file '%s'", foundname);
	DXFree((Pointer)foundname);
	return NULL;
    }

    if (!set_tp_entry(L_LOADED, foundname, handle, func)) {
	DXFree((Pointer)foundname);
	return NULL;
    }

    DXFree((Pointer)foundname);
    return func;
}

Error DXLoadAndRunObjFile(char *fname, char *envvar)
{
    int (*func)();
    
    func = DXLoadObjFile(fname, envvar);
    if (func == NULL)
	return ERROR;
    if (func == ALREADY_LOADED)
	return OK;

    if (DXQueryDebug("L"))
        DXEnableDebug("F", 1);
	
    (*func)();
    
    if (DXQueryDebug("L"))
        DXEnableDebug("F", 1);
	
    if (DXGetError() != ERROR_NONE)
	return ERROR;

    return OK;
}


Error DXUnloadObjFile(char *fname, char *envvar)
{
    char *foundname = NULL;
    struct loadtable *lp = NULL;

    if (!findfile(fname, &foundname, envvar))
	return ERROR;
    
    if (((lp = get_tp_entry(foundname)) != NULL) && 
	(lp->loadstate == L_UNLOADED)) {
	DXDebug("L", "`%s' found but already unloaded", foundname);
	DXFree((Pointer)foundname);
	return OK;
    }
    
    if (!lp) {
	DXDebug("L", "`%s' not found in load list", foundname);
	DXSetError(ERROR_BAD_PARAMETER, 
		   "cannot unload `%s'; not found in load list", foundname);
	DXFree((Pointer)foundname);
	return OK;
    }

    if (shl_unload(lp->h) < 0) {
	DXSetError(ERROR_DATA_INVALID, "cannot unload file '%s', %s", 
		   foundname, strerror(errno));
	DXFree((Pointer)foundname);
	return ERROR;
    }
    
    if (!set_tp_entry(L_UNLOADED, foundname, (Handle)0, (EntryPt)0)) {
	DXFree((Pointer)foundname);
	return ERROR;
    }
    
    DXFree((Pointer)foundname);
    return OK;
}


#endif  /* hp700 */


#if !defined(hp700) && !defined(intelnt) && !defined(WIN32)

#define __ROUTINES_DEF

/*
 * to make a dynamically loaded module, you have to link with these flags:
 *
 * < lots more stuff here >
 *
 * -Bdynamic
 *
 * -e DXEntry ??
 *  
 */



PFI DXLoadObjFile(char *fname, char *envvar)
{
    Handle handle;
    int (*func)();
    char *foundname = NULL;
    struct loadtable *lp = NULL;

    /* look at DXMODULES here and if you can't find the file and it
     * doesn't start with '/', start prepending dirnames until you find it
     * or you get to the end of the path.
     */
    DXDebug("L", "trying to load `%s'", fname);
    if (!findfile(fname, &foundname, envvar))
        return NULL;

    DXDebug("L", "found at `%s'", foundname);
    if (((lp = get_tp_entry(foundname)) != NULL) && 
	 (lp->loadstate == L_LOADED)) {
	DXDebug("L", "`%s' was already loaded", foundname);
	DXFree((Pointer)foundname);
	return ALREADY_LOADED;
    }

    /* args are:
     *  filename, mode.  the mode is: RTLD_LAZY (defer binding of procs)
     *  other vals are reserved for future expansion.
     */
    handle = dlopen(foundname, RTLD_LAZY|RTLD_GLOBAL);
    if (handle == NULL) {
	char *cp = dlerror();
	if (cp)
	    DXSetError(ERROR_DATA_INVALID, "cannot load file '%s', %s", 
		       fname, cp);
	else
	    DXSetError(ERROR_DATA_INVALID, "cannot load file '%s'", fname);
	DXFree((Pointer)foundname);
	return NULL;
    }

    if ((func = (PFI)dlsym(handle, "DXEntry")) == NULL) {
	DXSetError(ERROR_DATA_INVALID, 
		   "cannot find entry point 'DXEntry' in file '%s'", foundname);
	DXFree((Pointer)foundname);
	return NULL;
    }

    if (!set_tp_entry(L_LOADED, foundname, handle, func)) {
	DXFree((Pointer)foundname);
	return NULL;
    }

    DXFree((Pointer)foundname);
    return func;
}

Error DXLoadAndRunObjFile(char *fname, char *envvar)
{
    int (*func)();
    
    func = DXLoadObjFile(fname, envvar);
    if (func == NULL)
	return ERROR;
    if (func == ALREADY_LOADED)
	return OK;

    if (DXQueryDebug("L"))
        DXEnableDebug("F", 1);
	
    (*func)();
    
    if (DXQueryDebug("L"))
        DXEnableDebug("F", 0);
    
    if (DXGetError() != ERROR_NONE)
	return ERROR;

    return OK;
}


Error DXUnloadObjFile(char *fname, char *envvar)
{
    char *foundname = NULL;
    struct loadtable *lp = NULL;

    if (!findfile(fname, &foundname, envvar))
	return ERROR;
    
    if (((lp = get_tp_entry(foundname)) != NULL) && 
	(lp->loadstate == L_UNLOADED)) {
	DXDebug("L", "`%s' found but already unloaded", foundname);
	DXFree((Pointer)foundname);
	return OK;
    }
    
    if (!lp) {
	DXDebug("L", "`%s' not found in load list", foundname);
	DXSetError(ERROR_BAD_PARAMETER, 
		   "cannot unload `%s'; not found in load list", foundname);
	DXFree((Pointer)foundname);
	return OK;
    }

    if (dlclose(lp->h) < 0) {
	DXSetError(ERROR_DATA_INVALID, "cannot unload file '%s', %s", 
		   foundname, strerror(errno));
	DXFree((Pointer)foundname);
	return ERROR;
    }
    
    if (!set_tp_entry(L_UNLOADED, foundname, (Handle)0, (EntryPt)0)) {
	DXFree((Pointer)foundname);
	return ERROR;
    }
    
    DXFree((Pointer)foundname);
    return OK;
}


#endif  /* sun4, etc */

#if defined(intelnt) || defined(WIN32)

#define __ROUTINES_DEF	1


PFI DXLoadObjFile(char *fname, char *envvar)
{
    char  szName[128], szStr[128];
    HINSTANCE hinst;
    FARPROC	func;
    char *foundname = NULL;
    struct loadtable *lp = NULL;

    /* look at DXMODULES here and if you can't find the file and it
     * doesn't start with '/', start prepending dirnames until you find it
     * or you get to the end of the path.
     */
    strcpy(szName, fname);

    sprintf(szStr, "%s.dll", szName);

    DXDebug("L", "trying to load `%s'", fname);
    if (!findfile(szStr, &foundname, envvar))
        return NULL;

    DXDebug("L", "found at `%s'", foundname);
    if (((lp = get_tp_entry(foundname)) != NULL) && 
	 (lp->loadstate == L_LOADED)) {
	DXDebug("L", "`%s' was already loaded", foundname);
	DXFree((Pointer)foundname);
	return (PFI)ALREADY_LOADED;
    }

    /* args are:
     *  filename, options, address.   they recommend 0 for the address
     *  (tells the system gets to pick a load address), and the flags 
     *  have to have one of BIND_IMMEDIATE or BIND_DEFERRED (what to do
     *  with symbols - resolve them all now, or resolve them when they
     *  are referenced).
     */
    hinst = LoadLibrary(foundname);
    if (hinst == NULL) {
	DXSetError(ERROR_DATA_INVALID, "cannot load file '%s'", foundname);
	DXFree((Pointer)foundname);
	return NULL;
    }

    strcpy(szStr, "DXEntry");

    func = GetProcAddress(hinst, szStr);
    if (func == NULL) {
	DXSetError(ERROR_DATA_INVALID, 
		   "cannot load file `%s', %s", foundname, szStr);
	DXFree((Pointer)foundname);
	return NULL;
    }

    if (!set_tp_entry(L_LOADED, foundname, hinst, func)) {
	DXFree((Pointer)foundname);
	return NULL;
    }

    DXFree((Pointer)foundname);
    return (PFI) func;
}


Error DXLoadAndRunObjFile(char *fname, char *envvar)
{
    /*   int (*func)();   */
    FARPROC  func;


    func = (FARPROC) DXLoadObjFile(fname, envvar);
    if (func == NULL)
	return ERROR;
    if (func == ALREADY_LOADED)
	return OK;

    if (DXQueryDebug("L"))
        DXEnableDebug("F", 1);
	
    (*func)();
    
    if (DXQueryDebug("L"))
        DXEnableDebug("F", 0);
    
    if (DXGetError() != ERROR_NONE)
	return ERROR;

    return OK;
}

Error DXUnloadObjFile(char *fname, char *envvar)
{
    char *foundname = NULL;
    struct loadtable *lp = NULL;

    if (!findfile(fname, &foundname, envvar))
        return ERROR;

    if (((lp = get_tp_entry(foundname)) != NULL) && 
        (lp->loadstate == L_UNLOADED)) {
            DXDebug("L", "`%s' found but already unloaded", foundname);
            DXFree((Pointer)foundname);
            return OK;
    }

    if (!lp) {
        DXDebug("L", "`%s' not found in load list", foundname);
        DXSetError(ERROR_BAD_PARAMETER, 
            "cannot unload `%s'; not found in load list", foundname);
        DXFree((Pointer)foundname);
        return OK;
    }
    
    if (! FreeLibrary((HMODULE)lp->h)) {
        DXSetError(ERROR_INVALID_DATA, "cannot unload file '%s', %s", 
            foundname, strerror(errno));
        DXFree((Pointer)foundname);
        return ERROR;
    }
    
    if (!set_tp_entry(L_UNLOADED, foundname, (Handle)0, (EntryPt)0)) {
        DXFree((Pointer)foundname);
        return ERROR;
    }

    DXFree((Pointer)foundname);
    return OK;
}


#if defined(DX_NATIVE_WINDOWS)

void
DXFreeLoadedObjectFiles()
{
    struct loadtable *np;

	while ((np = ltp[DXProcessorId()]) != NULL)
	{
		FreeLibrary((HMODULE)np->h);
		ltp[DXProcessorId()] = np->next;
		DXFree((Pointer)np);
	}
}

#endif   /* DX_NATIVE_WINDOWS */


#endif   /*   DXD_WIN         */



/* this is the default case - if one of the previous sections
 *  doesn't get used, this one does.  it just sets error messages.
 */

#ifndef __ROUTINES_DEF



PFI DXLoadObjFile(char *fname, char *envvar)
{
    DXSetError(ERROR_NOT_IMPLEMENTED,
	       "dynamic loading of modules not supported on the %s platform",
	       DXD_ARCHNAME);
    return NULL;
}


Error DXLoadAndRunObjFile(char *fname, char *envvar)
{
    DXSetError(ERROR_NOT_IMPLEMENTED,
	       "dynamic loading of modules not supported on the %s platform",
	       DXD_ARCHNAME);
    return ERROR;
}

Error DXUnloadObjFile(char *fname, char *envvar)
{
    /* the unload routine needs the entry point address, which we
     * wouldn't need to save unless this is an important function.
     */
    DXSetError(ERROR_NOT_IMPLEMENTED, 
	       "cannot unload a dynamically loaded file on the %s platform",
	       DXD_ARCHNAME);
    return ERROR;
}

#endif  /* !SUPPORTED */



/* worker routine section */
static struct loadtable *get_tp_entry(char *filename) 
{
    struct loadtable *np;
    int id = DXProcessorId();

    if (!ltp[id])
	return NULL;

    for (np = ltp[id]; np; np=np->next)
	if (!strcmp(np->filename, filename))
	    return np;

    return NULL;
}

static struct loadtable *set_tp_entry(Ls loadstate, char *filename, Handle h,
					EntryPt func)
{
    struct loadtable *np;
    int id = DXProcessorId();

    if (!filename)
	return NULL;

    np = (struct loadtable *)DXAllocate(sizeof(struct loadtable));
    if (!np)
	return NULL;

    np->loadstate = loadstate;
    np->filename = (char *)DXAllocate(strlen(filename) + 1);
    if (!np->filename) {
	DXFree((Pointer)np);
	return NULL;
    }
    
    strcpy(np->filename, filename);
    np->h = h;
    np->func = func;
    np->next = NULL;

    if (ltp[id]) {
	np->next = ltp[id]->next;
	ltp[id]->next = np;
    } else
	ltp[id] = np;

    return np;
}
	
	


 
/* 
 * locate a file using each part of the DXMODULES path
 *  if that environment variable is defined.  
 *  return the actual pathname if successful in 'outname'
 */
static Error findfile(char *inname, char **outname, char *envvar)
{
    return _dxf_fileSearch(inname, outname, NULL, envvar ? envvar : "DXMODULES");
}

 

/* func for searching for a filename.   this routine belongs in libdx.
 * there are several duplicate routines like this in the importers.
 *
 * usage:
 *  extension can be null; otherwise inname.extension will be tried
 *  if inname starts with / then it is an absolute pathname and no further
 *   searching is done (besides trying an extension if given).
 *  environment can be null; otherwise it is expected to be the name of
 *   an environment variable whose value is a (string) directory list 
 *   (e.g. dir1:dir2:dir3).   inname and inname.extension will be tried 
 *   in each dir until the file is found or we run out of dirs.
 *
 *  the complete filename (suitable for passing to open()) will be returned
 *   in *outname.  this checks that the file is not a dir, and is readable.
 *  the caller is responsible for calling DXFree() on outname when finished.
 *  if this routine returns error, outname will be NULL and no space needs
 *   to be freed.
 */
Error _dxf_fileSearch(char *inname, char **outname, char *extension, 
		      char *environment)
{
    struct stat sb;
    char *datadir = NULL, *cp;
    int bytes = 0;

    *outname = NULL;

    if (!inname) {
	DXSetError(ERROR_INTERNAL, "filename must be specified to fileSearch");
	return ERROR;
    }

    /* simple check for mode bits - it has to match the given mode and it
     * can't be a directory.  i had originally planned to check for the
     * execute bits, but on alpha shared obj files don't have to be marked 
     * executable to be loaded, so i changed it to just check to be sure
     * the file can be read by someone.
     */
#ifdef DXD_WIN
#define MODE_OK(m) !S_ISDIR(m) 
#else
#define MODE_OK(m) (!S_ISDIR(m) && (m & (S_IRUSR|S_IRGRP|S_IROTH)))
#endif
#define MODE_ERR   "'%s' is not a plain file or cannot be opened for reading"
    
    /* try to open the filename as given, and save the name 
     *  for error messages later.
     */
    DXDebug("L", "trying at (A) to stat `%s'", inname);
    if (stat(inname, &sb) == 0) {
	if (!MODE_OK(sb.st_mode)) {
	    DXSetError(ERROR_BAD_PARAMETER, MODE_ERR, inname);
	    return ERROR;
	}
	
	/* if not absolute pathname, some loaders don't seem to
	 * want to search the current dir w/o ./ in front of the
	 * filename.
	 */
	*outname = (char *)DXAllocateLocalZero(strlen(inname)+3);
	if (!*outname) 
	    goto error;

#ifdef DXD_NON_UNIX_DIR_SEPARATOR
	if (inname[0] != '/' && inname[0] != '\\' && inname[1] != ':') {
	    strcpy(*outname, "./");
	    strcat(*outname, inname);
	} else
	    strcpy(*outname, inname);
#else
	if (inname[0] != '/') {
	    strcpy(*outname, "./");
	    strcat(*outname, inname);
	} else
	    strcpy(*outname, inname);
#endif
	return OK;
    }
    
    
    /* if absolute pathname and no optional extension, it's not found.
     */
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    if ((inname[0] == '/' || inname[0] == '\\' || inname[1] == ':') && extension == NULL) {
#else
    if (inname[0] == '/' && extension == NULL) {
#endif
	DXSetError(ERROR_BAD_PARAMETER, "#10903", "file", inname);
	return ERROR;
    }
    
    /* try the name as given with extension
     */
    if (extension) {
	/* space for basename, extension and ./ in front if found.
	 */
	*outname = (char *)DXAllocateLocalZero(strlen(inname) +
					       strlen(extension) + 4);
	if (!*outname) 
	    goto error;
	
	/* if not absolute pathname, start it with ./ (see comment above) */
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
	if (inname[0] != '/' && inname[0] != '\\' && inname[1] != ':') {
	    strcpy(*outname, "./");
	    strcat(*outname, inname);
	} else
	    strcpy(*outname, inname);
#else
	if (inname[0] != '/') {
	    strcpy(*outname, "./");
	    strcat(*outname, inname);
	} else
	    strcpy(*outname, inname);
#endif

	/* now add .extension */
	strcat(*outname, ".");
	strcat(*outname, extension);

	DXDebug("L", "trying at (A1) to stat `%s'", *outname);
	if (stat(*outname, &sb) == 0) {
	    if (!MODE_OK(sb.st_mode)) {
		DXSetError(ERROR_BAD_PARAMETER, MODE_ERR, *outname);
		goto error;
	    }
	    
	    return OK;
	}
    }
    
    /* ok, now start on the environment variable pathlist.
     * if there's no pathlist or this is an absolute pathname
     * it won't be found.
     */
    datadir = (char *)getenv(environment);
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    if (!datadir || ((inname[1] == ':') || (inname[0] == '/') || (inname[0] == '\\'))) {
#else
    if (!datadir || (inname[0] == '/')) {
#endif
	DXSetError(ERROR_BAD_PARAMETER, "#12247", inname, environment);
	return ERROR;
    }
    
    /* bytes is the full length of the datadir var, so it has to be
     * long enough for any individual dir in the list.
     */
    DXDebug("L", "%s is `%s'", environment, datadir);
    bytes = strlen(inname) + 5 + strlen(datadir) + 
	(extension ? strlen(extension) : 0);
    
    *outname = (char *)DXAllocateZero(bytes);
    if (!*outname)
	return ERROR;

    /* use each part of the pathname with the filename, and each without
     * and with the extension if it's given.
     */
    while (datadir) {
	
	/* if not absolute pathname, start it with ./ (see comment above) */
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
	if (datadir[0] != '/' && datadir[0] != '\\' && datadir[1] != ':') {
	    strcpy(*outname, "./");
	    strcat(*outname, datadir);
	} else
	    strcpy(*outname, datadir);
#else
	if (datadir[0] != '/') {
	    strcpy(*outname, "./");
	    strcat(*outname, datadir);
	} else
	    strcpy(*outname, datadir);
#endif
	if ((cp = strchr(*outname, DX_DIR_SEPARATOR)) != NULL)
	    *cp = '\0';
	strcat(*outname, "/");
	strcat(*outname, inname);

	DXDebug("L", "trying at (B) to stat `%s'", *outname);

#ifdef DXD_NON_UNIX_DIR_SEPARATOR
	
	{
		int i;
		for (i = 0; i < strlen(*outname); i++)
			if ((*outname)[i] == '\\')
			    (*outname)[i] = '/';
	}

#endif

	if (stat(*outname, &sb) == 0) {
	    if (!MODE_OK(sb.st_mode)) {
		DXSetError(ERROR_BAD_PARAMETER, MODE_ERR, *outname);
		goto error;
	    }
	    
	    return OK;
	}

	if (extension) {
	    strcat(*outname, ".");
	    strcat(*outname, extension);
	    DXDebug("L", "trying at (B1) to stat `%s'", *outname);
	    
	    if (stat(*outname, &sb) == 0) {
		if (!MODE_OK(sb.st_mode)) {
		    DXSetError(ERROR_BAD_PARAMETER, MODE_ERR, *outname);
		    goto error;
		}
	    
		return OK;
	    }
	}
	
	datadir = strchr(datadir, DX_DIR_SEPARATOR);
	if (datadir)
	    datadir++;
    }
    
    /* if you get here, you didn't find the file.  fall thru
     *  into error section.
     */
    DXSetError(ERROR_BAD_PARAMETER, "#12247", inname, environment);
    
  error:
    if (*outname) {
	DXFree((Pointer)*outname);
	*outname = NULL;
    }

    return ERROR;
}


	
