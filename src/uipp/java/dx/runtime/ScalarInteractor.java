//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////
package dx.runtime;
/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/ScalarInteractor.java,v 1.1.1.1 1999/03/24 15:17:32 gda Exp $
 */
import java.awt.*;

public class ScalarInteractor extends StepperInteractor {

public ScalarInteractor(int count)
{
    super(1);
}


protected String getTypeName() 
{
    return "Scalar";
}


protected boolean needArrows()
{
    return false;
}



} // end class Interactor

