/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _DX_CHOICE_H
#define _DX_CHOICE_H

#include "ImportableChoice.h"
#include "SymbolManager.h"

#define ClassDXChoice "DXChoice"

class ToggleButtonInterface;

class DXChoice : public ImportableChoice {
    private:
	static boolean	ClassInitialized;

    protected:

	static String	DefaultResources[];

    public:

	DXChoice (GARChooserWindow* gcw, Symbol sym) : 
	    ImportableChoice ("dxData", FALSE, TRUE, TRUE, FALSE, gcw, sym) {};
	static DXChoice* Allocator(GARChooserWindow* gcw, Symbol sym)
	    { return new DXChoice(gcw, sym); }
	~DXChoice(){};

	virtual void		initialize();
	virtual const char*	getFormalName() { return "Data Explorer file"; }
	virtual const char*	getInformalName() { return "DX"; }
	virtual const char*	getFileExtension() { return ".dx"; }
	virtual const char*	getImportType() { return "dx"; }

	const char *	getClassName() {
	    return ClassDXChoice;
	}

};

#endif  // _DX_CHOICE_H

