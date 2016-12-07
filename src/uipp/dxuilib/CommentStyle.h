/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _CommentStyle_h
#define _CommentStyle_h

#include "Base.h"

#define ClassCommentStyle "CommentStyle"

class LabelDecorator;

class CommentStyle: public Base {
  private:

  protected:

    CommentStyle(){};
    ~CommentStyle(){};

    //
    // improve the performance of parsing
    //
    static char* ParseBuffer;
    static int   ParseBufferNext;
    static int   ParseBufferSize;

    static void InitParseBuffer(int);
    static void AppendParseBuffer(const char*);

    boolean parseComment (const char*, const char*, int);
    boolean printComment (FILE*);

    virtual const char* getPrintableText()=0;
    virtual void        setPrintableText(const char*)=0;

  public:
    virtual boolean parseComment (LabelDecorator*, const char*, const char*, int)=0;
    virtual boolean printComment (LabelDecorator*, FILE*)=0;
    virtual const char* getKeyword()=0;

    const char* getClassName() { return ClassCommentStyle; }
};

#endif  // _CommentStyle_h

