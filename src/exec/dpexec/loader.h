/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/loader.h,v 1.2 2000/08/11 15:28:12 davidt Exp $
 */

#ifndef _LOADER_H
#define _LOADER_H

#include "utils.h"

Error DXLoadAndRunObjFile (char *fname, char *envvar);
Error DXUnloadObjFile (char *fname, char *envvar);
PFI   DXLoadObjFile (char *fname, char *envvar);
Error _dxf_fileSearch(char *inname, char **outname, char *extension,
                      char *environment);

#endif /* _LOADER_H */
