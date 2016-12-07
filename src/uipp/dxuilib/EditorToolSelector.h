/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _EditorToolSelector_h
#define _EditorToolSelector_h


#include "ToolSelector.h"
#include "DXApplication.h" // for RECENT_NETS and RECENT_MACROS

//
// Class name definition:
//
#define ClassEditorToolSelector	"EditorToolSelector"

class EditorWindow;
class Notebook;

//
// EditorToolSelector class definition:
//				
class EditorToolSelector : public ToolSelector
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    EditorWindow *editor;

    virtual void toolSelect(Symbol s);
    virtual Widget layoutWidgets(Widget parent);

    Notebook* notebook;

    virtual void adjustVisibility(int, int, int, int);

  public:
    //
    // Constructor:
    //
    EditorToolSelector(const char *name, EditorWindow *editor);

    //
    // Destructor:
    //
    ~EditorToolSelector();

    //
    // Unhighlight any selected tools in the tool list. 
    //
    virtual void deselectAllTools();
 
    //
    // is the object in the anchor window. At this level we know nothing
    // of windows.  EditorToolSelector has this knowledge.
    //
    virtual boolean inAnchor();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassEditorToolSelector;
    }
};


#endif // _EditorToolSelector_h
