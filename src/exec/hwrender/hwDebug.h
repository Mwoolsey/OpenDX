/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	hwDebug_h
#define	hwDebug_h
/*---------------------------------------------------------------------------*\
  This file contains declarations and defines for hardware debugging
\*---------------------------------------------------------------------------*/


#if defined(DEBUG)

#define ENTRY_CHAR "U"
#define EXIT_CHAR "U"
#define BODY_CHAR "V"
#define RESERVED_CHAR "W"
#define MESSAGE_CHAR "X"

#define DXHW_MESSAGE_NONE      0
#define DXHW_MESSAGE_VERBOSE   1
#define DXHW_MESSAGE_FROM_EXEC 2
#define DXHW_MESSAGE_FROM_UI   4

/*
 * Abstract this call so we can change the meaning of debugging trace
 * easily
 */
#define PRINT_FUNC printf

/*
 * This allows us to make function calls when debugging and have
 * them disappear when optimized.
 */
#define DEBUG_CALL(a) a

/*
 * Enable debugging if the environment variable is set.
 */
#define ENABLE_DEBUG() \
  { \
    static int debug_enabled=FALSE; \
    if (!debug_enabled && getenv("DXHW_DEBUG")) {\
      DXEnableDebug(getenv("DXHW_DEBUG"),1); \
      debug_enabled = TRUE; \
    } \
  }      

#define DEBUG_OFF() \
  { \
    if (getenv("DXHW_DEBUG")) {\
      DXEnableDebug((char *)getenv("DXHW_DEBUG"),0); \
      PRINT_FUNC("/************ trace off ***********/\n"); \
    } \
  }      

#define DEBUG_ON() \
  { \
    if (getenv("DXHW_DEBUG")) {\
      DXEnableDebug((char *)getenv("DXHW_DEBUG"),1); \
      PRINT_FUNC("/************ trace on ***********/\n"); \
    } \
  }      

/*
 * Debug macros.  Use these the same as using printf.  ENTRY and EXIT
 * insert special formating characters for use with indent (makes
 * trace look like 'c' code. 
 */
#define PRINT(a)                                            \
  if (DXQueryDebug(BODY_CHAR))                              \
  {                                                         \
    PRINT_FUNC a;                                           \
    PRINT_FUNC (";\n");                                     \
  }
/*
#define FROMSERVER(a)                                       \
   if(DXQueryDebug(MESSAGE_CHAR))                           \
   {                                                        \
      PRINT_FUNC ("<<<=== Message( " a " " ) from server"); \
   }

#define TOSERVER(a)                                         \
   if(DXQueryDebug(MESSAGE_CHAR))                           \
   {                                                        \
      PRINT_FUNC ("===>>> Message( " a " ) to Server");     \
   }
*/
#define ENTRY(a)                                            \
  if (DXQueryDebug(ENTRY_CHAR)) {                           \
    PRINT_FUNC ("{ ");                                      \
    PRINT_FUNC a;                                           \
    PRINT_FUNC (";\n");                                     \
  }

#define EXIT(a)                                             \
  if (DXQueryDebug(EXIT_CHAR)) {                            \
    PRINT_FUNC a;                                           \
    PRINT_FUNC (";");                                       \
    PRINT_FUNC ("}\n");                                     \
  }

/*
 * Puts a marker to separate major sections of the trace (the marker
 * looks like a 'c' comment).
 */
#define DEBUG_MARKER(text) \
  if (DXQueryDebug(ENTRY_CHAR)) { \
    int i; \
    PRINT_FUNC("/*"); \
    for (i=0;i<20;i++) PRINT_FUNC("="); \
    PRINT_FUNC(" %s ",text); \
    for (i=0;i<20;i++) PRINT_FUNC("="); \
    PRINT_FUNC("*/\n\n"); \
    fflush(stdout); \
  }

/*
 * undef's are only temporary until all files are brought forward
 * to use new debugging macros.
 */
#undef MPRINT
#define MPRINT(M) \
  PRINT(( "float MM[4][4] = {\n" \
	  "  { %12.6f, %12.6f, %12.6f, %12.6f },\n" \
	  "  { %12.6f, %12.6f, %12.6f, %12.6f },\n" \
	  "  { %12.6f, %12.6f, %12.6f, %12.6f },\n" \
	  "  { %12.6f, %12.6f, %12.6f, %12.6f },\n" \
	  "}", \
          M[0][0], M[0][1], M[0][2], M[0][3], \
          M[1][0], M[1][1], M[1][2], M[1][3], \
          M[2][0], M[2][1], M[2][2], M[2][3], \
          M[3][0], M[3][1], M[3][2], M[3][3]));

#undef VPRINT
#define VPRINT(V) \
  PRINT(( "float VV[] = { %12.6f, %12.6f, %12.6f }", V[0], V[1], V[2]));

#undef SPRINT
#define SPRINT(S) \
  PRINT(( "float SS[] = { %12.6f, %12.6f, %12.6f }", S.x, S.y, S.z));

#undef CPRINT
#define CPRINT(C) \
  PRINT(( "float CC[] = { %12.6f, %12.6f, %12.6f }", C.r, C.g, C.b));

/*
 * Used to turn on and off sections of code at run-time.
 */
#define RT_IFDEF(x) \
  if (getenv(#x)) {

#define RT_IFNDEF(x) \
  if (!getenv(#x)) {

#define RT_ELSE	\
  } else {

#define RT_ENDIF \
  }

#else /* if defined(DEBUG) */

#define DXHW_MESSAGE_NONE      0
#define DXHW_MESSAGE_VERBOSE   1
#define DXHW_MESSAGE_FROM_EXEC 2
#define DXHW_MESSAGE_FROM_UI   4

#define DEBUG_CALL(a)
#define ENABLE_DEBUG()
#define DEBUG_OFF()
#define DEBUG_ON()
#define ENTRY(a)
#define EXIT(a)
#define FROMSERVER(a)
#define TOSERVER(a)
#define PRINT(a)
#define CLIENT_MESSAGE(a)
#define DEBUG_MARKER(text)
#undef MPRINT
#define MPRINT(M)
#undef VPRINT
#define VPRINT(V)
#undef SPRINT
#define SPRINT(S)
#undef CPRINT
#define CPRINT(C)
#define RT_IFDEF(x)  if(0) {
#define RT_IFNDEF(x) if(1) {
#define RT_ELSE	     } else {
#define RT_ENDIF     }

#endif /* if defined(DEBUG) */

#if !defined(TIMER)
#ifdef DX_NO_TIMER
#define TIMER(s)
#else
#define TIMER(s) DXMarkTime(s);
#endif			      
#endif

#endif
