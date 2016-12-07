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
//
//

#ifndef _Separator_Decorator_h
#define _Separator_Decorator_h

#include <X11/Intrinsic.h>

#include "Decorator.h"

#define ClassSeparatorDecorator	"SeparatorDecorator"
class SetSeparatorAttrDlg;

class SeparatorDecorator : public Decorator
{

  // P R I V A T E   P R I V A T E   P R I V A T E
  // P R I V A T E   P R I V A T E   P R I V A T E
  private:

    static boolean SeparatorDecoratorClassInitialized;
    SetSeparatorAttrDlg *setAttrDialog;

    // D R A G - N - D R O P
    // D R A G - N - D R O P
    static Dictionary *DragTypeDictionary;
    static Widget      DragIcon;
 
  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  protected:
    static  String 	   DefaultResources[]; 

    virtual void 	   completeDecorativePart();

  // P U B L I C   P U B L I C   P U B L I C
  // P U B L I C   P U B L I C   P U B L I C
  public:
    virtual   void           openDefaultWindow() ;
    virtual   boolean	     hasDefaultWindow() { return TRUE; }
    static    Decorator*     AllocateDecorator (boolean devStyle);
    virtual   void	     setAppearance (boolean developerStyle);
    virtual   void           uncreateDecorator();


    // G E O M E T R Y   A N D   R E S O U R C E S
    // G E O M E T R Y   A N D   R E S O U R C E S
    virtual   void	     resizeCB();
	      void	     setVerticalLayout (boolean vertical = TRUE);

    // C O N T R O L   P A N E L   C O M M E N T   F U N C T I O N S
    // C O N T R O L   P A N E L   C O M M E N T   F U N C T I O N S
    virtual   boolean	     printComment (FILE *f);
    virtual   boolean	     parseComment (const char *comment, 
					      const char *filename, int line);

    // D R A G - N - D R O P
    // D R A G - N - D R O P
    virtual Dictionary* getDragDictionary() 
	{ return SeparatorDecorator::DragTypeDictionary; }
    virtual boolean     decodeDragType (int, char *, XtPointer *, unsigned long *, long);

    // J A V A     J A V A     J A V A     J A V A     J A V A     
    // J A V A     J A V A     J A V A     J A V A     J A V A     
    virtual boolean printAsJava (FILE* , const char*, int);
    virtual boolean printJavaResources (FILE* , const char*, const char*);
 

    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    virtual   void initialize();
    	  	   SeparatorDecorator(boolean developerStyle = TRUE);
    	  	  ~SeparatorDecorator(); 
    const    char* getClassName() { return ClassSeparatorDecorator; }
    virtual  boolean isA(Symbol classname);
};


#endif // _Separator_Decorator_h
