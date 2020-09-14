/*
 *  Neuron.cpp
 *
 * Copyright 2018 OFTNAI. All rights reserved.
 *
 */

#include "Neuron.h"

void Neuron::init(Region * region, u_short depth, u_short row, u_short col) {

	this->region = region;
    this->depth = depth;
    this->row = row;
    this->col = col;
    this->firingRate = 0;
	this->newFiringRate = 0;
	this->myOldFiringRate = 0;  // dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
}
