/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#ifndef _LIBDX_H
#define _LIBDX_H

#include <dxconfig.h>


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined(HAVE_WINDOWS_H) && (defined(intelnt) || defined(WIN32))
#include <windows.h>
#include <winsock.h>
#define S_IXUSR S_IEXEC
#define S_IXGRP S_IEXEC
#define S_IXOTH S_IEXEC
#define EADDRINUSE      WSAEADDRINUSE
#endif

#if defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#elif defined(HAVE_SOCKET_H)
#include <socket.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#if defined(HAVE__ERRNO) && !defined(errno)
#define errno _Errno()
#endif

#include <dx/arch.h>

#include <dx/error.h>
#include <dx/timing.h>
#include <dx/memory.h>
#include <dx/object.h>
#include <dx/cache.h>
#include <dx/task.h>
#include <dx/basic.h>

#include <dx/private.h>
#include <dx/array.h>
#include <dx/arrayhandles.h>
#include <dx/string.h>
#include <dx/field.h>
#include <dx/group.h>
#include <dx/helper.h>
#include <dx/extract.h>
#include <dx/component.h>
#include <dx/import.h>

#include <dx/partition.h>
#include <dx/resampling.h>
#include <dx/grow.h>

#include <dx/render.h>
#include <dx/xform.h>
#include <dx/screen.h>
#include <dx/clipped.h>
#include <dx/camera.h>
#include <dx/light.h>
#include <dx/image.h>

#include <dx/advanced.h>
#include <dx/geometry.h>

#include <dx/seglist.h>
#include <dx/hash.h>
#include <dx/invalid.h>

#include <dx/pending.h>
#include <dx/pick.h>
#include <dx/version.h>

#include <dx/UserInteractors.h>

#if INLINE
#include <vector.c>
#include <basic.c>
#endif

#ifndef NO_STD_H
#include <stdio.h>
#include <stdlib.h>
#endif

#endif

