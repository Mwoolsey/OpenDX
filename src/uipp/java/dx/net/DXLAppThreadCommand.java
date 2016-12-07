//////////////////////////////////////////////////////////////////////////////
//                         DX SOURCEFILE				    //
//////////////////////////////////////////////////////////////////////////////


/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/DXLAppThreadCommand.java,v 1.2 1999/07/09 19:30:00 daniel Exp $
 */

package dx.net;
import dx.client.*;
import dx.protocol.*;

public class DXLAppThreadCommand extends DXClientThreadCommand 
{


public DXLAppThreadCommand
    (DXLAppThread dxst, String str, int code)
{
    super (dxst, str, code);
}


public boolean execute(String inputLine)
{
    boolean retval = true;
    DXLAppThread dlt = (DXLAppThread)obj;
    threadMsg msg;
    switch (opcode) {
	case DXLAppThread.FINISHED:
	    msg = new doneMsg(inputLine);
	    retval = dlt.doneExecuting(msg);
	    break;
	case DXLAppThread.SETID:
	    msg = new setidMsg(inputLine);
	    retval = dlt.setId(msg);
	    break;
	case DXLAppThread.SETMACRO:
	    msg = new setmacroMsg(inputLine);
	    retval = dlt.setMacro(msg);
	    break;
	default:
	    retval = super.execute(inputLine);
	    break;
    }
    return retval;
}

} // end DXLAppThreadCommand
