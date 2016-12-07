/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/lex.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _LEX_H
#define _LEX_H

#include "parse.h"
#include "sfile.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define LINEFEED 10

#define YYLMAX_1        4000            /* max string/identifer length */
#define YYLMAX          (YYLMAX_1 + 1)

typedef struct _cmdBuf
{
    char *buf;
    struct _cmdBuf *next;
} CmdBuf;

extern SFILE       *yyin; /* from lex.c */

Error _dxf_ExInitLex();
void  _dxf_ExFlushNewLine();
void  _dxf_ExLexInit();
void  _dxf_ExUIPacketActive(int id, int t, int n);
void  _dxf_ExLexError();
void  _dxf_ExUIFlushPacket();
int   ExCheckParseBuffer();
int   yylex();
void  yygrabdata(char *buffer, int len);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _LEX_H */
