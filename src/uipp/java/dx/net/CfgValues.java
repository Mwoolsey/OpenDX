//


/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/CfgValues.java,v 1.3 1999/07/09 19:29:59 daniel Exp $
 */

package dx.net;

public class CfgValues extends Object {
    Object interactor;
    int x,y,width,height;

    public CfgValues (Object ntr, int x, int y, int width, int height) {
	super();
	this.interactor = ntr;
	this.x = x;
	this.y = y;
	this.width = width;
	this.height = height;
    }
}; // end CfgValues


