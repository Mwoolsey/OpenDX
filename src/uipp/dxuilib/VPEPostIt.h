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

#ifndef _VPEPostIt_h
#define _VPEPostIt_h

#include <X11/Intrinsic.h>

#include "VPEAnnotator.h"

class Dictionary;

#define ClassVPEPostIt	"VPEPostIt"

class VPEPostIt : public VPEAnnotator
{

  // P R I V A T E   P R I V A T E   P R I V A T E
  // P R I V A T E   P R I V A T E   P R I V A T E
  private:
    static boolean VPEPostItClassInitialized;
    Pixmap         bg_pixmap;
    void	   makePixmap();

  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  protected:
    static  String 	   DefaultResources[]; 
    virtual void           completeDecorativePart();

    virtual boolean requiresLineReroutingOnResize() { return FALSE; }

  // P U B L I C   P U B L I C   P U B L I C
  // P U B L I C   P U B L I C   P U B L I C
  public:
    static    Decorator*     AllocateDecorator (boolean devStyle);

    virtual   void	     setLabel(const char *newStr, boolean);
    virtual   void           setFont(const char *);
    virtual   boolean        printPostScriptPage(FILE *f);
    virtual   void           setResource (const char *, const char *);

    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    virtual  void  initialize();
    	  	   VPEPostIt(boolean developerStyle =TRUE);
    	  	  ~VPEPostIt(); 
    const    char* getClassName() { return ClassVPEPostIt; }
    virtual  boolean isA(Symbol classname);
};


#endif // _VPEPostIt_h
