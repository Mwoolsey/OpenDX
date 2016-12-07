/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _DropSite_h
#define _DropSite_h

#include <Xm/Xm.h>
#include <Xm/DragDrop.h>

#include "Base.h"
#include "TransferStyle.h"
#include "Dictionary.h"

//
// Class name definition:
//
#define ClassDropSite	"DropSite"

extern "C" void DropSite_HandleDrop(Widget, XtPointer, XtPointer);
extern "C" void DropSite_TransferProc(Widget w, XtPointer client_data, 
				Atom *seltype, Atom *type,
				XtPointer value, unsigned long *length, 
				int format);

//
// DropSite class definition:
//				
class DropSite : public Base 
{
  private:
    //
    // Private member data:
    //
    static boolean DropSiteClassInitialized;

    friend void DropSite_HandleDrop(Widget, XtPointer, XtPointer);
    friend void DropSite_TransferProc(Widget w, XtPointer client_data,
				Atom *seltype, Atom *type,
				XtPointer value, unsigned long *length,
				int format);


    // Saved in constructor for use during initializeRootWidget().
    Widget		drop_w;	

    XContext		context;
    Atom		*imports;
    int			num_imports;
    static int		drop_x,drop_y;

  protected:

    //
    // Protected member data:
    //
    char		*transfered_data;

    virtual void setDropWidget(Widget, unsigned char type = XmDROP_SITE_SIMPLE,
	Pixmap animation=(Pixmap)XmUNSPECIFIED_PIXMAP, 
	Pixmap anmiation_mask=(Pixmap)XmUNSPECIFIED_PIXMAP);
    virtual boolean transfer(char *, XtPointer, unsigned long, int, int);

    //
    // There was a Dictionary* up in private.  It's replaced with this call.
    // Subclasses want to enforce sharing a common dictionary among their members.
    //
    virtual Dictionary *getDropDictionary() = 0;

    //
    // Instead of passing around a stored value for this, make the subclasses
    // implement a proc which decodes according to an enum and then invokes a member.
    //
    virtual boolean decodeDropType (int, char *, XtPointer, unsigned long, int, int) = 0;

  public:

    //
    // Constructor:
    //
    DropSite();

    void addSupportedType (int, const char *typeName, boolean);
 
    //
    // Destructor:
    //
    ~DropSite(); 
  
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDropSite;
    }
};


#endif // _DropSite_h
