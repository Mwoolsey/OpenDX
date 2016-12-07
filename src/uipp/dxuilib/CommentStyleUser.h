/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _CommentStyleUser_h
#define _CommentStyleUser_h

#include "CommentStyle.h"

#define ClassCommentStyleUser "CommentStyleUser"

class CommentStyleUser: public CommentStyle {
  private:

    static LabelDecorator* ParsingDecorator;

  protected:

    virtual const char* getPrintableText();
    virtual void        setPrintableText(const char*);

  public:

    CommentStyleUser(){};
    ~CommentStyleUser(){};

    virtual boolean parseComment 
	(LabelDecorator* ldec, const char* comment, const char* file, int l) {
	CommentStyleUser::ParsingDecorator = ldec;
	return this->CommentStyle::parseComment(comment, file, l);
    }

    virtual boolean printComment (LabelDecorator* ldec, FILE* f) {
	CommentStyleUser::ParsingDecorator = ldec;
	return this->CommentStyle::printComment(f);
    }

    virtual const char* getKeyword() { return "user"; }

    const char* getClassName() { return ClassCommentStyleUser; }
};

#endif  // _CommentStyleUser_h

