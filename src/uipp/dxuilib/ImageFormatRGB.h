/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _ImageFormatRGB_h
#define _ImageFormatRGB_h


#include "PixelImageFormat.h"

//
// Class name definition:
//
#define ClassImageFormatRGB	"ImageFormatRGB"

//
// SaveImageDialog class definition:
//				
class ImageFormatRGB : public PixelImageFormat
{
  private:

    static String  DefaultResources[];
    static boolean ClassInitialized;

  protected:

    virtual void	initialize();

  public:

    //
    // Constructor:
    //
    ImageFormatRGB(ImageFormatDialog *dialog);

    static ImageFormat* Allocator (ImageFormatDialog* dialog) 
	{ return  new ImageFormatRGB(dialog); }

    //
    // Destructor:
    //
    ~ImageFormatRGB();

    virtual const char*		paramString() { return "rgb"; }
    virtual const char*		menuString() { return "RGB"; }
    virtual const char*		fileExtension() { return ".rgb"; }
    virtual boolean 		supportsAppend() { return TRUE; }
    virtual boolean		supportsDelayedColors() { return FALSE; }
    virtual void		eraseOutputFile(const char *srcfile);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassImageFormatRGB; }
    virtual boolean isA(Symbol classname);
};


#endif // _ImageFormatRGB_h
