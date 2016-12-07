/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "FileContents.h"
#include "IBMApplication.h"
#include "ListIterator.h"
#include "DXStrings.h"

List* FileContents::QueuedForDeletion = NUL(List*);
XtIntervalId FileContents::CleanUpTimer = 0;

//
// Don't fill the file system up with junk nets.  
// Problem: each time you press the visualize button, you
// have to copy a net from $DXROOT/ui.  So when can I delete that file?  The
// code is already running synced with dxlink, which makes me think it should
// be safe to unlink the file in this destructor, however dxui sometimes can't
// find the .cfg file because it's gone already.  So, I'll use a timer which
// is real hack, but it tends to work.  If the user quits before the timer
// goes off, then the QuitCommand will call us to cleanup one last time.
//
FileContents::~FileContents()
{
    if (this->contents) delete this->contents;
    if (this->out_fp) fclose(this->out_fp);
    if (this->in_file_name) delete this->in_file_name;

    if (this->sans_extension) {
	if (FileContents::QueuedForDeletion == NUL(List*))
	    FileContents::QueuedForDeletion = new List;
	FileContents::QueuedForDeletion->appendElement((void*)this->sans_extension);
    }

    if (this->out_file_name) {
	if (FileContents::QueuedForDeletion == NUL(List*))
	    FileContents::QueuedForDeletion = new List;

	// I want to unlink the file here, but a dxlink-ed dxui is going to
	// read the file asnychronously, so if I do delete it, then dxui won't
	// be able to read it.  I'll set a timer which should go off after
	// dxui has had its shot at the file. 
	XtAppContext apcxt = theApplication->getApplicationContext();

	FileContents::QueuedForDeletion->appendElement((void*)this->out_file_name);
	if (FileContents::CleanUpTimer)
	    XtRemoveTimeOut(FileContents::CleanUpTimer);
	FileContents::CleanUpTimer = 
	    XtAppAddTimeOut (apcxt, 10000, (XtTimerCallbackProc)
		FileContents_UnlinkTO, (XtPointer)0);
    }
}

extern "C" void FileContents_CleanUp()
{
    FileContents::CleanUp();
}

extern "C" void FileContents_UnlinkTO (XtPointer , XtIntervalId*)
{
    FileContents::CleanUpTimer = 0; 
    FileContents::CleanUp();
}

void FileContents::CleanUp()
{
    if (FileContents::CleanUpTimer) {
	XtRemoveTimeOut(FileContents::CleanUpTimer);
	FileContents::CleanUpTimer = 0;
    }
    if (!FileContents::QueuedForDeletion) return ;
    ListIterator it(*FileContents::QueuedForDeletion);
    char *out_file_name;
    while (out_file_name = (char*)it.getNext()) {
	if (out_file_name) {
	    if (out_file_name[0]) unlink (out_file_name);
	    delete out_file_name;
	}
    }
    FileContents::QueuedForDeletion->clear();
}

//
// We need 2 methods of initialization because when you start working on the
// second file (the .cfg file), it's name must match the first file's except
// for the extension.  Another c++ class is called for - something like
// NetCfgFileContents - in order to enforce this, but my fingers are tired 
// of typing.
//
boolean FileContents::initialize (const char* base_name) 
{
    ASSERT(this->sans_extension == NUL(char*));
    this->sans_extension = DuplicateString(base_name);
    return this->initialize();
}

const char* FileContents::sansExtension()
{
    ASSERT (this->out_file_name);
    int name_len = strlen(this->out_file_name);
    if (this->sans_extension == NUL(char*)) {
	this->sans_extension = DuplicateString(this->out_file_name);
	this->sans_extension[name_len-4] = '\0';
    }
    return this->sans_extension;
}

