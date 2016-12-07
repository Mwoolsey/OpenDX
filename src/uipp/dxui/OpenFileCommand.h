/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _OpenFileCommand_h
#define _OpenFileCommand_h

#include "DXApplication.h"
#include "OpenCommand.h"
#include "SymbolManager.h"

class OpenFileCommand : public OpenCommand {
    protected:
	Symbol s;
    public:
	OpenFileCommand(Symbol s) :
	    OpenCommand ("Open",0,TRUE,theDXApplication,NUL(Widget)) {
		ASSERT(s);
		this->s = s;
	    }
	~OpenFileCommand(){}
	boolean doIt(CommandInterface* ci) {
	    const char* fname = theSymbolManager->getSymbolString(this->s);
	    if (!theDXApplication->openFile(fname)) {
		theDXApplication->removeReferencedFile(fname);
		return FALSE;
	    }
	    return TRUE;
	}
};
#endif
