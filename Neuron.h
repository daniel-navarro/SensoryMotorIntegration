/*
 *  Neuron.h
 *
 * Copyright 2018 OFTNAI. All rights reserved.
 *
 */

#ifndef NEURON_H
#define NEURON_H

// Forward declarations
class Region;
class Synapse;

// Includes
#include <vector>
#include "Utilities.h"

using std::vector;

class Neuron {

    public:
        
        // Z,X,Y location of cell in region
        u_short depth, row, col;
        
        // Containing region
        // Cant use referenes because of insane copy constructability of vector class
        // u_short short regionNr;
        Region * region;
        
        float firingRate;     // For postsynaptic activation computation
		float newFiringRate;  // The reason this value was introduced in Neuron class was because we may run model in discrete scheme, which depends new* values
		float oldFiringRate;			  // dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001)  --> is it really mine? :)
		float myOldFiringRate;			  // dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
	
		// Init
        void init(Region * region, u_short depth, u_short row, u_short col);
                
    private:
        
        //vector<const Synapse *> efferentLink;
};

#endif // NEURON_H
