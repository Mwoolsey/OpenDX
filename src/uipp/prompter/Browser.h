/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _Browser_h
#define _Browser_h

#if defined(HAVE_FSTREAM)
#include <fstream>
#elif defined(HAVE_FSTREAM_H)
#include <fstream.h>
#else /* !HAVE_FSTREAM && !HAVE_FSTREAM_H */
#error "no fstream and no fstream.h"
#endif /* !HAVE_FSTREAM && !HAVE_FSTREAM_H */

#include <Xm/Xm.h>
#include "../base/IBMMainWindow.h"
#include "../base/CommandInterface.h"
#include "BrowserCommand.h"
#include "SearchDialog.h"

//
// Class name definition:
//
#define ClassBrowser	"Browser"

extern "C" void Browser_HelpCB(Widget w, XtPointer, XtPointer);

//
// Browser class definition:
//				
class Browser : public IBMMainWindow
{

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    //
    // Source file stream
    //
    std::ifstream *from;
    unsigned long file_size;
    int page_size;

    SearchDialog *searchDialog;

    static void AddHelpCallbacks(Widget);

    //
    // Creates the editor window workarea (as required by MainWindow class).
    //
    virtual Widget createWorkArea(Widget parent);

    Widget position_label;
    Widget text;
    Widget vsb;
    Widget byte_top_offset;
    Widget byte_start_offset;
    Widget byte_mark_offset;
    Widget line_top_offset;
    Widget line_mark_offset;

    unsigned long page_start;
    unsigned long page_start_line_no;

    unsigned long marker_pos;
    unsigned long marker_line_no;
    char marker_char[2];

    Boolean ignore_cb;

    void gotoByte(unsigned long offset);
    void gotoLine(unsigned long line_no);
    void updateOffsets(unsigned long new_pos);
    void loadBuffer(unsigned long file_offset);
    boolean prevPage();
    boolean nextPage();
    void adjustScrollBar(Boolean top);
    static void integerMVCB(Widget w, XtPointer clientData, XtPointer);
    static void motionCB(Widget w, XtPointer clientData, XtPointer);
    static void pageIncCB(Widget w, XtPointer clientData, XtPointer);
    static void pageDecCB(Widget w, XtPointer clientData, XtPointer);
    static void pageControlCB(Widget w, XtPointer clientData, XtPointer);
    static void output_formatCB(Widget w, XtPointer clientData, XtPointer);
    static void gotoLineTopOffsetCB(Widget, XtPointer, XtPointer);
    static void gotoByteTopOffsetCB(Widget, XtPointer, XtPointer);
    static void gotoByteStartOffsetCB(Widget, XtPointer, XtPointer);
    static void gotoLineMarkOffsetCB(Widget, XtPointer, XtPointer);
    static void gotoByteMarkOffsetCB(Widget, XtPointer, XtPointer);

    void createFileMenu(Widget parent);
    void createMarkMenu(Widget parent);
    void createPageMenu(Widget parent);
    void createSearchMenu(Widget parent);
    void createHelpMenu(Widget parent);


  protected:

    //
    // Menus & pulldowns:
    //
    Widget              fileMenu;
    Widget              markMenu;
    Widget              pageMenu;
    Widget              searchMenu;

    Widget              fileMenuPulldown;
    Widget              markMenuPulldown;
    Widget              pageMenuPulldown;
    Widget              searchMenuPulldown;

    //
    // File menu options:
    //
    CommandInterface*   placeMarkOption;
    CommandInterface*   clearMarkOption;
    CommandInterface*   gotoMarkOption;
    CommandInterface*   firstPageOption;
    CommandInterface*   lastPageOption;
    CommandInterface*   searchOption;
    CommandInterface*   closeOption;

    BrowserCommand	*placeMarkCmd;
    BrowserCommand	*clearMarkCmd;
    BrowserCommand	*gotoMarkCmd;
    BrowserCommand	*firstPageCmd;
    BrowserCommand	*lastPageCmd;
    BrowserCommand	*searchCmd;
    BrowserCommand	*closeCmd;


    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);
    friend void Browser_HelpCB(Widget w, XtPointer, XtPointer);

  public:

    //
    // Flags:
    //
    enum {
	    CLOSE,
	    FIRST_PAGE,
	    LAST_PAGE,
	    PLACE_MARK,
	    CLEAR_MARK,
	    GOTO_MARK,
	    SEARCH
    };
    boolean openFile(char *filename);
    void gotoMark();
    void placeMark();
    void clearMark();
    void firstPage();
    void lastPage();
    void searchForward(char *);
    void searchBackward(char *);

    void postSearchDialog();
    void unpostSearchDialog();

    //
    // Creates the editor window menus (as required by MainWindow class).
    //
    virtual void createMenus(Widget parent);

    //
    // Sets the default resources before calling the superclass
    // initialize() function.
    //
    virtual void initialize();

    //
    // Constructor:
    //
    Browser(MainWindow *);

    //
    // Destructor:
    //
    ~Browser();

    //
    // called by MainWindow::CloseCallback().
    //
    void closeWindow();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassBrowser;
    }
};


#endif // _Browser_h

