//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/DXLinkApplication.java,v 1.4 2006/03/28 20:07:52 davidt Exp $
 */
package dx.net;
/*
 *
 */

import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;
import dx.client.*;
import dx.protocol.server.*;
import dx.protocol.*;
import java.lang.reflect.*;

public abstract class DXLinkApplication extends DXApplication {

    //
    // Bean Properties
    // Bean Properties
    //
    private String netName = null;
    public void setNetName(String net) { this.netName = net; }
    public String getNetName() {
        if (this.netName == null) this.netName = getParameter("NETNAME");
        return this.netName;
    }

    private int executing;
    private Hashtable handlers;
protected dx.protocol.server.serverMsg getRequest() { return new startMsg(); }
    private int java_id;
    private String output_dir;
    private String output_url;
    private DXClientThread pending_start;
    private Vector object_applets;

    private Vector unassociated_dxloutputs;

    public void setOutputDir(String dir) { this.output_dir = dir; }
    public void setOutputUrl(String url) { this.output_url = url; }
    public String getOutputDir() { return this.output_dir; }
    public String getOutputUrl() { return this.output_url; }

    public boolean isExecuting() { return (executing > 0); }
    public int getJavaId() { return this.java_id; }


    public void setJavaId(int jid) {
        this.java_id = jid;
        super.setJavaId(jid);
    }

    public DXLinkApplication() {
        super();
        System.out.println (formal_version_string);
        this.java_id = 0;
        this.executing = 0;
        this.handlers = null;
        this.pending_start = null;
        this.unassociated_dxloutputs = null;
        this.object_applets = null;
    }


    public void disconnect(Thread t) {
        if (isExecuting())
            this.finishedExecuting();
        super.disconnect(null);
    }

    //
    // Normally we would want to kickoff command processing at this point
    // but we need to wait until the net has been loaded, so we'll start it
    // at the end of connect.
    //
    protected DXClientThread createCommandThread() {
        DXClientThread t = new DXLAppThread(this, is);
        this.pending_start = t;
        return t;
    }

    protected void connect() {
        super.connect();
        executing = 0;
        if (isConnected()) {
            loadMsg lm = new loadMsg();
            lm.setProgram(this.getNetName());
            send (lm.toString());
        }
        if (this.pending_start != null) {
            this.pending_start.start();
            this.pending_start = null;
        }
    }

    public synchronized void DXLExecuteOnce() {
        if (isConnected()) {
            SequencerNode sn = this.getSequencerNode();
            executing++;
            execOnceMsg eom;
            if (sn != null) {
                stepSeqMsg ssm = new stepSeqMsg();
                ssm.setFrame(sn.getFrame());
                ssm.setMin(sn.getMin());
                ssm.setMax(sn.getMax());
                ssm.setDelta(sn.getDelta());
                ssm.setName(sn.getName());
                ssm.setInstance(sn.getInstanceNumber());
                eom = ssm;
            } else {
                eom = new execOnceMsg();
            }

            eom.setMacroName(this.getMacroName());
            send(eom.toString());
            showStatus ("IBM Visualization Data Explorer is executing...");
        } else {
            showStatus ("...Data Explorer disconnected");
        }
        super.DXLExecuteOnce();
    }

    public synchronized void finishedExecuting() {
        executing--;
        showStatus ("Execution Finished");
        if (isExecuting())
            showStatus ("IBM Visualization Data Explorer is executing...");
        super.finishedExecuting();
    }

    public synchronized void DXLEndExecution() {
        super.DXLEndExecution();
        endExecMsg eem = new endExecMsg();
        send(eem.toString());
    }

    public synchronized void DXLSend (String msg) {
        if (isConnected()) {
            sendValueMsg svm = new sendValueMsg();
            svm.setValue(msg);
            send(svm.toString());
        }
        super.DXLSend(msg);
    }

    public synchronized void registerHandler (String matchStr, dx.net.PacketIF node) {
        if (this.handlers == null)
            this.handlers = new Hashtable(40);
        this.handlers.put(matchStr, (Object)node);
    }

    public boolean handleMessage (threadMsg msg) {
        if (this.handlers == null)
            return false;

        messageMsg mmsg = (messageMsg)msg;
        String msgTxt = mmsg.getMessage();
        int longest = 0;
        dx.net.PacketIF found_node = null;
        Enumeration enum1 = this.handlers.keys();
        while (enum1.hasMoreElements()) {
            String matchStr = (String)enum1.nextElement();
            if (msgTxt.startsWith(matchStr)) {
                int length = matchStr.length();
                if (length > longest) {
                    found_node = (dx.net.PacketIF)this.handlers.get(matchStr);
                    longest = length;
                }
            }
        }
        if (found_node != null) {
            found_node.handleMessage(msgTxt);
            return true;
        } else {
            return false;
        }
    }

