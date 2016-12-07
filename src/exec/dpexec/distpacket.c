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
#if defined(HAVE_WINIOCTL_H)
#include <winioctl.h>
#endif
#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif
#if defined(HAVE_SYS_FILIO_H)
#include <sys/filio.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif
#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif

#include "config.h"
#include "graph.h"
#include "exobject.h"
#include "d.h"
#include "sysvars.h"
#include "cache.h"
#include "distp.h"
#include "parse.h"
#include "obmodule.h"
#include "context.h"
#include "_variable.h"
#include "packet.h"
#include "vcr.h"
#include "log.h"
#include "nodeb.h"
#include "parsemdf.h"
#include "evalgraph.h"
#include "rih.h"
#include "command.h"
#include "_macro.h"

typedef struct dmargs
{
    DistMsg type;
    Pointer data;
    int to;
} dmargs;

extern Object _dxfExportBin_FP(Object o, int fd); /* from libdx/rwobject.c */
extern Object _dxfImportBin_FP(int fd); /* from libdx/rwobject.c */
extern Error _dxfByteSwap(void *dest, void *src, int ndata, Type t); /* from libdx/edfdata.c */


static void ExSendGDictPkg(GDictSend *pkg, int fd);
static void ExSendUIPkg(UIMsgPackage *pkg, int fd);
static void ExRecvUIPkg(int fd, int swap);
static void ExDPSendGvarPkg(DPSendPkg *pkg, int fd);
static void ExRecvDPSendPkg(int fd, int swap);
static Error ExCheckAcknowledge(SlavePeers *sp);
static void ExOKToSend(SlavePeers *sp);
static void ExCheckSendReq(SlavePeers *sp);
static Error DistributeMsgAP(dmargs *argp);
static void DistributeMsg(DistMsg type, Pointer data, int to);

static LIST(uint32) SavedCacheTags;
static int dontSendCacheTags = FALSE;
static int nslaves_done;

#define RCV_BUF(fd, ptr, n, t, s) \
    if(_dxf_ExReceiveBuffer(fd, ptr, n, t, s) < 0) { \
	DXUIMessage("ERROR", "Received bad distributed packet"); \
        return; \
    } 


int 
_dxf_ExReceiveBuffer(int fd, Pointer buffer, int ndata, Type t, int swap) 
{
    int sts;
    int already = 0;
    int size;
   
    size = ndata * DXTypeSize(t);
    do {
	sts = read(fd, ((char*)buffer) + already, size);
	already += sts;
	size -= sts;
    } while (sts > 0 && size > 0);

    if(swap)
        _dxfByteSwap(buffer, buffer, ndata, t);

    return sts;
}

int 
_dxf_ExWriteSock(int fd, Pointer buffer, int size)
{
    int sts;
    sts = write(fd, (char *)buffer, size);
    if(sts != size)
        DXUIMessage("ERROR", "error writing to socket");
    return(sts);
}

static void ReceivePVRequired(int fd, int swap)
{
    int pkt[5];

    RCV_BUF(fd, pkt, 5, TYPE_INT, swap);

    _dxf_ExMarkVarRequired(pkt[0], pkt[1], pkt[2], pkt[3], pkt[4]);
    
}

