/*
 *  HiddenNeuron.cpp
 *
 * Copyright 2018 OFTNAI. All rights reserved.
 *
 */

#include "HiddenNeuron.h"
#include "Param.h"
#include "Synapse.h"
#include "Region.h"
#include "HiddenRegion.h"
#include "InputRegion.h"
#include "InputNeuron.h"
#include "BinaryWrite.h"
#include <iostream>
#include <cstdlib>



using std::cout;
using std::endl;
using std::cerr;

void HiddenNeuron::init(HiddenRegion * region,
                        u_short depth, 
                        u_short row, 
                        u_short col, 
                        float * activationHistory, 
                        float * inhibitedActivationHistory, 
                        float * firingRateHistory, 
                        float * traceHistory, 
                        float * stimulationHistory,
                        float * effectiveTraceHistory,
                        bool saveNeuronHistory, 
                        bool saveSynapseHistory, 
                        u_short desiredFanIn,
                        float weightVectorLength,
                        int fixedBufferWeightHistorySize) {

    
	// Call base constructor
    Neuron::init(region, depth, row, col);
	
	// Set vars
    this->neuronHistoryCounter = 0;
    this->synapseHistoryCounter = 0;
	this->timeStep = 0;														// dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 
	this->saveNeuronHistory = saveNeuronHistory;
	this->saveSynapseHistory = saveSynapseHistory;
    this->desiredFanIn = desiredFanIn;
    this->weightVectorLength = weightVectorLength;

    
    // Setup buffer pointers
    this->activationHistory = activationHistory;
    this->inhibitedActivationHistory = inhibitedActivationHistory;
    this->firingRateHistory = firingRateHistory;
    this->traceHistory = traceHistory;
    this->effectiveTraceHistory = effectiveTraceHistory;
    this->stimulationHistory = stimulationHistory;
    
    // Allocate buffer space.
    this->fixedBufferWeightHistorySize = fixedBufferWeightHistorySize;
    this->fixedBufferTraceHistory = new float[fixedBufferWeightHistorySize];
	this->lastTraceBufferElement = 0; // init buffer indicator
	
    // Reserve, so only capacity changes, not size
    this->afferentSynapses.reserve(desiredFanIn);
    
	// Initialize all state variables to zero
	clearState(true);
}

HiddenNeuron::~HiddenNeuron() {
    afferentSynapses.clear();
}

void HiddenNeuron::addAfferentSynapse(const Neuron * preSynapticNeuron, float weight) {
    
    float * buffer;
    
    if (saveSynapseHistory) {
        
        if(desiredFanIn == afferentSynapses.size()) {
            
            cerr << "Attempting to add more synapses then there is space for in neuron buffer, blanknetwork does not match!" << endl;
            exit(EXIT_FAILURE);
        }
        
        // Ask region for history buffer
        //if(saveSynapseHistory) {
        //    
        //    cout << ">> Allocating for synapse buffer for neuron (" << row << "," << col << ")" << endl;
        //}
    
        buffer = (static_cast<HiddenRegion *>(region))->getSynapseHistorySlot();

    }
    else
        buffer = NULL;  
    
    // Add synapse to synapse lisr
    afferentSynapses.push_back(Synapse(weight, preSynapticNeuron, this, buffer));//, fixedBufferWeightHistorySize)); // historyLength
}

