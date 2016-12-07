/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "config.h"
#include "cachegraph.h"
#include "pmodflags.h"
#include "cache.h"
#include "utils.h"
#include "graph.h"
#include "log.h"
#include "crc.h"
#include "_variable.h"

#define STATIC_TAGS  100

/*
 * $$$$$ THIS ROUTINE SHOULD BE REPLACED WITH ONES TO DEAL WITH OBJECTS
 * $$$$$ PROPERLY.
 * The routine is called to compute the recipes for non-array objects.
 */

static uint32
computeDefaultRecipe (Object obj)
{
    uint32  crc = 0xFFFFFFFF;

    if (obj)
    {
	DXWarning ("#4610", obj);
	crc = _dxf_ExCRCLong (crc, (long) obj);
    }
    else
	crc = _dxf_ExCRCLong (crc, 0);
    
    return (EXTAG(crc));
}

/* 
 * Compute the recipe for a string object.  The crc is generated from the
 * bytes representing the string
 */

static
uint32 computeStringRecipe (String a)
{
    uint32 		crc;
    unsigned char	*p;

    crc = 0xFFFFFFFF;

    p = (unsigned char *) DXGetString (a);

    if (p)
    {
	int32		size;

	size = strlen ((char *)p);
	crc = _dxf_ExCRCByteV (crc, p, 1, size);
    }
    else
    {
	crc = _dxf_ExCRCInt (crc, 0);
    }

    return (EXTAG (crc));
}

/* 
 * Compute the recipe for an array object.  The crc is generated from the
 * parameters to the array (type, category, etc.) and the crc over the
 * bytes representing the data stored in the array.
 */

static uint32
computeArrayRecipe (Array a)
{
    uint32 		crc;
    Type		type;
    Category		category;
    int			items;
    int			rank;
    int			shape[32];
    int			i, size, tsize;
    unsigned char	*p;

    crc = 0xFFFFFFFF;

    DXGetArrayInfo (a, &items, &type, &category, &rank, shape);
    size = DXGetItemSize(a);
    tsize = DXTypeSize(type);

    crc = _dxf_ExCRCByte (crc, type);
    crc = _dxf_ExCRCByte (crc, category);
    crc = _dxf_ExCRCInt  (crc, items);
    crc = _dxf_ExCRCByte (crc, rank);

    if(DXGetArrayClass(a) == CLASS_REGULARARRAY) {
        Pointer origin;
        Pointer delta; 

        origin = DXAllocateLocal(2*size);
        delta = (Pointer)((char *)origin + size);
        DXGetRegularArrayInfo((RegularArray)a, NULL, origin, delta);
	crc = _dxf_ExCRCByteV (crc, (unsigned char *) "regulararray", 1, 12);
	crc = _dxf_ExCRCByteV (crc, origin, tsize, 2*size/tsize);
        DXFree(origin);
    }
    else {
        p = (unsigned char *) DXGetArrayData (a);
        if (p)
        {
	    if (rank)
	        for (i=0; i < rank; i++)
		    crc = _dxf_ExCRCInt (crc, shape[i]);
	    size *= items;
	    crc = _dxf_ExCRCByteV (crc, p, tsize, size/tsize);
        }
        else
        {
            crc = 0xFFFFFFFF;
	    crc = _dxf_ExCRCInt (crc, 0);
        }
    }

    return (EXTAG(crc));
}

static uint32 
computeRecipe(Program *p, int ind)
{
    ProgramVariable *pv;
    Object obj;
    uint32 tcrc;
    gvar *gv;

    if (ind == -1) 
        return(computeDefaultRecipe (NULL));

    pv = FETCH_LIST(p->vars, ind);
    gv = pv->gv;
    /* gv will be null for prehidden inputs for outboards, anything else?*/
    if(!gv)
        gv = _dxf_ExCreateGvar(GV_UNRESOLVED);

    /* if input already has a recipe, just use it,
     * Otherwise, it must be some kind of known variable
     */
    if (gv->reccrc == 0)
    {
	obj = gv->obj;
	if (obj == NULL)
	    obj = pv->dflt;

	if (obj == NULL)
	    tcrc = computeDefaultRecipe (obj);
	else
	{
	    switch (DXGetObjectClass (obj))
	    {
		case CLASS_ARRAY:
		    tcrc = computeArrayRecipe ((Array) obj);
		    break;

		case CLASS_STRING:
		    tcrc = computeStringRecipe ((String) obj);
		    break;

		default:
		    tcrc = computeDefaultRecipe (obj);
		    break;
	    }
	}
        pv->reccrc = gv->reccrc = tcrc;
	DXSetObjectTag (obj, tcrc);
    }

    return(gv->reccrc);
}

