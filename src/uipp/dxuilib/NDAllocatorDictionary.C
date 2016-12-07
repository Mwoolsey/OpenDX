/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


//
// This is the dictionary of NodeDefinition allocating functions.
// It is a defaulting dictionary which means that if a name/allocator 
// pair is not found then a default allocator is used (if installed).
// The constructor is responsible for setting the intial state of this
// type of dictionary.  
//


#include "NDAllocatorDictionary.h"
#include "NodeDefinition.h"
#include "ImageDefinition.h"
#include "DisplayDefinition.h"
#include "ColormapDefinition.h"
#include "ComputeDefinition.h"
#include "SelectorDefinition.h"
#include "SelectorListDefinition.h"
#include "ScalarDefinition.h"
#include "ScalarListDefinition.h"
#include "VectorDefinition.h"
#include "VectorListDefinition.h"
#include "SequencerDefinition.h"
#include "TransmitterDefinition.h"
#include "ReceiverDefinition.h"
#include "MacroParameterDefinition.h"
#include "ProbeDefinition.h"
#include "PickDefinition.h"
#include "EchoDefinition.h"
#include "StreaklineDefinition.h"
#include "PrintDefinition.h"
#include "ValueDefinition.h"
#include "ValueListDefinition.h"
#include "FileSelectorDefinition.h"
#include "ToggleDefinition.h"
#include "ResetDefinition.h"
#include "DXLInputDefinition.h"
#include "DXLOutputDefinition.h"
#include "GlobalLocalDefinition.h"


//
// Declaring an instance, instead of generating one at startup with 'new'
// may make for faster startup.
//
// /* static */ NDAllocatorDictionary __theNDAllocatorDictionary;
NDAllocatorDictionary *theNDAllocatorDictionary = NULL; 

//
// These are the symbols for the names of the Get and Set modules.
//
Symbol NDAllocatorDictionary::GetNodeNameSymbol = 0;
Symbol NDAllocatorDictionary::SetNodeNameSymbol = 0;


//
// Constructor function to do one-time initializations for the
// dictionary that contains 'named' functions that create NodeDefinitions.
//
NDAllocatorDictionary::NDAllocatorDictionary() 
{
   
    this->pushDefaultAllocator(NodeDefinition::AllocateDefinition);
    //
    // Special module definitions; those that aren't based on the default
    // class.
    //
    this->addAllocator("Display", DisplayDefinition::AllocateDefinition);
    this->addAllocator("Image",   ImageDefinition::AllocateDefinition);

    this->addAllocator("Colormap", ColormapDefinition::AllocateDefinition);
    this->addAllocator("Sequencer", SequencerDefinition::AllocateDefinition);

    this->addAllocator("Compute", ComputeDefinition::AllocateDefinition);
    this->addAllocator("Echo",    EchoDefinition::AllocateDefinition);
    this->addAllocator("Print",   PrintDefinition::AllocateDefinition);

    this->addAllocator("Probe",     ProbeDefinition::AllocateDefinition);
    this->addAllocator("ProbeList", ProbeDefinition::AllocateDefinition);
    this->addAllocator("Pick",      PickDefinition::AllocateDefinition);
    this->addAllocator("Streakline", StreaklineDefinition::AllocateDefinition);

    //
    // Special interactor definitions.
    //
    this->addAllocator("Selector", 
		       SelectorDefinition::AllocateDefinition);
    this->addAllocator("Scalar", 
		       ScalarDefinition::AllocateDefinition);
    this->addAllocator("ScalarList", 
		       ScalarListDefinition::AllocateDefinition);

    this->addAllocator("Integer",
		       ScalarDefinition::AllocateDefinition);
    this->addAllocator("IntegerList",
		       ScalarListDefinition::AllocateDefinition);

    this->addAllocator("Vector",
		       VectorDefinition::AllocateDefinition);
    this->addAllocator("VectorList",
		       VectorListDefinition::AllocateDefinition);

    this->addAllocator("Value", ValueDefinition::AllocateDefinition);
    this->addAllocator("ValueList", ValueListDefinition::AllocateDefinition);

    this->addAllocator("String",ValueDefinition::AllocateDefinition);
    this->addAllocator("StringList", ValueListDefinition::AllocateDefinition);
    this->addAllocator("FileSelector",FileSelectorDefinition::AllocateDefinition);
    this->addAllocator("Toggle",ToggleDefinition::AllocateDefinition);
    this->addAllocator("Reset",ResetDefinition::AllocateDefinition);

    //
    // Misc. supporting nodes.
    //
    this->addAllocator("Transmitter", 
                       TransmitterDefinition::AllocateDefinition);
    this->addAllocator("Receiver", 
                       ReceiverDefinition::AllocateDefinition);

    this->addAllocator("Input", 
                       MacroParameterDefinition::AllocateDefinition);
    this->addAllocator("Output", 
                       MacroParameterDefinition::AllocateDefinition);
    this->addAllocator("DXLInput", 
                       DXLInputDefinition::AllocateDefinition);
    this->addAllocator("SelectorList", 
                       SelectorListDefinition::AllocateDefinition);
    this->addAllocator("DXLOutput", 
                       DXLOutputDefinition::AllocateDefinition);

    //
    // These were added to change Get into GetLocal and GetGlobal and
    // Set into SetGlobal and SetLocal. 
    //
    this->addAllocator("Get", GlobalLocalDefinition::AllocateDefinition);
    this->addAllocator("Set", GlobalLocalDefinition::AllocateDefinition);
    NDAllocatorDictionary::GetNodeNameSymbol =
            theSymbolManager->registerSymbol("Get");
    NDAllocatorDictionary::SetNodeNameSymbol =
            theSymbolManager->registerSymbol("Set");

}
//
// 
//
NDAllocatorDictionary::~NDAllocatorDictionary() 
{
    this->popDefaultAllocator();
    // The definitions added in the constructor are cleared/deleted by the
    // Dictionary destructor.
  // theNDAllocatorDictionary = NULL;
}
//
// Find the allocator associated with name, or use the default allocator.
// Call the allocator with no arguments, and return the result.
//
NodeDefinition *NDAllocatorDictionary::allocate(const char *name)
{
    NodeDefinition *i = NUL(NodeDefinition*);
    NDAllocator a = this->findAllocator(name);
    if (a != NUL(NDAllocator)) i = a();
    return i;
}

