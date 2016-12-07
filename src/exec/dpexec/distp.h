/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/distp.h,v 1.8 2005/12/30 19:38:43 davidt Exp $
 */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DISTP_H
#define _DISTP_H

#include <dxconfig.h>

#include "exobject.h"
#include "graph.h"
#include "d.h"

#define DPMSG_VERSION    0x0d
#define DPMSG_SIGNATURE  0x100b0d13
#define DPMSG_SIGNATUREI 0x130d0b10

typedef enum 
{
    DM_NULL,		/*  0 */
    DM_PARSETREE,	/*  1 */
    DM_EVICTCACHE,	/*  2 */
    DM_EXECGRAPH,	/*  3 */
    DM_INSERTGDICT,	/*  4 */
    DM_INSERTGDICTNB,	/*  5 */
    DM_INSERTMDICT,	/*  6 */
    DM_SLISTEN,		/*  7 */
    DM_SCONNECT,	/*  8 */
    DM_SCONNECTPORT,	/*  9 */
    DM_SACCEPT,		/* 10 */
    DM_SLAVEID,         /* 11 */
    DM_GRAPHDONE, 	/* 12 */
    DM_GRAPHDELETE,     /* 13 */
    DM_INSERTCACHE,	/* 14 */
    DM_UIMSG,		/* 15 */
    DM_UIPCK,		/* 16 */
    DM_DPSEND,          /* 17 */
    DM_KILLEXECGRAPH,   /* 18 */
    DM_FLUSHGLOBAL,     /* 19 */
    DM_PVREQUIRED,      /* 20 */ 
    DM_CONNECTERROR,    /* 21 */
    DM_EVICTCACHELIST,  /* 22 */
    DM_FLUSHDICT,       /* 23 */
    DM_FLUSHCACHE,      /* 24 */
    DM_FLUSHMACRO,      /* 25 */
    DM_DPSENDREQ,       /* 26 */
    DM_DPSENDACK,       /* 27 */
    DM_PERSISTDELETE,   /* 28 */
    DM_LOADMDF,         /* 29 */
    DM_VERSION,         /* 30 */
    DM_BADFUNC,         /* 31 */
    DM_VCRREDO,         /* 32 */
    DM_LOOPDONE,        /* 33 */
    DM_DELETEPEER,      /* 34 */
    DM_EXIT,		/* 35 */
    DM_GRAPHDELETECONF  /* 36 */
} DistMsg; 

typedef enum 
{
    DICT_NONE,		/*  0 */
    DICT_GLOBAL,	/*  1 */
    DICT_MACRO,		/*  2 */
    DICT_GRAPH		/*  3 */
} DictType; 

#define TOMASTER 0
#define TOSLAVES 1
#define TOPEERS  2
#define TOPEER0  3

typedef struct dphosts
{
    char 	*name;
    int         islocal;
} dphosts;

typedef struct dpslave_id
{
    int 	id;
    int         namelen;
    char 	*name;

} dpslave_id;

typedef struct dpversion
{
    int 	signature;
    int         version;
} dpversion;

typedef struct dpgraphstat
{
    char        *prochostname;
    char	*procusername;
    char        *options;
    int		procfd;      /* for os2 this must always be a *socket* */
    int         SlaveId;
    int         SwapMsg;
    int         numpgrps;
    int         error;
} dpgraphstat;

/* this is similar to gvar in graph.h. Any changes made to gvar may need
 * to be added here too.
 */
typedef struct gvarpkg 
{
    _gvtype             type;
    uint32              reccrc;         /* cache tag            */
    double              cost;
    int                 skip;           /* error or route shutoff */
    int                 disable_cache;  /* disable output cacheing */
    int                 procId;
    int			hasobj;
} gvarpkg;

typedef struct DPSendPkg
{
    int                 index;
    int			excache;
    gvarpkg		gvp;
    Object		obj;
} DPSendPkg;

