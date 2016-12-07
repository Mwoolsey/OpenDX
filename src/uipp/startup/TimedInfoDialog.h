/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TimedInfoDialog_h
#define _TimedInfoDialog_h


#include "TimedDialog.h"


//
// Class name definition:
//
#define ClassTimedInfoDialog	"TimedInfoDialog"

//
// TimedInfoDialog class definition:
//				
class TimedInfoDialog : public TimedDialog
{
  private:
    char *title;
    char *msg;

  protected:
    virtual Widget createDialog (Widget parent);

  public:
    //
    // Constructor:
    //
    TimedInfoDialog (
	const char* name, 
	Widget parent, 
	int timeout 
    ): TimedDialog(name, parent, timeout) {
	this->title = NUL(char*);
	this->msg   = NUL(char*);
    };

    void setTitle(const char *title);
    void setMessage(const char *message);

    //
    // Destructor:
    //
    ~TimedInfoDialog() {
	if (this->title) delete this->title;
	if (this->msg)   delete this->msg;
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassTimedInfoDialog; }
};
#endif // _TimedInfoDialog_h