    //
    // Look for DXLOutput handlers.  There is a race here.  Some applets
    // may start after the net has already executed in which case they
    // would be invisible so that doesn't really matter.  They may
    // become visible later on, and we don't really have a mechanism for
    // figuring that out.
    //
    public void start() {
        super.start();
        if (this.unassociated_dxloutputs == null) {
            try {
                Class dc = Class.forName("dx.net.DXLOutputNode");
                this.unassociated_dxloutputs = this.network.makeNodeList(dc, false);
            } catch (ClassNotFoundException cnfe) {
            } catch (Exception e) {
            }
        }
        if (this.unassociated_dxloutputs == null)
            return ;

        if (this.unassociated_dxloutputs.size() > 0) {
            Class dhClass = null;
            try {
                dhClass = Class.forName("dx.net.DXLinkHandler");
            } catch (ClassNotFoundException cnfe) {
            }
            if (dhClass == null) return ;

            Vector assd = null;
            Enumeration enum1 = this.unassociated_dxloutputs.elements();
            while (enum1.hasMoreElements()) {
                DXLOutputNode don = (DXLOutputNode)enum1.nextElement();
                if (don.hasHandler()) {
                    if (assd == null)
                        assd = new Vector(4);
                    assd.addElement(don);
                    continue;
                }

                String key = don.getNotation();

				// Here, what we should do is look through all the applets
				// to locate one that can handle the DXLOutputNode. If we
				// can't find one, then there is a chance that not all the
				// applets have been loaded yet. This is a problem, because
				// then we won't have any hookup between them. So instead
				// what we need to do is sleep for a little bit and try
				// locating them again. If we do this over and over eventually
				// the applets should get loaded and we should be able to
				// hook them up. The biggest problem is going to be if the
				// applets don't exist for the OutputNode--then we need some
				// way to timeout--or maybe better throw and exception.
				
  				boolean foundApplet = false;
  				int numTries = 0;
                
                while(! foundApplet) {
                	numTries++;
                	
                	Enumeration appls = this.getAppletContext().getApplets();
                	int numApplets = 1;
                	while (appls.hasMoreElements()) {
                    	Applet a = (Applet)appls.nextElement();
                		//System.err.println("looking at applet num: " + numApplets++ + "  name: " + a.getClass().getName());
                    	if (dhClass.isInstance(a)) {
                    	    DXLinkHandler dh = (DXLinkHandler)a;
                        	if (dh.hasHandler(key)) {
                            	don.setOutputHandler (dh, null);
                            	if (assd == null)
                                	assd = new Vector(4);
                            	assd.addElement(don);
                            	foundApplet = true;
                            	break;
                        	}
                    	}
                	}
                	if(numTries > 120)
                		throw new NullPointerException("Unable to locate DXLOutputHandler for: " + key);
                	else
                		try {
                			Thread.sleep(500); // wait 1/2 seconds.
                		} catch (Exception e) {}
                }
                
                
                
            }

            if (assd != null) {
                enum1 = assd.elements();
                while (enum1.hasMoreElements()) {
                    DXLOutputNode don = (DXLOutputNode)enum1.nextElement();
                    this.unassociated_dxloutputs.removeElement(don);
                }
            }
        }
    }


    public void showObject (String name, String path, int id) {
        if ((this.object_applets == null) || (this.object_applets.size() < 1))
            this.locateObjectWindows();
        if (this.object_applets == null) return ;
        if (this.object_applets.size() < 1) return ;

        Enumeration enum1 = this.object_applets.elements();
        DXViewIF destination = null;
        while (enum1.hasMoreElements()) {
            DXViewIF dvi = (DXViewIF)enum1.nextElement();

            String nname = dvi.getNodeName();
            if (nname == null) continue;
            if (nname.equals(name)) {
                destination = dvi;
                break;
            }
        }
        if ((destination == null) && (id < this.object_applets.size()))
            destination = (DXViewIF)this.object_applets.elementAt(id);
        if (destination == null)
            destination = (DXViewIF)this.object_applets.elementAt(0);

        String urlstr = null;
        try {
            URL url = null;
            url = new URL(getCodeBase(), path);
            urlstr = url.toString();
        } catch (MalformedURLException mue) {
            System.out.println ("ActiveXApp: cannot create url from " + path);
        }
        if (urlstr != null)
            destination.setUrl ("!tear " + urlstr);
    }

    private void locateObjectWindows() {
        AppletContext apcxt = this.getAppletContext();
        Class suits_if = null;
        try {
            suits_if = Class.forName("dx.net.DXViewIF");
        } catch (ClassNotFoundException cnfe) {
            cnfe.printStackTrace();
            return ;
        }
        if (this.object_applets == null)
            this.object_applets = new Vector(4);

        Enumeration enum1 = apcxt.getApplets();
        while (enum1.hasMoreElements()) {
            Applet a = (Applet)enum1.nextElement();
            Class c = a.getClass();
            Class[] faces = c.getInterfaces();
            int len = faces.length;
            for (int i=0; i<len; i++) {
                if (faces[i].equals(suits_if))
                    this.object_applets.addElement(a);
            }
        }
    }

} // end DXLinkApplication

