/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include "ResourceManager.h"
#include "ListIterator.h"
#include "IBMApplication.h"
#include "WarningDialogManager.h"

#define VALUE_SEPARATOR ';'

#define ERRMSG "%s is not writable.  Session settings will not be saved."

#define MAX_LIST_VALUES 10

ResourceManager* theResourceManager = 0;

void ResourceManager::BuildTheResourceManager()
{
    if (theResourceManager == 0) 
	theResourceManager = new ResourceManager();
}

ResourceManager::ResourceManager()
{
    this->write_protection_complaint=FALSE;
}

ResourceManager::~ResourceManager()
{
    ListIterator iter(this->multi_valued);
    Symbol s;
    while ((s=(Symbol)iter.getNext())) {
	List* values = (List*)this->resources.findDefinition(s);
	if (!values) continue;
	ListIterator viter(*values);
	char* value;
	while ((value=(char*)viter.getNext())) {
	    delete value;
	}
	delete values;
    }
}

void ResourceManager::removeValue (const char* resource, const char* value)
{
    Symbol key = theSymbolManager->registerSymbol(resource);
    if (this->multi_valued.isMember((void*)key)) {
	this->removeListValue (key, value);
    } else {
	this->resources.removeDefinition (key);
    }
}

void ResourceManager::addValue (const char* resource, const char* value)
{
    Symbol key = theSymbolManager->registerSymbol(resource);
    if (this->multi_valued.isMember((void*)key)) {
	this->addListValue (key, value);
    } else {
	this->resources.replaceDefinition (key, value);
    }
}

//
// I suspect most systems  accept path names of up to
// 256 bytes.  The resource setting could get huge, so
// by default, limit the number of entries in list-values
// resources to 10.  Provide API for changing this number.
//
void ResourceManager::addListValue(Symbol key, const char* value)
{
    List* values = (List*)this->resources.findDefinition(key);
    ASSERT(values);
    Symbol v = theSymbolManager->registerSymbol(value);
    if (values->isMember((void*)v)) {
	values->removeElement((void*)v);
    }
#if APPEND
    values->appendElement((void*)v);
    if (values->getSize() > MAX_LIST_VALUES) 
	values->deleteElement(1);
#else
    values->insertElement((void*)v,1);
    while (values->getSize() > MAX_LIST_VALUES) {
	values->deleteElement(values->getSize());
    }
#endif
}

void ResourceManager::removeListValue(Symbol key, const char* value)
{
    List* values = (List*)this->resources.findDefinition(key);
    ASSERT(values);
    Symbol v = theSymbolManager->registerSymbol(value);
    values->removeElement((void*)v);
}


void ResourceManager::registerMultiValued(const char* resource)
{
    Symbol key = theSymbolManager->getSymbol(resource);
    boolean must_init = FALSE;
    if (!key) {
	key = theSymbolManager->registerSymbol(resource);
	must_init = TRUE;
    }
    ASSERT (this->multi_valued.isMember((void*)key)==FALSE);
    this->multi_valued.appendElement((void*)key);
    ASSERT (this->resources.findDefinition(key)==0);
    List* values = new List();
    this->resources.addDefinition(key, values);
    if (must_init) this->initializeMultiValued(key);
}

void ResourceManager::saveResources()
{
    int size = this->resources.getSize();
    for (int i=1; i<=size; i++) {
	Symbol key = this->resources.getSymbol(i);
	if (this->multi_valued.isMember((void*)key))
	    this->saveListResource(key);
	else
	    this->saveResource(key);
    }
}

void ResourceManager::saveListResource(Symbol key)
{
    List* values = (List*)this->resources.findDefinition(key);
    ASSERT(values);

    char spec[4096];
    if (values->getSize() == 0) {
	// Can't just return, must remove the spec from the file.
	spec[0] = '\0';
    }

    int os=0;
    ListIterator iter(*values);
    Symbol v;
    boolean first = TRUE;
    while ((v=(Symbol)(long)iter.getNext())) {
	const char* str = theSymbolManager->getSymbolString(v);
	if (!first) {
	    spec[os++] = VALUE_SEPARATOR;
	    spec[os] = '\0';
	} else {
	    first = FALSE;
	}
	strcpy (&spec[os], str);
	os+= strlen(str);
    }
    theIBMApplication->printResource(theSymbolManager->getSymbolString(key),spec);
}

