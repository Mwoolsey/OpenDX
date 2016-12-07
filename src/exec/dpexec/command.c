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
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif
#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif
#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#include "config.h"
#include "command.h"
#include "function.h"
#include "remote.h"
#include "compile.h"
#include "d.h"
#include "exobject.h"
#include "_variable.h"
#include "_macro.h"
#include "version.h"
#include "parse.h"
#include "graph.h"
#include "graphqueue.h"
#include "parsemdf.h"
#include "swap.h"
#include "loader.h"
#include "help.h"
#include "userinter.h"
#include "dxpfsmgr.h"
#include "evalgraph.h"
#include "dxmain.h"

#ifdef	DXD_WIN
#define		strcasecmp	stricmp
#endif

#ifdef DXD_LICENSED_VERSION
static char _dxd_LicenseKey[5];
#endif

#define MAXOBJECTS 21   /* must match mdf-c generated dx.mdf file */

/* dummy entry for master host */
static dpgraphstat masterentry = {NULL, NULL, NULL, -1, 0, 0, 0, 0};
static int *update_dptable = NULL;

/* Defined programatically in libdx */
extern void _dxf_user_modules(); /* from libdx/ */
extern void _dxf_private_modules(); /* from libdx/ */

/* Defined in libdx/client.c */
extern int _dxfHostIsLocal(char *host); /* from libdx/ */

/* Defined by yuiif.c */
void _dxf_ExReadDXRCFiles ();

typedef struct _EXTab		*EXTab;

typedef struct _EXTab
{
    char	*name;		/* string to match		*/
    PFI		func;		/* function to call on match	*/
    EXTab	table;		/* table to search on match	*/
    int		size;		/* size of the table		*/
} _EXTab;


static Error	CmdProc		(Object *in);
static Error    MustRunOnMaster (Object in0);

#if DXD_OS2_SYSCALL
static int _Optlink	Compare		(const void *key, const void *data);
#else
static int		Compare		(const void *key, const void *data);
#endif

static Error	ProcessTable	(char *cmd, Object *in, EXTab table, int len);

static Error	Assign		(char *c, Object *in);
static Error	AssignNoExec	(char *c, Object *in);
static Error	Describe	(char *c, Object *in);
static Error	FlushCache	(char *c, Object *in);
static Error	FlushGlobal	(char *c, Object *in);
static Error	FlushMacro	(char *c, Object *in);
static Error	Getenv		(char *c, Object *in);
static Error	GetKey		(char *c, Object *in);
static Error    GroupAttach  	(char *c, Object *in);
static Error    GroupDetach  	(char *c, Object *in);
static Error    HostDisconnect 	(char *c, Object *in);
static Error	KillGraph	(char *c, Object *in);
static Error	License		(char *c, Object *in);
static Error	LoadObjFile	(char *c, Object *in);
static Error	LoadInteractors	(char *c, Object *in);
static Error	MdfLoadFile	(char *c, Object *in);
static Error	MdfLoadString	(char *c, Object *in);
static Error	OutboardKill	(char *c, Object *in);
static Error	Pfsmgr		(char *c, Object *in);
static Error	PrintCache	(char *c, Object *in);
static Error	PrintExObjects	(char *c, Object *in);
static Error	PrintFunction	(char *c, Object *in);
static Error	PrintGlobal	(char *c, Object *in);
static Error	PrintGroups	(char *c, Object *in);
static Error	PrintHosts	(char *c, Object *in);
static Error	ProductVersion	(char *c, Object *in);
static Error	Reclaim		(char *c, Object *in);
static Error	Terminate	(char *c, Object *in);
static Error	Version		(char *c, Object *in);
static Error	SyncGraph	(char *c, Object *in);
static int 	UpdateWorkers	(char *host, char *user, char *options, int add);

#define	FCALL(_n,_f) 		{_n, _f,   NULL, 0}
#define	TABLE(_n,_t)		{_n, NULL, _t,   sizeof (_t)}
#define	FANDT(_n,_f,_t)		{_n, _f,   _t,   sizeof (_t)}


static _EXTab AssignTable[] =
{
    FCALL ("",      	  Assign),
    FCALL ("noexecute",   AssignNoExec)
};

static _EXTab FlushTable[] =
{
    FCALL ("cache",      FlushCache),
    FCALL ("dictionary", FlushGlobal),
    FCALL ("macro",      FlushMacro),
    FCALL ("macros",     FlushMacro)
};

static _EXTab GroupTable[] =
{
    FCALL ("attach",    GroupAttach),
    FCALL ("detach", 	GroupDetach)
};

static _EXTab HostTable[] =
{
    FCALL ("disconnect",    HostDisconnect)
};

static _EXTab MdfTable[] =
{
    FCALL ("describe", 	Describe),
    FCALL ("file",	MdfLoadFile),
    FCALL ("print", 	Describe),
    FCALL ("string", 	MdfLoadString)
};

static _EXTab OutboardTable[] =
{
    FCALL ("delete",        OutboardKill),
};

static _EXTab PrintTable[] =
{
    FCALL ("cache",      PrintCache),
    FCALL ("dictionary", PrintGlobal),
    FCALL ("env",	 Getenv),
    FCALL ("exobjects",  PrintExObjects),
    FCALL ("functions",  PrintFunction),
    FCALL ("global",     PrintGlobal),
    FCALL ("globals",    PrintGlobal),
    FCALL ("groups",	 PrintGroups),
    FCALL ("hosts",	 PrintHosts),
    FCALL ("mdf", 	 Describe),
    FCALL ("version",    Version)
};

static _EXTab ProductTable[] =
{
    FCALL ("version",    ProductVersion),
};


/*
 * The top level command table
 */

