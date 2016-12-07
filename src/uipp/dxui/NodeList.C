/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "NodeList.h"
#include "Node.h"
#include "ListIterator.h"
#include "DictionaryIterator.h"


NodeList::NodeList()
{
}


NodeList::~NodeList()
{	
    DictionaryIterator di(this->nodeDict);
    List* nl;
    while ( (nl = (List*)di.getNextDefinition()) ) 
	delete nl;
    this->nodeDict.clear();
}


void NodeList::clear()
{
    ASSERT(this);
    this->List::clear();
    DictionaryIterator di(this->nodeDict);
    List* nl;
    while ( (nl = (List*)di.getNextDefinition()) ) 
	delete nl;
    this->nodeDict.clear();
}


boolean NodeList::insertElement(const void* element, const int position)
{
    boolean status = this->List::insertElement(element, position);
    if (status == FALSE) return FALSE;

    //
    // If the Node wants special treatment, then add him
    //
    Node* n = (Node*)element;
    if (n->needsFastSort() == TRUE) {
	List* nl = this->getSubList(n);
	ASSERT (nl->isMember((void*)n) == FALSE);
	nl->appendElement((void*)n);
    }

    return TRUE;
}

List* NodeList::getSubList(Node* n)
{
    const char* node_class = n->getClassName();
    const void* def = this->nodeDict.findDefinition(node_class);
    if (def == NUL(char*)) {
	List* nl = new List;
	this->nodeDict.addDefinition(node_class, nl);
	def = (const void*)nl;
    }
    return (List*)def;
}

boolean NodeList::replaceElement(const void* element, const int position)
{
    void* old_element = (void*)this->getElement(position);
    boolean status = this->List::replaceElement(element, position);
    if (status == FALSE) return FALSE;

    Node* n = (Node*)old_element;
    if (n->needsFastSort() == TRUE) {
	List* nl = getSubList(n);
	ASSERT (nl->isMember((void*)n) == TRUE);
	nl->removeElement((void*)n);
    }

    n = (Node*)element;
    if (n->needsFastSort() == TRUE) {
	List* nl = getSubList(n);
	ASSERT (nl->isMember((void*)n) == FALSE);
	nl->appendElement((void*)n);
    }

    return status;
}


boolean NodeList::deleteElement(const int position)
{
    void* old_element = (void*)this->getElement(position);
    boolean status = this->List::deleteElement(position);
    if (status == FALSE) return FALSE;

    Node* n = (Node*)old_element;
    if (n->needsFastSort() == TRUE) {
	List* nl = getSubList(n);
	ASSERT(nl->isMember((void*)n) == TRUE);
	nl->removeElement((void*)n);
    }

    return status;
}

//
// Create a duplicate list with the same list items.
//
List* NodeList::dup()
{
    NodeList *nl = new NodeList;
    ListIterator iter(*this);
    const void *v;

    while ( (v = (const void*)iter.getNext()) )
        nl->appendElement(v);

    return nl;
}

List* NodeList::makeClassifiedNodeList(const char* classname)
{
    ASSERT(classname);
    int size = this->nodeDict.getSize();
    int i;
    for (i=1; i<=size; i++) {
	const char* cn = this->nodeDict.getStringKey(i);
	if (EqualString(cn, classname) == FALSE) continue;

	List* nl = (List*)this->nodeDict.getDefinition(i);
	return nl->dup();
    }
    ListIterator it(*this);
    Node* n;
    List* l = NUL(List*);
    while ( (n = (Node*)it.getNext()) ) {
	if (EqualString(classname, n->getClassName()) == TRUE) {
	    if (!l) l = new List;
	    l->appendElement((void*)n);
	}
    }
    return l;
}
