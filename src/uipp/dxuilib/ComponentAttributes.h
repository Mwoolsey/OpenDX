/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ComponentAttributes_h
#define _ComponentAttributes_h

#include "Base.h"


#define ClassComponentAttributes "ComponentAttributes"

class ComponentAttributes : public Base {

	friend class ScalarNode;
	friend class ScalarInstance;
	friend class ScalarListInstance;

    private:

        boolean integerTyped; 
	double	currentValue; 
	double	minimum; 
	double	maximum; 
	int	decimal_places;
	double	delta; 

   protected: 

	void setDecimals(int places) { this->decimal_places = places; }
	void setDelta(double delta) { this->delta = delta; }
	void setIntegerType()       { this->integerTyped = TRUE; }
	void setRealType()          { this->integerTyped = FALSE; }
	void setMinimum(double min) 
			{ this->minimum = min;
			  if (min > this->maximum)
			  	this->maximum = min;
			  if (this->currentValue < min )
			  	this->currentValue = min;
			}

	void setMaximum(double max) 
			{ this->maximum = max;
			  if (max < this->minimum)
			  	this->minimum = max;
			  if (this->currentValue > max )
			  	this->currentValue = max;
			}
   	void setValue(double val)   
			{ if (val < this->minimum) 
				this->currentValue = this->minimum; 
			  else if (val > this->maximum) 
				this->currentValue = this->maximum; 
			  else
				this->currentValue = val; 
			}

   public:
	ComponentAttributes() 
		{ this->decimal_places = 0; 
		  this->currentValue = 0.0; 
		  this->maximum =  1000000.00; 
	  	  this->minimum = -1000000.00; 
	 	  this->delta   =  1.00; 
		  this->setRealType(); 
		}
	~ComponentAttributes() {}

 	boolean	isIntegerType()	{ return this->integerTyped; } 

	// boolean isContinuous() { return this->continuous == TRUE; } 

#define ROUND(x) \
	( (double) ((x > 0.0) ? (int)(x + .5) : (int)(x - .5) ) )


	double getValue()
	   { return (this->integerTyped ? 
			ROUND(this->currentValue) : this->currentValue); }
	double getDelta() 
	   { return (this->integerTyped ? 
			ROUND(this->delta) : this->delta); }
	double getMinimum() 
	   { return (this->integerTyped ? 
			ROUND(this->minimum) : this->minimum); }
	double getMaximum() 
	   { return (this->integerTyped ? 
			ROUND(this->maximum) : this->maximum); }

#undef ROUND
	int getDecimals() { return this->decimal_places; }

        ComponentAttributes *duplicate() 
	   { ComponentAttributes *ca = new ComponentAttributes;
             *ca = *this;
	     return ca;
           }


   const char *getClassName()
	{
		return ClassComponentAttributes;
	}
};

#endif	// _ComponentAttributes_h
