/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/getset.c,v 1.7 2000/08/24 20:04:32 davidt Exp $
 */

#include <string.h>
#include <dx/dx.h>
#include "../dpexec/graph.h"

#define LOCAL 			1
#define GLOBAL 			0
#define DFLT_GLOBAL             2

#define SET_OBJECT		in[0]
#define SET_LINK		in[1]
#if 0
#define SET_TRIGGER_ALWAYS	in[2]
#endif
#if 1
#define SET_KEY			in[2]
#endif

extern int DXLoopFirst(); /* from dpexec/evalgraph.c */

static Error Set(Object *in, Object *out, int flag);

Error m_Set(Object *in, Object *out)
{
    if(DXLoopFirst())
        DXWarning("#5260", "Set", "Set", "Set");
    return(Set(in, out, DFLT_GLOBAL));
}

Error m_SetGlobal(Object *in, Object *out)
{
    return(Set(in, out, GLOBAL));
}

Error m_SetLocal(Object *in, Object *out)
{
    return(Set(in, out, LOCAL));
}

static Error Set(Object *in, Object *out, int flag)
{
    char   *key = NULL, *get_modid = NULL;
    char   *set_modid = NULL;
#if 0
    int    triggerAlways = 0;
#endif
    int    trigger;
    Object old = NULL;

    if (SET_LINK)
    {
#if 0
	if (SET_TRIGGER_ALWAYS)
	{
	    if (! DXExtractInteger(SET_TRIGGER_ALWAYS, &triggerAlways) ||
		triggerAlways < 0 || triggerAlways > 1)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#10070", "trigger");
		goto error;
	    }
	}
#endif

	if (DXGetObjectClass((Object)SET_LINK) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "link");
	    goto error;
	}

	get_modid = DXGetString((String)SET_LINK);
        if(DXLoopFirst()) {
	    const char *last_comp = DXGetModuleComponentName(get_modid, -1);

            if(flag == LOCAL) {
                if(strcmp(last_comp, "GetLocal") != 0) {
                    DXSetError(ERROR_BAD_PARAMETER,"#10760", "SetLocal", "GetLocal");
                    goto error;
                }

                set_modid = DXGetModuleId();

                if(DXCompareModuleMacroBase( set_modid, get_modid ) != OK) {
                    DXSetError(ERROR_DATA_INVALID, "#10762");
                    DXFreeModuleId((Pointer)set_modid);
                    goto error;
                }
                else 
                    DXFreeModuleId((Pointer)set_modid);
            }
            else { /* global */
                if(strcmp(last_comp, "GetGlobal") != 0 &&
                   strcmp(last_comp, "Get") != 0) {
                    if(flag == GLOBAL) 
                        DXSetError(ERROR_BAD_PARAMETER,
                                   "#10760", "SetGlobal", "GetGlobal");
                    else 
                        DXSetError(ERROR_BAD_PARAMETER, "#10760", 
                         "Set is now an alias for SetGlobal; SetGlobal", "GetGlobal");
                    goto error;
                }
            }
        }
    }

#if 1
    if (SET_KEY)
    {
	if (DXGetObjectClass((Object)SET_KEY) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "key");
	    goto error;
	}

	key = DXGetString((String)SET_KEY);
    }
    else
#endif
	key = get_modid;
    
    if (! key)
    {
#if 0
	DXSetError(ERROR_BAD_PARAMETER, "#10764", "either link or key");
#else
	DXSetError(ERROR_BAD_PARAMETER, "#10764", "link");
#endif
	goto error;
    }

    old = DXGetCacheEntry(key, 0, 0);

    trigger = !old || (DXGetObjectTag(old) != DXGetObjectTag(SET_OBJECT));

    if (trigger)
    {
	if (! DXSetCacheEntry(SET_OBJECT, CACHE_PERMANENT, key, 0, 0))
	    goto error;
    
#if 0
	if (triggerAlways)
	    DXReadyToRun(get_modid);
	else
#endif
	    DXReadyToRunNoExecute(key);
    }

    DXDelete(old);
    return OK;

error:
    out[0] = NULL;
    DXDelete(old);
    return ERROR;
}

#define GET_OBJECT		in[0]
#define GET_RESET		in[1]
#if 1
#define GET_KEY			in[2]
#endif

static Error Get(Object *in, Object *out, int flag);

Error m_GetLocal(Object *in, Object *out)
{
    return(Get(in, out, LOCAL));
}

Error m_GetGlobal(Object *in, Object *out)
{
    return(Get(in, out, GLOBAL));
}

Error m_Get(Object *in, Object *out)
{
    if(DXLoopFirst())
        DXWarning("#5260", "Get", "Get", "Get");
    return(Get(in, out, GLOBAL));
}

static Error Get(Object *in, Object *out, int flag)
{
    char *key = NULL, *modid = NULL;
    int  reset = 0;

    key = modid = (char *)DXGetModuleId();
    if (! modid)
	goto error;
    
    if(flag == LOCAL && DXLoopFirst())
        reset = 1;
    else if (GET_RESET)
    {
	if (! DXExtractInteger(GET_RESET, &reset) ||
	    reset < 0 || reset > 1)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "reset");
	    goto error;
	}
    }

#if 1
    if (GET_KEY)
    {
	key = DXGetString((String)GET_KEY);
    }
#endif

    if (reset)
    {
	out[0] = GET_OBJECT;
	DXSetCacheEntry(NULL, CACHE_PERMANENT, key, 0, 0);
    }
    else
    {
	out[0] = DXGetCacheEntry(key, 0, 0);
	if (out[0])
	    DXDelete(out[0]);
	else
	    out[0] = GET_OBJECT;
    }
    
    out[1] = (Object)DXNewString(modid);
    
    DXFreeModuleId((Pointer)modid);

    return OK;

error:
    DXFreeModuleId((Pointer)modid);
    out[0] = NULL;
    out[1] = NULL;
    return ERROR;
}
