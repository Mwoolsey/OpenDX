//

/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/ToggleNode.java,v 1.3 1999/07/09 19:30:00 daniel Exp $
 */

package dx.net;
import java.util.*;
import dx.runtime.BinaryInstance;
import dx.runtime.Interactor;

public class ToggleNode extends dx.net.SelectionNode {

    public ToggleNode(Network net, String name, int instance, String notation) {
	super(net, name, instance, notation);
    }

    protected String mungeString(String s) { return s; }

    //
    // unset=...
    // set=...
    //
    public boolean handleAssignment(String lhs, String rhs) {
	boolean handled = false;
	boolean set = false;
	if (lhs.equals("unset")) {
	    handled = true;
	} else if (lhs.equals("set")) {
	    handled = true;
	    set = true;
	}

	if (handled) {
	    if (this.values == null)
		this.values = new Vector(2);

            Interactor i = null;
	    int output_type = BinaryInstance.STRING;
	    if (this.interactors != null) {
		i = (Interactor)this.interactors.elementAt(0);
		BinaryInstance bi = (BinaryInstance)i;
		output_type = bi.getOutputType();
	    }

	    int number = BinaryInstance.INTEGER | BinaryInstance.SCALAR;
	    Vector value = null;
	    if ((output_type & BinaryInstance.STRING) != 0) {
		value = this.makeStrings(rhs);
	    } else if ((output_type & number) != 0) {
		value = this.makeNumbers(rhs);
	    } else {
		handled = false;
		System.out.println
		    ("Warning:  Your data-driven interactor received unparsable data");
		System.out.println ("    : " + rhs);
	    }
	    if (handled) {
		Object obj = value.elementAt(0);
		this.values.setElementAt(obj, (set?0:1));
	    }
	} 

	if (handled == false)
	    return super.handleAssignment(lhs,rhs);
	return true;
    }


    protected int getShadowingInput (int output) {
	if (output == 1) return 2;
	return -1;
    }

    //
    // Toggle has the unusual behavior that setting its output requires
    // setting a flag (set/unset) in addition to the normal work.
    //
    public void setOutputValue(String value) {
	String setval = (String)this.values.elementAt(0);
	String unsetval = (String)this.values.elementAt(1);
	boolean set = false;
	if (value.equals(setval)) {
	    this.selectOption(1);
	    set = true;
	} else if (value.equals(unsetval)) {
	    this.selectOption(2);
	} else {
	}
	super.setInputValueQuietly (3, set?"1":"0");
	super.setOutputValue(value);
    }

    public void selectOption (int s) {
	this.selections = null;
	super.selectOption(s);
    }


}; // end ToggleNode
