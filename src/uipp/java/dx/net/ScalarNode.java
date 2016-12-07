//

package dx.net;
import java.util.*;
import dx.runtime.*;

public class ScalarNode extends dx.net.InteractorNode
{

    private Vector values;
    private static final int ID_PARAM_NUM = 1; // Id used for UI messages
    private static final int DATA_PARAM_NUM = 2; // The input object
    private static final int CVAL_PARAM_NUM = 3; // Current Value
    private static final int REFRESH_PARAM_NUM = 4; // Does the module refresh
    private static final int MIN_PARAM_NUM = 5; // The current minimum
    private static final int MAX_PARAM_NUM = 6; // The current maximum
    private static final int INCR_PARAM_NUM = 7; // The current increment
    private static final int INCRMETHOD_PARAM_NUM = 8; // Method used to interpret incr.
    private static final int DECIMALS_PARAM_NUM = 9; // Number of decimals to display
    private static final int DFLTVAL_PARAM_NUM = 10; // The default output indicator
    private static final int LABEL_PARAM_NUM = 11; // The label


    public ScalarNode( Network net, String name, int instance, String notation )
    {
        super( net, name, instance, notation );
        this.values = new Vector( 2 );
    }

    public synchronized void setValues ( int i, int min, int max, int step, int current )
    {
        NumericValues nv = new NumericValues ( i, min, max, step, current );
        this.values.addElement( ( Object ) nv );
    }

    public synchronized void setValues ( int i, double min, double max, double step,
                                         int places, double current )
    {
        NumericValues nv = new NumericValues ( i, min, max, step, places, current );
        this.values.addElement( ( Object ) nv );
    }

    public void installValues ( Interactor i )
    {
        super.installValues( i );

        if ( this.values == null ) return ;

        Enumeration enum1 = this.values.elements();

        while ( enum1.hasMoreElements() ) {
            NumericValues nv = ( NumericValues ) enum1.nextElement();

            StepperInteractor si = ( StepperInteractor ) i;

            if ( nv.places == 0 ) {
                si.setValues( nv.which, ( int ) nv.min, ( int ) nv.max,
                              ( int ) nv.step, ( int ) nv.current );
            }

            else {
                si.setValues( nv.which, nv.min, nv.max, nv.step, nv.places, nv.current );
            }
        }
    }

    //
    // 0:  Scalar_%d: min=...
    // min=%g max=%g delta=%g value=%g decimals=%d
    // min=[%g %g %g] max=[%g %g %g] delta=[%g %g %g] value=[%g %g %g] decimals=[%g %g %g]
    //
    public boolean handleAssignment( String lhs, String rhs )
    {
        boolean handled = false;

        if ( lhs.equals( "min" ) ) {
            handled = true;
        }

        else if ( lhs.equals( "max" ) ) {
            handled = true;
        }

        else if ( lhs.equals( "delta" ) ) {
            handled = true;
        }

        else if ( lhs.equals( "value" ) ) {
            handled = true;
        }

        else if ( lhs.equals( "decimals" ) ) {
            handled = true;
        }

        if ( handled ) {
            try {
                int which = 1;
                StringTokenizer stok = new StringTokenizer( rhs, "[] " );

                while ( stok.hasMoreTokens() ) {
                    String tok = stok.nextToken();
                    NumericValues nv = ( NumericValues ) this.values.elementAt( which - 1 );
                    Double dv = new Double( tok );

                    if ( lhs.equals( "min" ) ) {
                        nv.min = ( nv.places == 0 ? dv.intValue() : dv.doubleValue() );
                    }

                    else if ( lhs.equals( "max" ) ) {
                        nv.max = ( nv.places == 0 ? dv.intValue() : dv.doubleValue() );
                    }

                    else if ( lhs.equals( "delta" ) ) {
                        nv.step = ( nv.places == 0 ? dv.intValue() : dv.doubleValue() );
                    }

                    else if ( lhs.equals( "value" ) ) {
                        nv.current = ( nv.places == 0 ? dv.intValue() : dv.doubleValue() );
                    }

                    else if ( lhs.equals( "decimals" ) ) {
                        nv.places = ( nv.places == 0 ? 0 : dv.intValue() );
                    }

                    else break;

                    which++;
                }
            }

            catch ( NullPointerException npe ) {
                System.out.println ( "ScalarNode.handleAssignment: " + lhs + "=" + rhs );
            }

            catch ( NumberFormatException nfe ) {
                System.out.println ( "ScalarNode.handleAssignment: " + lhs + "=" + rhs );
            }
        }

        if ( handled == false )
            return super.handleAssignment( lhs, rhs );

        return true;
    }

    protected void setOutputValue ( int param, String value )
    {
        if ( param != 1 ) {
            super.setOutputValue( param, value );
            return ;
        }

        Network net = this.getNetwork();
        DXLinkApplication dxa = ( DXLinkApplication ) net.getApplet();
        boolean resume = dxa.getEocMode();

        if ( resume ) dxa.setEocMode( false );

        super.setOutputValue( param, value );

        this.setInputValue ( ScalarNode.CVAL_PARAM_NUM, value );

        if ( resume ) dxa.setEocMode( true );
    }

    public void setInputValue ( int param, String value )
    {
        super.setInputValue( param, value );

        if ( param != ScalarNode.CVAL_PARAM_NUM ) return ;

        //
        // Insert the current value into the set of NumericValues for the Node
        // so that future data-driven messages don't clobber current value
        // in the interactor.
        //
        try {
            int which = 1;
            StringTokenizer stok = new StringTokenizer( value, "[] " );

            while ( stok.hasMoreTokens() ) {
                String tok = stok.nextToken();
                NumericValues nv = ( NumericValues ) this.values.elementAt( which - 1 );
                Double dv = new Double( tok );
                nv.current = ( nv.places == 0 ? dv.intValue() : dv.doubleValue() );
            }
        }

        catch ( NumberFormatException nfe ) {
            nfe.printStackTrace();
        }

        catch ( NullPointerException npe ) {
            npe.printStackTrace();
        }
    }

}

; // end ScalarNode