static _EXTab TopLevel[] =
{
    TABLE ("assign",   		AssignTable),
    FCALL ("describe", 		Describe),
    FCALL ("exit",     		Terminate),
    TABLE ("flush",    		FlushTable),
    FCALL ("getenv",   		Getenv),
    FCALL ("getkey",   		GetKey),
    TABLE ("group",    		GroupTable),
    TABLE ("host",    		HostTable),
    FCALL ("kill",     		KillGraph),
    FCALL ("license",  		License),
    FCALL ("load",     		LoadObjFile),
    FCALL ("loadinteractors",   LoadInteractors),
    TABLE ("mdf",      		MdfTable),
    TABLE ("nop",      		NULL),
    TABLE ("outboard", 		OutboardTable),
    FCALL ("pfsmgr",   		Pfsmgr),
    TABLE ("print",    		PrintTable),
    TABLE ("product",  		ProductTable),
    FCALL ("quit",     		Terminate),
    FCALL ("reclaim",  		Reclaim),
    FCALL ("sync",     		SyncGraph),
    FCALL ("version",  		Version)
#if 0
    FCALL ("unload",   UnLoadObjFile),
#endif
};

/*
 * The top level command interface
 */

Error _dxf_ExExecCommand (Object *in)
{
    Error	ret, i;

    for (i=0; i < (MAXOBJECTS + 1) && in[i]; i++)
        ;

    if (MustRunOnMaster (in[0]))
	ret = _dxf_ExRunOn (1, CmdProc, in, ((i+1) * sizeof(Object)));
    else
	ret = CmdProc (in);

    return (ret);
}

Error _dxf_ExExecCommandStr (char *cmd)
{
    Object in[MAXOBJECTS + 1];
    int ret, i;

    in[0] = (Object)DXNewString(cmd);
    if(!in[0])
        return(ERROR);
    for(i = 1; i < MAXOBJECTS + 1; i++) 
        in[1] = NULL;
    ret = _dxf_ExExecCommand(in);
    DXDelete(in[0]);
    return(ret);
}

void 
_dxf_ExDeletePeer(SlavePeers *sp, int closepeer)
{
    /* if name is NULL but we still have a valid file descrip then */
    /* we are effectively marking this to be deleted when the socket */
    /* closes. */
    if(closepeer) 
        DXRegisterInputHandler(NULL, sp->sfd, NULL);
    if(sp->peername) {
        DXFree(sp->peername);
        sp->peername = NULL;
    }
    sp->SlaveId = -1;
    sp->wait_on_ack = FALSE;
    sp->pending_req = FALSE;
    sp->sendq.head = NULL;
    sp->sendq.tail = NULL;
    if(closepeer) {
        close(sp->sfd);
        sp->sfd = -99; /* reuse */
    }
}

void
_dxf_ExDeletePeerByName(char *host, int closepeer)
{
    int i, limit;
    SlavePeers *sp;

    for (i = 0, limit = SIZE_LIST(_dxd_slavepeers); i < limit; ++i)
    {
        sp = FETCH_LIST(_dxd_slavepeers, i);
        if(sp->sfd == -99)
           continue;
        if (!strcmp(sp->peername,host)) {
            _dxf_ExDeletePeer(sp, closepeer);
            break;
        }
    }
}

void
_dxf_ExAddPeer(SlavePeers *sp) 
{
    int k, limit;
    SlavePeers *index;

    for (k = 0, limit = SIZE_LIST(_dxd_slavepeers); k < limit; ++k) {
        index = FETCH_LIST(_dxd_slavepeers, k);
        if(index->sfd == -99) { /* reuse entry */
            UPDATE_LIST(_dxd_slavepeers, *sp, k);
            return;
        }
    }
    /* we didn't find an entry to reuse */
    APPEND_LIST(SlavePeers, _dxd_slavepeers, *sp);
}

void
_dxf_ExDeleteHost(char *host, int err, int closepeer)
{
    dpgraphstat *index, *hostentry;
    PGassign *pgindex;
    int i, j, limit, limit2;
    char savehost[MAXHOSTNAMELEN];

    strcpy(savehost, host);
    _dxf_ExDeletePeerByName(savehost, closepeer);

    /* entry 0 is a dummy for the master so skip it */
    for (i = 1, limit = SIZE_LIST(_dxd_dpgraphstat); i < limit; ++i) {
        index = FETCH_LIST(_dxd_dpgraphstat, i);
        /* entry not used */
        if(index->procfd == -99)
            continue;
        if (!strcmp(index->prochostname,savehost)) {
            if(err)  
                DXUIMessage("ERROR", 
                 "PEER ABORT - Connection to peer %s has been broken.",savehost);
            else if(index->procfd >= 0) {
                int peerwait = 1;
                _dxf_ExDistMsgfd(DM_EXIT, (Pointer)&peerwait, index->procfd);
                _dxf_ExDistributeMsg(DM_DELETEPEER, (Pointer)savehost, strlen(savehost)+1, TOSLAVES);
            }
            for (j = 0, limit2 = SIZE_LIST(_dxd_pgassign); j < limit2; ++j) {
                pgindex = FETCH_LIST(_dxd_pgassign, j);
                if(pgindex->hostindex < 0)
                    continue; /* no host assignment */
                hostentry = FETCH_LIST(_dxd_dpgraphstat, pgindex->hostindex);
                if(!strcmp(hostentry->prochostname, savehost)) {
                    pgindex->hostindex = -1;
                    _dxf_ExGVariableSetStr(pgindex->pgname, NULL);
                }
            }

            DXFree(index->prochostname);
	    DXFree(index->procusername);
            DXFree(index->options);
            index->prochostname = NULL;
	    index->procusername = NULL;
            index->options = NULL;
            if(index->procfd >= 0) {
                close(index->procfd);
                (*_dxd_exNSlaves)--;
                /* If we have to, wait for the child. */
                if (!_dxd_exDebugConnect)
                    wait(0);
            }
            index->procfd = -99; 	/* reuse this entry */
            index->numpgrps = 0;
            /* set error to TRUE so we won't try to restart this entry */
            index->error = TRUE;       
            break;
        }    
    }
}

