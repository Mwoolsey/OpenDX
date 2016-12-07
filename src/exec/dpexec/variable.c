/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <time.h>
#include <dx/dx.h>
#include "config.h"
#include "_variable.h"
#include "background.h"
#include "graph.h"
#include "log.h"
#include "distp.h"
#include "rq.h"

typedef struct varupdateArg
{
    char *name;
    Object obj;
    int cause_execution;
} varupdateArg;

typedef struct vargetArg
{
    char *name;
    Object obj;
} vargetArg;

static Error ExVariableGet (vargetArg *va);

/*
 * These routines serve to insulate from the dictionary routines.  They
 * are used to insert variables into the dictionary.
 */

int
_dxf_ExVariableInsert(char *name, EXDictionary dict, EXO_Object obj)
{
    int		ret;

    if (!strcmp(name, "NULL"))
    {
	DXWarning ("#4630");
	return (OK);
    }
    else
    {
	ret = _dxf_ExDictionaryInsert (dict, name, obj);

	if (dict == _dxd_exGlobalDict) 
	    _dxf_ExBackgroundChange ();
        

	return (ret);
    }
}

int
_dxf_ExVariableInsertNoBackground(char *name, EXDictionary dict, EXO_Object obj)
{
    int		ret;

    if (!strcmp(name, "NULL"))
    {
	DXWarning ("#4630");
	return (OK);
    }
    else
    {
	ret = _dxf_ExDictionaryInsert (dict, name, obj);
	return (ret);
    }
}

int
_dxf_ExVariableDelete(char *name, EXDictionary dict)
{
    return (_dxf_ExDictionaryDelete (dict, name));
}

EXObj
_dxf_ExVariableSearch (char *name, EXDictionary dict)
{
    return (_dxf_ExDictionarySearch (dict, name));
}


/* global variable routines
 */

void
_dxf_ExGVariableSetStr (char *var, char *val)
{
    gvar	*gv;
    String	sobj;
    GDictSend   pkg;

    sobj = DXNewString (val);
    gv = _dxf_ExCreateGvar (GV_DEFINED);
    _dxf_ExDefineGvar (gv, (Object)sobj);
    gv->cost = CACHE_PERMANENT;
    _dxf_ExVariableInsertNoBackground (var, _dxd_exGlobalDict, (EXObj)gv);
    _dxf_ExCreateGDictPkg (&pkg, var, (EXObj)gv);
    _dxf_ExDistributeMsg (DM_INSERTGDICT, (Pointer)&pkg, 0, TOSLAVES);
}

char *
_dxf_ExGVariableGetStr(char *var)
{
    char	*val;

    vargetArg *va;

    va = (vargetArg *) DXAllocate(sizeof(vargetArg));
    va->name = var;
    va->obj = NULL;
    _dxf_ExRunOn (1, ExVariableGet, (Pointer)va, 0);
    if (DXExtractString ((Object)va->obj, &val) == NULL)
        val = NULL;
    DXFree(va);
    return (val);
}

/* create a new executive object (a gvar) for a normal dx object,
 * and insert it into the global dictionary.  if flag is not set,
 * insert it without causing a new graph execution.
 */
Error
_dxf_ExUpdateGlobalDict (char *name, Object obj, int cause_execution)
{
    Error       rc;
    gvar        *gv;
    GDictSend   pkg;
    int 	to;
    
    ExDebug("*1","adding %s to global dictionary", name);
    
    gv = _dxf_ExCreateGvar (GV_UNRESOLVED);
    _dxf_ExDefineGvar (gv, obj);
    
    if(_dxd_exRemoteSlave)
       to = TOPEER0;
    else
       to = TOSLAVES;
    if (cause_execution) {
	rc =  _dxf_ExVariableInsert(name, _dxd_exGlobalDict, (EXObj)gv);
        _dxf_ExCreateGDictPkg(&pkg, name, &gv->object);
        _dxf_ExDistributeMsg(DM_INSERTGDICT, (Pointer)&pkg, 0, to);
    }
    else {
	rc =  _dxf_ExVariableInsertNoBackground(name, _dxd_exGlobalDict, 
						(EXObj)gv);
        _dxf_ExCreateGDictPkg(&pkg, name, &gv->object);
        _dxf_ExDistributeMsg(DM_INSERTGDICTNB, (Pointer)&pkg, 0, to);
    }
    
    return rc;
}

Error ExVariableSet(varupdateArg *va)
{
    int ret; 

    if(va == NULL) 
        return(ERROR);
        
    ret = _dxf_ExUpdateGlobalDict(va->name, va->obj, va->cause_execution);
    DXFree(va);
    return(ret);
}

Error DXSetIntVariable(char *name, int val, int sync, int cause_execution) 
{
    Array       arr;

    ExDebug("*1","Setting IntVar %s to %d", name, val);

    if (!(arr = DXNewArray (TYPE_INT, CATEGORY_REAL, 0)))
        return ERROR;

    if (!DXAddArrayData (arr, 0, 1, (Pointer) &val))
        return ERROR; 

    return(DXSetVariable(name, (Object)arr, sync, cause_execution));
}

Error DXSetVariable(char *name, Object obj, int sync, int cause_execution) 
{
    varupdateArg *va;
    int ret;
 
    ExDebug("*1","Setting Var %s to obj %x", name, obj);
    if (DXProcessorId() == 0) {
        _dxf_ExUpdateGlobalDict(name, obj, cause_execution);
        return(OK);
    }

    va = (varupdateArg *) DXAllocate(sizeof(varupdateArg));
    va->name = name;
    va->obj = obj;
    va->cause_execution = cause_execution;
    if(sync) {
        ret = _dxf_ExRunOn (1, ExVariableSet, va, 0);
        return(ret);
    }
    else {
        _dxf_ExRQEnqueue(ExVariableSet, (Pointer)va, 1, 0, 1, 0);
        return(OK);
    }

}

static Error ExVariableGet (vargetArg *va)
{
    gvar        *gv;

    if ((gv = (gvar *) _dxf_ExGlobalVariableSearch (va->name)) == NULL)
        return (ERROR);

    va->obj = gv->obj;

    ExDelete (gv);

    return (OK);
}

Error DXGetVariable(char *name, Object *obj)
{
    vargetArg *va;
    int ret;

    va = (vargetArg *) DXAllocate(sizeof(vargetArg));
    va->name = name;
    ret = _dxf_ExRunOn (1, ExVariableGet, va, 0);

    if (ret == OK)
	*obj = va->obj;
    else
	*obj = NULL;

    DXFree(va);
    return (ret);
}


Error DXGetIntVariable(char *name, int *val)
{
    Object obj;
    
    if (!DXGetVariable(name, &obj))
	return ERROR;

    DXExtractInteger (obj, val);

    return OK;
}


    

/* 
 * find a variable by name in the global dictionary
 */
EXObj _dxf_ExGlobalVariableSearch (char *name)
{
    return _dxf_ExDictionarySearch (_dxd_exGlobalDict, name);
}


