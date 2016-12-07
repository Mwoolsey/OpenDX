/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>

/* 
 * this file is NOT used with the normal exec;  it is only linked in
 * with outboard modules.
 */

#include <string.h>

#if  defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if  defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_SYS_FILIO_H)
#include <sys/filio.h>
#endif

typedef Error (*PFE)();
typedef int (*PFI)();

#if DXD_NEEDS_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

Error _dxf_RegisterInputHandler (PFE proc, int fd, Pointer arg);

int _dxd_exOutboard = FALSE;
PFE _dxd_registerIHProc = _dxf_RegisterInputHandler;

extern Object _dxfExportBin_FP(Object o, int fd);
extern Object _dxfImportBin_FP(int fd);

extern Error _dxfSetOutboardMessage(int SockFD);
extern Error _dxfResetOutboardMessage();

int DXCheckRIH (int block); 
static Error DXInternalReadyToRun();

/* status of connection back to host exec
 */
#define HOST_CONNECTED 0
#define HOST_DATAREADY 1
#define HOST_CLOSED   -1
static int host_status = HOST_CLOSED;
static int host_socket = -1;
static Error datafromhost(int dxfd, Pointer arg);

/* for init stuff */
static int firsttime = 1;

static Error
callsetup (int dxfd)
{
    /*
     * Initialize the library
     */
    
    DXSetErrorExit (0);   /* have to do this before init is called */
    
    if (! DXinitdx ())
	return ERROR;
    
    /* save this for later and set the state */
    host_socket = dxfd;
    host_status = HOST_CONNECTED;
    
    /*
     * Initialize stuff for the message routines.
     */
    if (! _dxfSetOutboardMessage (dxfd))
	return ERROR;
    
    /*
     * register the socket back to the exec 
     */
    if (!DXRegisterInputHandler (datafromhost, dxfd, NULL))
	return ERROR;
    
    firsttime = 0;
    return OK;
}

/* 
 * return if user's RIH indicates something on a pipe, or if
 * the exec has sent something.
 */
int DXInputAvailable (int dxfd)
{
    if (firsttime && ! callsetup(dxfd)) {
	host_status = HOST_CLOSED;
	return OK;
    }

    while (host_status != HOST_CLOSED) {
	
	/* blocks here.
	 */
	DXCheckRIH (1);
	
	if (host_status == HOST_DATAREADY)
	    return 1;
    }
    
    /* connection to host closed */
    return -1;
}

/* return whether:
 *  1 = input is available
 *  0 = input is not available
 * -1 = connection is closed or other error 
 */
int DXIsInputAvailable (int dxfd)
{
    if (firsttime && ! callsetup(dxfd)) {
	host_status = HOST_CLOSED;
	return OK;
    }

    if (host_status != HOST_CLOSED) {
	
	/* does not block here.
	 */
	DXCheckRIH (0);
	
	if (host_status == HOST_DATAREADY)
	    return 1;

	/* data not ready */
	return 0;
    }
    
    /* connection to host closed */
    return -1;
}


/* 
 * monitor the connection to the host and make sure you know when
 * something is there
 */
static 
Error datafromhost (int dxfd, Pointer arg)
{
    int 	b;

    /* if the pipe is closed, set the status
     */
    if ((IOCTL(dxfd, FIONREAD, (char *)&b) < 0) || (b <= 0)) {
	host_status = HOST_CLOSED;
	return OK;
    }

    host_status = HOST_DATAREADY;
    return OK;
}


