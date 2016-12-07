/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SADialog_h
#define _SADialog_h


#include "SaveFileDialog.h"
//#include "MBMainWindow.h"

#include <Xm/Xm.h>

//
// Class name definition:
//
#define ClassSADialog	"SADialog"

class Dialog;
class Command;
class MBMainWindow;

//
// SADialog class definition:
//				
class SADialog : public SaveFileDialog
{
    static boolean ClassInitialized;

    MBMainWindow *mbmw;
    Command     *cmd;

  protected:
    static String  DefaultResources[];

    virtual void saveFile(const char *string); 

  public:
    //
    // Constructor:
    //
    SADialog(MBMainWindow *mbmw);

    //
    // Destructor:
    //
    ~SADialog(){}

    void setPostCommand(Command *cmd);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSADialog;
    }
};


#endif // _SADialog_h
