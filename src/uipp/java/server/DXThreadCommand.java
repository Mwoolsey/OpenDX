//
package server;

import dx.protocol.*;

public abstract class DXThreadCommand
{
    protected DXThread obj;
    protected String cmdstr;
    protected int opcode;

    public DXThreadCommand (DXThread tc, String str, int code)
    {
	opcode = code;
	cmdstr = str;
	obj = tc;
    }

    public final String getCommandString() { return cmdstr; }
    public abstract boolean execute(String inputLine);

} // end DXThreadCommand
