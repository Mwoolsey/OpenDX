/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _CDBInput
#define _CDBInput


#include "DXStrings.h"
#include "CDBParameter.h"
#include "../base/TextPopup.h"


//
// Class name definition:
//
#define ClassCDBInput	"CDBInput"

class ConfigurationDialog;

//
// CDBInput class definition:
//				
class CDBInput : public CDBParameter 
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
#if 11
    TextPopup *valueTextPopup;
#else
    Widget  valueWidget;
#endif
    boolean valueChanged;
    boolean initialValueIsDefault;
    char   *initialValue;
    int	    modified;    

    //
    // Constructor:
    //
    CDBInput() : CDBParameter() 
    {
#if 00 
	this->valueWidget = NULL;
#endif
	this->valueChanged = FALSE;
	this->initialValue = NULL;
	this->initialValueIsDefault = FALSE;
	this->valueTextPopup = new TextPopup();
	this->modified = 0;
    }

    //
    // Destructor:
    //
    ~CDBInput()
    {
	if (this->initialValue != NULL)
	    delete initialValue;
	if (this->valueTextPopup)
	    delete this->valueTextPopup;
    }

    void setInitialValue(const char *s)
    {
	if (this->initialValue)
	    delete this->initialValue;
	if (s)
	    this->initialValue = DuplicateString(s);
	else
	    this->initialValue = NULL;
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCDBInput;
    }
};


#endif // _CDBInput
