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
#include <stdlib.h>
#include <stdarg.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_WAIT_H)
#include <wait.h>
#endif
#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif
#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif
#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
#if defined(HAVE_SYS_TIMES_H)
#include <sys/time.h>
#endif
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_FILIO_H)
#include <sys/filio.h>
#endif
#if defined(HAVE_IO_H)
#include <io.h>
#endif
#if defined(HAVE_WINIOCTL_H)
#include <winioctl.h>
#endif
#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif
#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#include "remote.h"
#include "pmodflags.h"
#include "obmodule.h"
#include "config.h"
#include "utils.h"
#include "parse.h"
#include "distp.h"
#include "context.h"
#include "_macro.h"
#include "_variable.h"
#include "ccm.h"
#include "graph.h"

extern Object _dxfExportBin_FP(Object o, int fd); /* from libdx/rwobject.c */
extern Object _dxfImportBin_FP(int fd); /* from libdx/rwobject.c */
extern int _dxfHostIsLocal(char *host); /* from libdx/ */

#define MAX_STARTUP_ARGS 100

#define	BUFLEN	4096

Error ExHostToFQDN( const char host[], char fqdn[MAXHOSTNAMELEN] )
{
    long addr;
    struct hostent *hp, *hp2;

    hp = gethostbyname(host);
    if ( hp == NULL || hp->h_addr_list[0] == NULL ) {
       DXUIMessage("WARNING", "gethostbyname returned error");
       DXUIMessage("WARNING", "it appears %s doesn't have a host entry?", host);
       DXUIMessage("WARNING", "trying localhost.");
       addr = 0x7f000001; /* 127.0.0.1 */
    } else 
    	addr = *(long *)hp->h_addr_list[0];

    hp2 = gethostbyaddr((const char *)&addr, sizeof(struct in_addr),
                        AF_INET );
    if ( hp2 == NULL || hp2->h_name == NULL ) {
        if(_dxfHostIsLocal((char *)host)) {
            strncpy( fqdn, "localhost", MAXHOSTNAMELEN );
            DXMessage("Warning: Cannot get FQDN; no DNS entry. Assuming localhost.");
            return OK;
        }
        DXUIMessage("ERROR", "gethostbyaddr returned error");
        DXSetError(ERROR_UNEXPECTED, "gethostbyaddr error--is it possible this machine doesn't have a DNS/host entry?");
        return ERROR;
    }
    strncpy( fqdn, hp2->h_name, MAXHOSTNAMELEN );
    return OK;
}


/* This routine returns the pid of the forked child to communicate with (or
 * 0 if no waiting is required),
 * or, if failure, -1.  It (for now) also does a perror.  It also returns
 * remin, remout, and remerr, stdin,out,err for the created task.
 */
