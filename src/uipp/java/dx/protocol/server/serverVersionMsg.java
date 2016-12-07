//

package dx.protocol.server;
import java.net.*;
import java.io.*;

//
// JavaDir,path
//
public class serverVersionMsg extends serverMsg {
    private int major, minor, micro;
    public serverVersionMsg() {  
	super();
	major = minor = micro = 0;
    }
    public serverVersionMsg(String inputLine) {  
	super(inputLine);
	try {
	    int ndx = inputLine.indexOf((int)',');
	    String verstr = inputLine.substring(ndx+1);
	    ndx = verstr.indexOf((int)'.');
	    String majstr = verstr.substring(0,ndx);
	    int ndx2 = verstr.lastIndexOf((int)'.');
	    String minstr = verstr.substring(ndx+1,ndx2);
	    String micstr = verstr.substring(ndx2+1);

	    Integer maji = new Integer(majstr);
	    Integer mini = new Integer(minstr);
	    Integer mici = new Integer(micstr);

	    this.major = maji.intValue();
	    this.minor = mini.intValue();
	    this.micro = mici.intValue();
	} catch (Exception nfe) {
	}
    }

    public int getMajor() { return major; }
    public int getMinor() { return minor; }
    public int getMicro() { return micro; }

    public void setVersion(int major, int minor, int micro) {
	this.major = major;
	this.minor = minor;
	this.micro = micro;
    }
    public static String GetCommand() throws ClassNotFoundException
	{ return Class.forName("serverVersionMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + major + "." + minor + "." + micro;
    }
}
