/*
 *  BinaryWrite.cpp
 *
 * Copyright 2018 OFTNAI. All rights reserved.
 *
 */

// Forward declarations

// Includes
#include "BinaryWrite.h"

BinaryWrite::BinaryWrite() : fstream() {}

BinaryWrite::BinaryWrite(const char * filename) : fstream() { openFile(filename); }

BinaryWrite::BinaryWrite(const string & filename) : fstream() { openFile(filename); }

void BinaryWrite::openFile(const string & filename) { openFile(filename.c_str()); }

void BinaryWrite::openFile(const char * filename) {
    
    this->filename = filename;
    
    exceptions( std::ios_base::failbit | std::ios_base::badbit); //exceptions(ios_base::eofbit | ios_base::failbit | ios_base::badbit);
    
    // Open file
	try {
        
		open(filename, std::ios_base::out | std::ios_base::binary);
        
	} catch (fstream::failure e) { 
        
		cerr << "Unable to open file for writing: error = " << strerror(errno) << ", file = " << filename << endl;
        cerr.flush();
		exit(EXIT_FAILURE);
	}
}
