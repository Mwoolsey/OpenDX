/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <stdarg.h>
#include "dxlP.h"
#include "dxl.h"

enum DXLExecuteCtl {
        DXLEXECUTEONCE,
        DXLEXECUTEONCHANGE,
        DXLENDEXECUTION,
        DXLENDEXECUTEONCHANGE
};
typedef enum DXLExecuteCtl DXLExecuteCtlEnum;

#define BGbeginMSG "BG:  begin"
#define BGendMSG   "BG:  end"
#define FGbeginMSG "begin "
#define FGendMSG    NULL 
#define INTRstopMSG "stop"

static void _dxl_EndBG(DXLConnection *c, const char *msg, void *data)
{
    c->isExecuting --;
}

static void _dxl_BeginBG(DXLConnection *c, const char *msg, void *data)
{
    c->isExecuting ++;
}

static void _dxl_CompleteExecute(DXLConnection *c, const char *msg, void *data)
{
    int packetId = (int)(long)data;
    c->isExecuting--;
    _dxl_RemoveSystemHandler(c,PACK_COMPLETE,packetId,
			FGendMSG, _dxl_CompleteExecute);
}

static void _dxl_INTRstop(DXLConnection *c, const char *msg, void *data)
{
    c->isExecuting = 0;
    _dxl_RemoveSystemHandler(c,PACK_INTERRUPT,-1,INTRstopMSG, _dxl_INTRstop);
}
static DXLError _dxl_ManageExecuteHandlers(DXLConnection *conn, int install,
				int foreground, int packetId)
{
    if (conn->dxuiConnected)
	return ERROR;

    if (foreground) {
	if (install) {
	    if (!_dxl_SetSystemHandler(conn, PACK_COMPLETE,packetId, FGendMSG,
			(DXLMessageHandler)_dxl_CompleteExecute, (void*)packetId))
		return ERROR;
	} else {
	    /*
	     * FIXME: we should really use the correct packetId here 
	     */
	    if (!_dxl_RemoveSystemHandler(conn,PACK_COMPLETE,-1, FGendMSG,
					_dxl_CompleteExecute))
		return ERROR;
	}
	
    } else {
	if (install) {
	    if (!_dxl_SetSystemHandlerString(conn, PACK_INTERRUPT,INTRstopMSG,
			(DXLMessageHandler)_dxl_INTRstop, (void*)0))
		return ERROR;
	} else {
		;	/* Don't bother removing them.
			 * Re-installs don't cause memory to be used up
			 * and it doesn't hurt to leave them installed.
			 */ 
	}
	
    }
    return OK;

}

static DXLError
DXLExecuteCtl(DXLConnection *conn, DXLExecuteCtlEnum ectl, char *name, char **args)
{
   int i, sts = ERROR;
   const char *cmd = NULL;
   char namestr[1024];

   if (name)
       sprintf(namestr, "%s(\n", name);
   else
       sprintf(namestr, "main(\n");
    
   if (args)
   {
       for (i = 0; args[i]; i++)
       {
	   strcat(namestr, args[i]);
	   if (args[i+1])
	       strcat(namestr, ",");
       }
   }

   strcat(namestr, ");\n");
	    
   switch (ectl) {
	case DXLEXECUTEONCE: 	
	    if (conn->dxuiConnected) {
		cmd = "execute once"; 
	    } else {
		int packetId;
		/*
		if (conn->debugMessaging && conn->isExecuting)
		    _DXLError(conn, "already executing");
		*/
		/* FIXME: handle other program names */
		packetId = DXLSendPacket(conn,PACK_FOREGROUND,namestr);
		if (packetId >= 0) 
		    sts = _dxl_ManageExecuteHandlers(conn,1,1,packetId);
		conn->isExecuting++;
	    }
	    break;
	case DXLEXECUTEONCHANGE: 	
	    if (conn->dxuiConnected) {
		cmd = "execute onchange"; 
	    } else {
		int packetId;
		packetId = DXLSendPacket(conn,PACK_BACKGROUND,namestr);
		sts = (packetId >= 0);
		if (sts)
		{
		    _dxl_SetSystemHandler(conn, PACK_INFO, -1, BGbeginMSG,
			    (DXLMessageHandler)_dxl_BeginBG, (void*) NULL);
		    _dxl_SetSystemHandler(conn, PACK_INFO, -1, BGendMSG,
			    (DXLMessageHandler)_dxl_EndBG, (void*) NULL);
		    if (sts)
			DXLExecuteCtl(conn, DXLEXECUTEONCE,name,args);
		    if (sts)
			sts = _dxl_ManageExecuteHandlers(conn,1,0,-1);
		}
	    }
	    break;
	case DXLENDEXECUTION: 	
	    if (conn->dxuiConnected) {
		cmd = "execute end"; 
	    } else {
		sts = DXLSendPacket(conn,PACK_INTERRUPT,NULL) >= 0;    
		if (sts)
		    sts = DXLSendImmediate(conn,"sync") >= 0;    
		_dxl_RemoveSystemHandler(conn, PACK_INFO, -1, FGbeginMSG, _dxl_BeginBG);
		_dxl_RemoveSystemHandler(conn, PACK_INFO, -1, FGendMSG, _dxl_EndBG);
	    }
	    break;
	case DXLENDEXECUTEONCHANGE: 	
	    if (conn->dxuiConnected) {
		cmd = "execute endEOC"; 
	    } else {
		_dxl_RemoveSystemHandler(conn, PACK_INFO, -1, FGbeginMSG, _dxl_BeginBG);
		_dxl_RemoveSystemHandler(conn, PACK_INFO, -1, FGendMSG, _dxl_EndBG);
		sts = DXLSendPacket(conn,PACK_INTERRUPT,NULL) >= 0;    
		if (sts)
		    sts = DXLSendImmediate(conn,"sync") >= 0;    
	    }
	    break;
    }

   if (cmd)
	return DXLSend(conn, cmd); 
   else
	return sts;
}

