//////////////////////////////////////////////////////////////////////////////
//				DX SOURCEFILE				    //
//////////////////////////////////////////////////////////////////////////////


/*
 * $Header: /src/master/dx/src/uipp/java/dx/client/DXClientThreadCommand.java,v 1.1.1.1 1999/03/24 15:17:30 gda Exp $
 */


package dx.client;
import dx.protocol.*;
public class DXClientThreadCommand
{
    protected DXClientThread obj;
    protected String cmdstr;
    protected int opcode;


public DXClientThreadCommand (DXClientThread dxst, String str, int code)
{
    opcode = code;
    cmdstr = str;
    obj = dxst;
}

public String getCommandString()
{
    return cmdstr;
}

public boolean execute(String inputLine)
{
    boolean retval = true;
    threadMsg msg;
    switch (opcode) {
	case DXClientThread.MESSAGE:
	    msg = new messageMsg(inputLine);
	    retval = obj.message(msg);
	    break;
	default:
	    retval = false;
	    break;
    }
    return retval;
}

} // end DXClientThreadCommand
