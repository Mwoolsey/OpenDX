/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "ImageFormatPSGreyEnc.h"
#include "Application.h"

boolean ImageFormatPSGreyEnc::ClassInitialized = FALSE;

String ImageFormatPSGreyEnc::DefaultResources[] = {
    NUL(char*)
};


ImageFormatPSGreyEnc::ImageFormatPSGreyEnc (ImageFormatDialog* dialog) : 
    PostScriptImageFormat("PSGreyEncformat", dialog)
{

}

ImageFormatPSGreyEnc::~ImageFormatPSGreyEnc()
{
}


void ImageFormatPSGreyEnc::initialize()
{
    if (!ImageFormatPSGreyEnc::ClassInitialized) {
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    PostScriptImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormatPSGreyEnc::DefaultResources);
	ImageFormatPSGreyEnc::ClassInitialized = TRUE;
    }
}

boolean ImageFormatPSGreyEnc::isA (Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassImageFormatPSGreyEnc);
    if (s == classname)
        return TRUE;
    else
        return PostScriptImageFormat::isA(classname);
}

