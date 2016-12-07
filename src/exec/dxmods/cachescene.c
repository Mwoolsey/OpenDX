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

Error
m_CacheScene(Object *in, Object *out)
{
    char *tag;
    char *buf = NULL;

    if (! in[0])
    {
	DXSetError(ERROR_MISSING_DATA, "cache tag");
	goto error;
    }

    if (! in[1])
    {
	DXSetError(ERROR_MISSING_DATA, "cache object");
	goto error;
    }

    if (! in[2])
    {
	DXSetError(ERROR_MISSING_DATA, "cache camera");
	goto error;
    }

    if (! DXExtractString(in[0], &tag))
    {
	DXSetError(ERROR_MISSING_DATA, "cache tag must be STRING");
	goto error;
    }

    buf = (char *)DXAllocate(strlen(tag) + strlen(".object") + 1);
    if (! buf)
	goto error;
    
    sprintf(buf, "%s.object", tag);
    if (! DXSetCacheEntry(in[1], CACHE_PERMANENT, buf, 0, 0))
	goto error;
    
#if 0
    /* I believe this is left over from when we were using Get with */
    /* a key. */
    DXReadyToRunNoExecute(tag);
#endif

    DXFree((Pointer)buf);

    buf = (char *)DXAllocate(strlen(tag) + strlen(".camera") + 1);
    if (! buf)
	goto error;
    
    sprintf(buf, "%s.camera", tag);
    if (! DXSetCacheEntry(in[2], CACHE_PERMANENT, buf, 0, 0))
	goto error;

#if 0
    /* I believe this is left over from when we were using Get with */
    /* a key. */
    DXReadyToRunNoExecute(tag);
#endif

    DXFree((Pointer)buf);

    return OK;

error:
    DXFree((Pointer)buf);
    return ERROR;
}
