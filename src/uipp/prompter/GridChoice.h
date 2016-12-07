/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _GRID_CHOICE_H
#define _GRID_CHOICE_H

#include "NonimportableChoice.h"
#include "SymbolManager.h"

#define ClassGridChoice "GridChoice"

class Command;
class ToggleButtonInterface;

class GridChoice : public NonimportableChoice {
    private:
	static boolean	ClassInitialized;

	//
	// prevent recursion opening a .general file which points to itself
	//
	boolean recursive;

    protected:

	static String	DefaultResources[];

	virtual Widget  createBody (Widget parent, Widget top);

	ToggleButtonInterface*		grid1Option;
	ToggleButtonInterface*		grid2Option;
	ToggleButtonInterface*		grid3Option;
	ToggleButtonInterface*		grid4Option;
	ToggleButtonInterface*		positionsOption;
	ToggleButtonInterface*		singleTimeStepOption;
	ToggleButtonInterface*		blockOption;
	ToggleButtonInterface*		spreadSheetOption;

	Command*		gridTypeCmd;
	Command*		positionsCmd;

	Widget		dimension_l;
	Widget		dimension_s;
	Widget		data_org_l;
	Widget		data_org_rb;
	Widget		number_var_l;
	Widget		number_var_s;

	boolean		positions_was_on;

	virtual int	expandedHeight() 
	    { return (VERTICAL_LAYOUT?300:120); }

	//const char*	determineNetToRun(char **);

    public:

	GridChoice (GARChooserWindow* gcw, Symbol sym);
	static GridChoice* Allocator (GARChooserWindow* gcw, Symbol sym)
	    { return new GridChoice(gcw, sym); }
	~GridChoice();

	virtual void		initialize();
	virtual const char*	getFormalName() { 
	    return "Grid or Scattered file (General Array Format)"; 
	}
	virtual const char*	getInformalName() { return "Grid"; }
	virtual const char*	getFileExtension() { return ""; }
	virtual const char*	getImportType() { return "general"; }
	virtual const char*	getActiveHelpMsg() 
	    { return "Any flat file, ascii or binary"; }
	virtual boolean		sendDataFile() { return FALSE; }
	virtual boolean		canHandle(const char* ext);
	virtual boolean		prompter();
	virtual boolean		simplePrompter();
	virtual void		setCommandActivation (boolean file_checked=FALSE);
	virtual boolean		visualize();
	virtual boolean		verify(const char* seek = NUL(char*));
	virtual boolean         usesPrompter() { return TRUE; }
	virtual boolean		retainsControl(const char* new_file);

	boolean			setGridType();
	boolean			usePositions();

	const char *	getClassName() {
	    return ClassGridChoice;
	}

};

#endif  // _GRID_CHOICE_H

