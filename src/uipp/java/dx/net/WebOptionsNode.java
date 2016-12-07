//

package dx.net;
import java.util.*;
import java.applet.*;

public class WebOptionsNode extends dx.net.Node {

    private static final int WEB_FORMAT 	= 1;
    private static final int WEB_FILE 		= 2;
    private static final int WEB_ENABLED 	= 3;
    private static final int WEB_COUNTERS 	= 4;
    private static final int WEB_ORBIT 		= 5;
    private static final int WEB_ID 		= 6;
    private static final int WEB_IMGID		= 7;
    private static final int WEB_ORBIT_DELTA	= 8;
    private static final int WEB_CACHE_SIZE	= 9;
    private static final int WEB_GROUP_NAME	= 10;

    public WebOptionsNode(Network net, String name, int instance, String notation) {
	super(net, name, instance, notation);
    }

    public void setOrbitMode(boolean onoff) {
	this.setInputValue (WebOptionsNode.WEB_ORBIT, (onoff?"1":"0"));
    }

    public void enableJava() {
	super.enableJava();
	DXLinkApplication dxa = (DXLinkApplication)this.getNetwork().getApplet();
	int java_id = dxa.getJavaId();
	String file = 
	    "\"" + 
		dxa.getOutputDir() + "/" + 
		java_id + "_" + this.getInstanceNumber() + 
	    "\"" ;
	this.setInputValue(WebOptionsNode.WEB_FILE, file);
	this.setInputValue(WebOptionsNode.WEB_ENABLED, "1");
	this.setInputValue(WebOptionsNode.WEB_ID, ""+java_id);
    }

}; // end WebOptionsNode
