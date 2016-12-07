/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _IMPORTABLE_CHOICE_H
#define _IMPORTABLE_CHOICE_H

#include "TypeChoice.h"
#include "SymbolManager.h"

#define ClassImportableChoice "ImportableChoice"

class ToggleButtonInterface;
class GARChooserWindow;

class ImportableChoice : public TypeChoice {
    private:
	static boolean	ClassInitialized;

    protected:

	static String	DefaultResources[];

	virtual int	expandedHeight() 
	    { return (VERTICAL_LAYOUT?80:30); }

	ImportableChoice (
	    const char*         name,
	    boolean             browsable,
	    boolean             testable,
	    boolean             visualizable,
	    boolean             prompterable,
	    GARChooserWindow*   gcw,
	    Symbol              sym
	);

    public:

	~ImportableChoice(){};

	virtual const char*	getActiveHelpMsg();
	virtual int		getMinWidth() 
	    { return (VERTICAL_LAYOUT?410:575); }
	virtual boolean		hasSettings() { return TRUE; }

	const char *	getClassName() {
	    return ClassImportableChoice;
	}

};

#endif  // _IMPORTABLE_CHOICE_H

