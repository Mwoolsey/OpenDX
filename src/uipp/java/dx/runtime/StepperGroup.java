//////////////////////////////////////////////////////////////////////////////
//                       DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/StepperGroup.java,v 1.2 2005/10/27 19:43:08 davidt Exp $
 */
package dx.runtime;
import java.awt.*;

//
// Provide behavior similar to a Motif Label widget: support multi
// line labels which java labels don't seem to be able to do.
//

public class StepperGroup extends Panel
{
    //
    // private
    //
    int stepper_count;
    int next;
    Object[] stepper_array;
    Font local_font;
    Color local_fg;
    Color local_bg;
    boolean initialized;
    StepperInteractor notifier;
    boolean use_arrows = true;

    public StepperGroup( int count, StepperInteractor snot, boolean usem )
    {
        use_arrows = usem;
        stepper_count = count;
        stepper_array = new Object[ stepper_count ];
        next = 0;
        local_font = null;
        local_fg = null;
        local_bg = null;
        initialized = false;
        notifier = snot;
    }

    public StepperGroup( int count, StepperInteractor snot )
    {
        use_arrows = true;
        stepper_count = count;
        stepper_array = new Object[ stepper_count ];
        next = 0;
        local_font = null;
        local_fg = null;
        local_bg = null;
        initialized = false;
        notifier = snot;
    }

    public void setFont( Font newf )
    {
        if ( initialized == true ) {
            int i;

            for ( i = 0; i < next; i++ ) {
                Stepper lab = ( Stepper ) stepper_array[ i ];
                lab.setFont( newf );
            }
        }

        local_font = newf;
    }

    public void setBackground ( Color newc )
    {
        if ( initialized == true ) {
            int i;

            for ( i = 0; i < next; i++ ) {
                Stepper lab = ( Stepper ) stepper_array[ i ];
                lab.setBackground( newc );
            }
        }

        local_bg = newc;
        super.setBackground( newc );
    }

    public void setForeground ( Color newc )
    {
        if ( initialized == true ) {
            int i;

            for ( i = 0; i < next; i++ ) {
                Stepper lab = ( Stepper ) stepper_array[ i ];
                lab.setBackground( newc );
            }
        }

        local_fg = newc;
        super.setForeground( newc );
    }

    //
    //
    //
    public void init()
    {
        GridLayout gbl = new GridLayout( stepper_count, 1, 0, 0 );
        setLayout( gbl );

        try {
            int i;

            for ( i = 0; i < stepper_count; i++ ) {
                Stepper step = new Stepper( notifier );
                if ( local_fg != null ) step.setForeground( local_fg );
                if ( local_bg != null ) step.setBackground( local_bg );
                if ( local_font != null ) step.setFont ( local_font );
                if ( use_arrows == false ) step.setUseArrows( false );
                step.init();
                add ( step );
                stepper_array[ i ] = ( Object ) step;
            }
        }

        catch ( Exception e ) {
            e.printStackTrace();
        }

        initialized = true;
    }

    public void setValues ( int comp, double min, double max, double step, int p, double c )
    {
        Stepper s = ( Stepper ) stepper_array[ comp - 1 ];
        s.setValues ( min, max, step, p, c );
    }

    public void setValues ( int comp, int min, int max, int step, int current )
    {
        Stepper s = ( Stepper ) stepper_array[ comp - 1 ];
        s.setValues ( min, max, step, current );
    }

    public String getValue()
    {
        if ( stepper_count == 1 ) {
            Stepper s = ( Stepper ) stepper_array[ 0 ];
            return s.getValue();
        }

        String retval = "[ ";

        int i;

        for ( i = 0; i < stepper_count; i++ ) {
            Stepper s = ( Stepper ) stepper_array[ i ];
            retval = retval + " " + s.getValue();
        }

        retval = retval + " ]";
        return retval;
    }


} // end class Interactor

