//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////
package dx.runtime;
/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/StepperInteractor.java,v 1.2 2005/10/27 19:43:08 davidt Exp $
 */
import java.awt.*;
import dx.net.PacketIF;

public class StepperInteractor extends Interactor implements Notifier
{

        //
        //
        protected int stepper_count;
        protected boolean vector = false;

        //
        // Determining if our output type is int or float is a little funny
        // because regardless of the type of the actual data items, we may
        // treat the data as string data if we're outputing a list or a vector.
        //
        private boolean has_floats = false;
        private boolean has_ints = false;


        public void callCallback( Object src )
        {
                PacketIF pif = this.getNode();
                pif.setOutputValue ( this.getValue() );
        }

        public void setVector()
        {
                vector = true;
        }

        public void setValues ( int i, double min, double max, double step, int p, double current )
        {
                StepperGroup s = ( StepperGroup ) ipart;
                s.setValues( i, min, max, step, p, current );
                this.has_floats = true;
        }

        public void setValues ( int i, int min, int max, int step, int current )
        {
                StepperGroup s = ( StepperGroup ) ipart;
                s.setValues( i, min, max, step, current );
                this.has_ints = true;
        }

        public void setValue( String v )
        {}

        public String getValue()
        {
                StepperGroup s = ( StepperGroup ) ipart;
                return s.getValue();
        }

        protected String getTypeName()
        {
                return "Stepper";
        }

        public int getWeightX()
        {
                return 1;
        }

        public StepperInteractor( int count )
        {
                stepper_count = count;

                if ( count > 1 )
                        setVector();
        }

        public void createPart()
        {
                StepperGroup st = new StepperGroup( stepper_count, this, needArrows() );
                st.init();
                ipart = st;
        }

        public void setBounds ( int x, int y, int w, int h )
        {
                int required_height = 40 + ( stepper_count * 25 );

                if ( ( vertical == 1 ) && ( h < required_height ) ) {
                        super.setBounds ( x, y, w, required_height );
                }

                else {
                        super.setBounds ( x, y, w, h );
                }
        }

        protected boolean needArrows()
        {
                return true;
        }


} // end class Interactor

