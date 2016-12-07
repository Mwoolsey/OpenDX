/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#define tdmRender_c


#include <stdio.h>

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdlib.h>

#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(DX_NATIVE_WINDOWS)
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glaux.h>
#endif

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwMemory.h"
#include "hwSort.h"

#include "hwDebug.h"

/*
#if defined(sgi)
int TryDGL(char *, int);
#endif
*/

#define	CACHEPREFIX 	"tdmRender/"
#define CACHEKEY         0x1234ABCD

typedef Error (*Handler)(int, Pointer) ;
static char* _getFullHostName(char* givenName);
extern Error _dxfDrawMonoInCurrentContext (void*, dxObject, Camera, int buttonUp);

#if defined(DX_NATIVE_WINDOWS)

Error _dxfRenderImage(tdmChildGlobalP globals, int flag)
{
    DEFGLOBALDATA(globals);

    //PostMessage(XWINID, WM_PAINT, 0, 0);
    _dxfDraw(globals, OBJECT, CAMERA, 1);
    return OK;
}
tdmPortHandleP_dxfNewPortHandle(tdmParsedFormatP format);

#else /* !(DX_NATIVE_WINDOWS) */

static 	Display*	hwDpy = NULL;

static char* _getFullXserver(char* givenXserver);
int 		_dxfCatchWinopenErrors (Display *dpy, XErrorEvent *error) ;
Error 		_dxfProcessEvents (int fd, tdmChildGlobalP globals, int flag);

Error _dxfProcessEventsInputHandler(int fd, tdmChildGlobalP globals)
{
    return _dxfProcessEvents(fd, globals, 0);
}

#if 0
Error _dxfRenderImage(tdmChildGlobalP globals, int flag)
{
    return _dxfProcessEvents(-1, globals, flag);
}
Error _dxfRenderImageInputHandler(int fd, tdmChildGlobalP globals)
{
    return _dxfRenderImage(globals, 0);
}
#endif

static int
WindowChecker(int fd, void *d)
{
    tdmChildGlobalP globals = (tdmChildGlobalP)d;
    DEFGLOBALDATA(globals) ;
    return XPending(DPY);
}
tdmPortHandleP _dxfNewPortHandle(tdmParsedFormatP format, Display **dpy);

#endif

tdmParsedFormatT*
_tdmParseDisplayString (char *displayString,char** cacheIdP);

static Error
_validateDisplayString(tdmParsedFormatT	*pFormat, int Pass);

Error
_dxfDeleteParsedDisplayString(tdmParsedFormatT *pFormat);

static Error
_dxfDeleteCachedDisplayString(Pointer arg);

static tdmParsedFormatT*
_dxfCopyParsedDisplayString(tdmParsedFormatT *pFormat);

extern void
    _dxfSetInteractionMode(tdmChildGlobalP globals, int mode, dxObject args);

int isOpenGL = 0;

int CanDoOGL(char * Where) /* Are GLX Extensions Available on Where ? */
{
#if defined(DX_NATIVE_WINDOWS)
    return 1;
#else

    static int FirstPass = 1;
    static int RetVal;
    Display *Dpy;

    if(FirstPass)
    {
        if(getenv("DXHWMOD")) {
            if(0 == strcmp("DXhwdd.o", getenv("DXHWMOD")))
                return RetVal = 0;
            else
                return RetVal = 1;
        } else
            RetVal = 1;
        FirstPass = 0;
    }
    if(RetVal == 0)
        return 0;

    if((Dpy = XOpenDisplay(Where)))
    {
        int Extra, Major, Valid;

        Valid = XQueryExtension(Dpy, "GLX", &Major, &Extra, &Extra);
        XCloseDisplay(Dpy);
        if(Valid && (Major > 0))
            return 1;
        else
            return 0;
    }
    return 0;
#endif
}

