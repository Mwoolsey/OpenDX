/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _CDBOutput
#define _CDBOutput


#include "Cacheability.h"
#include "CDBParameter.h"


//
// Class name definition:
//
#define ClassCDBOutput	"CDBOutput"

class ConfigurationDialog;

//
// CDBOutput class definition:
//				
class CDBOutput : public CDBParameter 
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //


  public:
    Cacheability initialCache;
    //
    // Constructor:
    //
    CDBOutput()
    {
	this->cacheWidget = NULL;
	this->cachePulldown = NULL;
	this->fullButton = NULL;
	this->lastButton = NULL;
	this->offButton = NULL;
    }

    //
    // Destructor:
    //
    ~CDBOutput()
    {
    }

    Widget  cacheWidget;
    Widget  cachePulldown;
    Widget  fullButton;
    Widget  lastButton;
    Widget  offButton;

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCDBOutput;
    }
};


#endif // _CDBOutput
