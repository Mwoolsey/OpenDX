/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include "DXStrings.h"
#include "Application.h"
#include "TextFileFileDialog.h"
#include "TextFile.h"

#include <sys/stat.h>

boolean TextFileFileDialog::ClassInitialized = FALSE;

String TextFileFileDialog::DefaultResources[] =
{
        "*dialogTitle:     Save Macro As File...",
        "*dirMask:         *.net",
        NULL
};


void TextFileFileDialog::okFileWork(const char *filename)
{
    this->textFile->fileSelectCallback(filename);	
}

//
// Constructor for derived classes 
//
TextFileFileDialog::TextFileFileDialog(const char *name, TextFile *tf ) :
			   FileDialog(name, tf->getRootWidget())
{
    this->textFile = tf;
}
//
// Constructor for instances of THIS class
//
TextFileFileDialog::TextFileFileDialog( TextFile *tf ) :
			   FileDialog("textFileFileDialog", 
			    tf->getRootWidget())
{
    this->textFile = tf;
    if (NOT TextFileFileDialog::ClassInitialized)
    {
        TextFileFileDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

void TextFileFileDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, TextFileFileDialog::DefaultResources);
    this->FileDialog::installDefaultResources( baseWidget);
}