Error
_dxfRedrawInActiveContext (dxObject r, Camera c, char *displayString)
{
    Private 	    cacheObject = NULL;
    char*		    cacheId = NULL;

	/* Need to build up the globals then call _dxfDraw() */
	tdmChildGlobalP globals = NULL;
    tdmParsedFormatT *pFormat = NULL;

	    if (!c && !DXGetImageBounds(r,NULL,NULL,NULL,NULL))
        goto error;


    /* get host and window name from display string, fill in the cache id */
    if (!(pFormat = _tdmParseDisplayString (displayString, &cacheId)))
        goto error;

    /* validate the display string */
    if (!(_validateDisplayString(pFormat, 1)))
        goto error ;

    /* Get a pointer to the global data, out of the cache or create a new one. */
    if((cacheObject = (Private) DXGetCacheEntry(cacheId, 0, 0)) != NULL) {
        globals = (tdmChildGlobalP) DXGetPrivateData(cacheObject) ;

        /* Create a reference so this doesn't get deleted while I'm using it */
        DXReference ((Pointer)cacheObject);

        /* associate cacheId with cacheObject, set to permanent status */
        if(! DXSetCacheEntry((Pointer)cacheObject, CACHE_PERMANENT,
                             cacheId, 0, 0)) {
            /* Can't set cache entry for tdm globals */
            DXErrorGoto (ERROR_INTERNAL, "#13300") ;
        }

        /* save the cacheId to allow cache deletion upon DestroyNotify event */
        if(! (globals->cacheId = tdmAllocate(strlen(cacheId)+1))) {
            DXErrorGoto (ERROR_NO_MEMORY, "") ;
        }

        /* save cacheId */
        strcpy(globals->cacheId,cacheId) ;
    } else {
    	DXErrorGoto(ERROR_INTERNAL, "object must be previously rendered");
    }

    /* Now that we have a global store, use it */
    {
        int needInit = 0;
        DEFGLOBALDATA(globals);

        if (!OBJECT || (OBJECT && OBJECT_TAG != DXGetObjectTag(r)))
        {
            if (OBJECT)
                DXDelete(OBJECT);

            OBJECT = DXReference(r);
            OBJECT_TAG = DXGetObjectTag(r);
            needInit = 1;
        }

        if (!CAMERA || (CAMERA && CAMERA_TAG != DXGetObjectTag((dxObject)c)))
        {
            if (CAMERA)
                DXDelete(CAMERA);

            CAMERA = DXReference((dxObject)c);
            CAMERA_TAG = DXGetObjectTag((dxObject)c);
            needInit = 1;
        }

        if(CAMERA && needInit)
        {
            if(!_dxfInitRenderObject(LWIN)) {
                /* unable to set up for rendering */
                DXErrorGoto(ERROR_NO_HARDWARE_RENDERING, "#13350");
            }
        }
    }

    {
	DEFWINDATA(&(globals->win));        
    _dxfDrawMonoInCurrentContext(globals, OBJECT, CAMERA, 1);
    }

    if(cacheObject) {
        /* delete the working reference  */
        DXDelete((Pointer)cacheObject) ;
        cacheObject = NULL;
    }

    if (cacheId)
        tdmFree(cacheId);
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);


	return OK;

error:

    if(cacheObject) {
        /* delete the working reference  */
        DXDelete((Pointer)cacheObject) ;
        cacheObject = NULL;
    }

    if (cacheId)
        tdmFree(cacheId);
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);

    EXIT(("ERROR"));
    DEBUG_MARKER("_dxfAsyncRender EXIT");
    return ERROR ;
}

