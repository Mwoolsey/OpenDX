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
#include "Network.h"
#include "SaveAsDialog.h"
//#include "QuestionDialogManager.h"
#include "OpenCommand.h"


#include <sys/stat.h>

boolean SaveAsDialog::ClassInitialized = FALSE;

String SaveAsDialog::DefaultResources[] =
{
        "*dialogTitle:     Save As...",
        "*dirMask:         *.net",
        NULL
};


void SaveAsDialog::saveFile(const char *filename)
{
    boolean success;
    success = this->network->saveNetwork(filename);

    if(success AND this->postCmd)
	((OpenCommand*)this->postCmd)->execute();
}

SaveAsDialog::SaveAsDialog(Widget parent, Network *network) : 
                       SaveFileDialog("saveAsDialog", parent,".net")
{
    this->network = network;
    this->postCmd = NULL;

    if (NOT SaveAsDialog::ClassInitialized)
    {
        SaveAsDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void SaveAsDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SaveAsDialog::DefaultResources);
    this->SaveFileDialog::installDefaultResources( baseWidget);
}

char *SaveAsDialog::getDefaultFileName()
{
   const char *netname = this->network->getFileName();
   if (netname)
	return DuplicateString(netname);
   else
	return NULL;
}

#if 0	// Moved to FileDialog 7/18/94
//
// Install the current file name in the file selection box.
void SaveAsDialog::manage()
{
   this->FileDialog::manage(); 
   const char *file = this->cp->getNetwork()->getFileName();
   if (file)
       this->setFileName(file);
}

#endif
