/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/parsemdf.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _PARSEMDF_H
#define _PARSEMDF_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
 * token and private flag constants:
 */
#define T_NONE          0x00000000
#define T_MODULE        0x00000001
#define T_CATEGORY      0x00000002
#define T_DESCRIPTION   0x00000004
#define T_INPUT         0x00000008
#define T_REPEAT        0x00000010
#define T_OUTPUT        0x00000020
#define T_OPTIONS       0x00000040
#define T_FLAGS         0x00000080
#define T_OUTBOARD      0x00000100
#define T_LOADABLE      0x00000200
#define T_ERROR         0x00008000

#define PF_RELOAD       0x00000000   /* redefinition of an existing module */
#define PF_LOADABLE     0x00000001   /* run-time dynamicly loaded module */
#define PF_OUTBOARD     0x00000002   /* outboard module definition */

#define F_NONE          0

#define REPCOUNT        20   /* should match constant in mdf2c file */

/* this now works for both outboards and loadable modules.
 */

struct modargs {
    char *              thisarg;      /* name of this input/output */
    char *              deflt;        /* default value (not used yet) */
    struct modargs *    nextarg;      /* linked list pointer */
};

struct moddef {
    char *              m_name;       /* module name */
    PFI                 m_func;       /* entry point - function address */
    int                 m_flags;      /* module flags */
    int                 m_nin;        /* number of inputs */
    struct modargs *    m_innames;    /* list of inputs - modargs structs */
    int                 m_nout;       /* number of outputs */
    struct modargs *    m_outnames;   /* list of outputs - modargs structs */
    char *              m_exec;       /* for outboards, filename to run */
    char *              m_host;       /* for outboards, host to run on */
    int                 m_pflags;     /* private flags for parsemdf use */
    char *              m_loadfile;   /* filename for loadable modules */
};

Error DXLoadMDFFile(char *filename);
Error DXLoadMDFString(char *cp);
void _dxf_ExSendMdfPkg(Pointer *data, int tofd);
Error _dxf_ExRecvMdfPkg(int fromfd, int swap);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _PARSEMDF_H */
