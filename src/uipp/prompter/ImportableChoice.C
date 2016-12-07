/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ImportableChoice.h"
#include "GARApplication.h"
#include "GARChooserWindow.h"
#include "ToggleButtonInterface.h"
#include "Command.h"
#include "WarningDialogManager.h"
#include "DXStrings.h"

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>

//
// This class provides support for data types which are directly importable
// by MOD-import.  These data types get just a 2d/3d button and a visualize
// operations.
//

boolean ImportableChoice::ClassInitialized = FALSE;
String ImportableChoice::DefaultResources[] =
{
    NUL(char*)
};

ImportableChoice::ImportableChoice (
    const char* name,
    boolean     browsable, boolean      testable,
    boolean     visualizable, boolean   prompterable,
    GARChooserWindow *gcw,
    Symbol sym
) : TypeChoice(name, browsable, testable, visualizable, prompterable, gcw, sym)
{
}



const char *ImportableChoice::getActiveHelpMsg()
{
static char msg[256];

    sprintf (msg, "The import module can read %s files.", 
	this->getInformalName());
    return msg;
}


