//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/DXLAppThread.java,v 1.3 2005/10/27 19:43:06 davidt Exp $
 */
package dx.net;
import java.io.*;
import java.net.*;
import java.lang.*;
import java.util.*;
import java.applet.*;
import dx.client.*;
import dx.protocol.*;

public class DXLAppThread extends DXClientThread {
    public final static int FINISHED 	= DXClientThread.LASTMSG + 1;
    public final static int SETMACRO 	= DXClientThread.LASTMSG + 2;
    public final static int SETID 	= DXClientThread.LASTMSG + 3;


public DXLAppThread(DXLinkApplication apple, BufferedReader is)
{
    super(apple, is);
    try {
	actions.addElement((Object)new 
	    DXLAppThreadCommand(this, doneMsg.GetCommand(), DXLAppThread.FINISHED));
	actions.addElement((Object)new 
	    DXLAppThreadCommand(this, setidMsg.GetCommand(), DXLAppThread.SETID));
	actions.addElement((Object)new 
	    DXLAppThreadCommand(this, setmacroMsg.GetCommand(), DXLAppThread.SETMACRO));
    } catch (ClassNotFoundException cnfe) {
	cnfe.printStackTrace();
    }
}

public boolean doneExecuting(threadMsg msg)
{
    DXLinkApplication dlt = (DXLinkApplication)parent;
    dlt.finishedExecuting();
    return true;
}

public boolean message (threadMsg msg)
{
    DXLinkApplication dlt = (DXLinkApplication)parent;
    boolean status = dlt.handleMessage(msg);
    if (status) return status;
    return super.message(msg);
}

//
// Expect:
//	SetId:%d
//
public boolean setId(threadMsg msg)
{
    int java_id = 0;
    setidMsg sim = (setidMsg)msg;
    DXLinkApplication dlt = (DXLinkApplication)parent;
    dlt.setOutputUrl(sim.getOutputUrl());
    dlt.setOutputDir(sim.getOutputDir());
    dlt.setJavaId(sim.getJavaId());
    return true;
}

public boolean setMacro(threadMsg msg)
{
    setmacroMsg sim = (setmacroMsg)msg;
    DXLinkApplication dlt = (DXLinkApplication)parent;
    dlt.setMacroName (sim.getMacroName());
    return true;
}

}; // end DXLAppThread


