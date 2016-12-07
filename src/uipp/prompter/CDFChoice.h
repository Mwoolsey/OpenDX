/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _CDF_CHOICE_H
#define _CDF_CHOICE_H

#include "ImportableChoice.h"
#include "SymbolManager.h"

#define ClassCDFChoice "CDFChoice"

class ToggleButtonInterface;

class CDFChoice : public ImportableChoice {
    private:
	static boolean	ClassInitialized;

    protected:

	static String	DefaultResources[];

    public:

	CDFChoice (GARChooserWindow* gcw, Symbol sym) : ImportableChoice (
		"cdfData", FALSE, TRUE, TRUE, FALSE, gcw, sym) {};
	static CDFChoice* Allocator (GARChooserWindow* gcw, Symbol sym)
	    { return new CDFChoice(gcw, sym); }
	~CDFChoice(){};

	virtual void		initialize();
	virtual const char*	getFormalName() { return "CDF format"; }
	virtual const char*	getInformalName() { return "CDF"; }
	virtual const char*	getFileExtension() { return ".cdf"; }
	virtual const char*	getImportType() { return "CDF"; }

	const char *	getClassName() {
	    return ClassCDFChoice;
	}

};

#endif  // _CDF_CHOICE_H


