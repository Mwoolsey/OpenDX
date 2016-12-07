/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _ImageFormatPSColorEnc_h
#define _ImageFormatPSColorEnc_h


#include "PostScriptImageFormat.h"

//
// Class name definition:
//
#define ClassImageFormatPSColorEnc	"ImageFormatPSColorEnc"

//
// SaveImageDialog class definition:
//				
class ImageFormatPSColorEnc : public PostScriptImageFormat
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
    ImageFormatPSColorEnc(ImageFormatDialog *dialog);

    static ImageFormat* Allocator (ImageFormatDialog* dialog) 
	{ return  new ImageFormatPSColorEnc(dialog); }

    //
    // Destructor:
    //
    ~ImageFormatPSColorEnc();

    virtual const char*		paramString() { return "eps color"; }
    virtual const char*		menuString() { return "Color PostScript (enc)"; }
    virtual const char*		fileExtension() { return ".epsf"; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassImageFormatPSColorEnc; }
    virtual boolean isA(Symbol classname);
};


#endif // _ImageFormatPSColorEnc_h
