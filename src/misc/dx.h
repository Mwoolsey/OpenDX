/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>
#include <dx/arch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_PROCESS_H)
#include <process.h>
#endif

#if defined(HAVE__SPAWNVP) && !defined(HAVE_SPAWNVP)
#define spawnvp _spawnvp
#define HAVE_SPAWNVP 1
#endif

#define EXE_EXT ".exe"
#define DIRSEP "\\"
#define PATHSEP ";"

#define SCRIPTVERSION DXD_VERSION_STRING

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define SMALLSTR 50
#define MAXARGS 200
#define MAXNAME MAX_PATH
#define MAXENV  24576
#define MAXPARMS 200

/* String Types ----------------------------*/
typedef char smallstr[SMALLSTR];
typedef char envstr[MAXENV];
typedef char namestr[MAXNAME];
typedef char valuestr[MAXNAME];

/* Enum types ----------------------------*/
enum regCo {
  OPENDX_ID = 1,
  HUMMBIRD_ID = 2,
  HUMMBIRD_ID2 = 3,
  STARNET_ID = 4,
  LABF_ID = 5
};

enum regGet {
  GET = 1,
  CHECK = 2
};

enum xServer { UNKNOWN, EXCEED6, EXCEED7, XWIN32, WINAXE };


/* Macros ---------------------------- */
#define IfError(s)		\
    if (rc != ERROR_SUCCESS) {	\
	strcpy(errstr, s);	\
	goto error;		\
    }

#define IfError2(s, t, u) {			\
    if (rc != ERROR_SUCCESS) {			\
	sprintf(errstr, "%s %s %s", s, t, u);	\
	goto error;				\
    }						\
}

#define ErrorGoto(s) {		\
	strcpy(errstr, s);	\
	goto error;		\
    }

#define ErrorGoto2(s, t, u) {			\
	sprintf(errstr, "%s %s %s", s, t, u);	\
	goto error;				\
    }

#define KILLSEMI(s) {							\
	int kk;								\
	while ((kk = strlen(s)) && ((s[kk-1] == ';') || (s[kk-1] == ' ')))	\
	    s[kk-1] = '\0';						\
    }

#define setenvpair(s, v)	\
    if (s && *s && v && *v)	\
	if(!putenvstr(s, v, echo)) {	\
    printf("\nCannot set env var: %s\n", s); 	\
}
