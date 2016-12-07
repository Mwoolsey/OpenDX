//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/ArrowButton.java,v 1.2 2005/10/27 19:43:07 davidt Exp $
 */
package dx.runtime;
import java.awt.*;

//
//

public class ArrowButton extends Canvas
{
        //
        // private
        //
        static private int OUT = 1;
        static private int IN = 2;
        private int direction;
        private int state = OUT;
        private Stepper stepper;

        static public int LEFT = 1;
        static public int RIGHT = 2;

        public ArrowButton( int dir, Stepper step )
        {
                direction = dir;
                stepper = step;
        }

        public boolean handleEvent( Event e )
        {
                if ( e.id == Event.MOUSE_DOWN ) {
                        state = IN;
                        repaint();
                        return true;
                }

                else if ( e.id == Event.MOUSE_UP ) {
                        if ( state == IN ) {
                                if ( changeValue() )
                                        stepper.callCallback( this );
                        }

                        state = OUT;
                        repaint();
                        return true;
                }

                return false;
        }

        private boolean changeValue()
        {
                if ( direction == LEFT )
                        return stepper.decrement();
                else
                        return stepper.increment();
        }

        public Dimension preferredSize()
        {
                return new Dimension ( 20, 20 );
        }

        public Dimension minimumSize()
        {
                return new Dimension ( 15, 15 );
        }

        public void paint( Graphics g )
        {
                Dimension d = getSize();
                Color ts = Color.white;
                Color bs = Color.black;
                Color last;

                int x1, x2, x3;
                int y1, y2, y3;

                if ( direction == LEFT ) {
                        x1 = 4;
                        x2 = d.width - 1;
                        y3 = d.height - 5;
                        x3 = x2;
                        y1 = d.height >> 1;
                        y2 = 4;
                        last = ( state == OUT ? bs : ts );
                }

                else {
                        x1 = 0;
                        x2 = d.width - 5;
                        y3 = d.height - 5;
                        x3 = 0;
                        y1 = 4;
                        y2 = d.height >> 1;
                        last = ( state == OUT ? ts : bs );
                }

                int xPoints[] = new int[ 3 ];
                xPoints[ 0 ] = x1;
                xPoints[ 1 ] = x2;
                xPoints[ 2 ] = x3;
                int yPoints[] = new int[ 3 ];
                yPoints[ 0 ] = y1;
                yPoints[ 1 ] = y2;
                yPoints[ 2 ] = y3;
                g.setColor( new Color( 0xbb, 0xbb, 0xbb ) );
                g.fillPolygon( xPoints, yPoints, 3 );

                g.setColor( ( state == OUT ? ts : bs ) );
                g.drawLine ( x1, y1, x2, y2 );
                g.setColor( ( state == OUT ? bs : ts ) );
                g.drawLine ( x2, y2, x3, y3 );
                g.setColor( last );
                g.drawLine ( x3, y3, x1, y1 );

        }

} // end ArrowButton
