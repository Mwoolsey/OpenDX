/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "Field.h"
#include "../base/DXStrings.h"

boolean Field::FieldClassInitialized = FALSE;


Field::Field(char* name) : Base()
{
    //
    // Initialize member data.
    //

    this->name = DuplicateString(name); 
    this->layout_skip = DuplicateString("0");
    this->layout_width = DuplicateString("0");
    this->block_skip = DuplicateString("0");
    this->block_element = DuplicateString("0");
    this->block_width = DuplicateString("0");
    this->dependency = DuplicateString("positions");
    this->type = DuplicateString("double");
    this->structure = DuplicateString("scalar");
}

void Field::setType(char *type)
{
    if(this->type)
        delete this->type;
    this->type = DuplicateString(type);
}
void Field::setStructure(char *structure)
{
    if(this->structure)
        delete this->structure;
    this->structure = DuplicateString(structure);
}
void Field::setName(char *name)
{
    if(this->name)
        delete this->name;
    this->name = DuplicateString(name);
}
void Field::setLayoutSkip(char *layout_skip)
{
    if(this->layout_skip)
        delete this->layout_skip;
    this->layout_skip = DuplicateString(layout_skip);
}
void Field::setLayoutWidth(char *layout_width)
{
    if(this->layout_width)
        delete this->layout_width;
    this->layout_width = DuplicateString(layout_width);
}
void Field::setBlockSkip(char *block_skip)
{
    if(this->block_skip)
        delete this->block_skip;
    this->block_skip = DuplicateString(block_skip);
}
void Field::setBlockElement(char *block_element)
{
    if(this->block_element)
        delete this->block_element;
    this->block_element = DuplicateString(block_element);
}
void Field::setBlockWidth(char *block_width)
{
    if(this->block_width)
        delete this->block_width;
    this->block_width = DuplicateString(block_width);
}
void Field::setDependency(char *dependency)
{
    if(this->dependency)
        delete this->dependency;
    this->dependency = DuplicateString(dependency);
}

Field::~Field()
{
    if(this->type)
	delete this->type;
    if(this->name)
	delete this->name;
    if(this->structure)
	delete this->structure;
    if(this->layout_skip)
	delete this->layout_skip;
    if(this->layout_width)
	delete this->layout_width;
    if(this->block_skip)
	delete this->block_skip;
    if(this->block_element)
	delete this->block_element;
    if(this->block_width)
	delete this->block_width;
    if(this->dependency)
	delete this->dependency;
}
