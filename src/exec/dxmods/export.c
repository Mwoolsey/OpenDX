/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>
#include <string.h>
#include "vrml.h"
#include "exp_gai.h"

extern Object _dxfExportBin(Object o, char *dataset); /* from libdx/rwobject.c */
extern Error _dxfExportDX(Object o, char *filename, char *format); /* from libdx/edfprint.c */

/* formats we accept */
#define DT_UNKNOWN  0
#define	DT_DX       3
#define	DT_BIN      4 
#define	DT_NOTIMPL  5
#define DT_ARRAY    6
#define DT_VRML	    7

/* 0 = fieldname
 * 1 = filename
 * 2 = format string
 */
int
m_Export(Object *in, Object *out)
{
    char *filename, *format = NULL, *cp;
    int dt = DT_UNKNOWN;
    int req_dt = DT_UNKNOWN;
    

    /* if no object, return */
    if(!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    /* filename.  require : in name to be binary file (on array disk)
     */
    if(!DXExtractString(in[1], &filename)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "filename");
	return ERROR;
    }
#ifndef DXD_OS_NON_UNIX
    if (strchr(filename, ':'))
        req_dt = DT_BIN;
#endif
    
    /* check format string. 
     */
    if(in[2]) {
	if(!DXExtractString(in[2], &format)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "format");
	    return ERROR;
	}

	if(!strncmp(format, "dx", 2))
	    dt = DT_DX;
	else if(!strcmp(format, "bin"))
	    dt = DT_BIN;
	else if(!strncmp(format, "array",5) || !strncmp(format,"spreadsheet",11))
	    dt = DT_ARRAY;
	else if (!strcmp(format, "vrml"))
	    dt = DT_VRML;
	else
	    dt = DT_NOTIMPL;
    }

    if (dt == DT_UNKNOWN) {
#ifndef DXD_OS_NON_UNIX
	if (strchr(filename, ':'))
	    dt = DT_BIN;
	else 
#endif
	{
            if ((cp = strrchr(filename, '.')) != NULL) {
		if (!strcmp(cp, ".bin"))
		    dt = DT_BIN;
		else if (!strcmp(cp, ".dx"))
		    dt = DT_DX;
		else if (!strcmp(cp, ".wrl"))
		    dt = DT_VRML;
	    }
	}
        if (dt == DT_UNKNOWN)
	    dt = DT_DX;
    }

    if (dt == DT_NOTIMPL) {
	DXSetError(ERROR_BAD_PARAMETER, "#12238", format);
	return ERROR;
    }
    
#if DXD_HAS_LIBIOP
    if (req_dt == DT_BIN  &&  dt != DT_BIN) {
        DXSetError(ERROR_BAD_PARAMETER, "colon in name indicates array disk partition, and array disk currently only supports 'bin' format");
        return ERROR;
    }
#else
    if (req_dt == DT_BIN) {
        DXSetError(ERROR_BAD_PARAMETER, "colon in name indicates array disk partition, and array disk not supported on this architecture");
        return ERROR;
    }
#endif
    
    switch(dt) {
      case DT_DX:
	if(!_dxfExportDX(in[0], filename, format))
	    return ERROR;
	
	break;

      case DT_BIN:
	if(!_dxfExportBin(in[0], filename))
	    return ERROR;
	
	break;
      case DT_ARRAY:
	 if(!_dxfExportArray(in[0],filename,format))
	    return ERROR;
	 break;
      case DT_VRML:
	 if (!_dxfExportVRML(in[0],filename))
	    return ERROR;
	 break;
      default:
	DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unrecognized file format");

    }

    return OK;


}

