/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _ImageFormatGIF_h
#define _ImageFormatGIF_h


#include "PixelImageFormat.h"

//
// Class name definition:
//
#define ClassImageFormatGIF	"ImageFormatGIF"

//
// SaveImageDialog class definition:
//				
class ImageFormatGIF : public PixelImageFormat
{
  private:

    static String  DefaultResources[];
    static boolean ClassInitialized;

  protected:

    virtual void	initialize();

    virtual boolean		supportsPrinting() { return TRUE; }

  public:

    //
    // Constructor:
    //
    ImageFormatGIF(ImageFormatDialog *dialog);

    static ImageFormat* Allocator (ImageFormatDialog* dialog) 
	{ return  new ImageFormatGIF(dialog); }


    //
    // Destructor:
    //
    ~ImageFormatGIF();

    virtual const char*		paramString() { return "gif"; }
    virtual const char*		menuString() { return "GIF"; }
    virtual const char*		fileExtension() { return ".gif"; }
    virtual boolean		supportsDelayedColors() { return FALSE; }
    virtual boolean		requiresDelayedColors() { return TRUE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassImageFormatGIF; }
    virtual boolean isA(Symbol classname);
};


#endif // _ImageFormatGIF_h
