/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int DXLoopFirst(); /* from dpexec/evalgraph.c */
extern void DXLoopDone(int); /* from dpexec/evalgraph.c */
extern void DXSaveForId(char *); /* from dpexec/evalgraph.c */

struct NextNState {
    int start;
    int end;
    int delta;
    int direction;
    int next;
};
 
struct NextGroupState {
    int next;
    int end;
};
 
struct NextArrayState {
    int next;
    ArrayHandle ahandle;
    Object in_obj;
    Type type;
    Category category;
    int items, rank, shape[50], isize;
    Pointer buf;
};

Error NextArrState_delete(Pointer in)
{
    struct NextArrayState *astate=NULL;
    if(in)
        astate = (struct NextArrayState *)in;
        if(astate->buf)
            DXFree(astate->buf);
        if(astate->ahandle)
            DXFreeArrayHandle(astate->ahandle);
        if(astate->in_obj) 
            DXDelete(astate->in_obj);
        DXFree(in);
    return OK;
}

Error NextState_delete(Pointer in)
{
    if(in)
        DXFree(in);
    return OK;
}

Error
m_First(Object *in, Object *out)
{
    int first;
    out[0] = NULL;

    first = DXLoopFirst();

    out[0] = (Object)DXMakeInteger(first);
    return OK;
}

Error
m_Done(Object *in, Object *out)
{
    int done;

    if(!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "Done value must be set");
	return ERROR;
    }

    if(!DXExtractInteger(in[0], &done)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10010", "Done value");
	return ERROR;
    }
    DXLoopDone(done);
    return OK;
}

Error
m_ForEachN(Object *in, Object *out)
{
    struct NextNState *state = NULL;
    Object obj;
    char *modid;
    int first;

    out[0] = NULL;
    out[1] = NULL;
    
    modid = DXGetModuleId();

    first = DXLoopFirst();
    if(first) {
        if(!in[0]) {
            DXSetError(ERROR_MISSING_DATA,"start must be specified");
	    goto error_return;
        }
        if(!in[1]) {
            DXSetError(ERROR_MISSING_DATA,"end must be specified");
	    goto error_return;
        }

        state = DXAllocateZero(sizeof(struct NextNState));
        if(state == NULL)
            goto error_return;
        if(!DXExtractInteger(in[0], &state->start)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10010", "start value");
	    goto error_return;
        }

        if(!DXExtractInteger(in[1], &state->end)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10010", "end value");
	    goto error_return;
        }

        if(!in[2]) 
            state->delta = 1;
        else if(!DXExtractInteger(in[2], &state->delta)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10010", "delta value");
	    goto error_return;
        }

        DXLoopDone(FALSE);
        state->next = state->start;
        if(state->delta < 0) state->direction = -1;
        else state->direction = 1;
        if((state->start*state->direction) > (state->end*state->direction)) {
            DXLoopDone(TRUE);
            DXFreeModuleId(modid);
            DXFree(state);
            return OK;
        }
        /* cache the state information */
        obj = (Object) DXNewPrivate((Pointer)state, NextState_delete);
        DXSetCacheEntryV(obj, CACHE_PERMANENT, modid, 0, 0, NULL);
    }
    else {
        obj = DXGetCacheEntryV(modid, 0, 0, NULL);
        DXDelete(obj);
        if(obj == NULL) {
            DXSetError(ERROR_INTERNAL, "Lost state information for loop");
            goto error_return;
        }
        state = (struct NextNState *) DXGetPrivateData ((Private) obj);
        state->next += state->delta; 
    }

    out[0] = (Object)DXMakeInteger(state->next);

    if(state->next*state->direction>(state->end-state->delta)*state->direction)
    {
#if 0
        /* This will be taken care of by ExCleanupForState */
        DXSetCacheEntryV(NULL, 0, modid, 0, 0, NULL);
#endif
        DXLoopDone(TRUE);
        out[1] = (Object)DXMakeInteger(1);
    }
    else out[1] = (Object)DXMakeInteger(0);

    /* if it's the first time we save the modid so that the program */
    /* can clean up the state information when it terminates */
    if(first)
        DXSaveForId(modid);
    else
        DXFreeModuleId(modid);
    return OK;

error_return:
    out[0] = NULL;
    out[1] = NULL;
    if(state)
        DXFree(state);
    DXFreeModuleId(modid);
    DXLoopDone(TRUE);
    return ERROR;
}

