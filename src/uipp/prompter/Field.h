/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _Field_h
#define _Field_h

#include <Xm/Xm.h>
#include "../base/Base.h"

//
// Class name definition:
//
#define ClassField	"Field"


//
// Field class definition:
//				
class Field : public Base
{

  private:
    //
    // Private class data:
    //
    static boolean FieldClassInitialized;

    char	*type;
    char	*name;
    char	*structure;
    char 	*layout_skip;
    char 	*layout_width;
    char	*block_skip;
    char	*block_element;
    char	*block_width;
    char	*dependency;

  protected:


  public:

    //
    // Constructor:
    //
    Field(char *);

    //
    // Destructor:
    //
    ~Field();

    //
    // set/get the field name
    //
    void setName		(char *);
    void setType		(char *);
    void setStructure		(char *);
    void setLayoutSkip		(char *);
    void setLayoutWidth		(char *);
    void setBlockSkip		(char *);
    void setBlockElement	(char *);
    void setBlockWidth		(char *);
    void setDependency		(char *);

    //
    // set/get the field type
    //
    char *getName() 		{ return this->name; }
    char *getType()		{ return this->type; };
    char *getStructure()	{ return this->structure;};
    char *getLayoutSkip()	{ return this->layout_skip; };
    char *getLayoutWidth()	{ return this->layout_width; };
    char *getBlockSkip()	{ return this->block_skip; };
    char *getBlockElement()	{ return this->block_element; };
    char *getBlockWidth()	{ return this->block_width; };
    char *getDependency()	{ return this->dependency; };

    //
    // set/get the field structure
    //

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassField;
    }
};


#endif // _Field_h