Error _dxf_ExReceivePeerPacket(SlavePeers *sp)
{
    DistMsg	type;
    int         graphId;
    UIPackage	pkg;
    
    if(_dxf_ExReceiveBuffer(sp->sfd, &type, 1, TYPE_INT, sp->SwapMsg) < 0) {
#if 0
          DXUIMessage("ERROR", "bad distributed packet type %d", type);
          return(ERROR);
#endif
          _dxf_ExDie("bad distributed packet type %d", type);
    }
    if(type == DPMSG_SIGNATURE || type == DPMSG_SIGNATUREI) {
        if(_dxd_exDebug)
            printf("signature %x\n", type);
        /* if this is peer 0 - master peer - than SwapMsg will already be set */
        if(!sp->SwapMsg && type == DPMSG_SIGNATUREI)
            sp->SwapMsg = TRUE;
        return(OK);
    }

    if(_dxd_exDebug) 
        printf("packet type %d from peer %s\n", type, sp->peername); 
    else
        ExDebug("9", "packet type %d from peer %s", type, sp->peername); 
    switch(type) {
      case DM_DPSENDREQ:
          return(ExCheckAcknowledge(sp));
          break;
      case DM_DPSENDACK:
          ExOKToSend(sp);
          break;
      case DM_DPSEND:
          ExRecvDPSendPkg(sp->sfd, sp->SwapMsg);
          /* do we need to send out another request? */
          ExCheckSendReq(sp);
          break;
      case DM_UIMSG:
          ExRecvUIPkg(sp->sfd, sp->SwapMsg);
          break;
      case DM_UIPCK:
          if(_dxf_ExReceiveBuffer(sp->sfd, &(pkg.len), 1, TYPE_INT, 
                                  sp->SwapMsg) < 0) {
              DXUIMessage("ERROR", "bad packet data");
              return(ERROR);
          }
          if(_dxf_ExReceiveBuffer(sp->sfd, pkg.data, pkg.len, TYPE_UBYTE, 
                                  sp->SwapMsg) < 0)
          {
              DXUIMessage("ERROR", "bad packet data");
              return(ERROR);
          }
          if(_dxd_exRemote)
              _dxf_ExCheckPacket(pkg.data, pkg.len);
          break;
      case DM_INSERTGDICT:
          _dxf_ExRecvGDictPkg(sp->sfd, sp->SwapMsg, 0);
          break;
      case DM_INSERTGDICTNB:
          _dxf_ExRecvGDictPkg(sp->sfd, sp->SwapMsg, 1);
          break;
      case DM_INSERTCACHE:
          _dxf_ExCacheInsertRemoteTag(sp->sfd, sp->SwapMsg);
          break;
      case DM_EVICTCACHE:
          _dxf_ExCacheDeleteRemoteTag(sp->sfd, sp->SwapMsg);
          break;
      case DM_EVICTCACHELIST:
          _dxf_ExCacheListDeleteRemoteTag(sp->sfd, sp->SwapMsg);
          break;
      case DM_PVREQUIRED:
	  ReceivePVRequired(sp->sfd, sp->SwapMsg);
          break;
      case DM_BADFUNC:
          *_dxd_exKillGraph = TRUE;
          *_dxd_exBadFunc = TRUE;
          break;
      case DM_KILLEXECGRAPH:
          if(_dxf_ExReceiveBuffer(sp->sfd, &graphId, 1, TYPE_INT, 
                                  sp->SwapMsg) < 0) {
              DXUIMessage("ERROR", "bad packet data");
              return(ERROR);
          }
          /* don't actually do anything with graphId. Could be 
           * different from current graphId if coming from interrupt
           * packet from UI.
           */
          *_dxd_exKillGraph = TRUE;
          break;
      case DM_VCRREDO:
          _dxf_ExVCRRedo();
          break;
      default: 
          _dxf_ExDie("bad distributed packet type %d", type);
          break;
    }
    return(OK);
}


/*---------------------------------------------------*/
/* Common routine to funnel all msgs passed between  */
/* the master and slave execs. The size argument is  */
/* the size of the data. It can be 0 if all data is  */
/* in shared memory and accessable to all processors.*/
/*---------------------------------------------------*/

void _dxf_ExDistributeMsg(DistMsg type, Pointer data, int size, int to)
{
    int procId = 1;

    if(!_dxd_exRemoteSlave && *_dxd_exNSlaves <= 0)
        return;

    if(exJID == procId)
        DistributeMsg(type, data, to);
    else {
        dmargs *argp;

        argp = (dmargs *) DXAllocate(sizeof(dmargs));
        if(argp == NULL)
            _dxf_ExDie("Could not allocate memory for distributed message");
        argp->type = type;
        if(size > 0) {
            argp->data = (Pointer)DXAllocate (size);
            if(argp->data == NULL)
                _dxf_ExDie("could not allocate memory for distributed msg");
            memcpy (argp->data, data, size);
        }
        else argp->data = data;

        argp->to = to;
        _dxf_ExRunOn (1, DistributeMsgAP, argp, 0);
        if(size > 0)
            DXFree(argp->data); 
        DXFree(argp);
    }
}

static Error DistributeMsgAP(dmargs *argp)
{
    DistributeMsg(argp->type, argp->data, argp->to);
    return OK; 
}

static void DistributeMsg(DistMsg type, Pointer data, int to)
{
    int i, limit;
    dpgraphstat *index;
    SlavePeers *spindex;

    switch(to) {
      case TOSLAVES:
        /* entry 0 is for a dummy for the master so skip it */
        for (i = 1, limit = SIZE_LIST(_dxd_dpgraphstat); i < limit; ++i)
        {
           index = FETCH_LIST(_dxd_dpgraphstat, i);
           if (index->procfd < 0)
               continue;
           _dxf_ExDistMsgfd(type, data, index->procfd);
        }
        break;
      case TOPEERS:
	if (type == DM_EVICTCACHELIST && dontSendCacheTags)
	{
	    CacheTagList *ctpkg;
	    ctpkg = (CacheTagList *)data;
	    for (i = 0; i < ctpkg->numtags; ++i)
		APPEND_LIST(uint32, SavedCacheTags, ctpkg->ct[i]);
	    return;
	}
        if(dontSendCacheTags)
            DXWarning("Message type %d sent in middle of Export", type);
        for (i = 0, limit = SIZE_LIST(_dxd_slavepeers); i < limit; ++i)
        {
           spindex = FETCH_LIST(_dxd_slavepeers, i);
           if (spindex->sfd < 0 || spindex->peername == NULL)
               continue;
           _dxf_ExDistMsgfd(type, data, spindex->sfd);
        }
        break;
      case TOPEER0:
        spindex = FETCH_LIST(_dxd_slavepeers, 0);
        _dxf_ExDistMsgfd(type, data, spindex->sfd);
        break;
      default: /* TOMASTER */
        _dxf_ExDistMsgfd(type, data, _dxd_exMasterfd);
    }
}

