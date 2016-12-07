/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/packet.h,v 1.6 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _PACKET_H
#define _PACKET_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define PACK_INTERRUPT	1
#define PACK_SYSTEM	2
#define PACK_ACK	3
#define PACK_MACRODEF	4
#define PACK_FOREGROUND	5
#define PACK_BACKGROUND	6
#define PACK_ERROR	7
#define PACK_MESSAGE	8
#define PACK_INFO	9
#define PACK_LINQUIRY	10
#define PACK_LRESPONSE	11
#define PACK_LDATA	12
#define PACK_SINQUIRY	13
#define PACK_SRESPONSE	14
#define PACK_SDATA	15
#define PACK_VINQUIRY	16
#define PACK_VRESPONSE	17
#define PACK_VDATA	18
#define PACK_COMPLETE	19
#define PACK_IMPORT	20
#define PACK_IMPORTINFO	21
#define PACK_LINK	22

void  _dxf_ExCheckPacket(char *packet, int length);
Error _dxf_ExSPack (int type, int seqnumber, Pointer data, int len);
int   _dxf_ExNeedsWriting(void);
Error _dxf_ExInitPacket(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _PACKET_H */
