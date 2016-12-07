//

package dx.net;
import java.util.*;
import dx.runtime.BinaryInstance;
import dx.runtime.Interactor;

public class ResetNode extends dx.net.ToggleNode {

    public ResetNode(Network net, String name, int instance, String notation) {
	super(net, name, instance, notation);
    }

    //
    // Resetting oneshot
    //
    public void handleMessage(String msg) {
	if (msg.lastIndexOf ("Resetting oneshot") != -1) {
	    Interactor i = (Interactor)this.interactors.elementAt(0);
	    BinaryInstance bi = (BinaryInstance)i;
	    bi.selectOption (0);
	    return ;
	} 
	super.handleMessage(msg);
    }

    public void installHandlers() {
	DXApplication dxa = (DXApplication)this.getNetwork().getApplet();
	String macro_name = dxa.getMacroName();
	registerHandler (" 0:  " + macro_name + "_" + this.getMatchString() + "_out_1");
	registerHandler (macro_name + "_" + this.getMatchString() + "_out_1");
    }

    //
    // Only 2 possibilities:
    // When the user pushes the button in:
    // main_Reset_1_in_2, main_Reset_1_in_3, main_Reset_1_out_1[oneshot:"oranges"] =
    //	"apples", 1, "apples";
    //
    // or
    // When the user pulls the button out:
    // main_Reset_1_in_2, main_Reset_1_in_3, main_Reset_1_out_1 = "oranges", 0, "oranges";
    //
    public void setOutputValue (String value) {
	DXLinkApplication dxa = (DXLinkApplication)this.getNetwork().getApplet();
	String macro_name = dxa.getMacroName();
	String setval = (String)this.values.elementAt(0);
	String unsetval = (String)this.values.elementAt(1);
	String mstr = macro_name + "_" + this.getMatchString();
	String assignment = null;
	int output_type = BinaryInstance.INTEGER;
	if (this.interactors != null) {
	    Interactor i = (Interactor)this.interactors.elementAt(0);
	    BinaryInstance bi = (BinaryInstance)i;
	    output_type = bi.getOutputType();
	}

	int number = BinaryInstance.INTEGER | BinaryInstance.SCALAR;
	String lhs = mstr + "_in_2, " + mstr + "_in_3," + mstr + "_out_1";
	if (value.equals(setval)) {
	    if ((number & output_type) == 0) {
		assignment = lhs + "[oneshot:\"" + unsetval+ "\"] = " + setval +
		    ", 1, " + setval + ";";
	    } else {
		assignment = lhs + "[oneshot:" + unsetval+ "] = " + setval +
		    ", 1, " + setval + ";";
	    }
	} else if (value.equals(unsetval)) {
	    if ((number & output_type) == 0) {
		assignment = lhs + " = " + unsetval + ", 0, " + unsetval + ";";
	    } else {
		assignment = lhs + " = \"" + unsetval + "\", 0, \"" + unsetval + "\";";
	    }
	}
	if (assignment != null) {
	    DXLSend(assignment);
	} else {
	    System.out.println ("Reset Error: " + value + " doesn't match " +
	       setval + " or " + unsetval);
	}
    }


}; // end ResetNode
