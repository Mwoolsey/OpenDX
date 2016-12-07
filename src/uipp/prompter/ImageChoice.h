/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _IMAGECHOICE_H
#define _IMAGECHOICE_H

#include "NonimportableChoice.h"
#include "SymbolManager.h"

#define ClassImageChoice "ImageChoice"

extern "C" void ImageChoice_ParseCB(Widget, XtPointer, XtPointer);
extern "C" void ImageChoice_NumberCB(Widget, XtPointer, XtPointer);

class ImageChoice : public NonimportableChoice {
    private:
	static boolean	ClassInitialized;

	Widget format_om;

	//
	// Must be large enough to hold 1 widget for each supported image format.
	//
	Widget format_widget[10];

    protected:

	static String	DefaultResources[];
	static String   FormatNames[];
	static String   FormatTypes[];
	static String   FormatExt[];

	virtual Widget 	createBody (Widget parent, Widget top);
	virtual int	expandedHeight() 
	    { return (VERTICAL_LAYOUT?110:60); }

    public:

	ImageChoice (GARChooserWindow* gcw, Symbol sym);
	static ImageChoice* 
	    Allocator(GARChooserWindow* gcw, Symbol sym)
		{ return new ImageChoice (gcw, sym); }
	~ImageChoice();

	virtual void		initialize();
	virtual const char*	getFormalName() { return "Image file"; }
	virtual const char*	getInformalName() { return "Image"; }
	virtual const char*	getFileExtension() { return NUL(char*); }
	virtual boolean 	canHandle(const char*);
	virtual const char*	getActiveHelpMsg() { 
	    return "tiff, gif, or other image format.";
	}
	virtual boolean		visualize();
	virtual int             getMinWidth() { return (VERTICAL_LAYOUT?410:575); }
	virtual void		setCommandActivation(boolean file_checked=FALSE);


	const char *	getClassName() {
	    return ClassImageChoice;
	}

};

#endif  // _IMAGECHOICE_H

