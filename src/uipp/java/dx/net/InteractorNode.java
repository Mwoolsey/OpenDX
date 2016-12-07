//

package dx.net;
import java.util.*;
import dx.runtime.*;

public abstract class InteractorNode extends dx.net.DrivenNode
{

        private Vector cfgValues;
        protected Vector interactors;

        public InteractorNode( Network net, String name, int instance, String notation )
        {
                super( net, name, instance, notation );
                this.interactors = null;
        }

        public void addInteractor( Interactor i )
        {
                if ( this.interactors == null )
                        this.interactors = new Vector( 2 );

                this.interactors.addElement( ( Object ) i );

                i.setNode( this );
        }

        public void setBounds ( Interactor i, int x, int y, int width, int height )
        {
                if ( this.cfgValues == null )
                        this.cfgValues = new Vector( 2 );

                CfgValues cfg = new CfgValues ( i, x, y, width, height );

                this.cfgValues.addElement( ( Object ) cfg );
        }

        public void installValues ( Interactor i )
        {
                if ( this.cfgValues == null ) return ;

                Enumeration enum1 = this.cfgValues.elements();

                while ( enum1.hasMoreElements() ) {
                        CfgValues cfg = ( CfgValues ) enum1.nextElement();

                        if ( i != cfg.interactor ) continue;

                        i.setBounds( cfg.x, cfg.y, cfg.width, cfg.height );
                }
        }

        protected boolean needsSpacesReplaced()
        {
                return true;
        }

        public void handleMessage ( String msg )
        {
                super.handleMessage( msg );
                this.cfgValues = null;

                if ( this.interactors == null )
                        return ;

                Enumeration enum1 = this.interactors.elements();

                while ( enum1.hasMoreElements() ) {
                        Interactor i = ( Interactor ) enum1.nextElement();
                        this.installValues( i );
                }
        }

}

; // end InteractorNode
