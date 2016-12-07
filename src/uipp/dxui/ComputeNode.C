/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <ctype.h>

#include "ComputeNode.h"
#include "ErrorDialogManager.h"
#include "DXType.h"

ComputeNode::ComputeNode(NodeDefinition *nd, Network *net, int instnc) :
    Node(nd, net, instnc)
{
    this->expression = DuplicateString("");
    this->exposedExpression = FALSE;
}

ComputeNode::~ComputeNode()
{
    if (this->expression != NULL)
	delete expression;
    for (int i = 1; i < this->getInputCount(); ++i)
    {
	char *oldName = (char *)this->nameList.getElement(i);
	delete oldName;
    }
    this->nameList.clear();
}

boolean 
ComputeNode::initialize()
{
    int i;
    int count = this->getInputCount();
    char name[2];

    name[1] = '\0';
    for (i = 2; i <= count; ++i)
    {
	name[0] = 'a' + i - 2;
	this->setName(name, i - 1, FALSE);
    }
    return TRUE;
}

const char *
ComputeNode::getExpression()
{
    return this->expression;
}

void
ComputeNode::setExpression(const char *expr, boolean send)
{
    if (this->expression)
	delete this->expression;
    this->expression = DuplicateString(expr);
    this->exposedExpression = FALSE;

    char *input = this->resolveExpression();
    this->setInputValue(1, input, DXType::StringType, send);
    delete input;
}

void
ComputeNode::setName(const char *name, int i, boolean send)
{
    char *oldName = (char *)this->nameList.getElement(i);
    if (oldName)
    {
	delete oldName;
	this->nameList.replaceElement((void*)DuplicateString(name), i);
    }
    else
	this->nameList.insertElement((void*)DuplicateString(name), i);
	
    char *input = this->resolveExpression();
    this->setInputValue(1, input, DXType::StringType, send);
    delete input;
}
const char *
ComputeNode::getName(int i)
{
    return (char *)this->nameList.getElement(i);
}

boolean
ComputeNode::netParseAuxComment(const char *comment,
				const char *file,
				int lineno)
{
    // Compute comments are of the form
    // expression: value = a*0.1
    // name[2]: value = a
    if (strncmp(" expression:", comment, STRLEN(" expression:")) == 0)
    {
	this->setExpression(comment + STRLEN(" expression: value = "), FALSE);
	return TRUE;
    }
    else if (strncmp(" name[", comment, STRLEN(" name[")) == 0)
    {
	char *s = new char[STRLEN(comment)];
	int   i;
	if (sscanf(comment, " name[%d]: value = %[^\n]", &i, s) == 2)
	{
	    this->setName(s, i-1, FALSE);
	    
	} else {
	    ErrorMessage("Erroneous 'name' comment file %s, line %d",
				file,lineno);
	}
	delete s;
	return TRUE;
    }
    else
	return Node::netParseAuxComment(comment, file, lineno);
}

