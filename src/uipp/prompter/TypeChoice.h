/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _TYPECHOICE_H
#define _TYPECHOICE_H

#include "UIComponent.h"
#include "SymbolManager.h"

#include "dxl.h"

#define ClassTypeChoice "TypeChoice"

class GARChooserWindow;
class ToggleButtonInterface;
class ButtonInterface;
class Command;
class CommandScope;
class TypeChoice;
class MsgDialog;

#define VERTICAL_LAYOUT 1

typedef TypeChoice* (*TypeChoiceAllocator)(GARChooserWindow *, Symbol);

extern "C" void TypeChoice_InfoMsgCB (DXLConnection *, const char *, const void *);
extern "C" void TypeChoice_NetSeekerCB 
(DXLConnection *, const char *, const char *, void *);
extern "C" void TypeChoice_ErrorMsgCB (DXLConnection *, const char *, const void *);
extern "C" void TypeChoice_BrokenConnCB (DXLConnection *, void *);
extern "C" void TypeChoice_HandleMessagesCB (XtPointer , int*, XtInputId* );
extern "C" Boolean TypeChoice_SeekerWP (XtPointer );


class TypeChoice : public UIComponent {
    private:

	Symbol		choice_id;

	static 		MsgDialog* Messages;

	int		connection_mode;

	static XtInputId    InputId;
	static XtWorkProcId SeekerWP;

	//
	// Instead of closing/opening all the time, try keeping the connections
	// alive to reduce process starting/stopping.
	//
	static DXLConnection* ExecConnection;
	static DXLConnection* DxuiConnection;

    protected:

	static String	DefaultResources[];

	char*		previous_data_file;

	virtual void	setAttachments(ButtonInterface**, int, Widget);
	Widget		expand_form;
	boolean		browsable;
	boolean		testable;
	boolean		visualizable;
	boolean		prompterable;

	ToggleButtonInterface *		radio_button;

	TypeChoice (
	    const char*		name,
	    boolean		browsable,
	    boolean		testable,
	    boolean		visualizable,
	    boolean		prompterable,
	    GARChooserWindow*	gcw,
	    Symbol		sym
	);

	virtual Widget  createBody(Widget , Widget ) { return NUL(Widget); }

	Command*		browseCmd;
	Command*		verifyDataCmd;
	Command*		visualizeCmd;
	Command*		prompterCmd;
	Command*		simplePrompterCmd;
	Command*		setChoiceCmd;

	virtual int		expandedHeight() = 0;
	void netFoundCB(const char* str = NUL(char*));

	GARChooserWindow*	gcw;

	CommandScope*		getCommandScope();

	enum {
	    ImageMode	= 1,
	    EditMode	= 2,
	    ExecMode	= 3,
	    UserChoice	= 4
	};

	boolean			connect(int mode=TypeChoice::UserChoice);

	char*			net_to_run;

	friend void TypeChoice_InfoMsgCB (DXLConnection *, const char *, const void *);
	friend void TypeChoice_NetSeekerCB 
		(DXLConnection *, const char *, const char *, void *);
	friend void TypeChoice_ErrorMsgCB (DXLConnection *, const char *, const void *);
	friend void TypeChoice_BrokenConnCB (DXLConnection *, void *);
	friend void TypeChoice_HandleMessagesCB (XtPointer , int*, XtInputId* );
	friend Boolean TypeChoice_SeekerWP (XtPointer );


    public:

	~TypeChoice();

	virtual void 	createChoice(Widget parent);

	virtual void		manage();
	virtual void		unmanage();

	virtual int		getMinWidth() = 0;
	virtual void		initialize() {};
	virtual const char*	getFormalName() = 0;
	virtual const char*	getInformalName() = 0;
	virtual const char*	getImportType() { return NUL(char*); }
	virtual const char*	getFileExtension() = 0;
	virtual const char*	getActiveHelpMsg() = 0;
	virtual void		setCommandActivation (boolean file_checked=FALSE);
	virtual boolean		sendDataFile () { return TRUE; }
	virtual boolean		canHandle(const char*) { return FALSE; }
	virtual boolean		hasSettings() { return TRUE; }
	virtual boolean		prompter() { return TRUE; }
	virtual boolean		simplePrompter() { return TRUE; }
	virtual boolean		visualize();
	virtual boolean		verify(const char* seek = NUL(char*));
	virtual boolean		browse();
	virtual boolean		usesPrompter() { return FALSE; }
	virtual boolean		retainsControl(const char*) { return FALSE; }

	int		getHeightContribution();
	int		getMessageCnt();
	boolean 	isTestable() 		{ return this->testable; }
	boolean 	isBrowsable() 		{ return this->browsable; }
	boolean 	isVisualizable()	{ return this->visualizable; }
	boolean 	isPrompterable() 	{ return this->prompterable; }

	boolean 	selected();
	boolean		setChoice();
	void	 	setSelected (boolean state = TRUE);
	void    	appendMsg (const char *msg, boolean info = TRUE);
	void		showList();
	void		hideList();
	Widget  	getOptionWidget();
	Symbol  	getType() 	{ return this->choice_id; }

	const char *	getClassName() {
	    return ClassTypeChoice;
	}

};

#endif  // _TYPECHOICE_H
