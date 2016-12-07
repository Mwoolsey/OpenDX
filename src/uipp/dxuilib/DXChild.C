/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include "DXStrings.h"
#include "DXChild.h"

#ifdef OS2
#define INCL_DOSPROCESS
#define INCL_DOSDATETIME
#define INCL_DOSFILEMGR
#define INCL_DOSQUEUES
#include <os2.h>
#endif

#include "DXApplication.h"
#include "ErrorDialogManager.h"
#include "MsgWin.h"

#include <stdlib.h>
#include <stdio.h>
#if defined(HAVE_FCNTL_H)

#include <fcntl.h>
#endif

#if defined(HAVE_IO_H)
#include <io.h>
#endif

#if defined(HAVE_PROCESS_H)
#include <process.h>
#endif

#ifdef	DXD_WIN
#define 	close	_close
int LocalRead( int handle, void *buffer, unsigned int count );
int SetSocketMode(int  s, int iMode);
int UxRecv(int s, char *ExternalBuffer, int BuffSize, int Flags);
#define	read	LocalRead
#endif

#include <errno.h>
#include <ctype.h>

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(HAVE_NETDB_H)
#ifdef alphax
extern "C"
{
#endif
#include <netdb.h>
#ifdef alphax
}
#endif
#endif

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif

#if defined(HAVE_SYSENT_H)
#include <sysent.h>
#endif


#if HAVE_CC_OSFCN_H
#include <CC/osfcn.h>
#endif
#if HAVE_CC_LIBC_H
#include <CC/libc.h>
#endif

#if defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

#ifdef ibm6000
extern "C"
{
    void herror(const char *);
}
#endif

#if 0
#if (REQUIRES_BZERO_DECLARATION == 1)
extern "C" void bzero(char *, int);
#endif
#endif


#define verbose 0

#ifdef  DXD_WIN
#if defined(XtInputReadMask)
#undef XtInputReadMask
#endif

#define     XtInputReadMask     XtInputReadWinsock

void	ClearExecMessages(void *arg);

#endif

int
DXChild::HostIsLocal(const char *host)
{
    char localHostname[BUFSIZ];
    char localhostHostname[BUFSIZ];
    char remoteHostname[BUFSIZ];
    struct hostent *he;
    int  hostnameFound;
#if defined(HAVE_SYS_UTSNAME_H)

    struct utsname Uts_Name;

    if (strcmp ("unix", host) == 0)
        return TRUE;

    he = gethostbyname ((char*)host);
    if (he == NULL) {
        // If here, assume that the hostname is not in the dns; therefore,
        // the only machine we can connect to would be the localhost,
        // thus return true.
        return TRUE;
    }
    strcpy(remoteHostname, he->h_name);

    if (uname(&Uts_Name) >= 0 && (he=gethostbyname(Uts_Name.nodename)) != NULL) {
        strcpy(localHostname, he->h_name);
        hostnameFound = 1;
    } else {
        hostnameFound = 0;
    }

    he = gethostbyname ("localhost");
    if (he == NULL) {
        //	herror ("localhost");
        ErrorMessage("%s: Invalid host", "localhost");
        return (0);
    }
    strcpy(localhostHostname, he->h_name);

    return ((hostnameFound && strcmp (localHostname, remoteHostname) == 0) ||
            strcmp (localhostHostname, remoteHostname) == 0);
#endif

#ifdef DXD_WIN

    int i;

    if(_stricmp(host, "localhost") == 0
            ||
            _stricmp(host, "localPC") == 0)
        return TRUE;


    i = gethostname(localHostname, BUFSIZ - 1);
    if(strcmp(localHostname, host) == 0)
        return TRUE;

    he = gethostbyname(host);
    if(he == NULL || he->h_name == NULL)
        return TRUE; /* Host is Local   */


    strcpy(remoteHostname, he->h_name);
    he = gethostbyname(localHostname);
    if(he == NULL || he->h_name == NULL)
        return TRUE; /* Host is Local   */

    strcpy(localhostHostname, he->h_name);
    if(strcmp(remoteHostname, localhostHostname) != 0)
        i = FALSE;
    else
        i = TRUE;

    return i;


#endif

    return TRUE;

}
/* This routine returns the pid of the forked child to communicate with (or
 * 0 if no waiting is required),
 * or, if failure, -1.  It (for now) also does a perror.  It also returns
 * remin, remout, and remerr, stdin,out,err for the created task.
 */
