//

package dx.net;
import java.applet.*;
import java.util.Vector;
import java.net.*;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.*;
import java.lang.reflect.*;

import netscape.javascript.JSObject;

import vrml.external.field.*;
import vrml.external.Node;
import vrml.external.Browser;
import vrml.external.exception.*;

public abstract class WRLApplication 
    extends DXLinkApplication implements vrml.external.field.EventOutObserver {

    private Vector cosmos;
    private EventOutSFVec3f hitPointEO;
    private EventOutSFTime touchTimeEO;
    private EventOutSFVec3f vps_tp = null;
    private EventOutSFRotation vps_rot = null;
    private JSObject window;
    private JSObject parent;
    private JSObject document;
    private JSObject frames;

    protected WRLApplication() {
	super();
	this.vps_tp = null;
	this.vps_rot = null;
	this.cosmos = new Vector(4);
	this.window = null;
	this.parent = null;
	this.document = null;
	this.frames = null;
    }

    public void start() {
	super.start();
	if (this.window == null) {
	    this.window = (JSObject)JSObject.getWindow(this);
	    this.parent = (JSObject)window.getMember("parent");
	    this.document = (JSObject)window.getMember("document");
	    this.frames = (JSObject)parent.getMember("frames");
	}
    }

    public void showWorld(String name, String path, int id) {
	this.locateWorlds();
	URL url = null;
	try {
	    url = new URL(getCodeBase(), path);
	} catch (MalformedURLException mue) {
	    System.out.println ("WRLApplication: cannot create url from " + path);
	}
	boolean success = false;
	InputStream is = null;
	try {
	    Browser cosmo = (Browser)this.cosmos.elementAt(id);

	    //
	    // If you ever want to know for absolute sure, what methods
	    // are supported in your implementation of a vrml browser,
	    // then enable this code.
	    //
	    /*
	    Class cclass = cosmo.getClass();
	    Method[] meds = cclass.getMethods();
	    int mlen = meds.length;
	    int i;
	    for (i=0; i<mlen; i++) 
		System.out.println ("method."+i+": " + meds[i].toString());
	    */
	    URLConnection urlc = url.openConnection();
	    is = urlc.getInputStream();
	    int length = urlc.getContentLength();
	    InputStreamReader isr = new InputStreamReader(is);
	    char[] buf = new char[length];
	    isr.read (buf, 0, length);
	    String wrlstr = new String(buf);
	    vrml.external.Node[] newWorld = cosmo.createVrmlFromString(wrlstr);
	    cosmo.replaceWorld(newWorld);
	    success = true;
	} catch (Exception e) {
	    System.out.println ("WRLApplication: unable to assign to plugin");
	    e.printStackTrace();
	}
	try {
	    if (is != null) is.close();
	} catch (Exception e) {
	}
	if (success == false) 
	    this.getAppletContext().showDocument(url, name);
    }

    protected void locateWorlds() {
	JSObject doc = this.document;
	int frame_index = 0;
	boolean finished = false;
	while (finished == false) {
	    int i = 0;
	    try {
		JSObject embeds = (JSObject)doc.getMember("embeds");
		String lstr = embeds.getMember("length").toString();
		int length = Double.valueOf(lstr).intValue();
		i = 0;
		while (i<length) {
		    //
		    // Expect getSlot to throw an exception when reading past
		    // the end of the array.  Otherwise I don't know how
		    // many embeds there are.  There is an embeds.length
		    // property, but I don't know how to fetch its value.
		    //
		    if (embeds.getSlot(i) instanceof Browser) {
			Browser browser = (Browser)embeds.getSlot(i);
			this.cosmos.addElement((Object)browser);
		    }
		    i++;

		    //
		    // These calls will throw an exception if cosmo isn't finished
		    // reading in the .wrl file.
		    //
		    //vrml.external.Node TS = null;
		    //TS	    = (vrml.external.Node)browser.getNode("TS");
		    //hitPointEO    = (EventOutSFVec3f)TS.getEventOut("hitPoint_changed");
		    //touchTimeEO   = (EventOutSFTime)TS.getEventOut("touchTime");
		    //touchTimeEO.advise(this,null);
		    //vrml.external.Node VPS = (vrml.external.Node)browser.getNode("PS");
		    //vps_rot =(EventOutSFRotation)VPS.getEventOut("orientation_changed");
		    //vps_tp = (EventOutSFVec3f)VPS.getEventOut("position_changed");

		}
	    } catch (Exception e) {
		System.out.println ("locateWorlds: failed i=" + i);
		e.printStackTrace();
	    }
	    try {
		JSObject frame = (JSObject)this.frames.getSlot(frame_index);
		frame_index++;
		doc = (JSObject)frame.getMember("document");
	    } catch (Exception e) {
		finished = true;
	    }
	}
	if (this.cosmos.size() < 1) {
	    Browser browser = Browser.getBrowser(this);
	    if (browser != null)
		this.cosmos.addElement((Object)browser);
	}
    }

    public void callback(EventOut who,double when, Object dummy)
    {
	float []C=hitPointEO.getValue();
	float []spt = vps_tp.getValue();
	float []rot = vps_rot.getValue();

    }

}; // end WRLApplication
