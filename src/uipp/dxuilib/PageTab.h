/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _PageTab_h
#define _PageTab_h

#include "NotebookTab.h"
#include "DragSource.h"
#include "DXDropSite.h"

class WorkSpace;
class PageGroupRecord;
class PageSelector;
class Network;
class EditorWindow;

extern "C" void PageTab_ColorTimerTO (XtPointer clientData, XtIntervalId* );

#define ClassPageTab "PageTab"

class PageTab: public NotebookTab, public DXDropSite, public DragSource {
  private:

    WorkSpace* 		workSpace;
    PageGroupRecord*	group_rec;
    char*		group_name;
    boolean		set;
    int			position;
    int			desired_position;
    boolean		has_desired_position;
    XtIntervalId	color_timer;
    PageSelector*	selector;
    Pixel		pending_fg;

    static String	DefaultResources[];
    static boolean	ClassInitialized;
    static Pixmap	TopShadowPixmap;
    static Pixmap	BottomShadowPixmap;
    static Pixmap	AnimationPixmap;
    static Pixmap	AnimationMaskPixmap;
    static boolean 	BrokenServer;

    //
    // data type dictionary for drops.
    //
    static Dictionary *DropTypeDictionary;
    static Dictionary *DragTypeDictionary;

    //
    // Supply the type dictionary which stores the types we can supply for a dnd
    //
    virtual Dictionary *getDropDictionary() { return PageTab::DropTypeDictionary; }
    virtual Dictionary *getDragDictionary() { return PageTab::DragTypeDictionary; }

  protected:

    friend  void PageTab_ColorTimerTO (XtPointer clientData, XtIntervalId* );

    static Widget 	DragIcon;
    // Define constants for the dnd data types we understand
    enum {
        PageTrash,
        PageName,
        Modules
    };


    //
    // Drag - n - Drop
    //
    virtual boolean decodeDropType (int , char *, XtPointer , unsigned long , int , int );
    virtual boolean decodeDragType (int, char *, XtPointer*, unsigned long*, long);

    virtual boolean mergeNetElements (Network*, List*, int, int);

    virtual void activate();
    virtual void multiClick();

  public:

    PageTab (PageSelector*, WorkSpace* ws, const char* button_text);
    ~PageTab();

    //
    // In order to {un}setDropWidget
    //
    virtual void manage();
    virtual void unmanage();

    virtual void 	createButton(Widget );
    void 	createButton(Widget, PageGroupRecord*);

    void setState(boolean set=TRUE);
    WorkSpace* getWorkSpace() { return this->workSpace; }
    const char* getGroupName() { return this->group_name; }
    void setGroup(PageGroupRecord* rec);
    void setPosition(int, boolean designated_by_user=FALSE);
    int  getPosition();
    void setColor(Pixel);
    int getDesiredPosition();
    boolean hasDesiredPosition() { return this->has_desired_position; }
    boolean getDesiredShowing();
    const char* getClassName() { return ClassPageTab; }

};

#endif // _PageTab_h
