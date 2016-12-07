/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _FileSelectorInstance_h
#define _FileSelectorInstance_h


#include "ValueInstance.h"

class FileSelectorNode;
class FileSelectorDialog;

//
// Class name definition:
//
#define ClassFileSelectorInstance	"FileSelectorInstance"


//
// Describes an instance of an interactor in a control Panel.
//
class FileSelectorInstance : public ValueInstance {

      friend class FileSelectorNode;
      friend class FileSelectorDialog;

  private:
    char 	*fileFilter;

  protected:
    void	setFileFilter(const char *filter);

  public:
    FileSelectorInstance(FileSelectorNode *n);
	
    ~FileSelectorInstance(); 
    boolean printAsJava (FILE* jf);

    const char *getFileFilter() { return this->fileFilter; }

    const char *getClassName() 
	{ return ClassFileSelectorInstance; }
};

#endif // _FileSelectorInstance_h