/*---------------------------------------------------------------*/
/* This function maintains a table with path/cache tag entries   */
/* It searches a table looking for the current path and if found */
/*   1. does nothing if the cache tag hasn't changed             */ 
/*   2. replaces the current entry with the new path/tag entry   */
/*      if the cache tag has changed, and deletes the object     */
/*      identified by the old cache tag from the cache           */
/*  Note: cache entry may not necessarily be in the cache ...    */
/*  Question: Should this be conditional -i.e. based on the      */
/*            setting of a user specified "cache once" switch?   */  
/*---------------------------------------------------------------*/
int
_dxf_ExManageCacheTable(ModPath *mod_path, uint32 reccrc, int outnbr)
{
    int i, limit;
    pathtag  *pt;
    pathtag  npt;
    int	     ret = FALSE;

    DXDebug ("1", 
     "cache table entry: cpath = %s, reccrc = 0x%08x, outnbr = %d",
             _dxf_ExPathToString(mod_path), reccrc, outnbr);	

    limit = SIZE_LIST(_dxd_pathtags); 

    for (i = 0; i < limit; i++ ) { 
        pt = FETCH_LIST(_dxd_pathtags,i);
        if (pt->entry_inuse == TRUE) {
            if (_dxf_ExPathsEqual( &pt->mod_path, mod_path ) && 
	              pt->outnbr == outnbr) {
                DXDebug ("1", "  Found cache table entry %s / 0x%08x.",
                    _dxf_ExPathToString( &pt->mod_path ), pt->reccrc);	
                if (pt->reccrc != reccrc ) {
                    DXDebug ("1",
                     "Changing tag 0x%08x to 0x%08x for cache table entry %s.",
                          pt->reccrc, reccrc, 
			  _dxf_ExPathToString( &pt->mod_path ));	
                    if(pt->reccrc != 0)
                        _dxf_ExCacheDelete(pt->reccrc);
                    pt->reccrc = reccrc;
                    ret = TRUE;
                }
                return(ret);
            }
        }
    }

    /* Create new entry in pathtag table unless there is one 
     * available for reuse 
     */
    for (i = 0; i < limit; i++ ) { 
        pt = FETCH_LIST(_dxd_pathtags,i);
        if (pt->entry_inuse == FALSE) {
            DXDebug ("1", "  Reusing cache table entry %s.%d / 0x%08x.",
                _dxf_ExPathToString( &pt->mod_path ), pt->outnbr, pt->reccrc);	
            pt->reccrc = reccrc;
            pt->outnbr = outnbr;
            pt->entry_inuse = TRUE;
	    _dxf_ExPathCopy( &pt->mod_path, mod_path );
            return(TRUE);
        }
    }

    npt.reccrc = reccrc;
    npt.outnbr = outnbr;
    npt.entry_inuse = TRUE;
    _dxf_ExPathCopy( &npt.mod_path, mod_path );
    DXDebug ("1", "  Appending cache table entry %s.%d / 0x%08x.",
            _dxf_ExPathToString( &npt.mod_path ), npt.outnbr, npt.reccrc);	
    APPEND_LIST(pathtag,_dxd_pathtags,npt);
    return(TRUE); 
}

void _dxf_ExFlushPathTags()
{
    int i;
    pathtag             *pt;

    for (i = 0; i < SIZE_LIST(_dxd_pathtags); i++ ) {
        pt = FETCH_LIST(_dxd_pathtags,i);
        pt->entry_inuse = FALSE;
    }
}

/*
 * recursively generate the recipes for a node in the graph
 */