/* 
 * update a global variable without causing a graph execution.
 */
Error _dxf_ExStealthUpdate (char *name, Object obj)
{
    return _dxf_ExUpdateGlobalDict (name, obj, 0);
}

void _dxf_InitDPtableflag()
{
    update_dptable = (int *)DXAllocate(sizeof(int *));
    if(update_dptable == NULL)
        _dxf_ExDie("Unable to allocate space for distributed flag");
    *update_dptable = FALSE;
    INIT_LIST(_dxd_dpgraphstat);
    masterentry.prochostname = _dxd_exHostName;
    masterentry.procusername = NULL;
    APPEND_LIST(dpgraphstat, _dxd_dpgraphstat, masterentry);
}

/* if a new host was added to the table return true */
int _dxf_NewDPTableEntry()
{
    if(*update_dptable) {
        *update_dptable = FALSE;
        return(TRUE);
    }
    else
        return(FALSE);
}

/* host string is in the form "hostname -options".
 * This will update the list of slave hosts, for a delete command
 * the entry will not be removed from the table and the sockets will
 * not be closed.
 */

static int UpdateWorkers(char *host, char *user, char *options, int add)
{
    int k, limit; 
    dpgraphstat *index;
    dpgraphstat dpentry;
    int optlen = 0;
    int opterr = FALSE;

    if(add) {
        if(options && *options != '-') {
            /* print error but add to table anyway */
            DXSetError(ERROR_BAD_PARAMETER,"bad option string %s", options);
            opterr = TRUE;
        }
        if(options)
            optlen = strlen(options) + 1;
    }

    /* if new worker is the master don't add it to table of workers */
    if(_dxfHostIsLocal(host)) {
        if(add) {
            if(options != NULL) 
                DXWarning("%s is already connected, host arguments ignored.",
                          _dxd_exHostName);
            DXFree(host);
            return(0);
        }
        else return(-1);
    }

    /* entry 0 is a dummy for the master, skip it */
    for (k = 1, limit = SIZE_LIST(_dxd_dpgraphstat); k < limit; ++k) {
        index = FETCH_LIST(_dxd_dpgraphstat, k);
        /* entry not used */
        if(index->procfd == -99)
            continue;

        if (!strcmp(index->prochostname, host)) {
            ExDebug("*1","Current host %s  already in dpgraphstat", host);
            /* not adding new host to table */
            if(add) {
                DXFree(host);
                if(index->procfd >= 0) {
                    if(options != NULL) {
                        if(index->options != NULL &&
                           !strcmp(index->options, options)) {
                            index->error = opterr;
                            if(!opterr)
                                index->numpgrps++;
                            return(k);
                        }
                    }
                    else {
                        if(index->options == NULL) {
                            index->error = FALSE;
                            index->numpgrps++;
                            return(k);
                        }
                    }

                    /* new options differ from old options */
                    if(index->numpgrps == 0) {
                        int peerwait = 0;
                        _dxf_ExDistMsgfd(DM_EXIT, (Pointer)&peerwait, index->procfd);
                        if(index->procfd >= 0) {
                            close(index->procfd);
                            /* If we have to, wait for the child. */
                            if (!_dxd_exDebugConnect)
                                wait(0);
                        }
                        index->procfd = -1;
                        index->error = opterr;
                        if(!opterr)
                            index->numpgrps++;
                        if(index->options) {
                            if(optlen > (strlen(index->options) + 1));
                                index->options =
                                  DXReAllocate(index->options, optlen);
                        }
                        else
                            index->options = DXAllocateLocal(optlen);
                        if(!index->options) {
                            index->error = TRUE;
                            DXSetError(ERROR_NO_MEMORY,
                                 "Cannot allocate memory for host table entry");
                            return(k);
                        }
                        if(optlen > 0) 
                            strcpy(index->options, options);
                        else {
                            DXFree(index->options);
                            index->options = NULL;
                        }
                        *update_dptable = TRUE;
                        ExDebug("*7", "Set Flag to Update distributed table");
                        return(k);
                    }
                    else {
                        DXSetError(ERROR_BAD_PARAMETER,
                            "Host already connected, cannot specify new options");
                        index->error = TRUE;
                        return(k);
                    }
                }
                else {  /* host not connected */
                    if(optlen > 0) {
                        if(index->options) {
                            if(optlen > (strlen(index->options) + 1))
                                index->options = DXReAllocate(index->options,
                                                           optlen);
                        }
                        else
                            index->options = DXAllocateLocal(optlen);
                        if(!index->options) {
                            index->error = TRUE;
                            DXSetError(ERROR_NO_MEMORY, 
                             "Cannot allocate memory for host table entry");
                            return(k);
                        }
                        strcpy(index->options, options);
                    }
                    else { /* no options to add, free old ones */
                        if(index->options) {
                            DXFree(index->options);
                            index->options = NULL; 
                         }
                    }
                    index->error = opterr;
                    if(!opterr)
                        index->numpgrps++;
                    *update_dptable = TRUE;
                    ExDebug("*7", "Set Flag to Update distributed table");
                    return(k);
                }
            }
            else { /* not adding a host */
                index->numpgrps--;
                if(index->numpgrps == 0)
                    _dxf_ExDeleteHost(host, 0, 0);
                return(-1);
            }
        }
    }
    if(add) {
        /* if we got here we did not find host name in list */
        dpentry.error = opterr;
        dpentry.prochostname = host;
	dpentry.procusername = user;
        if(optlen > 0) {
            dpentry.options = (char *) DXAllocateLocal (optlen);
            if(!dpentry.options) {
                dpentry.error = TRUE;
                DXSetError(ERROR_NO_MEMORY,
                  "Could not allocate memory for remote host table entry");
            }
            else
                strcpy(dpentry.options, options);
        }
        else dpentry.options = NULL;
        dpentry.procfd = -1;
        dpentry.SlaveId = -1;
        dpentry.SwapMsg = -1;
        if(!opterr)
            dpentry.numpgrps = 1;
        else 
            dpentry.numpgrps = 0;
        /* entry 0 is a dummy for the master */
        for (k = 1, limit = SIZE_LIST(_dxd_dpgraphstat); k < limit; ++k) {
            index = FETCH_LIST(_dxd_dpgraphstat, k);
            if(index->procfd == -99) { /* reuse entry */
                UPDATE_LIST(_dxd_dpgraphstat, dpentry, k);
                *update_dptable = TRUE;
                ExDebug("*7", "Set Flag to Update distributed table");
                return(k);
            }
        }
        /* we didn't find an entry to reuse */
        APPEND_LIST(dpgraphstat, _dxd_dpgraphstat, dpentry); 
        *update_dptable = TRUE;
        ExDebug("*7", "Set Flag to Update distributed table");
        limit = SIZE_LIST(_dxd_dpgraphstat);
        return(limit-1);
    }
    else  
        return(-1);
}

