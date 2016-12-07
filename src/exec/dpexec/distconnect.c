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

#if defined(HAVE_IO_H)
#include <io.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_WINIOCTL_H)
#include <winioctl.h>
#endif

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#include "config.h"
#include "_macro.h"
#include "sysvars.h"
#include "distp.h"
#include "config.h"
#include "context.h"
#include "obmodule.h"
#include "cache.h"
#include "remote.h"
#include "command.h"
#include "distp.h"
#include "ccm.h"

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_SYS_FILIO_H)
#include <sys/filio.h>
#endif

static int slave_id = 1;  /* We start at 1 because master will be slave 0 */
static Error ConnectPStoS(dpgraphstat *index, dpgraphstat *index2);
static Error ConnectPMtoS(dpgraphstat *index);

extern int DXConnectToServer(char *host, int pport); /* from libdx/client.c */

static int
ExNextSlaveId()
{
    int next;

    next = slave_id;
    slave_id++;
    return(next);
}

static Error ExSendModuleInputHandler(int fd, Pointer arg)
{
    SlavePeers 	*sp = NULL;
    int         b, i, limit;

    for (i = 0, limit = SIZE_LIST(_dxd_slavepeers); i < limit; ++i) {
        sp = FETCH_LIST(_dxd_slavepeers, i);
        if (sp->sfd == fd) 
            break;
    }
    if(sp == NULL) {
        DXUIMessage("ERROR", "Input Handler can find peer entry in table.\n");
        return(ERROR);
    }

    if ((IOCTL(fd, FIONREAD, (char *)&b) < 0) || (b <= 0)) {
        if(sp->peername == NULL) { /* this was marked for delete */
            _dxf_ExDeletePeer(sp, 1);
            return(ERROR);
        }
        _dxf_ExExecCommandStr ("kill");
        if(_dxd_exContext->program)
            _dxd_exContext->program->runable = 0;
	if(_dxd_exDebug) 
	    printf("shut down socket for peer %s\n", sp->peername); 
        /* if I'm the master, close the master/slave connection */
        if(!_dxd_exRemoteSlave) 
            _dxf_ExDeleteHost(sp->peername, 1, 1);
        else 
            _dxf_ExDeletePeer(sp, 1);
        return(ERROR);
    }
    return(_dxf_ExReceivePeerPacket(sp));
}

void _dxf_ExUpdateDPTable()
{
    int	i, j,k;
    dpgraphstat *index, *index2; 
    char *opt;
    char ** t;
    char **av = NULL;
    int ac = 1;
    int addedhost = FALSE;
    int init_av = TRUE;
    dpversion dpv = {DPMSG_SIGNATURE, DPMSG_VERSION};
    dpslave_id dpslaveid;

    ExDebug("*1", "Checking distributed host table");
    /*----------------------------------------------------------------------*/
    /*  start up slaves if they have not already been started               */
    /*  start at 1 since the zeroth entry is a dummy for the master.        */
    /*----------------------------------------------------------------------*/
    for (k = 1; k < SIZE_LIST(_dxd_dpgraphstat); ++k) {
        index = FETCH_LIST(_dxd_dpgraphstat, k);
        if(index->procfd < 0 && (index->numpgrps > 0) && (!index->error)) {
            if(init_av) {
                /* Make an argc/argv pair for dx command and options */
                av = (char **)DXAllocate(2 * sizeof (char*));
                if (av == NULL) {
                    goto error_exit;
                }
                av[0] = (char *)DXAllocate(3 * sizeof (char));
                if (av[0] == NULL)
                    goto error_exit;
   
                strcpy(av[0], "dx");
                init_av = FALSE;
            }
            opt = index->options;
            if(opt == NULL)
                ac = 1;
            else {
                for (ac = 1; *opt; ++ac) {
                    t = (char**)DXReAllocate((char*)av,(ac + 1)*sizeof (char*));
                    if (t == NULL) {
                        goto error_exit;
                    }
                    else
                        av = t;
                    opt += strspn(opt, " \t");
                    for (i = 0; opt[i] != ' ' && opt[i] != '\0'; ++i)
                        ;
                    av[ac] = (char *)DXAllocate((i + 1)*sizeof (char));
                    if (av[ac] == NULL)
                        goto error_exit;
                    for (i = 0; *opt != ' ' && *opt != '\0'; ++i, ++opt)
                        av[ac][i] = *opt;
                    av[ac][i] = '\0';
                }
            }
            index->procfd = _dxfExRemoteExec (_dxd_exDebugConnect, 
                  index->prochostname, index->procusername, ac, av, 0);
            if(index->procfd < 0) {
                DXUIMessage("ERROR", "Connection to peer %s failed.\n",
                             index->prochostname);
                index->error = TRUE;
                index->numpgrps = 0;
                continue;
            }
            (*_dxd_exNSlaves)++;
            addedhost = TRUE;
            _dxf_ExDistMsgfd(DM_VERSION, (Pointer)&dpv, index->procfd);
            dpslaveid.id = index->SlaveId = ExNextSlaveId();
            dpslaveid.name = index->prochostname;
            dpslaveid.namelen = strlen(dpslaveid.name) + 1;
            /* Tell new slave it's slaveid */
            _dxf_ExDistMsgfd(DM_SLAVEID, (Pointer)&(dpslaveid), 
                                                      index->procfd);
            /* send state of global dictionary */
            _dxf_ExSendDict(index->procfd, _dxd_exGlobalDict);
            /* send state of macro definitions */
            _dxf_ExSendDict(index->procfd, _dxd_exMacroDict);
            /* add master as first peer */
            ConnectPMtoS(index);

            for(j = 1; j < SIZE_LIST(_dxd_dpgraphstat); j++) {
                if(j == k)
                    continue;
                index2 = FETCH_LIST(_dxd_dpgraphstat, j);
                if(index2->procfd >= 0)
                    ConnectPStoS(index,index2);
	    }
            DXMessage("Connected to %s\n", index->prochostname);
        }
    }

    /* flush non-permanent cache entries */
    if(addedhost)
        _dxf_ExCacheFlush(FALSE);

    if (av) {
        for (i = 0; i < ac; ++i)
            if (av[i] != NULL) {
                ExDebug("*1", "freeing %d %x", i, av[i]);
                DXFree (av[i]);
            }
        ExDebug("*1", "freeing av %x", av);
        DXFree (av);
    }
    return;

error_exit:
    _dxf_ExDie("Could not allocate memory for argument list");
}