Error
_dxfAsyncRender (dxObject r, Camera c, char *obsolete, char *displayString)
{
    Private 	    cacheObject = NULL;
    tdmChildGlobalP   globals = NULL;
    char*		    cacheId = NULL;
    tdmParsedFormatT *pFormat = NULL;
    dxObject	   attr;


    ENABLE_DEBUG();
    DEBUG_MARKER("_dxfAsyncRender ENTRY");
    ENTRY(("_dxfAsyncRender (0x%x, 0x%x, \"%s\", \"%s\")",
           r, c, obsolete, displayString));

# if defined(DEBUG)

    if(DXGetAttribute(r, "force env var")) {
        char *Buff;

        if(DXExtractString(DXGetAttribute(r, "force env var"), &Buff)) {
            PRINT(("Putting Variable %s\n", Buff));
            putenv(Buff);
        }
    }
#  endif


    if (!c && !DXGetImageBounds(r,NULL,NULL,NULL,NULL))
        goto error;


    /* get host and window name from display string, fill in the cache id */
    if (!(pFormat = _tdmParseDisplayString (displayString, &cacheId)))
        goto error;

    /* validate the display string */
    if (!(_validateDisplayString(pFormat, 1)))
        goto error ;

    /* Get a pointer to the global data, out of the cache or create a new one. */
    if((cacheObject = (Private) DXGetCacheEntry(cacheId, 0, 0)) != NULL) {
        globals = (tdmChildGlobalP) DXGetPrivateData(cacheObject) ;
#if defined(DX_NATIVE_WINDOWS)
        /*
         * Now lock globals so that rendering doesn't occur in another
         * thread until the setup is done.  If we create a globals structure 
         * here, the lock is created in CreateRenderModule, and its created 
         * owned.
         */
        {
            DEFGLOBALDATA(globals);
            if (DXWaitForSignal(1, &LOCK) == WAIT_TIMEOUT)
                goto error;
        }
#endif

    } else {
        if(!(globals = (tdmChildGlobalP)_dxfCreateRenderModule(pFormat)))
            return ERROR;

        /* save globals in cache; call _dxfEndRenderModule when cache deleted */
        if(!(cacheObject = (Private)
                           DXNewPrivate((Pointer) globals,
                                        (Error (*)(Pointer))_dxfEndRenderModule))) {
            /* Can't cache tdm globals */
            DXErrorGoto (ERROR_INTERNAL, "#13290");
        }

        /* Create a reference so this doesn't get deleted while I'm using it */
        DXReference ((Pointer)cacheObject);

        /* associate cacheId with cacheObject, set to permanent status */
        if(! DXSetCacheEntry((Pointer)cacheObject, CACHE_PERMANENT,
                             cacheId, 0, 0)) {
            /* Can't set cache entry for tdm globals */
            DXErrorGoto (ERROR_INTERNAL, "#13300") ;
        }

        /* save the cacheId to allow cache deletion upon DestroyNotify event */
        if(! (globals->cacheId = tdmAllocate(strlen(cacheId)+1))) {
            DXErrorGoto (ERROR_NO_MEMORY, "") ;
        }

        /* save cacheId */
        strcpy(globals->cacheId,cacheId) ;
    }

    /* Now that we have a global store, use it */
    {
        int needInit = 0;
        DEFGLOBALDATA(globals);

        if (!OBJECT || (OBJECT && OBJECT_TAG != DXGetObjectTag(r)))
        {
            if (OBJECT)
                DXDelete(OBJECT);

            OBJECT = DXReference(r);
            OBJECT_TAG = DXGetObjectTag(r);
            needInit = 1;
        }

        if (!CAMERA || (CAMERA && CAMERA_TAG != DXGetObjectTag((dxObject)c)))
        {
            if (CAMERA)
                DXDelete(CAMERA);

            CAMERA = DXReference((dxObject)c);
            CAMERA_TAG = DXGetObjectTag((dxObject)c);
            needInit = 1;
        }

        _dxfInitializeStereoSystemMode(globals, DXGetAttribute(r, "stereo system mode"));
        _dxfInitializeStereoCameraMode(globals, DXGetAttribute(r, "stereo camera mode"));

        if(CAMERA && needInit)
        {
            if(!_dxfInitRenderObject(LWIN)) {
                /* unable to set up for rendering */
                DXErrorGoto(ERROR_NO_HARDWARE_RENDERING, "#13350");
            }
        }

        /* Handle X events and redraw the image */
#if !defined(DX_NATIVE_WINDOWS)
        if(! _dxfProcessEvents (-1, globals, 1))
            goto error;
#endif

    }

    if ((attr = DXGetAttribute((dxObject)c, "camera interaction mode")) == NULL)
        if ((attr = DXGetAttribute(r, "object interaction mode")) == NULL)
            attr = DXGetAttribute(r, "interaction mode");

    if (attr) {
        Array amode;
        int mode;

        if (DXGetObjectClass(attr) == CLASS_GROUP) {
            if (NULL == (amode = (Array)DXGetMember((Group)attr, "mode")))
                DXErrorGoto(ERROR_DATA_INVALID,
                            "interaction mode args group must contain a member named \"mode\"");
        } else if (DXGetObjectClass(attr) != CLASS_ARRAY) {
            DXErrorGoto(ERROR_DATA_INVALID,
                        "interaction mode args must be a group or array");
        } else
            amode = (Array)attr;

        if (DXExtractInteger((dxObject)amode, &mode))
            _dxfSetInteractionMode(globals, mode, attr);
        else
            DXErrorGoto(ERROR_DATA_INVALID,
                        "interaction mode attribute must be an integer");
    } else
        _dxfSetInteractionMode(globals, 11, NULL);

    if(cacheObject) {
        /* delete the working reference  */
        DXDelete((Pointer)cacheObject) ;
        cacheObject = NULL;
    }

    if (cacheId)
        tdmFree(cacheId);
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);

    EXIT(("OK"));
    DEBUG_MARKER("_dxfAsyncRender EXIT");
    return OK ;

error:

    if(cacheObject) {
        /* delete the working reference  */
        DXDelete((Pointer)cacheObject) ;
        cacheObject = NULL;
    }

    if (cacheId)
        tdmFree(cacheId);
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);

    EXIT(("ERROR"));
    DEBUG_MARKER("_dxfAsyncRender EXIT");
    return ERROR ;
}

extern Field _dxfCaptureHardwareImage(tdmChildGlobalP);

dxObject
_dxfSaveHardwareWindow(char *where)
{
    Private           cacheObject = NULL;
    tdmChildGlobalP   globals = NULL;
    char*             cacheId = NULL;
    tdmParsedFormatT* pFormat = NULL;
    Field             image = NULL;

    /*
     * get host and window name from display string, fill in the cache id
     */
    if (!(pFormat = _tdmParseDisplayString (where, &cacheId)))
        goto error;

    /*
     * validate the display string
     */
    if (!(_validateDisplayString(pFormat, 1)))
        goto error ;

    /*
     * Get a pointer to the global data
     */
    cacheObject = (Private) DXGetCacheEntry(cacheId, 0, 0);
    if (! cacheObject) {
        DXSetError(ERROR_BAD_PARAMETER,
                   "window matching \"%s\" not found", where);
        goto error;
    } else {
        globals = (tdmChildGlobalP) DXGetPrivateData(cacheObject) ;
        image = _dxfCaptureHardwareImage(globals);
        if (! image)
            goto error;
    }


error:
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);

    if (cacheId)
        tdmFree(cacheId);

    /*
     * delete the working reference 
     */
    if(cacheObject)
        DXDelete((Pointer)cacheObject) ;

    return (dxObject)image;
}

int BackingStore = TRUE;


void
_dxfDestroyRenderModule(tdmChildGlobalP globals)
{
    DEFGLOBALDATA(globals) ;
    if (STEREOCAMERAMODE >= 0)
        _dxfExitStereoCameraMode(globals);
    if (STEREOSYSTEMMODE >= 0)
        _dxfExitStereoSystemMode(globals);
    DXDelete(OBJECT);
    tdmFree(WHERE);
    _dxf_deleteSortList(SORTLIST);
    tdmFree(globals);
}

