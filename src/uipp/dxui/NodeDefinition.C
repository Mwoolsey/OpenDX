/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "NodeDefinition.h"
#include "Node.h"
#include "ParameterDefinition.h"
#include "Parameter.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "Network.h"
#include "SIAllocatorDictionary.h"
#include "CDBAllocatorDictionary.h"
#include "ErrorDialogManager.h"

//
//  This is the dictionary that contains all Node (and derived) class
//  definitions.  Looking up a node by name returns a pointer to a
//  NodeDefinition; 
//
Dictionary *theNodeDefinitionDictionary = new Dictionary;


NodeDefinition::NodeDefinition()
{
    this-> nodeInstanceNumbers = 0; 
    this->description = NUL(char*); 
    this->input_repeats = this->output_repeats = 0; 
    this->writeableCacheability = TRUE;
    this->defaultCacheability = ModuleFullyCached;
    this->mdf_flags = 0;
    this->loadFile = NULL;
    this->outboardHost = NULL;
    this->outboardCommand = NULL;
    this->userTool = TRUE;
    this->uiLoadedOnly = FALSE;	
}
NodeDefinition::~NodeDefinition()
{
    //
    // Delete the input/output definitions
    //
    ParameterDefinition *pd;
    ListIterator iterator(this->inputDefs);
    while ( (pd = (ParameterDefinition*)iterator.getNext()) )
	delete pd;
    iterator.setList(this->outputDefs);
    while ( (pd = (ParameterDefinition*)iterator.getNext()) )
	delete pd;


    if (this->outboardHost)
	delete[] this->outboardHost;
    if (this->outboardCommand)
	delete[] this->outboardCommand;
    if (this->loadFile)
    	delete[] this->loadFile;
    if (this->description)
    	delete[] this->description;

    //
    // If this node definition was installed in the Node definition 
    // dictionary, then remove it.
    // Note that the constructor does NOT install this NodeDefinition in
    // theNodeDefinitionDictionary.  It is assumed that this happens elsewhere,
    // probably for no good reason.
    //
    NodeDefinition *nd = (NodeDefinition*)
        theNodeDefinitionDictionary->findDefinition(this->getNameSymbol());
    if (nd == this)
        theNodeDefinitionDictionary->removeDefinition(this->getNameSymbol());


}
Node *NodeDefinition::newNode(Network *net, int inst)
{
    Node *m = new Node(this, net, inst);
    return m;
}

NodeDefinition *NodeDefinition::AllocateDefinition()
{
        return new NodeDefinition;
}

///////////////////////////////////////////////////////////////////////////////
// Get the Nth Input or Output ParameterDefinition.
// n is indexed from 1.  If n is greater than the number of input/output
// defintions then it is a repeatable parameter, in which case we
// must account for this to find the correct definition.
//
ParameterDefinition *NodeDefinition::getInputDefinition(int n)
{

    ASSERT(n > 0);
    int cnt = this->getInputCount();

    if (n > cnt) {
	int repeats = this->input_repeats;
	ASSERT(repeats > 0);
	if (repeats == 1) {
	    n = cnt;
	} else {
	    int num = (((n-1) - cnt) % repeats) + 1;
	    n = cnt - repeats + num ;
	    ASSERT(n > cnt - repeats);
	    ASSERT(n <= cnt);
	}
    }
    return (ParameterDefinition*)this->inputDefs.getElement(n); 
}
//
// Get the n'th output parameter definition.
// n is indexed from 1.
//
ParameterDefinition *NodeDefinition::getOutputDefinition(int n)
{

    ASSERT(n > 0);
    int cnt = this->getOutputCount();

    if (n > cnt) {
	int repeats = this->output_repeats;
	ASSERT(repeats > 0);
	if (repeats == 1) {
	    n = cnt;
	} else {
	    int num = (((n-1) - cnt) % repeats) + 1;
	    n = cnt - repeats + num ;
	    ASSERT(n > cnt - repeats);
	    ASSERT(n <= cnt);
	}
    }
    return (ParameterDefinition*)this->outputDefs.getElement(n); 
}

