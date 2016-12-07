/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "MacroParameterNode.h"
#include "MacroParameterDefinition.h"
#include "Parameter.h"
#include "ParameterDefinition.h"
#include "Network.h"
#include "MacroDefinition.h"
#include "DXApplication.h"
#include "Ark.h"
#include "ConfigurationDialog.h"
#include "DXType.h"
#include "ListIterator.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "lex.h"
#include "DXValue.h"

MacroParameterNode::MacroParameterNode(NodeDefinition *nd,
					Network *net, int instnc) :
    UniqueNameNode(nd, net, instnc)
{
    this->index = -1;
}

	MacroParameterNode::~MacroParameterNode()
	{
		Network *net = this->getNetwork();
		MacroDefinition *md = net->getDefinition();
		Parameter *p;
		ParameterDefinition *pd = this->getParameterDefinition();

		if (this->isInput())
		{

			if (pd && !net->isDeleted() || net == theDXApplication->network)
			{
				if(!net->isDeleted())
				{
					if (this->index != net->getInputCount())
					{
						int newIndex = net->getInputCount()+1;
						this->moveIndex(newIndex, FALSE);
					}
					else
					{
						for (int i = this->index; i > 1; --i)
							if (!this->moveIndex(i - 1, FALSE))
								break;
						// this->index = i;
					}
				}
				if (md) md->removeInput(pd);
				if (pd) delete pd; 
			}
			p = this->getOutputParameter(1);
		}
		else
		{
			if (pd && !net->isDeleted() || net == theDXApplication->network)
			{
				if(!net->isDeleted())
				{
					if (this->index != net->getOutputCount())
					{
						int newIndex = net->getOutputCount()+1;
						this->moveIndex(newIndex, FALSE);
					}
					else
					{
						for (int i = this->index; i > 1; --i)
							if (!this->moveIndex(i - 1, FALSE))
								break;
					}
				}
				if (md) md->removeOutput(pd);
				if (pd) delete pd; 
			}
			p = this->getInputParameter(1);
		}

		if(p) delete p->getDefinition();

		if (net == theDXApplication->network)
		{
			if (md && md->getInputCount() == 0 && md->getOutputCount() == 0)
				net->makeMacro(FALSE);
		}
	}