void _dxf_ExComputeRecipes(Program *p, int funcInd)
{
    gfunc	*n = FETCH_LIST(p->funcs, funcInd);
    int		i, j, size, ntags, macro_tags=0, extra_tags, async_tags=0;
    gvar	*gv;
    uint32 	tcrc;
    gvar	*out;
    uint32 	*inTags, staticTags[STATIC_TAGS];
    int		varInd, *varIndp;
    int		varInd2;
    ProgramVariable *pv, *inpv, *pv_key;
    MacroRef 	*mr;
    AsyncVars 	*avars;
    _excache    fcache;
    _excache    pvcache;
    Program     *subP=NULL;
    int  tag_changed = FALSE;
    uint32 crc = 0xFFFFFFFF;
    char 	*async_name=NULL, *name;
    int		passthru = FALSE;
    char mod_cache_str[ MAX_PATH_STR_LEN ], *mod;
    
    /*  Get module path string  */
    mod = _dxf_ExGFuncPathToCacheString( n );
    if ( strlen(mod) > sizeof(mod_cache_str)-1 )
    _dxf_ExDie("Module path is too long");
    strcpy( mod_cache_str, mod );

check_async:
    if(n->flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL)) {
        if(p->loopctl.first && (n->flags & MODULE_ASYNCLOCAL)) {
            if(strcmp(n->name, "GetLocal") == 0)
                DXReadyToRunNoExecute(mod_cache_str);
        }
        varInd = *FETCH_LIST(n->inputs, n->nin - 2);
        pv = FETCH_LIST(p->vars, varInd);
        if(!pv->gv)
            _dxf_ExDie("Cannot compute a cache tag from a NULL gvar");
        if(n->flags & MODULE_ASYNCNAMED) {
            varInd2 = *FETCH_LIST(n->inputs, n->rerun_index);
            pv_key = FETCH_LIST(p->vars, varInd2);
            if(pv_key->gv && pv_key->gv->obj) {
                if(pv->gv->obj)
                    DXDelete(pv->gv->obj);
                pv->gv->obj = DXReference(pv_key->gv->obj);    
            }
        }
        if(!pv->gv->obj)
            _dxf_ExDie("Cannot compute a cache tag from a NULL gvar");
        /* Put an extra reference on this input, we don't want this gvar
           to be deleted until the current graph finishes executing. This
           is incase the results were thrown out of cache and because of
           a switch need to run again. If this is in a loop it will get
           an extra ref each time through the loop which isn't needed but
           will get cleaned up ok in the end.
        */
        ExReference(pv->gv);
        pv->refs++;
        async_name = DXGetString((String)pv->gv->obj);
        varInd = *FETCH_LIST(n->inputs, n->nin - 1);
        pv = FETCH_LIST(p->vars, varInd);
        gv = (gvar *)_dxf_ExGlobalVariableSearch(async_name);
        if(gv) {
            if(gv != pv->gv) {
                _dxf_ExUndefineGvar(pv->gv);
                _dxf_ExDefineGvar(pv->gv, gv->obj);
            }
            ExDelete(gv);
        }
        else {
            Object value;
            value = (Object) _dxfExNewInteger (0);
            _dxf_ExDefineGvar(pv->gv, (Object)_dxfExNewInteger(0));
            _dxf_ExUpdateGlobalDict (async_name, value, 0);
        }
        /* Put an extra reference on this input, we don't want this gvar
           to be deleted until the current graph finishes executing. This
           is incase the results were thrown out of cache and because of
           a switch need to run again. If this is in a loop it will get
           an extra ref each time through the loop which isn't needed but
           will get cleaned up ok in the end.
         */
         ExReference(pv->gv);
         pv->refs++;
    }

    if(n->ftype == F_MACRO) {
        subP = n->func.subP;
        macro_tags = SIZE_LIST(subP->macros);
        async_tags = SIZE_LIST(subP->async_vars);
        extra_tags = macro_tags + async_tags;
    }
    else if(n->flags & MODULE_LOOP) {
        /* to cause the cache tag of loop modules to change even though */
        /* their inputs don't change, use the counter for the loop */
        extra_tags = 1;
    }
    else extra_tags = 0;
    ntags = n->nin+extra_tags;

    if(ntags > STATIC_TAGS)
        inTags = (uint32 *)DXAllocate(ntags*sizeof(uint32));
    else inTags = staticTags;

    /* add the crc of all of the module inputs to the total crc */
    for (i = 0; i < SIZE_LIST(n->inputs); i++)
    {
	varIndp = FETCH_LIST(n->inputs, i);
        if(varIndp == NULL)
            _dxf_ExDie("Executive Inconsistency: _dxf_ExComputeRecipes");
        varInd = *varIndp;
      
        inTags[i] = computeRecipe(p, varInd);
	ExDebug ("1", "Cache value for input %s.%d = 0x%08x ",
			    n->name, i, inTags[i]);
    }

    if(n->ftype == F_MACRO) {
        for (j = 0; j < macro_tags; j++, i++)
        {
            crc = 0xFFFFFFFF;
            mr = FETCH_LIST(subP->macros, j);
            inTags[i] = _dxf_ExCRCInt(crc, mr->index);
        }
        for (j = 0; j < async_tags; j++, i++) {
            avars = FETCH_LIST(subP->async_vars, j);
            pv = FETCH_LIST(p->vars, avars->nameindx);
            if(!pv->gv || !pv->gv->obj)
                _dxf_ExDie("Cannot compute a cache tag from a NULL async name");
            name = DXGetString((String)pv->gv->obj);
            pv = FETCH_LIST(p->vars, avars->valueindx);
            gv = (gvar *)_dxf_ExGlobalVariableSearch(name);
            if(gv) {
                if(gv != pv->gv) {
                    _dxf_ExUndefineGvar(pv->gv);
                    _dxf_ExDefineGvar(pv->gv, gv->obj);
                }
                ExDelete(gv);
            }
            else {
                Object value;
                value = (Object) _dxfExNewInteger (0);
                _dxf_ExDefineGvar(pv->gv, (Object)_dxfExNewInteger(0));
                _dxf_ExUpdateGlobalDict (name, value, 0);
            }
            inTags[i] = computeRecipe(p, avars->valueindx);
        }
    }
    else if(n->flags & MODULE_LOOP) {
        /* to cause the cache tag of loop modules to change even though */
        /* their inputs don't change, use the counter for the loop */
        crc = 0xFFFFFFFF;
        inTags[i] = _dxf_ExCRCInt(crc, p->loopctl.counter);
    }

    if((n->flags & MODULE_PASSTHRU) && (n->nin == n->nout+1))
        passthru = TRUE;
        
    if(n->flags & (MODULE_CONTAINS_STATE | MODULE_CHANGES_STATE))
    {
        Object new_tag_obj, old_tag_obj;
        uint32 oldtag, newtag;

        newtag = _dxf_ExGenCacheTag (n->name,0,ntags,inTags);
        new_tag_obj = (Object)DXMakeInteger(newtag);
        old_tag_obj = DXGetCacheEntry(mod_cache_str, ntags, 0);
        if(!old_tag_obj) {
            DXSetCacheEntry(new_tag_obj, CACHE_PERMANENT, mod_cache_str, ntags, 0);
        }
        else {
            /* we don't want an extra reference on this */
            DXDelete(old_tag_obj); 
            if(!DXExtractInteger(old_tag_obj, (int *)&oldtag))
                oldtag = 0;
            /* this means we just caused a DXReadyToRun so we know */
            /* this macro will run this time, save the new cache tag */
            if(oldtag == 0) {
                DXSetCacheEntry(new_tag_obj,CACHE_PERMANENT,mod_cache_str,ntags,0);
                DXDelete(old_tag_obj);
            }
            else if(oldtag != newtag) {
                /* An input has changed since last time, we need to */ 
                /* cause execution of this macro and we need to recompute */
                /* the cache tag. */
                DXReadyToRunNoExecute(async_name);
                DXDelete(new_tag_obj);
                new_tag_obj = (Object)DXMakeInteger(0);
                DXSetCacheEntry(new_tag_obj,CACHE_PERMANENT,mod_cache_str,ntags,0);
                DXDelete(old_tag_obj);
                goto check_async;
            }
            else DXDelete(new_tag_obj);
        }
    }

    /* generate the recipes for all of the outputs from this module */
    /* Update the pathtag table if module cache attribute is "cache last" */
    n->tag_changed = FALSE;
    for (i = 0; i < SIZE_LIST(n->outputs); i++)
    {
	varIndp = FETCH_LIST(n->outputs, i);
        if(varIndp == NULL)
            _dxf_ExDie("Executive Inconsistency: _dxf_ExComputeRecipes");
        varInd = *varIndp;
	pv = FETCH_LIST(p->vars, varInd);
        pv->is_output = TRUE;
	out = pv->gv;

	if (out->reccrc == 0 || 
	    (n->required != -1 && (!IsSwitch(n) || !IsFastSwitch(n)))) {
            /* This code is currently just intended for macrostart */
            if(passthru) {
	        varInd2 = *FETCH_LIST(n->inputs, i+1);
		if (varInd2 != -1) {
                    inpv = FETCH_LIST(p->vars, varInd2);
                    tcrc = inpv->gv->reccrc;
		} else
	            tcrc = _dxf_ExGenCacheTag (n->name,i,n->nin+extra_tags,inTags);
            }
            else
	        tcrc = _dxf_ExGenCacheTag (n->name,i,n->nin+extra_tags,inTags);
            if(out->reccrc != 0 && tcrc != out->reccrc)
                _dxf_ExUndefineGvar(out);
            pv->reccrc = out->reccrc = tcrc;
	    /* make process group name part of cache tag */
	    if(n->procgroupid) {
		size = strlen (n->procgroupid);
		out->reccrc = EXTAG(_dxf_ExCRCByteV(out->reccrc & 0x7fffffff,
		    (unsigned char *) n->procgroupid, 1, size));
	    }
	    ExDebug ("1", "Cache value for %s.%d = 0x%08x ",
			    n->name, i, out->reccrc);
	}
        
        pvcache = pv->excache;
        fcache  = n->excache;
       
        if((n->flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL)) && 
           (pvcache == CACHE_ALL)) 
            pvcache = pv->excache = CACHE_LAST;
  
        /* I don't think we allow the user to turn off cacheing for */
        /* macros that have state in them. */
        if(n->flags & MODULE_CHANGES_STATE)
            pvcache = pv->excache = CACHE_LAST_PERM;
        if(n->flags & MODULE_CONTAINS_STATE)
            pvcache = pv->excache = CACHE_LAST;

        /* object lvl cache attribute overrides function lvl cache attribute */ 
        
        if (pvcache == CACHE_LAST || pvcache == CACHE_LAST_PERM) 
            tag_changed = _dxf_ExManageCacheTable(&n->mod_path, out->reccrc, i);
        else 
            if(fcache == CACHE_LAST && !pv->cacheattr) 
                tag_changed = _dxf_ExManageCacheTable(&n->mod_path, out->reccrc, i);
        n->tag_changed = tag_changed;
    }

    if(n->nout == 0 && !(n->flags & MODULE_SIDE_EFFECT)) {
        tcrc = _dxf_ExGenCacheTag (n->name, 0, n->nin+extra_tags, inTags);
        n->tag_changed = _dxf_ExManageCacheTable(&n->mod_path, tcrc, 0);
    }

    if(ntags > STATIC_TAGS)
        DXFree(inTags);
}