#if !defined(DX_NATIVE_WINDOWS)
static int
XChecker(int fd, void *d)
{
    tdmChildGlobalP globals = (tdmChildGlobalP)d;
    DEFGLOBALDATA(globals) ;
    return XPending(DPY);
}
#endif

#if !defined(DX_NATIVE_WINDOWS)
extern Error DXRegisterWindowHandlerWithCheckProc(Error (*proc) (int, Pointer),
            int (*check)(int, Pointer), Display *d, Pointer arg);
#endif

tdmChildGlobalP _dxfCreateRenderModule(tdmParsedFormatT *format)
{
    tdmChildGlobalP globals = NULL ;
    tdmPortHandleP  portHandle;

    ENTRY(("_dxfCreateRenderModule (0x%x)",format));

#if 0
    /* If different 'where' open new connections (NULL to newPortHandle) */
    if(!strlen(hwWhere) || strcmp(format->where,hwWhere))
#endif

#if !defined(DX_NATIVE_WINDOWS)

        hwDpy = NULL;

    if(!(portHandle = _dxfNewPortHandle(format,&hwDpy)))
#else

        if(!(portHandle = _dxfNewPortHandle(format)))
#endif

    {
        EXIT(("_dxfNewPortHandle failed"));
        return NULL;
    }
    /* Got the HW Layer, ReCheck the DisplayString */

    if (!(_validateDisplayString(format, 2)))
        goto error;

    /* allocate globals */
    if (! (globals = (tdmChildGlobalP)
                     tdmAllocateZero (sizeof(tdmChildGlobalT))))
        /* can't allocate render globals */
        DXErrorGoto (ERROR_NO_MEMORY, "#13270") ;

    {
        DEFGLOBALDATA(globals) ;
        DEFPORT(portHandle);

#if !defined(DX_NATIVE_WINDOWS)

        DPY = hwDpy;
#endif

        PORT_HANDLE = portHandle;

        STEREOCAMERAMODE = -1;
        STEREOSYSTEMMODE = -1;

        SORTLIST = _dxf_newSortList();

#if defined(DX_NATIVE_WINDOWS)
        /* Then input to the window does not come through the X event loop (there ain't
         * one, after all).  Instead, they are handled in a separate thread thats watching
         * messages for the window
         */
        LOCK = CreateMutex(NULL, TRUE, NULL);

        if (! _dxfCreateHWWindow(globals, format->name, NULL))
            goto error;

#else
        /* _dxfProcessEvents() is the input handler for the X connection fd */
        DXRegisterWindowHandlerWithCheckProc ((Handler) _dxfProcessEventsInputHandler,
                                              XChecker, DPY, (Pointer) globals) ;

        /* create graphics window */
        if (! _dxfCreateWindow (globals, format->name))
            goto error ;

#endif


        /* initialize graphics API */
        if (! _dxf_INIT_RENDER_MODULE (globals))
            goto error ;

        if (format->where) {
            WHERE = (char *)DXAllocate(strlen(format->where) + 1);
            strcpy(WHERE, format->where);
        } else
            WHERE = NULL;

        if (format->originalWhere) {
            ORIGINALWHERE = (char *)DXAllocate(strlen(format->originalWhere) + 1);
            strcpy(ORIGINALWHERE, format->originalWhere);
        } else
            ORIGINALWHERE = NULL;

        LINK = format->link;

        /* Undocumented environment varaible to enable FLING mode for
           Marketing persons.  This varaible is not presented to users. */
        if(getenv("DXFLING"))
            _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_FLING);

        if((_dxf_GET_VERSION(NULL) != 0x080001) &&
                (_dxf_GET_VERSION(NULL) > 0.0))
            _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_DOES_TRANS);

        if(getenv("DXHW_VERBOSE")) {
            int dsoVer;
            char * dsoStr;

            dsoVer = _dxf_GET_VERSION(&dsoStr);
            DXMessage("%s DSO rev %x.%x.%x", dsoStr,
                      dsoVer >> 16, (dsoVer & 0xff00) >> 8, dsoVer & 0xff);
        }

        /*
         * Environment varaible documented only for Freedom 6000.
         * Will disable frame buffer readbacks on machines that are slow.
         */
        if(getenv("DXNO_BACKING_STORE"))
            _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE);

        if(!BackingStore)
            _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE);
    }

    EXIT(("globals = 0x%x",globals));
    return globals ;

error:
    if (globals)
        _dxfDestroyRenderModule(globals);

    EXIT(("globals = NULL"));
    return NULL ;
}


#define EQLSTR(str1,str2) (!strcmp(str1,str2))

static char* _token(char* str, char* delimit, char* chaff, int inPlace);

#if 0
/* This is never called */
static int
_isSameInstance(tdmParsedFormatT *first,
                tdmParsedFormatT *second )
{
    ENTRY(("isSaveInstance(0x%x, 0x%x)",first,second));

    if(!EQLSTR(first->type,second->type) ||
            !EQLSTR(first->fullHost,second->fullHost) ||
            !EQLSTR(first->Xserver,second->Xserver) ||
            !EQLSTR(first->name,second->name)) {
        EXIT(("NO"));
        return 0;
    }

    EXIT(("YES"));
    return 1;
}
#endif

