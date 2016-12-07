//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/BinaryInstance.java,v 1.1.1.1 1999/03/24 15:17:32 gda Exp $
 */
package dx.runtime;
import java.awt.*;
import java.util.*;

//
// Interface for any Interactor object which uses multiple representations for
// a single thingy on the screen. Examples: Toggle has pushed-in/set-value and
// pulled-out/unset-value.  Selector has the option-name/option-value.
//
abstract public class BinaryInstance extends Interactor {
    //
    // Constants
    //
    static public final int INTEGER    = 0x00000001;
    static public final int SCALAR     = 0x00000005;
    static public final int STRING     = 0x00000020;
    static public final int VECTOR     = 0x00000008;
    static public final int LIST       = 0x01000000;

    protected int option_count;
    protected int output_type = BinaryInstance.STRING;
    protected String output_values[];
    protected String option_names[];

    abstract protected void newItem(int i, String s);

    //
    // Public
    //
    abstract public boolean isOptionSelected(int i);
    abstract public void selectOption(int i);
    abstract public void clearOptions();

public int getOptionCount() { return option_count; }
public int getOutputType() { return output_type; }

public void setOptionCount(int count)
{
    option_count = count;
    output_values = new String[count];
    option_names = new String[count];
}


public String getOptionValue (int i)
{
    if (i > option_count) return null;
    return output_values[i-1];
}

public void setOutputType (int type)
{
    output_type = type;
}

//
// The values returned are the not the ones displayed in the interactor,
// they're the ones the net developer assigned to each option in the
// interactor.
//
public String getOutputValue()
{
int i;
boolean anything_selected = false;

    String retval = null;
    if ((output_type & BinaryInstance.LIST) == BinaryInstance.LIST) 
	retval = "{ ";

    for (i=1; i<=option_count; i++) {
	if (isOptionSelected(i)) {
	    anything_selected = true;
	    if ((output_type & BinaryInstance.STRING) == BinaryInstance.STRING) {
		if (retval == null)
		    retval = "\"" + getOptionValue(i) + "\" ";
		else
		    retval = retval + "\"" + getOptionValue(i) + "\" ";
	    } else {
		if (retval == null)
		    retval = "" + getOptionValue(i) ;
		else
		    retval = retval + " " + getOptionValue(i);
	    }
	}
    }

    if (anything_selected == false)
	retval = "NULL";
    else if ((output_type & BinaryInstance.LIST) == BinaryInstance.LIST)
	retval = retval + " }";

    return retval;
}

public String getOutput2Value()
{
int i;
boolean anything_selected = false;

    String retval = null;
    if ((output_type & BinaryInstance.LIST) == BinaryInstance.LIST)
	retval = "{ ";

    for (i=1; i<=option_count; i++) {
	if (isOptionSelected(i)) {
	    anything_selected = true;
	    if (retval == null)
		retval = "\"" + getOptionName(i) + "\" ";
	    else
		retval = retval + "\"" + getOptionName(i) + "\" ";
	}
    }

    if (anything_selected == false) 
	retval = "NULL";
    else if ((output_type & BinaryInstance.LIST) == BinaryInstance.LIST)
	retval = retval + " }";
    return retval;
}
    

//
// If the subclass supplies a createPart method then it must supply a getValue also
//
public String getValue()
{
    return getOutputValue();
}

public void setOptions (Vector names, Vector values)
{
    clearOptions();
    int size = names.size();
    int i;
    setOptionCount (names.size());
    names.copyInto(option_names);
    values.copyInto(output_values);
    for (i=1;i <=size; i++) {
	String s = (String)names.elementAt(i-1);
	newItem(i,s);
    }
}


public String getOptionName(int i)
{
    if (i>option_count) return null;
    return option_names[i-1];
}

} // BinaryInstance

