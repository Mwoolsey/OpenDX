//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/ListSelector.java,v 1.3 2005/12/02 23:37:27 davidt Exp $
 */
package dx.runtime;
import java.awt.*;
import dx.net.PacketIF;
import java.awt.event.*;

public class ListSelector extends Selector implements ActionListener
{
        //
        // private member data
        //
        private boolean mode = false;
        
        //
        // protected member data
        //
        protected Color local_bg = null;

        public int getVerticalFill()
        {
                return GridBagConstraints.BOTH;
        }
        
        public void actionPerformed(ActionEvent ae) {
        	System.out.println("ListSelector.actionPerformed: ");
        	PacketIF pif = this.getNode();
            pif.setOutputValues ( this.getOutputValue(), this.getOutput2Value() );
        }


        //
        // Defaults to single selection list
        //
        protected void createPart()
        {
        	// The number of rows visible is not changeable once created.
        	// So we should use the bounds and try creating the list
        	// multiple times till we get one that is close to the
        	// size requested.
        		
                List ch = new List();

                if ( this.local_bg != null )
                        ch.setBackground( this.local_bg );
                
                ch.addActionListener(this);

                ipart = ch;

                if ( mode ) ch.setMultipleMode( true );
        }

        public void newItem( int i, String name )
        {
                List ch = ( List ) ipart;
                ch.add( name );
        }

        public void clearOptions()
        {
                List ch = ( List ) ipart;
                ch.removeAll();
        }

        public void setMode( boolean multi )
        {
                List ch = ( List ) ipart;

                if ( ch != null )
                        ch.setMultipleMode( multi );

                mode = multi;

                if ( multi == true ) {
                        int t = getOutputType();
                        t |= BinaryInstance.LIST;
                        setOutputType( t );
                }
        }

        public void selectOption( int i )
        {
                List ch = ( List ) ipart;
                ch.select( i - 1 );
        }

        public int getWeightY()
        {
                return 1;
        }

        public int getWeightX()
        {
                return 1;
        }

        public boolean isOptionSelected( int i )
        {
                int ndx = i - 1;
                List ch = ( List ) ipart;
                return ch.isIndexSelected( ndx );
        }

        public void setBackground( Color newc )
        {
                super.setBackground( newc );
                local_bg = newc;
                List ch = ( List ) ipart;

                if ( ch != null ) ch.setBackground( newc );
        }

        public Insets getComponentInsets()
        {
                if ( vertical == 1 )
                        return new Insets ( 10, 10, 16, 10 );
                else
                        return new Insets ( 10, 10, 10, 10 );
        }


} // end class Interactor

