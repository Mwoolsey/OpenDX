/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <string.h>
#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

#include "dxlP.h"
#include "dict.h"

typedef struct handler_data{
    DXLValueHandler 	handler;
    const void		*data;
} HandlerData;

#if 0
static HandlerData *NewHandlerData(DXLValueHandler
                                        handler, const void *data)
{
    HandlerData *hd = (HandlerData*)MALLOC(sizeof(HandlerData));
    if (!hd)
        return NULL;

    hd->handler = handler;
    hd->data = data;

    return hd;
}
#endif

static void DeleteHandlerData(HandlerData *hd)
{
    if (hd)
        FREE(hd);
}
static void DictDeleteHandlerData(void *data)
{
    DeleteHandlerData((HandlerData*)data);
}


/*
 * The message comming across is in the format
 *  "DXLOutput VALUE <name> <value>"
 * FIXME: what about values that are longer than a single message buffer.
 */
static char *link_value_token = "LINK:  DXLOutput VALUE"; 	


static void 
SystemValueHandler(DXLConnection *conn, const char *msg, void *data)
{
    HandlerData *hd = NULL;
    char format[1024];
    char varname[1024];
    int count; 
    
    /* Parse varname from msg which contains varname" */
    sprintf(format,"%s %%s %%n",link_value_token);
    if (sscanf(msg,format,varname,&count) == 1) 
	hd = (HandlerData*)DictFind(conn->valueHandlerDict, varname);

    if (hd) {
	/* Extract value from the message 
	 */
	char value[8192];
	strcpy(value, msg + count);
	(hd->handler)(conn,varname,(const char *)value,(void*)hd->data);
    }

}

static DXLError InitializeValueHandler(DXLConnection *conn)
{
    DXLPacketTypeEnum ptype; 
 
    conn->valueHandlerDict = NewDictionary();
    if (!conn->valueHandlerDict)
	return ERROR;

    if (conn->majorVersion > 2)
	ptype = PACK_LINK;
    else
	ptype = PACK_INFO;

    if (!_dxl_SetSystemHandlerString(conn, ptype, link_value_token, 
					SystemValueHandler, NULL))  
	return ERROR;

    return OK;
}
/*
 * Install a handler for the value that has been given the name 'name'.
 * One handler is allowed per named value.
 * Return 0 on error, non-zero on success.
 */
DXLError
DXLSetValueHandler(DXLConnection *conn, const char *varname,
			DXLValueHandler handler, const void *data)
{
    HandlerData *hd;

    if (!conn->valueHandlerDict && !InitializeValueHandler(conn)) 
	return ERROR;

    hd = (HandlerData*)MALLOC(sizeof(HandlerData));
    if (!hd)
	return ERROR;

    hd->handler = handler;
    hd->data = data;
    if (!DictInsert(conn->valueHandlerDict,varname,hd, DictDeleteHandlerData)) {
	FREE(hd);
 	return ERROR;
    }

    return OK;

}
/*
 * Remove the handler for the value  that has been given the name 'name'.
 * Return 0 on error, non-zero on success.
 */
DXLError
DXLRemoveValueHandler(DXLConnection *conn, const char *name)
{
    if (!conn->valueHandlerDict) 
	return OK;

    if (DictFind(conn->valueHandlerDict,name)) {
	DictDelete(conn->valueHandlerDict,name);
 	return OK;
    }

    return ERROR;
}


DXLError
DXLSetValue(DXLConnection *conn, const char *var, const char *value)
{
    int sts;
    if (conn->dxuiConnected) {
	char *buffer = MALLOC(strlen(var)+ strlen(value) +
				strlen("set receiver %s = %s"));
	if (conn->majorVersion <= 2)
	    sprintf(buffer, "set receiver %s = %s",var, value);
	else
	    sprintf(buffer, "set value %s = %s",var, value);
	sts = DXLSend(conn, buffer);
	FREE(buffer);
    } else {
	char *name = MALLOC(strlen(var) + 3);
	sprintf(name, "\"%s\"", var);
	exDXLExecuteOnceNamedWithArgs(conn, "SetDXLInputNamed", name, value, 0);
	FREE(name);
	sts = OK;
    }

    return sts;
}

