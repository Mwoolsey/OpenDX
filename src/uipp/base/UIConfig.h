/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

/*
 * $Header
 */

/*
// UI Configuration file for various platforms.
*/
#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
// ANSI sprint() returns an int, the number of characters placed in 
// output string.  Some other sprintf()'s return the input string.
*/
#ifdef sun4
# define NON_ANSI_SPRINTF
#endif

/*
// If this system doesn't implement strerror
*/
#ifdef sun4
# define NEEDS_STRERROR
#endif

/*
// If this system hasn't implemented strrstr 
*/
#ifndef hp700
# define NEEDS_STRRSTR
#endif


/*
// What type does malloc return
*/
#ifdef sun4
# define MALLOC_RETURNS malloc_t
#else
# define MALLOC_RETURNS void*
#endif

/*
// If int abs(int) is defined in math.h, and this causes problems vis a vis
// that defined in the ansi specified stdlib.h, define this.
*/
#ifdef hp700
# define ABS_IN_MATH_H
#endif

/*
// if there is no gethostname declaration, define this.
// ibm6000 because of AIX4.1 header files problems. Remove this when fixed.
*/
#if defined(alphax) || defined(aviion) || defined(solaris) || defined(ibm6000) || defined(sun4) || defined(os2)
#endif
/*
// define CFRONT_3_0_INLINE if you cannot have functional code after a
// return statement in an inline function.  Most DX headers don't do this
// anymore.
*/
#ifndef ibm6000
# define CFRONT_3_0_INLINE
#endif

/*
// Define this if the system has an herror function to print errors of 
// gethostbyname, etc.
*/
#if !defined(sun4) && !defined(hp700) && !defined(solaris) && !defined(alphax)
# define HAS_HERROR
#endif

/*
// Define EXECVE_2ND_TYPE to be the type expected for the second param
// and EXECVE_3RD_TYPE to be the type expected for the third parameter of
// the system call execve().
*/
#if defined(sgi)
# define EXECVE_2ND_TYPE char **
#elif defined(hp700) || defined(aviion) || defined(ibm6000) || defined(solaris) || defined(alphax) || defined(linux) || defined(freebsd)
# define EXECVE_2ND_TYPE char * const*
#else
# define EXECVE_2ND_TYPE const char **
#endif

#if defined(sun4) || defined(sgi)
# define EXECVE_3RD_TYPE char **
#elif defined(hp700) || defined(aviion) || defined(ibm6000) || defined(solaris) || defined(alphax) || defined(linux) || defined(freebsd)
# define EXECVE_3RD_TYPE char * const*
#else
# define EXECVE_3RD_TYPE const char **
#endif

/*
//
// Define whether or not the UI should use the license manager
//
*/
/*
//
// Define this if the architecture supports the IBM 7246 ClipNotify
// Extension library.
//
*/
#if defined(ibm6000) && !defined(_AIX41)
# define HAS_CLIPNOTIFY_EXTENSION
#endif

/*
//
// Define this to be 1 if your system has a working rename() library call
// to rename files. 
//
*/
#ifdef hp700		/* The hp rename() has missing data symbol "Error" */
# define HAS_RENAME	0
#else
# define HAS_RENAME	1
#endif

/*
//
// Define SIGNAL_RETURN_TYPE to be the type returned by the signal() routine.
//
*/
#if defined(sun4) || defined(solaris)
# define SIGNAL_RETURN_TYPE SIG_PF
#elif defined(sgi)
  typedef void (*DX_SIG_VAL)(int, ...);
# define SIGNAL_RETURN_TYPE DX_SIG_VAL
#else
  typedef void (*DX_SIG_VAL)(int);
# define SIGNAL_RETURN_TYPE DX_SIG_VAL
#endif

/*
// Define NO_CC_TEMPLATES if the C++ compiler cannot handle 3.0 templates.
*/
#if defined(intelnt) || defined(aviion) || defined(solaris) || defined(WIN32)
#define NO_CC_TEMPLATES
#endif

/*
// Define DUMMY_FOR_LIST_WIDGET if the list widgets need dummy code to resize.
*/
#if defined(aviion) || defined(hp700) || defined(sun4) || defined(sgi) || defined(alphax)
#define DUMMY_FOR_LIST_WIDGET
#endif