Error _dxf_ExPrintGroups()
{
    int i, limit;
    PGassign *index;
    dpgraphstat *hostentry;

    DXMessage("Group Attach Table");
    DXMessage("%-25s %s", "Groupname", "Hostname");
    for (i = 0, limit = SIZE_LIST(_dxd_pgassign); i < limit; ++i) {
        index = FETCH_LIST(_dxd_pgassign, i);
        if(index->hostindex >= 0) {
            hostentry = FETCH_LIST(_dxd_dpgraphstat, index->hostindex);
            DXMessage("%-25s %s", index->pgname, hostentry->prochostname);
        }
        else
            DXMessage("%-25s", index->pgname);
    }
    return OK; 
}

Error _dxf_ExPrintHosts()
{
    int i, limit;
    dpgraphstat *index;

    DXMessage("Number of Slaves %d", *_dxd_exNSlaves);
    DXMessage("Host Connection Table");
    DXMessage("%-25s %s", "Hostname", "Status");
    /* entry 0 is a dummy for the master */
    for (i = 1, limit = SIZE_LIST(_dxd_dpgraphstat); i < limit; ++i) {
        index = FETCH_LIST(_dxd_dpgraphstat, i);
        if(index->procfd == -99)
            continue;
        if(index->procfd < 0) {
            if(index->error) {
                if(index->prochostname)
                    DXMessage("%-25s %s", index->prochostname, "connect error");
            }
            else
                DXMessage("%-25s %s", index->prochostname, "not connected");
        }
        else {
            if(index->numpgrps == 1)
                DXMessage("%-25s %s: %d group  attached", 
                 index->prochostname, "connected", index->numpgrps);
            else
                DXMessage("%-25s %s: %d groups attached", 
                 index->prochostname, "connected", index->numpgrps);
        }
    }
    return OK; 
}

/* 
 * hack for now.  the mdf commands CAN'T be started with
 * the subroutine which is used to be sure the executive
 * commands run on processor 0, so start implementing a layer
 * which eventually could be used to run executive commands
 * which are not dependent on running on proc 0 from whichever
 * processor picked up the executive module in the first place.
 */
static Error MustRunOnMaster (Object in0)
{
    char        *cmd;

    if (!in0 || ! DXExtractString (in0, &cmd))
	return ERROR;

    if (strncmp(cmd, "mdf", 3))
	return OK;
    
    return ERROR;
}


static Error CmdProc (Object *in)
{
    Error	ret;
    char        *cmd;

    if (!in[0] || ! DXExtractString (in[0], &cmd)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "command input");
        return (ERROR);
    }

    ret = ProcessTable (cmd, &in[1], TopLevel, sizeof (TopLevel));
    return (ret);
}


#if DXD_OS2_SYSCALL
static int _Optlink
#else
static int
#endif
Compare (const void *key, const void *data)
{
    EXTab a = (EXTab) key;
    EXTab b = (EXTab) data;
    return (strcmp (a->name, b->name));
}


static Error ProcessTable (char *cmd, Object *in, EXTab table, int len)
{
    char	*next;
    char	buf[1024];
    _EXTab	dummy;
    EXTab	tab;
    Error	fret = OK;
    Error	tret = OK;

    for (next = cmd; next && *next && isspace (*next); next++)
	continue;

    buf[0] = '\0';
    if (sscanf (next, "%1023s", buf) != 1) {
        DXSetError(ERROR_BAD_PARAMETER, "#8310", cmd);
        return ERROR;
    }
    dummy.name = buf;
    tab = (EXTab) bsearch ((char *) &dummy, (char *) table,
			     len / sizeof (_EXTab), sizeof (_EXTab),
			     Compare);

    if (! tab) {
	DXSetError (ERROR_BAD_PARAMETER, "#8310", cmd);
	return (ERROR);
    }

    for (next = next + strlen (buf); next && *next && isspace (*next); next++)
	continue;

    if (tab->func)
	fret = (* tab->func) (next, in);

    if (tab->table) {
        if(strcmp(next, "") == 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#8312", cmd);
            return (ERROR);
        }
	tret = ProcessTable (next, in, tab->table, tab->size);
    }

    return (fret == OK && tret == OK ? OK : ERROR);
}


/*
 * Functions executed by the tables.
 */

static int Assign(char *c, Object *in) 
{
    return(ERROR);
}

static int AssignNoExec(char *c, Object *in) 
{
    char *name;

    if (!in[0] || ! DXExtractString (in[0], &name)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "variable name");
        return (ERROR);
    }
    return (_dxf_ExStealthUpdate(name, in[1]));

}

