/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "ImageFormatRGB.h"
#include "ImageFormatREX.h"
#include "ImageFormatTIF.h"
#include "ImageFormatMIF.h"
#ifdef HAVE_LIBMAGICK
#include "ImageFormatGIF.h"
#include "ImageFormatIM.h"
#endif /* def HAVE_LIBMAGICK */
#include "ImageFormatYUV.h"
#include "ImageFormatPSGrey.h"
#include "ImageFormatPSGreyEnc.h"
#include "ImageFormatPSColor.h"
#include "ImageFormatPSColorEnc.h"

#include "Dictionary.h"

Dictionary* theImageFormatDictionary = NUL(Dictionary*);

void
BuildTheImageFormatDictionary()
{
char keystr[2];

    Dictionary* dict = theImageFormatDictionary = new Dictionary;

    //
    // It doesn't matter what you use for the key string for the dictionary insertion.
    // It's only important that the key string lets you put the things into the 
    // dictionary in the order you want.  We don't need to look things up in the 
    // dictionary.  In fact it could have been just a list instead.
    //
    strcpy (keystr, "A");
    dict->addDefinition (keystr, (void*)ImageFormatRGB::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatREX::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatTIF::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatMIF::Allocator); keystr[0]++;
#ifdef HAVE_LIBMAGICK
//    dict->addDefinition (keystr, (void*)ImageFormatGIF::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatIM::Allocator); keystr[0]++;
#endif /* def HAVE_LIBMAGICK */
    dict->addDefinition (keystr, (void*)ImageFormatYUV::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatPSColor::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatPSColorEnc::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatPSGrey::Allocator); keystr[0]++;
    dict->addDefinition (keystr, (void*)ImageFormatPSGreyEnc::Allocator); keystr[0]++;
}
