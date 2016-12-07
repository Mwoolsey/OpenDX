//

/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/ValueNode.java,v 1.4 2005/10/27 19:43:07 davidt Exp $
 */

package dx.net;
import java.util.*;
import dx.runtime.*;

public class ValueNode extends dx.net.InteractorNode {

    private String value ;

    public ValueNode(Network net, String name, int instance, String notation) {
	super(net, name, instance, notation);
	this.value = null;
    }

    public synchronized void setValue (String value) {
		this.value = value;
    }

    public void installValues (Interactor i) {
		super.installValues(i);
		if (this.value == null) return ;
			i.setValue(this.value);
    }

}; // end ValueNode
