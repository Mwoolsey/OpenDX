/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _ImageFormatIM_h
#define _ImageFormatIM_h


#include "PixelImageFormat.h"

//
// Class name definition:
//
#define ClassImageFormatIM	"ImageFormatIM"

//
// SaveImageDialog class definition:
//				
class ImageFormatIM : public PixelImageFormat
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
    ImageFormatIM(ImageFormatDialog *dialog);

    static ImageFormat* Allocator (ImageFormatDialog* dialog) 
	{ return  new ImageFormatIM(dialog); }


    //
    // Destructor:
    //
    ~ImageFormatIM();

    virtual const char*		paramString() { return "ImageMagick supported format"; }
    virtual const char*		menuString() { return "ImageMagick supported format"; }
    virtual const char*		fileExtension() { return ""; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassImageFormatIM; }
    virtual boolean isA(Symbol classname);
};


#endif // _ImageFormatIM_h