int
DXChild::ConnectTo(const char *host,
                   const char *user,
                   const char *cwd,
                   int ac,
                   char *av[],
                   char *ep[],
                   int *remin,
                   int *remout,
                   int *remerr,
                   char *errstr)
{
    char s[BUFSIZ];
    char script_name[500],cmd[1000];
    FILE *fp;
    int i;
#if defined(DXD_WIN) || defined(OS2)

    int *in[2], *out[2], *err[2];
#else

    int in[2], out[2], err[2];
#endif

    char **rep;
    char **fargv = NULL;
    int child;
    struct hostent *he;
    int findx;
    char *pathEnv;
    int  j;
    char *dnum;
#if defined(HAVE_SYS_UTSNAME_H)

    struct utsname Uts_Name;
    char *local_rsh_cmd;

    local_rsh_cmd = getenv( "DXRSH" );
    if ( !local_rsh_cmd )
        local_rsh_cmd = RSH;

    /*
     * Initialize return values (to default negative results).
     */
    *remin  = -1;
    *remout = -1;
    *remerr = -1;
    *errstr = '\0';

    /*
     * Check to see if "host" is a valid hostname.
     */
    if (strcmp("unix", host) != 0) {
        he = gethostbyname ((char*)host);
        if (he == NULL) {
            ErrorMessage("Unable to resolve %s, connecting locally.", host);

            //	    	herror (host);
            //	    	ErrorMessage("%s: Invalid host", host);
            //	    	return (-1);
        }
    }

    for (pathEnv = NULL, i = 0; ep[i] && pathEnv == NULL; ++i)
        if (    strncmp (ep[i], "PATH=", STRLEN("PATH=")) == 0 ||
                strncmp (ep[i], "PATH =", STRLEN("PATH =")) == 0)
            pathEnv = ep[i];

    if (DXChild::HostIsLocal(host)) {
        char *path;
        char *opath;
        struct STATSTRUCT sbuffer;

        if (verbose)
            fprintf(stderr, "local\n");

        if (user != NULL)
            fprintf (stdout, "Different user on local machine ignored\n");

        fargv = (char**)CALLOC((ac + 1), sizeof (char*));
        if (fargv == NULL) {
            ErrorMessage("calloc() error: %s", strerror(errno));
            goto error_return;
        }
        for (i = 0; i < ac; ++i) {
            fargv[i] = av[i];
        }
        fargv[i] = 0;
        rep = ep;
        /* Scan through ep looking for "PATH".  If "PATH" is found, then
         * look through path for the specified command, and if it's
         * found and executable, replace it in fargv with the expanded 
         * path name.
         */
        if (*fargv[0] != '.' && *fargv[0] != '/') {
            path = strchr (pathEnv, '=') + 1;
            opath = path = strdup(path);
            if (path == NULL) {
                ErrorMessage("strdup() error: %s", strerror(errno));
                goto error_return;
            }
            while (*path) {
                for (i = 0;
                        *path != '\0' && *path != ':';
                        ++i, ++path) {
                    s[i] = *path;
                }
                if (*path == ':') {
                    ++path;
                }
                s[i] = '\0';
                strcat (s, "/");
                strcat (s, av[0]);
                if (STATFUNC(s, &sbuffer) < 0) {
                    /* if the file doesn't exist, go on */
                    if (errno == ENOENT) {
                        /* We have no more paths to search */
                        if (*path == '\0') {
                            FREE(opath);
                            goto error_return;
                        }
                    }
                    /* Ignore bad stats, we just can't find it */
                    continue;
                }
                /* If it's executable, and a regular file, we're done. */
                if ((sbuffer.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0 &&
                        (sbuffer.st_mode & S_IFREG) != 0) {
                    break;	/* out of while(*path) */
                }
            }
            fargv[0] = s;
            FREE (opath);
        }
    } else {		/* Host is NOT local */
        /* The general thread here is to first create a script on the
         * remote host using popen().  The script contains shell code to
         *   1) set up a similar environment to the current one (PATH,...)
         *   2) exec 'dx -...' 
         *   3) remove the script from the remote file system.
         * Then we set up fargv to contain the following: 
         *    rsh host [ -l user ] /bin/sh /tmp/dx-$host:$pid 
         */

        if (verbose)
            fprintf(stderr, "remote\n");

        fargv = (char**)CALLOC(12, sizeof(char*));

        if (fargv == NULL) {
            ErrorMessage("calloc() error: %s", strerror(errno));
            goto error_return;
        }


        if (uname(&Uts_Name) < 0) {
            ErrorMessage("uname() error: %s", strerror(errno));
            goto error_return;
        }
        sprintf(script_name,"/tmp/dx-%s:%d",Uts_Name.nodename,getpid());
        findx=0;
        fargv[findx++] = local_rsh_cmd;
        fargv[findx++] = (char *)host;

        if (user != NULL) {
            fargv[findx++] = "-l";
            fargv[findx++] = (char *)user;
            sprintf(cmd, "%s -c \"%s %s -l %s 'cat > %s' > /dev/null 2>&1\"",
                    BSH, local_rsh_cmd, host, user, script_name);
        } else
            sprintf(cmd, "%s -c \"%s %s 'cat > %s' > /dev/null 2>&1\"",
                    BSH, local_rsh_cmd, host, script_name);

        fargv[findx++] = BSH;
        fargv[findx++] = script_name;
        fargv[findx++] = NULL ;

        fp = popen(cmd,"w");
        if (!fp) {
            ErrorMessage("popen() error: %s", strerror(errno));
            goto error_return;
        }
        setbuf(fp,NULL); /* Unbuffered so file is 'immediately' updated */

        for (i = 0; ep[i]; ++i) {
            /* Environment variables which are NOT passed to remote process */
            static char *eignore[] = {"HOST=","HOSTNAME=","TZ=","SHELL=", 0};
            int ignore;

            /* Look for and skip certain environment variables */
            /* We could do this more quickly elsewhere, but ... */
            for (ignore=j=0 ; !ignore && eignore[j] ; j++)
                if (!strncmp(ep[i], eignore[j], STRLEN(eignore[j])))
                    ignore = 1;
            if (ignore)
                continue;

            /*
             * Write the environment variable to the remote script file 
             */
            if ((strncmp(ep[i],"DISPLAY=unix:",STRLEN("DISPLAY=unix:"))==0   ||
                    strncmp(ep[i],"DISPLAY=localhost:",
                            STRLEN("DISPLAY=localhost:"))==0 ||
                    strncmp(ep[i],"DISPLAY=:",    STRLEN("DISPLAY=:")) == 0) &&
                    (dnum = strchr(ep[i], ':')) != NULL) {
                fprintf(fp,"DISPLAY='%s%s';export DISPLAY\n",
                        Uts_Name.nodename,dnum);
            } else {
                int k;
                char evar[256], c;
                char eval[1024];

                for (j=0 ; ep[i][j] && (ep[i][j] != '=') ; j++)
                    evar[j] = ep[i][j];
                evar[j] = '\0';
                k = j + 1;	/* Skip the '=' sign */
                for (j=0 ; (c = ep[i][k]) ; k++, j++) {
                    if (c == '\'') {		/* ' -> '\'' */
                        /* Value contains a double quote */
                        eval[j++] = '\'';
                        eval[j++] = '\\';
                        eval[j++] = '\'';
                    }
                    eval[j] = c;
                }
                eval[j] = '\0';
                fprintf(fp,"%s='%s'; export %s\n", evar, eval, evar);
            }
        }

        if (cwd != NULL)
            fprintf(fp,"cd %s\n",cwd);

        fprintf(fp,"\n(sleep 15; rm -f %s) &\nexec ",script_name);
        for (i = 0; i < ac; ++i)
            fprintf(fp, "%s ",av[i]);
        fprintf(fp,"\n");
        pclose(fp);

        rep = ep;
    }

    /* Set up three pipes */
    if (pipe(in) < 0) {
        ErrorMessage("pipe(in) error: %s", strerror(errno));
        goto error_return;
    }
    if (pipe(out) < 0) {
        ErrorMessage("pipe(out) error: %s", strerror(errno));
        goto error_return;
    }
    if (pipe(err) < 0) {
        ErrorMessage("pipe(err) error: %s", strerror(errno));
        goto error_return;
    }

    child = fork();
    if (child == 0) {
        if (verbose && cwd != NULL)
            fprintf(stderr, "cd %s\n", cwd);
        fflush (stdout);
        if (verbose)
            fprintf(stderr, "exec %s\n", fargv[0]);
        fflush (stdout);
        if (verbose)
            for (i = 0; rep[i]; ++i)
                fprintf(stderr, "envp[%d] = `%s'\n", i, rep[i]);
        if (verbose)
            for (i = 0; fargv[i]; ++i)
                fprintf(stderr, "argv[%d] = `%s'\n", i, fargv[i]);
        close (in[1]);
        close (out[0]);
        close (err[0]);
        if (dup2(in[0], 0) < 0) {
            ErrorMessage("dup2() error: %s", strerror(errno));
            exit(1);
        }
        if (dup2(out[1], 1) < 0) {
            ErrorMessage("dup2() error: %s", strerror(errno));
            exit(1);
        }
        if (dup2(err[1], 2) < 0) {
            ErrorMessage("dup2() error: %s", strerror(errno));
            exit(1);
        }

        if (cwd != NULL && chdir (cwd) < 0) {
            ErrorMessage("chdir() error: %s", strerror(errno));
            exit(1);
        }
        if (execve(fargv[0], fargv, rep) < 0) {
            ErrorMessage("execve() error: %s", strerror(errno));
            exit(1);
        }
    } else if (child > 0) {
        close (in[0]);
        close (out[1]);
        close (err[1]);
        *remin = in[1];
        *remout = out[0];
        *remerr = err[0];
    } else {
        goto error_return;
    }


    if (fargv != NULL)
        FREE ((char*)fargv);
    return (child);

#endif                   // HAVE_SYS_UTSNAME_H


#ifdef	OS2

#define BuffSize     1000
#define HF_STDOUT    1

    if (DXChild::HostIsLocal(host)) {
        CHAR         LoadError[100];
        PSZ          Envs;
        RESULTCODES  ReturnCodes;
        APIRET       rc;
        PSZ          ptr;
        HFILE        hpR, hpW;
        int          PipeSize=4096;
        int          NRead, TotRead;
        char         Buff[BuffSize];
        HFILE        hfSave = -1;
        HFILE        hfNew = HF_STDOUT;


        rc=DosDupHandle(HF_STDOUT, &hfSave);
        rc=DosCreatePipe(&hpR, &hpW, PipeSize);
        rc=DosDupHandle(hpW, &hfNew);
        *remout = hpR;

        if (verbose)
            fprintf(stderr, "local\n");

        if (user != NULL)
            fprintf (stdout, "Different user on local machine ignored\n");

        Envs = 0;

        strcpy(cmd,"CMD.EXE");
        ptr = &cmd[strlen(cmd)+1];
        strcpy(ptr,"");
        for (i=0;i<ac;i++) {
            if (i==0)
                strcpy(ptr,"/C");
            strcat(ptr," ");
            strcat(ptr,av[i]);
        }
        ptr[strlen(ptr)+1]='\0';

        rc = DosExecPgm(LoadError, sizeof(LoadError), EXEC_ASYNC, cmd, Envs,
                        (PRESULTCODES)&ReturnCodes, "CMD.EXE");

        DosClose(hpW);
        rc=DosDupHandle(hfSave, &hfNew);

        child = -1;
        if (rc==0)
            child = ReturnCodes.codeTerminate;
        else
            ErrorMessage("Failure to spawn dx.cmd", "XXXX");
        return child;
    } else {
        return (-1);
    }

#endif		//	OS2

#ifdef	DXD_WIN

    char         args[1000];
    char         exename[255];

    char         *Envs;
    int          ReturnCodes, rc, iHostIsLocal, pid;

    HFILE        hpR0, hpW0;
    HFILE        hpR1, hpW1;
    HFILE        hpR2, hpW2;

    int          NRead, TotRead, bTest;
    HFILE        hfSave0 = -1;
    HFILE        hfSave1 = -1;
    HFILE        hfSave2 = -1;

    HFILE        hin  = _fileno(stdin);
    HFILE        hout = _fileno(stdout);
    HFILE        herr = _fileno(stderr);
    DWORD        dwordTmp;

    unsigned long       ThrdTID;
    STARTUPINFO         sInfo;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pInfo;

    iHostIsLocal = DXChild::HostIsLocal(host);
    if (verbose)
        fprintf(stderr, "local\n");

    if (user != NULL)
        fprintf (stdout, "Different user on local machine ignored\n");

    strcpy(exename,av[0]);
    for (i=0;i<strlen(exename);i++)
        if (exename[i]=='/')
            exename[i]='\\';

    if ((strstr(exename,".bat") || strstr(exename,".BAT")) && iHostIsLocal) {
        strcpy(cmd,"command.com /C "); // FIXME Different on NT and 95?

        strcat(cmd,exename);
    } else {
        strcpy(cmd, "");

        if(! iHostIsLocal) {
            strcat(cmd, "rsh ");
            strcat(cmd, host);
            strcat(cmd, " ");
        }

        strcat(cmd,exename);
    }

    for (i=1;i<ac;i++) {
        if (av[i] && *av[i]) {
            strcat(cmd, " ");
            strcat(cmd, av[i]);
        }
    }

    Envs = NULL;

    memset(&pInfo, 0, sizeof(pInfo));
    memset(&sInfo, 0, sizeof(sInfo));
    memset(&sa,    0, sizeof(sa));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);	// structure size
    sa.lpSecurityDescriptor = NULL;		// default descriptor
    sa.bInheritHandle = TRUE;			// inheritable

    bTest = CreatePipe( &this->hpipeRead,	// reading handle
                        &this->hpipeWrite,	// writing handle
                        &sa,			// lets handles be inherited
                        0 );			// default buffer size

    bTest = DuplicateHandle( GetCurrentProcess( ),
                             this->hpipeRead,		// original handle
                             GetCurrentProcess( ),
                             NULL,			// don't create new handle
                             0,
                             FALSE,			// not inheritable
                             DUPLICATE_SAME_ACCESS );

    sInfo.cb = sizeof(sInfo);
    sInfo.dwFlags = STARTF_USESTDHANDLES;
    sInfo.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
    sInfo.hStdOutput = this->hpipeWrite;
    sInfo.hStdError  = GetStdHandle( STD_ERROR_HANDLE );

    pid = rc = CreateProcess(NULL,
                             cmd,
                             NULL,
                             NULL,
                             TRUE,
                             0,
                             Envs,
                             NULL,	// CurDir
                             &sInfo,	// Startup Info
                             &pInfo	// address of PROCESS_INFORMATION
                            );
    if(rc == TRUE)  {
        CloseHandle(pInfo.hThread);	// discard new thread handle
        pid = pInfo.dwProcessId;
        rc = 0;
        bTest = (int) CreateThread(NULL, 0,
                                   (LPTHREAD_START_ROUTINE) ClearExecMessages,
                                   (LPVOID) this->hpipeRead, 0, &ThrdTID);
        SetThreadPriority((LPVOID) &ThrdTID, THREAD_PRIORITY_BELOW_NORMAL);
    } else {
        rc = 1;
        pid = -1;
    }

    *remout = -1;
    child = -1;

    if (rc == 0)
        child = pid;
    else
        ErrorMessage("Failure to spawn dx child process");

    return child;

#endif		//	DXD_WIN

error_return:
                sprintf(errstr,"Could not connect using '%s'\n",fargv[0]);
    if (fargv != NULL)
        FREE ((char*)fargv);
    return (-1);
}