boolean FileContents::initialize ()
{
    //
    // In order to build output filename, find the base name then
    // duplicate it onto the end of our tmp directory name.
    //
    int name_len = strlen(this->in_file_name);
    char* ext = &this->in_file_name[name_len-4];
    if (this->sans_extension == NUL(char*)) {
	const char* tmpdir = theIBMApplication->getTmpDirectory();
	char junk_file[512];
	sprintf (junk_file, "%s/foo.bar", tmpdir);
	char* base_name = GetFileBaseName (this->in_file_name, ext);
	this->sans_extension = UniqueFilename(junk_file);
	if (!this->sans_extension) return FALSE;
	this->out_file_name = new char[strlen(this->sans_extension) + 32];
	sprintf (this->out_file_name, "%s%s", this->sans_extension,ext);
	FILE* junk = fopen(this->sans_extension, "w");
	if (junk) fclose(junk);
    } else {
	this->out_file_name = new char[strlen(this->sans_extension) + 32];
	sprintf (this->out_file_name, "%s%s", this->sans_extension,ext);
    }
    
    //
    // Read the contents of the file.
    //
    FILE* in_fp = fopen(this->in_file_name, "r");
    ASSERT (in_fp);
    /*int filedes = fileno (in_fp);*/

    struct STATSTRUCT statbuf;
    STATFUNC(this->in_file_name, &statbuf);
    this->contents = new char[statbuf.st_size + 1];

    int bread = fread (this->contents, sizeof(char), (int)statbuf.st_size, in_fp);
    ASSERT (bread == statbuf.st_size);
    fclose (in_fp);
    this->contents[bread] = '\0';

    //
    // Open the output file.  This should truncate the file if it exists already.
    // Because of the way we use files, it's likely to exist already.
    //
    this->out_fp = fopen (this->out_file_name, "w");
    if (!this->out_fp) return FALSE;

    return TRUE;
}

void FileContents::close()
{
    if (!this->out_fp) return ;

    int len = strlen(this->contents);
    int brite = fwrite (this->contents, sizeof(char), len, this->out_fp);
    //
    // FIXME: what do you do with an error?
    //
    //ASSERT (brite == len);

    fclose (this->out_fp);
    this->out_fp = NUL(FILE*);
}

//
// It should be easy to implement this using regcmp/regex, but these functions
// don't exist on the pc platforms, and as far as I know, there isn't any
// decent substitute.  (They're also missing on sun4 and alphax, but those 2
// unix machines do have re_comp instead.)  See ui++/dxui/MacroDefinition.C
// for an example of some regular expression code.
//
void FileContents::replace (const char* pattern, const char* replacement)
{
    if (!this->out_fp) return ;
    if (!this->contents) return ;

    Changling* cling;
    List changlings;
    int pattern_len = strlen(pattern);
    ASSERT(pattern_len);

    //
    // Make a list of all the occurrences of pattern in this->contents.
    // We'll go back later and scan the resulting list and perform the replacing.
    //
    char* ptr = this->contents;
    char* cp;
    do {
	if (cp = strstr (ptr, pattern))
	    cling = new Changling(ptr, cp, pattern, replacement);
	else
	    cling = new Changling(ptr);
	changlings.appendElement((void*)cling);
	ptr = cp + pattern_len;
    } while (cp);

    char* tmp = this->contents;
    this->contents = this->processReplacements (&changlings);
    delete tmp;

    ListIterator it(changlings);
    while (cling = (Changling*)it.getNext())
	delete cling;
}


//
// clings is a list of objects.  Each obj identifies the first char  in src
// of a match and the first char in src after the end of the match.  Replace
// the intervening chars with the replacement pattern.
//
char* FileContents::processReplacements (List* clings)
{
    ASSERT ((clings) && (clings->getSize()));

    //
    // compute the size of the output.
    //
    int size_diff = 0;
    Changling* cling;
    ListIterator it(*clings);
    while (cling = (Changling*)it.getNext()) size_diff+= cling->size_diff;

    char* new_contents = new char[strlen(this->contents)+1 + size_diff];
    int next = 0;

    it.setList(*clings);
    while (cling = (Changling*)it.getNext()) {
	const char* cp;
	for (cp = cling->start; ((*cp) && (cp!=cling->begin)); cp++) 
	    new_contents[next++] = *cp;
	if (cling->replacement) {
	    strcpy (&new_contents[next], cling->replacement);
	    next+= strlen(cling->replacement);
	}
	new_contents[next] = '\0';
    }
    return new_contents;
}
