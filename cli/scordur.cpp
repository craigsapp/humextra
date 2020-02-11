//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Oct 23 19:44:36 PDT 2000
// Last Modified: Tue Feb 11 07:26:13 PST 2020 Added --total option
// Filename:      ...sig/examples/all/scordur.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/scordur.cpp
// Syntax:        C++; museinfo
//
// Description: measures the total length of a humdrum file in terms of
//     metrical beats.  If more than one data set in input file, then displays
//     the total beat duration of each set.
//


#include <string.h>

#include <iostream>

#include "humdrum.h"

using namespace std;


// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
double    displayResults     (HumdrumFile& hfile, int count, 
					               const string& filename);
void      example            (void);
void      usage              (const string& command);

// global variables:
Options   options;            // database for command-line arguments
int       numinputs;          // the total number of input files

//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
	HumdrumFile hfile;

	// process the command-line options
	checkOptions(options, argc, argv);

	// figure out the number of input files to process
	numinputs = options.getArgCount();
 
	string filename;
	double total = 0.0;
	int counter = 0;
	for (int i=0; i<numinputs || i==0; i++) {
		hfile.clear();

		// if no command-line arguments read data file from standard input
		if (numinputs < 1) {
			filename = "";
			hfile.read(cin);
		} else {
			filename = options.getArg(i+1);
			hfile.read(filename);
		}
	  
		hfile.analyzeRhythm();
		total += displayResults(hfile, numinputs, filename);
		counter++;
	}
	if (counter && options.getBoolean("total")) {
		cout << "Total:\t" << total << endl;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("t|total=b", "display total duration of all inputs");
	opts.define("author=b");               // author of program
	opts.define("version=b");              // compilation info
	opts.define("example=b");              // example usages
	opts.define("h|help=b");               // short description
	opts.process(argc, argv);
	
	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, October 2000" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 11 February 2020" << endl;
		cout << "compiled: " << __DATE__ << endl;
		cout << MUSEINFO_VERSION << endl;
		exit(0);
	} else if (opts.getBoolean("help")) {
		usage(opts.getCommand());
		exit(0);
	} else if (opts.getBoolean("example")) {
		example();
		exit(0);
	}
}
  


//////////////////////////////
//
// displayResults -- display the total rhythmic duration
//     of the file.
//

double displayResults(HumdrumFile& hfile, int count, const string& filename) {
	double output = 0.0;
	int maxline = hfile.getNumLines() - 1;
	if (maxline < 0) {
		return output;
	}
	output = hfile[maxline].getAbsBeat();
	cout << filename << ":\t";
	cout << output << "\n";
	return output;
}



//////////////////////////////
//
// example -- example usage of the quality program
//

void example(void) {
	cout <<
	"                                                                         \n"
	"# example usage of the scordur program.                                  \n"
	"     scordur chor217.krn                                                 \n"
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// usage -- gives the usage statement for the meter program
//

void usage(const string& command) {
	cout <<
	"                                                                         \n"
	"Measures the total length of a humdrum file in terms of                  \n"
	"metrical beats.  If more than one data set in input file, then displays  \n"
	"the total beat duration of each set.                                     \n"
	"                                                                         \n"
	"Usage: " << command << " [input1 [input2 ...]]                           \n"
	"                                                                         \n"
	"Options:                                                                 \n"
	"   --options = list of all options, aliases and default values           \n"
	"                                                                         \n"
	<< endl;
}



