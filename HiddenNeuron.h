/*
 *  HiddenNeuron.h
 *
 * Copyright 2018 OFTNAI. All rights reserved.
 *
 */

#ifndef HIDDENNEURON_H
#define HIDDENNEURON_H

// Forward declarations
class Param;
class Region;
class HiddenRegion;
class InputRegion;
class BinaryWrite;

// Includes
#include "Neuron.h"
#include "Synapse.h"
#include "Param.h"
#include <vector>
#include <gsl/gsl_randist.h>
#include <cfloat>
#include "Utilities.h"

using std::vector;

class HiddenNeuron: public Neuron {
	
	private:
        
        u_short desiredFanIn;
        float weightVectorLength;
    
        // History saving
        unsigned long neuronHistoryCounter;
        unsigned long synapseHistoryCounter;
		bool saveNeuronHistory;
        
        // History buffers
        float * activationHistory;
        float * inhibitedActivationHistory;
        float * firingRateHistory;
        float * traceHistory;
        float * stimulationHistory;
        float * effectiveTraceHistory;
    
        void output(BinaryWrite & file, const float * buffer);
    
        /////////////////////
        // Trace buffer
        /////////////////////
        int fixedBufferWeightHistorySize;
        int lastTraceBufferElement; // Round robin index
        float * fixedBufferTraceHistory;
    
    public:
    
        // temporarily moved
        bool saveSynapseHistory;
        
        // Data structures
        vector<Synapse> afferentSynapses;
        
        // Neuron State
        float activation;             // Normal weighted sum of input firing rates
        float inhibitedActivation;    // Activation after being passed through inhibit routine
        float trace;                  // Defines trace values for this neuron
        
        float effectiveTrace;		  // sigmoid(trace);
        float newActivation;
        float newInhibitedActivation;
        float newTrace;
    
        // inspection purposes
        float stimulation;            // Presynaptic stimulation, only used for inspection purposes.
        //float inhibition;             // total inhibition from neighboors

		int     timeStep;				   // dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 
		vector<float> myTraceHistory;	   // dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 	
        vector<float> mySquareDiffValue;   // dnavarro2016 convergenceTake2
        vector<float> myFiringRateHistory; // dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001)
    
		// Init
		void init(HiddenRegion * region, 
                  u_short depth, 
                  u_short row, 
                  u_short col, 
                  float * const activationHistory, 
                  float * const inhibitedActivationHistory, 
                  float * const firingRateHistory, 
                  float * const traceHistory,
                  float * const stimulationHistory,
                  float * const effectiveTraceHistory,
                  bool saveNeuronHistory, 
                  bool saveSynapseHistory,
                  u_short desiredFanIn,
                  float weightVectorLength,
                  int fixedBufferWeightHistorySize);
        
        // Destructor
        ~HiddenNeuron();
        
        void doTimeStep(bool save);
        void saveState();
        void clearState(bool resetTrace); // Does not clear history vectors, just state vars
		
        // Output data
        unsigned long int getTotalNumberAfferentSynapses(); // dnavarro2016 convergence
        void output(BinaryWrite & file, DATA data);
        
		// Setup network
        void setupAfferentSynapses(Region & preSynapticRegion, CONNECTIVITY connectivity, INITIALWEIGHT initialWeight, gsl_rng * rngController);    
        void samplePresynapticLocation(u_short preSynapticRegionDimension, u_short radius, gsl_rng * rngController, int & xSource, int & ySource);
        void addAfferentSynapse(const Neuron * preSynapticNeuron, float weight);
                                       
        // Synapse utils used when setting up connections
        bool areYouConnectedTo(const Neuron * n);
		void normalize();
		void normalize(float norm);
    
        
        float getDelayedTrace();
        void  addNewTraceValueToTraceBuffer();
		void  addMyDelayedTrace(float delayedTraceTime);							 // dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001)
		float getMyDelayedTrace(int timeStep, int delayedTraceTime);		         // dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001)
		float getMyDelayedFiringRate(int timeStep, int delayedTime);				 // dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
		void  addMyDelayedFiringRate(float delayedFiringRate);						 // dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001)
        void  addMySquareDiffValue(int timeStep, int delayedConvergenceTime, float currentWeight);        // dnavarro2016 convergenceTake2
        float getMyRMSConvergenceValue(int timeStep, int delayedTraceTime); // dnavarro2016 convergenceTake2
};

