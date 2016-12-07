/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _XmUtility_h
#define _XmUtility_h

#include <Xm/Text.h>


#define RES_CONVERT(res, str) XtVaTypedArg, res, XmRString, str, strlen(str)+1

//
// Return values used by VerifyNameText().
//
#define  XmU_TEXT_VALID		0
#define  XmU_TEXT_INVALID	1		
#define  XmU_TEXT_DELETE	2

extern void SetTextSensitive(Widget w, Boolean sens, Pixel foreground);

extern int  VerifyNameText(Widget text, XmTextVerifyCallbackStruct* cbs);

extern int  VerifyGridText(Widget text, XmTextVerifyCallbackStruct* cbs);

extern Widget CreateFormDialog(Widget parent, String name, ArgList arglist, Cardinal argcount);

#endif /* _XmUtility_h */

