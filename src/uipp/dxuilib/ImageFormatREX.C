/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ImageFormatREX.h"
#include "Application.h"
#include "DXStrings.h"
#if defined(DXD_WIN) || defined(OS2)
#define unlink _unlink
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif


boolean ImageFormatREX::ClassInitialized = FALSE;

String ImageFormatREX::DefaultResources[] = {
    NUL(char*)
};


ImageFormatREX::ImageFormatREX (ImageFormatDialog* dialog) : 
    PixelImageFormat("REXformat", dialog)
{

}

ImageFormatREX::~ImageFormatREX()
{
}


void ImageFormatREX::initialize()
{
    if (!ImageFormatREX::ClassInitialized) {
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    PixelImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormatREX::DefaultResources);
	ImageFormatREX::ClassInitialized = TRUE;
    }
}


void ImageFormatREX::eraseOutputFile(const char *fname)
{
    char *cp;
    char *srcfile = DuplicateString(fname);
    char *file_to_delete = new char[strlen(fname) + 8];
    if ( (cp = strstr (srcfile, ".r")) ) cp[0] = '\0';
    sprintf (file_to_delete, "%s.r", srcfile);
    unlink (file_to_delete);
    sprintf (file_to_delete, "%s.g", srcfile);
    unlink (file_to_delete);
    sprintf (file_to_delete, "%s.b", srcfile);
    unlink (file_to_delete);
    sprintf (file_to_delete, "%s.size", srcfile);
    unlink (file_to_delete);

    delete file_to_delete;
    delete srcfile;
}

boolean ImageFormatREX::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassImageFormatREX);
    if (s == classname)
	return TRUE;
    else
	return this->ImageFormat::isA(classname);
}

