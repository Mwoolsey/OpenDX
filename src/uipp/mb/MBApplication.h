/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _MBApplication_h
#define _MBApplication_h

#include <Xm/Xm.h>

#include "MainWindow.h"
#include "IBMApplication.h"
#include "List.h"


//
// Class name definition:
//
#define ClassMBApplication	"MBApplication"


typedef struct
{
    Pixel	insensitiveColor;
} MBResource;

class MBApplication : public IBMApplication
{

  private:
    //
    // Private class data:
    //
    static boolean    MBApplicationClassInitialized; // class initialized?

    void shutdownApplication() {};

    MainWindow		*mainWindow;

    boolean		is_dirty;

  protected:
    //
    // Overrides the Application class version:
    //   Initializes Xt Intrinsics with option list (switches).
    //
    virtual boolean initialize(unsigned int* argcp,
			    char**        argv);

    static MBResource	resource;

  public:

    boolean isDirty(){return this->is_dirty;}
    void setDirty(){this->is_dirty = TRUE;}
    void setClean(){this->is_dirty = FALSE;}

    Pixel getInsensitiveColor()
    {
	return this->resource.insensitiveColor;
    }
    //
    // Constructor:
    //
    MBApplication(char* className);

    //
    // Destructor:
    //
    ~MBApplication();

    //
    // Return the name of the application (i.e. 'Data Explorer',
    // 'Data Prompter', 'Medical Visualizer'...).
    //
    virtual const char *getInformalName();

    //
    // Return the formal name of the application (i.e.
    // 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
    //
    virtual const char *getFormalName();

    //
    // Get the applications copyright notice, for example...
    // "Copyright International Business Machines Corporation 1991-1993
    // All rights reserved"
    //
    virtual const char *getCopyrightNotice();

    virtual const char *getHelpDirFileName();

    virtual const char *getStartTutorialCommandString();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
        return ClassMBApplication;
    }
};


extern MBApplication* theMBApplication;

#endif /*  _MBApplication_h */