static int Describe (char *c, Object *in)
{
    char		*str;
    int			i, j, k, n;
    char		*key;
    node		*func;
    node		*minn;
    int			mini;
    node		**funcs	= NULL;
    char		help[1024];
    char		*hp;

    if ((c == NULL || *c == '\000') && (in == NULL || in[0] == NULL))
    {
	_dxf_ExDictionaryBeginIterate (_dxd_exMacroDict);
	for (n = 0; _dxf_ExDictionaryIterate (_dxd_exMacroDict, &key) != NULL; n++)
	    ;
	_dxf_ExDictionaryEndIterate (_dxd_exMacroDict);

	funcs = (node **) DXAllocateLocal (n * sizeof (node *));

	DXBeginLongMessage ();
	_dxf_ExDictionaryBeginIterate (_dxd_exMacroDict);
	for (i = 0, k = 0; i < n; i++)
	{
	    func = (node *) _dxf_ExDictionaryIterate (_dxd_exMacroDict, &key);
            /* skip over dummy function definitions */
            if (func->v.module.def.func == m__badfunc)
                continue;
	    if (funcs != NULL)
		funcs[k] = func;
	    else
		DXMessage ("%-25s %s\n", func->v.module.id->v.id.id, 
				     func->type == NT_MACRO
					 ? "macro" : "module");
            k++;
	}
	_dxf_ExDictionaryEndIterate (_dxd_exMacroDict);

	for (i = 0; i < k; i++)
	{
	    minn = funcs[i];
	    mini = i;
	    for (j = i; j < k; j++)
	    {
		if (strcasecmp (minn->v.function.id->v.id.id,
				funcs[j]->v.function.id->v.id.id) > 0)
		{
		    minn = funcs[j];
		    mini = j;
		}
	    }
	    func        = funcs[i];
	    funcs[i]    = funcs[mini];
	    funcs[mini] = func;
	}

	if (funcs != NULL)
	{
	    for (i = 0; i < k; i++)
		DXMessage ("%-25s %s\n", funcs[i]->v.module.id->v.id.id, 
				     funcs[i]->type == NT_MACRO
					 ? "macro" : "module");
	}
	DXEndLongMessage ();

	DXFree ((Pointer) funcs);
	return (OK);
    }

    if (c && *c) {
	while (*c)
	{
        int ret = 0;
	    help[0] = '\000';
	    ret = sscanf (c, "%1023s", help);
	    if (ret != 1 || help[0] == '\000')
		break;
	    str = _dxf_ExHelpFunction (help);
	    if (str)
	    {
		DXBeginLongMessage ();
		DXMessage (str);
		DXEndLongMessage ();
		DXFree ((Pointer) str);
	    }
	    for (c = c + strlen (help); *c && isspace (*c); c++)
		continue;
	}
    }
    else
    {
	for (i = 0; DXExtractString (in[i], &hp); i++)
	{
	    str = _dxf_ExHelpFunction (hp);
	    if (str)
	    {
		DXBeginLongMessage ();
		DXMessage (str);
		DXEndLongMessage ();
		DXFree ((Pointer) str);
	    }
	}
    }
    return (OK);
}


static int FlushCache (char *c, Object *in)
{
    _dxf_ExCacheFlush (FALSE);
    return (OK);
}


void _dxf_ExFlushGlobal (void)
{
    char *dummy=NULL;
    Object *in=NULL;

    FlushGlobal(dummy, in);
}

void ExResetGroupAssign ()
{
    int k, limit;
    PGassign *index;
    dpgraphstat *hostentry;

    for (k = 0, limit = SIZE_LIST(_dxd_pgassign); k < limit; ++k) {
        index = FETCH_LIST(_dxd_pgassign, k);
        hostentry = FETCH_LIST(_dxd_dpgraphstat, index->hostindex);
        if(!hostentry->error) 
            _dxf_ExGVariableSetStr(index->pgname,
                                       hostentry->prochostname);
    }
}

static int FlushGlobal (char *c, Object *in)
{
    if(!_dxd_exRemoteSlave) {
        /* _dxf_ExCacheFlush and _dxf_ExDictionaryPurge will send msgs 
         *  to  slaves and slaves will call each of these routines 
         *  seperately
         */
        _dxf_ExCacheFlush (TRUE);
        DXDebug ("1", "flushing dictionary");
        _dxf_ExDictionaryPurge (_dxd_exGlobalDict);
        _dxf_ExDistributeMsg(DM_FLUSHGLOBAL, NULL, 0, TOSLAVES);
    }
    _dxf_ExInitSystemVars ();
    ExResetGroupAssign ();
    return (OK);
}

void _dxf_ExFlushMacro (void)
{
    char *dummy=NULL;
    Object *in=NULL;

    FlushMacro(dummy, in);
}

static int FlushMacro (char *c, Object *in)
{
    if(!_dxd_exRemoteSlave) {
        /* _dxf_ExDictionaryPurge will send msg to slaves and cause 
         * the slaves to call this routine seperately 
         */
        _dxf_ExDictionaryPurge (_dxd_exMacroDict);
        _dxf_ExDistributeMsg(DM_FLUSHMACRO, NULL, 0, TOSLAVES);
    }
    _dxf_user_modules ();
    _dxf_private_modules ();
    _dxf_ExFunctionDone ();
    return (OK);
}


static int Getenv (char *c, Object *in)
{
    char	*env;
    char	**envp;

    DXBeginLongMessage ();
    if (c && *c)
    {
	env = (char *) getenv (c);
	if (env)
	    DXMessage ("%s=%s", c, env);
	else
	    DXMessage ("%s is not in the environment", c);
    }
    else
    {
	envp = (char **) _dxf_ExEnvp ();
	for ( ;envp && *envp; envp++)
	    DXMessage ("%s\n", *envp);
    }
    DXEndLongMessage ();

    return (OK);
}

/* Insert a group to hostname assignment in table. This will not make the
 * connection to the remote host until the current graph has finished
 * executing. If this is updated in the middle of a graph evaluation we
 * have a slave that is started but is not executing the current graph.
 * The master will see the new slave and try to wait for it to execute
 * the current graph which is doesn't have.
 * This takes a string list with each string element being:
 * "group1, group2, group3:hostname -options"
 */

