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
/* #include "dx.h" Not until we actually handle the object */
#include "dict.h"

extern Object _dxfImportBin_FP(int fd); /* from exec/libdx/rwobject.c */

typedef struct handler_data{
    DXLObjectHandler 	handler;
    const void		*data;
} HandlerData;

static HandlerData *NewHandlerData(DXLObjectHandler    
					handler, const void *data)
{
    HandlerData *hd = (HandlerData*)MALLOC(sizeof(HandlerData));
    if (!hd)
	return NULL;

    hd->handler = handler;
    hd->data = data;
 
    return hd; 
}

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
 *  "DXLOutput OBJECT <name> <size>"
 */
static char *link_object_token = "LINK:  DXLOutput OBJECT"; 	/* V2.x */
static char *object_token = "DXLOutput OBJECT"; 		/* V3.x + */

static void 
SystemObjectHandler(DXLConnection *conn, const char *msg, void *data)
{
    HandlerData *hd=NULL;
    char format[1024];
    char varname[1024];
    int object_size;

    /* Parse varname from msg which contains varname" */
    sprintf(format,"%s %%s %%d",object_token);
    if (sscanf(msg,format,varname,&object_size) == 2) {
	hd = DictFind(conn->objectHandlerDict, varname);
    } else {
	sprintf(format,"%s %%s %%d",link_object_token);
	if (sscanf(msg,format,varname,&object_size) == 2) 
	    hd = DictFind(conn->objectHandlerDict, varname);
    }

    if (hd) {
	/* Extract object from the message 
	 */
	 int fd = DXLGetSocket(conn);
	 Object o = (Object)_dxfImportBin_FP(fd);
	(hd->handler)(conn,varname,o,(void*)hd->data);
    }

}

static DXLError InitializeObjectHandler(DXLConnection *conn)
{
    conn->objectHandlerDict = NewDictionary();
    if (!conn->objectHandlerDict) 
	return ERROR;
	

    if (!_dxl_SetSystemHandlerString(conn, PACK_INFO, link_object_token, 
					SystemObjectHandler, NULL) ||
        !_dxl_SetSystemHandlerString(conn, PACK_LINK, object_token, 
					SystemObjectHandler, NULL))
	return ERROR;

    return OK;
}
/*
 * Install a handler for the object that has been given the name 'name'.
 * One handler is allowed per named object.
 * Return 0 on error, non-zero on success.
 */
DXLError
DXLSetObjectHandler(DXLConnection *conn, const char *varname,
			DXLObjectHandler handler, const void *data)
{
    HandlerData *hd;

    if (!conn->objectHandlerDict && !InitializeObjectHandler(conn)) 
	return ERROR;

    hd = NewHandlerData(handler,data); 
    if (!hd)
	return ERROR;

    if (!DictInsert(conn->objectHandlerDict,varname,hd, 
				DictDeleteHandlerData)) {
	DeleteHandlerData(hd);
 	return ERROR;
    }

    return OK;

}
/*
 * Remove the handler for the object that has been given the name 'name'.
 * Return 0 on error, non-zero on success.
 */
int
DXLRemoveObjectHandler(DXLConnection *conn, const char *name)
{
    HandlerData *hd;

    if (!conn->objectHandlerDict) 
	return 1;

    if ((hd = (HandlerData*)DictFind(conn->objectHandlerDict,name))) {
	DeleteHandlerData(hd);
 	return 1;
    }

    return 0;
}


