/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _NET_FILE_DIALOG_H
#define _NET_FILE_DIALOG_H

#include "FileDialog.h"

#define ClassNetFileDialog "NetFileDialog"

class StartupWindow;

class NetFileDialog: public FileDialog 
{
  private:
    StartupWindow* suw;

  protected:

    static String DefaultResources[];
    static boolean ClassInitialized;

    virtual void okFileWork(const char *string);
    virtual void helpCallback (Dialog*);

    virtual void installDefaultResources (Widget );

    virtual Widget createDialog(Widget parent);

  public:

    NetFileDialog (Widget parent, StartupWindow *suw);
    ~NetFileDialog();

    void showNewOption(boolean=TRUE);

    const char *getClassName() { return ClassNetFileDialog; }
};

#endif //_NET_FILE_DIALOG_H