static int GroupAttach(char *c, Object *in)
{
    int n, k, limit, grouplen, found;
    dpgraphstat *hostentry;
    char *str, *delimiter;
    char *hoststr, *userstr;
    PGassign pgassign, *index=NULL;
    int cerror = FALSE;
    char *hostname;
    char *username;
    char *optr;
    int hostlen, userlen;

    if(_dxd_exRemoteSlave)
        return(OK);

    if(!in[0]) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "group assignment");
        return (ERROR);
    }

    n = 0;
    while(DXExtractNthString(in[0], n, &str)) {
        hoststr = strchr(str, ':');
        if(hoststr == NULL) {
            DXSetError(ERROR_BAD_PARAMETER, "no host assignment");
            return(ERROR);
        }
        hoststr++;

        while(*hoststr == ' ')
            hoststr++;
        if(*hoststr == '\0') {
            DXSetError(ERROR_BAD_PARAMETER, "no host assignment");
            return(ERROR);
        }

       if ( strchr(hoststr, '@') ) {
            userstr = hoststr;
            hoststr = strchr(hoststr, '@') + 1;
            if ( *userstr == '@' ) {
               DXSetError(ERROR_BAD_PARAMETER, "no user assignment");
               return(ERROR);
           }
            optr = userstr;
           while(*optr != ' ' && *optr != '@')
               optr++;

            userlen = optr - userstr;
       }
       else
            userstr = NULL, userlen = 0;

        while(*hoststr == ' ')
            hoststr++;
        if(*hoststr == '\0') {
            DXSetError(ERROR_BAD_PARAMETER, "no host assignment");
            return(ERROR);
        }

        optr = hoststr;

        while(*optr && *optr != ' ' && *optr != ':')
            optr++;

        if(*optr)
            hostlen = optr - hoststr;
        else
            hostlen = strlen(hoststr);

        while(*optr && *optr == ' ')
            optr++;

        if(*optr == '\0')
            optr = NULL;

        while(*str == ' ')
            str++;
        if(*str == ':' || *str == ',') {
            DXSetError(ERROR_BAD_PARAMETER, "no group name");
            return(ERROR);
        }
        while(*str) {
	    hostname = (char *)DXAllocateLocal(hostlen + 1);
	    if(hostname == NULL)
		DXSetError(ERROR_NO_MEMORY,
			   "Cannot allocate memory for host table entry");
	    strncpy(hostname, hoststr, hostlen);
	    hostname[hostlen] = '\0';

            if ( userstr ) {
                username = (char *)DXAllocateLocal(userlen + 1);
                if(username == NULL)
                    DXSetError(ERROR_NO_MEMORY,
                               "Cannot allocate memory for user table entry");
                strncpy(username, userstr, userlen);
                username[userlen] = '\0';
                userstr = username;
            }

            delimiter = str;
            while(*delimiter && *delimiter != ',' && 
                  *delimiter != ' ' && *delimiter != ':') 
                delimiter++;
            grouplen = delimiter - str;
            pgassign.pgname = (char *)DXAllocateLocal(grouplen + 1);
            if(pgassign.pgname == NULL)
                DXSetError(ERROR_NO_MEMORY,
                       "Cannot allocate memory for group table entry");
            strncpy(pgassign.pgname, str, grouplen);
            pgassign.pgname[grouplen] = '\0';
            found = FALSE;
            /* search table to see if process group is already assigned */
            for (k = 0, limit = SIZE_LIST(_dxd_pgassign); k < limit; ++k) {
                index = FETCH_LIST(_dxd_pgassign, k);
                if(!strcmp(index->pgname, pgassign.pgname)) {
                    found = TRUE;
                    break;
                }
            } 
            if(found) {
                /* see if it is already attached */
                if(index->hostindex >= 0)
                    hostentry = FETCH_LIST(_dxd_dpgraphstat, index->hostindex);
                else hostentry = NULL;
                if(hostentry && (hostentry->error == FALSE) &&
                  !strcmp(hostentry->prochostname, hostname)) {
                    if(optr != NULL) {
                        if(hostentry->options != NULL &&
                           !strcmp(hostentry->options, optr)) {
                            DXFree(pgassign.pgname);
                            goto next_string;
                        }
                    }
                    else {
                        if(hostentry->options == NULL) {
                            DXFree(pgassign.pgname);
                            goto next_string;
                        }
                    }
                }
                /* replace host assignment in table */
                index->hostindex = UpdateWorkers(hostname, userstr, optr, 1);
                hostentry = FETCH_LIST(_dxd_dpgraphstat, index->hostindex);
                DXFree(pgassign.pgname);
                if(!hostentry->error) {
                    _dxf_ExGVariableSetStr(index->pgname, 
                                       hostentry->prochostname);
                    if(_dxd_exDebug)
                        printf("reassigning %s to %s\n",index->pgname,
                                hostentry->prochostname);
                }
                else 
                    cerror = hostentry->error;
            }
            else { /* adding new process group assignment */
                pgassign.hostindex = UpdateWorkers(hostname, userstr, optr, 1);
                hostentry = FETCH_LIST(_dxd_dpgraphstat, pgassign.hostindex);
                APPEND_LIST(PGassign, _dxd_pgassign, pgassign);
                if(!hostentry->error) {
                    _dxf_ExGVariableSetStr(pgassign.pgname, 
                                       hostentry->prochostname);
                    if(_dxd_exDebug)
                        printf("assigning %s to %s\n",
                             pgassign.pgname, hostentry->prochostname);
                }
                else
                    cerror = hostentry->error;
            }
next_string:
            str = delimiter;
            while(*str == ' ' || *str == ',')
                str++;
            if(*str == ':')
                break;
        }            
        n++;
    }
    if(cerror)
        return(ERROR);
    else 
        return(OK);
}


