//

package dx.net;
import java.util.*;
import dx.runtime.*;

public class SelectionNode extends dx.net.BinaryNode
{

        private Vector strings;
        protected Vector values;
        protected Vector selections;

        private final static int ID_PARAM_NUM = 1;
        private final static int CSTR_PARAM_NUM = 2;   // Current output string
        private final static int CVAL_PARAM_NUM = 3;   // Current output value
        private final static int STRLIST_PARAM_NUM = 4;   // group or stringlist
        private final static int VALLIST_PARAM_NUM = 5;   // integer or value list
        private final static int CULL_PARAM_NUM = 6;   // removes zero length strings
        private final static int LABEL_PARAM_NUM = 7;

        private final static int OUTPUT_VALUE = 1;
        private final static int OUTPUT_NAME = 2;

        public SelectionNode( Network net, String name, int instance, String notation )
        {
                super( net, name, instance, notation );
                this.selections = null;
                this.strings = this.values = null;
        }

        public void setValues ( Vector strings, Vector values )
        {
                this.strings = strings;
                this.values = values;
        }

        public void selectOption( int s )
        {
                Integer i = new Integer( s );

                if ( this.selections == null )
                        this.selections = new Vector( 4 );

                this.selections.addElement( i );
        }

        public void installValues ( Interactor i )
        {
                super.installValues( i );

                BinaryInstance bi = ( BinaryInstance ) i;

                if ( this.strings != null ) {
                        bi.setOptions( this.strings, this.values );

                        if ( this.selections != null ) {
                                Enumeration enum1 = this.selections.elements();

                                while ( enum1.hasMoreElements() ) {
                                        Integer I = ( Integer ) enum1.nextElement();
                                        bi.selectOption( I.intValue() );
                                }
                        }

                        bi.setEnabled( true );

                        //
                        // Since we've probably just been data driven, it's time
                        // to set our shadowing inputs.
                        //

                        if ( ( this.selections != null ) && ( this.selections.size() > 0 ) ) {
                                int type = bi.getOutputType();
                                String cstr = null;
                                String cval = null;

                                if ( this.selections.size() > 1 ) {
                                        cstr = "{ ";
                                        cval = "{ ";
                                        Enumeration enum1 = this.selections.elements();

                                        while ( enum1.hasMoreElements() ) {
                                                Integer s = ( Integer ) enum1.nextElement();
                                                int ind = s.intValue() - 1;
                                                cstr += "\"" + ( String ) this.strings.elementAt( ind ) + "\" ";

                                                if ( ( type & BinaryInstance.INTEGER ) != 0 )
                                                        cval += ( String ) this.values.elementAt( ind ) + " ";
                                                else
                                                        cval += "\"" + ( String ) this.values.elementAt( ind ) + "\" ";
                                        }

                                        cstr += " }";
                                        cval += " }";
                                }

                                else {
                                        Integer s = ( Integer ) this.selections.elementAt( 0 );
                                        int ind = s.intValue() - 1;
                                        cstr = "\"" + ( String ) this.strings.elementAt( ind ) + "\"";

                                        if ( ( type & BinaryInstance.INTEGER ) != 0 )
                                                cval = ( String ) this.values.elementAt( ind );
                                        else
                                                cval = "\"" + ( String ) this.values.elementAt( ind ) + "\"";
                                }

                                this.setInputValueQuietly ( SelectionNode.CSTR_PARAM_NUM, cstr );
                                this.setInputValueQuietly ( SelectionNode.CVAL_PARAM_NUM, cval );
                        }
                }

                else {
                        Vector t1 = new Vector( 2 );
                        Vector t2 = new Vector( 2 );
                        t1.addElement( "(no options)" );
                        t2.addElement( "NULL" );
                        bi.setOptions ( t1, t2 );
                        bi.setEnabled( false );
                }

        }

        protected String mungeString( String s )
        {
                StringBuffer sb = new StringBuffer( s );
                boolean changed = false;
                int ind = s.indexOf( "value list=" );

                if ( ind != -1 ) {
                        if ( sb.charAt( ind + 5 ) != ' ' )
                                System.out.println ( "SelectionNode.mungeString(1) error." + ind + ":" + s );

                        sb.setCharAt( ind + 5, '_' );

                        changed = true;
                }

                ind = s.indexOf( "string list=" );

                if ( ind != -1 ) {
                        if ( sb.charAt( ind + 6 ) != ' ' )
                                System.out.println ( "SelectionNode.mungeString(2) error." + ind + ":" + s );

                        sb.setCharAt( ind + 6, '_' );

                        changed = true;
                }

                if ( changed == false )
                        return super.mungeString( s );

                //
                // Now replace spaces which occur inside { } with %
                // Then let the superclass do its thing.
                // Then replace % which occur inside { } back into spaces
                //
                int length = sb.length();

                int i = 0;

                boolean inside = false;

                while ( i < length ) {
                        char c = sb.charAt( i );

                        if ( c == '{' ) {
                                inside = true;
                        }

                        else if ( c == '}' ) {
                                inside = false;
                        }

                        else if ( ( c == ' ' ) && ( inside ) )
                                sb.setCharAt( i, '%' );

                        i++;
                }

                String partially_munged = super.mungeString( new String( sb ) );
                sb = new StringBuffer( partially_munged );
                length = sb.length();
                i = 0;
                inside = false;

                while ( i < length ) {
                        char c = sb.charAt( i );

                        if ( c == '{' ) {
                                inside = true;
                        }

                        else if ( c == '}' ) {
                                inside = false;
                        }

                        else if ( ( c == '%' ) && ( inside ) )
                                sb.setCharAt( i, ' ' );

                        i++;
                }

                String munged = new String( sb );
                return munged;
        }

