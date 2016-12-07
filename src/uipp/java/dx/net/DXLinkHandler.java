//

/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/DXLinkHandler.java,v 1.3 1999/07/09 19:30:00 daniel Exp $
 */
package dx.net;

public interface DXLinkHandler {
    public void outputHandler(String key, String msg, Object data);
    public boolean hasHandler(String key);
};

