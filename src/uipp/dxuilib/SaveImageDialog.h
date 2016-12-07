/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _SaveImageDialog_h
#define _SaveImageDialog_h


#include "ImageFormatDialog.h"

//
// Class name definition:
//
#define ClassSaveImageDialog	"SaveImageDialog"

extern "C" void SaveImageDialog_ModifyCB (Widget, XtPointer, XtPointer);

class Dialog;
class ImageNode;
class Command;
class ToggleButtonInterface;
class ButtonInterface;
class CommandScope;
class List;
class ImageFileDialog;

//
// SaveImageDialog class definition:
//				
class SaveImageDialog : public ImageFormatDialog
{
  private:

    static boolean ClassInitialized;
    static String  DefaultResources[];

    Widget			file_name;
    char*			file;
    ImageFileDialog*		fsb;
    int				sid_dirty;

  protected:

    ToggleButtonInterface*	saveCurrentOption;
    ToggleButtonInterface*	saveContinuousOption;
    ButtonInterface*		fileSelectOption;

    Command*			saveCurrentCmd;
    Command*			saveContinuousCmd;
    Command*			fileSelectCmd;

    virtual Widget 		createControls(Widget parent );

    virtual boolean 		okCallback(Dialog *);
    virtual void		restoreCallback();
    virtual int			getRequiredHeight() { return 190; }

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);


    enum {
	DirtyFilename   	= 64,
	DirtyCurrent 		= 128,
	DirtyContinuous 	= 256,
	DirtyRecordEnabled	= 512
    };

    friend void SaveImageDialog_ModifyCB (Widget, XtPointer, XtPointer);

  public:

    virtual void 	setCommandActivation();

    virtual const char*	getOutputFile();
    virtual boolean	isPrinting() { return FALSE; }
    virtual boolean             postFileSelectionDialog();
    void		setFilename(const char *file, boolean skip_callbacks=TRUE);

    boolean		dirtyCurrent();
    boolean		dirtyContinuous() 
	{ this->sid_dirty|= DirtyContinuous; return TRUE; }

    //
    // Constructor:
    //
    SaveImageDialog(Widget parent, ImageNode* node, CommandScope* commandScope);

    //
    // Destructor:
    //
    ~SaveImageDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassSaveImageDialog; }
};


#endif // _SaveImageDialog_h