/*
*
* Are placed here because of inlining:
* Read note on: [9.6] How do you tell the compiler to make a non-member function inline?
* http://www.parashift.com/c++-faq-lite/inline-functions.html#faq-9.9
*
*/

#include <cfloat>
#include <cmath>

//////////////////// DEBUG
#include <math.h>
#include <iostream>

inline void HiddenNeuron::clearState(bool resetTrace) {
	
	firingRate = 0;
	newFiringRate = 0;
	oldFiringRate = 0;
	activation = 0;
    newActivation = 0;             
	inhibitedActivation = 0;
    newInhibitedActivation = 0;
    stimulation = 0;
	
	timeStep = 0;									// dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 
    myOldFiringRate = 0;							// dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
	
	
    //for(u_short s = 0;s < afferentSynapses.size();s++)
	//	afferentSynapses[s].resetBlockage();
    
    // Clear trace buffer
    for(int i=0;i < fixedBufferWeightHistorySize;i++)
        fixedBufferTraceHistory[i] = 0;
    
    // Reset buffer
    this->lastTraceBufferElement = 0; //  I dont think we need to do this really
	
    
	if(resetTrace) {
		trace = 0; 
		newTrace = 0;
		//oldTrace = 0;

		effectiveTrace = 0;
	}
}

// Housekeeping - switches old and new variables, and saves
// neuron states if saveState == true.
inline void HiddenNeuron::doTimeStep(bool save) {
    
	activation = newActivation;
	newActivation = FLT_MIN;
	
	inhibitedActivation = newInhibitedActivation;
	newInhibitedActivation = FLT_MIN;
	
	//oldFiringRate = firingRate;						// dnavarro2015 Implementing anti-Hebbian learning rule 11 (Rolls and Stringer, 2001) 
	firingRate = newFiringRate;
	newFiringRate = FLT_MIN;
	
	
	trace = newTrace;
	// n->newTrace MUST NOT BE RESET TO 0 since it is not always
	// recomputed on every time step (when p.trainAtTimeStepMultiple > 1 in discrete neurons),
    // which results that all learning is cancelled!

	timeStep++;										// dnavarro2015 Implementing anti-Hebbian learning rule 10 (Rolls and Stringer, 2001) 
	
	if(save)
		saveState();
}

inline void HiddenNeuron::saveState() {
    
    if(saveNeuronHistory) {
        
        activationHistory[neuronHistoryCounter] = activation;
        inhibitedActivationHistory[neuronHistoryCounter] = inhibitedActivation;
        firingRateHistory[neuronHistoryCounter] = firingRate;
        traceHistory[neuronHistoryCounter] = trace;
        stimulationHistory[neuronHistoryCounter] = stimulation;
        effectiveTraceHistory[neuronHistoryCounter] = effectiveTrace;
        neuronHistoryCounter++;
    }
    
    if(saveSynapseHistory) {
        
        for(u_short s = 0;s < afferentSynapses.size();s++)
            afferentSynapses[s].weightHistory[synapseHistoryCounter] = afferentSynapses[s].weight;
        
        synapseHistoryCounter++;
    }
}

inline void HiddenNeuron::normalize() {
	
	float norm = 0;
	
	for(u_short s = 0;s < afferentSynapses.size();s++)
		norm += afferentSynapses[s].weight * afferentSynapses[s].weight;
	
	normalize(norm);
}

// The reason we have this odd subroutine is because
// this is directly called during learning where norm
// is computed along with the weight update.
inline void HiddenNeuron::normalize(float norm) {
	
	norm = static_cast<float>(sqrt(norm));
	for(u_short s = 0;s < afferentSynapses.size();s++)
		afferentSynapses[s].weight *= weightVectorLength/norm;
}


#endif // HIDDENNEURON_H
