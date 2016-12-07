/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>
#include <stdio.h>
#ifdef DXD_LICENSED_VERSION
#include <license.h>
lictype _dxd_exForcedLicense    = NOLIC; /* Is the given license forced */
#endif
#include "../dpexec/_macro.h"
#include "../dpexec/userinter.h"

int _dxd_exMyPID                  = 0;      /* pid of the process */
int _dxd_exDebug                  = 0;
int _dxd_exRemoteSlave            = 0;
char *_dxd_exParseMesg            = 0;
int _dxd_exParseError             = 0;
int _dxd_ExHasLicense             = 0;
int *_dxd_exTerminating           = 0;
int _dxd_exRemote                 = 0;
char *_dxd_exHostName             = 0;

extern int _dxd_exOutboard;
extern void _dxf_user_modules(); /* from auto generated dxmods/user.c */

void DXInitModules()
{
    DXProcessors(1);
    DXinitdx();
    _dxf_ExMacroInit();
    _dxf_user_modules();
    DXSetErrorExit(1);
#ifdef DXD_LICENSED_VERSION
    if(!_dxd_exOutboard)
        ExGetLicense(RTLIBLIC, FALSE);
#endif
    _dxfLoadDefaultUserInteractors();
}

void _dxf_ExDistributeMsg(int type, Pointer data, int size, int to)
{
}

void _dxf_ExDie (char *format, ...)
{
    char buffer[1024];
    va_list arg;

    /* don't add a space before format in the next line or it won't
     * compile with the metaware compiler.
     */
    va_start(arg,format);
    vsprintf(buffer, format, arg);
    va_end(arg);

    if(_dxd_exRemoteSlave)
        DXUIMessage("ERROR", buffer);
    else {
        fputs  (buffer, stdout);
        fputs  ("\n",   stdout);
        fflush (stdout);
    }

    exit (-1);
}

Error
DXSetPendingCmd(char *major, char *minor, int (*job)(Private), Private data)
{
    return OK;
}

int _dxf_ExIsExecuting()
{
    return 0;
}

void
ExQuit() {
	exit(1);
}