Error DXCallOutboard (PFE m, int dxfd)
{
    static int  firsttime = 1;
    Array	oarr;
    int		nin	= 0;
    int		nout	= 0;
    Object	*ilist = NULL;
    Object	*olist = NULL;
    Error	ret    = ERROR;
    Error	modret;
    ErrorCode	ecode;
    char        *emessptr = NULL;
    Group	iobj	= NULL;
    Group	oobj	= NULL;
    Array	code	= NULL;
    String	mess	= NULL;
    int		i;
    int		*iptr;
    int		count	= 0;
    int		one = 1;
    int         zero = 0;


    if (firsttime && ! callsetup (dxfd)) {
	host_status = HOST_CLOSED;
	return ERROR;
    }

	
    /*
     * Import the remote object, extract the number of inputs and outputs,
     * and rip them apart appropriately.  group members are: 
     * input parm count, 
     * input object list (the pointers values don't mean anything in this
     * address space; the interesting part is whether they are NULL or not),
     * the output parm count,
     * and then each input object which isn't null.
     */

    iobj = (Group) _dxfImportBin_FP(dxfd);
    if(iobj == NULL)
        goto finish_up;

    if (!DXExtractInteger (DXGetEnumeratedMember (iobj, 0, NULL), &nin))
	goto finish_up;

    ilist = (Object *) DXAllocateZero(sizeof (Object) * nin);
    if (!ilist)
	goto finish_up;
    
    iptr = (int *)DXGetArrayData((Array)DXGetEnumeratedMember (iobj, 1, NULL));

    if (!DXExtractInteger (DXGetEnumeratedMember (iobj, 2, NULL), &nout))
	goto finish_up;

    count = 3;
    for (i=0; i<nin; i++) 
    {
	if (iptr[i] == (int)NULL)
	    continue;

	ilist[i] = DXGetEnumeratedMember(iobj, count++, NULL);
    }

    olist = (Object *) DXAllocateZero(sizeof (Object) * nout);
    if (!olist)
	goto finish_up;

    /*
     * Call the module, and save the error code if set.
     */

    DXResetError();
    
    _dxd_exOutboard = TRUE;
    modret = m (ilist, olist);
    _dxd_exOutboard = FALSE;

    /*
     * get these now, before we do something else which overwrites them.
     */
    ecode = DXGetError ();
    emessptr = DXGetErrorMessage();

    /* 
     * now BEFORE we do anything which allocates memory or new objects
     * check the return objects for validity.  we saw a case where the
     * object being returned was deleted, and then the DXNewGroup() 
     * below got the exact same address allocated, so when we put the
     * group together we put a reference to the parent group into the
     * same group as a child.  bad things followed...
     *
     * (the two calls above to geterror & geterrormsg don't allocate anything;
     * they return a value & ptr to a static respectively)
     */
    for (i = 0; i < nout; i++) {
	if (olist[i] == NULL)
	    continue;

	switch (DXGetObjectClass(olist[i])) {
	  case CLASS_DELETED:
	  case CLASS_MIN:
	  case CLASS_MAX:
	    if (ecode == ERROR_NONE) {
		DXSetError(ERROR_BAD_CLASS, "bad object returned as output %d from outboard module", i);
		ecode = DXGetError ();
		emessptr = DXGetErrorMessage();
	    } else
		DXAddMessage("bad object returned as output %d from outboard module", i);

	    olist[i] = NULL;
	  default: /* Lots of other classes */
	    break;
	}
    }

    /*
     * Set up for return, at least the return code and message.
     */
    
    oobj = DXNewGroup ();
    if (oobj == NULL)
	goto finish_up;

    if (!(code = DXNewArray (TYPE_INT, CATEGORY_REAL, 0)))
	goto finish_up;

    if (! DXAddArrayData (code, 0, 1, (Pointer) &ecode))
	goto finish_up;

    mess = DXNewString (emessptr);
    if (mess == NULL)
	goto finish_up;

    if (! DXSetEnumeratedMember (oobj, 0, (Object) code) ||
	! DXSetEnumeratedMember (oobj, 1, (Object) mess))
	goto finish_up;

    /*
     * If everything is OK then go ahead and return any objects too.
     */

    if (modret == OK)
    {
	/* send output list so the caller can tell which outputs
	 * were set.  only send the non-NULL ones.
	 */
	oarr = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!oarr)
	    goto finish_up;
	for (i = 0; i < nout; i++) {
	    if (! DXAddArrayData(oarr, i, 1, (olist+i) ? 
                                 (Pointer)&one : (Pointer)&zero))
		goto finish_up;
	}
	
	if (! DXSetEnumeratedMember(oobj, 2, (Object)oarr))
	    goto finish_up;
	

	count = 3;
        for (i = 0; i < nout; i++)
        {
	    if (olist[i] == NULL)
		continue;

	    if (! DXSetEnumeratedMember (oobj, count++, olist[i]))
		goto finish_up;

	}
    }

    if (!_dxfExportBin_FP ((Object)oobj, dxfd))
	goto finish_up;

    /* 
     * if you get to this point, there were no other errors.
     */
    ret = OK;