void
DXChild::setServer(char *s)
{
    if (this->server)
        delete this->server;
    this->server = DuplicateString(s);
    fprintf(stderr, "child: server set to %s\n", s);
}

#define RSIZE 512
void
DXChild::MakeLine(char *string, char *newString)
{
    int p = STRLEN(string);
    int i = 0;
    int newlen = STRLEN(newString);
    for (i = 0; i < newlen; ++i) {
        if (p > 0 && (p == RSIZE-1)) //  || string[p-1] == '\n'))
        {
            p = 0;
        }
        string[p++] = newString[i];
    }
    string[p] = '\0';
}

/* uipStartExecutive starts the dx script and returns information about the
 * resulting child.
 * Return Values if block is TRUE:
 *	 0 -- Success
 *	-1 -- Failure
 *	 1 -- Retry OK (The server was busy).
 * Return Values if block is FALSE:
 *	 0 -- Success
 *    Else -- Failure
 */
int
DXChild::StartExecutive(const char* host,
                        const char* user,
                        const char* cwd,
                        const char* cmd,
                        int*  in,
                        int*  out,
                        int*  err,
                        int*  port,
                        int*  child,
                        int   block,
                        char **rstringParam)
{
    extern char **environ;
    char **av = NULL;
    char **t;
    int	 ac = 0;
    int  i, j, k;
    int result;
    static char rstring[RSIZE];

    rstring[0] = '\0';
    result = -1;
    *child = -1;

    if (verbose) {
        fprintf(stderr, "Connect to `%s'\n", host);
        if (user)
            fprintf(stderr, "tas `%s'\n", user);
        if (cwd)
            fprintf(stderr, "tin `%s'\n", cwd);
        fprintf(stderr, "trunning `%s'\n", cmd);
    }

    /* Turn the optional cmd into an ac/av pair */
    av = (char **)CALLOC(1, sizeof (char*));
    if (av == NULL) {
        ErrorMessage("calloc() error: %s", strerror(errno));
        goto error_return;
    }
    for (ac = 0; *cmd; ++ac) {
        t = (char**)REALLOC ((char*)av, (ac + 1) * sizeof (char*));
        if (t == NULL) {
            ErrorMessage("realloc() error: %s", strerror(errno));
            goto error_return;
        } else
            av = t;
        /* Skip leading whitespace */
        cmd += strspn(cmd, " \t");
        
        i = 0; 
        while(cmd[i] != ' ' && cmd[i] != '\0') {
            if(cmd[i] == '"') {
            	i++;
            	while(cmd[i] != '"' && cmd[i] != '\0') {
            		i++;
            	}
            }
            if(cmd[i] != '\0')
            	i++;
        }
            	
            ;
        av[ac] = (char *)CALLOC((i + 1), sizeof (char));
        if (av[ac] == NULL)
            goto error_return;

		/* Now copy the string into the argv without quotes */
        for (j = 0, k = 0; j<i; ++j, ++cmd)
        	if(*cmd != '"')
                av[ac][k++] = *cmd;
        av[ac][k] = '\0';
    }

    *child =DXChild::ConnectTo(host, user, cwd, ac, av, environ, in, out, err,
                               rstring);


    if (!block) {
#ifndef DXD_WIN
        result = (*child < 0);
#else

        result = (*child == -1);
#endif

        goto error_return;
    }


error_return:
                if (av) {
                    for (i = 0; i < ac; ++i)
                        if (av[i] != NULL)
                            FREE (av[i]);
                    FREE ((char*)av);
                }
                /* remove the \n at the end of the string */
                char *p = strrchr(rstring,'\n');
    if (p)
        *p = '\0';
    *rstringParam = rstring;

    return (result);
}