Node *NodeDefinition::createNewNode(Network *net, int inst)
{
    if (!this->isAllowedInMacro() && net->isMacro())
    {
        ErrorMessage("The %s node is not allowed in a macro.",
            this->getNameString());
        return NULL;
    }

    Node *n = this->newNode(net, inst);

    if (n) {
	//
	// If initialization of the node fails delete the node and return NULL.
	// If initialize() fails, it is assumed that it issued an appropriate
	// error message, so we don't give one here.
	//
	if (!n->initialize()) {
	    delete n;
	    return NULL;
	}
    }

    return n;
}


boolean NodeDefinition::isDerivedFromMacro()
{
    return FALSE;
}
//
// Called once per new class (actually called on every new instance, but
// we assume we only have one instance of each node defintion in the
// whole system) after parsing an MDF record for this node.
//
void NodeDefinition::completeDefinition()
{

    if (theSIAllocatorDictionary) {
	SIAllocator sia = this->getSIAllocator();
	if (sia)
	   theSIAllocatorDictionary->addAllocator(this->getNameString(), sia);
    }

    if (theCDBAllocatorDictionary) {
	CDBAllocator cdba = this->getCDBAllocator();
	if (cdba)
	   theCDBAllocatorDictionary->addAllocator(this->getNameString(), cdba);
    }

    this->finishDefinition();
        
}

SIAllocator NodeDefinition::getSIAllocator()
{
   // Causes the default definition in the SIAllocatorDictionary to be used.
   return NULL;	
}
CDBAllocator NodeDefinition::getCDBAllocator()
{
   // Causes the default definition in the CDBAllocatorDictionary to be used.
   return NULL;	
}
//
// Get a new parameter for the node that corresponds to this node definition.
//
Parameter *NodeDefinition::newParameter(ParameterDefinition *pd, 
						Node *n, int index)
{
    return new Parameter(pd);
}
//
// Returns the name executive module that is called to do the work
// for this type of node.
// Be default, that name is always just the name of the Node.
//
const char *NodeDefinition::getExecModuleNameString()
{
    return this->getNameString();
}

//
// Reset all the NodeDefinitions in the given dictionary to 
// have the next instance number generated by 1. 
//
void NodeDefinition::ResetInstanceNumbers(Dictionary *dict)
{
    DictionaryIterator di(*dict);
    NodeDefinition  *nd;

    while ( (nd = (NodeDefinition*) di.getNextDefinition()) ) 
	nd->setNextInstance(1);
}

//
// Get the MDF record for this Node definition.
// The returned string must be delete by the caller.
//
char *NodeDefinition::getMDFString()
{
    char *header  = this->getMDFHeaderString();
    char *inputs  = this->getMDFParametersString(TRUE);
    char *outputs = this->getMDFParametersString(FALSE);

    ASSERT(header);

    char *mdf = new char [STRLEN(header) + 
				STRLEN(inputs) + STRLEN(outputs) + 2];
    sprintf(mdf, "%s%s%s",
		header,
		(inputs ? inputs : ""),
		(outputs ? outputs : ""));

    delete[] header;
    if (inputs) delete[] inputs;
    if (outputs) delete[] outputs;
    return mdf;
}
//
// Get the MDF header (stuff before the INPUT/OUTPUT statements) for this
// NodeDefinition. 
// The returned string must be delete by the caller.
//
char *NodeDefinition::getMDFHeaderString()
{
    char category[256];
    char description[256];
    char io_board[256];
    char flags[256];

    if (this->getCategoryString()) {
	sprintf(category,"CATEGORY %s\n", this->getCategoryString());
    } else {
	category[0] = '\0';
    }
    if (this->getDescription()) {
	sprintf(description,"DESCRIPTION %s\n", this->getDescription());
    } else {
	description[0] = '\0';
    }
    if (this->isOutboard()) {
	const char *host = this->getDefaultOutboardHost();
	sprintf(io_board,"OUTBOARD \"%s\" ; %s\n",
			this->getOutboardCommand(),
			host ? host : "");
    } else if (this->isDynamicallyLoaded()) {
	sprintf(io_board,"LOADABLE %s\n", this->getDynamicLoadFile());
    } else {
	io_board[0] = '\0';
    }
    if (this->mdf_flags) {
	sprintf(flags,"FLAGS%s%s%s%s%s%s%s%s%s\n",
		(this->isMDFFlagERR_CONT() ? " ERR_CONT" : ""),
		(this->isMDFFlagSWITCH() ? " SWITCH" : ""),
		(this->isMDFFlagPIN() ? " PIN" : ""),
		(this->isMDFFlagLOOP() ? " LOOP" : ""),
		(this->isMDFFlagSIDE_EFFECT() ? " SIDE_EFFECT" : ""),
		(this->isMDFFlagPERSISTENT() ? " PERSISTENT" : ""),
		(this->isMDFFlagASYNCHRONOUS() ? " ASYNC" : ""),
		(this->isMDFFlagREACH() ? " REACH" : ""),
		(this->isMDFFlagREROUTABLE() ? " REROUTABLE" : ""));
    } else  {
	flags[0] = '\0';
    }

    char *header = new char[128 + 
			STRLEN(category) +
			STRLEN(description) +
			STRLEN(io_board) +
			STRLEN(flags)]; 

    sprintf(header,"MODULE %s\n"
		   "%s"
		   "%s"
		   "%s"
		   "%s",
		    this->getNameString(),
		    category,
		    description,
		    io_board,
		    flags);

    return header;
}