DXLError
DXLExecuteOnce(DXLConnection *conn)
{
    return DXLExecuteCtl(conn, DXLEXECUTEONCE,NULL, NULL);
}

DXLError
DXLExecuteOnChange(DXLConnection *conn)
{
    return DXLExecuteCtl(conn, DXLEXECUTEONCHANGE,NULL, NULL);
}

DXLError
exDXLExecuteOnceNamed(DXLConnection *conn, char *name)
{
    return DXLExecuteCtl(conn, DXLEXECUTEONCE, name, NULL);
}

DXLError
exDXLExecuteOnChangeNamed(DXLConnection *conn, char *name)
{
    return DXLExecuteCtl(conn, DXLEXECUTEONCHANGE, name, NULL);
}

DXLError
DXLEndExecution(DXLConnection *conn)
{
    return DXLExecuteCtl(conn, DXLENDEXECUTION, NULL, NULL);
}

DXLError
DXLEndExecuteOnChange(DXLConnection *conn)
{
    return DXLExecuteCtl(conn, DXLENDEXECUTEONCHANGE, NULL, NULL);
}

DXLError
exDXLExecuteOnceNamedWithArgsV(DXLConnection *conn, char *name, char **args)
{
    return DXLExecuteCtl(conn, DXLEXECUTEONCE, name, args);
}

DXLError
exDXLExecuteOnceNamedWithArgs(DXLConnection *conn, char *name, ...)
{
    int i;
    char *args[100];
    va_list arg;

    va_start(arg, name);
    for (i = 0; i < 100; i++)
	if ((args[i] = va_arg(arg, char *)) == NULL)
	    break;
    
    return exDXLExecuteOnceNamedWithArgsV(conn, name, args);
}

DXLError
exDXLExecuteOnChangeNamedWithArgsV(DXLConnection *conn, char *name, char **args)
{
    return DXLExecuteCtl(conn, DXLEXECUTEONCHANGE, name, args);
}

DXLError
exDXLExecuteOnChangeNamedWithArgs(DXLConnection *conn, char *name, ...)
{
    int i;
    char *args[100];
    va_list arg;

    va_start(arg, name);
    for (i = 0; i < 100; i++)
	if ((args[i] = va_arg(arg, char *)) == NULL)
	    break;
    
    return exDXLExecuteOnChangeNamedWithArgsV(conn, name, args);
}

DXLError
DXLGetExecutionStatus(DXLConnection *conn, int *execStatus)
{
    if (conn->dxuiConnected) {
        char buf[100];
        int sts = DXLQuery(conn, "query execution", sizeof(buf), buf);
        if (! sts)
            return ERROR;
        if (1 != sscanf(buf, "execution state: %d", execStatus))
            return ERROR;
    } else {
        /* FIXME: make sure the begin/end execution handlers are installed */
        *execStatus = (conn->isExecuting != 0);
    }
    return OK;
}

