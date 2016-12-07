/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#include <string.h>

#include "dxlP.h"

DXLError
DXLSync(DXLConnection *conn)
{
    DXLError r;

    if (conn->syncing)
        return OK;

    conn->syncing = 1;
    if (conn->dxuiConnected) {
        r = DXLQuery(conn, "query sync", 0, NULL) >= 0 ? OK : ERROR;
    } else {
	DXLEvent event;
	r = ERROR;
	/* FIXME: we really need to fix DXLQuery() for the exec. */
        r = DXLSendImmediate(conn,"sync"); 
	if (r) {
	    int packetId = DXLSendPacket(conn, PACK_FOREGROUND,
					"Executive(\"nop\");\n");
            r = (packetId >= 0) && DXLGetPacketId(conn, PACK_COMPLETE,
                                                        packetId,  &event);
	    if (r)
		DXLClearEvent(&event);
	}
    }

    conn->syncing = 0;
    return r;

}
DXLError
uiDXLSyncExecutive(DXLConnection *conn)
{
    DXLError r;

    if (conn->dxuiConnected) {
        r = DXLQuery(conn,"query sync-exec", 0,NULL) >= 0 ? OK : ERROR; 
    } else {
        r = DXLSync(conn); 
    }

    return r;

}

DXLError
DXLGetValue(DXLConnection *conn, const char *varname, 
		const int valueLen, char *value)
{
    int l = strlen(varname);
    char *buffer = MALLOC(l + 16);
    int sts;

    if (!conn->dxuiConnected) {
	FREE(buffer);
	return ERROR;
    }

    sprintf(buffer, "query value %s",varname) ;
    sts = DXLQuery(conn, buffer, valueLen, value);
    FREE(buffer);
    return sts >= 0 ? OK : ERROR;
}


DXLError
DXLGetInputValue(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const int valueLen, char *value)
{
    int l = strlen(macro) + strlen(module);
    char *varname = MALLOC(l + 64);
    int sts;

    sprintf(varname, "%s_%s_%d_in_%d", macro, module, instance, number);
    sts = DXLGetValue(conn, varname, valueLen, value);
    FREE(varname);
    return sts;
}


DXLError
DXLGetInputInteger(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    int *value)
{
    char rbuf[128];
    int sts;

    sts = DXLGetInputValue(conn, macro, module, instance,number, 
					sizeof(rbuf), rbuf);
    if (sts == OK && sscanf(rbuf, "%d", value) != 1)
	sts = ERROR;
    return sts;
}

DXLError
DXLGetInputScalar(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    double *value)
{
    char rbuf[128];
    int sts;

    sts = DXLGetInputValue(conn, macro, module, instance,number, 
					sizeof(rbuf), rbuf);
    if (sts == OK && sscanf(rbuf, "%lg", value) != 1)
	sts = ERROR;
    return sts;
}
DXLError
DXLGetInputString(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const int valueLen, char *value)
{
    char *rbuf = MALLOC(valueLen + 32);
    int sts;

    sts = DXLGetInputValue(conn, macro, module, instance,number, 
					sizeof(rbuf), rbuf);
    if (sts == OK && sscanf(rbuf, "\"%s\"", value) != 1)
	sts = ERROR;
    FREE(rbuf);
    return sts;
}


DXLError
DXLGetOutputValue(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const int valueLen, char *value)
{
    int l = strlen(macro) + strlen(module);
    char *varname = MALLOC(l + 50);
    int sts;

    sprintf(varname, "%s_%s_%d_out_%d", macro, module, instance, number);
    sts = DXLGetValue(conn, varname, valueLen, value);
    FREE(varname);
    return sts;
}

DXLError
DXLGetOutputInteger(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    int *value)
{
    char rbuf[128];
    int sts;

    sts = DXLGetOutputValue(conn, macro, module, instance,number, 
					sizeof(rbuf), rbuf);
    if (sts == OK && sscanf(rbuf, "%d", value) != 1)
	sts = ERROR;
    return sts;
}

DXLError
DXLGetOutputScalar(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    double *value)
{
    char rbuf[128];
    int sts;

    sts = DXLGetOutputValue(conn, macro, module, instance,number, 
					sizeof(rbuf), rbuf);
    if (sts == OK && sscanf(rbuf, "%lg", value) != 1)
	sts = ERROR;
    return sts;
}
DXLError
DXLGetOutputString(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const int valueLen, char *value)
{
    char *rbuf = MALLOC(valueLen + 32);
    int sts;

    sts = DXLGetOutputValue(conn, macro, module, instance,number, 
					sizeof(rbuf), rbuf);
    if (sts == OK && sscanf(rbuf, "\"%s\"", value) != 1)
	sts = ERROR;
    FREE(rbuf);
    return sts;
}