finish_up:

    /*
     * get rid of space not needed anymore.  this doesn't matter for
     * one-shots, but for persistent modules we will run out of memory
     * eventually if these aren't deleted.
     */

    DXDelete ((Object) iobj);
    DXDelete ((Object) oobj);

    DXFree ((Pointer) ilist);
    DXFree ((Pointer) olist);

    return ret;
}


static int number_of_outputs = 0;       /* this is a hack for now */
static Object last_inputs = NULL;       /* ditto */
static int in_module = 0;               /* if set, exec waiting for output */

Error DXGetInputs(Object *in, int dxfd)
{
    static int  firsttime = 1;
    int		nin	= 0;
    Group	iobj	= NULL;
    int		i;
    int		*iptr;
    int		count	= 0;


    if (firsttime && ! callsetup (dxfd)) {
	host_status = HOST_CLOSED;
	return ERROR;
    }

    /*
     * Import the remote object, extract the number of inputs and outputs,
     * and rip them apart appropriately.  group members are: 
     * input parm count, 
     * input object list (the pointers values don't mean anything in this
     * address space; the interesting part is whether they are NULL or not),
     * the output parm count,
     * and then each input object which isn't null.
     */

    iobj = (Group) _dxfImportBin_FP(dxfd);
    if(iobj == NULL)
        goto error;

    if (!DXExtractInteger (DXGetEnumeratedMember (iobj, 0, NULL), &nin))
	goto error;

    memset(in, '\0', sizeof(Object)*nin);
    iptr = (int *)DXGetArrayData((Array)DXGetEnumeratedMember (iobj, 1, NULL));

    /* was nout */
    if (!DXExtractInteger (DXGetEnumeratedMember (iobj, 2, NULL), 
			   &number_of_outputs))
	goto error;

    count = 3;
    for (i=0; i<nin; i++) 
    {
	if (iptr[i] == (int)NULL)
	    continue;

	in[i] = DXGetEnumeratedMember(iobj, count++, NULL);
    }

    /* when does iobj get freed? */
    /* and how does nout get saved? */

    last_inputs = (Object)iobj;
    in_module++;
    return OK;

  error:
    DXDelete ((Object) iobj);
    last_inputs = NULL;
    return ERROR;
}

/* get rid of last input object set */
Error DXFreeInputs()
{
    if (last_inputs) {
	DXDelete (last_inputs);
	last_inputs = NULL;
    }
    return OK;
}

/* return outputs from module asynchronously
 */
