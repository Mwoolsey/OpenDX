/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _DragSource_h
#define _DragSource_h

#include <Xm/Xm.h>

#include "Base.h"
#include "TransferStyle.h"

class Dictionary;

//
// Class name definition:
//
#define ClassDragSource	"DragSource"

extern "C" void    DragSource_StartDrag  (Widget, XEvent*, String*, Cardinal*);
extern "C" Boolean DragSource_ConvertProc(Widget, Atom*, Atom*, Atom*, 
					  XtPointer*, unsigned long*, int*);
extern "C" void DragSource_DropFinishCB(Widget, XtPointer, XtPointer);
extern "C" void DragSource_TopLevelLeaveCB (Widget, XtPointer, XtPointer);
extern "C" void DragSource_TopLevelEnterCB (Widget, XtPointer, XtPointer);

//
// DragSource class definition:
//				
class DragSource : public Base 
{
  private:
    //
    // "Class" data
    static boolean 		DragSourceClassInitialized;
    static XtTranslations	drag_tranlations;
    static void*		TransferData;

    //
    // Private member data:
    //
    XContext		context;
    Widget		drag_icon;
    Widget		drag_context;
    Pixmap		drag_pixmap;
    Pixmap		drag_mask;
    int			icon_width;
    int			icon_height;
    long		operation;

    //
    // On behalf of pagetabs, enforce d-n-d inside a single top level window.
    // intra_toplevel == TRUE iff we want to enforce this behavior.
    // inside_own_shell is maintained during the drag operation.  It's set to TRUE
    // if the pointer is still inside the initiating top level shell.
    //
    Window		top_level_window;
    boolean		intra_toplevel;
    boolean		inside_own_shell;


    //
    // Private member functions:
    //
    friend void    DragSource_StartDrag  (Widget, XEvent*, String*, Cardinal*);
    friend Boolean DragSource_ConvertProc(Widget, Atom*, Atom*, Atom*, 
					  XtPointer*, unsigned long*, int*);
    friend void DragSource_DropFinishCB(Widget, XtPointer, XtPointer);
    friend void DragSource_TopLevelLeaveCB (Widget, XtPointer, XtPointer);
    friend void DragSource_TopLevelEnterCB (Widget, XtPointer, XtPointer);

    void startDrag(Widget, XEvent*);

  protected:

    //
    // Protected member data:
    //
    char		*converted_data;

    virtual void setDragWidget(Widget);
    virtual Widget createDragIcon(int width, int height, char *bits, char *maskbits);
    virtual void setDragIcon(Widget);
    virtual void dropFinish(long operation, int tag, unsigned char completion_status);

    //
    // This function is called prior to starting a drag.  
    // Abort aborts the drag and beeps,
    // Inactive aborts the drag, no beep.  Needed because it's easier to do
    // this than to reinstall translations in order to turn off a drag source.  The
    // book says that's the way to turn off a drag source.
    //
    enum {
	Proceed = 1,
	Abort = 2,
	Inactive = 3
    };
    virtual int decideToDrag(XEvent *){return DragSource::Proceed;};

    //
    // There was a Dictionary* up in private.  It's replaced with this call.
    // Subclasses want to enforce sharing a common dictionary among their members.
    //
    virtual Dictionary *getDragDictionary() = 0;

    //
    // Instead of passing around a stored value for this, make the subclasses
    // implement a proc which decodes according to an enum and then invokes a member.
    //
    virtual boolean decodeDragType (int, char *, XtPointer*, unsigned long*, long) = 0;

    void setIntraShellBehavior(boolean intra_shell) { 
	this->intra_toplevel = intra_shell; 
    }
    boolean isIntraShell();

  public:

    void addSupportedType (int, const char *, boolean);

    //
    // Constructor:
    //
    DragSource();
 
    //
    // Destructor:
    //
    ~DragSource(); 
  
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDragSource;
    }
};


#endif // _DragSource_h