/*
 * Enables development kit stuff that is not in the mainline product (yet?).
 * For example, VPE->File->Save as C Code option.
 * The hp and aviion can't handle aggregate initialization of arrays, and 
 * since this is only used for debuggable (unreleased) code, we can just 
 * not support this feature on the HP and DG.
 */
#if defined(DEBUG) && !defined(hp700) && !defined(aviion) && !defined(intelnt) && !defined(OS2) && !defined(WIN32)
# define DXUI_DEVKIT
#endif

/*
 * Define this if your R5 system does not have <X11/Xmu/Editres.h> and
 * the library, libXmu.a that goes with it.
 */
#if defined(hp700)
# define NO_EDITRES 
#endif

/*
 * Turn on vpe pages.  If you really want to redefine this, then you have back
 * out changes to Makefile and ProcessGroupManager.{C,h}  Makefile rev 8.15 was
 * the first Makefile in the main branch with page code.  ProcessGroupManager.{C,h}
 * rev 8.1 for both files were the first versions in the main branch with page
 * code.  Prior to this these files lived in tree 8.0.99.
 */
#define WORKSPACE_PAGES 1

/*
//
//
//
*/
#ifdef ibm6000
# define DXD_ARCHNAME "ibm6000"
# define DXD_HAS_CRYPT 1
#endif
#ifdef solaris
# define DXD_ARCHNAME "solaris"
# define DXD_HAS_CRYPT 1
#endif
#ifdef sun4
# define DXD_ARCHNAME "sun4"
# define DXD_HAS_CRYPT 1
#endif
#ifdef sgi
# define DXD_HAS_CRYPT 1
# define DXD_ARCHNAME "sgi"
#endif
#ifdef hp700
# define DXD_HAS_CRYPT 1
# define DXD_ARCHNAME "hp700"
#endif
#ifdef aviion 
# define DXD_HAS_CRYPT 1
# define DXD_ARCHNAME "aviion"
#endif
#ifdef os2
# define DXD_DO_NOT_REQ_SYS_PARAM_H
# define DXD_DO_NOT_REQ_UNISTD_H
# define DXD_ARCHNAME "os2"
# define DXD_OS2
# define DXD_SYSLIB_8_CHARS
# define DXD_NEEDS_TYPES_H
# define DXD_XTOFFSET_HOSED
# define DXD_IBM_OS2_SOCKETS
# define DXD_NON_UNIX_SOCKETS
#if 0
# define DXD_NON_UNIX_DIR_SEPARATOR
#endif
# define DXD_NON_UNIX_ENV_SEPARATOR
# define DXD_CR_IS_CRLF
# define DXD_LACKS_UTS
# define DXD_LACKS_FORK
# define DXD_LACKS_POPEN
# define DXD_OS_NON_UNIX
# define DXD_NEEDS_MKTEMP
# define DXD_MATH_LACKS_PI
# define DXD_NEEDS_CTYPE_H
# define DXD_NEEDS_PROCESS_H
# define DXD_HAS_CRYPT 0
#ifndef NO_MKTEMP
# define NO_MKTEMP
#endif
#endif

#ifdef alphax
# define DXD_HAS_CRYPT 1
# define DXD_ARCHNAME "alphax"
#endif





#if defined(intelnt) || defined(WIN32)
# define DXD_ARCHNAME "intelnt"
# define DXD_SYSLIB_8_CHARS
# define DXD_NEEDS_TYPES_H
# define DXD_XTOFFSET_HOSED
# define DXD_DO_NOT_REQ_SYS_SELECT_H
# define DXD_NON_UNIX_SOCKETS
# define DXD_CR_IS_CRLF
# define DXD_LACKS_UTS 
# define DXD_LACKS_FORK
# define DXD_NEEDS_PROCESS_H
# define DXD_NEEDS_CTYPE_H
# define DXD_HAS_CRYPT 0
#ifndef NO_MKTEMP
# define NO_MKTEMP
#endif

# define DXD_DO_NOT_REQ_SYS_PARAM_H
# define DXD_DO_NOT_REQ_UNISTD_H

#endif 


#endif 	/* __CONFIG_H__ */