char *
ComputeNode::resolveExpression()
{
	if (this->expression == NULL)
		return DuplicateString("");

	int j;
	int nameSize = this->nameList.getSize();
	int len = 0;
	for (j = 1; j <= nameSize; ++j)
	{
		len += STRLEN((char *)this->nameList.getElement(j)) + 1;
	}

	int  outLen = 3 + STRLEN(this->expression) + 2 * len;
	char *output = (char*)MALLOC(outLen);
	char token[512];
	boolean substituted = FALSE;
	int i = 0;
	j = 0;
	int k;

	output[j++] = '\"';

	for (;;)
	{
		/*
		* Copy over non-identifier sections of the expression.
		*/
		while (this->expression[i] &&
			!isalpha(this->expression[i]) &&
			this->expression[i] != '_')
		{
			if (j >= outLen)
			{
				outLen *= 2;
				output = (char*)REALLOC(output, outLen);
			}
			output[j++] = this->expression[i++];
		}

		/*
		* Get out of here if end of expression string.
		*/
		if (!this->expression[i])
			break;

		/*
		* Extract the identifier token.
		*/
		k = 0;
		while (isalpha(this->expression[i]) ||
			isdigit(this->expression[i]) ||
			this->expression[i] == '_')
		{
			token[k++] = this->expression[i++];
		}
		token[k] = '\0';

		/*
		* Is the identifier a component name (".x", ".y", etc.)?
		*/
		substituted = FALSE;
		if (j == 0 || (j > 0 && output[j - 1] != '.'))
		{
			/*
			* The name token is not a component name (".x", ".y", etc.).
			* Compare it against the name list and substitute if found.
			*/
			for (k = 1; k <= nameSize; k++)
			{
				if (EqualString(token, (char *)this->nameList.getElement(k)))
				{
					substituted = TRUE;

					if (j >= outLen)
					{
						outLen *= 2;
						output = (char*)REALLOC(output, outLen);
					}
					output[j++] = '$';

					k--;
					if (k < 10)
					{
						if (j >= outLen)
						{
							outLen *= 2;
							output = (char*)REALLOC(output, outLen);
						}
						output[j++] = '0' + k;
					}
					else if (k >= 10 && k < 100)
					{
						if (j >= outLen-1)
						{
							outLen *= 2;
							output = (char*)REALLOC(output, outLen);
						}
						output[j++] = '0' + (k / 10);
						output[j++] = '0' + (k % 10);
					}
					else
					{
						/*
						* If the number of Compute parameters ever go
						*  beyond 99, we'll worry about it then...
						*/
						ASSERT(FALSE);
					}
					break;
				}
			}
		}

		/*
		* Yes, it is a component name.
		* Don't substitute ".x", ".y", ".z" stuff.
		*/
		if (!substituted)
		{
			if (j+STRLEN(token) >= outLen)
			{
				outLen *= 2;
				output = (char*)REALLOC(output, outLen);
			}
			for (k = 0; token[k]; j++, k++)
			{
				output[j] = token[k];
			}
		}
	}

	if (j >= outLen-1)
	{
		outLen *= 2;
		output = (char*)REALLOC(output, outLen);
	}
	output[j++] = '\"';
	output[j] = '\0';
	char *realOutput = DuplicateString(output);
	FREE(output);
	return realOutput;
}

boolean ComputeNode::netPrintAuxComment(FILE *f)
{
    if (!this->Node::netPrintAuxComment(f))
	return FALSE;

    if (this->expression &&
	fprintf(f, "    // expression: value = %s\n", this->expression) < 0)
	return FALSE;

    int i;
    int s = this->nameList.getSize();
    for (i = 1; i <= s; ++i)
    {
	if (fprintf(f,
		"    // name[%d]: value = %s\n",
		i + 1,
		(char *)this->nameList.getElement(i)) < 0)
	    return FALSE;
    }

    return TRUE;
}

//
// Add/remove a set of repeatable input or output parameters to the
// this node.   An error  ocurrs if the parameter list indicated does
// not have repeatable parameters.
//
boolean ComputeNode::addRepeats(boolean input)
{
    
    if (input)
    {
	char name[2];
	int i = this->getInputCount() + 1;

	name[0] = 'a' + i - 2;
	name[1] = '\0';
	this->setName(name, i - 1, FALSE);
    }

    return this->Node::addRepeats(input);
}
boolean ComputeNode::removeRepeats(boolean input)
{
    if (input)
    {
	int i = this->getInputCount();
	char *oldName = (char *)this->nameList.getElement(i - 1);
	if (oldName)
	{
	    delete oldName;
	    this->nameList.deleteElement(i - 1);
	}
    }

    return this->Node::removeRepeats(input);
}
//
// Determine if this node is of the given class.
//
boolean ComputeNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassComputeNode);
    if (s == classname)
	return TRUE;
    else
	return this->Node::isA(classname);
}
