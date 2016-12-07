/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _ResourceManager_h
#define _ResourceManager_h

#include <Xm/Xm.h>
#include "Base.h"
#include "Dictionary.h"
#include "DXStrings.h"

#define ClassResourceManager "ResourceManager"

class ResourceManager : public Base
{
    private:
	Dictionary resources;
	List multi_valued;
	String tmp;
	boolean write_protection_complaint;
    protected:
	ResourceManager();
	void saveListResource(Symbol key);
	void saveResource(Symbol key);
	void addListValue(Symbol key, const char* value);
	void removeListValue(Symbol key, const char* value);
	void initializeValue(Symbol key);
	void initializeMultiValued(Symbol key);
    public:
	static void BuildTheResourceManager();
	~ResourceManager();
	void saveResources();
	void registerMultiValued(const char* resource);
	void addValue (const char* resource, const char* value);
	void removeValue (const char* resource, const char* value);
	void getValue (const char* resource, String& value);
	void getValue (const char* resource, List& value);
	const char* getClassName() { return ClassResourceManager; }
};

extern ResourceManager* theResourceManager;

#endif

