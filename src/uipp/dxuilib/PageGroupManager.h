/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _PageGroupManager_h
#define _PageGroupManager_h


#include "GroupManager.h"
#include "Dictionary.h"
#include "DXStrings.h"

//
// Class name definition:
//
#define ClassPageGroupManager	"PageGroupManager"
#define PAGE_GROUP "page"

class DXApplication;
class Network;
class UIComponent;
class PageSelector;


//
// The class to hold the group info
//
class PageGroupRecord : public GroupRecord
{
  friend class PageGroupManager;
  friend class PageSelector;
  friend class PageTab;
  private:
    
  protected:
     PageGroupRecord(Network *net, const char *name): GroupRecord(net, name) {
	this->page_window = NUL(UIComponent*);
	this->workspace = NUL(UIComponent*);
	this->order_in_list = 0;
	this->windowed = FALSE;
	this->showing = FALSE;
     }


    UIComponent* page_window;
    UIComponent* workspace;

    int order_in_list;
    boolean windowed;
    boolean showing;

  public:

    void setComponents (UIComponent* page_window, UIComponent* page);

    ~PageGroupRecord() { }
};


//
// PageGroupManager class definition:
//				
class PageGroupManager : public GroupManager
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

    virtual GroupRecord *recordAllocator(Network *net, const char *name) { 
	return new PageGroupRecord(net, name);
    }

  public:
    //
    // Destructor:
    //
    ~PageGroupManager(){}

    //
    // Constructor:
    //
    PageGroupManager(Network *net); 

    static boolean SupportsMacros() { return TRUE; }

    //
    // Parse/Print the  group assignment comment.
    //
    virtual boolean parseComment(const char *, 
			const char *, 
			int ,
			Network *);
    virtual boolean printComment(FILE *);

    virtual boolean survivesMerging() { return TRUE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassPageGroupManager;
    }
};


#endif // _PageGroupManager_h