int
ExConnectTo(char *host, char *user, char *cwd, int ac, char *av[], char *ep[], 
    char *spath, int *remin, int *remout, int *remerr)
{
 #if defined(HAVE_FORK) 
   char s[BUFSIZ];
    char script_name[500],cmd[1000];
    char localhost[MAXHOSTNAMELEN];
    FILE *fp = NULL;
    int i, k;
    char **rep;
    char *fargv[MAX_STARTUP_ARGS];
    struct hostent *he;
    int findx;
    char *pathEnv;
    int  j;
    int ret;
    int found;
    int rsh_noenv;
    char *local_rsh_cmd;
#if ! defined(DXD_HAS_LIBIOP)
    int in[2], out[2], err[2];
    int child;
#endif


#if HAVE_GETDTABLESIZE
    int  width = getdtablesize();
#else
#ifdef DXD_HAS_WINSOCKETS
    int  width = FD_SETSIZE;
#else
    int  width = MAXFUPLIM;
#endif
#endif
    char *dnum;

    local_rsh_cmd = getenv( "DXRSH" );
    if ( !local_rsh_cmd )
      local_rsh_cmd = RSH;

    /*
     * Initialize return values (to default negative results).
     */
    if (remin)
	*remin  = -1;
    if (remout)
	*remout = -1;
    if (remerr)
	*remerr = -1;

    /*
     * Check to see if "host" is a valid hostname.
     */
    if (strcmp("unix", host) != 0)
    {
	he = gethostbyname (host);
	if (he == NULL)
	{
	    /* herror (host); */
	    return (-1);
	}
    }

    for (pathEnv = NULL, i = 0; ep[i] && pathEnv == NULL; ++i)
	if (    strncmp (ep[i], "PATH=", strlen("PATH=")) == 0 ||
		strncmp (ep[i], "PATH =", strlen("PATH =")) == 0) 
	    pathEnv = ep[i];

    if (_dxfHostIsLocal(host)) {
	char *path;
	struct stat sbuffer;

	if (user != NULL)
	    fprintf (stdout, "Different user on local machine ignored\n");

        if(ac > MAX_STARTUP_ARGS) {
            DXUIMessage("ERROR", "number of arguments exceeds max");
            goto error_return;
        }
	for (i = 0; i < ac; ++i) 
	    fargv[i] = av[i];
	fargv[i] = 0;
        findx = ac+1;
	rep = ep;
        /* First search directories in spath for specified command. Then
	 * scan through ep looking for "PATH".  If "PATH" is found, then
	 * look through path for the specified command, and if it's
	 * found and executable, replace it in fargv with the expanded 
	 * path name.
	 */
#if DXD_HAS_LIBIOP
	if (*fargv[0] != '.' && *fargv[0] != '/' && 
            strncmp(fargv[0], "os ", 3) != 0 ) {
#else 
	if (*fargv[0] != '.' && *fargv[0] != '/') {
#endif
            found = FALSE;
            for(k = 0; k < 2; k++) { 
                if(k == 0)
                    path = spath;
                else {
	            path = strchr (pathEnv, '=');
                    if(path == NULL)
                        path = ".";
                    else path++;
                }
	        while (*path) {
		    for (i = 0; *path != '\0' && *path != ':'; ++i, ++path) 
		        s[i] = *path;
	 	    if (*path == ':') 
		        ++path;
		    s[i] = '\0';
		    strcat (s, "/");
		    strcat (s, av[0]);
		    if (stat (s, &sbuffer) < 0) {
		        /* if the file doesn't exist, go on */
		        if (errno == ENOENT) 
			    /* We have no more paths to search */
			    if (*path == '\0' && k == 1)  {
                                DXUIMessage("ERROR", "can't find %s", av[0]);
			        goto error_return;
                            }
		        /* Ignore bad stats, we just can't find it */
		        continue;
		    }
		    /* If it's executable, and a regular file, we're done. */
		    if ((sbuffer.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) != 0 &&
		        (sbuffer.st_mode & S_IFREG) != 0) {
                        found = TRUE;
		        break;	/* out of while(*path) */
                    }
                }
                if(found)
                    break;
	    }
	    fargv[0] = s;
	}
    }
    else {		/* Host is NOT local */
#if DXD_POPEN_OK
	/* The general thread here is to first create a script on the
	 * remote host using popen().  The script contains shell code to
	 *   1) set up a similar environment to the current one (PATH,...)
	 *   2) exec 'dx -...' 
	 *   3) remove the script from the remote file system.
	 * Then we set up fargv to contain the following: 
	 *    rsh host [ -l user ] /bin/sh /tmp/dx-$host:$pid 
	 */ 

        if(gethostname(localhost, sizeof(localhost)) < 0) {
            DXUIMessage("ERROR", "gethostname returned error");
            goto error_return;	
        }
       if ( ExHostToFQDN( localhost, localhost ) == ERROR )
            goto error_return;

	sprintf(script_name,"/tmp/dx-%s:%d",localhost,getpid());
	findx=0;
	fargv[findx++] = local_rsh_cmd;
	fargv[findx++] = host;

	if (user != NULL) {
	    fargv[findx++] = "-l";
	    fargv[findx++] = user;
	    sprintf(cmd,"%s %s -l %s 'cat > %s'", local_rsh_cmd,
		 host, user, script_name);
	} else
	    sprintf(cmd,"%s %s 'cat > %s'", local_rsh_cmd,
		host,script_name);

	fargv[findx++] = "/bin/sh";
	fargv[findx++] = script_name; 
        fargv[findx++] = ">";
        fargv[findx++] = "/dev/null";
	fargv[findx++] = NULL ;
		
	fp = popen(cmd,"w");
	if (!fp) {
		perror("popen");
		goto error_return;
	}
	setbuf(fp,NULL); /* Unbuffered so file is 'immediately' updated */
	
        rsh_noenv = getenv( "DXRSH_NOENV" ) != NULL; /*  Set $DISPLAY only?  */

	for (i = 0; ep[i]; ++i) {
	    /* Environment variables which are NOT passed to remote process */
	    static char *eignore[] = {"HOST=", "HOSTNAME=", "TZ=",
                                      "SHELL=", "DXHOST=", "ARCH=", 0}; 
	    int ignore;
	    
	    /* Look for and skip certain environment variables */
	    /* We could do this more quickly elsewhere, but ... */ 
	    for (ignore=j=0 ; !ignore && eignore[j] ; j++)
		if (!strncmp(ep[i], eignore[j], strlen(eignore[j])))
		   ignore = 1;
	    if (ignore)
		continue;

	    /* 
	     * Write the environment variable to the remote script file 
	     */
	    if ((strncmp(ep[i],"DISPLAY=unix:",strlen("DISPLAY=unix:"))==0 ||
		 strncmp(ep[i],"DISPLAY=:",    strlen("DISPLAY=:")) == 0) &&
		(dnum = strchr(ep[i], ':')) != NULL)
	    {
		fprintf(fp,"DISPLAY='%s%s';export DISPLAY\n",
							localhost,dnum);
	    } else if ( !rsh_noenv ) {
		int k;
		char evar[256], c;
		char eval[1024];
	
		for (j=0 ; ep[i][j] && (ep[i][j] != '=') ; j++)
			evar[j] = ep[i][j];
		evar[j] = '\0';
		k = j + 1;	/* Skip the '=' sign */
		for (j=0 ; (c=ep[i][k]); k++, j++) {
			if (c == '\'') {		/* ' -> '\'' */
				/* Value contains a double quote */
				eval[j++] = '\'';
				eval[j++] = '\\';
				eval[j++] = '\'';
			} 
			eval[j] = c; 
		}
		eval[j] = '\0';
                if(strcmp(evar, "PATH")) 
	            fprintf(fp,"%s='%s'; export %s\n", evar, eval, evar);
                else
	            fprintf(fp,"%s=$PATH:'%s'; export %s\n", evar, eval, evar);
	    }
	}
        /* replace PATH with new search path for outboard module */
        if(spath)
	    fprintf(fp,"PATH='%s':$PATH; export PATH\n", spath);

        if(cwd != NULL) {
            fprintf(fp, "\nif [ -d %s ]; then\n", cwd);
            fprintf(fp, " cd %s\n", cwd);
            fprintf(fp, "else\n echo cannot change directory to %s\n", cwd);
            fprintf(fp, "fi \n");
        }

	fprintf(fp,"\n(sleep 15; rm -f %s) &\nexec ", script_name);

	for (i = 0; i < ac; ++i)
	    fprintf(fp, "%s ",av[i]);
	fprintf(fp,"\n");
	ret = pclose(fp);
	if (ret == -1) {
	    perror("popen/pclose");
	    goto error_return;
	}
        else if(ret != 0)
            goto error_return;
		
	rep = ep;
#else
      DXUIMessage("ERROR", "popen is unavailable on this platform");
      goto error_return;	
#endif
    }

#if DXD_HAS_LIBIOP
    {
    char cmdpvs[1000];
    strcpy(cmdpvs, fargv[0]);
    for(i = 1; i < findx-1; i++) {
        strcat(cmdpvs, " ");
        strcat(cmdpvs, fargv[i]);
    }
    strcat(cmdpvs, "&");
    ExDebug("*2", "cmd to run %s", cmdpvs);
    system(cmdpvs);
    return(1);
    }
#elif sgi
#ifdef OLDSTUFF
    if(pcreateve(fargv[0], fargv, rep) < 0) {
        perror("pcreateve");
        exit(1);
    }
#else
    {
#if DXD_HAS_VFORK
        pid_t pid = vfork();
#else
        pid_t pid = fork();
#endif
       if ( pid < 0 ) {
           perror("fork()");
           exit(1);
       }
       else if ( pid == 0 ) {
           execve( fargv[0], fargv, rep );
           perror("execve()");
           exit(1);
       }
    }
#endif
    return(1);

#else
    /* Set up three pipes */
    if (remin && pipe(in) < 0) {
	perror("pipe(in)");
	goto error_return;
    }
    if (remout && pipe(out) < 0) {
	perror("pipe(out)");
	goto error_return;
    }
    if (remerr && pipe(err) < 0) {
	perror("pipe(err)");
	goto error_return;
    }

#if DXD_HAS_VFORK
    child = vfork();
#else
    child = fork();
#endif
    if (child == 0) {
	if (remin)
	    close (in[1]);
	if (remout)
	    close (out[0]);
	if (remerr)
	    close (err[0]);
	if (remin && dup2(in[0], 0) < 0) {
	    perror("dup2");
	    exit(1);
	}
	if (remout && dup2(out[1], 1) < 0) {
	    perror("dup2");
	    exit(1);
	}
	if (remerr && dup2(err[1], 2) < 0) {
	    perror("dup2");
	    exit(1);
	}
	/* Close all other file descriptors */
	for (i = 3; i < width; ++i) 
	    close(i);

	if (cwd != NULL && chdir (cwd) < 0) {
	    perror ("chdir");
	    exit(1);
	}
	if (execve(fargv[0], fargv, rep) < 0) {
	    perror("execve");
	    exit(1);
	}
    }
    else if (child > 0) {
	if (remin) {
	    close (in[0]);
	    *remin = in[1];
	}
	if (remout) {
	    close (out[1]);
	    *remout = out[0];
	}
	if (remerr) {
	    close (err[1]);
	    *remerr = err[0];
	}
    }
    else {
	perror("fork");
	goto error_return;
    }

    return (child);
#endif

error_return:
#endif
    return (-1);
}

Error ChildOutput(int fd, Pointer in)
{
    char buffer[512];
    int nbytes;

    nbytes = read(fd, buffer, 512);
    ExDebug("*1", "ChildOutput %d %d", fd, nbytes);
    if(nbytes <= 0) {
        /* EOF or terminate */
        DXRegisterInputHandler (NULL, fd, NULL);
        close(fd);
        return(ERROR);
    }
    else {
        buffer[nbytes] = '\0';
        DXMessage("%s: %s", (char *)in, buffer);
    }
    return(OK);
}

int _dxfExRemoteExec(int dconnect, char *host, char *ruser, int r_argc, 
                     char **r_argv, int outboard)
{
#ifdef DXD_WIN    /*   AJ   */
    return 0;
#else
    char                myhost[MAXHOSTNAMELEN];
    char		cwd[MAXPATHLEN];
    char		*spath	= NULL;
    int                 dxport, dxsock;
    int                 dxfd    = -1;
    struct sockaddr_in  dxserver;
#if DXD_SOCKET_UNIXDOMAIN_OK
    struct sockaddr_un  dxuserver;
    int			dxusock;
#endif
    int                 i;
    extern char         **environ; 
    int                 child = 0;
    int                 in = -1, out = -1, error = -1;
    char                **nargv = NULL;
    int                 nargc;

 
    printf("In _dxfExRemoteExec. Host is %s\n", host);
    if (gethostname (myhost, sizeof(myhost)) < 0)
        goto error;

    if ( ExHostToFQDN( myhost, myhost ) == ERROR)
        goto error;

    if (getcwd (cwd, sizeof (cwd)) == NULL)
	goto error;
    if (spath == NULL)
    {
	spath = getenv ("DXMODULES");
	if (spath == NULL)
	    spath = ".";
    }

    /* create the socket that will remain open between dx and module */
#if DXD_SOCKET_UNIXDOMAIN_OK
    dxport = _dxfSetupServer(OBPORT, &dxsock, &dxserver, &dxusock, &dxuserver);
#else
    dxport = _dxfSetupServer(OBPORT, &dxsock, &dxserver);
#endif
    if (dxport < 0)
        goto error;

    /* add hostname, port number and trailing NULL to arguments */   
    nargc = r_argc + 2;
 
    nargv = (char **)DXAllocateLocalZero((nargc + 1) * sizeof(char *));
    if(nargv == NULL) {
        DXUIMessage("ERROR", "Could not allocate memory for argument list");
        goto error;
    }
    for(i = 0; i < r_argc; i++)
        nargv[i] = r_argv[i];

    if(outboard) {
        nargv[r_argc] = (char *)DXAllocateLocal(
                                     (strlen(myhost)+1)*sizeof(char));
        if(nargv[r_argc] == NULL) {
            DXUIMessage("ERROR", 
                        "Could not allocate memory for argument list");
            goto error;
        }
        strcpy(nargv[r_argc], myhost);
        nargv[r_argc+1] = (char *)DXAllocateLocal(5*sizeof(char));
        if(nargv[r_argc+1] == NULL) {
            DXUIMessage("ERROR", 
                        "Could not allocate memory for argument list");
            goto error;
        }
        sprintf(nargv[r_argc+1], "%4d", dxport);
        nargv[r_argc+2] = NULL;
    }
    else {
        nargv[r_argc] = (char *)DXAllocateLocal(
                                     (strlen("-connect")+1)*sizeof(char));
        if(nargv[r_argc] == NULL) {
            DXUIMessage("ERROR", 
                        "Could not allocate memory for argument list");
            goto error;
        }
        strcpy(nargv[r_argc], "-connect");
        nargv[r_argc+1] = (char *)DXAllocateLocal(
                                  (strlen(myhost) + 6)*sizeof(char));
        if(nargv[r_argc+1] == NULL) {
            DXUIMessage("ERROR", 
                        "Could not allocate memory for argument list");
            goto error;
        }
        strcpy(nargv[r_argc+1], myhost);
        sprintf(nargv[r_argc+1]+strlen(myhost), ":%4d", dxport);
        nargv[r_argc+2] = NULL;
    }

    if(dconnect) {
        fprintf(stderr, "start on %s: ", host);
        for(i = 0; i < nargc; i++)
            fprintf(stderr, "%s ", nargv[i]);
        fprintf(stderr, "\n");    
        fflush(stderr);
    }
    else {
        if(outboard) {
            _dxfPrintConnectTimeOut(nargv[0], host);
            child = ExConnectTo(host, ruser, cwd, nargc, nargv,
                        environ, spath, &in, NULL, NULL);
        }
        else {
            _dxfPrintConnectTimeOut("Distributed DX", host);
            child = ExConnectTo(host, ruser, cwd, nargc, nargv,
                        environ, NULL, &in, &out, &error);
        }

        /* we don't need child's stdin */
        if(in >= 0)
            close(in);
        if(out >= 0) {
            ExDebug("*1", "register %d output for %s", out, host);
	    DXRegisterInputHandler (ChildOutput, out, (Pointer)host);
        }
        if(error >= 0) {
            ExDebug("*1", "register %d stderr for %s", error, host);
	    DXRegisterInputHandler (ChildOutput, error, (Pointer)host);
        }
    }

    if(child > 0 || dconnect) {
#if DXD_SOCKET_UNIXDOMAIN_OK
        /* if we are debugging do not set a timeout on connection */
        dxfd = _dxfCompleteServer(dxsock, dxserver,
                                  dxusock, dxuserver,
                                  !dconnect);
#else
        /* if we are debugging do not set a timeout on connection */
        dxfd = _dxfCompleteServer(dxsock, dxserver,
                                  !dconnect);
#endif
        if (dxfd < 0)
            goto error;
    }
    if(child == -1)
        goto error;

done:
    DXFree(nargv[r_argc]);
    DXFree(nargv[r_argc+1]);
    DXFree(nargv);
    return (dxfd);

error:
    if (dxfd >= 0)
	close (dxfd);
    goto done;

#endif
}

Error OBDelete(Pointer in)
{
    ObInfo *ob_info = (ObInfo *)in;

    close(ob_info->fd);
#ifndef DXD_HAS_LIBIOP
    if (!_dxd_exDebugConnect)
        wait(0);
#endif
    DXRegisterInputHandler (NULL, ob_info->fd, NULL);

    DXFree((Pointer)ob_info->modid);

    DXFree((Pointer)ob_info);
    return(OK);
}

Error OBAsync(int fd, Pointer in)
{
    int msb_signature = 0xEF33AB88;
    int lsb_signature = 0x88AB33EF;
    ObInfo *ob_info = (ObInfo *)in;
    int i;

    /* make sure this is from an outboard ReadyToRun routine,
     * and handle errors.
     */
    if (read(fd, &i, sizeof(i)) <= 0) {
	/* put an ioctl here to be sure?  this should be ok */
	DXRegisterInputHandler (NULL, ob_info->fd, NULL);
	ob_info->deleted = 1;
	return ERROR;
    }

    if (i == msb_signature || i == lsb_signature)
	DXReadyToRun(ob_info->modid);

    return OK;
}

#define HIDDEN_INPUTS 5    /* exec name, host name, flags, #inputs, #outputs */
#define PREDEFINED_EXP_GRP_MEMBERS 2  /* sent to outboard: #in, inlist, #out */
#define PREDEFINED_IMP_GRP_MEMBERS 3  /* returned: rc, rc_string, #out */

#define OUTERRMSG  "connection to outboard module failed"

/* all outboards call this as their module entry point.
 */
Error DXOutboard (Object *in, Object *out)
{
    return _dxfExRemote(in, out);
}

Error
_dxfExRemote (Object *in, Object *out)
{
    Error	ret	= ERROR;
    char	*cmd;
    char	*hostp;
    char	*userp;
    int		*iptr;
    int		i;
    int		nin;
    int		nout;
    int		flags;
    int		cnt;
    int		memcount;
    int		persistent = 0;
    int		async = 0;
    char	*asyvarname;
    Array	parmlist = NULL;
    Group	g	 = NULL;
    char	buff[BUFLEN];
    char	host[MAXHOSTNAMELEN];
    int		fd = -1;
    int		instance = 0;
    Object	obj;
    ErrorCode	rc;
    char	*rs	= NULL;
    ObInfo      *ob_info = NULL;
    char        *av[2];
    Object	attr;
    int		isMsg;
    Object      cachelist[2];
    char	cachetag[32];
    int 	one = 1; 
    int 	zero = 0;


    /* 
     * extract info from hidden parms.
     */
    if (! DXExtractString (in[0], &cmd))
	goto cleanup;
    if (! DXExtractString (in[1], &hostp))
	goto cleanup;
    if (! DXExtractInteger (in[2], &flags))
	goto cleanup;
    if (! DXExtractInteger (in[3], &nin))
	goto cleanup;
    if (! DXExtractInteger (in[4], &nout))
	goto cleanup;

    ExDebug("*2","DXOutboard hidden inputs: %s, %s, %d, %d, %d", 
	    cmd, hostp, flags, nin, nout);

    
    strcpy (buff, hostp);
    hostp = buff;
    userp = strchr (buff, ',');
    if (userp)
	*userp++ = '\000';
    if ( ExHostToFQDN( hostp, host ) == ERROR )
	goto cleanup;
    hostp = host;

    persistent = flags & MODULE_PERSISTENT;
    async = flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL);

    instance = _dxf_ExGetCurrentInstance();
    if (async)
	DXExtractString(in[nin-2], &asyvarname);

    nin -= HIDDEN_INPUTS;

    g = DXNewGroup ();
    if (g == NULL)
	goto cleanup;
    memcount = 0;

    /* 
     * package remaining parms into a single group object for transmission
     * to remote process.  members are: input parm count, input object list
     * (so remote process knows which are null and which are being sent),
     * the maximum number of outputs the module will produce, and then all
     * inputs which aren't null.
     */
    obj = (Object)_dxfExNewInteger (nin);

    if (! DXSetEnumeratedMember (g, memcount++, obj))
	goto cleanup;

    parmlist = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (!parmlist)
	goto cleanup;

    for (i = 0; i < nin; i++) {
	if (! DXAddArrayData(parmlist, i, 1, in[HIDDEN_INPUTS+i] ? 
			     (Pointer)&one : (Pointer)&zero))
	    goto cleanup;
    }

    if (! DXSetEnumeratedMember(g, memcount++, (Object)parmlist))
	goto cleanup;

    if (! DXSetEnumeratedMember (g, memcount++, in[4]))
	goto cleanup;
    
    for (i = HIDDEN_INPUTS; i < nin+HIDDEN_INPUTS; i++)
    {
	if (! in[i])
	    continue;
	
	if (! DXSetEnumeratedMember (g, memcount++, in[i]))
	    goto cleanup;
    }

    cachelist[0] = in[0];
    cachelist[1] = in[1];
    sprintf(cachetag, "RemoteConnect%d", instance);

    /*
     * if this is a persistent module and you have already opened the
     * socket connection, get the open file descriptor from the cache.
     * make sure the socket is still alive.
     */
    if (persistent &&
	((obj = DXGetCacheEntryV(cachetag, 0, 2, cachelist)) != NULL))
    {
        ob_info = (ObInfo *) DXGetPrivateData ((Private) obj);
        fd = ob_info->fd;
        DXDelete ((Object) obj);  /* get rid of our extra reference here */
	if (ob_info->deleted) {
	    DXSetError (ERROR_DATA_INVALID, OUTERRMSG);
	    DXSetCacheEntryV(NULL, 0, cachetag, 0, 2, cachelist);
	    goto cleanup;
	}
    }
    else 
    {
	av[0] = cmd;
	av[1] = NULL;
	
	fd = _dxfExRemoteExec (_dxd_exDebugConnect, hostp, userp, 1, av, 1);
	
        if (fd < 0)
        {
	    DXSetError (ERROR_BAD_PARAMETER,
			"connection to host %s failed", hostp);
	    goto cleanup;
        }
        ExDebug("*2", "Connected to Outboard module");

	if (persistent)
	{
	    ob_info = (ObInfo *) DXAllocateZero (sizeof(ObInfo));
	    if (!ob_info)
	    {
		close (fd);
		goto cleanup;
	    }
	    ob_info->fd = fd;
	    if (async)
		ob_info->modid = (char *)DXGetModuleId();

	    obj = (Object) DXNewPrivate ((Pointer) ob_info, OBDelete);
	    if (!obj) 
	    {
		close (fd);
		DXFree (ob_info->modid);
		DXFree (ob_info);
		goto cleanup;
	    }

	    DXSetCacheEntryV((Object)obj, CACHE_PERMANENT, 
			     cachetag, 0, 2, cachelist);
	}
    }

    /*
     * Send the data to the remote module and get rid of the packaging.
     */
    if (_dxfExportBin_FP ((Object) g, fd) == ERROR) {
	DXSetError(ERROR_DATA_INVALID, "Failed to send data to outboard module");
	goto cleanup;
    }
    ExDebug("*2", "Sending Input Objects to Outboard Module");

    DXDelete ((Object) g);
    g = NULL;


    /*
     * Import objects until you get one that's not a message or 
     * an async request to run.
     *
     */
    do {
        int b;
	isMsg = 0;
	if ((obj = _dxfImportBin_FP (fd)) == NULL) {
            fd_set fdset;
            struct timeval tv;
            int nsel;
	    DXSetError(ERROR_DATA_INVALID, "Failed to receive data from outboard module");
            FD_ZERO(&fdset);
            FD_SET(fd, &fdset);
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            nsel = select(fd + 1, (SELECT_ARG_TYPE *) &fdset, NULL, NULL, &tv);
            if(nsel > 0) {
                if((IOCTL(fd, FIONREAD, (char *)&b) < 0) || b <= 0)  {
                    DXSetError(ERROR_DATA_INVALID, 
                               "Connection to outboard module has been broken");
                    if(persistent)
                        DXSetCacheEntryV(NULL, 0, cachetag, 0, 2, cachelist);
                }
            }
	    goto cleanup;
	}
        ExDebug("*2", "Received Object from Outboard Module");
	attr = DXGetAttribute(obj, "message");
	if (attr != NULL && DXExtractInteger(attr, &isMsg) != NULL && isMsg)
	{
	    /* message */
	    if (isMsg == 1)
	    {
		Object whoObj = NULL;
		Object msgObj = NULL;
		char *who, *msg;
		va_list nolist;
		
		whoObj = DXGetEnumeratedMember((Group)obj, 0, NULL);
		if (!whoObj || !DXExtractString(whoObj, &who))
		    goto message_cleanup;
		
		msgObj = DXGetEnumeratedMember((Group)obj, 1, NULL);
		if (!msgObj || !DXExtractString(msgObj, &msg))
		    goto message_cleanup;

		DXqmessage (who, msg, nolist);
	    }
	    /* async request */
	    if (isMsg == 2)
	    {
		if (async)
		    DXReadyToRun((Pointer)asyvarname);
	    }
 message_cleanup: ;
	}
	if (isMsg)
	    DXDelete(obj);
    } while(isMsg);
    g = (Group)obj;

    /* turn on input handler here */
    if (persistent && async) {
	if (ob_info->async_handler == 0) {
	    DXRegisterInputHandler (OBAsync, ob_info->fd, ob_info);
	    ob_info->async_handler = 1;
	}
    }

    /* return from outboard.  values are packaged into a single group
     * for transmission.  members are: return code, return message string,
     * output object list (so this routine knows how many members aren't
     * null) and the actual output objects.
     */

    /*
     * Check the error code.  If it is not a good one then get the
     * error message and set the error status and return.
     */

    obj = DXGetEnumeratedMember (g, 0, NULL);
    if (obj == NULL)
	goto cleanup;
    if (! DXExtractInteger (obj, (int *) &rc))
	goto cleanup;
    if (rc != ERROR_NONE)
    {
	obj = DXGetEnumeratedMember (g, 1, NULL);
	if (obj == NULL)
	    goto cleanup;
	if (! DXExtractString (obj, &rs))
	    goto cleanup;
        ExDebug("*2", "Outboard Module returning with error %s", rs);
	DXSetError (rc, rs);
	goto cleanup;
    }

    /*
     * Find out how many outputs were generated and extract them.
     * Here we reference the returned objects so that they don't get
     * deleted when we get rid of the packaging.  BUT, before we
     * return we need to unreference them so that we are handing
     * out objects with a reference count of zero.
     */

    obj = DXGetEnumeratedMember (g, 2, NULL);
    if (obj == NULL)
	goto cleanup;

    iptr = (int *) DXGetArrayData ((Array) obj);
    if (iptr == NULL)
	goto cleanup;

    for (i = 0, cnt = 3; i < nout; i++)
    {
	if (*iptr++)
	{
	    obj = DXGetEnumeratedMember (g, cnt++, NULL);
	    if (obj == NULL)
		goto cleanup;
	    out[i] = DXReference (obj);
            ExDebug("*2", "Setting output %d for Outboard Module", i);
	}
    }

    DXDelete ((Object) g);
    g = NULL;
    for (i = 0; i < nout; i++)
	DXUnreference (out[i]);

    ret = OK;
    /* fall thru */

  cleanup:
    DXDelete ((Object) g);

    /* fall thru */

    if (!persistent && fd >= 0)
    {
	close (fd);
	/* If we have to, wait for the child. */
#ifndef DXD_HAS_LIBIOP
	if (!_dxd_exDebugConnect)
	    wait(0);
#endif
    }

    return (ret);
}