extern "C" void DXChild_OutQueuedInputHandler(XtPointer  clientData,
            int       *socket,
            XtInputId *id)
{
    char buffer[1000];
    DXChild *obj = (DXChild *)clientData;
    if (obj->input_handlers_stalled)
        return ;
    int sts = 0;

    if ((sts = read(*socket, buffer, 1000)) < 0) {
        ErrorMessage("Unable to read from executive, read() error: %s",
                     strerror(errno));
    } else if (sts == 0)
        obj->closeOutput();
    else {
        int i = 0;
        int j = STRLEN(obj->outLine);
        for (i = 0; i < sts; ++i, ++j) {
            if (buffer[i] == '\n' || j == obj->lineSize) {
                char *s = obj->outLine;
                int port;
                obj->outLine[j] = '\n';
                obj->outLine[j+1] = '\0';
                theDXApplication->getMessageWindow()->addInformation(
                    obj->outLine);
                if ((s = strstr(obj->outLine, "host = ")) != NULL) {
                    char buf[256];
                    sscanf(s, "host = %s", buf);
                    theDXApplication->setServer(buf);
                    obj->setServer(buf);
                }
                if ((s = strstr(obj->outLine, "port = ")) != NULL) {
                    sscanf(obj->outLine, "port = %d\n", &port);
                    theDXApplication->connectToServer(port, obj);
                }
                obj->outLine[0] = '\0';
                j = -1;
            } else {
                obj->outLine[j] = buffer[i];
                obj->outLine[j+1] = '\0';
            }
        }
    }
}
extern "C" void DXChild_ErrQueuedInputHandler(XtPointer  clientData,
            int       *socket,
            XtInputId *id)
{
    char buffer[1000];
    DXChild *obj = (DXChild *)clientData;
    if (obj->input_handlers_stalled)
        return ;
    int sts = 0;

    if ((sts = read(*socket, buffer, 1000)) < 0) {
        ErrorMessage("Unable to read from executive, read() error: %s",
                     strerror(errno));
    } else if (sts == 0)
        obj->closeError();
    else {
        int i = 0;
        int j = STRLEN(obj->errLine);
        for (i = 0; i < sts; ++i, ++j) {
            if (buffer[i] == '\n' || j == obj->lineSize) {
                obj->errLine[j] = '\0';
                //		write(2, obj->errLine, j + 1);
                theDXApplication->getMessageWindow()->addError(obj->errLine);
                obj->errLine[0] = '\0';
                j = -1;
            } else {
                obj->errLine[j] = buffer[i];
                obj->errLine[j+1] = '\0';
            }
        }
    }
}
extern "C" void DXChild_OutInputHandler(XtPointer  clientData,
                                            int       *socket,
                                            XtInputId *id)
{
    char buffer[1000];
    DXChild *obj = (DXChild *)clientData;
    if (obj->input_handlers_stalled)
        return ;
    int sts = 0;

    if ((sts = read(*socket, buffer, 1000)) < 0) {
        ErrorMessage("Unable to read from executive, read() error: %s",
                     strerror(errno));
    } else if (sts == 0)
        obj->closeOutput();
    else {
        int i = 0;
        int j = STRLEN(obj->outLine);
        for (i = 0; i < sts; ++i, ++j) {
            if (buffer[i] == '\n' || j == obj->lineSize) {
                obj->outLine[j] = '\n';
                obj->outLine[j+1] = '\0';
                theDXApplication->getMessageWindow()->addInformation(
                    obj->outLine);
                // write(1, obj->outLine, j + 1);
                obj->outLine[0] = '\0';
                j = -1;
            } else {
                obj->outLine[j] = buffer[i];
                obj->outLine[j+1] = '\0';
            }
        }
    }
}
extern "C" void DXChild_ErrInputHandler(XtPointer  clientData,
                                            int       *socket,
                                            XtInputId *id)
{
    char buffer[1000];
    DXChild *obj = (DXChild *)clientData;
    if (obj->input_handlers_stalled)
        return ;
    int sts = 0;

    if ((sts = read(*socket, buffer, 1000)) < 0) {
        ErrorMessage("Unable to read from executive, read() error: %s",
                     strerror(errno));
    } else if (sts == 0)
        obj->closeError();
    else {
        int i = 0;
        int j = STRLEN(obj->errLine);
        for (i = 0; i < sts; ++i, ++j) {
            if (buffer[i] == '\n' || j == obj->lineSize) {
                obj->errLine[j] = '\0';
                theDXApplication->getMessageWindow()->addError(
                    obj->errLine);
                //		write(2, obj->errLine, j+1);
                obj->errLine[0] = '\0';
                j = -1;
            } else {
                obj->errLine[j] = buffer[i];
                obj->errLine[j+1] = '\0';
            }
        }
    }
}

