//

package dx.net;
import java.util.*;

//
// src/dxmods/sequencer.m  wants max > min.  i.o.w delta must be positive
//
public final class SequencerNode extends dx.net.DrivenNode 
{
    private int next;
    private boolean reset;

    private final static int ID_PARAM_NUM    	= 1;
    private final static int DATA_PARAM_NUM  	= 2;
    private final static int FRAME_PARAM_NUM 	= 3;
    private final static int MIN_PARAM_NUM   	= 4;
    private final static int MAX_PARAM_NUM   	= 5;
    private final static int DELTA_PARAM_NUM 	= 6;
    private final static int ATTR_PARAM_NUM  	= 7;

    public SequencerNode(Network net, String name, int instance, String notation) {
	super(net, name, instance, notation);
	this.reset = true;
    }

    public boolean handleAssignment (String lhs, String rhs) {
	boolean handled = false;

	if (lhs.equals("min")) {
	    this.setInputValueString (MIN_PARAM_NUM, rhs);
	    handled = true;
	} else if (lhs.equals("max")) {
	    this.setInputValueString (MAX_PARAM_NUM, rhs);
	    handled = true;
	} else if (lhs.equals("delta")) {
	    this.setInputValueString (DELTA_PARAM_NUM, rhs);
	    handled = true;
	} else if (lhs.equals("frame")) {
	    this.setInputValueString (FRAME_PARAM_NUM, rhs);
	    handled = true;
	}

	if (handled) return true;
	return super.handleAssignment(lhs,rhs);
    }

    //
    // set min to nextframe, execute once, increment nextframe
    //
    public synchronized void step(DXApplication dxapp) {
	int min = this.getMin(); 
	int max = this.getMax();
	int delta = this.getDelta();

	if (this.reset) {
	    this.next = min;
	    this.reset = false;
	    String str = "{ " +min+ " " +max+ " " +delta+ " " +min+ " " +max+ " 1 }";
	    this.setInputValueQuietly (SequencerNode.ATTR_PARAM_NUM,  str);
	} 

	if (this.next > max) {
	    this.reset = true;
	    if (dxapp.getEocMode() == false)
		dxapp.DXLSend("@frame=" + min + ";");
	} else {
	    int tmp = this.next;
	    this.next+= delta;
	    dxapp.DXLSend("@frame=" + tmp + ";");
	    if (dxapp.getEocMode() == false)
		dxapp.DXLExecuteOnce();
	}
    }

    public void reset() { this.reset = true; }

    public boolean hasMoreFrames() { return (this.next <= this.getMax()); }

    public int getSteps() {
	int min = this.getMin(); 
	int max = this.getMax();
	int delta = this.getDelta();

	if (delta == 0) return 10;
	int steps = (max-min) / delta;
	return steps+1;
    }
    protected boolean needsSpacesReplaced() { return true; }

    public int getMin() {
	int min = 0;
	String minstr = this.getInputValueString (SequencerNode.MIN_PARAM_NUM);
	try { min = Integer.parseInt(minstr);
	} catch (NullPointerException npe) {
	    System.out.println ("SequencerNode: Missing min value");
	} catch (NumberFormatException nfe) {
	    System.out.println ("SequencerNode: Illegal min value: " + minstr);
	}
	return min;
    }
    public int getMax() {
	int max = 100;
	String maxstr = this.getInputValueString (SequencerNode.MAX_PARAM_NUM);
	try { max = Integer.parseInt(maxstr);
	} catch (NullPointerException npe) {
	    System.out.println ("SequencerNode: Missing max value");
	} catch (NumberFormatException nfe) {
	    System.out.println ("SequencerNode: Illegal max value: " + maxstr); }
	return max;
    }
    public int getDelta() {
	int delta = 1;
	String dltstr = this.getInputValueString (SequencerNode.DELTA_PARAM_NUM);
	try { delta = Integer.parseInt(dltstr);
	} catch (NullPointerException npe) {
	    //System.out.println ("SequencerNode: Missing delta value");
	} catch (NumberFormatException nfe) {
	    //System.out.println ("SequencerNode: Illegal delta value: " + dltstr);
	}
	return delta;
    }

    public int getFrame() { 
	if (this.reset) {
	    this.next = this.getMin();
	    this.reset = false;
	}
	return this.next; 
    }

    public void stepQuietly() {
	if (this.reset) {
	    this.next = this.getMin();
	    this.reset = false;
	} else {
	    this.next+= this.getDelta();
	    if (this.next > this.getMax())
		this.reset = true;
	}
    }

}; // end SequencerNode

