//

package dx.net;

public class NumericValues extends Object {
    int which;
    int places;
    double min, max, step, current;

    public NumericValues (int which, double min, double max, 
	double step, int places, double current) {
	super();
	this.which = which;
	this.min = min;
	this.max = max;
	this.step = step;
	this.current = current;
	this.places = places;
    }

    public NumericValues (int which, int min, int max, int step, int current) {
	super();
	this.which = which;
	this.min = min;
	this.max = max;
	this.step = step;
	this.current = current;
	this.places = 0;
    }
}; // end NumericValues


