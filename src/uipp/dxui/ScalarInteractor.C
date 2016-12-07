/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include <Xm/MainW.h>

#include "ScalarInteractor.h"
#include "ScalarNode.h"
#include "ScalarInstance.h"
#include "../widgets/Number.h"
#include "WarningDialogManager.h"


extern "C" void ScalarInteractor_NumberWarningCB(Widget  widget,
                        XtPointer clientData,
                        XtPointer callData)
{
    XmNumberWarningCallbackStruct* warning;

#if 1
    while(NOT XmIsMainWindow(widget))
    {
        widget = XtParent(widget);
    }
#endif
    warning = (XmNumberWarningCallbackStruct*)callData;
    // FIXME: this should be modal
    // ModalWarningMessage(widget, warning->message);
    WarningMessage(warning->message);
}


