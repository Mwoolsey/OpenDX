/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "utils.h"
#include "userinter.h"
#include "loader.h"

void *_dxd_UserInteractors = NULL;
int _dxd_nUserInteractors = 0;

extern int DXDefaultUserInteractors(int *n, void *t); /* from dxmods/definter.c */

Error
_dxfLoadUserInteractors(char *fname)
{
    char *path = (char *)getenv("DXUSERINTERACTORS");
    int (*func)() = DXLoadObjFile(fname, path);
    if (! func)
	DXWarning("unable to open user interactor file %s", fname);
    else
	(*func)(&_dxd_nUserInteractors, &_dxd_UserInteractors);

    return OK;
}

Error
_dxfLoadDefaultUserInteractors()
{
    char *fname;

    if ((fname = (char *)getenv("DX_USER_INTERACTOR_FILE")) != NULL)
	return _dxfLoadUserInteractors(fname);
    else
	return DXDefaultUserInteractors(&_dxd_nUserInteractors, &_dxd_UserInteractors);

    return OK;
}

