/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/license.h,v 1.5 2000/08/11 15:28:12 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _LICENSE_H
#define _LICENSE_H

#define AUTH_MSG_LEN 1024 

#define GOT_CONCURRENT 0xafe3
#define GOT_NODELOCKED 0xb673
#define GOT_NONE       0x0000

#define KEY1 "a9"
#define KEY2 "Pp"
#define ANYWHERE_HOSTID "00000000"

#define HOST_LEN  301
#define KEY_LEN    27


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef enum
{
        NOLIC    = 0,      /* Used as a flag by _dxd_exForcedLicense */
        DXLIC    = 0xde1d, /* Standard DX (development) license */
        MPLIC    = 0xfe45, /* DX SMP license */
        RTLIC    = 0x4398, /* DX Run-time (no VPE) license */
        RTLIBLIC = 0x24a3  /* DX Run-time libraries (libDXcallm.a) license */
} lictype;

Error ExGetLicense(lictype ltype, int forceNetLS);
Error ExGetPrimaryLicense();
void _dxf_ExReleaseLicense();
int _dxfCheckLicenseChild(int child);

#endif /* _LICENSE_H */
