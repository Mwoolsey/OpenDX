/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _ImageFormatTIF_h
#define _ImageFormatTIF_h


#include "PixelImageFormat.h"

//
// Class name definition:
//
#define ClassImageFormatTIF	"ImageFormatTIF"

//
// SaveImageDialog class definition:
//				
class ImageFormatTIF : public PixelImageFormat
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
    ImageFormatTIF(ImageFormatDialog *dialog);

    static ImageFormat* Allocator (ImageFormatDialog* dialog) 
	{ return  new ImageFormatTIF(dialog); }


    //
    // Destructor:
    //
    ~ImageFormatTIF();

    virtual const char*		paramString() { return "tiff"; }
    virtual const char*		menuString() { return "TIFF"; }
    virtual const char*		fileExtension() { return ".tiff"; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassImageFormatTIF; }
    virtual boolean isA(Symbol classname);
};


#endif // _ImageFormatTIF_h