//
// Get the list of INPUT/OUTPUT lines in the MDF and the REPEAT if any
// The returned string must be delete by the caller.
//
char *NodeDefinition::getMDFParametersString(boolean inputs)
{
#define CHUNK 256
    char *params = NULL; 
    int currend = 0, maxlen = 0;
    ListIterator li;
    ParameterDefinition *pd;

    if (inputs)
	li.setList(this->inputDefs);	
    else
	li.setList(this->outputDefs);	

    while ( (pd = (ParameterDefinition*)li.getNext()) ) {
	char *line = pd->getMDFString();
	int linelen = STRLEN(line) + 1;
	if (linelen + currend > maxlen) {
	    maxlen = ( CHUNK > linelen ? CHUNK : linelen) + currend;
	    params = (char*)REALLOC(params, maxlen * sizeof(char));
	    if (currend == 0)	// The first time
		*params = '\0';
	}
	strcat(&params[currend], line);
	currend += linelen - 1;
	delete[] line;
    } 

    if (inputs && this->isInputRepeatable()) {
	if (maxlen - currend < 32)
	    params = (char*)REALLOC(params, (32 + currend) * sizeof(char));
	sprintf(&params[currend],"REPEAT %d\n",this->getInputRepeatCount());
    } else if (!inputs && this->isOutputRepeatable()) {
	if (maxlen - currend < 32)
	    params = (char*)REALLOC(params, (32 + currend) * sizeof(char));
	sprintf(&params[currend],"REPEAT %d\n",this->getOutputRepeatCount());
    }

    return params;
}
#define MDF_SWITCH		0x01
#define MDF_ERR_CONT		0x02
#define MDF_PIN			0x04
#define MDF_SIDE_EFFECT		0x08
#define MDF_PERSISTENT  	0x10
#define MDF_ASYNCHRONOUS 	0x20
#define MDF_REROUTABLE   	0x40
#define MDF_REACH		0x80
#define MDF_LOOP		0x100

