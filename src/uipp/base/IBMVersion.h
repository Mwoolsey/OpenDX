/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _IBMVersion_h
#define _IBMVersion_h


/*
 * Create a single number from the 3 version numbers that can be used to
 * do integer compares to determine relative versions. 
 *
 */
#define VERSION_NUMBER(maj,min,mic)     (((maj) << 16) + ((min) << 8) + (mic))


/*
 * Define this while code is in development, but undef it before a release.
 */
#if 0 
#define BETA_VERSION 
#endif

/* The DX release version number. */
#define IBM_MAJOR_VERSION	DXD_VERSION
#define IBM_MINOR_VERSION	DXD_RELEASE
#define IBM_MICRO_VERSION	DXD_MODIFICATION


#endif /* _IBMVersion_h */