Error
_dxf_ExDeleteRemote (char *name, int instance)
{
    node	*module;
    node	*id;
    int		flags;
    int		both;
    Object	in[2];
    char	cachetag[32];


    module = (node*)_dxf_ExMacroSearch(name);

    if (module == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#8370", name);
	return ERROR;
    }

    flags = module->v.function.flags;
    both = MODULE_OUTBOARD | MODULE_PERSISTENT;
    if ((flags & both) != both)
    {
	return OK;
    }

    id = module->v.module.in;
    in[0] = ((Object)id->v.id.dflt->v.constant.obj);
    id = id->next;
    in[1] = ((Object)id->v.id.dflt->v.constant.obj);
    sprintf(cachetag, "RemoteConnect%d", instance);

    DXSetCacheEntryV(NULL, 0, cachetag, 0, 2, in);

    return (OK);
}

Error
_dxf_ExDeleteReallyRemote (char *procgroup, char *name, int instance)
{
    node	*module;
    int		flags;
    int		both;
    int		fd;
    DelRemote   d;

    module = (node *)_dxf_ExMacroSearch(name);

    if (module == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#8370", name);
	return ERROR;
    }

    flags = module->v.function.flags;
    both = MODULE_OUTBOARD | MODULE_PERSISTENT;
    if ((flags & both) != both)
    {
	return OK;
    }

    d.del_namelen = strlen(name) + 1;
    d.del_name = name;
    d.del_instance = instance;

    /* this has to happen on the current host for the process group */
    fd = _dxf_ExGetSocketId(procgroup);
    if (fd < 0) {
	DXSetError(ERROR_DATA_INVALID, 
		   "can't find connection to host for process group '%s'",
		   procgroup);
	return ERROR;
    }

    _dxf_ExDistMsgfd(DM_PERSISTDELETE, (Pointer)&d, fd);

    return (OK);
}

