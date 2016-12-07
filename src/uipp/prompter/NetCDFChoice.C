/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NetCDFChoice.h"
#include "GARApplication.h"

boolean NetCDFChoice::ClassInitialized = FALSE;
String NetCDFChoice::DefaultResources[] =
{
    NUL(char*)
};

void NetCDFChoice::initialize() 
{
    if (NetCDFChoice::ClassInitialized) return ;
    NetCDFChoice::ClassInitialized = TRUE;
    this->setDefaultResources
	(theApplication->getRootWidget(), TypeChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), ImportableChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), NetCDFChoice::DefaultResources);
}