/* i've added some new code here.  the first time through, a display
 * string is parsed up and stored in a tdmParsedFormatT struct.
 * a copy of the parse struct plus a parsed cache id string is put
 * into a private object and cached with the display string as the key.
 * the next time through, the info in the private object is retrieved
 * instead of reparsing.  the most offensive part of reparsing is that
 * it calls gethostname() which can be expensive if your nameserver is
 * overloaded or down.  nsc 08may96
 */

/* new. this routine deletes the private data associated with a cached
 * private object.  the format of the private data is given below.
 */
static Error _dxfDeleteCachedDisplayString(Pointer arg)
{
    ubyte **ptr = (ubyte **)arg;

    if (!tdmFree((char *)(ptr[0])))
        return ERROR;
    if (!_dxfDeleteParsedDisplayString((tdmParsedFormatT*)(ptr[1])))
        return ERROR;

    if (!tdmFree(ptr))
        return ERROR;

    return OK;
}

/* existing.  this routine deletes a tdmParsedFormatT struct from memory
 */
Error _dxfDeleteParsedDisplayString(tdmParsedFormatT *pFormat)
{

    ENTRY(("_dxfDeleteParsedDisplayString(0x%x)",pFormat));

    if(!pFormat) {
        EXIT(("ERROR"));
        return ERROR;
    }

    if(pFormat->type)
        tdmFree(pFormat->type);

    if(pFormat->where)
        tdmFree(pFormat->where);

    if(pFormat->name)
        tdmFree(pFormat->name);

    if(pFormat->Xserver)
        tdmFree(pFormat->Xserver);

    if(pFormat->fullHost)
        tdmFree(pFormat->fullHost);

    if(pFormat->localHost)
        tdmFree(pFormat->localHost);

    if(pFormat->originalWhere)
        tdmFree(pFormat->originalWhere);

    /* notice they skip the cacheId.  it's never initialized so this is good. */

    tdmFree(pFormat);

    EXIT(("OK"));
    return OK;
}

/* new.  this makes and returns a new copy of the struct and
 * all the strings in it.
 */
static tdmParsedFormatT*
_dxfCopyParsedDisplayString(tdmParsedFormatT *pFormat)
{
    tdmParsedFormatT *newcopy = NULL;

    ENTRY(("_dxfDeleteParsedDisplayString(0x%x)",pFormat));

    if(!pFormat)
        goto error;

    newcopy = tdmAllocateZero(sizeof(tdmParsedFormatT));
    if (!newcopy)
        goto error;

#define DO_COPY(thing) \
    if (pFormat->thing) { \
	newcopy->thing = tdmAllocate(strlen(pFormat->thing) + 1); \
	if (!newcopy->thing) \
	    goto error; \
	strcpy(newcopy->thing, pFormat->thing); \
    }

    DO_COPY(type);
    DO_COPY(where);
    DO_COPY(fullHost);
    DO_COPY(localHost);
    DO_COPY(Xserver);
    DO_COPY(name);
    newcopy->link = pFormat->link;
    DO_COPY(originalWhere);
    /* DO_COPY(cacheId);  -- this seems never to be used nor initialized */

    EXIT(("OK"));
    return newcopy;

error:
    /* should clean up partially allocated structs here */
    EXIT(("ERROR"));
    return NULL;
}

/* i added a private data object to this routine, cached by
 * the contents of the display string.  if not found, the
 * string is parsed and both the cacheid string and the 
 * parsed tdm structure are saved for next time.  the structure
 * of the private data block is:
 *   addr of cacheIdP char string
 *   addr of tdmParsedFormatT struct
 * if found in cache, the contents are copied so they can be
 * deleted in the exit code and not affect the cached versions.
 */