/* This routine does not close the sockets associated with a host. 
 * This routine takes a string list of group names to be removed from the
 * process assignment table.
 */

static int GroupDetach(char *c, Object *in)
{
    char *groupname;
    int n, k, limit;
    PGassign *index;
    dpgraphstat *hostentry;

    if(_dxd_exRemoteSlave)
        return(OK);

    if(!in[0]) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "group name");
        return (ERROR);
    }

    n = 0;
    while(DXExtractNthString(in[0], n, &groupname)) {
        for (k = 0, limit = SIZE_LIST(_dxd_pgassign); k < limit; ++k) {
            index = FETCH_LIST(_dxd_pgassign, k);
            if(!strcmp(index->pgname, groupname)) {
                hostentry = FETCH_LIST(_dxd_dpgraphstat, index->hostindex);
                if(hostentry) {
                    if(!hostentry->error)
                        index->hostindex = UpdateWorkers(
				  hostentry->prochostname,
                                  hostentry->procusername, NULL, 0);
                }
                _dxf_ExGVariableSetStr(index->pgname, NULL);
                _dxf_ExCacheFlush(FALSE);
                DXFree(index->pgname);
                DELETE_LIST(PGassign, _dxd_pgassign, k);
                break;
            }
        } 
        n++;
    }
    return(OK);
}

static int HostDisconnect(char *c, Object *in)
{
    int n;
    char *hostname;

    n = 0;
    while(DXExtractNthString(in[0], n, &hostname)) {
        _dxf_ExDeleteHost(hostname, 0, 0);
        n++;
    }

    return(OK);
}

static int KillGraph (char *c, Object *in)
{
    if (!_dxf_ExGQAllDone ()) {
	*_dxd_exKillGraph = TRUE;
        _dxf_ExDistributeMsg(DM_KILLEXECGRAPH,
                             (Pointer)_dxd_exContext->graphId, 0, TOPEERS);
    }
    return (OK);
}

static int LoadObjFile(char *c, Object *in)
{
    char *str;

    if (!in[0] || ! DXExtractString (in[0], &str)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "object filename");
        return (ERROR);
    }

    return DXLoadObjFile(str, "DXMODULES") != NULL;
}

static int LoadInteractors(char *c, Object *in)
{
    return _dxf_ExRunOn (1, _dxfLoadUserInteractors, (Pointer)c, strlen(c)+1);
}

static int MdfLoadFile(char *c, Object *in)
{
    char *str;

    if (!in[0] || ! DXExtractString (in[0], &str)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "mdf filename");
        return (ERROR);
    }

    return DXLoadMDFFile(str);
}

static int MdfLoadString(char *c, Object *in)
{
    char *str;

    if (!in[0] || ! DXExtractString (in[0], &str)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "mdf definition");
        return (ERROR);
    }

    return DXLoadMDFString(str);
}

/* disconnect the pipe between the exec and a specific outboard module.  
 *  args are: module name, instance number (may be omitted - defaults to 0),
 *  and process group name (may be null - defaults to localhost).
 */
static int OutboardKill (char *c, Object *in)
{
    char *module;
    int instance;
    char *procgroup;

    if (!in[0] || ! DXExtractString (in[0], &module)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "outboard module name");
        return (ERROR);
    }

    if (!in[1])
	instance = 0;
    else if (! DXExtractInteger (in[1], &instance)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10010", "outboard instance number");
        return (ERROR);
    }

    if (!in[2])
	procgroup = NULL;
    else if (! DXExtractString (in[2], &procgroup)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "outboard process group");
        return (ERROR);
    }

    if (procgroup) {
	if (!_dxf_ExDeleteReallyRemote (procgroup, module, instance))
	    return ERROR;
    } 
    else if (!_dxf_ExDeleteRemote (module, instance))
	return ERROR;

    return (OK);
}

static int Pfsmgr (char *c, Object *in)
{
    int		argc	= 0;
    char	*argv[1024];
    char	*buf	= NULL;
    char	*b;
    int		len;
    Error	ret	= ERROR;

    if (! c)
    {
	ret = OK;
	goto cleanup;
    }

    len = strlen (c) + 1;
    b = buf = (char *) DXAllocateLocal (len);
    if (buf == NULL)
	goto cleanup;
    memcpy (buf, c, len);

    while (*b)
    {
	argv[argc++] = b;
	while (*b && ! isspace (*b))
	    b++;
	while (*b && isspace (*b))
	    b++;
    }

    for (b = buf; *b; b++)
	if (isspace (*b))
	    *b = (char) NULL;

    ret = _dxf_pfsmgr (argc, argv);

cleanup:
    DXFree ((Pointer) buf);
    return (ret);
}


static int PrintCache (char *c, Object *in)
{
    _dxf_ExDictPrint (_dxd_exCacheDict);
    return (OK);
}


static int PrintExObjects (char *c, Object *in)
{
#ifdef LEAK_DEBUG
    PrintEXObj (); 
#else
    DXMessage ("#1280");
#endif
    return (OK);
}


static int PrintFunction (char *c, Object *in)
{
    _dxf_ExDictPrint (_dxd_exMacroDict);
    return (OK);
}


static int PrintGlobal (char *c, Object *in)
{
    _dxf_ExDictPrint (_dxd_exGlobalDict);
    return (OK);
}

static int PrintGroups (char *c, Object *in)
{
    _dxf_ExPrintGroups();
    return (OK);
}

static int PrintHosts (char *c, Object *in)
{
    _dxf_ExPrintHosts();
    return (OK);
}

static int Reclaim (char *c, Object *in)
{
    DXTraceTime (1);
    _dxf_ExReclaimMemory (atol (c));
    DXTraceTime (0);
    DXPrintTimes ();
    return (OK);
}

static int SyncGraph (char *c, Object *in)
{
    _dxd_exParseAhead = FALSE;
    return(OK);
}