void HiddenNeuron::setupAfferentSynapses(Region & preSynapticRegion, CONNECTIVITY connectivity, INITIALWEIGHT initialWeight, gsl_rng * rngController) {
    
    if(connectivity == FULL) {
        
        for(int d = 0;d < preSynapticRegion.depth;d++)
            for(int i = 0;i < preSynapticRegion.verDimension;i++)
                for(int j = 0;j < preSynapticRegion.horDimension;j++) {
                    
                    float weight = initialWeight != ZERO ? static_cast<float>(gsl_rng_uniform(rngController)) : 0;
                    addAfferentSynapse(preSynapticRegion.getNeuron(d, i, j), weight);
                }
        
    } else if (connectivity == SPARSE) {
        
        for(int d = 0;d < preSynapticRegion.depth;d++) {
            
            u_short connectionsMade = 0;

            while(connectionsMade < desiredFanIn) {
                
                // Sample location
                unsigned long int rowSource = gsl_rng_uniform_int(rngController, preSynapticRegion.verDimension);
                unsigned long int colSource = gsl_rng_uniform_int(rngController, preSynapticRegion.horDimension);
                
                // Grab neuron
                Neuron * preSynapticNeuron = preSynapticRegion.getNeuron(d, rowSource, colSource);
                
                // Make sure we don't reconnect
                if(!areYouConnectedTo(preSynapticNeuron)) {
                    
                    float weight = initialWeight != ZERO ? static_cast<float>(gsl_rng_uniform(rngController)) : 0;
                    addAfferentSynapse(preSynapticNeuron, weight);
                    
                    connectionsMade++;
                }
            }
        }
    } else if (connectivity == SPARSE_BIASED) {
        
        cerr << "BIASED BIASED BIASED" << endl;
        
        u_short connectionsMade = 0;
        
        // Sample row
        unsigned long int rowSource = gsl_rng_uniform_int(rngController, preSynapticRegion.verDimension);
        // In the future just set int rowSourcMean to something, and use it to set int rowSource = Gauss(rowSourcMean) inside loop

        while(connectionsMade < desiredFanIn) {
            
            // Sample location
            unsigned long int colSource = gsl_rng_uniform_int(rngController, preSynapticRegion.horDimension);
            
            for(int d = 0;d < preSynapticRegion.depth;d++) {
                
                // Grab neuron
                Neuron * preSynapticNeuron = preSynapticRegion.getNeuron(d, rowSource, colSource);
                
                
                // Make sure we don't reconnect - NOT NECASsARY necessary
                //if(areYouConnectedTo(preSynapticNeuron)) {
                //    
                //    cerr << "Tried to reconnect, failure!" << endl;
                //    isTraining(isTraining_FAILURE);
                //}
                //
                
                float weight = initialWeight != ZERO ? static_cast<float>(gsl_rng_uniform(rngController)) : 0;
                addAfferentSynapse(preSynapticNeuron, weight);
                
                connectionsMade++;
            }
        }
    } else {

    	cerr << "Incorrect connectivity parameter!" << endl;
    	exit(EXIT_FAILURE);
    }

}

bool HiddenNeuron::areYouConnectedTo(const Neuron * n) {
    
    for(u_short s = 0;s < afferentSynapses.size();s++)
        if(afferentSynapses[s].preSynapticNeuron == n)
            return true;
    
    return false;
}

// dnavarro2016 convergence
unsigned long int HiddenNeuron::getTotalNumberAfferentSynapses() {
    return afferentSynapses.size();
    
}

void HiddenNeuron::output(BinaryWrite & file, DATA data) {
    
    if(data == FIRING_RATE)
    	output(file, firingRateHistory);
    else if(data == ACTIVATION)
    	output(file, activationHistory);
    else if(data == INHIBITED_ACTIVATION)
    	output(file, inhibitedActivationHistory);
    else if(data == TRACE)
    	output(file, traceHistory);
    else if(data == STIMULATION)
    	output(file, stimulationHistory);
    else if(data == EFFECTIVE_TRACE)
    	output(file, effectiveTraceHistory);
    else if(data == FAN_IN_COUNT)
        file << static_cast<u_short>(afferentSynapses.size());
    else if(data == WEIGHTS_FINAL) {
        
        // Iterate afferent synapses
    	for(std::vector<Synapse>::iterator s = afferentSynapses.begin(); s != afferentSynapses.end();s++) {
            
            const Neuron * n = (*s).preSynapticNeuron;
            file << n->region->regionNr << n->depth << n->row << n->col << (*s).weight;
        }
        
    } else if(data == WEIGHT_HISTORY) {

        // Iterate afferent synapses
        for(std::vector<Synapse>::iterator s = afferentSynapses.begin(); s != afferentSynapses.end();s++)  {

        	// Output presynaptic neuron description
        	const Neuron * n = (*s).preSynapticNeuron;
        	file << n->region->regionNr << n->depth << n->row << n->col;

            // Output weight history for this synapse
            for(unsigned long t = 0;t < synapseHistoryCounter;t++)
                file << (*s).weightHistory[t];
        }

    } else if(data == WEIGHT_AND_NEURON_HISTORY) {
        
        // Output neuron description
        file << region->regionNr << depth << row << col << static_cast<u_short>(afferentSynapses.size());
            
        // Output neuron history
        output(file, firingRateHistory);
        output(file, activationHistory);
        output(file, inhibitedActivationHistory);
        output(file, traceHistory);
        output(file, stimulationHistory);
        output(file, effectiveTraceHistory);
        
        // Dump synapse descriptins afferent synapses
        for(std::vector<Synapse>::iterator s = afferentSynapses.begin(); s != afferentSynapses.end();s++) {
            
            const Neuron * n = (*s).preSynapticNeuron;
        	file << n->region->regionNr << n->depth << n->row << n->col; // region, depth, row, col
        }

        // Dump afferent synapses histories
        for(std::vector<Synapse>::iterator s = afferentSynapses.begin(); s != afferentSynapses.end();s++)  {
            
            // Output weight history for this synapse
            for(unsigned long t = 0;t < synapseHistoryCounter;t++)
                file << (*s).weightHistory[t];
        }
    }
}