boolean
MacroParameterNode::initialize()
{
    this->UniqueNameNode::initialize();
    Network *net = this->getNetwork();
    if (!net->isMacro())
	net->makeMacro(TRUE);
    MacroDefinition *md = net->getDefinition();
    ParameterDefinition *param=NULL;

    boolean input = this->isInput();
    if (!md->isReadingNet()) {
	param = new ParameterDefinition(-1);
	param->addType(new DXType(DXType::ObjectType));
    }

    char s[100];
    if (input)
    {
	if (!md->isReadingNet()) {
	    int n = md->getFirstAvailableInputPosition();
	    param->markAsInput();
	    if (n <= md->getInputCount()) {
		ParameterDefinition *dummyPd = md->getInputDefinition(n);
		ASSERT(dummyPd->isDummy());
		md->replaceInput(param,dummyPd);
	    } else {
		md->addInput(param);
	    }
	    this->setIndex(n);
	    sprintf(s, "input_%d", this->index);
	    param->setName(s);
	    param->setDefaultValue("(no default)");
	}

	//
	// The Parameter  must have its own ParameterDefinition since
	// it may change depending upon what we are connected to.
	// FIXME: ParameterDefinition should have a dup() method.
	//
	Parameter *p = this->getOutputParameter(1);
	ParameterDefinition *pd = p->getDefinition();
#if 11
	ParameterDefinition *newpd = pd->duplicate();
#else
	ParameterDefinition *newpd = 
	    new ParameterDefinition(pd->getNameSymbol());
	List *l = pd->getTypes();
	DXType *dxt;
	for (ListIterator li(*l); dxt = (DXType*)li.getNext();)
	    newpd->addType(dxt);
	newpd->markAsOutput();
	newpd->setDefaultVisibility(pd->getDefaultVisibility());
	newpd->setViewability(pd->isViewable());
	newpd->setDescription(pd->getDescription());
	newpd->setWriteableCacheability(pd->hasWriteableCacheability());
	newpd->setDefaultCacheability(pd->getDefaultCacheability());
	if (pd->isRequired())
	    newpd->setRequired();
	else
	    newpd->setNotRequired();
	if (pd->isDefaultValue())
	    newpd->setDefaultValue(pd->getDefaultValue());
	else
	    newpd->setDescriptiveValue(pd->getDefaultValue());
#endif
	p->setDefinition(newpd);
    }
    else
    {
	if (!md->isReadingNet()) {
	    int n = md->getFirstAvailableOutputPosition();
	    param->markAsOutput();
	    if (n <= md->getOutputCount()) {
		ParameterDefinition *dummyPd = md->getOutputDefinition(n);
		ASSERT(dummyPd->isDummy());
		md->replaceOutput(param,dummyPd);
	    } else {
		md->addOutput(param);
	    }
	    this->setIndex(n);
	    sprintf(s, "output_%d", this->index);
	    param->setName(s);
	    param->setDefaultValue("(no default)");
	}

	//
	// The Parameter  must have its own ParameterDefinition since
	// it may change depending upon what we are connected to.
	// FIXME: ParameterDefinition should have a dup() method.
	//
	Parameter *p = this->getInputParameter(1);
	ParameterDefinition *pd = p->getDefinition();
#if 11
	ParameterDefinition *newpd = pd->duplicate();
#else
	ParameterDefinition *newpd =
	    new ParameterDefinition(pd->getNameSymbol());
	List *l = pd->getTypes();
	DXType *dxt;
	for (ListIterator li(*l); dxt = (DXType*)li.getNext();)
	    newpd->addType(dxt);
	newpd->markAsInput();
	newpd->setDefaultVisibility(pd->getDefaultVisibility());
	newpd->setViewability(pd->isViewable());
	newpd->setDescription(pd->getDescription());
	newpd->setWriteableCacheability(pd->hasWriteableCacheability());
	newpd->setDefaultCacheability(pd->getDefaultCacheability());
	if (pd->isRequired())
	    newpd->setRequired();
	else
	    newpd->setNotRequired();
	if (pd->isDefaultValue())
	    newpd->setDefaultValue(pd->getDefaultValue());
	else
	    newpd->setDescriptiveValue(pd->getDefaultValue());
#endif
	p->setDefinition(newpd);
    }
    return TRUE;
}

const char *MacroParameterNode::getParameterNameString()
{
    ParameterDefinition *pd = this->getParameterDefinition();
    ASSERT(pd);
    return pd->getNameString();
}

ParameterDefinition *MacroParameterNode::getParameterDefinition(
				boolean includeDummies)
{
    MacroDefinition *md = this->getNetwork()->getDefinition();

    //
    // During deletion of a MacroDefinition, there is no MacroDefinition
    // for this node and therefore no ParameterDefinition.  So return NULL.
    //
    if (!md)
	return NULL;

    int idx = this->getIndex();
    if (idx <= 0)
	return NULL;

    if (this->isInput()) {
	ASSERT(this->isInputRepeatable() == FALSE);
 	if (idx > md->getInputCount())
	    return NULL;
    } else {
	ASSERT(this->isOutputRepeatable() == FALSE);
	if (idx > md->getOutputCount())
	    return NULL;
    }

    ParameterDefinition *pd;
    if (this->isInput()) {
	if (includeDummies)
	    pd = md->getInputDefinition(idx);
	else  
	    pd = md->getNonDummyInputDefinition(idx);
    } else {
	if (includeDummies)
	    pd = md->getOutputDefinition(idx);
	else  
	    pd = md->getNonDummyOutputDefinition(idx);
    }

    return pd;
}

char *MacroParameterNode::netNodeString(const char *prefix)
{
    char *string;
    ParameterDefinition *param = this->getParameterDefinition();
    ASSERT(param);
    const char *name = param->getNameString();
    if (this->isInput())
    {
	ASSERT(this->getOutputCount() == 1);
	char *outputs = this->outputParameterNamesString(prefix);
	string = new char[STRLEN(outputs) + STRLEN(name) + 6];
	sprintf(string, "%s = %s;\n", outputs, name);
	delete outputs;
    }
    else
    {
	ASSERT(this->getInputCount() == 1);
	char *inputs = this->inputParameterNamesString(prefix);
	string = new char[STRLEN(inputs) + STRLEN(name) + 6];
	sprintf(string, "%s = %s;\n", name, inputs);
	delete inputs;
    }
    return string;
}

