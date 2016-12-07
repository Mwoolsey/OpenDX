/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include "Network.h"
#include "SaveCFGDialog.h"
#include "ControlPanel.h"
#include "Application.h"
#include "ErrorDialogManager.h"
#include "DXStrings.h"


boolean SaveCFGDialog::ClassInitialized = FALSE;

String SaveCFGDialog::DefaultResources[] =
{
        "*dialogTitle:         Save Configuration...",
        "*dirMask:         *.cfg",
        NULL
};

void SaveCFGDialog::saveFile(const char *string)
{
    boolean  result;
    char *op;

    result = this->network->saveCfgFile(string);
    op = "writing";

    if(NOT result)
        ErrorMessage("Error while %s configuration file %s", op, string);

}

SaveCFGDialog::SaveCFGDialog(Widget parent, Network *net) : 
                SaveFileDialog("saveCFGDialog", parent, ".cfg")
{
    this->network= net;
    this->hasCommentButton = FALSE;

    if (NOT SaveCFGDialog::ClassInitialized)
    {
        SaveCFGDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void SaveCFGDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SaveCFGDialog::DefaultResources);
    this->SaveFileDialog::installDefaultResources( baseWidget);
}
// FIXME: There is a chunk of code here which belongs in Network.  It actually
// sits in Network::FilenameToCfgname but it's ifdef-ed out.  Remove this one
// and enable that one.
char *SaveCFGDialog::getDefaultFileName()
{
   const char *netname = this->network->getFileName();
   char *file = new char[STRLEN(netname) + 1];
   strcpy (file, netname);

    //
    // Remove the .net extension if present
    //
#define NetExtension ".net"
    const char *netext = strrstr(file, NetExtension);
    int netExtLen = STRLEN(NetExtension);
    if ((netext) && (STRLEN(netext) == netExtLen)) {
        int filelen = STRLEN(file);
        file[filelen-netExtLen] = '\0';
    }

   return Network::FilenameToCfgname(file);
}