DXChild::DXChild(char *host,
                 char *cmd,
                 int   block,
                 char *user,
                 char *cwd)
{
    int port = 0;
    char *s = NULL;

    this->lineSize = 1000;
    this->outLine = new char[this->lineSize + 2];
    this->errLine = new char[this->lineSize + 2];
    *this->outLine = '\0';
    *this->errLine = '\0';
    this->deletion_wpid = NUL(XtWorkProcId);
    this->input_handlers_stalled = FALSE;

    this->server = DuplicateString(host);

    int result = StartExecutive(this->server,
                                user,
                                cwd,
                                cmd,
                                &this->in,
                                &this->out,
                                &this->err,
                                &port,
                                &this->child,
                                block,
                                &s);
    if (!block && result < 0) {
        this->queued = 1;
#if !defined(DXD_WIN) && !defined(OS2)

        this->outId = XtAppAddInput(theApplication->getApplicationContext(),
                                    this->out,
                                    (XtPointer)XtInputReadMask,
                                    DXChild_OutQueuedInputHandler,
                                    (XtPointer)this);
        this->errId = XtAppAddInput(theApplication->getApplicationContext(),
                                    this->err,
                                    (XtPointer)XtInputReadMask,
                                    DXChild_ErrQueuedInputHandler,
                                    (XtPointer)this);
#endif                          //SMH

    } else if (result == 0) {
        this->queued = 0;
#if !defined(DXD_WIN) && !defined(OS2)

        this->outId = XtAppAddInput(theApplication->getApplicationContext(),
                                    this->out,
                                    (XtPointer)XtInputReadMask,
                                    DXChild_OutInputHandler,
                                    (XtPointer)this);
        this->errId = XtAppAddInput(theApplication->getApplicationContext(),
                                    this->err,
                                    (XtPointer)XtInputReadMask,
                                    DXChild_ErrInputHandler,
                                    (XtPointer)this);
#endif                          //SMH
        //	if (block)
        //	{
        //	    theDXApplication->connectToServer(port, this);
        //	}
    } else {
        this->queued = 0;
    }

    if (!IsBlankString(s)) {
        this->errorString = DuplicateString(s);
    } else {
        this->errorString = NULL;
    }
}