boolean  MacroParameterNode::netParseAuxComment(const char* comment,
							const char *file,
							int lineno)
{
#define PARAM_COMMENT " parameter: position = "
#define PARAM_FMT " parameter: position = %d, name = '%[^']', value = '%[^']'"\
	", descriptive = %d, description = '%[^']', required = %d, visible = %d"
#define PARAM_FMT1 " parameter: position = %d, name = '%[^']', value = '%[^']'"\
	", descriptive = %d, description = '%[^']', required = %d"

	if (EqualSubstring(PARAM_COMMENT, comment, STRLEN(PARAM_COMMENT)))
	{
		int pos;
		char name[1000];
		char value[1000];
		int descriptive;
		char description[1000];
		int required, visible;
		int itemsParsed = sscanf(comment, PARAM_FMT, 
			&pos,
			name,
			value,
			&descriptive,
			description,
			&required,
			&visible);
		if (itemsParsed != 7) {
			itemsParsed = sscanf(comment, PARAM_FMT1, 
				&pos,
				name,
				value,
				&descriptive,
				description,
				&required);
			if (itemsParsed != 6)
				return this->UniqueNameNode::netParseAuxComment(comment,file,lineno);
			visible = TRUE;
		}
		if (!this->getNetwork()->isMacro())
			this->getNetwork()->makeMacro(TRUE);
		if (this->isInput())
		{
			this->moveIndex(pos);
			ParameterDefinition *pd = this->getParameterDefinition();
			pd->setName(name);
			pd->setDescription(description);
			if (required)
				pd->setRequired();
			else if (descriptive)
				pd->setDescriptiveValue(value);
			else if (IsBlankString(value))
				pd->setDefaultValue("(no default)");
			else
				pd->setDefaultValue(value);
			pd->setDefaultVisibility((boolean)visible);
		}
		else
		{
			this->moveIndex(pos);
			ParameterDefinition *pd = this->getParameterDefinition();
			pd->setName(name);
			pd->setDescription(description);
			pd->setDefaultVisibility((boolean)visible);
		}
		return TRUE;
	}
	else
		return this->UniqueNameNode::netParseAuxComment(comment,file,lineno);
}

	
//
// Based on the node's connectivity, select values for OPTIONS
// automatically.  The strategy is
// - Try to keep the OPTIONS the node already has.
// - look for OPTIONS in the next connected downstream node(s)
// - for each value in OPTIONS, see if the value can be coerced
//   to our current type(s)
// - Prefer to use OPTIONS from the node at the other end of 
//   prefer because it's probably the node were adding via ::addIOArk().
//
// At this point in the code, our new list of types has already been
// computed based on our new connectivity.
//
void MacroParameterNode::setTypeSafeOptions (Ark* preferred)
{
	// This method is nonsense in the case of an Output tool.
	ASSERT (this->isInput());

	//
	// If we have OPTIONS && any item in OPTION is illegal input given
	// our new type, then remove all the OPTIONS.
	//
	ParameterDefinition *macroPd = this->getParameterDefinition();
	const char *const *options = macroPd->getValueOptions();
	List *types = macroPd->getTypes();
	if (options && options[0]) {
		boolean can_coerce = TRUE;
		for (int i=0; options[i] && can_coerce; i++) {
			const char* option = options[i];
			can_coerce&= this->canCoerceValue (option, types);
		}
		if (!can_coerce) {
			macroPd->removeValueOptions();
			options = NULL;
		}
	}

	// If our current OPTIONS are safe to keep using...
	if (options && options[0]) return;

	//
	// If the parameter has no option values, then check each downstream
	// input tab.  If any has option values, then try to use those but
	// only if each of the option values is legal given our new type.
	//
	Node* destination;
	NodeDefinition* destinationDef;
	ParameterDefinition* destinationParamDef;
	int paramIndex;

	// First check the new ark to see if it gave give us
	// option values.  option strings are stored as a null-terminated
	// array of char*.
	const char *const *dest_option_values = NULL;
	boolean can_coerce = TRUE;
	if (preferred) {
		destination = preferred->getDestinationNode(paramIndex);
		destinationDef = destination->getDefinition();
		destinationParamDef = destinationDef->getInputDefinition(paramIndex);
		dest_option_values = destinationParamDef->getValueOptions();

		if (dest_option_values && dest_option_values[0]) {
			for (int i=0; dest_option_values[i]&&can_coerce; i++) {
				can_coerce&= this->canCoerceValue (dest_option_values[i], types);
			}
		}
	}

	if ((can_coerce) && (dest_option_values) && (dest_option_values[0])) {
		for (int i=0; dest_option_values[i];  i++) {
			if (!macroPd->addValueOption (dest_option_values[i])) {
				ErrorMessage(
					"Cannot add %s to Options values ", 
					dest_option_values[i]);
				break;
			}
		}
	} else {
		// get all the destination nodes  When we're called as via ::addIOArk(),
		// the new connection has not yet been added to our list
		// of downstream nodes.
		List *arks = (List*)this->getOutputArks(1);
		ListIterator iter(*arks);
		//iter.setList(*arks);

		// the new ark didn't supply options values so check
		// all the other arks.  These loops say:
		// For each arc
		//     For each type
		Ark* ark;
		while ((ark = (Ark*)iter.getNext()) != NULL) {
			destination = ark->getDestinationNode(paramIndex);
			destinationDef = destination->getDefinition();
			destinationParamDef = destinationDef->getInputDefinition(paramIndex);

			// for each option value see, see if it can be coerced.
			dest_option_values = destinationParamDef->getValueOptions();
			if ((!dest_option_values) || (!dest_option_values[0])) continue;

			can_coerce = TRUE;
			for (int i=0; dest_option_values[i]&&can_coerce; i++) {
				can_coerce&= this->canCoerceValue (dest_option_values[i], types);
			}
			if (can_coerce) {
				for (int i=0; dest_option_values[i];  i++) {
					if (!macroPd->addValueOption (dest_option_values[i])) {
						ErrorMessage(
							"Cannot add %s to Options values ", 
							dest_option_values[i]);
						// This is a tough spot.  We've checked the values
						// already and determined that all are safe, however
						// we failed in putting one of them into the node.
						// So we bail on all the others and we bail on
						// checking for other nodes.  This probably never
						// happens unless the user enters more than 64
						// options, which is something the node will refuse
						// to handle.
						break;
					}
				}
				break;
			}
		}
	}
}

