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
#include "d.h"
#include "_macro.h"

EXDictionary _dxd_exMacroDict = NULL;

/*
 * These routines are simply wrappers around the dictionary routines to
 * reduce (minimally at best) the complexity of the interface.
 */

Error
_dxf_ExMacroInit(void)
{
    _dxd_exMacroDict = _dxf_ExDictionaryCreate (2048, TRUE, FALSE);
    return (_dxd_exMacroDict == NULL? ERROR: OK);
}

int
_dxf_ExMacroInsert(char *name, EXObj obj)
{
    return(_dxf_ExDictionaryInsert (_dxd_exMacroDict, name, obj));
}

int
_dxf_ExMacroDelete(char *name)
{
    return(_dxf_ExDictionaryDelete (_dxd_exMacroDict, name));
}

EXObj
_dxf_ExMacroSearch(char *name)
{
    return(_dxf_ExDictionarySearch(_dxd_exMacroDict, name));
}