/* given a process group, return the socket file descriptor to the current
 * host to which that group is assigned.
 */
int _dxf_ExGetSocketId(char *procname)
{
    int i, limit;
    char *hostname;
    dpgraphstat *index;

    hostname = _dxf_ExGVariableGetStr(procname);

    /* entry 0 is for a dummy entry for the master */
    for (i = 1, limit = SIZE_LIST(_dxd_dpgraphstat); i < limit; ++i)
    {
       index = FETCH_LIST(_dxd_dpgraphstat, i);
       if (!strcmp(index->prochostname, hostname))
	   return index->procfd;
    }
    return (-1);
}


void _dxf_ExDistMsgfd(DistMsg type, Pointer data, int tofd)
{
    UIPackage *uipkg;
    CacheTagList *ctpkg;
    DelRemote *drpkg;
    DictType whichdict = DICT_NONE;
    dpversion *dpv=NULL;
    dpslave_id *dpslaveid;
    char *name;
    int len;

    if(tofd < 0)
        return;
    if(_dxd_exDebug)
        printf("sending pkg %d\n", type);
    else
        ExDebug("7", "sending pkg %d", type);
    if(type == DM_VERSION) {
        /* need to write signature first */
        dpv = (dpversion *)data;
        _dxf_ExWriteSock(tofd, &dpv->signature, sizeof(int));
    }
    _dxf_ExWriteSock(tofd, &type, sizeof(DistMsg));
    switch(type) {
      case DM_PARSETREE:
      case DM_INSERTMDICT:
          _dxf_ExWriteTree((struct node *)data, tofd);
          break;
      case DM_EXECGRAPH: 
      case DM_KILLEXECGRAPH: 
          _dxf_ExWriteSock(tofd, &data, sizeof(int));
          break;
      case DM_EXIT:
          _dxf_ExWriteSock(tofd, (int *)data, sizeof(int));
          break;
      case DM_BADFUNC:
      case DM_GRAPHDONE:
          break;
      case DM_INSERTGDICT:  
      case DM_INSERTGDICTNB:  
          ExSendGDictPkg((GDictSend *)data, tofd);
          break;
      case DM_UIMSG:
          ExSendUIPkg((UIMsgPackage *)data, tofd);
          break;
      case DM_UIPCK:
          uipkg = (UIPackage *)data;
          _dxf_ExWriteSock(tofd, &(uipkg->len), sizeof(int));
          _dxf_ExWriteSock(tofd, uipkg->data, sizeof(char)*uipkg->len);
          break;
      case DM_DPSENDREQ:
          _dxf_ExWriteSock(tofd, (int *)data, sizeof(int));
          break;
      case DM_DPSENDACK:
          break;
      case DM_DPSEND:
          ExDPSendGvarPkg((DPSendPkg *)data, tofd);
          break;
      case DM_INSERTCACHE:
      case DM_EVICTCACHE:
          _dxf_ExWriteSock(tofd, (uint32 *)data, sizeof(uint32));
          break;
      case DM_EVICTCACHELIST:
          ctpkg = (CacheTagList *)data;
          _dxf_ExWriteSock(tofd, &(ctpkg->numtags), sizeof(int));
          _dxf_ExWriteSock(tofd, ctpkg->ct, ctpkg->numtags * 
                                             sizeof(uint32));
          break;
      case DM_SLAVEID:
          dpslaveid = (dpslave_id *)data;
          _dxf_ExWriteSock(tofd, (int *)&dpslaveid->id, sizeof(int));
          _dxf_ExWriteSock(tofd, (int *)&dpslaveid->namelen, sizeof(int));
          _dxf_ExWriteSock(tofd, dpslaveid->name, dpslaveid->namelen);
          break;
      case DM_VERSION:
          _dxf_ExWriteSock(tofd, (int *)&dpv->version, sizeof(int));
          break;
      case DM_FLUSHGLOBAL:
      case DM_FLUSHMACRO:
          break;
      case DM_FLUSHCACHE:
          _dxf_ExWriteSock(tofd, (int *)data, sizeof(int));
          break;
      case DM_FLUSHDICT:
          if((EXDictionary)data == _dxd_exGlobalDict)
              whichdict = DICT_GLOBAL;
          else if((EXDictionary)data == _dxd_exMacroDict)
              whichdict = DICT_MACRO;
          else if((EXDictionary)data == _dxd_exGraphCache)
              whichdict = DICT_GRAPH;
          else DXUIMessage("ERROR", "flush dictionary: bad dictionary");
          _dxf_ExWriteSock(tofd, &whichdict, sizeof(int));
          break;
      case DM_GRAPHDELETE:
      case DM_GRAPHDELETECONF:
      case DM_VCRREDO:
          break;
      case DM_PERSISTDELETE:
	  drpkg = (DelRemote *)data;
	  _dxf_ExWriteSock(tofd, &drpkg->del_namelen, sizeof(int));
	  _dxf_ExWriteSock(tofd, drpkg->del_name, drpkg->del_namelen);
	  _dxf_ExWriteSock(tofd, &drpkg->del_instance, sizeof(int));
          break;
      case DM_LOADMDF:
	  _dxf_ExSendMdfPkg(data, tofd);
	  break;
      case DM_PVREQUIRED:
          _dxf_ExWriteSock(tofd, (int *)data, 5 * sizeof(int));
          break;
      case DM_DELETEPEER:
          name = (char *)data;
          len = strlen(name) + 1;
	  _dxf_ExWriteSock(tofd, &len, sizeof(int));
	  _dxf_ExWriteSock(tofd, name, len);
          break;
      default:
          DXMessage("sending bad package type %d", type);
          break;
    }
}

