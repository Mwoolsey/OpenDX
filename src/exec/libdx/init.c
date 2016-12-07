/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>
#include "internals.h"
#include "diskio.h"

Error user_init();

static int been_here = 0;

Error
DXinitdx()
{
    if (!been_here) {
	been_here = 1;

	if (!_dxf_initlocks())  return ERROR;
	if (!_dxf_initmemory()) return ERROR;
	if (!_dxf_initmessages()) return ERROR;
	if (!_dxf_inittiming()) return ERROR;
	if (!_dxf_initobjects()) return ERROR;
	if (!_dxf_initstringtable()) return ERROR;
	if (!_dxf_initdisk()) return ERROR;
	if (!_dxf_initcache()) return ERROR;
	if (!_dxf_initRIH()) return ERROR;
	if (!_dxf_initloader()) return ERROR;
#if DXD_HAS_LIBIOP
	if (!_dxf_initfb()) return ERROR;
#endif
	if (!user_init()) return ERROR;
    }
    return OK;
}

