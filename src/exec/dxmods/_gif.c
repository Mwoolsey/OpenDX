/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdlib.h>
#include <fcntl.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_rw_image.h"

char * _dxf_BuildGIFReadFileName 
    ( char  *buf,
      int   bufl,
      char  *basename,  /* Does not have an extension */
      char  *fullname,  /* As specified */
      int   framenum,
      int   *numeric,   /* Numbers postfixed onto name or not? */
      int   *selection  /* file name extension ? (enumeral) */ )
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "gif format has been removed for legal reasons");
    return NULL;
}

SizeData * _dxf_ReadImageSizesGIF
               ( char     *name,
                 int      startframe,
                 SizeData *data,
                 int      *use_numerics,
                 int      ext_sel,
                 int      *multiples )
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "gif format has been removed for legal reasons");
    return NULL;
}

Field _dxf_InputGIF
	(int width, int height, char *name, int relframe, int delayed, char *colortype)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "gif format has been removed for legal reasons");
    return NULL;
}

Error
_dxf_write_gif(RWImageArgs *iargs)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "gif format has been removed for legal reasons");
    return ERROR;
}