Error
_dxf_ExCreateGDictPkg(GDictSend *pkg, char *name, EXObj ex_obj)
{
    gvar *gv;

    /* are we running distributed? */
    if(!_dxd_exRemoteSlave && *_dxd_exNSlaves <= 0)
        return(OK);

    pkg->varname = name;
    pkg->exclass = ex_obj->exclass;
    if(pkg->exclass == EXO_CLASS_GVAR) {
        gv = (gvar *)ex_obj;
        pkg->gvp.type = gv->type;
        pkg->gvp.reccrc = gv->reccrc;
        pkg->gvp.cost = gv->cost;
        pkg->gvp.skip = gv->skip;
        pkg->gvp.disable_cache = gv->disable_cache;
        pkg->gvp.procId = gv->procId;
        if(gv->obj)
            pkg->gvp.hasobj = 1;
        else pkg->gvp.hasobj = 0;
        pkg->obj = gv->obj;
        return(OK);
    }
    else return(ERROR);
}

static void
SendSavedCacheTags()
{                  
    CacheTagList ctpkg;  
    int i;
    int numsaved = SIZE_LIST(SavedCacheTags);
    uint32 *sct;

    while(numsaved > 0) {
        sct = (uint32 *)SavedCacheTags.vals;
        ctpkg.numtags = (numsaved > N_CACHETAGLIST_ITEMS) ?
					N_CACHETAGLIST_ITEMS : numsaved;
        for(i = 0; i < ctpkg.numtags; i++) 
            ctpkg.ct[i] = sct[i];
        _dxf_ExDistributeMsg(DM_EVICTCACHELIST, (Pointer)&ctpkg, 
                             sizeof(CacheTagList), TOPEERS); 
        numsaved -= ctpkg.numtags;
        sct += ctpkg.numtags;
    } 
    INIT_LIST(SavedCacheTags);   
}     

static void
ExSendGDictPkg(GDictSend *pkg, int fd)
{
    int len, ret;

    len = strlen(pkg->varname) + 1;
    ret = _dxf_ExWriteSock(fd, &len, sizeof(int));
    if(ret != sizeof(int))
        printf("_dxf_ExWriteSock returned %d instead of 1\n", ret);
    ret = _dxf_ExWriteSock(fd, pkg->varname, sizeof(char)*len);
    if(ret != len*sizeof(char))
        printf("_dxf_ExWriteSock returned %d instead of %d\n", ret, len);
    ret = _dxf_ExWriteSock(fd, &(pkg->exclass), sizeof(exo_class));
    if(ret != sizeof(exo_class))
        printf("_dxf_ExWriteSock returned %d instead of 1\n", ret);
    ret = _dxf_ExWriteSock(fd, &(pkg->gvp), sizeof(gvarpkg));
    if(ret != sizeof(gvarpkg))
        printf("_dxf_ExWriteSock returned %d instead of 1\n", ret);
    if(pkg->obj)
    {
	dontSendCacheTags = TRUE;
        _dxfExportBin_FP (pkg->obj, fd);
	dontSendCacheTags = FALSE;
        SendSavedCacheTags();
    }
}

gvar *
ConverttoGvar(gvarpkg *gp, Object obj)
{
    gvar *gv;

    gv = _dxf_ExCreateGvar (gp->type);
    if(obj)
        _dxf_ExDefineGvar (gv, obj);
    gv->reccrc = gp->reccrc;
    gv->cost = gp->cost;
    gv->skip = (_skip_state) gp->skip;
    gv->disable_cache = gp->disable_cache;
    gv->procId = gp->procId;
    return(gv);
}

