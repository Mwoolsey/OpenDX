/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifdef DXUI_DEVKIT

#include "DXStrings.h"
#include "Application.h"
#include "Network.h"
#include "SaveAsCCodeDialog.h"

boolean SaveAsCCodeDialog::ClassInitialized = FALSE;

String SaveAsCCodeDialog::DefaultResources[] =
{
        "*dialogTitle:     Save As C Code...",
        "*dirMask:         *.c",
        NULL
};


void SaveAsCCodeDialog::saveFile(const char *filename)
{
    this->network->saveAsCCode(filename);
}

SaveAsCCodeDialog::SaveAsCCodeDialog(Widget parent, Network *network) : 
                       SaveFileDialog("saveAsCCodeDialog", parent,".c")
{
    this->network = network;

    if (NOT SaveAsCCodeDialog::ClassInitialized)
    {
        SaveAsCCodeDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void SaveAsCCodeDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SaveAsCCodeDialog::DefaultResources);
    this->SaveFileDialog::installDefaultResources( baseWidget);
}

char *SaveAsCCodeDialog::getDefaultFileName()
{
   const char *netname = this->network->getFileName();
   if (netname) {
	char *p, *cname = DuplicateString(netname);
 	p = (char*)strrstr(cname,".net");
	if (p) *p = '\0';
	strcat(cname,".c");
	return cname; 
   } else
	return NULL;
}

#endif // DXUI_DEVKIT
