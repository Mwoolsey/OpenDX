/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _GroupManager_h
#define _GroupManager_h


#include "Base.h"
#include "Dictionary.h"
#include "DXStrings.h"

//
// Class name definition:
//
#define ClassGroupManager	"GroupManager"

class List;
class DXApplication;
class Network;
class GroupedObject;


//
// The class to hold the group info
//
class GroupRecord 
{
  friend class GroupManager;
  friend class GroupedObject;
  private:
    
    Network *network;
    boolean dirty;
    char *name;

  protected:

    GroupRecord(Network *network, const char *name) {
	this->network = network;
        this->dirty   = FALSE;
	this->name    = DuplicateString(name);
    };

  public:
    	    boolean  isDirty () { return this->dirty; }
    	    void     setDirty (boolean dirty = TRUE) { this->dirty = dirty; }
    	    Network *getNetwork () { return this->network; }
    virtual boolean  changesWhere () { return FALSE; }

    const   char    *getName() { return this->name; }

    virtual ~GroupRecord() { 
	if (this->name) delete this->name;
    }
};


//
// GroupManager class definition:
//				
class GroupManager : public Base
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    boolean  dirty;
    //
    // The host-argument dictionary 
    //
    Dictionary	arguments;
    //
    // A dictionary of lists of GroupRecord. 
    //
    Dictionary	groups;

    DXApplication *app;

    Network* network;

    //
    // Constructor:
    //
    GroupManager(Network*, Symbol groupID);

    virtual GroupRecord *recordAllocator(Network *net, const char *name) = 0;

    Symbol groupID;

  public:
    //
    // Destructor:
    //
    ~GroupManager();

    //
    // Remove all nodes from all groups and all groups from this manager.
    //
    virtual void clear();    

    //
    // Return the number of groups.
    //
    int getGroupCount() { return this->groups.getSize(); }

    //
    // Check if the given group exists.
    //
    boolean hasGroup(const char *name)
    {
	return (boolean)
	    ((GroupRecord*)this->groups.findDefinition(name) != NUL(GroupRecord*));
    }
   
    //
    // Return the group record
    //
    GroupRecord *getGroup(const char *name)
    {
	return (GroupRecord*)this->groups.findDefinition(name);
    }

    //
    // Return the Nth group's name (1 based).
    //
    const char* getGroupName(int n)
    {
	return this->groups.getStringKey(n);
    }

    //
    // Create a new group of nodes with the given name.
    // If name is already active, then return FALSE.
    //
    boolean 	createGroup(const char *name, Network *net);

    //
    // Add more modules to the existing group.
    //
    boolean 	addGroupMember(const char *name, Network *net);

    //
    // Remove modules from the existing group.
    //
    virtual boolean 	removeGroupMember(const char *name, Network *net);

    //
    // Called when reading a network.
    //
    boolean 	registerGroup(const char *name, Network *net);
    //
    // Removes the nodes from the named group.
    // Return FALSE if the group does not exist. 
    //
    boolean 	removeGroup(const char *name,Network *net); 

    // 
    // return the network a group resides.
    //
    Network *getGroupNetwork(const char* name);

    //
    // Select nodes in the group.
    //
    boolean selectGroupNodes(const char *name);

    //
    //
    //
    boolean isDirty() { return this->dirty; }
    void setDirty(boolean set = TRUE) { this->dirty = set; }

    //
    // Parse/Print the  group assignment comment.
    //
    virtual boolean parseComment(const char *, 
			const char *, 
			int ,
			Network *){ return TRUE; }
    virtual boolean printComment(FILE *){ return TRUE; }
    virtual boolean printAssignment(FILE *){ return TRUE; }

    const char *getManagerName();
    Symbol getManagerSymbol() { return this->groupID; }

    virtual boolean survivesMerging() { return FALSE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassGroupManager;
    }
};


#endif // _GroupManager_h
