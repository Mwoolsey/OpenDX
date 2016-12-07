/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _PixelImageFormat_h
#define _PixelImageFormat_h


#include "ImageFormat.h"

//
// Class name definition:
//
#define ClassPixelImageFormat	"PixelImageFormat"

extern "C" void PixelImageFormat_SizeTO (XtPointer, XtIntervalId*);
extern "C" void PixelImageFormat_ModifyCB (Widget, XtPointer, XtPointer);
extern "C" void PixelImageFormat_ParseSizeCB (Widget, XtPointer, XtPointer);
extern "C" void PixelImageFormat_ParseSizeEH (Widget, XtPointer, XEvent*, Boolean*);

//
// SaveImageDialog class definition:
//				
class PixelImageFormat : public ImageFormat
{
  private:

    static boolean 	ClassInitialized;
    int			dirty;
    boolean		use_nodes_resolution;
    boolean		use_nodes_aspect;
    char*		size_val;
    Widget		size_text;
    XtIntervalId 	size_timer;

  protected:

    static String  DefaultResources[];

    void			parseImageSize(const char*);

    virtual Widget 		createBody(Widget parent);
    virtual void 		setCommandActivation();
    virtual void		shareSettings (ImageFormat*);
    virtual boolean		supportsPrinting() { return FALSE; }

    //
    // Constructor:
    //
    PixelImageFormat(const char *name, ImageFormatDialog* dialog);

    enum {
	DirtyResolution		= 64,
	DirtyAspect		= 128
    };


    friend void PixelImageFormat_SizeTO (XtPointer, XtIntervalId*);
    friend void PixelImageFormat_ModifyCB (Widget, XtPointer, XtPointer);
    friend void PixelImageFormat_ParseSizeCB (Widget, XtPointer, XtPointer);
    friend void PixelImageFormat_ParseSizeEH (Widget, XtPointer, XEvent*, Boolean*);

  public:
    //
    // Destructor:
    //
    ~PixelImageFormat();

    virtual int			getRecordResolution() { return this->width; }
    virtual double		getRecordAspect() { return this->aspect; }
    virtual boolean		useLocalResolution();
    virtual boolean		useLocalAspect();
    virtual int			getRequiredHeight() { return 45; }
    virtual void		applyValues() { this->dirty = 0; }

    const char* getClassName() { return ClassPixelImageFormat; }
    virtual boolean isA(Symbol classname);
};


#endif // _PixelImageFormat_h
