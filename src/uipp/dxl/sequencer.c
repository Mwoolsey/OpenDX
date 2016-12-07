/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "dxlP.h"
#include "dxl.h"

static DXLError
uiDXLSequencerCtl(DXLConnection *conn, DXLSequencerCtlEnum action)
{
    const char *cmd = NULL;

    switch (action) {
        case SeqPlayForward:    cmd = "sequencer play forward;"; break;
        case SeqPlayBackward:   cmd = "sequencer play backward;"; break;
        case SeqPause:          cmd = "sequencer pause;"; break;
        case SeqStep:           cmd = "sequencer step;"; break;
        case SeqStop:           cmd = "sequencer stop;"; break;
        case SeqPalindromeOn:   cmd = "sequencer palindrome on;"; break;
        case SeqPalindromeOff:  cmd = "sequencer palindrome off;"; break;
        case SeqLoopOn:         cmd = "sequencer loop on;"; break;
        case SeqLoopOff:        cmd = "sequencer loop off;"; break;
    }

    if (!cmd)
	return ERROR;
    else
	return DXLSend(conn, cmd);
}
static DXLError
exDXLSequencerCtl(DXLConnection *conn, DXLSequencerCtlEnum action)
{
    const char *cmd = NULL;

    switch (action) {
        /* FIXME: handler other than main() macro */
        case SeqPlayForward:    
				if (!DXLSend(conn,"sequence main();\n") ||
				    !DXLSend(conn,"forward;\n"))
				    return ERROR;
				cmd = "play;\n";
				break;
        case SeqPlayBackward:   
				if (!DXLSend(conn,"sequence main();\n") ||
				    !DXLSend(conn,"backward;\n")) 
				    return ERROR;
				cmd = "play;\n";
				break;
        case SeqPause:          cmd = "pause;"; break;
        case SeqStep:           cmd = "step;"; break;
        case SeqStop:           cmd = "stop;"; break;
        case SeqPalindromeOn:   cmd = "palindrome on;"; break;
        case SeqPalindromeOff:  cmd = "palindrome off;"; break;
        case SeqLoopOn:         cmd = "loop on;"; break;
        case SeqLoopOff:        cmd = "loop off;"; break;
    }

    if (!cmd)
	return ERROR;
    else
	return DXLSend(conn, cmd);
}

DXLError
DXLSequencerCtl(DXLConnection *conn, DXLSequencerCtlEnum action)
{
    if (conn->dxuiConnected)
        return uiDXLSequencerCtl(conn,action);
    else
        return exDXLSequencerCtl(conn,action);
}

