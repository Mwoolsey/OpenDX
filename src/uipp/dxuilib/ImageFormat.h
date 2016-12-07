/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ImageFormat_h
#define _ImageFormat_h


#include "UIComponent.h"
#include "SymbolManager.h"

//
// Class name definition:
//
#define ClassImageFormat	"ImageFormat"

//
// This is a default value which must match something on the exec side.
// It's in the WriteImage module which belongs to Frank Suits.
//
#define DEFAULT_GAMMA 2.0

class ImageNode;
class ImageFormatDialog;
class Command;
class ImageFormat;

typedef ImageFormat* (*ImageFormatAllocator)(ImageFormatDialog* );

//
// SaveImageDialog class definition:
//				
class ImageFormat : public UIComponent
{
  private:

    Widget		menuButton;

  protected:

    static String  DefaultResources[];

    int				width;
    double			aspect;
    ImageFormatDialog*		dialog;

    virtual void		initialize(){};

    boolean isMetric();

    void			setTextSensitive (Widget , boolean );

    //
    // Constructor:
    //
    ImageFormat(const char *name, ImageFormatDialog* dialog);

  public:

    //
    // Destructor:
    //
    ~ImageFormat();

    virtual void 		setCommandActivation() = 0;
    virtual void		shareSettings(ImageFormat*) = 0;
    virtual const char* 	paramString() = 0;
    virtual const char* 	menuString() = 0;
    virtual const char* 	fileExtension() = 0;
    virtual boolean		supportsAppend() { return FALSE; }
    virtual boolean		supportsPrinting() = 0;
    virtual boolean		useLocalResolution() = 0;
    virtual boolean		useLocalAspect() = 0;
    virtual boolean		useLocalFormat();
    virtual void		changeUnits(){};
    virtual void		applyValues(){};
    virtual void		restore(){};
    virtual boolean		supportsDelayedColors() { return FALSE; }
    virtual boolean		requiresDelayedColors() { return FALSE; }
    virtual void		eraseOutputFile(const char *fname);

    virtual const char*		getRecordFormat();
    virtual int			getRecordResolution() = 0;
    virtual double		getRecordAspect() = 0;
    virtual void		parseRecordFormat(const char *){}
    virtual Widget 		createBody(Widget) = 0;
    virtual int			getRequiredHeight() = 0;

    void			setMenuButton(Widget button) { this->menuButton = button;}
    Widget			getMenuButton() { return this->menuButton;}

    const char* getClassName() { return ClassImageFormat; }
    virtual boolean isA(Symbol classname);
};


#endif // _ImageFormat_h
