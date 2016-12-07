/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _DXDragSource_h
#define _DXDragSource_h

#include <X11/Intrinsic.h>
#include "DragSource.h"

#include "enums.h"

//
// Class name definition:
//
#define ClassDXDragSource "DXDragSource"
class Network;

#define DXINTERACTORS "DXINTERACTORS"
#define DXTRASH "DXTRASH"
#define DXMODULES "DXMODULES"
#define DXINTERACTOR_NODES "DXINTERACTOR_NODES"

//
// A special DragSource.  Invoke a virtual function to produce 2 files
// (a .net, and a .cfg) and a header string.  Take those 3 pieces and
// ship them to the X server as the Drag data.  The "convert" proc was
// virtual in DragSource, but now the buck stops here, since anyone who
// inherits from me wants the special service my convert proc provides.
//
class DXDragSource : public DragSource
{

  private:
	PrintType printType;
	static boolean DXDragSourceClassInitialized;

  protected:
	//
	// invoke createNetFiles(), and then grab the bytes and ship
	// them.  This operation would have to be duplicated by
	// any WorkSpace which wanted to ship .net,.cfg across.
	//
	boolean convert (Network *, char *, XtPointer*, unsigned long*, long);

	//
	// Create the .net and .cfg files and the header string.
	// return the names of the files.
	//
	virtual boolean createNetFiles (Network *net, FILE *netf, char *cfgFile);

  public:
	// The initialize method grabs a chunk of memory for the
	// transfers.  Always reuse the same peice of memory.  This
	// avoids leaks.
	virtual void initialize();

	void setPrintType (PrintType printType) { this->printType = printType; }

	//
	// Constructor
	// PrintCut means a dnd operation in the vpe.  As of 2/95, this class
	// was used by EditorWorkSpace and ControlPanelWorkSpace. The later
	// wants to set PrintType to PrinCPBuffer - an enum recognized by
	// various members of Network.
	//
	DXDragSource (PrintType = PrintCut);

	//
	// Destructor
	//
	~DXDragSource ();

	const char* getClassName() { return ClassDXDragSource; }
};


#endif // _DXDragSource_h
