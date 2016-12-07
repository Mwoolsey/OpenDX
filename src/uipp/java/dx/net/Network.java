//

package dx.net;
import java.util.*;
import java.applet.*;

public class Network extends Vector {

    private Vector panelList;
    private DXApplication dxapp;

    public DXApplication getApplet() { return dxapp; }
    public void setApplet(DXApplication a) { this.dxapp = a; }

    public Network() {
	super();
	this.panelList = new Vector(4);
	this.dxapp = null;
    }

    public int getPanelCount() { 
	synchronized (this.panelList) {
	    if (this.panelList == null) return 0;
	    return this.panelList.size();
	}
    }

    public ControlPanel getPanel(int i) throws ArrayIndexOutOfBoundsException {
	synchronized (this.panelList) {
	    return (ControlPanel)this.panelList.elementAt(i);
	}
    }

    public synchronized Vector makeNodeList(Class nodeClass, boolean incsub) {
	if (this.size() == 0) return null;
	Vector retval = null;
	Enumeration enum1 = this.elements();
	while (enum1.hasMoreElements()) {
	    Node n = (Node)enum1.nextElement();
	    Class nc = n.getClass();
	    while (nc != null) {
		if (nc.equals(nodeClass)) {
		    if (retval == null) retval = new Vector(10);
		    retval.addElement((Object)n);
		    break;
		}
		if (incsub) 
		    nc = nc.getSuperclass();
		else
		    nc = null;
	    }
	}
	return retval;
    }

    public synchronized Vector makeNodeList(Class nClass, boolean incsub, String notat) {
	Vector v = this.makeNodeList(nClass, incsub);
	if (v == null) return null;
	Vector retval = null;
	Enumeration enum1 = v.elements();
	while (enum1.hasMoreElements()) {
	    Node n = (Node)enum1.nextElement();
	    if (n.getNotation().equals(notat)) {
		if (retval == null) retval = new Vector(2);
		retval.addElement((Object)n);
	    }
	}

	return retval;
    }

    public synchronized Vector makeNodeList(String nodename) {
	if (this.size() == 0) return null;
	Vector retval = null;
	Enumeration enum1 = this.elements();
	while (enum1.hasMoreElements()) {
	    Node n = (Node)enum1.nextElement();
	    String match = n.getName();
	    if (nodename.equals(match)) {
		if (retval == null) retval = new Vector(10);
		retval.addElement((Object)n);
	    } else if (nodename.equals(match + "_" + n.getInstanceNumber())) {
		if (retval == null) retval = new Vector(10);
		retval.addElement((Object)n);
	    } else if (nodename.equals(n.getMatchString())) {
		if (retval == null) retval = new Vector(10);
		retval.addElement((Object)n);
	    }
	}
	return retval;
    }

    public synchronized void addNode(Node n) {
	this.addElement((Object)n);
    }

    public synchronized Node getNode(String name, int instance) {
	Vector v = this.makeNodeList(name);
	Enumeration enum1 = v.elements();
	while (enum1.hasMoreElements()) {
	    Node n = (Node)enum1.nextElement();
	    if (n.getInstanceNumber() == instance) 
		return n;
	}
	return null;
    }

    public void addPanel(ControlPanel p) {
	synchronized (this.panelList) {
	    this.panelList.addElement((Object)p);
	}
    }

}; // end Network