boolean
MacroParameterNode::canCoerceValue (const char* option, List* types)
{
	ListIterator iter;
	boolean coerced = FALSE;
	DXType* dxtype;
	for (iter.setList(*types) ; (dxtype = (DXType*)iter.getNext()) ; ) {
		char* s = DXValue::CoerceValue (option, dxtype->getType());
		if (s) {
			coerced = TRUE;
			delete s;
			break;
		}
	}
	return coerced;
}
	
boolean MacroParameterNode::addIOArk(List *io, int index, Ark *a)
{
	List *newTypesList = NULL;
	if (this->isInput())
	{
		int i;
		Node *dest = a->getDestinationNode(i);
		Parameter *pin = ((MacroParameterNode*)dest)->getInputParameter(i);
		ParameterDefinition *pind = pin->getDefinition();

		ParameterDefinition *macroPd = this->getParameterDefinition();
		List *outTypes = macroPd->getTypes();
		newTypesList = DXType::IntersectTypeLists(
			*outTypes,
			*pind->getTypes());

		ListIterator li(*outTypes);
		DXType *t;
		while( (t = (DXType*)li.getNext()) )
		{
			macroPd->removeType(t);
		}

		Parameter *pout = this->getOutputParameter(1);
		ParameterDefinition *nodePd = pout->getDefinition();
		outTypes = nodePd->getTypes();

		li.setList(*outTypes);
		while( (t = (DXType*)li.getNext()) )
		{
			nodePd->removeType(t);
		}

		li.setList(*newTypesList);
		while( (t = (DXType*)li.getNext()) )
		{
			DXType *newt = t->duplicate(); 
			nodePd->addType(newt);
			macroPd->addType(t);
		}
		delete newTypesList;

		this->setTypeSafeOptions(a);
	}
	else
	{
		int i;
		Node *dest = a->getSourceNode(i);
		Parameter *pin = ((MacroParameterNode*)dest)->getOutputParameter(i);
		ParameterDefinition *pind = pin->getDefinition();

		ParameterDefinition *macroPd = this->getParameterDefinition();
		List *outTypes = macroPd->getTypes();
		newTypesList = DXType::IntersectTypeLists(
			*outTypes,
			*pind->getTypes());

		ListIterator li(*outTypes);
		DXType *t;
		while( (t = (DXType*)li.getNext()) )
		{
			macroPd->removeType(t);
		}

		Parameter *pout = this->getInputParameter(1);
		ParameterDefinition *nodePd = pout->getDefinition();
		outTypes = nodePd->getTypes();

		li.setList(*outTypes);
		while( (t = (DXType*)li.getNext()) )
		{
			nodePd->removeType(t);
		}

		li.setList(*newTypesList);
		while( (t = (DXType*)li.getNext()) )
		{
			DXType *newt = t->duplicate(); 
			nodePd->addType(newt);
			macroPd->addType(t);
		}
		delete newTypesList;
	}

	if (this->getConfigurationDialog())
	{
		this->getConfigurationDialog()->changeInput(1);
		this->getConfigurationDialog()->changeOutput(1);
	}

	return this->UniqueNameNode::addIOArk(io, index, a);
}
boolean MacroParameterNode::removeIOArk(List *io, int index, Ark *a)
{
    int destIndex;
    int srcIndex;
    Node* destinationNode = a->getDestinationNode(destIndex);
    Node* sourceNode = a->getSourceNode(srcIndex);
    NodeDefinition* destinationDefinition = destinationNode->getDefinition();
    if (!this->UniqueNameNode::removeIOArk(io, index, a))
	return FALSE;

    ParameterDefinition *macroPd = this->getParameterDefinition();

    //
    // During deletion of a MacroDefinition, there is no MacroDefinition
    // for this node and therefore no ParameterDefinition.  So just return.
    //
    if (!macroPd)
	return TRUE;

    Parameter *pout;
    if (this->isInput())
	pout = this->getOutputParameter(1);
    else
	pout = this->getInputParameter(1);

    ParameterDefinition *nodePd = pout->getDefinition();

    List *outTypes = macroPd->getTypes();
    DXType *t;
    while( (t = (DXType*)(outTypes->getElement(1))) )
    {
	macroPd->removeType(t);
    }

    outTypes = nodePd->getTypes();
    while( (t = (DXType*)(outTypes->getElement(1))) )
    {
	nodePd->removeType(t);
    }

    // for each arc connected to this, intersect this type
    // list with "typesList" and set this to be the current types list.
    const List *arcs;
    if (this->isInput())
	arcs = this->getOutputArks(srcIndex);
    else
	arcs = this->getInputArks(destIndex);
    

    ListIterator li;
    li.setList(*(List *)arcs);

    List *typesList = new List;
    typesList->appendElement(new DXType(DXType::ObjectType));

    while( (a = (Ark*)li.getNext()) )
    {
	ParameterDefinition *pind;
	if (this->isInput())
	{
	    int dummy;
	    Node *dest = a->getDestinationNode(dummy);
	    pind = ((MacroParameterNode*)dest)->getInputParameter(dummy)->
				    getDefinition();
	}
	else
	{
	    int dummy;
	    Node *dest = a->getSourceNode(dummy);
	    pind = ((MacroParameterNode*)dest)->getOutputParameter(dummy)->
				    getDefinition();
	}
	List *newTypesList = DXType::IntersectTypeLists(
	    *typesList,
	    *pind->getTypes());
	// FIXME:  don't you have to delete the items in the list first?
	delete typesList;
	typesList = newTypesList;
    }


    li.setList(*typesList);
    while( (t = (DXType*)li.getNext()) )
    {
	DXType *newt = t->duplicate(); 
	nodePd->addType(newt);
	macroPd->addType(t);
    }
    delete typesList;

    if (this->isInput()) {
	const char *const *dest_option_vals;
	const char *const *src_option_vals;
	// If our current option values are equal to the options
	// values at the other end of the ark that we're disconnecting,
	// then assume that our option values came over this ark, and
	// toss ours out.  This prevents us from discarding a user's input.
	boolean we_disconnected_the_one = FALSE;

	ParameterDefinition *dpd = destinationDefinition->getInputDefinition(destIndex);
	dest_option_vals = dpd->getValueOptions();
	ParameterDefinition *macroPd = this->getParameterDefinition();
	src_option_vals = macroPd->getValueOptions();

	if (src_option_vals && src_option_vals[0] && 
	    dest_option_vals && dest_option_vals[0]) {
	    int i=0;
	    while (dest_option_vals[i] && src_option_vals[i]) {
		if (!EqualString (dest_option_vals[i], src_option_vals[i])) break;
		i++;
	    }
	    if ((dest_option_vals[i] == NULL) && (src_option_vals[i] == NULL))
		we_disconnected_the_one = TRUE;
	}

	if (we_disconnected_the_one) {
	    macroPd->removeValueOptions();
	    this->setTypeSafeOptions();
	}
    }
    if (this->isInput())
    {
	if (this->getConfigurationDialog())
	    this->getConfigurationDialog()->changeInput(1);
    }
    else
    {
	if (this->getConfigurationDialog())
	    this->getConfigurationDialog()->changeOutput(1);
    }
    return TRUE;
}