static int Terminate (char *c, Object *in)
{
    exit (0);
    /* not reached */
    return (OK);
}

static int License (char *c, Object *in)
{
    
#ifdef DXD_LICENSED_VERSION
    
    static int license_tried = FALSE;
    int i;
    char keybuf[14];
    char typebuf[5];
    char key[9];
    char salt[3];
    
    /* decode and check license string here */
    
    if (license_tried)
	exit(0);		/* our UI will never call $license twice */
    else
	license_tried = TRUE;
    
    for(i=0;i<13;i++)
	keybuf[i] = c[i];
    keybuf[13] = '\0';
    
    for(i=0;i<4;i++)
	typebuf[i] = c[i+13];
    typebuf[4] = '\0';
    
    for(i=0;i<4;i++)
	key[i*2] = _dxd_LicenseKey[i];
    
    key[1] = 'g';
    key[3] = '3';
    key[5] = '$';
    key[7] = 'Q';
    key[8] = '\0';
    
    salt[0] = '4';
    salt[1] = '.';
    salt[2] = '\0';
    
    if(strcmp(keybuf,(const char *)crypt(key,salt))){
	DXMessage("License Error: Invalid license Message\n");
	DXUIMessage("LICENSE","UNAUTHORIZED");
	return (OK);
    }
    
    sscanf(typebuf,"%04x",&i);
   
    /* This junk below is to preserve byte order, beware ! */
 
    switch(i^(*((ubyte *)&keybuf[4])<<8)+(*((ubyte *)&keybuf[5]))) {
	
      case GOT_NODELOCKED:
	if (!ExGetPrimaryLicense())
	    DXMessage("Exec could not get a license (UI has nodelocked)\n");
	else 
	    DXUIMessage("LICENSE", "AUTHORIZED");
	break;
	
      case GOT_CONCURRENT:
	_dxd_ExHasLicense = TRUE;
	DXUIMessage("LICENSE", "AUTHORIZED");
	DXMessage("Running under UI's concurrent license\n");
	break;
	
      default:
	DXMessage("License Error: Invalid license Message\n");
	break;
    }
    
    if (_dxd_ExHasLicense == FALSE)
        DXUIMessage("LICENSE","UNAUTHORIZED");
    
    if (_dxd_ExHasLicense && _dxd_exRemote)
	_dxf_ExReadDXRCFiles();
    
#else  /* DXD_LICENSED_VERSION */
    
    if (_dxd_exRemote)
	_dxf_ExReadDXRCFiles();

    DXUIMessage("LICENSE", "AUTHORIZED");

#endif /* !DXD_LICENSED_VERSION */

    return (OK);
}

static int GetKey  (char *c, Object *in)
{
#ifdef DXD_LICENSED_VERSION  

    int i;

    srand(time(NULL));
    i = rand();
    i = (i<4096) ? (i+4096) : (i); /* forces to be 4 0x chars */
    
    sprintf(_dxd_LicenseKey, "%x\n", i);
    DXUIMessage("LICENSE", _dxd_LicenseKey);

#else  /* DXD_LICENSED_VERSION */

    DXUIMessage("LICENSE","ffff");	/* write key ffff in case UI tries */
                                        /* to use license protocol  */
#endif /* !DXD_LICENSED_VERSION */
    
    return (OK);
}



static int Version (char *c, Object *in)
{
    int		n;
    int		m0, m1, m2;
    char	buf[256];
    int		err = TRUE;
    char	*b;

    if (c == NULL || *c == '\000')
    {
	DXBeginLongMessage ();
        DXMessage ("Executive version:        %d %d %d\n", 
	            DXD_VERSION, DXD_RELEASE, DXD_MODIFICATION);
        DXMessage ("Executive/UI interface:   %d %d %d\n",
	            UI_MAJOR, UI_MINOR, UI_MICRO);
        DXMessage ("Creation date:            %s\n", EX_COM_DATE);
        DXMessage ("Creation host:            %s\n", EX_COM_HOST);
	DXEndLongMessage ();
	return (OK);
    }

#if DXD_PRINTF_RETURNS_COUNT
    n = sprintf (buf,
		 "Executive/UI version mismatch:  %d %d %d vs ",
		 UI_MAJOR, UI_MINOR, UI_MICRO);
#else
    {
        sprintf (buf,
		 "Executive/UI version mismatch:  %d %d %d vs ",
		 UI_MAJOR, UI_MINOR, UI_MICRO);
        n = strlen(buf);
    }
#endif
    b = buf + n;

    n = sscanf (c, "%d%d%d", &m0, &m1, &m2);

    if (n == 3 && m0 == UI_MAJOR && m1 == UI_MINOR && m2 == UI_MICRO)
	err = FALSE;
    else
	sprintf (b, "%s", *c ? c : "<no version specified>");

    if (err)
	DXUIMessage ("ERROR", buf);

    return (OK);
}


static int ProductVersion (char *c, Object *in)
{
    int		n;
    int		m0, m1, m2;

    if (c == NULL || *c == '\000') {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "Product Version string required for version check");
	return ERROR;
    }
    
    n = sscanf (c, "%d%d%d", &m0, &m1, &m2);
#define VERSION_ID(a,b,c)	(((a) << 16) + ((b) << 8) + (c))
    
    if (n != 3 || 
	(VERSION_ID(m0,m1,m2) > 
	 VERSION_ID(DXD_VERSION,DXD_RELEASE,DXD_MODIFICATION))) {
	DXWarning("User Interface or .net file version is newer than Executive version");
    }

    return (OK);
}

/* No longer used so make it undefined */
#if 0
static Error UnLoadObjFile(char *c, Object *in)
{
    char *str;

    if (!in[0] || ! DXExtractString (in[0], &str)) {
        DXSetError (ERROR_BAD_PARAMETER, "#10200", "object filename");
        return (ERROR);
    }

    return DXUnloadObjFile(str);
}
#endif
