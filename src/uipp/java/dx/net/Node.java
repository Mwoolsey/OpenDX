//


package dx.net;
import java.util.*;
import java.applet.*;

//-------------------------------------------------------
// 3/17/2006 jer
// Added method: setOutputValueQuietly(String)
// This is to support Interactors that want to keep the
// server informed of changed values without causing
// an execution.
//-------------------------------------------------------

public class Node extends Object implements dx.net.PacketIF {

    private int instance;
    private Network net;
    private String name;
    private int param_number;

    private Vector input_arcs;
    private Hashtable input_values;

    private String notation;

    public String getNotation() { return this.notation; }

    public void addInputArc (int input, Node src, int output) {
        if (this.input_arcs == null)
            this.input_arcs = new Vector(4);
        if (this.isInputConnected(input) == true) return ;
        Arc a = new Arc(src, output, this, input);
        this.input_arcs.addElement(a);
    }

    public void setInputValueString (int input, String value) {
        if (this.input_values == null)
            this.input_values = new Hashtable(4);
        //if (this.isInputConnected(input) == true) return ;
        Integer i = new Integer(input);
        //
        // Can hashtable hold duplicate keys?
        //
        if (this.input_values.containsKey(i) == true)
            this.input_values.remove(i);
        this.input_values.put(i, value);
    }

    public String getInputValueString(int input) {
        if (this.input_values == null) return null;
        Integer i = new Integer(input);
        return (String)this.input_values.get(i);
    }

    protected Node getSourceNode(int input) {
        if (this.input_arcs == null) return null;
        Enumeration enum1 = this.input_arcs.elements();
        while (enum1.hasMoreElements()) {
            Arc a = (Arc)enum1.nextElement();
            if (a.getInputParamNumber() == input)
                return a.getSourceNode();
        }
        return null;
    }

    protected boolean isInputConnected(int input) {
        if (this.input_arcs == null) return false;
        Enumeration enum1 = this.input_arcs.elements();
        while (enum1.hasMoreElements()) {
            Arc a = (Arc)enum1.nextElement();
            if (a.getInputParamNumber() == input)
                return true;
        }
        return false;
    }

public void installHandlers() { }
    public void enableJava() { this.installHandlers(); }

    public Node(Network net, String name, int instance, String notation) {
        super();
        this.instance = instance;
        this.net = net;
        this.name = name;
        this.param_number = 1;
        this.input_arcs = null;
        this.input_values = null;
        this.notation = notation;
    }

    public String getMatchString() { return null; }
    public void   parseInput(String msg){ }
    public void   setParameterNumber(int p) { this.param_number = p; }

    public String getName() { return this.name; }
    public Network getNetwork() { return this.net; }
    public int  getInstanceNumber() { return this.instance; }
    public int  getParameterNumber() { return this.param_number; }

    public void setInputValue (int param, String value) {
        DXLinkApplication dxa = (DXLinkApplication)this.net.getApplet();
        String macro_name = dxa.getMacroName();
        String assignment = macro_name + "_" + name + "_" + this.instance +
                            "_in_" + param + " = " + value + ";";
        DXLSend(assignment);
    }

    public void setInputValueQuietly (int param, String value) {
        DXLinkApplication dxa = (DXLinkApplication)this.net.getApplet();
        String macro_name = dxa.getMacroName();
        String assignment = "Executive(\"assign noexecute\"," +
                            "\"" + macro_name + "_" + name + "_" + this.instance +
                            "_in_" + param + "\"," + value + ");";
        DXLSend(assignment);
    }

    public void setOutputValue (String value) {
        this.setOutputValue (this.getParameterNumber(), value);
    }
    public void setOutputValues (String name, String value) { }

    //------------------------------------------------------------------
    // BEGIN: 3/17/2006 jer
    public void setOutputValueQuietly(String value) {
        this.setOutputValueQuietly (this.getParameterNumber(), value);
    }

    protected void setOutputValueQuietly (int param, String value) {
        DXLinkApplication dxa = (DXLinkApplication)this.net.getApplet();
        String macro_name = dxa.getMacroName();
        String assignment = "Executive(\"assign noexecute\"," +
                            "\"" + macro_name + "_" + name + "_" + this.instance +
                            "_out_" + param + "\"," + value + ");";
        DXLSend(assignment);
    }
    // END: 3/17/2006 jer
    //------------------------------------------------------------------

    protected void setOutputValue (int param, String value) {
        DXLinkApplication dxa = (DXLinkApplication)this.net.getApplet();
        String macro_name = dxa.getMacroName();
        String assignment = macro_name + "_" + name + "_" + this.instance +
                            "_out_" + param + " = " + value + ";";
        DXLSend(assignment);
    }

    protected void DXLSend(String assignment) {
        try {
            DXApplication dxa = (DXApplication)this.net.getApplet();
            dxa.DXLSend(assignment);
        } catch (Exception e) {
        }
    }

    protected void registerHandler (String matchStr) {
        try {
            DXLinkApplication dxa = (DXLinkApplication)this.net.getApplet();
            dxa.registerHandler(matchStr, this);
        } catch (Exception e) {
        }
    }

    public void handleMessage (String msg) {
        StringTokenizer stok = new StringTokenizer(msg, ";");
        while (stok.hasMoreTokens()) {
            String tok = stok.nextToken();
            try {
                StringTokenizer assign_tok = new StringTokenizer(tok, "=");
                String lhs = assign_tok.nextToken();
                StringTokenizer remspaces = new StringTokenizer(lhs, " ");
                String realLhs = null;
                while (remspaces.hasMoreTokens())
                    realLhs = remspaces.nextToken();
                lhs = realLhs;
                String rhs = assign_tok.nextToken();
                handleAssignment (lhs, rhs);
            } catch (Exception e) {
            }
        }
    }

    public boolean handleAssignment (String lhs, String rhs) {
        //System.out.println (this.name + ": not handled: " + lhs + "=" + rhs);
        return false;
    }

}; // end Node

