//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////
package dx.runtime;
/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/StringInteractor.java,v 1.2 2005/10/27 19:43:08 davidt Exp $
 */
import java.awt.*;

public class StringInteractor extends Interactor
{

        //
        // private member data
        //

        //
        // protected member data
        //

        protected String getTypeName()
        {
                return "String";
        }

        public int getWeightX()
        {
                return 1;
        }

        public void setBounds ( int x, int y, int w, int h )
        {
                if ( vertical == 1 )
                        super.setBounds ( x, y, w, 65 );
                else
                        super.setBounds ( x, y, w, h );
        }

        public int getVerticalFill()
        {
                return GridBagConstraints.BOTH;
        }

        public Insets getComponentInsets()
        {
                if ( vertical == 1 )
                        return new Insets ( 0, 10, 0, 10 );
                else
                        return new Insets ( 0, 0, 0, 10 );
        }


} // end class Interactor

