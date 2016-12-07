/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include <string.h>
#include <X11/IntrinsicP.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>

#include "DXStrings.h"
#include "lex.h"
#include "XmUtility.h"

#define  ValidNameCharSet  "abcdefghijklmnopqrstuvwxyz" \
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                           "1234567890"			\
                           "- _"

#define  ValidDimensionCharSet  "1234567890"                 \
 			   	".x"


//
// Set a text widget's sensitivity.
//
void SetTextSensitive(Widget w, Boolean sens, Pixel fg)
{
    XtVaSetValues(w,
        XmNforeground, fg,
        XmNsensitive, sens,
        NULL);
}

//
// Verify a name string input from the text widget.
// The leading spaces are not allowed for a name.
//
int VerifyNameText(Widget text, XmTextVerifyCallbackStruct* cbs)
{
    int i;

    if(cbs->text->ptr == NULL) //delete
        return XmU_TEXT_DELETE;

    char *cp = XmTextGetString(text);
    if (IsBlankString(cp)
        AND IsBlankString(cbs->text->ptr))
    {
        cbs->doit = False;
	XtFree(cp);
        return XmU_TEXT_INVALID;
    }
    XtFree(cp);

    if(cbs->currInsert == 0 AND IsWhiteSpace(cbs->text->ptr))
    {
        cbs->doit = False;
        return XmU_TEXT_INVALID;
    }

    for(i = 0; i < cbs->text->length; i++)
    {
        if(NOT strchr(ValidNameCharSet,cbs->text->ptr[i]))
        {
            cbs->doit = False;
            return XmU_TEXT_INVALID;
        }
    }

    return XmU_TEXT_VALID;
}

//
// Verify a grid definition string(eg. 240x550) input from the text widget.
//
int VerifyGridText(Widget text, XmTextVerifyCallbackStruct* cbs)
{
    int i;

    if(cbs->text->ptr == NULL) 		//delete
        return XmU_TEXT_DELETE;

    char *cp = XmTextGetString(text);
    char *x = strchr(cp,'x');  //already has 'x'?
    XtFree(cp);

    for(i = 0; i < cbs->text->length; i++)
    {
        if(NOT strchr(ValidDimensionCharSet,cbs->text->ptr[i])
	   OR (x AND cbs->text->ptr[i] =='x'))
        {
            cbs->doit = False;
            return XmU_TEXT_INVALID;
        }
    }

    return XmU_TEXT_VALID;
}