tdmParsedFormatT*
_tdmParseDisplayString (char *displayString, char** cacheIdP)
{
    char *whereHostP = NULL, *XserverP = NULL;
    tdmParsedFormatT	*pFormat;
    dxObject id;
    ubyte **priv;

    ENTRY(("_tdmParseDisplayString (\"%s\", 0x%x)",displayString, cacheIdP));

    /* if we've been here before with this display string, get it from cache.
     */
    if ((id = DXGetCacheEntry(displayString, CACHEKEY, 0)) != NULL) {

        priv = (ubyte **)DXGetPrivateData((Private)id);
        if (!priv)
            goto reparse;

        *cacheIdP = tdmAllocate(strlen((char *)(priv[0])) + 1);
        if (!(*cacheIdP))
            goto error;

        strcpy(*cacheIdP, (char *)(priv[0]));
        pFormat = _dxfCopyParsedDisplayString((tdmParsedFormatT*)(priv[1]));

        /* GetCacheEntry adds a reference.  we have a copy now so the extra
         * reference to the cached copy can be deleted.
         */
        DXDelete(id);

        EXIT(("pFormat (found in cache) = 0x%x",pFormat));
        return pFormat;
    }

reparse:
    /* if new string, or if the old cache info has been flushed, do it again
     */
    if(!(pFormat = tdmAllocate(sizeof(tdmParsedFormatT))))
        goto error;

    pFormat->originalWhere = tdmAllocate(strlen(displayString)+1);
    strcpy(pFormat->originalWhere, displayString);

    pFormat->type = _token(displayString,","," \t",0);
    pFormat->where = _token(NULL,","," \t",0);
    pFormat->name = _token(NULL,","," \t",0);

    if(!pFormat->type || !pFormat->where || !pFormat->name)
        goto error;

    /* Set defaults for NULL or blank values */

    /* type */
    if(!*pFormat->type) {
        tdmFree(pFormat->type);
        if(!(pFormat->type = _token("X","","",0)))
            goto error;
    }

    /* name */
    if(!*pFormat->name) {
        tdmFree(pFormat->name);
        if(!(pFormat->name = _token("Image","","",0)))
            goto error;
    }

    /* where */
    /*
     * If no 'where' given default comes for "DISPLAY" env variable.
     * If a 'where' is given, we must set the "DISPLAY" env to it.
     */
    if(!*pFormat->where) {
        char*	env_dis;

        tdmFree(pFormat->where);
        if(!(env_dis = (char*)getenv("DISPLAY"))) {
            pFormat->where = NULL;
        } else {
            if(!(pFormat->where = _token(env_dis,"","",0)))
                goto error;
        }
    }

    pFormat->link = pFormat->name &&
                    strlen(pFormat->name) > 2 &&
                    pFormat->name[0] == '#' &&
                    pFormat->name[1] == 'X';

#if !defined(DX_NATIVE_WINDOWS)

    /*
     * Divide up the 'where' parameter and get the full host name and server
     * designator (ex. :0.0)
     */

    whereHostP = _token(pFormat->where,":"," \t",0);
    XserverP = _token(NULL,""," \t",0);
    pFormat->Xserver = _getFullXserver(XserverP);
    pFormat->fullHost = _getFullHostName(whereHostP);
    pFormat->localHost = _getFullHostName("localhost");

    if(! pFormat->Xserver || !whereHostP || !XserverP)
        goto error;

    if(!pFormat->fullHost || !pFormat->localHost) {
        /* gethostname failed */
        DXSetError (ERROR_INTERNAL,"#13690");
        goto error;
    }

    if( cacheIdP &&
            (*cacheIdP = tdmAllocate(strlen(CACHEPREFIX)+
                                     strlen(pFormat->type)+
                                     strlen(pFormat->fullHost)+
                                     strlen(pFormat->Xserver)+
                                     strlen(pFormat->name)+
                                     1))) {
        strcpy(*cacheIdP,CACHEPREFIX);
        strcat(*cacheIdP,pFormat->type);
        strcat(*cacheIdP,pFormat->fullHost);
        strcat(*cacheIdP,pFormat->Xserver);
        strcat(*cacheIdP,pFormat->name);
    }

    tdmFree(whereHostP);
    tdmFree(XserverP);
#else

    if( cacheIdP &&
            (*cacheIdP = tdmAllocate(strlen(CACHEPREFIX)+
                                     strlen(pFormat->type)+
                                     strlen(pFormat->name)+
                                     1))) {
        strcpy(*cacheIdP,CACHEPREFIX);
        strcat(*cacheIdP,pFormat->type);
        strcat(*cacheIdP,pFormat->name);
    }


    pFormat->Xserver = NULL;
    pFormat->fullHost = NULL;
    pFormat->localHost = NULL;

#endif
    /* cache this info for next time */
    priv = (ubyte **)tdmAllocate(sizeof(ubyte *) * 2);
    if (!priv)
        goto error;

    priv[0] = (ubyte *)tdmAllocate(strlen(*cacheIdP) + 1);
    if (!priv[0])
        goto error;

    strcpy((void *)(priv[0]), (void *)*cacheIdP);
    priv[1] = (ubyte *)_dxfCopyParsedDisplayString(pFormat);
    if (!priv[1])
        goto error;

    id = (dxObject)DXNewPrivate((Pointer)priv, _dxfDeleteCachedDisplayString);
    if (!id)
        goto error;

    /* put it in the cache using the original display string as the key. */
    if (!DXSetCacheEntry(id, 0.0, displayString, CACHEKEY, 0))
        goto error;

    EXIT(("pFormat = 0x%x",pFormat));
    return pFormat;

error:
#if !defined(DX_NATIVE_WINDOWS)

    if(whereHostP)
        tdmFree(whereHostP);
    if(XserverP)
        tdmFree(XserverP);
#endif

    _dxfDeleteParsedDisplayString(pFormat);

    EXIT(("ERROR: pFormat = NULL"));
    return NULL ;
}

