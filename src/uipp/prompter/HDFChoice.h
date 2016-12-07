/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _HDF_CHOICE_H
#define _HDF_CHOICE_H

#include "ImportableChoice.h"
#include "SymbolManager.h"

#define ClassHDFChoice "HDFChoice"

class ToggleButtonInterface;

class HDFChoice : public ImportableChoice {
    private:
	static boolean	ClassInitialized;

    protected:

	static String	DefaultResources[];

    public:

	HDFChoice (GARChooserWindow* gcw, Symbol sym) : ImportableChoice (
	    "hdfData", FALSE, TRUE, TRUE, FALSE, gcw, sym) {};
	static HDFChoice* Allocator(GARChooserWindow* gcw, Symbol sym)
	    { return new HDFChoice(gcw, sym); }
	~HDFChoice(){};

	virtual void		initialize();
	virtual const char*	getFormalName() { return "HDF format"; }
	virtual const char*	getInformalName() { return "HDF"; }
	virtual const char*	getFileExtension() { return ".hdf"; }
	virtual const char*	getImportType() { return "hdf"; }

	const char *	getClassName() {
	    return ClassHDFChoice;
	}

};

#endif  // _HDF_CHOICE_H