Error DXSetOutputs(Object *olist, int dxfd)
{
    static int  firsttime = 1;
    Array	oarr;
    Error	ret    = ERROR;
    ErrorCode	ecode;
    Group	iobj	= NULL;
    Group	oobj	= NULL;
    Array	code	= NULL;
    String	mess	= NULL;
    int		i;
    int		count	= 0;
    int		one = 1;
    int         zero = 0;


    if (firsttime && ! callsetup (dxfd)) {
	host_status = HOST_CLOSED;
	return ERROR;
    }

	
    ecode = DXGetError();

    /*
     * Set up for return, at least the return code and message.
     */
    
    oobj = DXNewGroup ();
    if (oobj == NULL)
	goto finish_up;

    if (!(code = DXNewArray (TYPE_INT, CATEGORY_REAL, 0)))
	goto finish_up;

    if (! DXAddArrayData (code, 0, 1, (Pointer) &ecode))
	goto finish_up;

    mess = DXNewString (DXGetErrorMessage ());
    if (mess == NULL)
	goto finish_up;

    if (! DXSetEnumeratedMember (oobj, 0, (Object) code) ||
	! DXSetEnumeratedMember (oobj, 1, (Object) mess))
	goto finish_up;

    /*
     * If everything is OK then go ahead and return any objects too.
     */

    if (ecode == ERROR_NONE)
    {
	/* send output list so the caller can tell which outputs
	 * were set.  only send the non-NULL ones.
	 */
	oarr = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!oarr)
	    goto finish_up;
	for (i = 0; i < number_of_outputs; i++) {
	    if (! DXAddArrayData(oarr, i, 1, (olist+i) ? 
                                 (Pointer)&one : (Pointer)&zero))
		goto finish_up;
	}
	
	if (! DXSetEnumeratedMember(oobj, 2, (Object)oarr))
	    goto finish_up;
	

	count = 3;
        for (i = 0; i < number_of_outputs; i++)
        {
	    if (olist[i] == NULL)
		continue;

	    if (! DXSetEnumeratedMember (oobj, count++, olist[i]))
		goto finish_up;

	}
    }

    /* if the exec isn't already sitting there waiting for results,
     *  alert it that something is going to come back down the pipe,
     *  wait for it to be called and eat the new inputs, and send
     *  the new outputs.
     */
    if (!in_module) {
	DXInternalReadyToRun();
	
	iobj = (Group) _dxfImportBin_FP (dxfd);
	if (iobj == NULL)
	    goto finish_up;
	DXDelete ((Object)iobj);
    }

    if (!_dxfExportBin_FP ((Object)oobj, dxfd))
	goto finish_up;

    /* 
     * if you get to this point, there were no other errors.
     */
    ret = OK;


finish_up:

    /*
     * get rid of space not needed anymore.  this doesn't matter for
     * one-shots, but for persistent modules we will run out of memory
     * eventually if these aren't deleted.
     */
    DXDelete ((Object) oobj);
    in_module = 0;

    return ret;
}


/* Do NOT make this > 32 */
#define	MAXRIH		32

typedef struct
{
    PFE		proc;
    PFI		check;
    int 	flag;
    int		fd;
    Pointer	arg;
} _EXRIH, *EXRIH;


static int		nRIH	= 0;
static _EXRIH		handlers[MAXRIH];
static lock_type	*rih_lock;
static int		*rih_count;


static Error RIHDelete (int fd);
static Error RIHInsert (Error (*proc) (int, Pointer),
		int fd, Pointer arg, int (*check) (int, Pointer));


/* is this necessary?  it apparently wasn't being called before...
 */
Error _dxf_initRIH (void)
{
    rih_lock = (lock_type *) DXAllocate (sizeof (lock_type));
    if (rih_lock == NULL)
	return (ERROR);
    if (DXcreate_lock (rih_lock, "input handlers") != OK)
	    return (ERROR);

    rih_count = (int *) DXAllocate (sizeof (int));
    if (rih_count == NULL)
	return (ERROR);
    *rih_count = 0;

    return (OK);
}

Error _dxf_RegisterInputHandler (Error (*proc) (int, Pointer), int fd, Pointer arg)
{
    return ((proc == NULL) ? RIHDelete (fd) : RIHInsert (proc, fd, arg, NULL));
}


Error DXRegisterInputHandler (Error (*proc) (int, Pointer), int fd, Pointer arg)
{
   return (_dxd_registerIHProc(proc, fd, arg));
}


Error
DXRegisterInputHandlerWithCheckProc(Error(*proc)(int, Pointer),
			int (*check)(int, Pointer), int fd, Pointer arg)
{
    /* check proc not needed with call module */
    return DXRegisterInputHandler(proc, fd, arg);
}


/* see the comments in dpexec/rih.c for more info on this.
 *  they should stay in sync.
 */