void 
_dxf_ExRecvGDictPkg(int fd, int swap, int nobkgrnd)
{
    GDictSend pkg;
    gvar *gv;
    int len;

    RCV_BUF(fd, &len, 1, TYPE_INT, swap);
    pkg.varname = DXAllocate(len);
    if(!pkg.varname) 
        _dxf_ExDie("Could not allocate memory for variable name");
    RCV_BUF(fd, pkg.varname, len, TYPE_UBYTE, swap);
    if(_dxd_exDebug)
        printf("var %s\n", pkg.varname); 
    else 
        ExDebug("7", "var %s", pkg.varname); 
    RCV_BUF(fd, &(pkg.exclass), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.gvp.type), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.gvp.reccrc), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.gvp.cost), 1, TYPE_DOUBLE, swap);
    RCV_BUF(fd, &(pkg.gvp.skip), 4, TYPE_INT, swap);
    pkg.obj = NULL;
    if(pkg.gvp.hasobj) 
        pkg.obj = _dxfImportBin_FP (fd);
    
    if(pkg.exclass == EXO_CLASS_GVAR) {
        gv = ConverttoGvar(&(pkg.gvp), pkg.obj);
        if(pkg.gvp.hasobj && pkg.obj == NULL) {
            gv->skip = GV_SKIPERROR;
            DXPrintError(NULL);
            return;
        }
        if(nobkgrnd)
            _dxf_ExVariableInsertNoBackground(pkg.varname, 
                                         _dxd_exGlobalDict, (EXObj)gv);
        else 
            _dxf_ExVariableInsert(pkg.varname, _dxd_exGlobalDict, (EXObj)gv);
    }
    DXFree(pkg.varname);
}

static void
ExDPSendGvarPkg(DPSendPkg *pkg, int fd)
{
    int ret;

    ret = _dxf_ExWriteSock(fd, &(pkg->index), sizeof(int));
    if(ret != sizeof(int))
        printf("_dxf_ExWriteSock returned %d instead of %ld\n",ret,sizeof(int));
    ret = _dxf_ExWriteSock(fd, &(pkg->excache), sizeof(int));
    if(ret != sizeof(int))
        printf("_dxf_ExWriteSock returned %d instead of %ld\n",ret,sizeof(int));
    ret = _dxf_ExWriteSock(fd, &(pkg->gvp), sizeof(gvarpkg));
    if(ret != sizeof(gvarpkg))
        printf("_dxf_ExWriteSock returned %d instead of %ld\n", ret, 
                                                     sizeof(gvarpkg));
    if(pkg->obj)
    {
	dontSendCacheTags = TRUE;
        _dxfExportBin_FP (pkg->obj, fd);
	dontSendCacheTags = FALSE;
        SendSavedCacheTags();
    }
}

static void
ExRecvDPSendPkg(int fd, int swap)
{
    DPSendPkg           pkg;
    ProgramVariable	*pv;

    RCV_BUF(fd, &(pkg.index), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.excache), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.gvp.type), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.gvp.reccrc), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.gvp.cost), 1, TYPE_DOUBLE, swap);
    RCV_BUF(fd, &(pkg.gvp.skip), 4, TYPE_INT, swap);
    pkg.obj = NULL;
    if(pkg.gvp.hasobj) 
        pkg.obj = _dxfImportBin_FP (fd);

    pv = FETCH_LIST(_dxd_exContext->program->vars, pkg.index);
    if(pv == NULL) {
        *_dxd_exKillGraph = TRUE;
        DXUIMessage("ERROR", "Missing variable in program");
        return;
    }
    if(pv->gv == NULL)
        pv->gv = ConverttoGvar(&(pkg.gvp), pkg.obj);

    DXDebug("1", "received index %d 0x%08x 0x%08x\n", pkg.index, pv->gv->reccrc, pkg.gvp.reccrc);
    if ( pv->gv->reccrc != pkg.gvp.reccrc) {
        DXUIMessage("ERROR", "%s: bad packet data -- recipe", _dxd_exHostName);
        printf("%x, %x\n", pv->gv->reccrc, pkg.gvp.reccrc);
	pv->gv->reccrc = pkg.gvp.reccrc;
    }
    pv->gv->cost = pkg.gvp.cost;
    pv->gv->skip = (_skip_state) pkg.gvp.skip;
    pv->gv->disable_cache = pkg.gvp.disable_cache;
    pv->gv->procId = pkg.gvp.procId;

    if(pkg.gvp.hasobj && pkg.obj == NULL) {
        pv->gv->skip = GV_SKIPERROR;
        DXPrintError(NULL);
        return;
    }
    _dxf_ExDefineGvar(pv->gv, pkg.obj);
    if(_dxd_exCacheOn && pkg.excache && !pkg.gvp.disable_cache && !pkg.gvp.skip)
        _dxf_ExCacheInsert (pv->gv);
}


void _dxf_ExSendDict(int fd, EXDictionary dict)
{
    char *key;
    
    _dxf_ExDictionaryBeginIterate (dict);

    if(dict == _dxd_exMacroDict) {
        node *n;
        while((n = (node *) _dxf_ExDictionaryIterate (_dxd_exMacroDict, &key)))
        {
            /* don't send over normal modules, only macros 
	     *  or modules which were loaded at runtime.
	     */
            if(n->type == NT_MACRO)
                _dxf_ExDistMsgfd(DM_INSERTMDICT, (Pointer)n, fd);
	/* this the change for sending over outboard modules.  nsc 21nov93 */
	    else if (n->type == NT_MODULE && (n->v.module.flags & RUNTIMELOAD))
		_dxf_ExDistMsgfd(DM_INSERTMDICT, (Pointer)n, fd);
        }
    }
    else if(dict == _dxd_exGlobalDict) {
        EXObj obj;
        GDictSend pkg;

        while((obj = _dxf_ExDictionaryIterate (_dxd_exGlobalDict, &key)))
        {
            _dxf_ExCreateGDictPkg(&pkg, key, obj);
            _dxf_ExDistMsgfd(DM_INSERTGDICT, (Pointer)&pkg, fd);
        } 
    }

    _dxf_ExDictionaryEndIterate (dict);
}

