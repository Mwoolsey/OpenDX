/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _JavaNet_h
#define _JavaNet_h

#include "Network.h"

//
// Class name definition:
//
#define ClassJavaNet	"JavaNet"

class Node;
class ImageNode;
class MacroNode;
class Command;

//
// Network class definition:
//				
class JavaNet : public Network
{

  private:

    friend class DXApplication; // For the constructor

    const char*	getHtmlHeader();


  protected:

    //
    // Called only by DXApplication
    //
    JavaNet();

    static String UnsupportedTools[];

    static List*  MakeUnsupportedToolList(JavaNet*);

    char*	base_name;

    char*	html_file;
    char*	make_file;
    char*	applet_file;
    char*	bean_file;

    FILE*	html_f;
    FILE*	make_f;
    FILE*	applet_f;
    FILE*	bean_f;

    boolean	setOutputName (const char*);
    boolean	netToApplet();

    Command*	saveWebPageCmd;
    Command*	saveAppletCmd;
    Command*	saveBeanCmd;

    //
    // In addition to the work done in Network, we need to print references to
    // special macros used in exporting gifs and wrls
    //
    virtual boolean printMacroReferences(FILE *f, boolean inline_define,
	    PacketIFCallback echoCallback, void *echoClientData);

    boolean requires(const char* format);


  public:
    
    //
    // Destructor:
    //
    ~JavaNet();

    virtual boolean saveWebPage();
    virtual boolean saveApplet();
    virtual boolean saveBean();

    virtual Command* getSaveWebPageCommand() { return this->saveWebPageCmd; }
    virtual Command* getSaveAppletCommand() { return this->saveAppletCmd; }
    virtual Command* getSaveBeanCommand();

    virtual boolean isJavified();

    virtual boolean saveNetwork(const char *name, boolean force = FALSE);

    virtual void changeExistanceWork(Node *n, boolean adding);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassJavaNet; }
};


#endif // _JavaNet_h

