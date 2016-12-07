/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "HDFChoice.h"
#include "GARApplication.h"


boolean HDFChoice::ClassInitialized = FALSE;
String HDFChoice::DefaultResources[] =
{
    NUL(char*)
};


void HDFChoice::initialize() 
{
    if (HDFChoice::ClassInitialized) return ;
    HDFChoice::ClassInitialized = TRUE;
    this->setDefaultResources
	(theApplication->getRootWidget(), TypeChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), ImportableChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), HDFChoice::DefaultResources);
}