/* Connect peers master to slave */
static Error ConnectPMtoS(dpgraphstat *index)
{
    DistMsg		type;
    int  		dxport, nselect;
    int                 dxsock;
#if DXD_SOCKET_UNIXDOMAIN_OK
    int                 dxusock;
#endif
    int                 dxfd    = -1;
    int			len, slaveid;
    struct sockaddr_in  dxserver;
#if DXD_SOCKET_UNIXDOMAIN_OK
    struct sockaddr_un  dxuserver;
#endif
    SlavePeers		spentry;
    fd_set              fdset;


    if(_dxd_exDebug)
        printf("Connecting %s to %s\n", _dxd_exHostName, index->prochostname);

#if DXD_SOCKET_UNIXDOMAIN_OK
    dxport = _dxfSetupServer(OBPORT, &dxsock, &dxserver, &dxusock, &dxuserver);
#else
    dxport = _dxfSetupServer(OBPORT, &dxsock, &dxserver);
#endif
    if (dxport < 0)
    {
        DXUIMessage("ERROR", "#1: Failed connecting %s to %s. Bad port number", 
                        _dxd_exHostName, index->prochostname);
        return(ERROR);
    }

    /* tell slave to connect */
    type = DM_SCONNECT;
    if (write(index->procfd, &type, sizeof(DistMsg)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    len = strlen(_dxd_exHostName) + 1;
    if (write(index->procfd, &len, sizeof(int)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    if (write(index->procfd, _dxd_exHostName, sizeof(char)*len) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    if (write(index->procfd, &dxport, sizeof(int)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    slaveid = 0; /* this slave is the master */
    if (write(index->procfd, &slaveid, sizeof(int)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }

    /* select becomes os2_select in arch.h for os2 */
    FD_ZERO (&fdset);
    FD_SET(index->procfd, &fdset);
    nselect = select (index->procfd + 1, (SELECT_ARG_TYPE *) &fdset, NULL,NULL,NULL);
    if(nselect > 0) {
        if(_dxf_ExReceiveBuffer(index->procfd, &type, 1, TYPE_INT, 0) < 0) {
            DXUIMessage("ERROR", "bad distributed packet type");
            return(ERROR);
        }
        if(type == DPMSG_SIGNATURE || type == DPMSG_SIGNATUREI) {
            if(type == DPMSG_SIGNATUREI)
                index->SwapMsg = TRUE;
            else
                index->SwapMsg = FALSE;
            if(_dxf_ExReceiveBuffer(index->procfd, &type, 1, TYPE_INT,
                                    index->SwapMsg) < 0) {
                DXUIMessage("ERROR", "bad distributed packet type");
                return(ERROR);
            }
        }
        if(type == DM_SACCEPT) {
#if DXD_SOCKET_UNIXDOMAIN_OK
            /* call _dxfCompleteServer and set a timeout on connect */
            dxfd = _dxfCompleteServer(dxsock, dxserver, dxusock, dxuserver, 1);
#else
            /* call _dxfCompleteServer and set a timeout on connect */
            dxfd = _dxfCompleteServer(dxsock, dxserver, 1);
#endif
            if (dxfd < 0)
            {
                DXUIMessage("ERROR", "#2: Failed connecting %s to %s\n", 
                             _dxd_exHostName, index->prochostname);
                return(ERROR);
            }

            len = strlen(index->prochostname) + 1;
            spentry.peername = (char *)DXAllocateLocal(len);
            if(!spentry.peername) 
                _dxf_ExDie("Cannot allocate memory for peername");

            strcpy(spentry.peername, index->prochostname);
            spentry.SlaveId = index->SlaveId;
            spentry.sfd = dxfd;
            spentry.SwapMsg = index->SwapMsg;
            _dxf_ExSendQInit(&(spentry.sendq));
            spentry.wait_on_ack = FALSE;
            spentry.pending_req = FALSE;
            _dxf_ExAddPeer(&spentry);
            DXRegisterInputHandler(ExSendModuleInputHandler, dxfd, NULL);

            if(_dxd_exDebug)
                printf("Done Connect Master to Slave peer at port %d\n", dxport);
            return(OK);
        }
    }
    DXUIMessage("ERROR", "#3: Failed connecting %s to %s", 
                      _dxd_exHostName, spentry.peername);
    return(ERROR);
}

/* Connect peers slave to slave */
static Error ConnectPStoS(dpgraphstat *index, dpgraphstat *index2)
{
    int 	len, port, nselect;
    DistMsg 	type;
    fd_set      fdset;

    if(_dxd_exDebug)
        printf("Connecting %s to %s\n", index->prochostname, 
                                        index2->prochostname);
    type = DM_SLISTEN;
    /* give first slave hostname of slave to connect to */
    if (write(index->procfd, &type, sizeof(DistMsg)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    len = strlen(index2->prochostname) + 1;
    if (write(index->procfd, &len, sizeof(int)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    if (write(index->procfd, index2->prochostname, sizeof(char)*len) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    if (write(index->procfd, &(index2->SlaveId), sizeof(int)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }

    /* wait for a port to accept from */
    FD_ZERO (&fdset);
    FD_SET(index->procfd, &fdset);
    nselect = select (index->procfd + 1, (SELECT_ARG_TYPE *) &fdset, NULL,NULL,NULL);
    if(nselect > 0) {
        if(_dxf_ExReceiveBuffer(index->procfd, &type, 1, TYPE_INT,
                                index->SwapMsg) < 0) {
            DXUIMessage("ERROR", "bad distributed packet type");
            return(ERROR);
        }
        if(type == DM_SCONNECTPORT) {
            if(_dxf_ExReceiveBuffer(index->procfd, &port, 1, TYPE_INT,
                                    index->SwapMsg) < 0) {
                DXUIMessage("ERROR", "bad port number");
                return(ERROR);
            }

            /* tell other slave to connect */
            type = DM_SCONNECT;
            if (write(index2->procfd, &type, sizeof(DistMsg)) == -1) {
                DXUIMessage("ERROR", "Failed writing to socket.");
                return(ERROR);
            }
            len = strlen(index->prochostname) + 1;
            if (write(index2->procfd, &len, sizeof(int)) == -1) {
                DXUIMessage("ERROR", "Failed writing to socket.");
                return(ERROR);
            }
            if (write(index2->procfd, index->prochostname, sizeof(char)*len)  == -1) {
                DXUIMessage("ERROR", "Failed writing to socket.");
                return(ERROR);
            }
            if (write(index2->procfd, &port, sizeof(int))  == -1) {
                DXUIMessage("ERROR", "Failed writing to socket.");
                return(ERROR);
            }
            if (write(index2->procfd, &(index->SlaveId), sizeof(int))  == -1) {
                DXUIMessage("ERROR", "Failed writing to socket.");
                return(ERROR);
            }

            FD_ZERO (&fdset);
            FD_SET(index2->procfd, &fdset);
            nselect = select (index2->procfd + 1, (SELECT_ARG_TYPE *) &fdset, 
                                                         NULL,NULL,NULL);
            if(nselect > 0) {
                /* when second slave has connected tell first slave to accept */
                if(_dxf_ExReceiveBuffer(index2->procfd, &type, 1, TYPE_INT, 
                                        index2->SwapMsg) < 0) {
                    DXUIMessage("ERROR", "bad distributed packet type");
                    return(ERROR);
                }
                if(type == DM_GRAPHDONE) {
                    _dxf_SlaveDone();
                    FD_ZERO (&fdset);
                    FD_SET(index2->procfd, &fdset);
                    nselect = select (index2->procfd + 1, (SELECT_ARG_TYPE *) &fdset, 
                                      NULL,NULL,NULL);
                    if(nselect <= 0) 
                        goto error_return;
                    if(_dxf_ExReceiveBuffer(index2->procfd, &type, 1,
                                             TYPE_INT, index2->SwapMsg) < 0) {
                        DXUIMessage("ERROR", "bad distributed packet type");
                        return(ERROR);
                    }
                }
                if(type == DM_SACCEPT) {
                    if (write(index->procfd, &type, sizeof(DistMsg))  == -1) {
                        DXUIMessage("ERROR", "Failed writing to socket.");
                        return(ERROR);
                    }
                    if(_dxd_exDebug) {
                        printf("connected %s to %s at port %d\n", 
                          index->prochostname, index2->prochostname, port);
                        printf("Done Connect\n");
                    }
                    return(OK);
                }
                else {
                    DXUIMessage("ERROR", 
                                "#4: Failed connecting %s to %s", 
                                index->prochostname, index2->prochostname);
                    DXMessage("Peer did not accept connection, packet type returned %d\n", type);
                    return(ERROR);
                }
            } /* select */
        }
        else {
error_return:
            DXUIMessage("ERROR", 
                        "5: Failed connecting %s to %s", 
                            index->prochostname, index2->prochostname);
            return(ERROR);
        }
    } /* select */   
    if(_dxd_exDebug) {
        printf("connected %s to %s at port %d\n", index->prochostname, index2->prochostname, port);
        printf("Done Connect\n");
    }
   return OK; 
}

Error _dxf_ExSlaveListen()
{
    DistMsg		msgtype;
    int  		dxport;
    int                 dxsock;
#if DXD_SOCKET_UNIXDOMAIN_OK
    int                 dxusock;
#endif
    int                 dxfd    = -1;
    int			len;
    struct sockaddr_in  dxserver;
#if DXD_SOCKET_UNIXDOMAIN_OK
    struct sockaddr_un  dxuserver;
#endif
    SlavePeers		spentry;

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &len, 1, TYPE_INT, 
                            _dxd_exSwapMsg) < 0) 
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

    spentry.peername = (char *)DXAllocateLocal(len);
    if(!spentry.peername) 
        _dxf_ExDie("%s: could not allocate memory", _dxd_exHostName);
    spentry.SwapMsg = FALSE;

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, spentry.peername, len,
                            TYPE_UBYTE, _dxd_exSwapMsg) < 0) 
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &(spentry.SlaveId), 1,
                            TYPE_INT, _dxd_exSwapMsg) < 0)
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

#if DXD_SOCKET_UNIXDOMAIN_OK
    dxport = _dxfSetupServer(OBPORT, &dxsock, &dxserver, &dxusock, &dxuserver);
#else
    dxport = _dxfSetupServer(OBPORT, &dxsock, &dxserver);
#endif
    if (dxport < 0)
    {
        DXFree(spentry.peername);
        msgtype = DM_CONNECTERROR;
        write(_dxd_exMasterfd, &msgtype, sizeof(DistMsg));
        return(ERROR);
    }

    msgtype = DM_SCONNECTPORT;
    if (write(_dxd_exMasterfd, &msgtype, sizeof(DistMsg)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    }
    if (write(_dxd_exMasterfd, &dxport, sizeof(int)) == -1) {
        DXUIMessage("ERROR", "Failed writing to socket.");
        return(ERROR);
    } 
 
    /* wait for accept */
    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &msgtype, 1, TYPE_INT,
                            _dxd_exSwapMsg) < 0)
    {
        DXUIMessage("ERROR", "%s: bad distribute packet type", _dxd_exHostName);
        return(ERROR);
    }
    if(msgtype == DM_SACCEPT) {
#if DXD_SOCKET_UNIXDOMAIN_OK
        /* call _dxfCompleteServer and set a timeout on connect */
        dxfd = _dxfCompleteServer(dxsock, dxserver, dxusock, dxuserver, 1);
#else
        /* call _dxfCompleteServer and set a timeout on connect */
        dxfd = _dxfCompleteServer(dxsock, dxserver, 1);
#endif
        if (dxfd < 0)
        {
            DXUIMessage("ERROR","Connection to %s failed.\n", spentry.peername);
            DXFree(spentry.peername);
            return(ERROR);
        }
        spentry.sfd = dxfd;
        _dxf_ExSendQInit(&(spentry.sendq));
        spentry.wait_on_ack = FALSE;
        spentry.pending_req = FALSE;
        _dxf_ExAddPeer(&spentry);
        DXRegisterInputHandler(ExSendModuleInputHandler, dxfd, NULL);
        /* send signature to peer, this must be the first msg that */
        /* the peer gets to determine if it needs to swap bytes    */
        msgtype = (DistMsg) DPMSG_SIGNATURE;
        if (write(dxfd, &msgtype, sizeof(int)) == -1) {
            DXUIMessage("ERROR", "Failed writing to socket.");
            return(ERROR);
        } 
    }
    else {
        DXUIMessage("ERROR", "Connect to %s failed.", spentry.peername);  
        DXFree(spentry.peername);
        return(ERROR);
    }
    return(OK);
}

Error _dxf_ExSlaveConnect()
{
    int len, port, fd;
    SlavePeers spentry;
    DistMsg msgtype = DM_SACCEPT;
    spentry.peername = NULL;
    spentry.SwapMsg = FALSE;

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &len, 1, TYPE_INT, 
                            _dxd_exSwapMsg) < 0) 
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

    spentry.peername = (char *)DXAllocateLocal(len);
    if(!spentry.peername) 
        _dxf_ExDie("%s: couldn't allocate memory", _dxd_exHostName);

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, spentry.peername, len,
                            TYPE_UBYTE, _dxd_exSwapMsg) < 0)
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &port, 1, TYPE_INT,
                            _dxd_exSwapMsg) < 0) 
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &(spentry.SlaveId),1,
                            TYPE_INT, _dxd_exSwapMsg) < 0)
        _dxf_ExDie("%s: bad packet data", _dxd_exHostName);

    /* connecting as peer to master */
    if(spentry.SlaveId == 0) {
        msgtype = (DistMsg) DPMSG_SIGNATURE;
        if (write(_dxd_exMasterfd, &msgtype, sizeof(int)) == -1) {
            DXUIMessage("ERROR", "Failed writing to socket.");
            return(ERROR);
        }
        spentry.SwapMsg = _dxd_exSwapMsg;
    }
    fd = DXConnectToServer(spentry.peername, port);
    if(fd > 0) {
        spentry.sfd = fd;
        _dxf_ExSendQInit(&(spentry.sendq));
        spentry.wait_on_ack = FALSE;
        spentry.pending_req = FALSE;
        _dxf_ExAddPeer(&spentry);
        DXRegisterInputHandler(ExSendModuleInputHandler, fd, NULL);
        msgtype = DM_SACCEPT;
        if (write(_dxd_exMasterfd, &msgtype, sizeof(DistMsg)) == -1) {
            DXUIMessage("ERROR", "Failed writing to socket.");
            return(ERROR);
        }

        /* send signature to peer, this must be the first msg that */
        /* the peer gets to determine if it needs to swap bytes    */
        msgtype = (DistMsg) DPMSG_SIGNATURE;
        if (write(fd, &msgtype, sizeof(int)) == -1) {
            DXUIMessage("ERROR", "Failed writing to socket.");
            return(ERROR);
        }
	

	/* send MP license failed message now that it's safe to use DXMessage */
#ifdef DXD_LICENSED_VERSION
	if(spentry.SlaveId == 0) {
	    ExLicenseFinish();	
       	}
#endif

	return(OK);
    }

    DXFree(spentry.peername);
    msgtype = DM_CONNECTERROR;
    write(_dxd_exMasterfd, &msgtype, sizeof(DistMsg));
    return(ERROR);
}

int _dxf_SuspendPeers()
{
    int i, ilimit;
    SlavePeers *sp;
    for (i = 0, ilimit = SIZE_LIST(_dxd_slavepeers); i < ilimit; ++i)
    {
	sp = FETCH_LIST(_dxd_slavepeers, i);
        if(sp->sfd == -99) 
            continue;
	DXRegisterInputHandler(NULL, sp->sfd, NULL);
    }
    return OK;
}

int _dxf_ResumePeers()
{
    int i, ilimit;
    SlavePeers *sp;
    for (i = 0, ilimit = SIZE_LIST(_dxd_slavepeers); i < ilimit; ++i)
    {
	sp = FETCH_LIST(_dxd_slavepeers, i);
        if(sp->sfd == -99) 
            continue;
	DXRegisterInputHandler(ExSendModuleInputHandler, sp->sfd, NULL);
    }
    return OK;
}

