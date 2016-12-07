/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _TutorWindow_h
#define _TutorWindow_h

#include <Xm/Xm.h>
#include "HelpWin.h"

//
// Class name definition:
//
#define ClassTutorWindow	"TutorWindow"

class CommandScope;
class Command;
class Dictionary;

//
// TutorWindow class definition:
//				
class TutorWindow : public HelpWin
{

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
 
  protected:

    static String DefaultResources[];
    //
    // Menus & pulldowns:
    //

    Widget		fileMenu;

    Widget		fileMenuPulldown;

    //
    // File menu options:
    //

    //
    // This routine is called by initialize() function to create
    // the menus for the menubar in the window.
    //
    virtual void createMenus(Widget parent);
    virtual void createFileMenu(Widget parent);


  public:

    TutorWindow();

    //
    // Destructor:
    //
    ~TutorWindow();

    //
    // Sets the default resources before calling the superclass
    // initialize() function.
    //
    virtual void initialize();

    //
    // Any unmanage is an implied exit().
    //
    virtual void unmanage();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTutorWindow;
    }
};


#endif // _TutorWindow_h
