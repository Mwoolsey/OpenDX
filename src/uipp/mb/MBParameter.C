/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "MBParameter.h"
#include "DXStrings.h"

boolean MBParameter::MBParameterClassInitialized = FALSE;

MBParameter::MBParameter() : Base()
{
    //
    // Initialize member data.
    //

    this->name = NULL;
    this->type_name = NULL;
    this->description = NULL;
    this->default_value = NULL;
    this->structure = DuplicateString("Field/Group");
    this->data_type = DuplicateString("float");
    this->data_shape = DuplicateString("Scalar");
    this->counts = DuplicateString("1");
    this->positions = DuplicateString("Not required");
    this->connections = DuplicateString("Not required");
    this->element_type = DuplicateString("Not required");
    this->dependency = DuplicateString("Positions or connections");
    this->required = FALSE;
    this->descriptive = FALSE;
    this->type = 0;

}

void MBParameter::setType(unsigned long type)
{
    this->type = type;
}
void MBParameter::setTypeName(char * name)
{
    this->type_name = name;
}
void MBParameter::addType(unsigned long type)
{
    this->type |= type;
}
void MBParameter::removeType(unsigned long type)
{
    this->type &= ~type;
}
void MBParameter::setRequired(boolean required)
{
    this->required = required;
}
void MBParameter::setDescriptive(boolean descriptive)
{
    this->descriptive = descriptive;
}
void MBParameter::setName(char *name)
{
    if(this->name)
        delete this->name;
    this->name = DuplicateString(name);
}
void MBParameter::setDescription(char *description)
{
    if(this->description)
        delete this->description;
    this->description = DuplicateString(description);
}
void MBParameter::setDefaultValue(char *default_value)
{
    if(this->default_value)
        delete this->default_value;
    this->default_value = DuplicateString(default_value);
}
void MBParameter::setStructure(char *structure)
{
    if(this->structure)
        delete this->structure;
    this->structure = DuplicateString(structure);
}
void MBParameter::setDataType(char *data_type)
{
    if(this->data_type)
        delete this->data_type;
    this->data_type = DuplicateString(data_type);
}
void MBParameter::setDataShape(char *data_shape)
{
    if(this->data_shape)
        delete this->data_shape;
    this->data_shape = DuplicateString(data_shape);
}
void MBParameter::setsimpleDataShape(char *data_shape)
{
    if(this->data_shape)
        delete this->simple_data_shape;
    this->simple_data_shape = DuplicateString(simple_data_shape);
}
void MBParameter::setCounts(char *counts)
{
    if(this->counts)
        delete this->counts;
    this->counts = DuplicateString(counts);
}
void MBParameter::setPositions(char *positions)
{
    if(this->positions)
        delete this->positions;
    this->positions = DuplicateString(positions);
}
void MBParameter::setConnections(char *connections)
{
    if(this->connections)
        delete this->connections;
    this->connections = DuplicateString(connections);
}
void MBParameter::setElementType(char *element_type)
{
    if(this->element_type)
        delete this->element_type;
    this->element_type = DuplicateString(element_type);
}
void MBParameter::setDependency(char *dependency)
{
    if(this->dependency)
        delete this->dependency;
    this->dependency = DuplicateString(dependency);
}

MBParameter::~MBParameter()
{
    if(this->name) delete this->name;
    if(this->description) delete this->description;
    if(this->default_value) delete this->default_value;
    if(this->structure) delete this->structure;
    if(this->data_type) delete this->data_type;
    if(this->data_shape) delete this->data_shape;
    if(this->counts) delete this->counts;
    if(this->positions) delete this->positions;
    if(this->connections) delete this->connections;
    if(this->element_type) delete this->element_type;
    if(this->dependency) delete this->dependency;
}
