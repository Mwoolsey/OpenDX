/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>


#include "DXStrings.h"
#include "Application.h"
#include "TextFile.h"
#include "TextFileFileDialog.h"


boolean TextFile::ClassInitialized = FALSE;
String TextFile::DefaultResources[] =
{
        ".fsdButton.labelString: ...",
        ".fsdButton.recomputeSize: True",
        NULL
};


TextFile::TextFile(const char *name) : UIComponent(name)
{
    this->initInstanceData();
}
TextFile::TextFile() : UIComponent("textFile")
{
    this->initInstanceData();

    if (NOT TextFile::ClassInitialized)
    {
        TextFile::ClassInitialized = TRUE;
	this->installDefaultResources();
    }
}
void TextFile::installDefaultResources()
{
    ASSERT(theApplication);
    this->setDefaultResources(theApplication->getRootWidget(),
				TextFile::DefaultResources);
}

void TextFile::initInstanceData() 
{
    this->textFileFileDialog = NULL;
    this->fileName = NULL;
    this->setTextCallback = NULL;
    this->changeTextCallback = NULL;
    this->callbackData = NULL;
}

TextFile::~TextFile()
{
}

void TextFile::initialize()
{
}

extern "C" void TextFile_ButtonCB(Widget w,
				XtPointer clientdata,
				XtPointer callData)
                               		
{
    TextFile *tf = (TextFile *)clientdata;
    tf->postFileSelector();	  

}

void TextFile::postFileSelector()
{
    if(!this->textFileFileDialog){
	this->textFileFileDialog = new TextFileFileDialog(this);
    }

    this->textFileFileDialog->post();
					
}	

extern "C" void TextFile_ActivateTextCB(Widget    widget,
                            XtPointer clientData,
                            XtPointer callData)
{
    ASSERT(clientData);
    TextFile *tf = (TextFile*) clientData;
    char *v = XmTextGetString(widget);
    tf->activateTextCallback(v, tf->callbackData);
    XtFree(v);
}
extern "C" void TextFile_ValueChangedTextCB(Widget    widget,
                            XtPointer clientData,
                            XtPointer callData)
{
    ASSERT(clientData);
    TextFile *tf = (TextFile*) clientData;
    char *v = XmTextGetString(widget);
    tf->valueChangedTextCallback(v, tf->callbackData);
    XtFree(v);
}
Widget TextFile::createTextFile(Widget parent, 
				TextFileSetTextCallback stc,
				TextFileChangeTextCallback ctc,
				void *callData)
{
    ASSERT(!this->getRootWidget());
    this->initialize();

    this->setTextCallback = stc;
    this->changeTextCallback = ctc;
    this->callbackData = callData;

    Widget form = XtVaCreateWidget(this->name,xmFormWidgetClass, parent, NULL);
    this->setRootWidget(form);

    this->fsdButton = XtVaCreateManagedWidget("fsdButton",
        xmPushButtonWidgetClass,form,
        XmNrightAttachment,     XmATTACH_FORM,
	XmNrightOffset,		5,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		3,		// Center on text widget
        NULL);

    this->fileName = XtVaCreateManagedWidget("fileName",
        xmTextWidgetClass,form,
	XmNleftAttachment, 	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->fsdButton,
	XmNrightOffset,		10,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
        NULL);
	
    this->enableCallbacks();
    this->manage();
    return this->getRootWidget();
}

//
// De/Install callbacks for the text widget. 
//
void TextFile::enableCallbacks(boolean enable)
{
    ASSERT(this->getRootWidget() && this->fileName);

    XtRemoveCallback(this->fileName, XmNactivateCallback,
		(XtCallbackProc)TextFile_ActivateTextCB, 
		(XtPointer)this);
    XtRemoveCallback(this->fileName, XmNvalueChangedCallback,
		(XtCallbackProc)TextFile_ValueChangedTextCB, 
		(XtPointer)this);

    if (enable) {
	XtAddCallback(this->fileName, XmNactivateCallback,
		    (XtCallbackProc)TextFile_ActivateTextCB, 
		    (XtPointer)this);
	XtAddCallback(this->fileName, XmNvalueChangedCallback,
		    (XtCallbackProc)TextFile_ValueChangedTextCB, 
		    (XtPointer)this);
    	XtAddCallback(this->fsdButton,XmNactivateCallback,
		    (XtCallbackProc)TextFile_ButtonCB,
		    (XtPointer)this);
	}


}
char *TextFile::getText()
{
    return XmTextGetString(this->fileName);
}
void TextFile::setText(const char *value, boolean doActivate)
{
    XmTextSetString(this->fileName, (char*)value);

    //
    // Right justify the text
    //
    int len = STRLEN(value);
    XmTextShowPosition(this->fileName,len);
    XmTextSetInsertionPosition(this->fileName,len);

    if (doActivate)
	XtCallCallbacks(this->fileName,XmNactivateCallback, this);
}
void TextFile::fileSelectCallback(const char *value)
{
    this->setText(value, TRUE);
}
void TextFile::activateTextCallback(const char *value, void *callData)
{
    if (this->setTextCallback)
	this->setTextCallback(this, value, callData);
}
void TextFile::valueChangedTextCallback(const char *value, void *callData)
{
    if (this->changeTextCallback)
	this->changeTextCallback(this, value, callData);
}


