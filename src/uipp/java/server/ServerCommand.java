//
package server;
import java.net.*;
import dx.protocol.server.*;

public class ServerCommand
{
    private String cmdstr;
    private int opcode;

public ServerCommand (String cmdstr, int opcode)
{
    this.opcode = opcode;
    this.cmdstr = cmdstr;
}

public String getCommandString()
{
    return cmdstr;
}

public boolean execute(String inputLine, Socket csock)
{
    boolean retval = true;
    dx.protocol.server.serverMsg msg;
    switch (opcode) {
	case DXServer.STARTDX:
	    msg = new startMsg(inputLine);
	    retval = DXServer.startDX(msg, csock);
	    break;
	case DXServer.STATUS:
	    msg = new statusMsg(inputLine);
	    retval = DXServer.status(msg, csock);
	    break;
	case DXServer.SHUTDOWN:
	    msg = new shutdownMsg(inputLine);
	    retval = DXServer.shutdown(msg, csock);
	    break;
    }
    return retval;
}

} // end ServerCommand
