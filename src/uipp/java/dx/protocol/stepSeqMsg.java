//

package dx.protocol;
import java.net.*;
import java.util.*;

//
//
public class stepSeqMsg extends execOnceMsg {

    int frame;
    int min;
    int max;
    int delta;
    int instance;
    int paramNo;
    String name;


    public void setFrame(int frame) { this.frame = frame; }
    public void setMin(int min) { this.min = min; }
    public void setMax(int max) { this.max = max; }
    public void setDelta(int delta) { this.delta = delta; }
    public void setInstance(int inst) { this.instance = inst; }
    public void setParamNumber(int pno) { this.paramNo = pno; }
    public void setName(String name) { this.name = name; }

    public int getFrame() { return this.frame; }
    public int getMin() { return this.min; }
    public int getMax() { return this.max; }
    public int getDelta() { return this.delta; }
    public int getInstance() { return this.instance; }
    public int getParamNumber() { return this.paramNo; }
    public String getName() { return this.name; }

    public stepSeqMsg() { 
	super(); 
	this.frame = this.min = this.max = this.delta = 0;
	this.instance = 0;
	this.paramNo = 7;
	this.name = null;
    }
    public stepSeqMsg(String inputLine) { 
	super(inputLine);
	//
	// parse our 4 ints, remove them from the inputLine,
	// then call superclass on what's left over.
	//
	this.frame = this.min = this.max = this.delta = 0;
	int comma = inputLine.indexOf((int)',');
	try {
	    comma = inputLine.indexOf((int)',', comma+1);
	    StringTokenizer stok = new StringTokenizer(inputLine, ",");
	    String tok = stok.nextToken();
	    tok = stok.nextToken();

	    String minstr = stok.nextToken();
	    String maxstr = stok.nextToken();
	    String deltastr = stok.nextToken();
	    String framestr = stok.nextToken();
	    String inststr = stok.nextToken();
	    String pnostr = stok.nextToken();
	    this.name = stok.nextToken();
	    this.frame = (new Integer(framestr)).intValue();
	    this.min = (new Integer(minstr)).intValue();
	    this.max = (new Integer(maxstr)).intValue();
	    this.delta = (new Integer(deltastr)).intValue();
	    this.instance = (new Integer(inststr)).intValue();
	    this.paramNo = 7;
	} catch (Exception e) {
	}
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.stepSeqMsg").getName(); }
    public String toString() { 
	return super.toString() + "," + this.min + "," + this.max + "," + 
	    this.delta + "," + this.frame + "," + this.instance + "," + 
	    this.paramNo + "," + this.name;
    }

    public String getSequencerParams() {
	String macro_name = this.getMacroName();
	String value = 
	    "{ " + 
		this.min + " " + this.max + " " + this.delta + " " + 
		this.min + " " + this.max + " " + this.frame + 
	    " }";
	return 
	    "Executive(\"assign noexecute\"," +
		"\"" + macro_name + "_" + this.name + "_" + this.instance +
		"_in_" + this.paramNo + "\"," + value + ");";
    }
    public String getSequencerFrame() {
	return "@frame=" + this.frame + ";";
    }
}
