/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _Tab_h
#define _Tab_h


#include "UIComponent.h"
// #include "StandIn.h"


//
// Class name definition:
//
#define ClassTab	"Tab"

class StandIn;

//
// StandIn class definition:
//				
class Tab : public UIComponent
{

  private:

    // static Boolean ClassInitialized;
    // static String  DefaultResources[];

    StandIn	*standIn;

    Position	x;              /* x position                   */
    Position	y;              /* y position                   */

    Widget	lineWidget;	/* "Line" to tab that is connected */

  public:

    //
    // Constructor:
    //
    Tab(StandIn *standIn);

    //
    // Destructor:
    //
    ~Tab();

    //
    // Creates the tab widget 
    //
    void createTab(Widget parent, boolean createManaged = TRUE);

    //
    // Returns TRUE if the object has been initialized,
    //   i.e., the window root widget has been created.
    //
    boolean isInitialized()
    {
        return this->getRootWidget() != NUL(Widget);
    }

    //
    // Moves the Tab
    //
    void moveTabXY(Position x, Position y, boolean update);
    void moveTabX(Position x, boolean update);
    void moveTabY(Position y, boolean update);

    //
    // Return info about the Tab
    //
    int getTabX(){return this->x;};
    int getTabY(){return this->y;};
    Widget getLine(){return this->lineWidget;};

    //
    // Set the lineWidget
    //
    void setLine(Widget  lineWidget){this->lineWidget = lineWidget;};

    //
    // Set the background color of a tab
    //
    void setBackground(Pixel  background);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
        return ClassTab;
    }
};

#endif // _Tab_h
