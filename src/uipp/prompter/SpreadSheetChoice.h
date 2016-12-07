/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _SSHEETCHOICE_H
#define _SSHEETCHOICE_H

#include "NonimportableChoice.h"
#include "SymbolManager.h"

#define ClassSpreadSheetChoice "SpreadSheetChoice"

extern "C" void SpreadSheetChoice_ParseCB(Widget, XtPointer, XtPointer);
extern "C" void SpreadSheetChoice_NumberCB(Widget, XtPointer, XtPointer);

class DXValue;

class SpreadSheetChoice : public NonimportableChoice {
    private:
	static boolean	ClassInitialized;

#if USES_DXLINK
	Widget		row_numbers;
	Widget		starting_row;
	Widget		ending_row;
	Widget		delta_row;
	Widget		restrict_names;
#endif
	Widget		delimiter;

	char*		saved_columns;
	char*		saved_delimiter;

	boolean		parseValue(Widget);

	DXValue*	names_value;
	DXValue*	delimiter_value;

    protected:

	static String	DefaultResources[];

	ToggleButtonInterface*	tableOption;
	ToggleButtonInterface*	matrixOption;
#if USES_DXLINK
	ToggleButtonInterface*	restrictOption;
	ToggleButtonInterface*	specifyRowsOption;
#endif
	ToggleButtonInterface*	useDelimiterOption;

	Command*		restrictCmd;
	Command*		useRowsCmd;
	Command*		delimiterCmd;

	virtual Widget 	createBody (Widget parent, Widget top);
	virtual int	expandedHeight() 
	    { return (VERTICAL_LAYOUT?180:80); }
        friend void SpreadSheetChoice_ParseCB(Widget, XtPointer, XtPointer);
        friend void SpreadSheetChoice_NumberCB(Widget, XtPointer, XtPointer);

    public:

	SpreadSheetChoice (GARChooserWindow* gcw, Symbol sym);
	static SpreadSheetChoice* 
	    Allocator(GARChooserWindow* gcw, Symbol sym)
		{ return new SpreadSheetChoice (gcw, sym); }
	~SpreadSheetChoice();

	virtual void		initialize();
	virtual const char*	getFormalName() { return "Spreadsheet format file"; }
	virtual const char*	getInformalName() { return "Spreadsheet"; }
	virtual const char*	getFileExtension() { return ""; }
	virtual const char*	getActiveHelpMsg() { 
	    return "Columns of ascii data, typically output by a spreadsheet program";
	}
	virtual boolean		verify(const char* seek=NUL(char*));
	virtual boolean		visualize();

	boolean			restrictNamesCB();
	boolean			useRowsCB();
	boolean			useDelimiterCB();

	const char *	getClassName() {
	    return ClassSpreadSheetChoice;
	}

};

#endif  // _SSHEETCHOICE_H