int
_dxf_ExInitRemote (void)
{
    return (OK);
}

static void ExIncrementGlobalVariable (char *name, int cause_execution)
{
    int val;

    if(!DXGetIntVariable(name, &val))
        val = 0;

    DXSetIntVariable(name, val+1, 1, cause_execution);
}

/* the point of making the module id routines is to hide
 * exactly what a module id is.  right now it's the
 * fully qualified pathname down to the module, plus an
 * instance number, all in string form; but at some time
 * we might want to convert module ids over to simple numbers.
 * hopefully having these cover routines will mean that
 * people's code won't break if we do this.
 */

/* allocate space for the cpath of the current module
 *  and return it.
 */
Pointer DXGetModuleId()
{
    Pointer id;
    int len;

    len = DXGetModuleCacheStrLen() + 1;
    
    id = DXAllocate(len);
    if (!id)
	return NULL;

    DXGetModuleCacheStr(id);
    return id;
}

/* copy a module id
 */
Pointer DXCopyModuleId(Pointer id)
{
    Pointer nid;

    if (id == NULL || (char *)id == '\0')
	return NULL;

    nid = DXAllocate(strlen((char *)id) + 1);
    if (!nid)
	return NULL;

    strcpy(nid, id);
    return nid;
}

/* compare two module ids
 */
Error DXCompareModuleId(Pointer id1, Pointer id2)
{
    if (id1 == NULL || id2 == NULL)
	return ERROR;

    return ((strcmp(id1, id2) == 0) ? OK : ERROR);
}

/* get the space back from a module id string
 */
Error DXFreeModuleId(Pointer id)
{
    if (!id)
	return OK;

    DXFree(id);
    return OK;
}

Error DXReadyToRun(Pointer id)
{
    if (!id)
	return ERROR;

    ExIncrementGlobalVariable (id, 1);
    return OK;
}

Error DXReadyToRunNoExecute(Pointer id)
{
    if (!id)
	return ERROR;

    ExIncrementGlobalVariable (id, 0);
    return OK;
}