Error
m_ForEachMember(Object *in, Object *out)
{
    Group group;
    struct NextGroupState *gstate = NULL;
    struct NextArrayState *astate = NULL;
    Array array, retarray;
    Pointer nextitem;
    char *modid;
    Object obj;
    int first;

    modid = DXGetModuleId();

    if(!in[0]) {
        DXSetError(ERROR_BAD_PARAMETER, "#10000", "group or list");
        goto error_return;
    }

    out[0] = out[1] = out[2] = NULL;

    first = DXLoopFirst();

    switch(DXGetObjectClass(in[0])) {
      case CLASS_MULTIGRID:
      case CLASS_COMPOSITEFIELD:
      case CLASS_SERIES:
      case CLASS_GROUP:
          group = (Group)(in[0]);
          if(first) {
              DXLoopDone(FALSE);
              gstate = DXAllocateZero(sizeof(struct NextGroupState));
              if(gstate == NULL)
                  goto error_return;
              gstate->next = 0;
              DXGetMemberCount(group, &gstate->end);
              gstate->end--;
              obj = (Object)DXNewPrivate((Pointer)gstate, NextState_delete);
              DXSetCacheEntryV((Object)obj, CACHE_PERMANENT,
                             modid, 0, 0, NULL);
          }
          else {
              obj = DXGetCacheEntryV(modid, 0, 0, NULL);
              DXDelete(obj);
              if(obj == NULL) {
                  DXSetError(ERROR_INTERNAL, "Lost state information for loop");
                  goto error_return;
              }
              gstate = (struct NextGroupState *) DXGetPrivateData((Private)obj);
              gstate->next++; 
          }
          if(gstate->end >= 0) {
              out[0] = (Object)DXGetEnumeratedMember(group, gstate->next, NULL);
              out[1] = (Object)DXMakeInteger(gstate->next);
              if(gstate->next == gstate->end) {
                  DXLoopDone(TRUE);
                  out[2] = (Object)DXMakeInteger(1);
              }
              else
                  out[2] = (Object)DXMakeInteger(0);
          }
          else { /* we have an empty group */
              out[0] = NULL;
              out[1] = (Object)DXMakeInteger(0);
              out[2] = (Object)DXMakeInteger(1);
              DXLoopDone(TRUE);
          }    
          break;
      case CLASS_ARRAY:
      case CLASS_REGULARARRAY:
      case CLASS_PATHARRAY:
      case CLASS_PRODUCTARRAY:
      case CLASS_MESHARRAY:
          array = (Array)in[0];
          if(first) {
              DXLoopDone(FALSE);
              astate = DXAllocateZero(sizeof(struct NextArrayState));
              if(astate == NULL)
                  goto error_return;
              DXGetArrayInfo(array, &astate->items, &astate->type, 
                             &astate->category, &astate->rank, astate->shape);
              astate->isize = DXGetItemSize(array);
              astate->buf = DXAllocate(astate->isize);
              if(astate->buf == NULL)
                 goto error_return;
              astate->ahandle = DXCreateArrayHandle(array);
              if(!astate->ahandle)
                  goto error_return;
              DXReference(in[0]);
              astate->in_obj = in[0];
              astate->next = 0;
              astate->items--;
              obj = (Object)DXNewPrivate((Pointer)astate, NextArrState_delete);
              DXSetCacheEntryV((Object)obj, CACHE_PERMANENT,
                             modid, 0, 0, NULL);
          }
          else {
              obj = DXGetCacheEntryV(modid, 0, 0, NULL);
              DXDelete(obj);
              if(obj == NULL) {
                  DXSetError(ERROR_INTERNAL, "Lost state information for loop");
                  goto error_return;
              }
              astate = (struct NextArrayState *) DXGetPrivateData((Private)obj);
              astate->next++;
          }
          if(astate->items >= 0) {
              nextitem = DXGetArrayEntry(astate->ahandle, astate->next,
                                         astate->buf);
              retarray = DXNewArrayV(astate->type, astate->category, 
                                     astate->rank, astate->shape);
              retarray = DXAddArrayData(retarray, 0, 1, nextitem);
              out[0] = (Object)retarray;
              out[1] = (Object)DXMakeInteger(astate->next);
              if(astate->next == astate->items) {
                  DXLoopDone(TRUE);
                  out[2] = (Object)DXMakeInteger(1);
              }
              else out[2] = (Object)DXMakeInteger(0);
          }
          else { /* empty array */
              out[0] = NULL;
              out[1] = (Object)DXMakeInteger(0);
              out[2] = (Object)DXMakeInteger(1);
              DXLoopDone(TRUE);
          }
          break;
      default:
          DXSetError(ERROR_BAD_PARAMETER, "input must be group or list");
          goto error_return;
    }

    /* if it's the first time we save the modid so that the program */
    /* can clean up the state information when it terminates */
    if(first)
        DXSaveForId(modid);
    else
        DXFreeModuleId(modid);
    return OK;

error_return:
    out[0] = NULL;
    out[1] = NULL;
    out[2] = NULL;
    if(astate) 
        DXFree(astate);
    if(gstate)
        DXFree(gstate);
    DXFreeModuleId(modid);
    DXLoopDone(TRUE);
    return ERROR;
}
