//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/Stepper.java,v 1.2 2005/10/27 19:43:08 davidt Exp $
 */
package dx.runtime;
import java.applet.Applet;
import java.awt.*;
import java.io.*;
import java.lang.*;
import java.net.*;


//
//

public class Stepper extends Panel implements Notifier
{
    //
    // private
    //
    private int decimal_places;
    private boolean integers = false;
    private double min = -1000000.0;
    private double max = 1000000.0;
    private double value = 0.0;
    private double step = 1.0;
    private boolean use_arrows = true;
    private ArrowButton left ;
    private ArrowButton right;
    private TextField txt;
    private Font local_font;
    private Color local_bg;
    private Color local_fg;
    private boolean initialized = false;
    private Notifier notifier = null;

    public void callCallback ( Object arrow )
    {
        if ( notifier != null )
            notifier.callCallback( this );
    }

    public void setValue( String v )
    {
        double tmp = value;

        try {
            if ( integers ) {
                value = Integer.valueOf( v ).intValue();
            }
            else {
                value = Float.valueOf( v ).floatValue();
            }
            setText( v );
        }
        catch ( NumberFormatException nfe ) {
            System.out.println ( "Illegal numeric entry: " + v );
            value = tmp;
            setText( getValue() );
        }
    }

    private void setText( String newstr )
    {
        StringBuffer sb = new StringBuffer( newstr );
        int len = sb.length();
        int i;
        boolean decimal_seen = false;
        boolean exp_seen = false;
        int decimal_digits = 0;

        for ( i = 0; i < len; i++ ) {
            char c;

            try {
                c = sb.charAt( i );
            }

            catch ( StringIndexOutOfBoundsException si ) {
                System.out.println ( "index " + i + " is longer than " + len );
                break;
            }

            if ( c == '.' ) {
                decimal_seen = true;
                continue;
            }

            if ( c == 'E' )
                exp_seen = true;

            if ( ( decimal_seen ) && ( c >= '0' ) && ( c <= '9' ) )
                decimal_digits++;

            if ( ( decimal_digits > decimal_places ) && ( exp_seen == false ) ) {
                if ( ( c >= '0' ) && ( c <= '9' ) )
                    sb.setCharAt ( i, '0' );
            }
        }

        String s = new String( sb );

        try {
            if ( integers ) value = Integer.valueOf( s ).intValue();
            else value = Float.valueOf( s ).floatValue();
        }

    catch ( NumberFormatException nfe ) {}

        Float fl = new Float( s );

        if ( integers )
            txt.setText( Integer.toString( fl.intValue() ) );
        else
            txt.setText( fl.toString() );
    }

    public Stepper( Notifier snot )
    {
        notifier = snot;
    }

    public void setValues ( double mn, double mx, double stp, int places, double current )
    {
        min = mn;
        max = mx;
        step = stp;
        value = current;
        integers = false;
        decimal_places = places;
        setText( "" + value );
    }

    public void setValues ( int mn, int mx, int stp, int current )
    {
        min = ( int ) mn;
        max = ( int ) mx;
        step = stp;
        value = ( int ) current;
        integers = true;
        decimal_places = 0;
        setText( "" + value );
    }

    public void setFont( Font newf )
    {
        if ( initialized == true ) {
            txt.setFont( newf );
        }

        local_font = newf;
    }

    public boolean decrement()
    {
        double tmp = value;
        value -= step;

        if ( value < min )
            value = min;

        setText ( getValue() );

        if ( value != tmp ) return true;
        else return false;
    }

    public boolean increment()
    {
        double tmp = value;
        value += step;

        if ( value > max )
            value = max;

        setText ( getValue() );

        if ( value != tmp ) return true;
        else return false;
    }

    public boolean action ( Event e, Object o )
    {
        if ( e.target == txt ) {
            String s = txt.getText();
            double tmp = value;

            try {
                Float f = new Float( s );

                if ( integers == false ) {
                    value = f.floatValue();

                    if ( ( value > max ) || ( value < min ) ) {
                        if ( value < min ) value = min;

                        if ( value > max ) value = max;

                        f = new Float( value );
                    }

                    setText ( f.toString() );
                }
                else {
                    Integer in = new Integer( f.intValue() );
                    value = in.intValue();

                    if ( ( value > max ) || ( value < min ) ) {
                        if ( value < min ) value = min;

                        if ( value > max ) value = max;

                        f = new Float( value );

                        in = new Integer( f.intValue() );
                    }

                    setText( in.toString() );
                }

                if ( ( notifier != null ) && ( value != tmp ) )
                    notifier.callCallback( this );
            }

            catch ( NumberFormatException nfe ) {
                value = tmp;
                System.out.println ( "Illegal numeric entry: " + s );
                setText( getValue() );
            }

            return true;
        }

        return false;
    }

    public String getValue()
    {
        Float f = new Float( value );

        if ( integers ) {
            Integer i = new Integer( f.intValue() );
            return i.toString();
        }

        else
            return f.toString();
    }

    public void setBackground ( Color newc )
    {
        if ( initialized == true ) {
            txt.setBackground( Color.white );

            if ( use_arrows ) {
                left.setBackground( newc );
                right.setBackground( newc );
            }
        }

        local_bg = newc;
        super.setBackground( newc );
    }

    public void setForeground ( Color newc )
    {
        if ( initialized == true ) {
            txt.setForeground( Color.black );

            if ( use_arrows ) {
                left.setForeground( newc );
                right.setForeground( newc );
            }
        }

        local_bg = newc;
        super.setForeground( newc );
    }

    public void setUseArrows( boolean usem )
    {
        if ( initialized == true ) return ;

        use_arrows = usem;
    }

    //
    //
    //
    public void init()
    {
        BorderLayout gbl = new BorderLayout( 4, 4 );
        setLayout( gbl );

        try {
            if ( use_arrows ) {
                left = new ArrowButton( ArrowButton.LEFT, this );

                add ( "West", left );

                right = new ArrowButton( ArrowButton.RIGHT, this );

                add ( "East", right );
            }

            txt = new TextField( "", 10 );
            add ( "Center", txt );
            txt.setBackground( Color.white );
            txt.setForeground( Color.black );

            if ( local_bg != null ) {
                if ( use_arrows ) {
                    left.setBackground( local_bg );
                    right.setBackground( local_bg );
                }
            }
            if ( local_fg != null ) {
                if ( use_arrows ) {
                    left.setForeground( local_fg );
                    right.setForeground( local_fg );
                }
            }
            if ( local_font != null )
                txt.setFont ( local_font );
        }
        catch ( Exception e ) {
            e.printStackTrace();
        }
        initialized = true;
    }



} // end class Stepper