void NodeDefinition::setMDFFlag(boolean val, long flag)
{   
    if (val)
        this->mdf_flags |=  flag;
    else
        this->mdf_flags &=  ~flag;
}
void NodeDefinition::setMDFFlagERR_CONT(boolean val)
{
    this->setMDFFlag(val, MDF_ERR_CONT);
}
boolean NodeDefinition::isMDFFlagERR_CONT()
{
    return (this->mdf_flags & MDF_ERR_CONT) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagSWITCH(boolean val )
{
    this->setMDFFlag(val, MDF_SWITCH);
}
boolean NodeDefinition::isMDFFlagSWITCH()
{
    return (this->mdf_flags & MDF_SWITCH) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagPIN(boolean val )
{
    this->setMDFFlag(val, MDF_PIN);
}
boolean NodeDefinition::isMDFFlagPIN()
{
    return (this->mdf_flags & MDF_PIN) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagLOOP(boolean val )
{
    this->setMDFFlag(val, MDF_LOOP);
}
boolean NodeDefinition::isMDFFlagLOOP()
{
    return (this->mdf_flags & MDF_LOOP) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagSIDE_EFFECT(boolean val )
{
    this->setMDFFlag(val, MDF_SIDE_EFFECT);
}
boolean NodeDefinition::isMDFFlagSIDE_EFFECT()
{
    return (this->mdf_flags & MDF_SIDE_EFFECT) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagPERSISTENT(boolean val )
{
    this->setMDFFlag(val, MDF_PERSISTENT);
}
boolean NodeDefinition::isMDFFlagPERSISTENT()
{
    return (this->mdf_flags & MDF_PERSISTENT) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagASYNCHRONOUS(boolean val )
{
    this->setMDFFlag(val, MDF_ASYNCHRONOUS);
}
boolean NodeDefinition::isMDFFlagASYNCHRONOUS()
{
    return (this->mdf_flags & MDF_ASYNCHRONOUS) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagREROUTABLE(boolean val )
{
    this->setMDFFlag(val, MDF_REROUTABLE);
}
boolean NodeDefinition::isMDFFlagREROUTABLE()
{
    return (this->mdf_flags & MDF_REROUTABLE) ? TRUE : FALSE ;
}
void NodeDefinition::setMDFFlagREACH(boolean val )
{
    this->setMDFFlag(val, MDF_REACH);
}
boolean NodeDefinition::isMDFFlagREACH()
{
    return (this->mdf_flags & MDF_REACH) ? TRUE : FALSE ;
}
void NodeDefinition::setDefaultOutboardHost(const char *host)
{
   if (this->outboardHost)
	delete[] this->outboardHost;
   this->outboardHost = DuplicateString(host);
}
boolean NodeDefinition::isOutboard()
{
   return (this->outboardCommand != NULL);
}
void NodeDefinition::setOutboardCommand(const char *command)
{
   if (this->outboardCommand)
	delete[] this->outboardCommand;

   if (this->loadFile) {
        delete[] this->loadFile;
	this->loadFile = NULL;
   }

   this->outboardCommand = DuplicateString(command);
}
boolean NodeDefinition::isDynamicallyLoaded()
{
   return (this->loadFile != NULL);
}
const char *NodeDefinition::getDynamicLoadFile()
{
    return this->loadFile;
}
void NodeDefinition::setDynamicLoadFile(const char *file)
{
   if (this->loadFile) 
	delete[] this->loadFile;

   //
   // I tool can be either outboard or inboard (static) or inboard
   // (dynamic), but not both.
   //
   if (this->outboardCommand) {
        delete[] this->outboardCommand;
	this->outboardCommand = NULL;
   }

   this->loadFile = DuplicateString(file);
}
void NodeDefinition::setDescription(const char *d)
{ 
    if (this->description)
	delete[] this->description;

    this->description = DuplicateString(d);
}

// 
// Add a selectable value for the n'th input.
//
boolean NodeDefinition::addInputValueOption(int n, const char *value)
{
    
    ParameterDefinition *pd = this->getInputDefinition(n);
    if (pd) 
	return pd->addValueOption(value);
    return FALSE;
}
void NodeDefinition::setUILoadedOnly()
{
    this->uiLoadedOnly = TRUE;
}
boolean NodeDefinition::isUILoadedOnly()
{
    return this->uiLoadedOnly;
}

#if 0 	// This is in Node.C
// 
// Get the selectable values for the n'th input.
//
const char * const *NodeDefinition::getInputValueOptions(int n)
{
    
    ParameterDefinition *pd = this->getInputDefinition(n);
    if (pd) 
	return pd->getValueOptions(options);
    if (options)
	*options = NULL;
    return 0;
}
#endif



