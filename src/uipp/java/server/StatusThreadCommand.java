//
package server;
import dx.protocol.*;

public final class StatusThreadCommand extends DXThreadCommand
{

    public StatusThreadCommand (StatusThread dxst, String str, int code)
    {
	super(dxst, str, code);
    }


    public boolean execute(String inputLine)
    {
	boolean retval = true;
	threadMsg msg;
	StatusThread st = (StatusThread)obj;
	switch (opcode) {
	    case StatusThread.USERS:
		msg = new usersMsg(inputLine);
		retval = st.users(msg);
		break;
	    case StatusThread.VISITS:
		msg = new visitsMsg(inputLine);
		retval = st.visits(msg);
		break;
	    case StatusThread.UPTIME:
		msg = new uptimeMsg(inputLine);
		retval = st.uptime(msg);
		break;
	    case StatusThread.DISCONNECT:
		msg = new stopMsg(inputLine);
		retval = st.disconnect(msg);
		break;
	}
	return retval;
    }

} // end StatusThreadCommand