static Error
_validateDisplayString(tdmParsedFormatT *pFormat, int Pass)
{
    static char newDisplay[MAXHOSTNAMELEN + 31];
    /* DISPLAY=<hostname>:<10d>.<10d>\n */

    ENTRY(("_validateDisplayString(0x%x)", pFormat));

    BackingStore = TRUE;
    /* if 'type' does not start with 'X' we don't do HW rendering */
    if(pFormat->type[0] != 'X') {
        DXSetError(ERROR_BAD_PARAMETER, "invalid type %s", pFormat->type);
        goto error;
    }
#if !defined(DX_NATIVE_WINDOWS)
    if(!pFormat->where) {
        /* no where parameter to Display() module, and no DISPLAY env var */
        DXSetError(ERROR_BAD_PARAMETER,
                   "unable to determine where to create display window");
        goto error;
    }
    /*
     * We must have a 'where' by now, and is must  be of the form
     * [hostname]:<number>[.<number>]
     */
    if((strlen(pFormat->where) == 0) ||
            strchr(pFormat->where,':') == NULL) {
        /* DISPLAY environment variable '%s' is not set or is invalid */
        DXSetError(ERROR_BAD_PARAMETER,"#13700",pFormat->where);
        goto error;
    }

    /*
     * Are we attempting remote rendering?  Can only do that using OpenGL or SGI GL.
     * Check on pass 2 so we know what graphics library we are using
     */
    if(Pass == 2 && !EQLSTR(pFormat->localHost,pFormat->fullHost)) {
        /*
         * If so, don't do BackingStore
         */
        BackingStore = FALSE;

#if !defined(sgi)

        if (isOpenGL == 0) /* then we DID NOT load openGL and
            			     we can only do distributed rendering
            			     on an SGI
            			     */
        {
            DXSetError(ERROR_NOT_IMPLEMENTED, "Cannot do remote hardware rendering");
            goto error;
        }
#endif

    }

    /* Make sure the DISPLAY env variable is set to what we intend to use */
    {
        char* env_dis = (char*)getenv("DISPLAY");
        if(env_dis && !EQLSTR(env_dis,pFormat->where))
        {
            /* reset the defaults DISPLAY */
            sprintf(newDisplay,"DISPLAY=%s",pFormat->where);
            putenv(newDisplay);
        }
    }

#endif

    EXIT(("OK"));
    return OK;
error:

    EXIT(("ERROR"));
    return ERROR;
}

/*
 *  There are three ways for the graphics window to be deleted (and the
 *  render instance to die).
 *
 *  	o The window manager kills the window, _dxfProcessEvents()
 *  	  fields the notification and deletes the cache.
 *  	o _dxfProcessEvents() gets a WindowDestroy event and deletes the cache.
 *  	o The user or UI cause an executive `flushdictionary' command
 *  	  to be executed, flushing the cache.
 *
 *  Deleting the cache entry which references the tdm globals invokes
 *  _dxfEndRenderModule() as a callback to cleanup window resources.
 *  _dxfAsyncDelete() is no longer used to destroy the window; we now unmap
 *  it instead.
 */

/*
 * This function is called each time the Display module
 * (i.e. the software renderer) determines that it will be
 * rendering the image.  That is, whenever HW rendering is
 * not an attribute of the object
 */
Error
_dxfAsyncDelete (char *where)
{
    char *cacheId ;
    Private cacheObject ;
    tdmChildGlobalP globals ;
    tdmParsedFormatT *format;

    DEBUG_MARKER("_dxfAsyncDelete ENTRY");
    ENTRY(("_dxfAsyncDelete (\"%s\")",where));

    if(!(format = _tdmParseDisplayString (where, &cacheId))) {
        EXIT(("ERROR"));
        DEBUG_MARKER("_dxfAsyncDelete EXIT");
        return ERROR;
    } else
        _dxfDeleteParsedDisplayString(format);

    /* NOTE:
     * The only way for us to know if this window is doing HW rendering
     * is to look in the cache using the 'where' parameter. If we find
      * a match and the window is mapped, unmap it. 
     *
     * If the entry is not in cache HW has never been done or the 
     * instance has been deleted. In either case this is not an error.
     */
    if ((cacheObject = (Private) DXGetCacheEntry (cacheId, 0, 0))!=NULL) {
        globals = (tdmChildGlobalP) DXGetPrivateData(cacheObject) ;
        {
            DEFGLOBALDATA(globals) ;
            /* delete the reference created by DXGetCacheEntry() */
            DXDelete((Pointer)cacheObject) ;

            if (MAPPED) {
                /* unmap the graphics window */
                PRINT(("unmapping window %d", XWINID));
#if !defined(DX_NATIVE_WINDOWS)

                XUnmapWindow(DPY, XWINID) ;
                XFlush(DPY) ;
#endif

                MAPPED = 0 ;
            }
        }
    } else {
        PRINT(("instance not in cache"));
    }

    tdmFree(cacheId) ;
    EXIT(("OK"));
    DEBUG_MARKER("_dxfAsyncDelete EXIT");
    return OK ;
}

