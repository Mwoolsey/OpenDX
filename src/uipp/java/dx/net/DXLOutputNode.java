//


/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/DXLOutputNode.java,v 1.3 1999/07/09 19:30:00 daniel Exp $
 */

// Received $lnk:0:  0:  LINK:  DXLOutput VALUE DXLOutput_1 [2 2]


package dx.net;
import java.util.*;

public class DXLOutputNode extends dx.net.DrivenNode {

    private Hashtable msgHandlers;
    private final static int HANDLER_SLOT 	= 0;
    private final static int DATA_SLOT 		= 1;

    public DXLOutputNode(Network net, String name, int instance, String notation) {
	super(net, name, instance, notation);
	this.msgHandlers = null;
    }

    public String getMatchString() { return this.getNotation(); }

    public void installHandlers() { 
	registerHandler(this.getMatchString()); 
	registerHandler("LINK:  DXLOutput VALUE " + this.getMatchString()); 
	registerHandler(" 0:  LINK:  DXLOutput VALUE " + this.getMatchString()); 
    }

    public synchronized Object setOutputHandler(DXLinkHandler dxh, Object data)
	throws NullPointerException {

	if (this.msgHandlers == null)
	    this.msgHandlers = new Hashtable(4);

	Object[] obj = new Object[2];
	obj[DXLOutputNode.HANDLER_SLOT] = (Object)dxh;
	obj[DXLOutputNode.DATA_SLOT] = data;
	return this.msgHandlers.put (this.getNotation(), obj);
    }


    public void handleMessage (String msg) {
	if (this.msgHandlers == null) return ;
	boolean handled = false;
	try {
	    int ind = msg.indexOf("VALUE");
	    ind = msg.indexOf(this.getNotation(), ind+5);
	    ind+= this.getNotation().length() + 1;
	    String result = msg.substring(ind);

	    Object[] obj = (Object[])this.msgHandlers.get(this.getNotation());
	    DXLinkHandler dxh = (DXLinkHandler)obj[DXLOutputNode.HANDLER_SLOT];
	    dxh.outputHandler (this.getNotation(), result, obj[DXLOutputNode.DATA_SLOT]);

	    handled = true;
	} catch (Exception e) {
	    e.printStackTrace();
	}
	if (handled == false)
	    super.handleMessage(msg);
    }

    public synchronized boolean hasHandler() { return (this.msgHandlers != null); }


}; // end DXLOutputNode
