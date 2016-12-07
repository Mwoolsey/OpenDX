/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _MBParameter_h
#define _MBParameter_h

#include <Xm/Xm.h>
#include "Base.h"

//
// Class name definition:
//
#define ClassMBParameter	"MBParameter"

//
// MBParameter class definition:
//				
class MBParameter : public Base
{

  private:
    //
    // Private class data:
    //
    static boolean MBParameterClassInitialized;

    char	*name;
    char	*description;
    boolean	required;
    char	*default_value;
    boolean	descriptive;
    unsigned long type;
    char        *type_name;
    char	*structure;
    char	*data_type;
    char	*data_shape;
    char	*simple_data_shape;
    char	*counts;
    char	*positions;
    char	*connections;
    char	*element_type;
    char	*dependency;

  protected:


  public:

    //
    // Constructor:
    //
    MBParameter();

    //
    // Destructor:
    //
    ~MBParameter();

    //
    // set/get the field name
    //
    void setType(unsigned long type);
    void setTypeName(char *name);
    void addType(unsigned long type);
    void removeType(unsigned long type);
    void setRequired(boolean required);
    void setDescriptive(boolean descriptive);
    void setName(char *name);
    void setDescription(char *description);
    void setDefaultValue(char *default_value);
    void setStructure(char *structure);
    void setDataType(char *data_type);
    void setDataShape(char *data_shape);
    void setsimpleDataShape(char *simple_data_shape);
    void setCounts(char *counts);
    void setPositions(char *positions);
    void setConnections(char *connections);
    void setElementType(char *element_type);
    void setDependency(char *dependency);

    //
    // set/get the field type
    //
    unsigned long getType()	{return this->type;		};
    char *getTypeName()		{return this->type_name;	};
    boolean getRequired()	{return this->required;		};
    boolean getDescriptive()	{return this->descriptive;	};
    char *getName()		{return this->name;		};
    char *getDescription()	{return this->description;	};
    char *getDefaultValue()	{return this->default_value;	};
    char *getStructure()	{return this->structure;	};
    char *getDataType()		{return this->data_type;	};
    char *getDataShape()	{return this->data_shape;	};
    char *getsimpleDataShape()	{return this->simple_data_shape;};
    char *getCounts()		{return this->counts;		};
    char *getPositions()	{return this->positions;	};
    char *getConnections()	{return this->connections;	};
    char *getElementType()	{return this->element_type;	};
    char *getDependency()	{return this->dependency;	};

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMBParameter;
    }
};


#endif // _MBParameter_h