static char*
_token(char* str, char* delimit, char* chaff, int inPlace)
{
    /* Find the end of the first token */
    int		offset;
    static char	*savedStr;
    char		*ret;
    int		tmp;

    /*
     * This gets called a lot so don't trace it... 
     * ENTRY(("_token(\"%s\", \"%s\", \"%s\", %d)",
     *        str, delimit, chaff, inPlace));
     */

    if(!str && !savedStr) {
        /* EXIT(("!str && !savedStr")); */
        return NULL;
    }

    if(!str)
        str = savedStr;

    offset = strcspn(str,delimit);
    savedStr = str + (offset + (int)(str[offset] != '\0')) ;

    /* create the return pointer */
    if(inPlace) {
        ret = str;
        ret[offset] = '\0';
        tmp = strspn(ret,chaff);
        ret += tmp;
        ret[strcspn(ret,chaff)] = '\0' ;
    } else {
        if(!(ret = tdmAllocate(offset+1))) {
            /* EXIT(("tdmAllocate failed")); */
            return NULL;
        }
        strncpy(ret,str,offset);
        ret[offset] = '\0';
        tmp = strspn(ret,chaff);
        if(tmp)
            strcpy(ret,ret+tmp);	/* Move the text to the front of ret */
        ret[strcspn(ret,chaff)] = '\0' ;
    }

    /* EXIT(("ret = \"%s\"",ret)); */
    return ret;
}

static char*
_getFullHostName(char* givenName)
{
    char		shortName[MAXHOSTNAMELEN+1];
    struct hostent	*h_ent;
    char			*ret;

    ENTRY(("_getFullHostName(\"%s\")",givenName));

    if(!givenName) {
        EXIT(("givenName == NULL"));
        return NULL;
    }

    /*
     * Get the short version of the host name
     */
    if(EQLSTR(givenName,"") ||
            EQLSTR(givenName,"unix") ||
            EQLSTR(givenName,"localhost")) {
        if(gethostname(shortName,MAXHOSTNAMELEN)) {
            strcpy(shortName,givenName);
        }
    } else {
        strcpy(shortName,givenName);
    }

    /*
     * XXX Should check for dotted decimal notation here
     */

    /*
     * Get the full name 
     */
    h_ent = gethostbyname(shortName);
    if (! h_ent) {
        h_ent = gethostbyname(shortName);
        if (! h_ent) {
            unsigned long inaddr;
            inaddr = inet_addr(shortName);
            if (inaddr != -1)
                h_ent = gethostbyaddr((char *)&inaddr, sizeof(unsigned long), AF_INET);
        }
    }

    if (h_ent) {
        ret = tdmAllocate(strlen(h_ent->h_name)+1);
        if (! ret)
            return NULL;

        strcpy(ret,h_ent->h_name);
    } else {
        ret = tdmAllocate(strlen(shortName) + 1);
        if (! ret)
            return NULL;

        strcpy(ret,shortName);
    }

    EXIT(("ret = \"%s\"",ret));
    return ret;
}

static char*
_getFullXserver(char* givenXserver)
{
    char*	ret;

    ENTRY(("_getFullXserver(\"%s\")",givenXserver));

    if(!strchr(givenXserver,':')) {
        if(!strchr(givenXserver,'.')) {
            ret = tdmAllocate(strlen(givenXserver)+4);
            if (!ret)
                goto error;
            strcpy(ret,":");
            strcat(ret,givenXserver);
            strcat(ret,".0");
        } else {
            ret = tdmAllocate(strlen(givenXserver)+2);
            if (!ret)
                goto error;
            strcpy(ret,":");
            strcat(ret,givenXserver);
        }
    } else {
        if(!strchr(givenXserver,'.')) {
            ret = tdmAllocate(strlen(givenXserver)+3);
            if (!ret)
                goto error;
            strcpy(ret,givenXserver);
            strcat(ret,".0");
        } else {
            ret = tdmAllocate(strlen(givenXserver)+1);
            if (!ret)
                goto error;
            strcpy(ret,givenXserver);
        }
    }

    EXIT(("ret = \"%s\"",ret));
    return ret;

error:
    EXIT(("ERROR"));
    return NULL;
}

static hwFlagsT servicesFlags = 0;

hwFlags
_dxf_SERVICES_FLAGS()
{
    return &servicesFlags;
}

void *
_dxfGetStereoWindowInfo(char *where, void *lwi, void *rwi)
{
    Private           cacheObject = NULL;
    tdmChildGlobalP   globals = NULL;
    char*             cacheId = NULL;
    tdmParsedFormatT* pFormat = NULL;

    /*
     * get host and window name from display string, fill in the cache id
     */
    if (!(pFormat = _tdmParseDisplayString (where, &cacheId)))
        goto error;

    /*
     * validate the display string
     */
    if (!(_validateDisplayString(pFormat, 1)))
        goto error ;

    /*
     * Get a pointer to the global data
     */
    cacheObject = (Private) DXGetCacheEntry(cacheId, 0, 0);
    if (! cacheObject) {
        DXSetError(ERROR_BAD_PARAMETER,
                   "window matching \"%s\" not found", where);
        goto error;
    } else {
        globals = (tdmChildGlobalP) DXGetPrivateData(cacheObject) ;
        {
            DEFGLOBALDATA(globals) ;
            if (lwi)
                *(WindowInfo *)lwi = LEFTWINDOWINFO;
            if (rwi)
                *(WindowInfo *)rwi = RIGHTWINDOWINFO;
        }
    }

    return (void *)globals;

error:
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);

    if (cacheId)
        tdmFree(cacheId);

    if(cacheObject)
        DXDelete((Pointer)cacheObject) ;

    return NULL;
}