DXChild::~DXChild()
{
    if (this->out >= 0) {
        close(this->out);
#if !defined(DXD_WIN) && !defined(OS2)

        XtRemoveInput(this->outId);
#endif

    }
    if (this->err >= 0) {
        close(this->err);
#if !defined(DXD_WIN) && !defined(OS2)

        XtRemoveInput(this->errId);
#endif

    }
    if (this->in >= 0) {
        close(this->in);
    }

    if (this->child > 1) {
#if defined(DXD_WIN)
        TerminateProcess((HANDLE)this->child,-1);
#else

        kill(this->child, SIGTERM);
        wait(0);
#endif

    }

    delete this->server;
    if (this->errorString != NULL)
        delete this->errorString;
    delete this->outLine;
    delete this->errLine;

}


void
DXChild::closeOutput(boolean closeConnection)
{
    if (this->out >= 0) {
        close (this->out);
        XtRemoveInput(this->outId);
    }
    this->out = -1;
    if (closeConnection && this->err == -1) {
        theDXApplication->closeConnection(this);
        if (this->deletion_wpid == NUL(XtWorkProcId))
            this->deletion_wpid =
                XtAppAddWorkProc (theDXApplication->getApplicationContext(),
                                  (XtWorkProc) DXChild_DeleteObjectWP, (XtPointer)this);
    }
}

void
DXChild::closeError(boolean closeConnection)
{
    if (this->err >= 0) {
        close (this->err);
#if !defined(DXD_WIN) && !defined(OS2)

        XtRemoveInput(this->errId);
#endif

    }
    this->err = -1;
    if (closeConnection && this->out == -1) {
        theDXApplication->closeConnection(this);
        if (this->deletion_wpid == NUL(XtWorkProcId))
            this->deletion_wpid =
                XtAppAddWorkProc (theDXApplication->getApplicationContext(),
                                  (XtWorkProc) DXChild_DeleteObjectWP, (XtPointer)this);
    }
}

extern "C" Boolean DXChild_DeleteObjectWP (XtPointer clientData)
{
    DXChild* c = (DXChild*)clientData;
    ASSERT(c);
    delete c;
    return TRUE;
}

void
DXChild::unQueue()
{
    if (this->out >= 0) {
        XtRemoveInput(this->outId);
        this->outId = XtAppAddInput(theApplication->getApplicationContext(),
                                    this->out,
                                    (XtPointer)XtInputReadMask,
                                    DXChild_OutInputHandler,
                                    (XtPointer)this);
    }
    if (this->err >= 0) {
#ifndef  DXD_WIN
        XtRemoveInput(this->errId);
        this->errId = XtAppAddInput(theApplication->getApplicationContext(),
                                    this->err,
                                    (XtPointer)XtInputReadMask,
                                    DXChild_ErrInputHandler,
                                    (XtPointer)this);
#endif

    }
    this->queued = FALSE;
}