void MacroParameterNode::setIndex(int index)
{
    this->index = index;
}
boolean MacroParameterNode::moveIndex(int index, boolean issue_error)
{
    if (index == this->index)
	return TRUE;
    if (this->index == -1)
    {
	this->setIndex(index);
	return TRUE;
    }
    //ParameterDefinition *macroPd = this->getParameterDefinition();
    boolean result;
    if (this->isInput())
	result = this->getNetwork()->moveInputPosition(this, index);
    else
	result = this->getNetwork()->moveOutputPosition(this, index);
    if (result)
	this->setIndex(index);
    else if (issue_error)
	ErrorMessage("Cannot move %s %d to %d, "
		     "it would overwrite another %s.",
		     this->getNameString(), this->index, index,
		     this->getNameString());
    if (this->getConfigurationDialog())
    {
	if (this->isInput())
	    this->getConfigurationDialog()->changeInput(1);
	else
	    this->getConfigurationDialog()->changeOutput(1);
    }
    return result;
}

boolean MacroParameterNode::netPrintAuxComment(FILE *f)
{
    if (!this->UniqueNameNode::netPrintAuxComment(f))
	return FALSE;

    ParameterDefinition *pd = this->getParameterDefinition();
#define PARAM_OFMT " parameter: position = %d, name = '%s', value = '%s'"\
		   ", descriptive = %d, description = '%s', required = %d"\
		   ", visible = %d\n"
    
    const char *dflt = pd->getDefaultValue();
    if (dflt == NULL || *dflt == '\0' || EqualString(dflt,"NULL"))
	dflt = " ";
    const char *desc = pd->getDescription();
    if (desc == NULL || *desc == '\0')
	desc = " ";

    if (fprintf(f, "    //" PARAM_OFMT,
	this->index,
	pd->getNameString(),
	dflt, 
	pd->isDefaultDescriptive(),
	desc,
	pd->isRequired(),
	pd->getDefaultVisibility()) < 0)
	return FALSE;
    return TRUE;
}
//
// Determine if this node an input or output 
// Should not be called until after initialization.
//
boolean MacroParameterNode::isInput()
{ 
    return this->getDefinition()->getInputCount() == 0;
}

