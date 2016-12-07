/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _FileContents_h_
#define _FileContents_h_

#include "DXStrings.h"
#include <stdio.h>
#include <sys/stat.h>
#include <Xm/Xm.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

class List;
class QuitCommand;

extern "C" void FileContents_UnlinkTO (XtPointer , XtIntervalId*);
extern "C" void FileContents_CleanUp ();

class FileContents {
    private:

      char* in_file_name;
      char* out_file_name;
      char* sans_extension;

      FILE* out_fp;

      char* contents;

      char* processReplacements(List*);

      friend class QuitCommand;  // so that QuitCommand can make a call to CleanUp().
      static List* QueuedForDeletion;
      static void  CleanUp();
      static XtIntervalId CleanUpTimer;

      friend void FileContents_UnlinkTO (XtPointer , XtIntervalId*);
      friend void FileContents_CleanUp ();

    public:

      FileContents (const char* in_file) {
	this->in_file_name = DuplicateString(in_file);
	this->out_file_name = NUL(char*);
	this->out_fp = NUL(FILE*);
	this->contents = NUL(char*);
	this->sans_extension = NUL(char*);
      }

      ~FileContents();

      boolean initialize();
      boolean initialize(const char*);
      void close();
      void replace (const char* pattern, const char* replacement);

      //
      // When the app is done, we should go back and delete files.
      //
      static void Cleanup(boolean drop_everything);

      const char* getFileName() { return this->out_file_name; }
      const char* sansExtension();
};

//
// Each object is really just a set of pointers into a larger text string.
// I go scanning through the text .net,.cfg files looking for patterns to
// replace.  As I find a pattern I make on of these objects.  Then after
// I've created all the objects I need, I loop over a list of these
// Changling objects and perform the actual replacing.
//
class Changling {
  private:
    friend class FileContents;

    //
    // *start is the first char to copy preceeding the match.
    //
    const char* start;

    //
    // *begin is the first char NOT to copy.  It's also the first char of the match.
    //
    const char* begin;

    //
    // The string to substitute.
    //
    const char* replacement;

    //
    // The difference is size between the match and its replacement so we can
    // do the required realloc.
    //
    int size_diff;

    Changling (const char* start) {
	this->start = start;
	this->begin = NUL(char*);
	this->size_diff = 0;
	this->replacement = NUL(char*);
    }
    Changling (const char* start, const char* loc, 
	    const char* pattern, const char* replacement) {
	this->start = start;
	this->begin = loc;
	this->size_diff = strlen(replacement) - strlen(pattern);
	this->replacement = replacement;
    }
  public:
    ~Changling(){};
};

#endif //_FileContents_h_