void HiddenNeuron::output(BinaryWrite & file, const float * buffer) {
    
    for(unsigned long t = 0;t < neuronHistoryCounter;t++)
        file << buffer[t];
    
}

//////////////////////////////////////////////////////////////////////////
// Trace buffer
//////////////////////////////////////////////////////////////////////////

 float HiddenNeuron::getDelayedTrace() {
     
     // The elemnt in the buffer directly behind (lower index, with wrap arround) the oldest, is the newest
     return fixedBufferTraceHistory[lastTraceBufferElement];
 }
 
 void HiddenNeuron::addNewTraceValueToTraceBuffer() {
 
     // Save new addition in the position of the oldest
     fixedBufferTraceHistory[lastTraceBufferElement] = newTrace;
 
     // move position of the oldest along
     lastTraceBufferElement = (lastTraceBufferElement == fixedBufferWeightHistorySize-1) ? 0 : lastTraceBufferElement+1;
 }

// dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
void HiddenNeuron::addMyDelayedFiringRate(float delayedFiringRate) {
	myFiringRateHistory.push_back(delayedFiringRate);
}

// dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
float HiddenNeuron::getMyDelayedFiringRate(int timeStep, int delayedTime) {
	/*if (delayedTraceTime > myTraceHistory.size()) { // not likely to get this exception
	 cerr << "Array Index Out of Bounds Exception:\n Required " << delayedTraceTime << " but we only have: " << myTraceHistory.size();
	 exit(EXIT_FAILURE);
	 }*/
	
	if ((timeStep - delayedTime) < 0) {
		return 0; // if there is still no trace time for the current neuron, then the trace value is zero
	}
	
	return myFiringRateHistory.at((timeStep - delayedTime)); //(delayedTraceTime >= 0 ? myTraceHistory[delayedTraceTime] : 0);
}


// dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 
/**
 * TODO indexing vector by 'n', include trace time value, test it!! ---> define "neuronID"
 */
void HiddenNeuron::addMyDelayedTrace(float delayedTraceTime) {
	myTraceHistory.push_back(delayedTraceTime);
}

// dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 
/**
 * TODO Handle vector indexed by 'n' with trace history vectors
 */
float HiddenNeuron::getMyDelayedTrace(int timeStep, int delayedTraceTime) {

	
	if ((timeStep - delayedTraceTime) < 0) {
		return 0; // if there is still no trace value for the current neuron at the present time, then the trace value is zero
	}
	
	return myTraceHistory.at((timeStep - delayedTraceTime)); //(delayedTraceTime >= 0 ? myTraceHistory[delayedTraceTime] : 0);
}

// dnavarro2016 convergenceTake2
void HiddenNeuron::addMySquareDiffValue(int timeStep, int delayedConvergenceTime, float currenWeight) {
    float auxConvValue;
   // if (((timeStep - delayedConvergenceTime) >= 0)) { // if we already do have some synaptic weight to compare with, then
        auxConvValue = getMyDelayedTrace(timeStep, delayedConvergenceTime);
        mySquareDiffValue.push_back((auxConvValue-currenWeight)*(auxConvValue-currenWeight));
   // }
}

float HiddenNeuron::getMyRMSConvergenceValue(int timeStep, int delayedTraceTime) {
    float rms = 0;
    for (std::vector<float>::iterator it = mySquareDiffValue.begin() ; it != mySquareDiffValue.end(); ++it) {
            rms += mySquareDiffValue.at(*it);
    }
    //return rms/mySquareDiffValue.size(); //(delayedTraceTime >= 0 ? myTraceHistory[delayedTraceTime] : 0);
    return mySquareDiffValue.at(timeStep);
}
