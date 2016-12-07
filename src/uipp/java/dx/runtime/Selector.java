//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/Selector.java,v 1.4 2006/03/26 23:50:23 davidt Exp $
 */
package dx.runtime;
import java.awt.*;
import dx.net.PacketIF;
import java.lang.reflect.*;
//
//
//
//
//

public class Selector extends BinaryInstance
{

    //
    // private member data
    //

    //
    // protected member data
    //
    protected boolean selected[];
    protected boolean singleItemSelectable()
    {
        return false;
    }

    public boolean action ( Event e, Object o )
    {
        if ( ( e.target == ipart ) && ( e.target instanceof Choice ) ) {
            PacketIF pif = getNode();
            pif.setOutputValues( this.getOutputValue(), this.getOutput2Value() );
            return true;
        }

        return false;
    }

    protected void createPart()
    {
        ipart = new Choice();
    }

    protected void newItem ( int i, String name )
    {
        Choice ch = ( Choice ) ipart;
        ch.addItem( name );
    }

    public void clearOptions()
    {
        Choice ch = ( Choice ) ipart;


        try {
            int length = ch.getItemCount();
            int i;

            for ( i = length - 1; i >= 0; i-- )
                ch.remove( i );
        }

        catch ( Exception nsme ) {
            nsme.printStackTrace();
            System.out.println
            ( "SelectorInteractor error: Your Java implementation is pre 1.1" );
            System.out.println
            ( "    Data-driven OptionMenu-style SelectorInteractors aren't available." );
            System.out.println
            ( "    Try using ScrolledList-style instead." );
        }
    }

    //
    //
    //
    protected String getTypeName()
    {
        return "Selector";
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
    // There is no way to implement this without DXType and DXValue
    //
    public void setValue( String s )
    {}

    public void selectOption( int i )
    {
        Choice ch = ( Choice ) ipart;
        ch.select( i - 1 );
    }


    public boolean isOptionSelected( int i )
    {
        int ndx = i - 1;
        Choice ch = ( Choice ) ipart;

        if ( ndx == ch.getSelectedIndex() )
            return true;
        else
            return false;
    }

    public void setEnabled( boolean tf )
    {
        super.setEnabled( tf );

        if ( this.isEnabled() == true ) {
            if ( ( this.getOptionCount() < 2 ) && ( this.singleItemSelectable() == false ) )
                setEnabled( false );
        }
    }

} // end class Selector

