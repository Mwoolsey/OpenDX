/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




// 
// 3/24/95 dawood - Static macros are macros that do not execute unless
// their inputs change.  Previously, the UI used global variables to
// set hardcoded (i.e. tab down, unconnected) inputs to modules in macros.
// Now we place the parameter values write in the argument list to the
// modules.  Among other things this means that we must send a new
// definition of the macro (before an execution) each time the user
// changes a hardcoded input value. 
//
#define STATIC_MACROS

#include "DXStrings.h"
#include "lex.h"
#include "Node.h"
#include "NodeDefinition.h"
#include "Network.h"
#include "DXPacketIF.h"
#include "Parameter.h"
#include "StandIn.h"
#include "ConfigurationDialog.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "Ark.h"
#include "ListIterator.h"
#include "DXApplication.h"
#include "ProcessGroupManager.h"
#include "EditorWindow.h"
#if WORKSPACE_PAGES
#include "Dictionary.h"
#include "DictionaryIterator.h"
#endif

#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif


//
// Define the maxium number of repeatable inputs and outputs.
//
#define MAX_INPUT_SETS	21
#define MAX_OUTPUT_SETS 21

static char *__indent = "    ";

void Node::setDefinition(NodeDefinition *nd) 
{ 

    this->definition = nd;
    if (this->instanceNumber < 1)
	this->instanceNumber = nd->newInstanceNumber();
}

Node::Node(NodeDefinition *nd, Network *net, int inst)
{ 

    this->network = net; 
    this->instanceNumber = inst;
    this->setDefinition(nd);		// This may also set the instance # 
    this->vpe_xpos = this->vpe_ypos = 0; 
    this->labelSymbol = 0;
    this->standin = NULL; 
    this->cdb = NULL; 
    this->moduleMessageId = NULL; 
    this->nodeCacheability = nd->getDefaultCacheability(); 
    this->buildParameterLists();
    this->marked = FALSE;
#if WORKSPACE_PAGES
#else
    this->groupNameSymbol = 0;
#endif
    this->layout_information = NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
// Delete all elements that are considered part of the Node.  This includes
// input and output parameters, and a Node's standin if it has one.
//
Node::~Node()
{
    Parameter *p;
    ListIterator iterator;

    //
    // If this node has a messaging protocol and it gets deleted during
    // execution, be sure that the handler gets removed from the packet
    // handler so that the handler doesn't get called on a freed node.
    //
    if (this->moduleMessageId) {
	if (this->getNetwork() == theDXApplication->network) {
	    //
	    // don't remove the handler in the case of destroying
	    // a temporary network - i.e. drag-n-drop
	    //
	    DXPacketIF *pif = theDXApplication->getPacketIF();
	    if  (pif) 
		pif->setHandler(DXPacketIF::INFORMATION,
			NULL, (void*)this, this->moduleMessageId);
	}
	delete this->moduleMessageId;
    }

    //
    // Delete the Configuration Dialog for this node
    //
    if (this->cdb)  {
	delete this->cdb;
        this->cdb = NULL;	// Should speed up arc deletion.
    }
    
    //
    // Delete the standin for this node
    //
    StandIn *saved_standin = this->standin;
    this->standin = NULL;	// Should speed up arc deletion.

	

    //
    // Delete all the parameters in the Input parameter List
    //
    FOR_EACH_NODE_INPUT(this,p,iterator)
        delete p;

    //
    // Delete all the parameters in the Output parameter List
    //
    FOR_EACH_NODE_OUTPUT(this,p,iterator)
        delete p;

    //
    // Now that the parameters and arcs have been deleted we can delete
    // the StandIn.
    // NOTE: this depends upon the fact that deletion of the StandIn does not
    //	require reading the state of the Node (i.e. its parameters), otherwise
    //  deletion will need to be placed above Parameter deletion.
    //
    if (saved_standin)  
	delete saved_standin; 

    if (this->network)
	this->network->setDirty();

    //
    // If this is a persistent outboard module, be sure to tell the executive
    // that it has been deleted.
    //
    NodeDefinition *nd = this->definition;
    if (nd->isOutboard() && nd->isMDFFlagPERSISTENT()) {
	DXPacketIF *pif = theDXApplication->getPacketIF();
	if (pif) {
	    char buf[512];
#if WORKSPACE_PAGES
	    const char *gname = 
		this->getGroupName(theSymbolManager->getSymbol(PROCESS_GROUP));
#else
	    const char *gname = this->getGroupName();
#endif
	    if (gname)
		sprintf(buf,
			"Executive(\"outboard delete\",\"%s\",%d,\"%s\");\n",
			this->getNameString(),
			this->getInstanceNumber(),
			gname);
	    else
		sprintf(buf,
			"Executive(\"outboard delete\",\"%s\",%d);\n",
			this->getNameString(),
			this->getInstanceNumber());
	    pif->send(DXPacketIF::FOREGROUND, buf);
	    // FIXME: is the following necessary
	    // pif->sendImmediate("sync");
	}
    }

    if (this->layout_information) delete this->layout_information;
}
//
// Determine if this node is a node of the given class
//
boolean Node::isA(const char *classname)
{
    Symbol s = theSymbolManager->registerSymbol(classname);
    return this->isA(s);
}
//
// Determine if this node is of the given class.
//
boolean Node::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassNode);
    return (s == classname);
}

