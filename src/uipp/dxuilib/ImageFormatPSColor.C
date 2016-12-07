/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "ImageFormatPSColor.h"
#include "Application.h"

boolean ImageFormatPSColor::ClassInitialized = FALSE;

String ImageFormatPSColor::DefaultResources[] = {
    NUL(char*)
};


ImageFormatPSColor::ImageFormatPSColor (ImageFormatDialog* dialog) : 
    PostScriptImageFormat("PSColorformat", dialog)
{

}

ImageFormatPSColor::~ImageFormatPSColor()
{
}


void ImageFormatPSColor::initialize()
{
    if (!ImageFormatPSColor::ClassInitialized) {
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    PostScriptImageFormat::DefaultResources);
	this->setDefaultResources (theApplication->getRootWidget(),
	    ImageFormatPSColor::DefaultResources);
	ImageFormatPSColor::ClassInitialized = TRUE;
    }
}

boolean ImageFormatPSColor::isA (Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassImageFormatPSColor);
    if (s == classname)
	return TRUE;
    else
	return PostScriptImageFormat::isA(classname);
}
