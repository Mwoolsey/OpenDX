
package dx.net;
import java.awt.*;
import java.awt.event.*;
import java.util.Vector;

public class TabbedPanel extends Panel implements MouseListener
{
    protected Vector tabName = new Vector();
    protected Vector panels = new Vector();

    protected Panel cardPanel = new Panel();

    protected int[] tabWidths;
    protected String selectedPanel;

    protected Font tabFont = null;

    protected Insets insets;
    protected int tabHeight = 21;
    protected int tabWidthBuffer = 13;

    protected int insetPadding = 14;

    public TabbedPanel()
    {
        insets = new Insets( insetPadding + tabHeight, insetPadding,
                             insetPadding, insetPadding );
        setLayout( null );
        addMouseListener( this );

        cardPanel.setLayout( new CardLayout() );

        add ( cardPanel );

        tabFont = new Font( "Dialog", 12, Font.PLAIN );
    }

    //
    // MouseListener interface.
    //

    public void mousePressed( MouseEvent event )
    {
        if ( event.getY() < tabHeight ) {
            int xLoc = 0;

            for ( int i = 0; i < tabName.size(); i++ ) {
                xLoc += tabWidths[ i ];

                if ( event.getX() < xLoc ) {
                    setPanel( ( String ) tabName.elementAt( i ) );
                    return ;
                }
            }
        }
    }

    public void mouseReleased( MouseEvent event )
    {}

    public void mouseEntered( MouseEvent event )
    {}

    public void mouseExited( MouseEvent event )
    {}

    public void mouseClicked( MouseEvent event )
    {}

    //
    // Methods.
    //

    public Dimension getMinimumSize()
    {
        int w = 0;
        int h = 0;

        FontMetrics metrics = getFontMetrics( tabFont );

        for ( int i = 0; i < tabName.size(); i++ ) {
            String s = ( String ) tabName.elementAt( i );

	        if(metrics.stringWidth(s) == 0)
                w += 7 * s.length() + tabWidthBuffer;
            else {
                w += metrics.stringWidth( s ) + tabWidthBuffer;
            }
        }

        for ( int i = 0; i < panels.size(); i++ ) {
            Dimension d = ( ( Panel ) panels.elementAt( i ) ).getMinimumSize();
            w = Math.max( w, d.width + insets.left + insets.right );
            h = Math.max( h, d.height );
        }

        h += insets.top + insets.bottom;

        return new Dimension( w, h );
    }

    public Dimension getPreferredSize()
    {
        return getMinimumSize();
    }

    public void setBounds( int nx, int ny, int nw, int nh )
    {
        cardPanel.setBounds( insets.left,
                             insets.top,
                             nw - insets.left - insets.right,
                             nh - insets.top - insets.bottom );
        super.setBounds( nx, ny, nw, nh );
    }

    public void addPanel( Panel panel, String label )
    {
        tabName.addElement( label );
        panels.addElement( panel );
        cardPanel.add( label, panel );

        if ( panels.size() == 1 ) {
            selectedPanel = label;
        }
    }

    public void removePanel( String label )
    {
        int index = tabName.indexOf( label );

        if ( index == -1 ) {
            throw new IllegalArgumentException( label +
                                                " is not a panel in this TabbedPanel" );
        }

        tabName.removeElementAt( index );
        panels.removeElementAt( index );

        if ( label.equals( selectedPanel ) ) {
            if ( tabName.size() > 0 )
                setPanel( ( String ) tabName.elementAt( 0 ) );
            else
                selectedPanel = null;
        }
    }

    public void setPanel( String label )
    {
        if ( label.equals( selectedPanel ) ) {
            return ;
        }

        selectedPanel = label;
        ( ( CardLayout ) cardPanel.getLayout() ).show( cardPanel, label );

        repaint();
    }

    public String getSelectedPanelName()
    {
        return this.selectedPanel;
    }

    public Panel getSelectedPanel()
    {
        return ( Panel ) panels.elementAt( tabName.indexOf( selectedPanel ) );
    }

    public Insets getInsets()
    {
        return insets;
    }

    public void setInsets( Insets insets )
    {
        this.insets = insets;
    }

    public int getTabHeight()
    {
        return this.tabHeight;
    }

    public void setTabHeight( int tabHeight )
    {
        this.tabHeight = tabHeight;
    }

    public int getTabWidthBuffer()
    {
        return this.tabWidthBuffer;
    }

    public void setTabWidthBuffer( int tabWidthBuffer )
    {
        this.tabWidthBuffer = tabWidthBuffer;
    }

