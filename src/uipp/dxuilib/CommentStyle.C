/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "CommentStyle.h"
#include "DXStrings.h"
#include "WarningDialogManager.h"

//
// Buffer text during parsing rather than repeatedly doing a strcat, setLabel
// for the save of efficiency.
//
char* CommentStyle::ParseBuffer = NUL(char*);
int   CommentStyle::ParseBufferNext = 0;
int   CommentStyle::ParseBufferSize = 0;


void CommentStyle::InitParseBuffer(int estimated_size)
{
int required_size = estimated_size + 64;

    //
    // Get a new buffer if the old one is too small.
    //
    if (CommentStyle::ParseBufferSize < required_size) {
	if (CommentStyle::ParseBuffer) delete CommentStyle::ParseBuffer;
	CommentStyle::ParseBuffer = new char[required_size];
    }
    CommentStyle::ParseBuffer[ CommentStyle::ParseBufferNext = 0 ] = '\0';
}

void CommentStyle::AppendParseBuffer(const char* text)
{
    ASSERT(text);
    int len = strlen(text);

    //
    // If out of space realloc the buffer.  This should never be the case unless
    // code is added down the road for escaping special chars which could increase
    // the required size to more then strlen says.
    //
    int required_size = len + CommentStyle::ParseBufferNext;
    if (required_size >= CommentStyle::ParseBufferSize) {
	char* cp = new char[required_size + 128];
	CommentStyle::ParseBufferSize = required_size = 128;
	strcpy (cp, CommentStyle::ParseBuffer);
	delete CommentStyle::ParseBuffer;
	CommentStyle::ParseBuffer = cp;
    }

    if (CommentStyle::ParseBufferNext) {
	CommentStyle::ParseBuffer[CommentStyle::ParseBufferNext++] = '\n';
	CommentStyle::ParseBuffer[CommentStyle::ParseBufferNext] = '\0';
    }

    char* cp = &CommentStyle::ParseBuffer[CommentStyle::ParseBufferNext];
    strcpy (cp, text);
    CommentStyle::ParseBufferNext+= len;
}


#define NO_TEXT_SYMBOL "<NULL>"
//
// format:
//	// annotation %s_begin: NO_TEXT_SYMBOL\n
//	// annotation %s: %s\n
//	// annotation %s_end: NO_TEXT_SYMBOL\n
//
boolean CommentStyle::printComment (FILE* f)
{
char begin_stmnt[64];
char end_stmnt[64];

    sprintf (begin_stmnt, "%s_begin", this->getKeyword());
    sprintf (end_stmnt, "%s_end", this->getKeyword());

    const char* cp = this->getPrintableText();

    if ((cp) && (cp[0])) {

	if (fprintf (f, "    // annotation %s: %ld\n", begin_stmnt, strlen(cp)) < 0)
	    return FALSE;

	char* line_to_print = DuplicateString(cp);
	//
	// The constant PBS imposes a maximum line length.
	//
#	define PBS 256
	char print_buf[PBS];
	int line_len_exceeded = 0;
	int i = 0, next = 0;
	while (1) {
	    print_buf[next] = line_to_print[i];
	    if (next == (PBS-2)) {
		print_buf[++next] = '\n';
		line_len_exceeded++;
	    }
	    if (print_buf[next] == '\n') {
		print_buf[next] = '\0';
		int j;
		boolean is_line_whitespace = TRUE;
		for (j=0; j<next; j++) {
		    if ((print_buf[j] != ' ') &&
			(print_buf[j] != '\t')) {
			is_line_whitespace = FALSE;
			break;
		    }
		}
		if (!is_line_whitespace) {
		    if (fprintf (f, 
			"    // annotation %s: %s\n", this->getKeyword(), print_buf) < 0)
			return FALSE;
		} else {
		    if (fprintf (f, "    // annotation %s: %s\n", 
			this->getKeyword(), NO_TEXT_SYMBOL) < 0)
			return FALSE;
		}
		next = 0;
	    } else if (print_buf[next] == '\0') {
		int j;
		boolean is_line_whitespace = TRUE;
		for (j=0; j<next; j++) {
		    if ((print_buf[j] != ' ') &&
			(print_buf[j] != '\t')) {
			is_line_whitespace = FALSE;
			break;
		    }
		}
		if (!is_line_whitespace) {
		    if (fprintf (f, "    // annotation %s: %s\n", 
			this->getKeyword(), print_buf) < 0)
			return FALSE;
		} else {
		    if (fprintf (f, "    // annotation %s: %s\n", 
			this->getKeyword(), NO_TEXT_SYMBOL) < 0)
			return FALSE;
		}
		break;
	    } else {
		next++;
	    }
	    i++;
	}
	if (line_len_exceeded) {
	    WarningMessage ("%d lines of program annotation"
		"exceeded maximum length of %d", line_len_exceeded, PBS);
	}
	delete line_to_print;

	if (fprintf (f, "    // annotation %s: %s\n", end_stmnt, NO_TEXT_SYMBOL) < 0)
	    return FALSE;
    }
    return TRUE;
}

//
//
boolean CommentStyle::parseComment (const char *comment, const char *file, int l)
{
int items_parsed;
boolean retVal = FALSE;

char begin_stmnt[64];
char end_stmnt[64];

    sprintf (begin_stmnt, "%s_begin", this->getKeyword());
    sprintf (end_stmnt, "%s_end", this->getKeyword());


    //
    // As of 3.1.4, we want to print the text of the decorator on multiple lines
    // using statefullness already implemented in ControlPanel and now in Network
    // as well, with the addition of vpe pages. 
    //
    if (EqualSubstring(" annotation",comment,11)) {
	char additional_text[256];
	additional_text[0] = '\0';
	char keyword[64];
	items_parsed = sscanf (comment, " annotation %[^:]: %[^\n]",
	    keyword, additional_text);
	if (items_parsed == 2) {
	    //
	    // Count leading spaces.
	    //
	    const char* cp = strchr(comment, ':');
	    int leading_space = 0;
	    cp+=2;
	    char c = *cp;
	    while ((c != '\n') && ((c == ' ')||(c == '\t'))) 
		c = *(++cp), additional_text[leading_space++] = ' ';
	    ASSERT(leading_space < 255);
	    items_parsed = sscanf (comment, " annotation %[^:]: %[^\n]",
		keyword, &additional_text[leading_space]);
	    ASSERT(items_parsed == 2);
	}

	if (items_parsed == 2) {
	    if (EqualString(keyword, this->getKeyword())) {
		if (EqualString(additional_text, NO_TEXT_SYMBOL)) {
		    //additional_text[0]='\t';
		    additional_text[0]='\0';
		} else {
		}
		CommentStyle::AppendParseBuffer(additional_text);
		retVal = TRUE;
	    } else if (EqualString(keyword, begin_stmnt)) {
		int str_size = 128;
		sscanf (additional_text, "%d", &str_size);
		CommentStyle::InitParseBuffer(str_size);
		retVal = TRUE;
	    } else if (EqualString(keyword, end_stmnt)) {
		this->setPrintableText(CommentStyle::ParseBuffer); 
		retVal = TRUE;
	    } else {
		//
		// This comment was intended for a different CommentStyle object
		//
	    }
	} else {
	    WarningMessage ("Unrecognized text in LabelDecorator "
			    "Comment (file %s, line %d)", file, l);
	}
    }
    return retVal;
}