void ResourceManager::saveResource(Symbol key)
{
    Symbol v = (Symbol)(long)this->resources.findDefinition(key);
    theIBMApplication->printResource(theSymbolManager->getSymbolString(key), 
	theSymbolManager->getSymbolString(v));
}

void ResourceManager::getValue (const char* resource, String& result)
{
    Symbol key = theSymbolManager->getSymbol(resource);
    if (!key) {
	key = theSymbolManager->registerSymbol(resource);
	this->initializeValue(key);
    }
    result[0] = '\0';
    Symbol s = (Symbol)(long)this->resources.findDefinition(resource);
    const char* str = theSymbolManager->getSymbolString(s);
    strcpy (result, str);
}
void ResourceManager::getValue (const char* resource, List& result)
{
    result.clear();
    List* values = (List*)this->resources.findDefinition(resource);
    if (!values) return ;
    ListIterator iter(*values);
    const void* v;
    while ((v=iter.getNext())) result.appendElement(v);
}

void ResourceManager::initializeValue(Symbol key)
{
    const char* keyStr = theSymbolManager->getSymbolString(key);
    const char* class_name = theIBMApplication->getApplicationClass();

    char fname[256];
    if (!theIBMApplication->getApplicationDefaultsFileName(fname, TRUE)) {
	if (!this->write_protection_complaint) {
	    ModalWarningMessage (theApplication->getRootWidget(), ERRMSG, fname);
	    this->write_protection_complaint = TRUE;
	}
    }
    XrmDatabase db = XrmGetFileDatabase(fname);
    if (!db) return ;
    char* type=0;
    XrmValue value;
    char rspec[256];
    sprintf (rspec, "%s*%s", class_name,keyStr);
    if (XrmGetResource(db, rspec, rspec, &type, &value)) {
	const char* spec = value.addr;
	if ((spec) && (spec[0])) {
	    char* cp = DuplicateString(spec);
	    this->resources.addDefinition(key,cp);
	}
    }
    XrmDestroyDatabase(db);
}

void ResourceManager::initializeMultiValued(Symbol key)
{
    const char* keyStr = theSymbolManager->getSymbolString(key);
    const char* class_name = theIBMApplication->getApplicationClass();

    char fname[256];
    if (!theIBMApplication->getApplicationDefaultsFileName(fname, TRUE)) {
	if (!this->write_protection_complaint) {
	    ModalWarningMessage (theApplication->getRootWidget(), ERRMSG, fname);
	    this->write_protection_complaint = TRUE;
	}
    }
    XrmDatabase db = XrmGetFileDatabase(fname);
    if (!db) return ;
    char* type=0;
    XrmValue value;
    char rspec[256];
    sprintf (rspec, "%s*%s", class_name,keyStr);
    if (XrmGetResource(db, rspec, rspec, &type, &value)) {
	const char* spec = value.addr;
	if ((spec) && (spec[0])) {
	    int spec_len = strlen(spec);

	    List* values = (List*)this->resources.findDefinition(key);
	    ASSERT(values);

	    char buf[256];
	    int si=0;
	    for (int i=0; i<=spec_len; i++) {
		if ((i==spec_len) || (spec[i] == VALUE_SEPARATOR)) {
		    int len = i-si;
		    if (len>0) {
			strncpy (buf, &spec[si], len);
			buf[len] = '\0';
			Symbol v = theSymbolManager->registerSymbol(buf);
			values->appendElement((void*)v);
		    }
		    si = i+1;
		}
	    }
	}
    }
    XrmDestroyDatabase(db);
}