//
// Determine if this node is of the given class.
//
boolean MacroParameterNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassMacroParameterNode);
    if (s == classname)
	return TRUE;
    else
	return this->UniqueNameNode::isA(classname);
}

//
// Match output_index of this node to input_index of n.  Returns TRUE
// if they can connect.
//
boolean MacroParameterNode::typeMatchOutputToInput(
    int output_index,
    Node *n,
    int input_index)
{
    ASSERT( output_index >= 1 );
    ASSERT( input_index >= 1 );
 
    List *tout = this->getOutputTypes(output_index);
    List *tin  = n->getInputTypes(input_index);
    ASSERT(tin && tout);

    List *newTypesList = DXType::IntersectTypeLists(*tin, *tout);
    boolean result = (newTypesList != NULL && newTypesList->getSize() != 0);
    if (newTypesList != NULL) 
	// FIXME: don't you have to delete the items in the list first?
	delete newTypesList;

    return result;
}

const char* MacroParameterNode::getUniqueName()
{
    ParameterDefinition *pd = this->getParameterDefinition();
    return pd->getNameString();
}

boolean MacroParameterNode::canSwitchNetwork(Network *from, Network *to)
{

    if (!to->canBeMacro()) {
	WarningMessage(
		"Attempt to add %s tool to network would make the \n"
		"network a macro, but the network contains tools which will\n"
		"not allow it to become a macro.\n"
		"Attempt ignored.", this->getNameString());
	return FALSE; 
    }

    return this->UniqueNameNode::canSwitchNetwork(from,to);
}

