//
package server;
import dx.protocol.*;

public final class ServerThreadCommand extends DXThreadCommand
{

    public ServerThreadCommand (DXServerThread dxst, String str, int code)
    {
	super(dxst, str, code);
    }


    public boolean execute(threadMsg msg)
    {
	boolean retval = true;
	DXServerThread dxst = (DXServerThread)obj;
	switch (opcode) {
	    case DXServerThread.EXECUTE:
		retval = dxst.executeOnce(msg);
		break;
	    case DXServerThread.SEQUENCE:
		retval = dxst.stepSequencer(msg);
		break;
	    case DXServerThread.LOAD:
		retval = dxst.loadProgram(msg);
		break;
	    case DXServerThread.SENDVALUE:
		retval = dxst.sendValue (msg);
		break;
	    case DXServerThread.ENDEXECUTION:
	    case DXServerThread.NOOP:
		retval = dxst.checkConnection (msg);
		break;
	    default:
		retval = false;
		break;
	}
	return retval;
    }

    public boolean execute(String inputLine)
    {
	threadMsg msg;
	DXServerThread dxst = (DXServerThread)obj;
	switch (opcode) {
	    case DXServerThread.EXECUTE:
		msg = new execOnceMsg(inputLine);
		break;
	    case DXServerThread.SEQUENCE:
		msg = new stepSeqMsg(inputLine);
		break;
	    case DXServerThread.LOAD:
		msg = new loadMsg(inputLine);
		break;
	    case DXServerThread.SENDVALUE:
		msg = new sendValueMsg(inputLine);
		break;
	    case DXServerThread.ENDEXECUTION:
	    case DXServerThread.NOOP:
		msg = new noopMsg(inputLine);
		break;
	    default:
		msg = null;
		break;
	}
	return execute(msg);
    }

} // end ServerThreadCommand