//
// Wait for a connection, and if it succedes, return 0, if it gets queued,
// return 1, and if it fails, return -1.
//
int
DXChild::waitForConnection()
{
    char *s, rdbuffer[BUFSIZ+1];
    int  sts;
    int result = 0;
    int port;
    static char rstring[RSIZE];
    rstring[0] = '\0';
#if defined(HAVE_SYS_UTSNAME_H)

    fd_set fds;
#endif

    /* Until we get port = ..., read from stdout and error.  Close the
     * port down and fail if we get an eof on either.
     */

#if !defined(HAVE_SYS_UTSNAME_H)

    result = 0;
    /* Need to loop here because dx.cmd first sends "Starting DX Exec", followed by
     * "[E:\usr1\....\dxexec -r -B" 
     * A few seconds later, "port = 1900" comes across.  Thus need to loop over the read
     * strings until port = appears, or one of the other options...
     * Maybe should put timeout here...
     */
#ifdef DXD_WIN

    DWORD	dwordTmp;
    int iRet, iLoop = 0;
    while (result==0) {

        iLoop = 0;
        while(1) {
            iRet = PeekNamedPipe(this->hpipeRead, rdbuffer,
                                 sizeof(rdbuffer), &dwordTmp,
                                 NULL, NULL);
            if(iRet && dwordTmp > 0)
                break;
            Sleep(10);
            iLoop++;
            if(iLoop > 1000) {
                ErrorMessage("read() error: %s", strerror(errno));
                return -1;
            }
        }


        sts = ReadFile(this->hpipeRead, // handle of file to read
                       rdbuffer,	// address of buffer that receives data
                       sizeof(rdbuffer),	// number of bytes to read
                       &dwordTmp,	// address of number of bytes read
                       NULL 	// address of structure for data
                      );
        sts = dwordTmp;	//	strlen(rdbuffer);
        rdbuffer[sts] = '\0';
        theDXApplication->getMessageWindow()->addInformation(rdbuffer);
        fprintf(stderr, "========= %s\n", rdbuffer);
        if (sts >= 0) {
            DXChild::MakeLine(rstring, rdbuffer);
            if ((s = strstr(rdbuffer, "host = ")) != NULL) {
                char buf[256];
                sscanf(s, "host = %s", buf);
                theDXApplication->setServer(buf);
                this->setServer(buf);
            }
            if ((s = strstr(rdbuffer, "port = ")) != NULL) {
                sscanf (s, "port = %d", &port);
                if (this->isQueued())
                    this->unQueue();
                theDXApplication->connectToServer(port, this);
                return 0;
            } else if (strstr(rdbuffer, "Execution has been queued")   ||
                       strstr(rdbuffer, "Server appears to be in use") ||
                       strstr(rdbuffer, "rror"))           // suits added
            {
                return 1;
            }
        }
    }  // while

#endif	//	if DXD_WIN

#ifdef  OS2
    while (result==0)
    {
        result = DosRead(this->out, rdbuffer, BUFSIZ,(PULONG) &sts);   // This will block until something to read...
        if (result != 0) {
            ErrorMessage("read() error: %s", strerror(errno));
            this->closeOutput(FALSE);
            result = -1;
        }
        //	#endif     // if OS2
        else {
            rdbuffer[sts] = '\0';
            theDXApplication->getMessageWindow()->addInformation(
                rdbuffer);
            if (result >= 0) {
                DXChild::MakeLine(rstring, rdbuffer);
                if ((s = strstr(rdbuffer, "host = ")) != NULL) {
                    char buf[256];
                    sscanf(s, "host = %s", buf);
                    theDXApplication->setServer(buf);
                    this->setServer(buf);
                }
                if ((s = strstr(rdbuffer, "port = ")) != NULL) {
                    sscanf (s, "port = %d", &port);
                    if (this->isQueued())
                        this->unQueue();
                    theDXApplication->connectToServer(port, this);
                    return 0;
                }
                if (strstr(rdbuffer, "Execution has been queued")        ||
                        strstr(rdbuffer, "Server appears to be in use") ||
                        strstr(rdbuffer, "rror"))           // suits added
                {
                    return 1;
                }
            }
        }
    }  // while
#endif		//	OS2
    return result;
#endif                   // HAVE_SYS_UTSNAME_H


#if defined(HAVE_SYS_UTSNAME_H)

    //
    // Stop using the subevent loop.  The call to XtAppNextEvent has the side effect
    // of checking sockets identified to XtAppAddInput and possibly going into the
    // associated event handler.  So select() might say that there is no input ready
    // at this->out but by the time the call to XtAppNextEvent is made, something
    // may have arrived.
    // The event handler for this->out does not know
    // anything about "port =".  The result is that that handler swallows that string
    // and the code below never sees it, so the port # is lost and we can't connect
    // to the server.  The correct fix would be to erase the while loop below and
    // to build the smarts into the event handlers.  Doing that would also accomplish
    // what I originally intended to do which was to process X events during the
    // 2 to 5 seconds we sit here idly waiting for input from the fresh server.
    //
#define USE_SUB_EVENT_LOOP 0
#if USE_SUB_EVENT_LOOP

    this->input_handlers_stalled = TRUE;
    Display *d = theApplication->getDisplay();
    XtAppContext app = theApplication->getApplicationContext();
    struct timeval to;
    int counts = 0;
#define MAX_COUNTS 1000000
#endif    //    USE_SUB_EVENT_LOOP

    int status;

    while (this->child >= 0 && (this->out >= 0 || this->err >= 0)) {
        int width = (this->out > this->err ? this->out : this->err) + 1;
        FD_ZERO(&fds);
        if (this->out >= 0)
            FD_SET(this->out, &fds);
        if (this->err >= 0)
            FD_SET(this->err, &fds);
#if USE_SUB_EVENT_LOOP
        //
        // Do coarse polling because there are important timers created during
        // program startup and they're being ignored by calling select().
        // The "counts" var says that we'll poll at most 1 million times
        // which at 2/10 second each poll is a long time, but the loop will
        // still terminate as soon as there is an error on a socket.
        // I testing this by killing the exec from csh while dxui was waiting for
        // it to start.  The results are OK but not great.  The ui doesnt' report
        // the failure to connect until several seconds later.
        //
        // This AppPending stuff isn't living down below with the other X stuff
        // because that other X stuff is only executed if the socket from the
        // server has input which is not the case if a timer goes off.
        if (XtAppPending (app) & XtIMTimer)
            XtAppProcessEvent (app, XtIMTimer);
        FD_SET(ConnectionNumber(d), &fds);
        to.tv_sec = 0;
        to.tv_usec = 200000; // 2/10 of a second
        if (counts++ >= MAX_COUNTS)
            status = -1;
        else
            status = select(width, (SELECT_ARG_TYPE *)&fds, NULL, NULL, &to);
#else   //   USE_SUB_EVENT_LOOP

        status = select(width, (SELECT_ARG_TYPE *)&fds, NULL, NULL, NULL);
#endif   //   USE_SUB_EVENT_LOOP

        if (status < 0) {
            this->input_handlers_stalled = FALSE;
            return result;
        }
        if (this->err >= 0 && FD_ISSET(this->err, &fds)) {
            sts = read(this->err, rdbuffer, BUFSIZ);
            if (sts < 0) {
                ErrorMessage("read() error: %s", strerror(errno));
                this->closeError(FALSE);
                result = -1;
            } else if (sts == 0) {
                this->closeError(FALSE);
                result = -1;
            } else {
                rdbuffer[sts] = '\0';
                DXChild::MakeLine(rstring, rdbuffer);
                if ((s = strstr(rdbuffer, "host = ")) != NULL) {
                    char buf[256];
                    sscanf(s, "host = %s", buf);
                    theDXApplication->setServer(buf);
                    this->setServer(buf);
                }
                if (strstr (rdbuffer, "Server appears to be in use")) {
                    result = 1;
                    this->input_handlers_stalled = FALSE;
                    return result;
                }
            }
        }
#if USE_SUB_EVENT_LOOP
        else if (FD_ISSET(ConnectionNumber(d), &fds)) {
            XEvent event;
            // XmUpdateDisplay() is a higher level way of doing the same
            // thing, but it's a very expensive function - uses XSync().
            boolean server_ready = FALSE;
            while ((!server_ready) && (XtAppPending(app) & XtIMXEvent)) {
                XtAppNextEvent (app, &event);
                switch (event.type) {
                case ButtonPress:
                case ButtonRelease:
                case KeyPress:
                case KeyRelease:
                    break;
                default:
                    printf ("%s[%d] XEvent\n", __FILE__,__LINE__);
                    theDXApplication->passEventToHandler (&event);
                    break;
                }
                //
                // Check server readiness each time thru this loop because
                // the -program option can keep us here for a long time and
                // we only want to process events while the real work isn't ready.
                //
                FD_ZERO(&fds);
                if (this->out >= 0)
                    FD_SET(this->out, &fds);
                if (this->err >= 0)
                    FD_SET(this->err, &fds);
                to.tv_sec = 0;
                to.tv_usec = 0;
                status = select(width, (SELECT_ARG_TYPE *)&fds, NULL, NULL, &to);
                if (status > 0) {
                    if (this->err >= 0 && FD_ISSET(this->err, &fds)) {
                        server_ready = TRUE;
                    } else if (this->out >= 0 && FD_ISSET(this->out, &fds)) {
                        server_ready = TRUE;
                    }
                } else {
                    server_ready = TRUE;
                }
            }
        }
#endif		//   USE_SUB_EVENT_LOOP

        else if (this->out >= 0 && FD_ISSET(this->out, &fds)) {
            sts = read(this->out, rdbuffer, BUFSIZ);
            if (sts < 0) {
                ErrorMessage("read() error: %s", strerror(errno));
                this->closeOutput(FALSE);
                result = -1;
            } else if (sts == 0) {
                this->closeOutput(FALSE);
                result = -1;
            } else {
                char *s;
                rdbuffer[sts] = '\0';
                theDXApplication->getMessageWindow()->addInformation(
                    rdbuffer);
                // write (1, rdbuffer, sts);

                if (result >= 0) {
                    DXChild::MakeLine(rstring, rdbuffer);

                    if ((s = strstr(rdbuffer, "host = ")) != NULL) {
                        char buf[256];
                        sscanf(s, "host = %s", buf);
                        theDXApplication->setServer(buf);
                        this->setServer(buf);
                    }
                    if ((s = strstr(rdbuffer, "port = ")) != NULL) {
                        sscanf (s, "port = %d", &port);
                        if (this->isQueued())
                            this->unQueue();
                        theDXApplication->connectToServer(port, this);
                        this->input_handlers_stalled = FALSE;
                        return 0;
                    }
                    if (strstr(rdbuffer, "Execution has been queued")   ||
                            strstr(rdbuffer, "Server appears to be in use")) {
                        return 1;
                    } else if (strstr(rdbuffer, "rror")) {
                        this->errorString = DuplicateString(rdbuffer);
                        return -1;
                    }
                }
            }
        }
    }

    /* If we ever get here, there was an error in the read,
     * then wait for any children to quit, and fail.
     * Sigterm the child, just for good measure.
     */
    while (this->err >= 0 && (sts = read (this->err, rdbuffer, BUFSIZ)) > 0) {
        //	write (2, rdbuffer, sts);
        rdbuffer[sts] = '\0';
        theDXApplication->getMessageWindow()->addError(rdbuffer);
    }
    if (this->child > 0) {
        result = -1;
    }

    this->errorString = DuplicateString(rstring);


    this->input_handlers_stalled = FALSE;
    return (result);
#endif			// !defined(DXD_LACKS_UTD)
}



#ifdef	DXD_WIN
int LocalRead( int handle, void *buffer, unsigned int count )
{
    int i;

    SetSocketMode(handle, 0);
    //	i = _read(handle, buffer, count);

    i = UxRecv(handle, (char *)buffer, count, 0);

    return i;
}

void	ClearExecMessages(void *arg)
{
#define		READ_BUFFER_SIZE	2048

    int bTest, iPrint;
    DWORD dwordTmp;
    char	achStr[READ_BUFFER_SIZE];

    if(getenv("DEBUG_EXEC_MESSAGES"))
        iPrint = 1;
    else
        iPrint = 0;

    Sleep(30*1000);
    if(1) {
        do {
            if(!arg)
                break;
            bTest = ReadFile(arg, // handle of file to read
                             achStr,	// address of buffer that receives data
                             READ_BUFFER_SIZE, 	// number of bytes to read
                             &dwordTmp,	// address of number of bytes read
                             NULL 	// address of structure for data
                            );
            if(bTest && iPrint)
                printf("FROM DX %s \n ", achStr);
        } while(0);
    }
    if(iPrint)
        printf("UI:Terminating ClearExecMessages \n");

    return;
}


#endif  //     DXD_WIN

