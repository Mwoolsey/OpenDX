/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DataFileDialog.h"
#include "Application.h"
#include "GARChooserWindow.h"

boolean DataFileDialog::ClassInitialized = FALSE;

String DataFileDialog::DefaultResources[] = {
    ".dialogTitle:		Data File Selection",
    "*dialogStyle:		XmDIALOG_MODELESS",
    ".dialogStyle:		XmDIALOG_MODELESS",
    NUL(char*)
};

DataFileDialog::DataFileDialog (Widget parent, GARChooserWindow *gcw) :
    FileDialog ("dataFileDialog", parent)
{
    this->gcw = gcw;

    if (!DataFileDialog::ClassInitialized) {
	this->installDefaultResources(theApplication->getRootWidget());
	DataFileDialog::ClassInitialized = TRUE;
    }
}

void DataFileDialog::installDefaultResources (Widget baseWidget)
{
    this->setDefaultResources (baseWidget, DataFileDialog::DefaultResources);
    this->FileDialog::installDefaultResources(baseWidget);
}

DataFileDialog::~DataFileDialog(){}


void DataFileDialog::okFileWork (const char *string)
{
#if defined(intelnt)
    char *str= new char[strlen(string) + 2];
    strcpy(str, string);
/* Convert to Unix if DOS */
    for(int i=0; i<strlen(str); i++)
        if(str[i] == '\\') str[i] = '/';
    this->gcw->setDataFile(str);
    delete(str);
#else
    this->gcw->setDataFile(string);
#endif
}
