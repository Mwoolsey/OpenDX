/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _IBMApplication_h
#define _IBMApplication_h


#include <Xm/Xm.h>
#include "Application.h"

//
// Class name definition:
//
#define ClassIBMApplication	"IBMApplication"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//

extern "C" void IBMApplication_IBMButtonHelpAP(Widget, XEvent*, String*, Cardinal*);
extern "C" void IBMApplication_ColorProc(XColor*, XColor*, XColor*, XColor*, XColor*);
extern "C" Boolean IBMApplication_String2Pixel (Display *d, XrmValue args[], Cardinal nargs,
        XrmValue *from, XrmValue *to, XtPointer *closure);


class Command;
class HelpWin;

typedef struct
{
    //
    // options and resources:
    //
    String	UIRoot;

    Boolean	wizard;		// True is we're opening wizard windows by default
    String	noWizardNames;	// list of windows for which we want no wizard
} IBMResource;

//
// IBMApplication class definition:
//				
class IBMApplication : public Application 
{
  private:
    //
    // Private class data:
    //
    friend void IBMApplication_IBMButtonHelpAP(Widget, XEvent*, String*, Cardinal*);
    void initLogo();		// Creates the Logo pixmap
    void initIcon();		// Creates the Icon pixmap
    Pixmap logo_pmap;		// Logo pixmap;
    Pixmap icon_pmap;		// Icon pixmap;
    int    num_colors;		// The number of allocated colors
    char   *aboutAppString;  // The text in the aboutApp dialog.

    friend Boolean IBMApplication_String2Pixel (Display *d, XrmValue args[], Cardinal nargs,
        XrmValue *from, XrmValue *to, XtPointer *closure);
    static XmColorProc   DefColorProc;
    friend void IBMApplication_ColorProc(XColor *bg_color, XColor *fg_color,
			    XColor *sel_color, XColor *ts_color, XColor *bs_color);

  protected:
    static IBMResource resource; // resources and options

    //
    // Initialize the window system.
    //
    virtual boolean initializeWindowSystem(unsigned int *argcp, char **argv);

    //
    // Protected member data:
    //
    static const String	DefaultResources[];
    static XtActionsRec actions[];


    HelpWin		*helpWindow;

    boolean initialize(unsigned int* argcp, char** argv);

    //
    // Load application specific action routines
    //
    virtual void addActions();


    virtual HelpWin *newHelpWindow();

    //
    // Handle Xt Warnings (called by Application_XtWarningHandler)
    // Handle X Errors (called by XErrorHandler, static, above)
    //
    virtual void handleXtWarning(char *message);

    //
    // Constructor for the subclasses:
    //
    IBMApplication(char* className);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);


    //
    // W I Z A R D S    W I Z A R D S    W I Z A R D S  
    // W I Z A R D S    W I Z A R D S    W I Z A R D S  
    //
    List*       noWizards;
    void 	parseNoWizardNames();
    void 	printNoWizardNames();

    //
    // prevent leakage of the tech-support text
    //
    char* techSupportString;

  public:

    ~IBMApplication();

    Command		*helpOnContextCmd;
    Command		*genericHelpCmd;
    Command		*helpTutorialCmd;

    const char *getUIRoot()
    {
	return this->resource.UIRoot;
    }

    //
    // Displays any help required.
    //
    virtual void helpOn(const char *topic);

    //
    // Start the tutorial on behalf of the application.
    //
    virtual boolean startTutorial();

    //
    // Get the command string that will start the tutorial.
    //
    virtual const char *getStartTutorialCommandString();

    //
    // Get the Logo pixmap
    //
    virtual Pixmap getLogoPixmap(boolean create_if_necessary=FALSE);
    virtual void cleanupLogo();

    //
    // Get the Icon pixmap
    //
    virtual Pixmap getIconPixmap(){return this->icon_pmap;};

    //
    // Get the version string - among other things, this is used as the 
    // the string that is place in the About dialog that pops up when
    // 'Help on Version' is requested.
    //
    virtual void getVersionNumbers(int *maj, int *min, int *mic); 
    const char *getAboutAppString(); 
    const char *getTechSupportString();

    //
    // Get the name of the directoy that contains the help files.
    // This returns $UIRoot/help
    //
    const char *getHelpDirectory();
    const char *getHTMLDirectory();

    //
    // If there is a copyright notice, we create the Logo and Icon data 
    // structures and then call the super class method,  
    // otherwise, we just return.
    //
    virtual void postCopyrightNotice();

    //
    // W I Z A R D S     W I Z A R D S     W I Z A R D S
    // W I Z A R D S     W I Z A R D S     W I Z A R D S
    // inWizardMode() is virtual because GAR wants to default to TRUE instead of
    // FALSE.  The command line arg has the ability only to turn on the value.
    //
    virtual 	boolean inWizardMode() { return this->resource.wizard; }
    boolean 	isWizardWindow(const char* wiz_name);
    void 	appendNoWizardName(const char* nowiz_name);

    //
    // print a application default to the resource file.  For example:
    // DX.width: 300x500 into $HOME/DX
    //
    void printResource (const char* resource, const char* value);
    virtual boolean getApplicationDefaultsFileName(char* res_file, boolean create=FALSE);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassIBMApplication;
    }

    //	Get Temp. directory
    const char *getTmpDirectory(boolean bList = FALSE);
};

extern IBMApplication *theIBMApplication;

#endif // _IBMApplication_h
