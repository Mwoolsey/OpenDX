/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* 
 * (C) COPYRIGHT International Business Machines Corp. 1995
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 * 
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <dxconfig.h>
#include "macroutil.h"

#include <dx/dx.h>
#include "../dpexec/pmodflags.h"
#include "../dpexec/function.h"

/*
 * These are modules which are internal to the exec and aren't exposed to
 * the user in the dx.mdf file.
 */
Error
_dxf_private_modules()
{
    {
    DXAddModule("MacroStart", m_MacroStart, 
	MODULE_PASSTHRU,
        51, "count",  "i00", "i01", "i02", "i03", "i04", "i05", "i06", "i07",
        "i08", "i09", "i10", "i11", "i12", "i13", "i14", "i15", "i16", "i17", 
	"i18", "i19", "i20", "i21", "i22", "i23", "i24", "i25", "i26", "i27", 
	"i28", "i29", "i30", "i31", "i32", "i33", "i34", "i35", "i36", "i37", 
	"i38", "i39", "i40", "i41", "i42", "i43", "i44", "i45", "i46", "i47", 
	"i48", "i49",
        50,    "o00", "o01", "o02", "o03", "o04", "o05", "o06", "o07", "o08", 
	"o09", "o10", "o11", "o12", "o13", "o14", "o15", "o16", "o17", "o18", 
	"o19", "o20", "o21", "o22", "o23", "o24", "o25", "o26", "o27", "o28", 
	"o29", "o30", "o31", "o32", "o33", "o34", "o35", "o36", "o37", "o38", 
	"o39", "o40", "o41", "o42", "o43", "o44", "o45", "o46", "o47", "o48", 
	"o49");
    }
    {
    /* This used to be set to MODULE_PASSBACK too but there were problems */
    /* with not generating all the outputs especially when the macro      */
    /* was upstream from a switch */
    DXAddModule("MacroEnd", m_MacroEnd,
        MODULE_LOOP | MODULE_ERR_CONT,
        51, "count",  "i00", "i01", "i02", "i03", "i04", "i05", "i06", "i07",
        "i08", "i09", "i10", "i11", "i12", "i13", "i14", "i15", "i16", "i17", 
	"i18", "i19", "i20", "i21", "i22", "i23", "i24", "i25", "i26", "i27", 
	"i28", "i29", "i30", "i31", "i32", "i33", "i34", "i35", "i36", "i37", 
	"i38", "i39", "i40", "i41", "i42", "i43", "i44", "i45", "i46", "i47", 
	"i48", "i49", 
	50,    "o00", "o01", "o02", "o03", "o04", "o05", "o06", "o07", "o08", 
	"o09", "o10", "o11", "o12", "o13", "o14", "o15", "o16", "o17", "o18", 
	"o19", "o20", "o21", "o22", "o23", "o24", "o25", "o26", "o27", "o28", 
	"o29", "o30", "o31", "o32", "o33", "o34", "o35", "o36", "o37", "o38", 
	"o39", "o40", "o41", "o42", "o43", "o44", "o45", "o46", "o47", "o48", 
	"o49");
    }
    return OK;
}
