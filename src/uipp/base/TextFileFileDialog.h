/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TextFileFileDialog_h
#define _TextFileFileDialog_h


#include <Xm/Xm.h>

#include "FileDialog.h"
#include "TextFile.h"

//
// Class name definition:
//
#define ClassTextFileFileDialog	"TextFileFileDialog"

class Dialog;
class Network;
class Command;
class TextFileDialog;
class TextFile;

//
// TextFileFileDialog class definition:
//				
class TextFileFileDialog : public FileDialog
{
    static boolean ClassInitialized;
    static String  DefaultResources[];


  protected:
    Network *network;
    TextFile *textFile;

    virtual void okFileWork(const char *filename);

    //
    // Constructor for derived classes only:
    //
    TextFileFileDialog(const char *name, TextFile *tf);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Constructor for instances of THIS class only:
    //
    TextFileFileDialog(TextFile *tf);


    //
    // Destructor:
    //
    ~TextFileFileDialog(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTextFileFileDialog;
    }
};


#endif // _TextFileFileDialog_h