int DXCheckRIH (int block)
{
    int			i;
    fd_set		fdset;
    EXRIH		r;
    struct timeval	tv;
    int			ret	= FALSE;
    /*Error		rval;*/
    int			mval;
    int			cfnd;
    int			fd;

    if (nRIH <= 0)
	return (ret);

    for (cfnd = 0, i = 0, r = handlers; i < nRIH; i++, r++)
    {
	if (r->check)
	{
	    r->flag = (* r->check) (r->fd, r->arg);
	    if (! cfnd)
		cfnd = r->flag;
	}
	else
	    r->flag = 0;
    }

    FD_ZERO(&fdset);

    for (i = 0, r = handlers, mval = 0; i < nRIH; i++, r++)
    {
	fd = r->fd;
	if (mval < fd)
	    mval = fd;
	FD_SET(fd, &fdset);
    }

    /*
     * If check input is found, don't block
     */
    if (cfnd)
    {
	tv.tv_sec  = 0;
	tv.tv_usec = 0;
	select (mval + 1, &fdset, NULL, NULL, &tv);
    }
    else
    {
	/*
	 * blocking or non-blocking select, depending on the parm
	 */
	if (block == 0)
	{
	    tv.tv_sec  = 0;
	    tv.tv_usec = 0;
	    if (select (mval + 1, &fdset, NULL, NULL, &tv) <= 0)
		return (ret);
	}
	else
	{
	    if (select (mval + 1, &fdset, NULL, NULL, NULL) <= 0)
		return (ret);
	}
    }

    for (i = 0, r = handlers; i < nRIH; i++, r++)
    {
	fd = r->fd;
	if (r->flag || FD_ISSET(fd, &fdset))
	{
	    /*rval = (* r->proc) (fd, r->arg);*/
	    (* r->proc) (fd, r->arg);
	    ret = TRUE;
	}
    }

    return (ret);
}


static Error RIHDelete (int fd)
{
    int		i;
    int		j;
    EXRIH	r;
    EXRIH	nr;

    /*
     * Look through all of the registered input handlers.  If we find the
     * one associated with this file descriptor then close up the ranks.
     */

    for (i = 0, r = handlers; i < nRIH; i++, r++)
    {
	if (r->fd == fd)
	{
	    for (j = i + 1, nr = r + 1; j < nRIH; j++, r++, nr++)
		*r = *nr;
	    nRIH--;
	    return (OK);
	}
    }

    return (OK);
}


static Error RIHInsert (Error (*proc) (int, Pointer), int fd, Pointer arg, 
		int (*check)(int, Pointer))
{
    int		i;
    EXRIH	r;

    /*
     * See if they are just changing the input handler.  If so then
     * remember the new data.
     */

    for (i = 0, r = handlers; i < nRIH; i++, r++)
    {
	if (r->fd == fd)
	{
	    r->check = check;
	    r->proc  = proc;
	    r->arg   = arg;
	    return (OK);
	}
    }

    if (nRIH >= MAXRIH)
	DXErrorReturn (ERROR_INTERNAL, "too many input handlers registered");

    /*
     * Looks like we are installing a new one.
     */

    r = &handlers[nRIH++];

    r->proc  = proc;
    r->check = check;
    r->fd    = fd;
    r->arg   = arg;

    return (OK);
}



/* standalone versions of these routines.  the real ones are in 
 *  the exec.  these just need to stuff something down the socket
 *  to alert the exec we are ready to run.
 */
Pointer DXGetModuleId()
{
    Pointer id;

    id = DXAllocate(sizeof(int));
    if (!id)
	return NULL;

    *(int *)id = host_socket;
    return id;
}

Error DXReadyToRunNoExecute(Pointer ida)
{
    return OK;
}


Error DXReadyToRun(Pointer id)
{
    unsigned int signature = 0xEF33AB88;

    if (!id)
	return ERROR;

    if (write(*(int *)id, (char *)&signature, sizeof(int)) == -1)
        return ERROR;

    return OK;
}

/* private version */
static Error DXInternalReadyToRun()
{
#if 1
    unsigned int signature = 0xEF33AB88;
#else
    unsigned int signature = 0x12EE44CD;
#endif

    if (write(host_socket, (char *)&signature, sizeof(int)) == -1)
        return ERROR;

    return OK;
}

Error DXFreeModuleId(Pointer id)
{
    if (!id)
	return OK;

    DXFree(id);
    return OK;
}