#if 0
/* This routine is for calculating the output tag of a macro */
void _dxf_ExComputeOutTags(Program *p)
{
    uint32 inTags[100];
    int	varInd, *varIndp;
    ProgramVariable *pv;
    ProgramRef *pr;
    MacroRef *mr;
    int i, j, extra_tags, macro_tags;
    Program *subP;
    gfunc *gf;
    uint32 crc = 0xFFFFFFFF;
 
    gf = FETCH_LIST(p->funcs, p->cursor);

    for (i = 0; i < SIZE_LIST(gf->inputs) && i < 100; i++)
    {
	varIndp = FETCH_LIST(gf->inputs, i);
        if(varIndp == NULL)
            _dxf_ExDie("Executive Inconsistency: _dxf_ExComputeOutTags");
        varInd = *varIndp;
      
	if (varInd == -1)
	    inTags[i] = computeDefaultRecipe (NULL);
	else
	{
	    pv = FETCH_LIST(p->vars, varInd);
            inTags[i] = pv->gv->reccrc;
         }
    }

    subP = gf->func.subP;

    macro_tags = SIZE_LIST(subP->macros);
    for (j = 0; j < macro_tags && i < 100; j++, i++)
    {
        mr = FETCH_LIST(subP->macros, j);
        inTags[i] = _dxf_ExCRCInt(crc, mr->index);
    }

    extra_tags = macro_tags;

    for (i = 0; i < SIZE_LIST(gf->outputs); i++)
    {
	varIndp = FETCH_LIST(gf->outputs, i);
        if(varIndp == NULL)
            _dxf_ExDie("Executive Inconsistency: _dxf_ExComputeOutTags");
        varInd = *varIndp;
	pv = FETCH_LIST(p->vars, varInd);
        pv->reccrc = pv->gv->reccrc = _dxf_ExGenCacheTag(gf->name, i, 
                                                         gf->nin + extra_tags,
                                                         inTags);
    }
}
#endif
