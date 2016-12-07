/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _enums_h
#define _enums_h

//
// The following are passed into the xxxPrintNode routines, specifying
// the type of printing to be done.  These are used by both Network
// and Node.
//
     enum PrintType {
                PrintExec = 0,     // Destination: exec
                PrintFile,         // Destination: file
                PrintCut,          // Destination: cut/paste
                PrintCPBuffer      // Destination: cut/paste
     };

//
// For ImageWindow and ImageNode 
//
enum DirectInteractionMode
{
    NONE,
    CAMERA,
    CURSORS,
    PICK,
    NAVIGATE,
    PANZOOM,
    ROAM,
    ROTATE,
    ZOOM
};

#endif