//////////////////////////////////////////////////////////////////////////////
//
// Set the x,y positions on the VPE (visual program editor).
// If a standin exists for this node, then ask the StandIn to do it 
// otherwise just set the values 
//
void Node::setVpePosition(int x, int y) 
{ 
    this->vpe_xpos = x;
    this->vpe_ypos = y;

    if (this->standin) 
	this->standin->setXYPosition(x,y);

}
//////////////////////////////////////////////////////////////////////////////
//
// Get the x,y positions on the VPE (visual program editor).
// If a standin exists for this node, then ask the StandIn for the position
// otherwise use the values that were read from the .net file.
//
void Node::getVpePosition(int *x, int *y) 
{ 
    if (this->standin) {
	this->standin->getXYPosition(x,y);
	// Do this in case the standin goes away, but the node does not?!
	this->vpe_xpos = *x;
	this->vpe_ypos = *y;
    } else {
	*x = this->vpe_xpos;
	*y = this->vpe_ypos;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Output the names of list of input parameters for this node.
// If on input have a value print its name as $module_$instance_$in_$input.
// If the input is taking it's value from another node's output, then print
// ${outputmodule}_${outputmodule_instance}_out_${outputmodule_output}.
//
// Note that the trailing newline ('\n') is not printed.
//
//  Like this...
//
//    Import_1_in_1,
//    Import_1_in_2,
//    Import_1_in_3,
//    Import_1_in_4,
//    Import_1_in_5,
//    Import_1_in_6
//
char *Node::inputParameterNamesString(const char *varprefix, const char *indent)
{
    const char *name ;
    char  *retstr;
    int	  i, num_params;

    name = this->getNameString();
    ASSERT(name);

    num_params = inputParameters.getSize();
    if (num_params == 0)
	return NULL;

    //
    // Get some buffers.
    //
    if (!indent)
	indent = "";
#if !defined(STATIC_MACROS)
    int paramlen = 
		(STRLEN(indent) + STRLEN(varprefix) + STRLEN(name) + 16) + 300;
    char *buf = new char[ paramlen ]; 
    retstr = new char[num_params * paramlen ];
#else
    char buf[1024];
    retstr = new char[num_params * 1024];
#endif

    //
    // Print them 
    //
    i = 1;
    *retstr = '\0';
    char *pstr = retstr;
    for (i=1 ; i<=num_params ; i++, pstr+=STRLEN(pstr))  {
	Parameter *p;
	if ((p = this->getInputParameter(i))->isConnected())
	{
	    int  param;
	    Ark  *a = p->getArk(1);
	    ASSERT(a);
	    Node *onode = a->getSourceNode(param);
	    ASSERT(onode);
	    sprintf(buf, "%s%s%s_%d_out_%d", indent,varprefix, 
			onode->getNameString(), 
			onode->getInstanceNumber(), param);
	}
	else
	{
#if defined(STATIC_MACROS)
	    //
	    // If this is a macro then print the input value now, otherwise
	    // reference a global variable and print that value later.
	    //
	    if (this->network->isMacro())
		sprintf(buf,"%s%s",indent,this->getInputValueString(i));
	    else
#endif
	    sprintf(buf,"%s%s%s_%d_in_%d", indent, varprefix, 
			name, this->instanceNumber, i);
	}
	if (i != num_params)
	    strcat(buf,",\n");
        strcat(pstr,buf);
    }

#if !defined(STATIC_MACROS)
    delete buf;
#endif
    return retstr;
}
//
// Get the name of the input as specified in the network
// (i.e. main_Display_1_in_3)
// If buf is not provided the returned string must be deleted
// by the caller.
//
char *Node::getNetworkIONameString( int index, boolean input, char *buffer)
{
    char *buf;
    const char *prefix = this->network->getPrefix();

#if defined(STATIC_MACROS)
    // 
    // Macros do not use input parameter names because we always send
    // the value of the input in the tools argument list. 
    // 
    // FIXME: now that I've removed the assert, are static macros endangered?
    //ASSERT(!this->network->isMacro());
#endif
    if (buffer)
	buf = buffer;
    else
        buf = new char[128];

    sprintf(buf,"%s%s_%d_%s_%d", 
		prefix, 
		this->getNameString(), 
		this->getInstanceNumber(),
		(input ? "in" : "out"),
		index);
     return buf;
}

//////////////////////////////////////////////////////////////////////////////
//
// Output the names of the list of output parameters for this node.
// Note that the trailing newline ('\n') is not printed.
// It prints out the cache attribute for each output if it differs from
// that of the node.
//
//  Like this...
//
//    AutoColor_2_out_1[cache: 0],
//    AutoColor_2_out_2
//
//
char *Node::outputParameterNamesString(const char *varprefix)
{
    if (this->getOutputCount() == 0)
	return NULL;

    const char *name ;
    char  *buf, *retstr, *newprefix;
    int	  i, paramlen, num_params;

    name = this->getNameString();
    ASSERT(name);

    /*io = "out";*/
    num_params = outputParameters.getSize();

    //
    // Get some buffers.
    //
    paramlen = (STRLEN(varprefix) + STRLEN(name) + 16) + 300 + 10;
    buf = new char[ paramlen ]; 
    newprefix = new char[paramlen + STRLEN(varprefix)];
    retstr = new char[num_params * paramlen ];
    sprintf(newprefix,"%s%s_%d_out_",varprefix,name,this->instanceNumber);

    //
    // Print the node name
    //
    i = 1;
    *retstr = '\0';
    char *pstr = retstr;
    for (i=1 ; i<=num_params ; i++, pstr+=STRLEN(pstr))  {
	Parameter *p = this->getOutputParameter(i);
	ASSERT(p);
	if (this->getNodeCacheability() == p->getCacheability())
	    sprintf(buf,"%s%d%s", newprefix, i, (i==num_params ? "" : ",\n"));
	else
	    sprintf(buf,"%s%d[cache: %d]%s",
		    newprefix, i, p->getCacheability(), 
		    (i==num_params ? "" : ",\n"));
	strcat(pstr,buf);
    }

    delete buf;
    delete newprefix;
    return retstr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Save any other files that relevant to this mode 
// The name passed in is file name used to save the network (without the
// .net extension).
// By default, nodes do not have auxiliary files.
//
boolean     Node::auxPrintNodeFile()
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Print the '|'d section below for this node into what will be a .net file. 
//
// FIXME: Do we need to be able to print to a string, can we just get
//		by with a FILE? 
//
/*
 *  //
 *  // node Import[1]: x = 48, y = 14, inputs = 6, label = Import
||  // input[1]: type = 64, value = "network8.dx"
 *  //
*/
boolean Node::printIOComment(FILE *f, boolean input, int i, 
				const char *indent, boolean valueOnly)
{
    int status = 1;
    if (!indent)
	indent = "";

    if (input)
    {
	Parameter *p = this->getInputParameter(i);
	// Only print it if it is not receiving input from another output
	// (i.e. it does not have an arc) 
//
// This ifdef is so that we save parameter values even if they have a
// connected arc.  This is useful for parameters whose arc-fed values
// are echoed back to the UI. This includes the data-driven interactors
// and some of Image's parameters. 	dawood 8/22/95 
//
#if 00	 
	if (!p->isConnected() && p->hasValue()) {
	    if (valueOnly) {
		status = fprintf(f,
		    "%s// input[%d]: defaulting = %d, value = %s\n",
		    indent,
		    i,
		    p->isDefaulting() ? 1 : 0,
		    p->getSetValueString());
	    } else {
		status = fprintf(f,
		    "%s// input[%d]: defaulting = %d, visible = %d,"
			" type = %d, value = %s\n",
		    indent,
		    i,
		    (p->isDefaulting() ? 1 : 0),
		    (p->isVisible()? 1 : 0),
		    p->getValueType(),
		    p->getSetValueString());
	    }
	} else if (!valueOnly && p->isViewable() &&
	         (p->isVisibleByDefault() != p->isVisible()))
#else
	if (p->hasValue()) {
	    if (valueOnly) { 
		status = fprintf(f,
		    "%s// input[%d]: defaulting = %d, value = %s\n",
		    indent,
		    i,
		    (p->isDefaulting() || p->isConnected() ? 1 : 0),
		    p->getSetValueString());
	    } else {
		// When a parameter is connected, print the value as
		// defaulting, so that when we read it back in, we do a 
		// setInputSetValue() instead of a setInputValue()
		status = fprintf(f,
		    "%s// input[%d]: defaulting = %d, visible = %d,"
			" type = %d, value = %s\n",
		    indent,
		    i,
		    (p->isDefaulting() || p->isConnected() ? 1 : 0),
		    (p->isVisible()? 1 : 0),
		    (int) p->getValueType(),
		    p->getSetValueString());
	    }
	} else if (!valueOnly && p->isViewable() &&
	         (p->isVisibleByDefault() != p->isVisible()))
#endif
	    status = fprintf(f,
		"%s// input[%d]: visible = %d\n",
		indent,
		i, (p->isVisible() ? 1 : 0));
    }
    else
    {
	Parameter *p = this->getOutputParameter(i);
	// Only print it if it is sending a value to an input 
	// (i.e. it has an arc) 
	if (p->hasValue()) {
	    if (valueOnly) {
		status = fprintf(f,
		    "%s// output[%d]: defaulting = %d, value = %s\n",
		    indent,
		    i,
		    p->isDefaulting() ? 1 : 0,
		    p->getSetValueString());
	    } else { 
		status = fprintf(f,
		    "%s// output[%d]: visible = %d, type = %d, value = %s\n",
		    indent,
		    i,
		    (p->isVisible()? 1 : 0),
		    (int) p->getValueType(),
		    p->getSetValueString());
	    }
	} else if (!valueOnly && p->isViewable() &&  
		 (p->isVisibleByDefault() != p->isVisible()))
	    status = fprintf(f,
		"%s// output[%d]: visible = %d\n",
		indent,
		i, 
		(p->isVisible() ? 1 : 0));
    }

    return status > 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// Print the '|'d section below for this node into what will be a .net file. 
//
// FIXME: for now we don't do the comments (PrintType).
// FIXME: Do we need to be able to print to a string, can we just get
//		by with a FILE? 
//
/*
||  //
||  // node Import[1]: x = 48, y = 14, inputs = 6, label = Import
||  // input[1]: type = 64, value = "network8.dx"
||  //
*/
boolean Node::netPrintCommentHeader(FILE *f)
{
    int i, status, x, y, num;
    const char *name;
    
    name = this->getNameString();
    ASSERT(name);

    status = fprintf(f,"%s// \n",__indent);
    if (status < 0)
	return FALSE;

    //
    // Print the node name comment 
    //
    this->getVpePosition(&x, &y);
    if (this->getOutputCount() != this->getDefinition()->getOutputCount())
	status = fprintf(f,"%s// node %s[%d]: x = %d, y = %d, inputs = %d, "
			   "outputs = %d, label = %s\n",
		    __indent,
		    this->getNameString(),
		    this->getInstanceNumber(), x, y, 
		    this->getInputCount(),
		    this->getOutputCount(),
		    this->getLabelString());
    else
	status = fprintf(f,"%s// node %s[%d]: x = %d, y = %d, inputs = %d, "
			   "label = %s\n",
		    __indent,
		    this->getNameString(),
		    this->getInstanceNumber(), x, y, 
		    this->getInputCount(),
		    this->getLabelString());
    if (status < 0)
	return FALSE;

    //
    // Print the inputs that have values
    //
    num = this->getInputCount();
    for (i=1 ; i<=num ; i++) {
	if (!this->printIOComment(f, TRUE, i,__indent))
	    return FALSE;
    }

    //
    // Print the outputs that have values
    //
    num = this->getOutputCount();
    for (i=1 ; i<=num ; i++) {
	if (!this->printIOComment(f, FALSE, i, __indent))
	    return FALSE;
    }

#if WORKSPACE_PAGES
    if (!this->printGroupComment(f))
#else
    if(NOT this->netPrintPgrpComment(f))
#endif
	return FALSE;

    if (!this->netPrintAuxComment(f))
	return FALSE;

    status = fprintf(f,"%s//\n",__indent);
    if (status < 0)
	return FALSE;

    return TRUE; 

}

#if WORKSPACE_PAGES
#else
boolean Node::netPrintPgrpComment(FILE *f)
{
    const char *gname = this->getGroupName();
    if(NOT gname)
	return TRUE;

    if(fprintf(f,"%s// process group: %s\n",__indent, gname) < 0)
	return FALSE;

    return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////////////////
// Print the '|'d section below for this node.  If PrintType is set
// then also print the '||'d section. In the example below 'prefix' 
// contains the 2 leading spaces.
//
//
/*
||  //
||  // node Import[1]: x = 48, y = 14, inputs = 6, label = Import
||  // input[1]: type = 64, value = "network8.dx"
||  //
 |  Import_1_out_1 =
 |       Import(
 |          Import_1_in_1,
 |          Import_1_in_2,
 |          Import_1_in_3,
 |          Import_1_in_4,
 |          Import_1_in_5,
 |          Import_1_in_6
 |      ) [instance: 1, cache: 1, pgroup: "pg1"];
*/
char *Node::netNodeString(const char *prefix)
{
    int buflen;
    const char *name;
    char *outputs, *inputs, *module, *attributes, *s;
    
    //
    // Get the name of the module that is used to do the work for this node.
    //
    name = this->getExecModuleNameString();
    ASSERT(name);
	
    //
    // Print the outputs for this node 
    //
    outputs = this->outputParameterNamesString(prefix);


    //
    // Print the Name of this node as 'Name(\n'
    //
    module = new char[STRLEN(__indent) + STRLEN(name) + 16];
    sprintf(module,"%s%s(",__indent,name);			

    //
    // Print node inputs 
    //
    inputs = this->inputParameterNamesString(prefix,__indent);

#if WORKSPACE_PAGES
    const char *gname = this->getGroupName(theSymbolManager->getSymbol(PROCESS_GROUP));
#else
    const char *gname = this->getGroupName();
#endif
    if(NOT gname)
    {
    	char *fmt = "%s) [instance: %d, cache: %d];";
    	attributes = new char[STRLEN(__indent) + STRLEN(fmt) + 32];
    	sprintf(attributes, fmt, __indent, 
			    this->instanceNumber,
			    this->getNodeCacheability());

    }
    else
    {
    	char *fmt = "%s) [instance: %d, cache: %d, group: \"%s\"];";
    	attributes = new char[STRLEN(__indent) + STRLEN(fmt) +
			      STRLEN(gname) + 32];
    	sprintf(attributes, fmt, __indent, 
			    this->instanceNumber,
			    this->getNodeCacheability(),
			    gname);
    }

    buflen = STRLEN(module) + STRLEN(outputs) + 
		STRLEN(inputs) + STRLEN(attributes); 
    s = new char[buflen+40];
    if (this->getOutputCount() == 0)
    {
	sprintf(s,"%s\n%s\n%s\n",
		    module,(inputs ? inputs : "") ,attributes); 
    }
    else
    {
	sprintf(s,"%s = \n%s\n%s\n%s\n",
		    (outputs ? outputs : ""),
		    module,
		    (inputs ? inputs : ""),
		    attributes); 
    }

    if (outputs) delete outputs;
    delete module;
    if (inputs)  delete inputs;
    delete attributes;

    return s;
}
//
// Do any work that must be done before sending the macro/network
// to the server. 
// Be default, Nodes do not have any work that needs to be done for sending. 
//
void Node::prepareToSendNode()
{
}
void Node::prepareToSendValue(int index, Parameter *p)
{
}

boolean Node::netPrintNode(FILE *f, PrintType dest, const char *prefix,
		PacketIFCallback callback, void *clientdata)
{
    char *s;
    DXPacketIF *pif = theDXApplication->getPacketIF();
    boolean r = TRUE;

    if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
        if (!this->netPrintCommentHeader(f))
	    return FALSE;
    } else if (pif) {	// We have a connection to the executive/server
  	//
  	// If this node has a message protocol with the executive, then
  	// update any state associated with the protocol.
  	//
	if (this->hasModuleMessageProtocol()) 
	    this->updateModuleMessageProtocol(pif);
  	//
  	// Do any work that must be done before executing the macro/network
  	// that this node belongs to.
  	//
	this->prepareToSendNode();
    }
   
    s = this->netNodeString(prefix);


    if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
	if (fputs(s, f) < 0)
	    r = FALSE;
	if (callback != NUL(PacketIFCallback))
	    callback(clientdata,s);
    } else {
	ASSERT (dest == PrintExec);
	pif->sendBytes(s);
    }

    delete s;
	
    return r;

}

//
// Returns a string that is used to register this->ExecModuleMessageHandler() 
// when this->hasModuleMessageProtocol() return TRUE.
// This version, returns an id that is unique to  this instance of this node.
//
const char *Node::getModuleMessageIdString()
{

   if (!this->moduleMessageId) {
       const char *name = this->getNameString();
       int n = STRLEN(name) + 16;
       this->moduleMessageId = new char[n];

       sprintf(this->moduleMessageId,"%s_%d",name, this->getInstanceNumber());
   }

   return (const char*)this->moduleMessageId;
}

//
// Update the state of message handling for a module/UI message. 
// This is called only when we send a Node's module call to the executive
// and the node has a module messaging protocol as defined by
// this->hasModuleMessageProtocol(). 
//
void Node::updateModuleMessageProtocol(DXPacketIF *pif)
{
    const char *id = this->getModuleMessageIdString();

	if (this->expectingModuleMessage()) {
		//
		// Install a callback to handle messages from the module 
		//
		pif->setHandler(DXPacketIF::INFORMATION,
			Node::ExecModuleMessageHandler,
			(void*)this, id);
	} else  {
		//
		// Remove the handler in case it was previously installed.
		//
		pif->setHandler(DXPacketIF::INFORMATION,
			NULL,   (void*)this, id);
	}
}
//
// Return TRUE/FALSE, indicating whether or not we support a message protocol 
// between the executive module that runs for this node and the  UI.
// Be default Nodes do not have message protocols.
// 
boolean Node::hasModuleMessageProtocol()
{
    return FALSE;
}
//
// Return TRUE/FALSE, indicating whether or not we expect to receive
// a message from the UI when our module executes in the executive. 
// Be default Nodes do not expect messages.
// 
boolean Node::expectingModuleMessage()
{
    return FALSE;
}
//
// This dispatches messages to this->execModuleMessageHandler().
// Messages are not noticed unless this handler is installed
// (in this->netPrintNode()).
//
void Node::ExecModuleMessageHandler(void *clientData, int id, void *line)
{
   Node *node = (Node*)clientData;
   node->execModuleMessageHandler(id,(const char*)line);
}
//
// Called when a message is received from the executive after
// this->ExecInfoMessageHandler() is registered to receive messages
// for this node.  The format of the message coming back is defined
// by the derived class.
//
void Node::execModuleMessageHandler(int id, const char *line)
{
    fprintf(stderr,"Node: Saw unexpected module message, id %d...\n",id);
    fprintf(stderr,"%s\n",line);
}


#if 0
//////////////////////////////////////////////////////////////////////////////
//
// Get name of a parameter and its value 
// The format of the string is 'Parameter name = value;'.
// index is 1 based.
// It returns the string representing the assignment. 
// The returned string must be deleted by the caller.
//
int Node::getParameterNameEqualsValue(Parameter *p, 
					const char *prefix, int index)
{
    const char *pval;
    char *p, *retstr, pname[512];

    pname[0] = '\0';
    this->strcatParameterNameLvalue(pname, p, prefix, index);
    strcat(s, " = ");
    if (p->isInput())
	pval = this->getInputValueString(index);
    else
	pval = this->getOutputValueString(index);

    retstr = new char[STRLEN(pname) + STRLEN(pval) + 2];

    p = retstr;
    strcpy(retstr,pname);
    p += STRLEN(retstr);
    strcat(p,v);
    p += STRLEN(p);
    strcat(p,";");

    return retstr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Concatenate the name of a parameter but not its value to the given string. 
// index is 1 based.  it returns the ammount of stuff added to s
//
int Node::strcatParameterNameLvalue(char *s, Parameter *p, 
	const char *prefix, int index)
{

    const char *ioname;

    if (p->isInput())
	ioname = "in";
    else
	ioname = "out";

    ASSERT(s);
    s += STRLEN(s);
    return SPRINTF(s,"%s%s_%d_%s_%d",
		prefix, this->getNameString(),this->getInstanceNumber(), 
		ioname, index);
}
int Node::strcatParameterValueString(char *s, Parameter *p, int index)
{
    const char *v = p->getValueString();

    if (s)
	strcat(s, v);
    return STRLEN(v);
}
///////////////////////////////////////////////////////////////////////////////
//
// Print the value of the index'th parameter if it contains a value
// (i.e. it's arc list is empty).  Returns NULL if no string is to be printed.
// index is 1 based.
// The returned string must be deleted by the caller.
// 
char *Node::ioValueString(List *io, int index, const char *prefix)
{
    char *retstr = NUL(char*);
    Parameter *p;

    ASSERT(index >= 1);

    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    if (p->isNeededValue()) {		
	const char *pval;
	int pname_size;
	char *c, pname[512];

	pname[0] = '\0';
	pname_size = this->strcatParameterNameLvalue(pname, p, prefix, index);
	strcat(pname, " = ");
	pname_size += 3;
	if (p->isInput())
	    pval = this->getInputValueString(index);
	else
	    pval = this->getOutputValueString(index);

	retstr = new char[pname_size + STRLEN(pval) + 2];

	c = retstr;
	strcpy(retstr,pname);
	c += STRLEN(retstr);
	strcat(c,pval);
	c += STRLEN(c);
	strcat(c,";");

    }
    return retstr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Print the value of the input and output parameters  if they contain a value
// (i.e. it's arc list is empty).  
// index is 1 based.
// Returns NULL if no values need to be printed, otherwise a string containing
// parameter value pairs.
//
char *Node::valuesString(const char *prefix)
{
     
    char  *s=NUL(char*), *buf;
    int   size, cnt, i;
    boolean first = TRUE;;

#if defined(STATIC_MACROS)
    // 
    // Macros do not use global variables as inputs so we do not 
    // need to send the input value assignments.
    // 
    if (!this->network->isMacro())  {
#endif
    //
    // Print the input values
    //
    cnt = this->getInputCount();
    for (i=1 ; i<=cnt ; i++) {
	if ( (buf = this->inputValueString(i,prefix)) )
	{
	    if (s) 
		first = FALSE;
	    size = STRLEN(buf) + STRLEN(s) + 2;
	    s = (char*)REALLOC(s,size);
	    if (first) 
		s = strcpy(s, buf);
	    else
		s = strcat(s,buf);
	    strcat(s,"\n"); 
	    delete buf;
	}
    }
#if defined(STATIC_MACROS)
    }
#endif

    //
    // Print the output values
    //
    cnt = this->getOutputCount();
    for (i=1 ; i<=cnt ; i++) {
	if ( (buf = this->outputValueString(i,prefix)) )
	{
	    if (s) 
		first = FALSE;
	    size = STRLEN(buf) + STRLEN(s) + 2;
	    s = (char*)REALLOC(s,size);
	    if (first) 
		s = strcpy(s, buf);
	    else
		s = strcat(s,buf);
	    strcat(s,"\n"); 
	    delete buf;
	}
    }

    return s;

}

boolean Node::printValues(FILE *f, const char *prefix, PrintType dest)
{
    char *s;
    boolean r = TRUE;

    s = this->valuesString(prefix);
    if (s) {
	if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
	    if (fputs(s, f) < 0)
		r = FALSE;
	} else {
	    ASSERT (dest == PrintExec);
	    DXPacketIF* pif = theDXApplication->getPacketIF();
	    pif->sendBytes(s);
	}

	delete s;
    }
    return r;
}

///////////////////////////////////////////////////////////////////////////////
//
// Send all values that are in one expression.
boolean Node::sendValues(boolean ignoreDirty)
{
    DXPacketIF *pif  = theDXApplication->getPacketIF();

    if (!pif)
	return TRUE;

    const char *prefix = this->network->getPrefix();
    int i, cnt;
    char *names=NULL;
    char *values=NULL;
    int nameLen = 0;
    int valueLen = 0;

    //
    // Print the dirty inputs
    //
    cnt = this->getInputCount();
    ListIterator li(this->inputParameters);
    Parameter *p;
#if defined(STATIC_MACROS)
    // 
    // Macros do not use global variables as inputs so we do not 
    // need to send the input value assignments.
    // 
    if (!this->network->isMacro())  {
#endif
		for (i=1 ; i<=cnt && NULL != (p = (Parameter *)li.getNext()); i++)
		{
			if (p->isNeededValue(ignoreDirty))
			{
				//
				// Do any work that is necessary before sending the value .
				//
				this->prepareToSendValue(i, p);
				if (nameLen == 0)
				{
					names = (char *)MALLOC(100);
					*names = '\0';
					nameLen = this->strcatParameterNameLvalue(names, p, prefix, i);

					int l = this->strcatParameterValueString(NULL, p, i);
					values = (char *)MALLOC(l+10);
					*values = '\0';
					valueLen = this->strcatParameterValueString(values, p, i);
				}
				else
				{
					names = (char *)REALLOC((void*)names, nameLen + 100 + 2);
					strcat(names, ", ");
					nameLen += 2 + this->strcatParameterNameLvalue(names, p, prefix, i);

					int l = this->strcatParameterValueString(NULL, p, i);
					values = (char *)REALLOC((void*)values, valueLen + l+10);
					strcat(values, ", ");
					valueLen += 2 + this->strcatParameterValueString(values, p, i);
				}
				p->clearDirty();
			}
		}
#if defined(STATIC_MACROS)
    }
#endif

    //
    // Print the outputs that have values
    //
    cnt = this->getOutputCount();
    li.setList(this->outputParameters);
    for (i=1 ; i<=cnt && NULL != (p = (Parameter *)li.getNext()); i++)
    {
	if (p->isNeededValue(ignoreDirty))
	{
	    //
	    // Do any work that is necessary before sending the value .
	    //
	    this->prepareToSendValue(i, p);

	    if (nameLen == 0)
	    {
		names = (char *)MALLOC(100);
		*names = '\0';
		nameLen = this->strcatParameterNameLvalue(names, p, prefix, i);

		int l = this->strcatParameterValueString(NULL, p, i);
		values = (char *)MALLOC(l+10);
		*values = '\0';
		valueLen = this->strcatParameterValueString(values, p, i);
	    }
	    else
	    {
		names = (char *)REALLOC((void*)names, nameLen + 100 + 2);
		strcat(names, ", ");
		nameLen += 2 + this->strcatParameterNameLvalue(names, p, prefix, i);

		int l = this->strcatParameterValueString(NULL, p, i);
		values = (char *)REALLOC((void*)values, valueLen + l+10);
		strcat(values, ", ");
		valueLen += 2 + this->strcatParameterValueString(values, p, i);
	    }
	    p->clearDirty();
	}
    }

    if (nameLen > 0)
    {
	char *s = new char[nameLen + 3 + valueLen + 2];
	strcpy(s, names);
	strcat(s, " = ");
	strcat(s, values);
	strcat(s, ";");
	pif->send(DXPacketIF::FOREGROUND, s);
	delete s;
	FREE((void*)names);
	FREE((void*)values);
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// Parse the .net file comment section for this node. 
//
// Set the instance number, the x,y position, and its inputs. 
//
boolean Node::netParseComment(const char* comment, 
		const char* filename, int lineno)
{
    ASSERT(comment);

    /*
     * Ignore comments that we do not recognize 
     */
    return this->netParseNodeComment(comment, filename, lineno) ||
		this->parseIOComment(TRUE, comment, filename, lineno) ||
		this->parseIOComment(FALSE, comment, filename, lineno) ||
#if WORKSPACE_PAGES
		this->parseGroupComment(comment, filename, lineno) ||
#else
		this->netParsePgrpComment(comment, filename, lineno) ||
#endif
		this->netParseAuxComment(comment,filename,lineno);
}

//
// Empty virtual method to parse comments (if any) that come after the 
// 'node', 'input', 'ouput' and 'process group' comments in the .net file. 
//
boolean Node::netParseAuxComment(const char* comment,
                const char* filename, int lineno)
{
    return FALSE;
}
// 
// Empty virtual method to be overridden by subclasses that prints out
// node specific comments after the standard comments in the .net file.
// 
boolean Node::netPrintAuxComment(FILE *f)
{
    return TRUE;
}

#if WORKSPACE_PAGES
#else
boolean Node::netParsePgrpComment(const char* comment,
                		  const char* filename, int lineno)
{
    char *name;

    if (!EqualSubstring(" process group:",comment,15))
	return FALSE;

    name = strchr(comment, ':');
    name++;
    SkipWhiteSpace(name);

    if(NOT name)    return FALSE;

    this->addToGroup(name);
    return TRUE;
}
#endif


//
// Parse an 'input[i]' or 'output[i]' .net file comment. 
//
boolean Node::parseIOComment(boolean input, const char* comment,
							 const char* filename, int lineno, boolean valueOnly)
{
	int      defaulting = 0, allowed_params, items_parsed, ionum, r, type_tmp;
	int  visible = TRUE;
	Type     type = DXType::UndefinedType;
	char     *value, *ioname;
	boolean	parse_error = FALSE;

	ASSERT(comment);

	if (input) {
		if (!EqualSubstring(" input[",comment,7))
			return FALSE;

		if (valueOnly) {
			if (sscanf(comment, " input[%d]: defaulting = %d", 
				&ionum, &defaulting) != 2)
				parse_error = TRUE;
			type = DXType::UndefinedType;
		} else {
			items_parsed = sscanf(comment,
				" input[%d]: defaulting = %d, visible = %d, type = %d", 
				&ionum, &defaulting, &visible, &type_tmp);
			type = (Type) type_tmp;

			if (items_parsed != 4) {
				// Invisibility added 3/30/93
				items_parsed = sscanf(comment, " input[%d]: visible = %d", 
					&ionum, &visible);
				if (items_parsed != 2) {
					// Backwards compatibility added 3/25/93
					items_parsed = sscanf(comment, " input[%d]: type = %d", 
						&ionum, &type_tmp);
					type = (Type) type_tmp;

					if (items_parsed != 2) {
						items_parsed = sscanf(comment,
							" input[%d]: defaulting = %d, type = %d", 
							&ionum, &defaulting, &type_tmp);
						type = (Type) type_tmp;
						if (items_parsed != 3) 
							parse_error = TRUE;
					} 
				}
				else 
				{
					defaulting = 1;
				}
			}
		}
		ioname = "input";
		allowed_params = this->getInputCount();
	} else {	// An output

		if (!EqualSubstring(" output[",comment,8))
			return FALSE;

		if (valueOnly) {
			if (sscanf(comment, " output[%d]: defaulting = %d", 
				&ionum, &defaulting) != 2)
				parse_error = TRUE;
			type = DXType::UndefinedType;
		} else {
			// Invisibility added 3/30/93
			items_parsed = sscanf(comment, 
				" output[%d]: visible = %d, type = %d", 
				&ionum, &visible, &type_tmp);
			type = (Type) type_tmp;

			if (items_parsed != 3) {

				items_parsed = sscanf(comment, " output[%d]: type = %d", 
					&ionum, &type_tmp);
				type = (Type) type_tmp;

				if (items_parsed != 2) {
					items_parsed = sscanf(comment, " output[%d]: visible = %d", 
						&ionum, &visible);
					if (items_parsed != 2)
						parse_error = TRUE;
				}
			}
		}
		ioname = "output";
		allowed_params = this->getOutputCount();
	}

	if (parse_error)
	{
		ErrorMessage ("Can't parse %s comment file %s line %d", 
			ioname, filename, lineno);
		return TRUE;
	}
	/*
	* If the input parameter is out of bounds, then something is wrong...
	*/
	if (ionum > allowed_params)	
	{
		ErrorMessage ("Bad %s number (%d) file %s line %d", 
			input? "input": "output", ionum, filename, lineno);
		return TRUE;
	}



	/*
	* If parsed ok and node exists, convert value.
	*/
	value = (char *) strstr(comment, "value =");
	if (value != NUL(char*))
	{
		value = strchr(value,'=') + 2;

		//
		// When we went to the C++ ui, the bit masks for types changed.
		// If we're reading a pre-UI++ network then convert the types.
		// valueOnly was added in version 3.
		//
		if (this->getNetwork()->getNetMajorVersion() <= 1)
			type = DXType::ConvertVersionType(type);


		if (input) {
			if (value[0] == '(' && value[STRLEN(value)-1] == ')') {
				// Skip descriptive settings
				defaulting = 1;
				r = DXType::ObjectType;
#if 11
			} else if (defaulting) {
#else
				//
				// This check for NULL shouldn't really be necessary, except
				// that there is a bug somewhere in ConfigurationDialog
				// that seems too risky to fix just before 3.1
				// Bug is as follows, 
				//   1) Place Echo
				//   2) Open CDB and type NULL into first param
				//   3) Save net
				//   4) Read in net.
				//   5) Execute and nothing goes in MsgWin, which is correct
				//   6) Open CDB, note non-null value in CDB and execute and get
				//	  	a different result.
				//
			} else if (defaulting || EqualString(value,"NULL")) {
#endif
				r = this->setInputSetValue(ionum, value, type, FALSE);
				if (r == DXType::UndefinedType && 
					type != DXType::UndefinedType) 	
					r = this->setInputSetValue(ionum, value, 
					DXType::UndefinedType, FALSE);
			} else {
				r = this->setInputValue(ionum, value, type, FALSE);
				if (r == DXType::UndefinedType && 
					type != DXType::UndefinedType) 	
					r = this->setInputValue(ionum, value, 
					DXType::UndefinedType, FALSE);
			}
		} else {
			r = this->setOutputValue(ionum, value, type, FALSE);
			if (r == DXType::UndefinedType && 
				type != DXType::UndefinedType) 	
				r = this->setOutputValue(ionum, value, DXType::UndefinedType, 
				FALSE);
		}

		if (r == DXType::UndefinedType) {
			ErrorMessage(
				"Encountered an erroneous input value (file %s, line %d)",
				filename, lineno);
			return TRUE;
		}
	}

	if (!valueOnly) {
		if (input) {
			this->setInputVisibility(ionum, (boolean)visible);
		} else {	// Outputs always use the value found in the file
			this->useAssignedOutputValue(ionum, FALSE);
			this->setOutputVisibility(ionum, (boolean)visible);
		}
	}


	return TRUE;

}


boolean Node::netParseNodeComment(const char* comment,
								  const char* filename, int lineno)
{
	int        items_parsed;
	NodeDefinition *nd;
	int        instance;
	int        x;
	int        y;
	int        n_inputs;
	int        n_outputs;
	int        i;
	char       node_name[1024];
	char       labelstr[1024];

	/*
	* Parse the comment.
	*/

	if (!EqualSubstring(" node ",comment,6))
		return FALSE;

	items_parsed =	// Version DX/6000 2.0 style comments 
		sscanf
		(comment,
		" node %[^[][%d]: x = %d, y = %d, inputs = %d, outputs = %d, label = %[^\n]",
		node_name,
		&instance,
		&x,
		&y,
		&n_inputs,
		&n_outputs,
		labelstr);

	if (items_parsed != 7) {	// Try pre Version DX/6000 2.0 style comments
		items_parsed =	// Version DX/6000 1.2 style comments 
			sscanf
			(comment,
			" node %[^[][%d]: x = %d, y = %d, inputs = %d, label = %[^\n]",
			node_name,
			&instance,
			&x,
			&y,
			&n_inputs,
			labelstr);

		if (items_parsed != 6) {// Try pre Version DX/6000 1.2 style comments
			items_parsed =
				sscanf(comment,
				" node %[^[][%d]: x = %d, y = %d, label = %[^\n]",
				node_name,
				&instance,
				&x,
				&y,
				labelstr);
			if (items_parsed != 5)
			{
				// FIXME : should be ModelessErrorMessageDialog 
#ifdef NOT_YET
				ErrorMessage
					("#10001", "node", filename, lineno);
				_error_occurred = TRUE;
#else
				ErrorMessage("Can not parse node comment at "
					"line %d in file %s",
					lineno, filename);
#endif
				return TRUE;
			}
		}
	}

	/*
	* Get definition for this node. 
	*/
	nd = (NodeDefinition*) 
		theNodeDefinitionDictionary->findDefinition(node_name);
	if (!nd) {
		ErrorMessage("Undefined module %s at line %d in file %s",
			node_name, lineno, filename); 
		return FALSE;
	}

	//    printf ("Number of inputs found = %d, number defined = %d\n",
	//		n_inputs, this->definition->getInputCount());
	/*
	* If old syle comment, set number of inputs to that of module definition.
	*/
	if (items_parsed < 7)
		n_outputs = this->getOutputCount();
	if (items_parsed < 6)
		n_inputs =  this->getInputCount();

	//
	// Set the label for this module.
	//
	this->setLabelString(labelstr);

	/*
	* Set the instance number in the node.
	*/
	this->setInstanceNumber(instance);
	this->setVpePosition(x,y);


	/*
	* Count the inputs, if not the default and the inputs are repeatable 
	* then add some inputs.
	* If there are fewer inputs in the file, silently assume that
	* this is an old network.
	*/
	if (n_inputs != this->getInputCount()) {
		if (this->isInputRepeatable()) {
			int delta_inputs = n_inputs - this->getInputCount();
			boolean adding = delta_inputs > 0;
			if (!adding) 
				delta_inputs = -delta_inputs;
			int sets; 
			if (delta_inputs % this->getInputRepeatCount() != 0) {
				ErrorMessage("Number of repeatable input parameters does not "
					"divide number of parameters for module %s",
					this->getNameString());
				return TRUE;
			}
			sets = delta_inputs/getInputRepeatCount();
			for (i=0  ; i<sets ; i++) {
				if (adding) {
					if (!this->addInputRepeats()) {
						ErrorMessage("Can't add repeated input parameters");
						return TRUE;
					}
				} else {
					if (!this->removeInputRepeats()) {
						ErrorMessage("Can't remove repeated input parameters");
						return TRUE;
					}
				}
			}
		}
		// else expect the network parser to recognize that the definition
		// has changed and indicate so.	
	}

	if (n_outputs != this->getOutputCount()) {
		if (this->isOutputRepeatable()) {
			int delta_outputs = n_outputs - this->getOutputCount();
			boolean adding = delta_outputs > 0;
			if (!adding) 
				delta_outputs = -delta_outputs;
			int sets; 
			if (delta_outputs % this->getOutputRepeatCount() != 0) {
				ErrorMessage("Number of repeatable output parameters does "
					"not divide number of parameters for module %s",
					this->getNameString());
				return TRUE;
			}
			sets = delta_outputs/getOutputRepeatCount();
			for (i=0  ; i<sets ; i++) {
				if (adding) {
					if (!this->addOutputRepeats()) {
						ErrorMessage("Can't add repeated output parameters");
						return TRUE;
					}
				} else {
					if (!this->removeOutputRepeats()) {
						ErrorMessage("Can't remove repeated output parameters");
						return TRUE;
					}
				}
			}
		}
		// else expect the network parser to recognize that the definition
		// has changed and indicate so.	
	}


	/*
	* Increment the module instance count if the current
	* node instance count is higher.
	*/
	if (instance > this->definition->getCurrentInstance())
		this->definition->setNextInstance(instance+1);

	return TRUE;

}

//
// Set the stored value.
// If the parameter is not defaulting, this is
// the same as setValue, but if it is defaulting, then we set the
// value but leave the parameter clean and defaulting and ignore 'send'.
//
// Internal note: We handle our own notification (i.e. calls to 
// ioParameterStatusChanged()) so that multiple intermediate notifications
// are avoided. 
//
Type Node::setInputSetValue(int index, const char *value, 
				Type type,
				boolean send)
{

    boolean was_defaulting = this->isInputDefaulting(index);

    //
    // If the parameter is already set (i.e. tab down) then just do
    // a normal set and return.
    //
    if (!was_defaulting)
	return this->setInputValue(index,value,type,send);
	
    //
    // First set the value, don't send it and don't do notification (this
    // is the same as a normal set except we don't do notification).
    //
    type = this->setIOValue(&this->inputParameters, index, value, 
						type, FALSE, FALSE);

    if (type != DXType::UndefinedType) {
	//
	// Second, set the parameter back to defaulting, but again, don't
	// do notification (again, this is the same as a normal setting of 
	// a parameter to defaulting, but no notification).
	//
	this->setIODefaultingStatus(index, TRUE, TRUE, FALSE, FALSE);

	//
	// Third, clear the parameter's dirty bit as we are only changing
	// the set value (i.e. not the value the exec knows about).
	//
	this->clearInputDirty(index);

	//
	// Now do the notification that we've jumped through hoops to
	// get right!
	//
	this->notifyIoParameterStatusChanged(TRUE, index, 
				Node::ParameterSetValueChanged);
    }

    return type;

}


//
//  Mark the given parameter as clean.
//
void Node::setIODirty(List *io, int index, boolean dirty)
{
    ASSERT(index >= 1);
    Parameter *p = (Parameter*)io->getElement(index);
    ASSERT(p);
    if (dirty)
        p->setDirty();
    else
        p->clearDirty();
#if defined(STATIC_MACROS)
    // 
    // If an input parameter is being marked dirty and it  is in a macro
    // then we need to resend the macro since the parameters value is
    // contained within the macro definition. 
    // 
    if (io == &this->inputParameters && this->network->isMacro())
	this->network->setDirty();
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the index'th parameter value of the parameter list given by io
// to the given value  of type t.  if t==DXType::UndefinedType, then
// be sure it can be assigned to one of the allowed types in the 
// ParameterDefinition.  If 'value' is NULL, then clear the value (and handle
// as a successful setting) and return the default type for the given 
// parameter.
// We you the Parameter methods to try and help certain values, become
// the given type (i.e. by adding "'s, {}'s, []'s and so on). 
// If send is true (the default), the results will be sent to the server
// if possible.
// If notify is TRUE, then call ioParameterStatusChanged() with one of
// Node::ParameterSetValueChanged and Node::ParameterValueChanged.
// index is 1 based.
//
//
Type Node::setIOValue(List *io,
		      int index,
		      const char *value,
		      Type t,
		      boolean send,
		      boolean notify)
{
    ASSERT( index >= 1 );
 
    Parameter *p = (Parameter*)io->getElement(index);
    ASSERT(p); 
    boolean was_set = !p->isDefaulting();

    Type type = DXType::UndefinedType;
    if (t == DXType::UndefinedType)
     	type = p->setValue(value);
    else if (p->setValue(value, t))
	type = t;

    //
    // If a NULL value is found (i.e. clearing the value), then return the
    // type as the default type for the parameter as "NULL" should match any
    // type.
    //
    if (!value) 
        type = p->getDefaultType();

    if (type != DXType::UndefinedType) {	// Success?
	//
	// And now send the value to the executive if there is a connection.
	// Note that we don't need to set the network dirty, because we can
	// send the changes ourselves.
	//
	if (send)
	{
	    DXPacketIF *pif = theDXApplication->getPacketIF(); 
	    if (pif != NUL(DXPacketIF*)) 
		 this->sendValues(FALSE);
	}

	if (notify) {
	    //
	    // Let those who need to know that the value has changed.
	    //
	    this->notifyIoParameterStatusChanged(io == &this->inputParameters, index,
				was_set ?  
				    Node::ParameterSetValueChanged :
				    Node::ParameterValueChanged);
	}
     }
     return type;
}

//
// This is similar to setIOValue.
// We update the values internally, and send the shadowing input
// value back to the executive with an Executive() call.
// We use the Executive call instead of a direct assignment, because if
// we are currently in execute-on-change  mode, the assignment would cause
// extra executions.  The Executive() call (a dictionary update) avoids that.
//
Type Node::setIOValueQuietly(List *io, int index, const char *value, Type t)
{
    t = this->setIOValue(io, index, value, t, FALSE);
    if (t == DXType::UndefinedType)
	return t;

    this->sendValuesQuietly();

    return t;
}

//
// Send all dirty input and output values to the executive in the
// quiet way using the Executive("assign noexecute",...); call.
//
void Node::sendValuesQuietly()
{
    DXPacketIF *pif = theDXApplication->getPacketIF();
#define MAXMSGLEN 4096  // Longest message that the exec can accept
    char msg[MAXMSGLEN];
    char varname[500];
    const char *varval;
    int i;

#if defined(STATIC_MACROS)
    // 
    // Values sent quietly are always global variables.  Tool inputs
    // inside of macros are never sent as global variables so we want 
    // to be sure we are not trying to do this. 
    // 
    // FIXME: now that I've removed the assert, are static macros endangered?
    //ASSERT(!this->network->isMacro());
#endif
    if (!pif)
        return;

    for (i=1 ; i<=this->getInputCount() ; i++) {
        Parameter *p = this->getInputParameter(i);
        if (p->isNeededValue(FALSE)) {
            (void)this->getNetworkInputNameString(i, varname);
            varval = this->getInputValueString(i);
            sprintf(msg,"Executive(\"assign noexecute\",\"%s\",%s);",
                                        varname,varval);
            pif->send(DXPacketIF::FOREGROUND, msg);
            p->clearDirty();
        }
    }
    for (i=1 ; i<=this->getOutputCount() ; i++) {
        Parameter *p = this->getOutputParameter(i);
        if (p->isNeededValue(FALSE)) {
            (void)this->getNetworkOutputNameString(i, varname);
            varval = this->getOutputValueString(i);
            sprintf(msg ,"Executive(\"assign noexecute\",\"%s\",%s);",
                                        varname,varval);
            pif->send(DXPacketIF::FOREGROUND, msg);
            p->clearDirty();
        }
    }


}

//
// Notify anybody that needs to know that a parameter has changed its
// value.
//
void Node::notifyIoParameterStatusChanged(boolean input, int index, 
                                NodeParameterStatusChange status) 
{
    if (!this->network->isDeleted()) 
	this->ioParameterStatusChanged(input, index, status);
}


void Node::ioParameterStatusChanged(boolean input, int index, 
				NodeParameterStatusChange status)
{

    // 
    // If we have Configuration Dialog, let it know the value, arc or 
    // visibility was changed.
    //
    if (this->cdb)  {
	if (input)
	    this->cdb->changeInput(index);
	else
	    this->cdb->changeOutput(index);
    }

    //
    // Now notify all nodes receiving this output that the value has changed.
    //
    if (!input && (status & PARAMETER_VALUE_CHANGED)) { 
	Ark *a;
	List *l = (List*) this->getOutputArks(index);
	ListIterator iterator(*l);

	while ( (a = (Ark*)iterator.getNext()) ) {
	    int in_index;
	    Node *n = a->getDestinationNode(in_index); 
	    n->notifyIoParameterStatusChanged(TRUE, in_index, status);
	}
    }

    //
    // Let the standin know about this. We don't notify StandIns about
    // arc changes since they are the ones that generate them. 
    //
    if (this->standin && !(status & PARAMETER_ARC_CHANGED)) 
	this->standin->ioStatusChange(index, !input, status);

    //
    // Tell the network that it has changed. 
    //
#if defined(STATIC_MACROS)
    //
    // If this is an input and the tool is in a macro, then we must always
    // resend the macro defnition because input values are contained within
    // the macro definition.  Otherwise we only need to mark the network
    // dirty if an arc has changed.
    //
    if (input && this->network->isMacro())
	this->network->setDirty();
    else 
#endif
    if (status & PARAMETER_ARC_CHANGED)
	this->network->setDirty();
    else
	this->network->setFileDirty();
}
//
// Add an Ark to to the index'th parameter of the parameter list given by io. 
// index is 1 based.
//
boolean Node::addIOArk(List *io, int index, Ark *a) 
{

     Parameter *p;
     ASSERT( index >= 1 );
 
     p = (Parameter*)io->getElement(index);

     ASSERT(p); 
     if (!p->addArk(a))
	return FALSE;

     this->notifyIoParameterStatusChanged(io == &this->inputParameters, index,
				Node::ParameterArkAdded);
     return TRUE;

}
////////////////////////////////////////////////////////////////////////////////
// 
// Determine if the index'th parameter in the give list is connected
// (i.e. has an arc) to another parameter.
// 
boolean Node::isIOConnected(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->isConnected();
}
////////////////////////////////////////////////////////////////////////////////
// 
//  is the index'th parameter defaulting?
// 
boolean Node::isIODefaulting(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->isDefaulting();
}
////////////////////////////////////////////////////////////////////////////////
// 
//  is the index'th parameter defaulting?
// 
boolean Node::isIOSet(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->hasValue();
}
////////////////////////////////////////////////////////////////////////////////
// 
//  Get the default value of the index'th parameter in the given list.
// 
const char *Node::getIODefaultValueString(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    ParameterDefinition *d = p->getDefinition();

    return d->getDefaultValue();
}
////////////////////////////////////////////////////////////////////////////////
// 
//  Get the value of the index'th parameter in the given list.
// 
const char *Node::getIOValueString(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->getValueString();
}
////////////////////////////////////////////////////////////////////////////////
// 
//  Get the type of the set value of the index'th parameter in the given list.
// 
Type Node::getIOSetValueType(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return (p->hasValue() ? p->getValueType() : DXType::UndefinedType);
}
////////////////////////////////////////////////////////////////////////////////
// 
//  Get the set value of the index'th parameter in the given list, ignoring
//  whether it's defaulting or not (NULL if not set).
// 
const char *Node::getIOSetValueString(List *io, int index)
{

    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->getSetValueString();
}
////////////////////////////////////////////////////////////////////////////////
//
// Get the name of the parameter (as it appears in the MDF).
// If there are repeatable parameter and the index indicates one of
// the repeatable parameters (not in the first set), then add an index#
// For example, Compute uses the following names
//              "expression", "input", "input1", "input2",...
//
char *Node::getIONameString(List *io, int index, char *buf)
{
    Parameter *p;
    ASSERT( index >= 1 );
    int repeat_num;     // The number on the MDF's REPEAT line
    int mdf_params=0;    // The number of in/outputs including the first
			// set of inputs/outputs 
    boolean input = (io == &this->inputParameters);
    char pname[128];
    const char *rval;
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    rval = p->getNameString();

    if ((input && this->isInputRepeatable()) ||
        (!input && this->isOutputRepeatable())) {
	NodeDefinition *def = this->getDefinition();
 	if (input) {
	    repeat_num = def->getInputRepeatCount();
	    mdf_params = def->getInputCount();
	} else {
	    repeat_num = def->getOutputRepeatCount();
	    mdf_params = def->getOutputCount();
	}
    } else {
        repeat_num = 0;
    }
   

    //
    // If there are repeatable parameter and the index indicates one of
    // the repeatable parameters (not in the first set), then add an index
    // For example, Compute uses the following names
    //          "expression", "input", "input1", "input2",...
    //
    if ((repeat_num != 0) && (index > mdf_params)) {
        index = (index - mdf_params - 1) / repeat_num + 1;
        sprintf(pname,"%s%d",rval,index);
        rval = pname;
    }

    if (buf) {
        strcpy(buf,rval);
        rval = buf;
    } else {
        rval = DuplicateString(rval);
    }

    return (char*)rval;

}

////////////////////////////////////////////////////////////////////////////////
// 
// Get a list of strings that represent the allowed types for the index'th
// parameter in the give list.  The return list is null terminated.
// 
const char * const *Node::getIOTypeStrings(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->getTypeStrings();
}

////////////////////////////////////////////////////////////////////////////////
// 
// Get a list of allowed types for the index'th
// parameter in the give list.  The list must NOT be deleted by the caller.
// 
List *Node::getIOTypes(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->getDefinition()->getTypes();
}

////////////////////////////////////////////////////////////////////////////////
// 
// Get the list of arcs for the index'th parameter in the given list. 
// 
const List *Node::getIOArks(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return (const List*)&p->arcs;
}

////////////////////////////////////////////////////////////////////////////////
// 
// Return TRUE/FALSE indicating whether or not the give parameter from
// the given list is set to be visible.
// 
boolean Node::isIOVisible(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->isVisible();
}
////////////////////////////////////////////////////////////////////////////////
// 
// Set TRUE/FALSE indicating whether or not the give parameter from
// the given list is set to be visible.
// 
void Node::setIOVisibility(List *io, int index, boolean v)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    if (p->isVisible() == v)
	return;
 
    p->setVisibility(v);

    this->notifyIoParameterStatusChanged(io == &this->inputParameters, index,
			   (v ? Node::ParameterBecomesVisible : 
				Node::ParameterBecomesInvisible));
}

////////////////////////////////////////////////////////////////////////////////
// 
// Set TRUE/FALSE indicating whether or not all parameters from
// the given list is set to be visible.
// 
// FIXME: 
// See the #if 0's below.  It's really bad to be doing an XtSetValues here
// This was done because the Workspace widget doesn't deal with standins being
// managed/unmanaged if they've got lines connected.  This needs attention after
// the 3.1 release.  (You have to do something like manage/unmanage because watching
// an Image standin grow/shrink is painfull.
// 
// A possible fix:  Go to WorkspaceW.c.  In the GeometryManager method, look for
// PerformSpaceWars.  Add a call there to RerouteLines.  Then you can switch back
// to {un}manage.  Drawback: it's an expensive fix.
// 
void Node::setAllIOVisibility(List *io, boolean v)
{
    Parameter *p;
    ListIterator li(*io);
    int index = 1;
    boolean isInput = io == &this->inputParameters;
 
#if 0
    this->standin->unmanage();
#else
    XtVaSetValues (this->standin->getRootWidget(),
	XmNmappedWhenManaged, False,
    NULL);
#endif
    while( (p = (Parameter *)li.getNext()) )
    {
	boolean newv = v || p->isConnected();
	boolean oldv = p->isVisible();
	if (newv != oldv) {
	    p->setVisibility(newv);
	    this->notifyIoParameterStatusChanged(isInput, index,
			   (newv ? Node::ParameterBecomesVisible : 
			   	   Node::ParameterBecomesInvisible));
	}
	++index;
    }
#if 0
    this->standin->manage();
#else
    XtVaSetValues (this->standin->getRootWidget(),
	XmNmappedWhenManaged, True,
    NULL);
#endif
}
////////////////////////////////////////////////////////////////////////////////
// 
// Return TRUE/FALSE indicating whether or not the give parameter from
// the given list is viewable. 
// 
boolean Node::isIOViewable(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->isViewable();
}

////////////////////////////////////////////////////////////////////////////////
// 
// Return TRUE/FALSE indicating whether or not the given i/o parameter is
// a required parameter.  
// 
boolean Node::isIORequired(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->isRequired();
}
////////////////////////////////////////////////////////////////////////////////
// 
// Return TRUE/FALSE indicating whether or not the given parameter list 
// contains parameters that can be hidden. 
// 
boolean Node::hasHideableIO(List *io)
{
    ListIterator iterator(*io);
    Parameter *p;
    while ( (p = (Parameter*)iterator.getNext()) ) {
	if (p->isVisible() && !p->isConnected())
	    return TRUE;
    }
    return FALSE;
}
////////////////////////////////////////////////////////////////////////////////
// 
// Return TRUE/FALSE indicating whether or not the given parameter list 
// contains parameters that can be exposed. 
// 
boolean Node::hasExposableIO(List *io)
{
    ListIterator iterator(*io);
    Parameter *p;

    while ( (p = (Parameter*)iterator.getNext()) ) {
	if (p->isViewable() && !p->isVisible())
	    return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// 
// Get the description for the index'th parameter in the given list.
// 
const char *Node::getIODescription(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);

    return p->getDescription();
}

////////////////////////////////////////////////////////////////////////////////
boolean Node::isIOCacheabilityWriteable(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
    p = (Parameter*)io->getElement(index);
    ASSERT(p);
    return p->hasWriteableCacheability();

}
////////////////////////////////////////////////////////////////////////////////
// 
// Get the cacheability for the index'th parameter in the given list.
// 
Cacheability Node::getIOCacheability(List *io, int index)
{
    Parameter *p;
    ASSERT( index >= 1 );
    p = (Parameter*)io->getElement(index);
    ASSERT(p);
    return p->getCacheability();
}

////////////////////////////////////////////////////////////////////////////////
// 
// Set the cacheability for the index'th parameter in the given list.
// 
void Node::setIOCacheability(List *io, int index, Cacheability c)
{
    Parameter *p;
    ASSERT( index >= 1 );
 
    p = (Parameter*)io->getElement(index);
    ASSERT(p);
    if (c != p->getCacheability())
    {
	//
	// Version 2.0.0 (possibly an unreleased version) was putting
	// 'cache:1' on interactor outputs, which was causing an assertion
	// failure.  So now, if the cacheability is not writable during
	// a .net read, then ignore the request.
	//
	boolean r = this->isIOCacheabilityWriteable(io, index);
	if (!r && this->network->isReadingNetwork())
	   return;
	else
	   ASSERT(r);
	p->setCacheability(c);
	this->getNetwork()->setDirty();
	if (this->cdb)
	    this->cdb->changeOutput(index);
    }
}

////////////////////////////////////////////////////////////////////////////////
// 
// Set the cacheability for the node.
// 
void Node::setNodeCacheability(Cacheability val)
{
    if (val != this->getNodeCacheability())
    {
	ASSERT(this->hasWriteableCacheability());
	this->nodeCacheability = val;
	this->getNetwork()->setDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////
// 
// 
// 
// 
boolean Node::typeMatchOutputToInput(int output_index, 
				     Node *dest, int input_index)
{
    Parameter *pin, *pout;
    ASSERT( output_index >= 1 );
    ASSERT( input_index >= 1 );
 
    pout = (Parameter*)this->outputParameters.getElement(output_index);
    pin  = (Parameter*)dest->inputParameters.getElement(input_index);
    ASSERT(pin && pout);

    return pin->typeMatch(pout);
}


////////////////////////////////////////////////////////////////////////////////
//
// Determine if this node has repeatable inputs that can be removed.
// We can remove repeats if the total number of parameters in this node
// is currently greater than the number of non-repeatable input parameters 
// found in the definition (the initial number of inputs minus the number of
// repeats).
//
boolean Node::hasRemoveableInput()
{
    NodeDefinition *def = this->definition;
    int icnt = this->getInputCount(); 

    if (!def->isInputRepeatable() || (icnt == 0))
	return FALSE;

    return icnt > (def->getInputCount() - def->getInputRepeatCount());
}
boolean Node::hasRemoveableOutput()
{
    NodeDefinition *def = this->definition;
    int icnt = this->getOutputCount(); 

    if (!def->isOutputRepeatable() || (icnt == 0))
	return FALSE;

    return icnt > (def->getOutputCount() - def->getOutputRepeatCount());
}
////////////////////////////////////////////////////////////////////////////////
// 
// Add a set of repeatable input or output parameters to the given node. 
// This request may be made by the parse (below) or by the Editor or StandIn. 
// If we have ConfigurationDialog, let it know about the added parameters.
//
boolean Node::addRepeats(boolean input)
{
    int i, iocnt, repeats;
    List *plist;
    ParameterDefinition *pd;
    NodeDefinition *nd;
    Parameter *p;

    nd = this->definition;

    if (input) {
	repeats = nd->getInputRepeatCount();
	plist = &this->inputParameters;
        iocnt = this->getInputCount();
    } else {
	repeats = nd->getOutputRepeatCount();
	plist = &this->outputParameters;
        iocnt = this->getOutputCount();
    }


    ASSERT(repeats > 0);

    this->network->setDirty();	// Let our network know that we have changed.

    for (i=1 ; i<=repeats ; i++) {	// Indexing is 1 based	
	if (input)
	    pd = (ParameterDefinition*) nd->getInputDefinition(iocnt+i);
 	else 	
	    pd = (ParameterDefinition*) nd->getOutputDefinition(iocnt+i);
 	p = nd->newParameter(pd,this,iocnt+i);
	plist->appendElement(p);	
        if (this->cdb) { 	// Let the configuration Dialog know about this
            if (input)
                this->cdb->newInput(iocnt + i);
            else
                this->cdb->newOutput(iocnt + i);
	}
        if (this->standin) { 	// Let the standin know about this
            if (input)
                this->standin->addInput(iocnt + i);
            else
                this->standin->addOutput(iocnt + i);
	}
    }
    

    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
// 
// Remove a set of repeatable input or output parameters from the given node. 
// The parameters that are removed are deleted.
// NOTE: this is a friend of the Node class.
//
boolean Node::removeRepeats(boolean input)
{
    int i, repeats;
    List *plist;
    Parameter *p;
    NodeDefinition *nd;
    
    nd = this->definition;

    if (input) {
	repeats = nd->getInputRepeatCount();
	plist = &this->inputParameters;
    } else {
	repeats = nd->getOutputRepeatCount();
	plist = &this->outputParameters;
    }


    ASSERT(repeats > 0);

    this->network->setDirty();	// Let our network know that we have changed.

    int cnt = plist->getSize();
    for (i=0 ; i<repeats ; i++)  {
	p = (Parameter*)plist->getElement(cnt);
	delete p;
	plist->deleteElement(cnt);
	if (this->cdb) {
            if (input)
                this->cdb->deleteInput(cnt);
            else
                this->cdb->deleteOutput(cnt);
	}
        if (this->standin) { 	// Let the standin know about this
            if (input)
                this->standin->removeLastInput();
            else
                this->standin->removeLastOutput();
	}
	cnt--;
    }
    

    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
// 
// Change a parameter from either the assigned or default value to the
// default or assigned value. 
// Set the index'th i/o parameter to use either the default value
// or the assigned valued. if notify is TRUE then call 
// ioParameterStatusChanged() with Node::ParameterSetValueToDefaulting.
// If there is a connection to the executive, then send the change.
//
void Node::setIODefaultingStatus(int index,
				 boolean input,
				 boolean defaulting,
				 boolean send,
				 boolean notify)
{
    Parameter *p;
    if (input)
	p = this->getInputParameter(index); 
    else
	p = this->getOutputParameter(index); 

    boolean was_defaulting = p->isDefaulting();
    if (was_defaulting == defaulting)
	return;

    p->setUnconnectedDefaultingStatus(defaulting);

    //
    // And now send the value to the executive if there is a connection.
    // Note that we don't need to set the network dirty, because we can
    // send the changes ourselves.
    //
    if (send)
    {
	DXPacketIF *pif = theDXApplication->getPacketIF(); 
	if (pif != NUL(DXPacketIF*)) 
	     this->sendValues(FALSE);
    }

    ASSERT(!p->isConnected());
    if (notify)
	this->notifyIoParameterStatusChanged(input, index,  
			Node::ParameterSetValueToDefaulting);
    

}
///////////////////////////////////////////////////////////////////////////////
// 
// Open this nodes configuration dialog box. 
//
void Node::openConfigurationDialog(Widget parent)
{
    if (!this->cdb)
	this->newConfigurationDialog(parent);
    this->cdb->post();
}
///////////////////////////////////////////////////////////////////////////////
// 
// Define the default action that is taken when the StandIn asks for the
// the default action. 
//
void Node::openDefaultWindow(Widget parent)
{
    this->openConfigurationDialog(parent);
}

///////////////////////////////////////////////////////////////////////////////
// 
// Display help for this node in a window. 
// FIXME: use real help system here.
//
void Node::openHelpWindow(Widget parent)
{
    ErrorMessage("Help not available for %s\n",this->getNameString());
}

//////////////////////////////////////////////////////////////////////////////
//
// Print the stuff that goes in a .cfg file. 
// The default is to succeed without printing anything.
//
boolean Node::cfgPrintNode(FILE *f, PrintType)
{
   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////
//
// Parse a comment in a .cfg file comment section for this node. 
// Be default nodes have lines of the following form
// '// node %s[%d]:'
//
boolean Node::cfgParseComment(const char* comment, 
		const char* filename, int lineno)
{
    return this->cfgParseNodeLeader(comment,filename,lineno);
}

boolean Node::setLabelString(const char *label)
{

    this->labelSymbol = theSymbolManager->registerSymbol(label);
    if (this->cdb)
	this->cdb->changeLabel();
    if (this->getStandIn())
	this->getStandIn()->notifyLabelChange();
    return TRUE;
}
const char *Node::getLabelString()
{
    if (this->labelSymbol == 0)
        return this->getNameString();
    else
        return theSymbolManager->getSymbolString(this->labelSymbol);
}

boolean Node::initialize()
{
    return TRUE;
}

boolean Node::deleteArk(Ark *a)
{
    Node*      sourceNode;
    Node*      destNode;
    int        sourceIndex, destIndex;

    sourceNode = a->getSourceNode(sourceIndex);
    destNode   = a->getDestinationNode(destIndex);


    destNode->removeInputArk(destIndex, a);
    sourceNode->removeOutputArk(sourceIndex, a);

    return TRUE;

}

boolean Node::removeIOArk(List *io, int index, Ark *a)
{
    Parameter *p = (Parameter*)io->getElement(index);

    p->removeArk(a);  

    this->notifyIoParameterStatusChanged(io == &this->inputParameters, index,
				Node::ParameterArkRemoved);

    return TRUE;
}


void
Node::updateDefinition()
{
	struct ArkInfo {
		int   srcIndex;
		int   dstIndex;
		Node *src;
		Node *dst;
	};

	//
	// Types may have changed so delete the cdb.
	//
	if (this->cdb) {
		delete this->cdb;
		this->cdb = NULL;
	}

#if 11 	// This is the beginning of an attemp to fix bug DAWOOD91 
#else
	boolean hadStandIn = FALSE;

	//
	// Recreate the standin if need be
	//
	if (this->standin) {
		delete this->standin;
		this->standin = NULL;
		hadStandIn = TRUE;
	}
#endif

	// For each of the input ParameterDefinitions in the new definition, 
	// make sure that the types of the parameter are correct, and 
	// disconnect arcs or reset the values to NULL.

	int numInputs = this->getInputCount();
	boolean *defaulting = NULL;
	boolean *visibilities = NULL;
	char **values = NULL;
	List **inputArks = NULL;
	if (numInputs != 0)
	{
		defaulting = new boolean[numInputs];
		values = new char *[numInputs];
		inputArks = new List *[numInputs];
		visibilities = new boolean[numInputs];
	}
	int i;
	for (i = 1; i <= numInputs; ++i)
	{
		inputArks[i-1] = NULL;
		values[i-1] = NULL;
		defaulting[i-1] = FALSE;
		visibilities[i-1] = this->isInputVisible(i);
		if (this->isInputConnected(i))
		{
			const List *arcs = this->getInputArks(i);
			inputArks[i-1] = new List;
			ListIterator li(*(List*)arcs);
			Ark *a;
			while( (a = (Ark*)li.getNext()) )
			{
				ArkInfo *ai = new ArkInfo;
				ai->src = a->getSourceNode(ai->srcIndex);
				ai->dst = a->getDestinationNode(ai->dstIndex);
				inputArks[i-1]->appendElement(ai);
			}
		}
		else if (!(defaulting[i-1] = this->isInputDefaulting(i)))
			values[i-1] = DuplicateString(this->getInputValueString(i));
		delete this->getInputParameter(i);
	}
	this->inputParameters.clear();

	int numOutputs = this->getOutputCount();
	List  **outputArks = NULL;
	if (numOutputs != 0)
		outputArks = new List *[numOutputs];
	for (i = 1; i <= numOutputs; ++i)
	{
		outputArks[i-1] = NULL;
		if (this->isOutputConnected(i))
		{
			const List *arcs = this->getOutputArks(i);
			outputArks[i-1] = new List;
			ListIterator li(*(List*)arcs);
			Ark *a;
			while( (a = (Ark*)li.getNext()) )
			{
				ArkInfo *ai = new ArkInfo;
				ai->src = a->getSourceNode(ai->srcIndex);
				ai->dst = a->getDestinationNode(ai->dstIndex);
				outputArks[i-1]->appendElement(ai);
			}
		}
		delete this->getOutputParameter(i);
	}
	this->outputParameters.clear();

	this->buildParameterLists();

	for (i = 1; i <= numInputs && i <= this->getInputCount(); ++i)
	{
		this->setInputVisibility(i, visibilities[i-1]);
		if (inputArks[i-1])
		{
			ListIterator li(*inputArks[i-1]);
			ArkInfo *ai;
			while( (ai = (ArkInfo*)li.getNext()) )
			{
				if (ai->src->typeMatchOutputToInput(ai->srcIndex,
					ai->dst, ai->dstIndex))
				{
					Ark *newArk = new Ark(ai->src, ai->srcIndex, 
						ai->dst, ai->dstIndex);
					if (ai->src->getNetwork()->getEditor() && 
						ai->src->getStandIn())
						ai->src->getStandIn()->addArk(
						ai->src->getNetwork()->getEditor(),
						newArk);
				}
				delete ai;
			}
			delete inputArks[i-1];
		}
		else if (defaulting[i-1])
			this->useDefaultInputValue(i);
		else
			this->setInputValue(i, values[i-1]);
	}
#if 11 
	// must increase j by the number of repeats added in buildParameterLists()
	int repeats = this->getInputCount() - this->definition->getInputCount();
	int j = numInputs + repeats; 
	for (; j >= i; --j)
	{
		if (this->cdb)
			this->cdb->deleteInput(j);
		if (this->standin)
			this->standin->removeLastInput();
	}
	for (; i <= this->getInputCount(); ++i)
	{
		if (this->cdb)
			this->cdb->newInput(i);
		if (this->standin)
			this->standin->addInput(i);
	}
#endif
	if (numInputs != 0)
	{
		delete[] defaulting;
		delete[] visibilities;
		delete[] values;
		delete[] inputArks;
	}

	for (i = 1; i <= numOutputs && i <= this->getOutputCount(); ++i)
	{
		if (outputArks[i-1])
		{
			ListIterator li(*outputArks[i-1]);
			ArkInfo *ai;
			while( (ai = (ArkInfo*)li.getNext()) )
			{
				if (ai->src->typeMatchOutputToInput(ai->srcIndex,
					ai->dst, ai->dstIndex))
				{
					Ark *newArk = new Ark(ai->src, ai->srcIndex, 
						ai->dst, ai->dstIndex);
					if (ai->src->getNetwork()->getEditor() && 
						ai->src->getStandIn())
						ai->src->getStandIn()->addArk(
						ai->src->getNetwork()->getEditor(),
						newArk);
				}
				delete ai;
			}
			delete outputArks[i-1];
		}
		this->notifyIoParameterStatusChanged(FALSE, i, Node::ParameterValueChanged);
	}
#if 11 
	j = numOutputs;
	for (; j >= i; --j)
	{
		if (this->cdb)
			this->cdb->deleteOutput(j);
		if (this->standin)
			this->standin->removeLastOutput();
	}
	for (; i <= this->getOutputCount(); ++i)
	{
		if (this->cdb)
			this->cdb->newOutput(i);
		if (this->standin)
			this->standin->addOutput(i);
	}
#endif
	if (numOutputs != 0)
		delete[] outputArks;

#if 11 
#else
	if (hadStandIn) {
		EditorWindow *e = this->getNetwork()->getEditor();
		this->standin = this->newStandIn(e->getWorkSpace());
	}
#endif

	//
	// Because of undo in Editor, provide notification that a defintion
	// has changed.  The undo list will probably have to be tossed out
	//
	EditorWindow *e = this->getNetwork()->getEditor();
	if (e) e->notifyDefinitionChange(this);
}

//
// Based on a Node's definition, build a parameter list for the given node.
//
boolean Node::buildParameterLists()
{
    int ninputs, noutputs, i;
    Parameter *p; 
    ParameterDefinition *pd;
    NodeDefinition *nd = this->getDefinition();
    
    ninputs = nd->getInputCount();
    for (i=1 ; i<=ninputs ; i++) {
	pd = nd->getInputDefinition(i);
	ASSERT(pd);
        p = nd->newParameter(pd,this,i);
        ASSERT(p);
	boolean r = this->appendInput(p); // FIXME: handle error 
	ASSERT(r);
    }
    if (nd->isInputRepeatable())
	this->addRepeats(TRUE);

    noutputs = nd->getOutputCount();
    for (i=1 ; i<=noutputs ; i++) {
	pd = nd->getOutputDefinition(i);
	ASSERT(pd);
        p = nd->newParameter(pd,this,i);
        ASSERT(p);
	boolean r = this->appendOutput(p);
	ASSERT(r);
    }
    if (nd->isOutputRepeatable())
	this->addRepeats(FALSE);

    return TRUE; 
}

#if WORKSPACE_PAGES
#else
void Node::setGroupName(const char *name)
{
    if (!name) {
	this->groupNameSymbol = 0;
    } else {
	this->groupNameSymbol = theSymbolManager->registerSymbol(name);
    }

    this->network->setDirty();
}

const char* Node::getGroupName()
{
    if (!this->groupNameSymbol)
	return NULL;

    return theSymbolManager->getSymbolString(this->groupNameSymbol);
}

void Node::addToGroup(const char* group)
{
    this->setGroupName(group);
    theDXApplication->PGManager->registerGroup(group, this->network);
}
#endif

char *Node::netEndOfMacroNodeString(const char *prefix)
{
    return NULL;
}

char *Node::netBeginningOfMacroNodeString(const char *prefix)
{
    return NULL;
}


//
// Print the invocation of any script language that is
// to occur at the beginning of the containing macro.
//
boolean	Node::netPrintBeginningOfMacroNode(FILE *f, 
		      PrintType dest, 
		      const char *prefix,
		      PacketIFCallback callback,
		      void *clientdata)
{
    boolean r = TRUE;

    char *s = netBeginningOfMacroNodeString(prefix);
    if (s == NULL)
	return TRUE;

    if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
	if (fputs(s, f) < 0)
	    r = FALSE;
	if (callback != NUL(PacketIFCallback))
	    callback(clientdata,s);
    } else {
	ASSERT (dest == PrintExec);
	DXPacketIF *pif = theDXApplication->getPacketIF(); 
	pif->sendBytes(s);
    }


    delete s;

    return r;
}

//
// Print the invocation of any script language that is
// to occur at the end of the containing macro.
//
boolean	Node::netPrintEndOfMacroNode(FILE *f, 
		      PrintType dest, 
		      const char *prefix,
		      PacketIFCallback callback,
		      void *clientdata)
{
    boolean r = TRUE;

    char *s = netEndOfMacroNodeString(prefix);
    if (s == NULL)
	return TRUE;

    if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
	if (fputs(s, f) < 0)
	    r = FALSE;
	if (callback != NUL(PacketIFCallback))
	    callback(clientdata,s);
    } else {
	ASSERT (dest == PrintExec);
	DXPacketIF *pif = theDXApplication->getPacketIF(); 
	pif->sendBytes(s);
    }

    delete s;

    return r;
}
boolean Node::isInputRepeatable()
{
    NodeDefinition *def = this->definition;
    if (!def->isInputRepeatable())
	return FALSE;
    int icnt = this->getInputCount(); 
    int rcnt = icnt - (def->getInputCount() - def->getInputRepeatCount());
    int sets = rcnt / def->getInputRepeatCount();
    return sets < (MAX_INPUT_SETS);
}
boolean Node::isOutputRepeatable()
{
    NodeDefinition *def = this->definition;
    if (!def->isOutputRepeatable())
	return FALSE;
    int icnt = this->getOutputCount(); 
    int rcnt = icnt - (def->getOutputCount() - def->getOutputRepeatCount());
    int sets = rcnt / def->getOutputRepeatCount();
    return sets < (MAX_OUTPUT_SETS);
}

//
// Reset the node to using the default cfg state (probably before reading
// in a new cfg file). 
//
void Node::setDefaultCfgState()
{
}
//
// Return TRUE if this node has state that will be saved in a .cfg file.
// At this level, nodes do not have cfg state.
//
boolean Node::hasCfgState()
{
    return FALSE;
}


//
// Generate a new instance number for this node.  We notify the
// standIn and mark the network and the node dirty.
// The new instance number is returned.
//
int Node::assignNewInstanceNumber()
{
    NodeDefinition *nd = this->definition;
    ASSERT(nd);
    this->instanceNumber = nd->newInstanceNumber();
#if 0
    // FIXME: must notify standIn when in UIDebug mode
    if (this->standIn) {
	this->standIn-??
#endif

    //
    // Require that the network resend all tools. 
    //
    this->network->setDirty();

    //
    // Mark all inputs and outputs dirty so that they are resent with the
    // new parameter names.
    //
    int i, cnt = this->getInputCount();
    for (i=1 ; i<=cnt ; i++)
	this->setInputDirty(i);     
    cnt = this->getOutputCount();
    for (i=1 ; i<=cnt ; i++)
	this->setOutputDirty(i);     

    if (this->moduleMessageId) {
        delete this->moduleMessageId;
        this->moduleMessageId = NULL; 
    }
    return this->instanceNumber;
}
//
// Disconnect all input and output arcs from this node.
//
boolean Node::disconnectArks()
{
    int i;
    ListIterator li;
    Parameter *p;

    //
    // Disconnect the inputs.
    //
    li.setList(this->inputParameters);
    for (i=1 ; (p = (Parameter*)li.getNext()) ; i++) {
	if (p->isConnected()) {
	    p->disconnectArks();
	    this->notifyIoParameterStatusChanged(TRUE, i, Node::ParameterArkRemoved);
	}
    }

    //
    // Disconnect the outputs.
    //
    li.setList(this->outputParameters);
    for (i=1 ; (p = (Parameter*)li.getNext()) ; i++) {
	if (p->isConnected()) {
	    p->disconnectArks();
	    this->notifyIoParameterStatusChanged(FALSE, i, Node::ParameterArkRemoved);
	}
    }


    return TRUE;
}
//
// Return TRUE if the node can be switched (pasted/merged/moved) from the 'from'
// net to the 'to' net.
//
boolean Node::canSwitchNetwork(Network *from, Network *to)
{
    if (to->isMacro() && !this->isAllowedInMacro()) {
	WarningMessage("%s tools are not allowed in macros.\n"
			"Attempt to add a %s tool to macro ignored.",
			this->getNameString(), this->getNameString());
	return FALSE; 
    }

    return TRUE;
}
//
// Do whatever is necessary to switch the node to the new Network.
//
void Node::switchNetwork(Network *from, Network *to, boolean silently)
{
    if (this->cdb) {
	delete this->cdb;
	this->cdb = NULL;
    }
    if (this->standin) {
	delete this->standin;
	this->standin = NULL;
    }

    this->network = to;
}

//
// Get the selectable values for the n'th input.
// This returns a pointer to a constant array of pointers to
// constant strings which is NOT to be manipulated by the caller.
// The returned array of pointers is NULL terminated.
//
const char * const *Node::getInputValueOptions(int index)
{
    Parameter *p;
    ASSERT( index >= 1 );

    p = (Parameter*)this->inputParameters.getElement(index);
    ASSERT(p);

    return p->getValueOptions();
}

#ifdef DXUI_DEVKIT 

#define OBJVAL_FORMAT "%s_%s"
#define MODINPUT_FORMAT "%s_%d_in"
#define MODOUTPUT_FORMAT "%s_%d_out"
#define INDENT "    "

static int phaseNumber = 0;
boolean Node::beginDXCallModule(FILE *f)
{
    const char *name = this->getNameString();
    int i, instance_number = this->getInstanceNumber();
    char modinput_name[128];
    char modoutput_name[128];
    int input_count = this->getInputCount();
    int output_count = this->getOutputCount();

    if (phaseNumber == 0) {
	phaseNumber = 1;
	if (fprintf(f,	"    /*\n"
		      	"     * Temporary variables used by all module calls\n"
		      	"     */\n"
			"    int 	  i, rcode;\n"
			"    Object	  objinput[32];\n"
			"    ModuleInput  minput[32];\n"
			"    ModuleOutput moutput[32];\n"
			"    /*\n"
		      	"     * Per module variables follow (if needed)\n"
		      	"     */\n") <= 0)
	    return FALSE;
    }
	
		
    if (input_count > 0) {
        sprintf(modinput_name, MODINPUT_FORMAT,name,instance_number);
	for (i=1 ; i<=input_count ; i++) {
	    if (!this->isInputConnected(i) && !this->isInputDefaulting(i)) {
		char oname[128];
		Parameter *p = this->getInputParameter(i);
		sprintf(oname,OBJVAL_FORMAT,modinput_name, 
					    this->getInputNameString(i));
		char *code = p->getObjectCodeDecl(INDENT,oname);
		if (code) {
		    if (fprintf(f,code) <= 0)
			return FALSE;
		    delete code;
		}
	    }
	}
    }

    if (output_count > 0) {
        sprintf(modoutput_name, MODOUTPUT_FORMAT,name,instance_number);
	for (i=1 ; i<=output_count ; i++) {
	    if (this->isOutputConnected(i)) {
		char oname[256];
		sprintf(oname,OBJVAL_FORMAT,modoutput_name,
					this->getOutputNameString(i));
		if (fprintf(f,"%sObject %s = NULL;\n",INDENT,oname) <= 0)
		    return FALSE;
	    }
	}
    }

    return TRUE;

}
boolean Node::callDXCallModule(FILE *f)
{
    const char *name = this->getNameString();
    int objnum, i, instance_number = this->getInstanceNumber();
    boolean r;

    if (phaseNumber == 1) {
	phaseNumber = 2;
	if (fprintf(f,"\n    /*\n"
			"     * Some initializations\n" 
			"     */\n"
			"    for (i=0 ; i<32 ; i++) \n"
			"        objinput[i] = NULL;\n") <= 0)
	    return FALSE;
    }

    if (fprintf(f,"\n    /*\n"
		    "     * %s:%d - setup in/outputs and call module\n"
		    "     */\n",name,instance_number) <= 0)
	return FALSE;

    char modinput_name[128];
    int ninput = 0;
    int input_count = this->getInputCount();
    if  (input_count <= 0) {
	strcpy(modinput_name,"NULL");
    } else {
	//
	// Generate the code to set the ModuleInputs 
	//
	sprintf(modinput_name, MODINPUT_FORMAT,name,instance_number);
	for (objnum=0, i=1 ; i<=input_count ; i++) {

	    if (this->isInputDefaulting(i)) 
		continue;

	    const char *input_name = this->getInputNameString(i);
	    char input_val[512];
	    char *init_code = NULL;
	    if (this->isInputConnected(i)) {
		Parameter *p = this->getInputParameter(i);
		Ark  *a = p->getArk(1);
		int param;
		ASSERT(a);
		Node *onode = a->getSourceNode(param);
		ASSERT(onode);
		char oname[256];
		sprintf(oname, MODOUTPUT_FORMAT,onode->getNameString(),
						onode->getInstanceNumber());
		sprintf(input_val, "%s_%s", oname,
					onode->getOutputNameString(param));
	
	    } else {	// Parameter has a tab down value
		Parameter *p = this->getInputParameter(i);
		char oname[256];
		sprintf(oname,OBJVAL_FORMAT,modinput_name,
					this->getInputNameString(i));
		sprintf(input_val, "objinput[%d]", objnum++);
		init_code = p->getObjectCreateCode(INDENT,oname,input_val);
	    }
	
	    if (init_code && fprintf(f,init_code) <= 0) {
		delete init_code;
		return FALSE;
	    }
	    r = fprintf(f,"%sDXModSetObjectInput(minput + %d,\"%s\",%s);\n",
				    INDENT,ninput++, input_name, input_val) > 0;
	    if (init_code)
		delete init_code;
	    if (!r)
		return FALSE;
	}
    }

    //
    // Generate the code to set the ModuleOutputs
    //
    char modoutput_name[128];
    int noutput = 0;
    int output_count = this->getOutputCount();

    //
    // Generate the code to set the ModuleOutputs 
    //
    sprintf(modoutput_name, MODOUTPUT_FORMAT,name,instance_number);
    for (i=1 ; i<=output_count ; i++) {
	if (!this->isOutputConnected(i))
	    continue;

	const char *output_name = this->getOutputNameString(i);
	char output_objptr[512];
	// FIXME; what if the output has a value (i.e. an interactor)
	char oname[256];
	sprintf(oname,OBJVAL_FORMAT,modoutput_name,
				this->getOutputNameString(i));
	sprintf(output_objptr,"&%s",oname);

	if (fprintf(f,"%sDXModSetObjectOutput(moutput + %d,\"%s\",%s);\n",
		      INDENT,noutput++,output_name,output_objptr) <= 0)
	    return FALSE;
    }


    //
    // Finally, generate the call to DXCallModule 
    //
    if (fprintf(f,"%srcode = DXCallModule(\"%s\",%d,minput,%d,moutput);\n",
			INDENT,this->getNameString(),ninput,noutput) <= 0)
	return FALSE;

    
    //
    // Reference connected outputs
    //
    for (i=1 ; i<=output_count ; i++) 
	if (this->isOutputConnected(i))
	{
	    const char *output_name = this->getOutputNameString(i);
	    char oname[256];

	    sprintf(oname,OBJVAL_FORMAT,modoutput_name,
				    this->getOutputNameString(i));

	    if (fprintf(f,"%sDXReference(%s);\n", INDENT, oname) <= 0)
		return FALSE;
	}
	    
	    
    if (fprintf(f,"%sif (rcode == ERROR) goto error;\n", INDENT) <= 0)
	return FALSE;

    return TRUE;
}
boolean Node::endDXCallModule(FILE *f)
{

    const char *name = this->getNameString();
    int i, instance_number = this->getInstanceNumber();
    char modinput_name[128];
    char modoutput_name[128];
    int input_count = this->getInputCount();
    int output_count = this->getOutputCount();

    phaseNumber = 0;

    if (input_count > 0) {
        sprintf(modinput_name, MODINPUT_FORMAT,name,instance_number);
	for (i=1 ; i<=input_count ; i++) {
	    if (!this->isInputConnected(i) && !this->isInputDefaulting(i)) {
		char oname[256];
		sprintf(oname,OBJVAL_FORMAT,modinput_name,
					this->getInputNameString(i));
		Parameter *p = this->getInputParameter(i);
	  	char *cleanup_code = p->getObjectCleanupCode(INDENT,oname);
		if (cleanup_code) {
		    if (fprintf(f,cleanup_code) <= 0) {
			delete cleanup_code;
			return FALSE;
		    }
		    delete cleanup_code;
		}
	    }
	}
    }   

    if (output_count > 0) {
        sprintf(modoutput_name, MODOUTPUT_FORMAT,name,instance_number);
	for (i=1 ; i<=output_count ; i++) {
	    if (this->isOutputConnected(i)) {
		char oname[256];
		sprintf(oname,OBJVAL_FORMAT,modoutput_name,
					this->getOutputNameString(i));
		if (fprintf(f,"%sif (%s) DXDelete(%s);\n",
					INDENT,oname,oname) <= 0)
		    return FALSE;
	    }
	}
    }


    return TRUE;

}
#endif	// DXUI_DEVKIT
//
// See if the given string is a viable label to be used as an identifier.
// Also make sure it is not a reserved script language word.
// Return TRUE if ok, FALSE otherwise and issue and error message.
//
boolean Node::verifyRestrictedLabel(const char *label)
{
    int junk = 0;
    if (!IsIdentifier(label, junk) || junk != STRLEN(label))
    {
        ErrorMessage("%s name \"%s\" must consist of "
                     "letters, numbers, or the character '_', and "
                     "begin with a letter",
                     this->getNameString(),
                     label);
        return FALSE;
    }

    if (IsReservedScriptingWord(label)) {
        ErrorMessage("%s name \"%s\" is a reserved word",
                     this->getNameString(),
                     label);
        return FALSE;
    }

    return TRUE;
}

boolean Node::cfgPrintNodeLeader(FILE *f)
{

    if (fprintf(f, "//\n// node %s[%d]:\n",
                this->getNameString(),
                this->getInstanceNumber()) < 0)
        return FALSE;

    return TRUE;
}
boolean Node::cfgParseNodeLeader(const char *comment, 
				const char *file, int lineno)
{
    int d;
    if (sscanf(comment, " node %*[^[][%d]:",&d) != 1)
        return FALSE;

    return TRUE;
}

boolean Node::printAsJava(FILE* f)
{
    if (this->hasJavaRepresentation() == FALSE) return TRUE;

    const char* indent = "        ";
    const char* nn = this->getJavaNodeName();
    const char* ns = this->getNameString();
    int instno = this->getInstanceNumber();

    fprintf (f,
	"\n\n%s%s %s_%d = new %s (this.network, \"%s\", %d, \"%s\");\n",
	indent, nn, ns, instno, nn, ns, instno, this->getLabelString()
    );
    fprintf (f,
	"%sthis.network.addElement((Object)%s_%d);\n",
	indent, ns, instno
    );


    //
    // Loop over all inputs.  If the node wants that input's value
    // printed as java, then oblige.
    //
    int i, icnt = this->getInputCount();
    for (i=1; i<=icnt; i++) {
	char src_name[128];
	int src_out = 0;
	strcpy (src_name, "null");
	if (this->printInputAsJava(i)) {
	    if (this->isInputConnected(i)) {
		const List* ia = (List*)getInputArks(i);
		Ark* a = NUL(Ark*);
		Node* src = NUL(Node*);
		if (ia) {
		    ListIterator li(*(List*)ia);
		    a = (Ark*)li.getNext();
		}
		if (a) src = (Node*)a->getSourceNode(src_out);
		if ((src)&&(src->isA(src->hasJavaRepresentation()))) {
		    const char* src_ns = src->getNameString();
		    int src_instno = src->getInstanceNumber();
		    sprintf (src_name, "%s_%d", src_ns, src_instno);
		}

		fprintf (f, "%s%s_%d.addInputArk (%d, %s, %d);\n",
		    indent, ns, instno, i, src_name, src_out);
	    }
	    const char* valstr = this->getJavaInputValueString(i);
	    if ((valstr) && (EqualString(valstr, "NULL")==FALSE)) {
		if (valstr[0] == '"') {
		    if (fprintf (f, "%s%s_%d.setInputValueString(%d, %s);\n",
			indent, ns, instno, i, valstr) == FALSE) return FALSE;
		} else {
		    if (fprintf (f, "%s%s_%d.setInputValueString(%d, \"%s\");\n",
			indent, ns, instno, i, valstr) == FALSE) return FALSE;
		}
	    }
	}
    }

    return TRUE;
}

void Node::setLayoutInformation(Base* li)
{
    if (this->layout_information)
	delete this->layout_information;
    this->layout_information = li;
}
