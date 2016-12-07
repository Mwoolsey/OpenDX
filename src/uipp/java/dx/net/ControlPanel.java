//

/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/ControlPanel.java,v 1.5 2005/12/26 21:33:43 davidt Exp $
 */

package dx.net;
import java.util.*;
import java.awt.*;
import dx.runtime.*;

public class ControlPanel extends Panel
{

    private Network net;
    private String name;
    private int instance;
    private Vector interactors;
    private Vector decorators;
    private Dimension size = new Dimension(0, 0);
    //private Checkbox tab;

    public ControlPanel( Network net, String name, int instance )
    {
        super();

        if ( name == null ) this.name = "ControlPanel " + instance;
        else this.name = name;

        this.instance = instance;
        this.net = net;
        this.interactors = null;
    }

    public String getName()
    {
        return this.name;
    }

//    public Checkbox getTab()
//    {
//        return this.tab;
//    }

    public synchronized void addInteractor( Interactor i )
    {
        if ( this.interactors == null )
            this.interactors = new Vector( 4 );

        this.interactors.addElement( ( Object ) i );
    }

    public synchronized void addDecorator( Component d )
    {
        if ( this.decorators == null )
            this.decorators = new Vector( 4 );

        this.decorators.addElement( ( Object ) d );
    }

	// Added to help get TabbedPanel to show ControlPanels correctly.
	
	public Dimension getMinimumSize() {
		if(size.width > 0 || size.height > 0)
			return size;
		else if(this.getLayout() == null) {
			int w = 0;
			int h = 0;
		
			for (int i = 0; i<this.getComponentCount(); i++) {
				Rectangle r = getComponent(i).getBounds();
				w = Math.max(w, r.x+r.width);
				h = Math.max(h, r.y+r.height);
			}
			
			return new Dimension(w, h);
		} else {
			return super.getMinimumSize();
		}
	}
	
	public void setSize(Dimension d) {
		size = d;
	}
	
	public Dimension getSize() {
		return getMinimumSize();
	}
	
	public Dimension getPreferredSize () {
		if(size.width > 0 || size.height > 0)
			return size;
		if(this.getLayout() == null) 
			return getMinimumSize();
		else
			return super.getPreferredSize();
	}
	
    public synchronized void buildPanel()
    {
        this.setLayout( null );

        if ( this.interactors != null ) {
            Enumeration enum1 = interactors.elements();
            Interactor i;

            while ( enum1.hasMoreElements() ) {
                i = ( Interactor ) enum1.nextElement();
                i.init();
                InteractorNode n = ( InteractorNode ) i.getNode();
                n.installValues( i );
                this.add( i );
            }
        }

        if ( this.decorators != null ) {
            Enumeration enum1 = decorators.elements();
            Component d;

            while ( enum1.hasMoreElements() ) {
                d = ( Component ) enum1.nextElement();
                this.add( d );
            }
        }

        //this.tab = new Checkbox( this.name );
    }

}

; // end ControlPanel
