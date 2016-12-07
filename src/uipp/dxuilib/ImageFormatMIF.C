/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "ImageFormatMIF.h"
#include "Application.h"

boolean ImageFormatMIF::ClassInitialized = FALSE;

String ImageFormatMIF::DefaultResources[] = {
    NUL(char*)
};


ImageFormatMIF::ImageFormatMIF (ImageFormatDialog* dialog) : 
    PixelImageFormat("MIFformat", dialog)
{

}

ImageFormatMIF::~ImageFormatMIF()
{
}


void ImageFormatMIF::initialize()
{
    if (!ImageFormatMIF::ClassInitialized) {
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    PixelImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormatMIF::DefaultResources);
	ImageFormatMIF::ClassInitialized = TRUE;
    }
}

boolean ImageFormatMIF::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassImageFormatMIF);
    if (s == classname)
	return TRUE;
    else
	return this->ImageFormat::isA(classname);
}

