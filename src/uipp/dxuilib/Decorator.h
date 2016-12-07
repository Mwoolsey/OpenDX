/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// 
//
//

#ifndef _Decorator_h
#define _Decorator_h

#include <X11/Intrinsic.h>

#include "WorkSpaceComponent.h"
#include "DynamicResource.h"
#include "DXDragSource.h"

class DecoratorStyle;
class Network;
class DecoratorInfo;
class Dialog;

#define ClassDecorator	"Decorator"
#define DEFAULT_SETTING "(default)"


class Decorator : public WorkSpaceComponent, public DXDragSource
{
  friend class DecoratorStyle;

  // P R I V A T E   P R I V A T E   P R I V A T E
  // P R I V A T E   P R I V A T E   P R I V A T E
  private:
    WidgetClass	widgetClass;

    // 
    // When the decorator is in a c.p. there is a requirement for it to know its
    // ControlPanel pointer (because dnd requires printing which requires ownerCp
    // inside Network::cfgPrint).  But it would be too wierd to put a ControlPanel*
    // in here because sometimes we live in the vpe.
    // 
    DecoratorInfo* dndInfo;

    static 	boolean 	 DecoratorClassInitialized;
    static      const char *	 ColorNames[];
    static      const char *	 ColorValues[];

  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  protected:
    // S H A R E W A R E
    static  String 	   DefaultResources[]; 
    static  int		   HiLites;

    virtual void 	   createDecorator();
    Dimension 	x,y,width, height;
    DecoratorStyle *style;

    // add late breaking news - just like interactors, but not required
    virtual void completeDecorativePart() {};


    // D R A G - N - D R O P   R E L A T E D   S T U F F
    // D R A G - N - D R O P   R E L A T E D   S T U F F
    static Widget DragIcon;
 
    // 1 enum for each type of Drag data we can supply.  Pass these to addSupportedType
    // and decode them in decodeDragType.  These replace the use of func pointers.
    // The dictionary lives in the subclasses.
    enum {
        Modules,
        Interactors,
        Trash,
	Text
    };

    virtual int decideToDrag(XEvent *);
    virtual void    dropFinish (long, int, unsigned char);
    //
    // Providing these 2 means that subclasses don't have to provide dnd
    // functionality.  This makes sense for placing arbitrary widgets 
    // which may/may not provide a hunk of data.
    //
    virtual boolean decodeDragType (int, char *, XtPointer *, unsigned long *, long )
	{ return FALSE; }
    virtual Dictionary* getDragDictionary() { return NUL(Dictionary*); }
 
    //
    // Constructor
    //
    Decorator(void *wClass, const char * name, boolean developerStyle = TRUE);

    //
    // When decorators are in the vpe we always want them to get a new size when
    // something changes, but in panels, it seems better to maintain dimensions.
    // Inside panels, users try to line things up and make widths of several things
    // be equal, so I don't want to keep changing widths for them.  That's never
    // the case in the vpe.
    //
    virtual boolean resizeOnUpdate() { return FALSE; }

    //
    // this is for java support. It chops strings up into their individual
    // lines.  It's used in LabelDecorator and it's a copy of code from
    // InteractorInstance.  
    // FIXME: It needs to be pushed up the heirarchy.
    //
    static int CountLines(const char*);
    static const char* FetchLine(const char*, int);

  // P U B L I C   P U B L I C   P U B L I C
  // P U B L I C   P U B L I C   P U B L I C
  public:
    virtual   void	     openDefaultWindow();
    virtual   Dialog*	     getDialog() { return NUL(Dialog*); }
    virtual   void	     setAppearance (boolean developerStyle);
	      void           setStyle (DecoratorStyle *ds) {this->style = ds; }
	      DecoratorStyle *getStyle () { return this->style; }
    virtual   boolean	     hasDefaultWindow() { return FALSE; }
    virtual   void	     uncreateDecorator();
    virtual   void	     setSelected (boolean state);


    // G E O M E T R Y   G E O M E T R Y   G E O M E T R Y   
    // G E O M E T R Y   G E O M E T R Y   G E O M E T R Y   
    virtual   void	     setXYPosition (int x, int y);
    virtual   void	     setXYSize (int w, int h);
    virtual   void	     getXYPosition (int *x, int *y);
    virtual   void	     getXYSize (int *x, int *y);

    // D Y N A M I C   R E S O U R C E S
    // D Y N A M I C   R E S O U R C E S
    virtual boolean parseResourceComment (const char *comment,
    const   char    *filename, int line);
    virtual void    setResource (const char *, const char *);
    const char **getSupportedColorNames() { return Decorator::ColorNames; }
    const char **getSupportedColorValues() { return Decorator::ColorValues; }

    // C O N T R O L   P A N E L   C O M M E N T   F U N C T I O N S
    // C O N T R O L   P A N E L   C O M M E N T   F U N C T I O N S
    virtual   boolean	     printComment (FILE *f);
    virtual   boolean	     parseComment (const char *comment, 
					      const char *filename, int line);

    virtual   Network*       getNetwork();
              void           setDecoratorInfo(DecoratorInfo *); 
              DecoratorInfo* getDecoratorInfo() { return this->dndInfo; }
              boolean        createNetFiles (Network*, FILE*, char *);

    // J A V A     J A V A     J A V A     J A V A     J A V A
    // J A V A     J A V A     J A V A     J A V A     J A V A
    virtual boolean printAsJava (FILE*, const char*, int) { return TRUE; }
    virtual boolean printJavaResources (FILE*, const char*, const char*);


    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    virtual   void manage() { this->WorkSpaceComponent::manage(); };
    virtual   void manage(WorkSpace *workSpace);
    ~Decorator(); 
    const    char* getClassName() { return ClassDecorator; }
    virtual  boolean isA(Symbol classname);
};


#endif // _Decorator_h