void MacroParameterNode::switchNetwork(Network *from, Network *to, boolean silently)
{
    MacroDefinition *md = from->getDefinition();
    ParameterDefinition *pd = this->getParameterDefinition();
    ParameterDefinition *dummyPd;
    int n;

    dummyPd = new ParameterDefinition(-1);
    dummyPd->setDummy(TRUE);
    dummyPd->addType(new DXType(DXType::ObjectType));

    if(this->isInput())
	md->replaceInput(dummyPd, pd);
    else
	md->replaceOutput(dummyPd, pd);

    //
    // Switch the Network pointers
    //
    this->UniqueNameNode::switchNetwork(from, to, silently);

    //
    // Make sure we are a macro
    //
    to->makeMacro(TRUE);

    md = to->getDefinition();

    if(this->isInput())
    {
	n = md->getFirstAvailableInputPosition();

	if (n <= md->getInputCount()) {
	    dummyPd = md->getInputDefinition(n);
	    ASSERT(dummyPd->isDummy());
	    md->replaceInput(pd,dummyPd);
	} else {
	    md->addInput(pd);
	}
	this->setIndex(n);
    }
    else
    {
	n = md->getFirstAvailableOutputPosition();

	if (n <= md->getOutputCount()) {
	    ParameterDefinition *dummyPd = md->getOutputDefinition(n);
	    ASSERT(dummyPd->isDummy());
	    md->replaceOutput(pd,dummyPd);
	} else {
	    md->addOutput(pd);
	}
	this->setIndex(n);
    }

    //
    // See if the input is named input_%d or output_%d
    // and if so, change it to match the possibly new index.
    //
    pd = this->getParameterDefinition();
    char buf[64];
    if (this->isInput()) {
	strcpy(buf,"input_");
    } else {
	strcpy(buf,"output_");
    }
    int buflen = strlen(buf);
    const char* current_name = pd->getNameString();
    const char* new_name = NUL(char*);
    boolean name_reset = FALSE;
    if (EqualSubstring(current_name, buf, buflen)) {
	int endint = 0;
	const char *end = &current_name[strlen(current_name)];
	if (IsInteger(current_name+buflen,endint) &&
	    ((current_name+buflen+endint) == end)) {
		sprintf(buf+buflen,"%d",this->getIndex());
		pd->setName(buf);
		name_reset = TRUE;
	}
    }

    //
    // Resolve name conflicts with other nodes using global names.
    //
    if (!name_reset) {
	int name_clash=0, i;
	if (this->isInput()) {
	    int count = to->getInputCount();
	    for (i=1 ; !name_clash && (i <= count) ; i++) {
		ParameterDefinition *opd = to->getInputDefinition(i);
		if ((i != this->getIndex()) &&
		    EqualString(current_name,opd->getNameString()))
		    name_clash = i;
	    }
	} else {
	    int count = to->getOutputCount();
	    for (i=1 ; !name_clash && (i <= count) ; i++) {
		ParameterDefinition *opd = to->getOutputDefinition(i);
		if ((i != this->getIndex()) &&
		    EqualString(current_name,opd->getNameString()))
		    name_clash = i;
	    }
	}

	if (name_clash) {
	    char name_buf[64];
	    if (this->isInput()) {
		sprintf(name_buf,"input_%d", this->getIndex());
	    } else {
		sprintf(name_buf,"output_%d", this->getIndex());
	    }
	    new_name = name_buf;

	    if (!silently) 
		WarningMessage (
		    "Parameter name `%s' is the same name used by parameter #%d.\n"
		    "Your macro %s has been renamed \"%s\".",
		    current_name, name_clash, (this->isInput()? "Input" : "Output"),
		    new_name
		);
	    pd->setName(name_buf);
	    name_reset = TRUE;
	}
    }

    if (!name_reset) {
	const char* conflict = to->nameConflictExists(this, this->getUniqueName());
	if (conflict) {
	    char name_buf[64];
	    if (this->isInput()) {
		sprintf(name_buf,"input_%d", this->getIndex());
	    } else {
		sprintf(name_buf,"output_%d", this->getIndex());
	    }
	    new_name = name_buf;

	    if (!silently)
		WarningMessage (
		    "A %s with name \"%s\" already exists.\n"
		    "Your macro %s has been renamed \"%s\".",
		    conflict, this->getUniqueName(), (this->isInput()? "Input" : "Output"),
		    name_buf
		);
	    pd->setName(name_buf);
	    name_reset = TRUE;
	}
    }
}