static void 
ExSendUIPkg(UIMsgPackage *pkg, int fd) 
{
    _dxf_ExWriteSock(fd, &(pkg->ptype), sizeof(int));
    _dxf_ExWriteSock(fd, &(pkg->graphId), sizeof(int));
    _dxf_ExWriteSock(fd, &(pkg->len), sizeof(int));
    _dxf_ExWriteSock(fd, pkg->data, sizeof(char)*pkg->len);
}

static void 
ExRecvUIPkg(int fd, int swap) 
{
    UIMsgPackage pkg;

    RCV_BUF(fd, &(pkg.ptype), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.graphId), 1, TYPE_INT, swap);
    RCV_BUF(fd, &(pkg.len), 1, TYPE_INT, swap);
    if(pkg.len > MSG_BUFLEN) {
        DXUIMessage("ERROR", "UI packet too big");
        return;
    }
    RCV_BUF(fd, pkg.data, pkg.len, TYPE_UBYTE, swap);
    _dxf_ExQMessage(pkg.ptype, pkg.graphId, pkg.len, pkg.data);
}

void _dxf_ResetSlavesDone()
{
    nslaves_done = 0;
}

void _dxf_SlaveDone()
{
    nslaves_done++; 
}

Error
_dxf_ExWaitOnSlaves()
{
    fd_set              fdset;
    int 		nslaves, limit, k, nselect;
    dpgraphstat 	*index; 
    DistMsg		pcktype;
    int			maxfd;
    int			b;
    SlavePeers          *sp;
    int                 repeat_loop;

    ExDebug("1", "Waiting on slaves");
    /* make sure all outstanding send requests are handled */
    for(k = 0; k < SIZE_LIST(_dxd_slavepeers); k++) {
        sp = FETCH_LIST(_dxd_slavepeers, k);
        while(sp->sendq.head != NULL)
            _dxf_ExCheckRIH();
    }
    repeat_loop = FALSE;

wait:
    nslaves = 0;
    maxfd = 0;
    FD_ZERO (&fdset);
    limit = SIZE_LIST(_dxd_dpgraphstat);
    /* entry 0 is a dummy for the master */
    for (k = 1; k < limit; ++k) {
        index = FETCH_LIST(_dxd_dpgraphstat, k);
        if(index->procfd > 0 && (index->numpgrps > 0)) {
            nslaves++;
            FD_SET(index->procfd, &fdset);
            if(index->procfd > maxfd) maxfd = index->procfd;
        }
    }
    if(nslaves > 0)
        ExDebug("7", "waiting on %d slaves", nslaves);

    while(nslaves_done < nslaves) {
        _dxf_ExCheckRIH ();
        nselect = select (maxfd + 1, (SELECT_ARG_TYPE *) &fdset, NULL, NULL, NULL);
        if(nselect > 0) { 
            /* find out who is sending messages */
            for (k = 0; k < limit; ++k) {
                index = FETCH_LIST(_dxd_dpgraphstat, k);
                if(index->procfd > 0 && (index->numpgrps > 0)) {
                    if (FD_ISSET(index->procfd, &fdset)) {
                        if((IOCTL(index->procfd, FIONREAD, (char *)&b) < 0) || 
                            (b <= 0)) {
                            ExDebug("7", "EOF from slave socket");
                            _dxf_SlaveDone();
                            close(index->procfd);
                            /* If we have to, wait for the child. */
                            if (!_dxd_exDebugConnect)
                                wait(0);
                            index->procfd = -1;
                            /* send error message? 
                             * FIX - remove from table and send a message?  
                             */
                            continue;
                        }
                        if(_dxf_ExReceiveBuffer(index->procfd, &pcktype, 1,
                                                TYPE_INT, index->SwapMsg) < 0) 
                            DXUIMessage("ERROR", "bad distributed packet type");
			if(_dxd_exDebug)
			    printf("packet type %d from %s fd %d\n",
				pcktype, index->prochostname, index->procfd);
			else
			    ExDebug("7", "packet type %d from %s fd %d",
				pcktype, index->prochostname, index->procfd);
                        switch(pcktype) {
                          case DM_GRAPHDONE:
                              _dxf_SlaveDone();
                              break;
                          case DM_GRAPHDELETECONF:
                              _dxf_SlaveDone();
                              break;
                          default:
                              _dxf_ExDie("bad message type %d", pcktype);
                              /* Maybe this is too severe? */
                              /* FIX - better way to recover */
                        } /* switch */
                    } /* ISSET */
                } /* valid fd */
            } /* for k */	
        } /*  nselect > 0 */
        FD_ZERO (&fdset);
        for (k = 0; k < limit; ++k) {
            index = FETCH_LIST(_dxd_dpgraphstat, k);
            if(index->procfd >= 0 && (index->numpgrps > 0)) 
                FD_SET(index->procfd, &fdset);
        }
    } /* while not done */
    if(repeat_loop == TRUE)
        return ERROR; 

    while(_dxf_ExCheckRIH());
    if(nslaves > 0)
        ExDebug("7", "all slaves are done with graph");
    if(_dxf_NewDPTableEntry()) {
        ExDebug("*7", "Run _dxf_ExUpdateDPTable on processor 1");
        /* Since WaitOnSlaves is run on processor 0 we don't need to use
         * RunOn to call _dxf_ExUpdateDPTable
         * _dxf_ExRunOn (1, _dxf_ExUpdateDPTable, NULL, 0);
         */
         _dxf_ExUpdateDPTable();
    }
    _dxf_ExDistributeMsg(DM_GRAPHDELETE, NULL, 0, TOSLAVES);

    repeat_loop = TRUE;
    _dxf_ResetSlavesDone();
    goto wait;

    return OK; 
}

