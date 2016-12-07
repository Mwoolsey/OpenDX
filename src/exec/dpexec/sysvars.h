/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/sysvars.h,v 1.5 2004/06/09 16:14:29 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	__SYSVARS_H
#define	__SYSVARS_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define	VCR_ID_START		"@startframe"
#define	VCR_ID_END		"@endframe"
#define	VCR_ID_DELTA		"@deltaframe"
#define	VCR_ID_NEXT		"@nextframe"
#define	VCR_ID_FRAME		"@frame"

#define PROMPT_ID_PROMPT	"@prompt"
#define PROMPT_ID_CPROMPT	"@cprompt"

#ifndef EX_PROMPT
#define EX_PROMPT	"dx> " 
#endif
#ifndef EX_CPROMPT
#define EX_CPROMPT	"  > "
#endif

char *_dxf_ExPromptGet(char *var);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif	/* __SYSVARS_H */