typedef struct dps_elem
{
    struct dps_elem     *next;          /* linked list of elements */
    DPSendPkg           *pkg;           /* request package for send object */
} dps_elem;


typedef struct DPSendQ
{ 
    dps_elem *head;
    dps_elem *tail;
} DPSendQ;

typedef struct SlavePeers 
{
    char 	*peername;
    int         sfd;
    int         SlaveId;
    int         SwapMsg;
    DPSendQ     sendq;
    int         wait_on_ack;
    int         pending_req;
} SlavePeers;

typedef struct PGassign  
{
    char 	*pgname;
    int	        hostindex;
} PGassign;

typedef struct GDictSend
{
    char		*varname;
    exo_class   	exclass; 
    gvarpkg   	 	gvp;
    Object		obj; 
} GDictSend;

/* for deleting a persistent outboard from a remote exec */
typedef struct DelRemote {
    int 		del_namelen;
    char 		*del_name;
    int 		del_instance;
} DelRemote;

#define MAX_UI_PACKET  4096

/* This number has to be smaller than MAX_UI_PACKET, there needs to be extra */
/* space in the UI packet for adding a header to the message for the UI      */
/* This value should be the same as MAX_MSGLEN in  dxmods/interact.h         */

#define MSG_BUFLEN 4000

/* these are for DXMessage, DXSetError, DXWarning */
typedef struct UIMsgPackage
{
    int 		ptype;
    int 		graphId;
    int			len;
    char 		data[MSG_BUFLEN];
} UIMsgPackage;

/* these are for direct calls to _dxf_ExSPack */
typedef struct UIPackage
{
    int			len;
    char		data[MAX_UI_PACKET];
} UIPackage;

#define N_CACHETAGLIST_ITEMS 10
typedef struct CacheTagList
{
    int numtags;
    uint32 ct[N_CACHETAGLIST_ITEMS];
} CacheTagList;

/* from distpacket.c */
int   _dxf_ExValidGroupAttach(char *groupname);

/* from distconnect.c */
void  _dxf_ExUpdateDPTable();
Error _dxf_ExSlaveListen();
Error _dxf_ExSlaveConnect();
void  _dxf_ResetSlavesDone();

/* from distp.c */
int   _dxf_SuspendPeers();
int   _dxf_ResumePeers();
void  _dxf_ExDistributeMsg(DistMsg type, Pointer data, int size, int to);
void  _dxf_ExDistMsgfd(DistMsg type, Pointer data, int tofd);
Error _dxf_ExWaitOnSlaves();
void  _dxf_ExWaitForPeers();
void  _dxf_ExSendDict(int fd, EXDictionary dict);
void  _dxf_ExRecvGDictPkg(int fd, int swap, int nobkgrnd);
void  _dxf_ExReqDPSend(ProgramVariable *pv, int varindex, SlavePeers *sp);
Error _dxf_ExReceivePeerPacket(SlavePeers *sp);
void  _dxf_ExSendPVRequired(int gid, int sgid, int funcId, int varId, int requiredFlag);
int   _dxf_ExReceiveBuffer(int fd, Pointer buffer, int n, Type t,int swap);
void  _dxf_ExSendParseTree(node *n);
Error _dxf_ExSendQInit(DPSendQ *sendq);
Error _dxf_ExCreateGDictPkg(GDictSend *pkg, char *name, EXObj ex_obj);
int   _dxf_ExWriteSock(int fd, Pointer buffer, int size);
int   _dxf_ExGetSocketId(char *procname);
void  _dxf_SlaveDone();

/* from distqueue.c */
void _dxf_ExSendQEnqueue (DPSendQ *sendq, DPSendPkg *pkg);
DPSendPkg *_dxf_ExSendQDequeue (DPSendQ *sendq);
int _dxf_ExSendQEmpty(DPSendQ *sendq);

#endif /* _DISTP_H */
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
