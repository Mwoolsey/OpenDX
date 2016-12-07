/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ImageCDB_h
#define _ImageCDB_h


#include "ConfigurationDialog.h"


//
// Class name definition:
//
#define ClassImageCDB	"ImageCDB"

//
// Referenced classes
class Node;

extern "C" void ImageCDB_ActivateOutputCacheCB(Widget , XtPointer , XtPointer);

//
// ImageCDB class definition:
//				
class ImageCDB : public ConfigurationDialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    friend void ImageCDB_ActivateOutputCacheCB(Widget , XtPointer , XtPointer);

  protected:

    virtual Widget createOutputs(Widget parent, Widget top);

    Widget imageCacheOM;
    Widget imageCacheLabel;
    Widget internalFullButton;
    Widget internalLastButton;
    Widget internalOffButton;

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    static ConfigurationDialog *AllocateConfigurationDialog(Widget parent,
							   Node *node);

    virtual void changeOutput (int );

    //
    // Constructor:
    //
    ImageCDB(Widget parent, Node *node);

    //
    // Destructor:
    //
    ~ImageCDB();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassImageCDB;
    }
};


#endif // _ImageCDB_h
