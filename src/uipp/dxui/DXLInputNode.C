/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXLInputNode.h"
#include "DXApplication.h"
#include "Network.h"
#include "Parameter.h"
#include "DXPacketIF.h"
#include "List.h" 
#include "Ark.h"
#include "InfoDialogManager.h"
#include "ErrorDialogManager.h"

boolean DXLInputNode::Initializing = FALSE;

DXLInputNode::DXLInputNode(NodeDefinition *nd, Network *net, int instnc) :
    UniqueNameNode(nd, net, instnc)
{
}
boolean DXLInputNode::initialize()
{
    char label[512];

    if (!this->getNetwork()->isReadingNetwork()) {
	sprintf(label,"%s_%d",this->getNameString(),this->getInstanceNumber());
	DXLInputNode::Initializing = TRUE;
	this->setLabelString(label);
	DXLInputNode::Initializing = FALSE;
    }

    return TRUE;
}

DXLInputNode::~DXLInputNode()
{
}


char *DXLInputNode::netNodeString(const char *prefix)
{
    char      *string = new char[512], inputname[128], outputname[128];
    const char *label = (char*)this->getLabelString(); 

    this->getNetworkOutputNameString(1,outputname);

    if (this->isInputConnected(1)) {
	//
	// Get the name of the output that is connected to first input 
	//
	char srcoutputname[128];
  	List *l = (List*)this->getInputArks(1);
	ASSERT(l);
	Ark *a = (Ark*)l->getElement(1);
	ASSERT(a);
	int index;
	Node *n = a->getSourceNode(index);
	ASSERT(n);
	this->getNetworkInputNameString(1,inputname);
	n->getNetworkOutputNameString(index,srcoutputname);
	sprintf(string, "    %s = %s;\n"
			"    %s = %s;\n" 
			"    %s = %s;\n", 
		inputname, srcoutputname,// This makes the Network catch the Ark
		label, inputname,
		outputname, label);  	
    } else {
	sprintf(string,"    %s = %s;\n", outputname,label);
    }


    return string;
}



char        *DXLInputNode::valuesString(const char *prefix)
{
    const char *label = this->getLabelString();
    const char *value = this->getInputValueString(1);
    char *vs = new char [STRLEN(label) +  STRLEN(value) + 6];
    sprintf(vs,"%s = %s;\n", label, value);
    return vs;
}

boolean     DXLInputNode::sendValues(boolean ignoreDirty )
{
    DXPacketIF *pif  = theDXApplication->getPacketIF();

    if (!pif)
        return TRUE;

    Parameter *p = this->getInputParameter(1);

    if (p->isNeededValue(ignoreDirty)) {
	char *vs = this->valuesString("");
 	pif->send(DXPacketIF::FOREGROUND, vs);	
	delete vs;
    }
    return TRUE;
}

//
// Determine if this node is of the given class.
//
boolean DXLInputNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassDXLInputNode);
    if (s == classname)
	return TRUE;
    else
	return this->UniqueNameNode::isA(classname);
}

//
// This is the same as the super-class except that we enforce restriction
// by default, and we make sure that it does not conflict with other nodes
// in the network (e.g. Transmitters).
//
boolean DXLInputNode::setLabelString(const char *label)
{
    if (EqualString(label, this->getLabelString()))
        return TRUE;

    Network *n = this->getNetwork();

    if (!DXLInputNode::Initializing && !n->isReadingNetwork()) {
	if (!this->verifyRestrictedLabel(label))
	    return FALSE;

	const char* conflict = n->nameConflictExists(this, label);
	if (conflict) {
	    ErrorMessage("A %s with name \"%s\" already exists.", 
						conflict, label);
	    return FALSE;
	}
    }


    return this->UniqueNameNode::setLabelString(label);
}

int DXLInputNode::assignNewInstanceNumber()
{
int instnum = this->UniqueNameNode::assignNewInstanceNumber();

    //
    // Compare the existing label against DXLInput_%d
    //
    const char* cur_label = this->getLabelString();
    const char* matchstr = "DXLInput_";
    boolean change_label = EqualSubstring (cur_label, matchstr, strlen(matchstr));
    if (change_label) {
	char *new_label = new char [2+strlen(cur_label)];
	sprintf (new_label, "%s%d", matchstr, instnum);
	this->setLabelString (new_label);
	delete new_label;
    }

    return instnum;
}


boolean DXLInputNode::printAsBean(FILE* f)
{
    if (this->isInputConnected(1)) return TRUE;
    if (this->isOutputConnected(1) == FALSE) return TRUE;

    char* indent = "    ";

    char buf[128];
    strcpy (buf, this->getLabelString());
    char PropName[128], propName[128];
    if ((buf[0] >= 'A') && (buf[0] <= 'Z'))
	buf[0]+= ('a' - 'A');
    strcpy (propName, buf);
    buf[0]-= ('a' - 'A');
    strcpy (PropName, buf);

    fprintf (f, "%sprivate float %s;\n", indent, propName);
    fprintf (f, "%spublic void set%s(float new_value) {\n", indent, PropName);
    indent = "\t";
    fprintf (f, "%sthis.%s = (float)new_value;\n", indent, propName);
    fprintf (f, "%sif (this.rmi == null) return %s;\n", indent, propName);
    fprintf (f, "%sthis.rmi.setScalar(\"%s\", new_value);\n", indent, this->getLabelString());
    indent = "    ";
    fprintf (f, "%s}\n\n", indent);
    fprintf (f, "%spublic float get%s() {\n", indent, PropName);
    indent = "\t";
    fprintf (f, "%sreturn (float)this.%s;\n", indent, propName);
    indent = "    ";
    fprintf (f, "%s}\n\n", indent);

    return TRUE;
}

boolean DXLInputNode::printAsBeanInitCall(FILE* f)
{
    if (this->isInputConnected(1)) return TRUE;
    if (this->isOutputConnected(1) == FALSE) return TRUE;
    const char* value = this->getInputValueString(1);
    if ((value == NUL(char*)) || (value[0] == '\0')) return TRUE;
    if (EqualString(value, "NULL")) return TRUE;
    if (EqualString(value, "\"NULL\"")) return TRUE;

    char* indent = "\t";

    char buf[128];
    strcpy (buf, this->getLabelString());
    char PropName[128], propName[128];
    if ((buf[0] >= 'A') && (buf[0] <= 'Z'))
	buf[0]+= ('a' - 'A');
    strcpy (propName, buf);
    buf[0]-= ('a' - 'A');
    strcpy (PropName, buf);

    fprintf (f, "%sset%s((float)%s);\n", indent, PropName, value);

    return TRUE;
}

