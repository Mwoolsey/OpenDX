/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "SpreadSheetChoice.h"
#include "DXChoice.h"
#include "GridChoice.h"
#include "CDFChoice.h"
#include "NetCDFChoice.h"
#include "HDFChoice.h"
#include "ImageChoice.h"

#include "Dictionary.h"

Dictionary* theTypeChoiceDictionary = NUL(Dictionary*);

void
BuildTheTypeChoiceDictionary()
{
    theTypeChoiceDictionary = new Dictionary;

    //
    // The first argument is a dictionary key.  Changes the key changes the order
    // of the entry in the dictionary and thus changes the appearance of the window
    // because the window is constructed by scanning the dictionary from top to 
    // bottom.
    //
    theTypeChoiceDictionary->addDefinition ("A", (void*)DXChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("B", (void*)CDFChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("C", (void*)NetCDFChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("D", (void*)HDFChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("E", (void*)ImageChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("F", (void*)GridChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("G", (void*)SpreadSheetChoice::Allocator);
#if 0
    theTypeChoiceDictionary->addDefinition ("Spreadsheet",  SpreadSheetChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("Grid", GridChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("NetCDF", NetCDFChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("CDF",  CDFChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("HDF", HDFChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("DX", DXChoice::Allocator);
    theTypeChoiceDictionary->addDefinition ("Image", ImageChoice::Allocator);
#endif
}