#if 0
static DXLError
DXLSetTabValue(DXLConnection *conn, const char *var, const char *value)
{
    int sts;
    char *buffer = MALLOC(strlen(var)+ strlen(value) + 16);

    if (conn->dxuiConnected) {
	if (conn->majorVersion <= 2)
	    sprintf(buffer, "set value %s = %s",var, value);
	else
	    sprintf(buffer, "set tab %s = %s",var, value);
    } else {
	sprintf(buffer, "%s = %s;",var, value);
    }
    sts = DXLSend(conn, buffer);
    FREE(buffer);

    return sts;
}
#endif 

DXLError
DXLSetInteger(DXLConnection *conn, const char *gvarname, const int value)
{
    char buf[128];
    sprintf(buf,"%d",value);
    return DXLSetValue(conn,gvarname,buf);
}

DXLError
DXLSetScalar(DXLConnection *conn, const char *gvarname, const double value)
{
    char buf[128];
    sprintf(buf,"%.*g",8,value);
    return DXLSetValue(conn,gvarname,buf);
}
DXLError
DXLSetString(DXLConnection *conn, const char *gvarname, const char *value)
{
    int r, l = strlen(value) + 3; 
    char *buffer = MALLOC(l);

    sprintf(buffer, "\"%s\"",value);
    r = DXLSetValue(conn,gvarname,buffer);
    FREE(buffer);
    return r;
}

DXLError
DXLSetInputValue(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const char *value)
{
    int l = strlen(macro) + strlen(module);
    char *varname = MALLOC(l + 32);
    int sts;

    sprintf(varname, "%s_%s_%d_in_%d", macro, module, instance, number);
    sts = DXLSetValue(conn, varname, value);
    FREE(varname);
    return sts;
}

DXLError
DXLSetInputInteger(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const int value)
{
    char buf[128];
    sprintf(buf,"%d",value);
    return DXLSetInputValue(conn,macro,module,instance,number,buf);
}

DXLError
DXLSetInputScalar(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const double value)
{
    char buf[128];
    sprintf(buf,"%f",value);
    return DXLSetInputValue(conn,macro,module,instance,number,buf);
}
DXLError
DXLSetInputString(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const char *value)
{
    int r, l = strlen(value) + 3; 
    char *buffer = MALLOC(l);

    sprintf(buffer, "\"%s\"",value);
    r = DXLSetInputValue(conn,macro,module,instance,number,buffer);
    FREE(buffer);
    return r;
}

DXLError
DXLSetOutputValue(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const char *value)
{
    int l = strlen(macro) + strlen(module);
    char *varname = MALLOC(l + 32);
    int sts;

    sprintf(varname, "%s_%s_%d_out_%d", macro, module, instance, number);
    sts = DXLSetValue(conn, varname, value);
    FREE(varname);
    return sts;
}

DXLError
DXLSetOutputInteger(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const int value)
{
    char buf[128];
    sprintf(buf,"%d",value);
    return DXLSetOutputValue(conn,macro,module,instance,number,buf);
}

DXLError
DXLSetOutputScalar(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const double value)
{
    char buf[128];
    sprintf(buf,"%.*g",8,value);
    return DXLSetOutputValue(conn,macro,module,instance,number,buf);
}
DXLError
DXLSetOutputString(DXLConnection *conn,
    const char *macro, const char *module, const int instance, const int number,
    const char *value)
{
    int r, l = strlen(value) + 3; 
    char *buffer = MALLOC(l);

    sprintf(buffer, "\"%s\"",value);
    r = DXLSetOutputValue(conn,macro,module,instance,number,buffer);
    FREE(buffer);
    return r;
}
