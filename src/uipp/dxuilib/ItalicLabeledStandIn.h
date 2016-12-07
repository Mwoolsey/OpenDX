/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _ItalicLabeledStandIn_h
#define _ItalicLabeledStandIn_h


#include "LabeledStandIn.h"


//
// Class name definition:
//
#define ClassItalicLabeledStandIn	"ItalicLabeledStandIn"

//
// ItalicLabeledStandIn class definition:
//				

class ItalicLabeledStandIn : public LabeledStandIn
{
  private:
    static boolean ClassInitialized;

    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    static String DefaultResources[];
    virtual const char *getButtonLabelFont();

    //
    // Constructor:
    //
    ItalicLabeledStandIn(WorkSpace *w, Node *node);

  public:
    //
    // Allocator that is installed in theSIAllocatorDictionary
    //
    static StandIn *AllocateStandIn(WorkSpace *w, Node *n);

    //
    // Destructor:
    //
    ~ItalicLabeledStandIn();

    virtual void initialize();

    virtual const char *getPostScriptLabelFont();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassItalicLabeledStandIn;
    }
};


#endif // _ItalicLabeledStandIn_h
