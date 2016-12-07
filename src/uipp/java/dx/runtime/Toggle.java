//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/Toggle.java,v 1.2 2005/10/27 19:43:08 davidt Exp $
 */
package dx.runtime;
//
// FIXME: We're only supporting the case where the output value is of type int.
//

import java.awt.*;
import dx.net.PacketIF;

public class Toggle extends BinaryInstance {

    //
    // private member data
    //

    //
    // protected member data
    //

public boolean action (Event e, Object o)
{
    if ((e.target == ipart) && (e.target instanceof Checkbox)) {
	PacketIF pif = this.getNode();
	pif.setOutputValue (getValue());
	return true;
    }
    return false;
}

//
// By default we'll create a text widget for the interactor.  This should
// probably be a virtual method, but there will be so may interactors for
// which there is no java translation that we want to be able to use
// a plain text widget by default
//
protected void createPart() {
    Checkbox cb = new Checkbox();
    ipart = cb;
    //cb.setLabel(" ");
}


//
//
//
protected String getTypeName() {
    return "Toggle";
}

public Insets getComponentInsets()
{
    return new Insets (0,0,0,0);
}

public boolean isOptionSelected(int i)
{
    Checkbox cb = (Checkbox)ipart;
    if ((cb.getState() == true) && (i == 1))
	return true;
    else if ((cb.getState() == false) && (i == 2))
	return true;
    return false;
}

//
// If the subclass supplies a createPart method then it must supply a getValue also
//
public String getValue()
{
    return getOutputValue();
}

//
// If the subclass supplies a createPart method then it must supply a setValue also
//
// FIXME: I don't know how to convert between ints and strings in java.
public void setValue(String s)
{
}

public void selectOption (int i)
{
    Checkbox cb = (Checkbox)ipart;
    if (i == 1)
	cb.setState(true);
    else
	cb.setState(false);
}

public String getOptionName(int i) { return ""; }
public void newItem(int i, String s) {  }

public void clearOptions() {}

} // end class Toggle

