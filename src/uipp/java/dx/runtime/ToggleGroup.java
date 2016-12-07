//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/ToggleGroup.java,v 1.1.1.1 1999/03/24 15:17:32 gda Exp $
 */
package dx.runtime;
//
//
//

import java.awt.*;
import dx.net.PacketIF;

public class ToggleGroup extends ListSelector {

    //
    // private member data
    //
    private Object buttons[];
    private Color local_fg = null;
    private Font local_font = null;
    private Panel pan = null;
    private boolean initialized = false;

    //
    // protected member data
    //
    protected boolean singleItemSelectable() { return true; }

public boolean action (Event e, Object o)
{
    if (e.target instanceof Checkbox) {
	PacketIF pif = this.getNode();
	pif.setOutputValues (this.getOutputValue(), this.getOutput2Value());
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
    pan = new Panel();
    if (local_bg != null)
	pan.setBackground(local_bg);
    ipart = pan;
    if (option_count > 0) {
	GridLayout gbl = new GridLayout(option_count, 0, 0, 0);
	pan.setLayout(gbl);
    }
    initialized = true;
    if (option_count == 0) return ;

    try {
	int i;
	for (i=1; i<=option_count; i++) 
	    newItem (i, getOptionName(i));
    } catch (Exception e) {
	e.printStackTrace();
    }
    validate();
}

//
//
//
protected String getTypeName() {
    return "ButtonList";
}

public boolean isOptionSelected(int i)
{
    Checkbox cb = (Checkbox)buttons[i-1];
    if (cb.getState() == true)
	return true;
    return false;
}

public void selectOption (int i)
{
    if (buttons == null) return ;
    Checkbox cb = (Checkbox)buttons[i-1];
    if (cb == null) return ;
    cb.setState(true);
}

public void newItem(int i, String s)
{
    if (initialized == false) return ;
    if (i>option_count) return ;
    Checkbox cb = new Checkbox(s);
    pan.add(cb, i-1);
    buttons[i-1] = cb;
    if (local_fg != null)
	cb.setForeground(local_fg);
    if (local_bg != null)
	cb.setBackground(local_bg);
    if (local_font != null)
	cb.setFont(local_font);
}
public void clearOptions() 
{
    pan.removeAll();
    buttons = null;
}

public void setOptionCount(int count)
{
    super.setOptionCount(count);
    buttons = new Object[count];
    if (initialized == true) {
	GridLayout gbl = new GridLayout(count, 0, 0, 0);
	pan.setLayout(gbl);
    }
}

public void setBackground(Color newc) {
    super.setBackground(newc);
    if (this.pan != null) this.pan.setBackground(newc);
    if (buttons != null) {
	int i;
	for (i=this.option_count; i>=0; i--) {
	    Checkbox cb = (Checkbox)buttons[i];
	    cb.setBackground(newc);
	}
    }
}

} // end class ToggleGroup

