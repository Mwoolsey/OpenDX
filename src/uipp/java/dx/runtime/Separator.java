//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/Separator.java,v 1.2 2005/10/27 19:43:08 davidt Exp $
 */
package dx.runtime;
import java.awt.*;

public class Separator extends Canvas
{

    //
    // private member data
    //
    private int insetPadding = 5;


    //
    // protected member data
    //
    protected Insets insets;
    protected int horizontal = 1;
    protected Color local_color = null;
    protected Dimension size;
    int x = 0;
    int y = 0;

    public Separator()
    {
        insets = new Insets( insetPadding, insetPadding, insetPadding, insetPadding );
        size = new Dimension( 11, 11 );
    }

    public void setForeground( Color c )
    {
        super.setForeground( c );
        local_color = c;
    }

    //
    //
    //
    public void setVertical ()
    {
        horizontal = 0;
    }

    public void setInsets( Insets insets )
    {
        this.insets = insets;
    }

    public Insets getInsets()
    {
        return insets;
    }

    public void setHorizontal ()
    {
        horizontal = 1;
    }

    public void setSize( Dimension d )
    {
        int minHeight = insets.bottom + insets.top + 4;
        int minWidth = insets.left + insets.right + 4;
        size.height = ( d.height < minHeight ? minHeight : d.height );
        size.width = ( d.width < minWidth ? minWidth : d.width );
    }

    public void setSize( int w, int h )
    {
        setSize( new Dimension( w, h ) );
    }

    // Should be able to add a setBounds method. Look at what
    // paint has to do with setBounds
    public void setBounds( int x1, int y1, int w, int h )
    {
        x = x1;
        y = y1;
        setSize( new Dimension( w, h ) );
        Dimension d = getMinimumSize();
        super.setBounds( x, y, d.width, d.height );
    }

    public Dimension getMinimumSize()
    {
        int h;
        int w;

        h = Math.max( size.height, 4 + insets.bottom + insets.top );
        w = Math.max( size.width, 4 + insets.left + insets.right );
        return new Dimension( w, h );

    }

    public Dimension getPreferredSize()
    {
        return getMinimumSize();
    }

    public Dimension getMaximumSize()
    {
        return getMinimumSize();
    }

    public Dimension getSize()
    {
        return getMinimumSize();
    }

    //
    // Add shadowThickness
    //
    public void paint( Graphics g )
    {
        Color bg;

        if ( local_color == null )
            bg = getBackground();
        else
            bg = local_color;

        g.setColor( bg );
        int w = size.width - 1 - insets.left - insets.right;
        int h = size.height - 1 - insets.top - insets.bottom;

        if ( horizontal == 1 )
            h = 2;
        else
            w = 2;

        g.draw3DRect ( insets.left, insets.top, w, h, false );
    }


} // end class Separator