    public void paint( Graphics g )
    {
        super.paint( g );

        Color lb = Color.white;
        Color bg = getBackground();

        if ( bg.equals( lb ) )
            lb = new Color( 220, 220, 220 );

        int top = tabHeight - 1;
        int left = 0;
        int bottom = getSize().height - 1;
        int right = left + getSize().width - 1;
        g.setColor( Color.darkGray );
        g.drawLine( left, bottom, right - 1, bottom ); // Bottom left to bottom right
        g.drawLine( right, top, right, bottom );       // Topright to bottomright
        g.setColor( Color.gray );
        g.drawLine( left + 1, bottom - 1, right - 2, bottom - 1 ); // Bottom left to bottom right
        g.drawLine( right - 1, top + 1, right - 1, bottom - 1 );   // Topright to bottomright
        g.setColor( lb );
        g.drawLine( left, top, left, bottom - 1 ); // Topleft to bottomleft
        
        tabWidths = new int[ tabName.size() ];
        int selected = -1;
        int selectedLoc = 0;
        int xLoc = 2;
        FontMetrics metrics = null;

        metrics = g.getFontMetrics( tabFont );

        for ( int i = 0; i < tabName.size(); i++ ) {
            String label = ( String ) tabName.elementAt( i );
            int lw;

	        if(metrics.stringWidth( label ) == 0)
                lw = 7 * label.length();
            else {
                lw = metrics.stringWidth( label );
            }

            tabWidths[ i ] = lw + tabWidthBuffer;

            if ( tabName.elementAt( i ).equals( selectedPanel ) ) {
                selected = i;
                selectedLoc = xLoc;
            } else {
                paintTab( g, false, xLoc, 0, tabWidths[ i ], top, label );
            }
            xLoc += tabWidths[ i ] - 1;
        }

        if ( selected > -1 ) {
            paintTab( g, true, selectedLoc, 0, tabWidths[ selected ], top,
                      ( String ) tabName.elementAt( selected ) );
            g.setColor( lb );
            g.drawLine( left, top, selectedLoc - 2, top ); // Topleft to topright - left side
            g.drawLine( selectedLoc + tabWidths[ selected ] + 2, top, right - 1,
                        top );       // Topleft to topright - right side
        } else {
            g.setColor( lb );
            g.drawLine( left, top, right - 1, top ); // Topleft to topright
        }
    }

    private void paintTab( Graphics g, boolean selected, int x, int y, int
                           width, int height, String label )
    {
        Color lb = Color.white;
        Color bg = getBackground();

        if ( bg.equals( lb ) )
            lb = new Color( 220, 220, 220 );

        int left = x;
        int top = y + 2;
        int right = x + width - 1;
        int bottom = y + height - 1;
        height -= 2;

        if ( selected ) {
            top -= 2;
            left -= 2;
            right += 2;
            bottom += 1;
        }

        g.setColor( Color.darkGray );
        g.drawLine( right - 1, top + 2, right - 1, bottom ); // Topright to bottomright
        g.drawRect( right - 2, top + 1, 0, 0 );              // Topright corner

        g.setColor( Color.gray );
        g.drawLine( right - 2, top + 2, right - 2, bottom ); // Topright to bottomright

        g.setColor( lb );
        g.drawLine( left, top + 2, left, bottom );   // Topleft to bottomleft
        g.drawLine( left + 2, top, right - 3, top ); // Topleft to topright
        g.drawRect( left + 1, top + 1, 0, 0 );       // Topleft corner

        g.setColor( Color.black );

        FontMetrics metrics;
        int lw;
        int lh;

        metrics = g.getFontMetrics( tabFont );
        if(metrics.stringWidth( label ) != 0) {
            g.setFont( tabFont );
            lw = metrics.stringWidth( label );
            lh = metrics.getHeight();
        } else {
            lw = label.length() * 7;
            lh = 14;
        }

        g.drawString( label, x + ( ( width - lw ) / 2 ),
                      lh + top + 1 );
//        System.out.print( "drawing at: " );
//        System.out.println( x + ( ( width - lw ) / 2 ) );
//        System.out.println( bottom );
//        System.out.println( left );
//
//        if ( tabFont == null )
//	        System.out.println( "null tab font" );

        if ( selected ) {
            g.setColor( getBackground() );
            g.drawLine( left + 1, bottom, right - 3, bottom ); // Bottomleft to bottomright
            g.drawLine( left + 1, top + 3, left + 1, bottom ); // Topleft to bottomleft indented by one
        }
    }
}