void 
_dxf_ExWaitForPeers()
{
    fd_set              fdset;
    int 		npeers, npeers_done, limit, k, nselect;
    SlavePeers  	*sp; 
    DistMsg		pcktype;
    int			maxfd;
    int			b;

    npeers = npeers_done = 0;
    maxfd = 0;
    FD_ZERO (&fdset);
    limit = SIZE_LIST(_dxd_slavepeers);
    /* don't wait for peer0 which is the master */
    for (k = 1; k < limit; ++k) {
        sp = FETCH_LIST(_dxd_slavepeers, k);
        if(sp->sfd > 0) {
            npeers++;
            FD_SET(sp->sfd, &fdset);
            if(sp->sfd > maxfd) maxfd = sp->sfd;
        }
    }
    if(npeers > 0) {
        if(_dxd_exDebug)
            printf("waiting for %d peers to close\n", npeers);
        else 
            ExDebug("7", "waiting for %d peers", npeers);
    }

    while(npeers_done < npeers) {
        nselect = select (maxfd + 1, (SELECT_ARG_TYPE *) &fdset, NULL, NULL, NULL);
        if(nselect > 0) { 
            /* find out who is sending messages */
            for (k = 1; k < limit; ++k) {
                sp = FETCH_LIST(_dxd_slavepeers, k);
                if(sp->sfd > 0) {
                    if (FD_ISSET(sp->sfd, &fdset)) {
                        if((IOCTL(sp->sfd, FIONREAD, (char *)&b) < 0) || 
                            (b <= 0)) {
                            ExDebug("7", "EOF from peer socket");
                            npeers_done++;
                            close(sp->sfd);
                            sp->sfd = -99;
                            continue;
                        }
                        if(_dxf_ExReceiveBuffer(sp->sfd, &pcktype, 1,
                                                TYPE_INT, sp->SwapMsg) < 0) 
                            DXUIMessage("ERROR", "bad peer packet type");
			if(_dxd_exDebug)
			    printf("packet type %d from peer %s fd %d\n",
				pcktype, sp->peername, sp->sfd);
			else
			    ExDebug("7", "packet type %d from peer %s fd %d",
				pcktype, sp->peername, sp->sfd);
                        switch(pcktype) {
                          default:
                              _dxf_ExDie("bad message type %d", pcktype);
                              /* Maybe this is too severe? */
                              /* FIX - better way to recover */
                        } /* switch */
                    } /* ISSET */
                } /* valid fd */
            } /* for k */	
        } /*  nselect > 0 */
        FD_ZERO (&fdset);
        for (k = 1; k < limit; ++k) {
            sp = FETCH_LIST(_dxd_slavepeers, k);
            if(sp->sfd >= 0) 
                FD_SET(sp->sfd, &fdset);
        }
    } /* while not done */
}

void 
_dxf_ExSendPVRequired(int gid, int sgid, int funcId, int varId, int requiredFlag)
{
    int pkt[5];
    pkt[0] = gid;
    pkt[1] = sgid;
    pkt[2] = funcId;
    pkt[3] = varId;
    pkt[4] = requiredFlag;

    _dxf_ExDistributeMsg(DM_PVREQUIRED, (Pointer)pkt, 5 * sizeof(int), TOPEERS);
}


int _dxf_ExValidGroupAttach(char *groupname)
{
    PGassign *pgindex;
    int j, limit;

    if(groupname == NULL)
        return(TRUE);

    for (j = 0, limit = SIZE_LIST(_dxd_pgassign); j < limit; ++j) {
        pgindex = FETCH_LIST(_dxd_pgassign, j);
        if(!strcmp(pgindex->pgname, groupname)) {
            dpgraphstat *hostentry;
            hostentry = FETCH_LIST(_dxd_dpgraphstat, pgindex->hostindex);
            if(hostentry && hostentry->error) {
                DXUIMessage("ERROR", 
                     "Group %s can not execute on host with failed connection.",
                     pgindex->pgname);
                return(FALSE);
            }
            else
                return(TRUE);
        }
    }
    /* if group isn't in table default to running on the master,
     * in which case this group has a valid connection.
     */ 
    return(TRUE);
}