        protected Vector makeStrings( String rhs )
        {
                Vector names = new Vector( 8 );
                StringBuffer current_string = null;
                StringBuffer sb = new StringBuffer( rhs );
                int length = sb.length();
                int i;
                boolean in_string = false;

                for ( i = 0; i < length; i++ ) {
                        char c = sb.charAt( i );

                        if ( in_string == false ) {
                                if ( c == '"' )
                                        in_string = true;
                        }

                        else {
                                if ( c == '"' ) {
                                        if ( current_string == null )
                                                names.addElement( new String() );
                                        else
                                                names.addElement( new String( current_string ) );

                                        current_string = null;

                                        in_string = false;
                                }

                                else {
                                        if ( current_string == null )
                                                current_string = new StringBuffer( length );

                                        current_string.append( c );
                                }
                        }
                }

                return names;
        }

        protected Vector makeNumbers ( String rhs )
        {
                Vector values = new Vector( 8 );
                StringTokenizer stok = new StringTokenizer ( rhs, " {}" );

                while ( stok.hasMoreTokens() )
                        values.addElement( stok.nextToken() );

                return values;
        }

        //
        // 'value list={...}' or 'string list={...}' or 'index=%d'
        //
        public boolean handleAssignment( String lhs, String rhs )
        {
                boolean handled = false;

                if ( lhs.equals( "string_list" ) ) {
                        handled = true;
                        this.strings = this.makeStrings( rhs );
                }

                else if ( lhs.equals( "value_list" ) ) {
                        handled = true;
                        Interactor i = null;
                        int output_type = BinaryInstance.STRING;
                        this.selections = null;

                        if ( this.interactors != null ) {
                                i = ( Interactor ) this.interactors.elementAt( 0 );
                                BinaryInstance bi = ( BinaryInstance ) i;
                                output_type = bi.getOutputType();
                        }

                        int number = BinaryInstance.INTEGER | BinaryInstance.SCALAR;

                        if ( ( output_type & BinaryInstance.STRING ) != 0 ) {
                                this.values = this.makeStrings( rhs );
                        }

                        else if ( ( output_type & number ) != 0 ) {
                                this.values = this.makeNumbers( rhs );
                        }

                        else {
                                handled = false;
                                System.out.println
                                ( "Warning:  Your data-driven interactor received unparsable data" );
                                System.out.println ( "    : " + rhs );
                        }
                }

                else if ( lhs.equals( "index" ) ) {
                        // index=%d
                        // or
                        // index={ %d %d ... %d }
                        this.selections = null;

                        try {
                                StringTokenizer stok = new StringTokenizer( rhs, "{ }" );

                                while ( stok.hasMoreTokens() ) {
                                        String tok = stok.nextToken();
                                        Integer i = new Integer( tok );
                                        this.selectOption( i.intValue() + 1 );
                                }

                                handled = true;
                        }

                        catch ( NumberFormatException nfe ) {
                                System.out.println
                                ( "Warning:  Your data-driven interactor received an unparsable index" );
                                System.out.println ( "    : " + rhs );
                        }
                }

                if ( handled == false )
                        return super.handleAssignment( lhs, rhs );

                return true;
        }


        protected void setOutputValue ( int param, String value )
        {
                Network net = this.getNetwork();
                DXLinkApplication dxa = ( DXLinkApplication ) net.getApplet();
                boolean resume = dxa.getEocMode();

                if ( resume ) dxa.setEocMode( false );

                super.setOutputValue( param, value );

                int soparm = this.getShadowingInput( param );

                if ( soparm != -1 )
                        this.setInputValue( soparm, value );

                if ( resume ) dxa.setEocMode( true );

        }

        protected int getShadowingInput( int output )
        {
                switch ( output ) {
                case SelectionNode.OUTPUT_VALUE:
                        return SelectionNode.CVAL_PARAM_NUM;
                case SelectionNode.OUTPUT_NAME:
                        return SelectionNode.CSTR_PARAM_NUM;
                }

                return -1;
        }


}

; // end SelectionNode
