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
#include "PrintProgramDialog.h"
#include "PrintProgramFileDialog.h"


#include <sys/stat.h>

boolean PrintProgramFileDialog::ClassInitialized = FALSE;

String PrintProgramFileDialog::DefaultResources[] =
{
        "*dialogTitle:     Save Program As PostScript...",
        "*dirMask:         *.ps",
        NULL
};


void PrintProgramFileDialog::okFileWork(const char *filename)
{
    this->printProgramDialog->setFileName(filename);
}

PrintProgramFileDialog::PrintProgramFileDialog( PrintProgramDialog *ppd,
						Network *network ) :
			   FileDialog("printProgramFileDialog", 
			    ppd->getRootWidget())
{
    this->network = network;
    this->printProgramDialog = ppd;

    if (NOT PrintProgramFileDialog::ClassInitialized)
    {
        PrintProgramFileDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void PrintProgramFileDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				PrintProgramFileDialog::DefaultResources);
    this->FileDialog::installDefaultResources( baseWidget);
}

char *PrintProgramFileDialog::getDefaultFileName()
{
   char *newfile = NULL; 
   const char *file = this->network->getFileName();

   if (file) {
	newfile = DuplicateString(file);
        char *c = strstr(newfile,".net");
        if (c) {
            strcpy(c,".ps");
        } else {
            strcat(newfile,".ps");
        }
   }
   return newfile;
}