/* do not send single constant assignments to remote slaves */
void
_dxf_ExSendParseTree(node *n) 
{
    node *n2 = n;

    switch(n2->type) {
        case NT_PACKET:      
            switch(n2->v.packet.type) {
                case PK_INTERRUPT:
                case PK_LINQ:
                case PK_SINQ:
                case PK_VINQ:
                case PK_IMPORT:
                case PK_MACRO:
                case PK_BACKGROUND:
                    _dxf_ExDistributeMsg(DM_PARSETREE, (Pointer)n, 0, TOSLAVES);
                    return;
                    break;

                case PK_SYSTEM:
                case PK_FOREGROUND:
                    if(n2->v.packet.packet == NULL)
                        return;
                    n2 = n2->v.packet.packet;
                    if(n2->type != NT_ASSIGNMENT) {
                        _dxf_ExDistributeMsg(DM_PARSETREE, (Pointer)n,
                                             0, TOSLAVES);
                        return;
                    }
                    /* fall through and check for assignment */
		default:
		  break;
            }
        case NT_ASSIGNMENT:      
            n2 = n2->v.assign.rval;
            if(n2->type == NT_CONSTANT || n2->type == NT_ID)
                return;
            /* no break, fall through to next case */
        default: 
            _dxf_ExDistributeMsg(DM_PARSETREE, (Pointer)n, 0, TOSLAVES);
    }
}

void _dxf_ExReqDPSend(ProgramVariable *pv, int varindex, SlavePeers *sp)
{
    DPSendPkg  *pkg;

    pkg = (DPSendPkg *)DXAllocate(sizeof(DPSendPkg));
    if(pkg == NULL)
        _dxf_ExDie("Can not allocate memory for DPSend Packet");
    pkg->index = varindex;
    pkg->excache = pv->excache;
    pkg->gvp.type = pv->gv->type;
    pkg->gvp.reccrc = pv->gv->reccrc;
    pkg->gvp.cost = pv->gv->cost;
    pkg->gvp.skip = pv->gv->skip;
    pkg->gvp.disable_cache = pv->gv->disable_cache;
    pkg->gvp.procId = pv->gv->procId;
    if(pv->gv->obj)
        pkg->gvp.hasobj = 1;
    else pkg->gvp.hasobj = 0;
        pkg->obj = pv->gv->obj;

    _dxf_ExSendQEnqueue(&(sp->sendq), pkg);
    if(!sp->wait_on_ack) {
        sp->wait_on_ack = TRUE;
        _dxf_ExDistMsgfd(DM_DPSENDREQ, &(_dxd_exSlaveId), sp->sfd);
    }
}

static Error ExCheckAcknowledge(SlavePeers *sp)
{
    int rslave_id;   

    if(_dxf_ExReceiveBuffer(sp->sfd, &rslave_id, 1, TYPE_INT, sp->SwapMsg) < 0)
    {
        DXUIMessage("ERROR", "dpsend request packet data");
        return(ERROR);
    }
    /* if we have no outstanding requests or their id is less than ours 
     * then we can send an acknowledge. If two peers have outstanding
     * requests the peer with the lowest slaveId wins.
     */
    if(!sp->wait_on_ack || rslave_id < _dxd_exSlaveId) 
        _dxf_ExDistMsgfd(DM_DPSENDACK, NULL, sp->sfd);
    else { /* remember we have an outstanding request that is not acked */ 
        sp->pending_req = TRUE;
        if(_dxd_exDebug) 
            printf("pending request from peer %s\n", sp->peername); 
        else
            ExDebug("9", "pending request from peer %s", sp->peername); 
    }
        
    return(OK);
}

static void ExOKToSend(SlavePeers *sp) 
{
    DPSendPkg *pkg;
    pkg = _dxf_ExSendQDequeue(&(sp->sendq));
    if(pkg != NULL) {
        _dxf_ExDistMsgfd(DM_DPSEND, (Pointer)pkg, sp->sfd);
        DXFree(pkg);
        sp->wait_on_ack = FALSE;
        if(sp->pending_req) {
            sp->pending_req = FALSE;
            _dxf_ExDistMsgfd(DM_DPSENDACK, NULL, sp->sfd);
        }
        if(!_dxf_ExSendQEmpty(&(sp->sendq))) {
            _dxf_ExDistMsgfd(DM_DPSENDREQ, &(_dxd_exSlaveId), sp->sfd);
            sp->wait_on_ack = TRUE;
        }
    }
    else
        DXUIMessage("ERROR", "No DPSend Package");  
}

static void ExCheckSendReq(SlavePeers *sp)
{
    if(!sp->wait_on_ack && !sp->pending_req && !_dxf_ExSendQEmpty(&(sp->sendq))) 
    {
        _dxf_ExDistMsgfd(DM_DPSENDREQ, &(_dxd_exSlaveId), sp->sfd);
        sp->wait_on_ack = TRUE;
    }
}
