/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _DXVersion_h
#define _DXVersion_h

#include "../base/IBMVersion.h"

/* The DX release version number. */
/* FIXME: If IBM_*_VERSION != DX_*_VERSION then 
 * DXApplication::getVersionNumbers(), must be implemented to return
 * the right version numbers (i.e. DX* not IBM*).
 */
#define DX_MAJOR_VERSION	IBM_MAJOR_VERSION	
#define DX_MINOR_VERSION	IBM_MINOR_VERSION	
#define DX_MICRO_VERSION	IBM_MICRO_VERSION	


/* The version number for the ui/exec communication stream. */
#define PIF_MAJOR_VERSION	2
#define PIF_MINOR_VERSION	0
#define PIF_MICRO_VERSION	0

/* The version number for the .net and .cfg files. */
/*
 * In general the user gets a message (see Network.c) if:
 *    1) The UI is parsing a .net or .cfg file that has a later
 *			version number then its own internal version number
 *			(i.e. NETFILE_*_VERSION).
 *
 * Rules for changing the net file version number:
 *
 *   Major - Change this number between releases if
 *		1) An older version of the UI (by at least the major
 *			version number) will not be able to parse
 *			the change. 
 * 
 *   Minor - Change this number between releases if 
 *		1) if you want the user to be notified when the ui is reading
 *		 	a .net (not .cfg) that is older by at least a minor 
 *			version number. 
 *			 
 *   Micro - Change this number between releases if 
 *		1) anything changes in a .net or .cfg file and the major 
 *			and minor numbers were not changed. In general, 
 *			the user does not get notified about differences in 
 *			micro version numbers (other than the standard 
 *			'module definition changed' message).
 *
 *
 *
 *
 * 2.0.0 adds lots
 * 2.0.1 adds another parameter to the Image macro.
 * 2.0.2 actually nothing has changed (as of 4/5/94), but this number
 *      was bumped in the PVS 2.0.2 release when we thought we were just
 *      going to use the DX version numer.
 * 2.0.3 version that goes out with dx 2.1.1 and has the .cfg 'version'
 *      comment in front of the 'time' comment.
 * 3.0.0 adds 'size = %dx%d' to .cfg instance comments and changed Colormap
 *	parameters (requires major number change).  
 * 3.0.1 uses include macro comments in the top of .net files.  The version
 *	of DX which writes 3.0.1 nets cannot create new Get/Set modules.
 *	Assumption: the include macro comments won't cause problems for 
 *	earlier versions of DX.
 * 3.1.0 adds vpe decorators, several new modules, a new param in ScalarNode.
 * 	also adds pages and annotation group comments.
 * 3.1.1 adds a transmitter/receiver bug fix in which the net was not written out
 *	in sorted order.  The bumped micro number allows TransmitterNode to scan
 * 	the network conditionally.
 * 	...adds multi line capability to LabelDecorators in {print,parse}Comment
 * 3.1.2 (DX3.1.5) Streakline added a param.  Users will _not_ be notified
 * 3.2.0 Adds OPTIONS line in the mdf section in macros' .net files
 */
#define NETFILE_MAJOR_VERSION   3 
#define NETFILE_MINOR_VERSION   2 
#define NETFILE_MICRO_VERSION   0 



#endif /* _DXVersion_h */
