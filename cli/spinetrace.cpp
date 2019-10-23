//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Oct 16 21:44:03 PDT 2000
// Last Modified: Mon Oct 16 21:55:02 PDT 2000
// Filename:      ...sig/examples/all/spinetrace.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/spinetrace.cpp
// Syntax:        C++; museinfo
//
// Description:   Identifies data field spine memberships
//

#include <iostream>
#include <string>

#include "humdrum.h"

using namespace std;


// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      printSpineAnalysis (HumdrumFile& infile);
void      usage              (const string& command);

// global variables
Options   options;            // database for command-line arguments

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	checkOptions(options, argc, argv);
	HumdrumStream streamer(options);
	HumdrumFile infile;

	while (streamer.read(infile)) {
		infile.analyzeSpines();
		printSpineAnalysis(infile);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("debug=b");                // determine bad input line num
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
		cout << argv[0] << ", version: 3 May 2018" << endl;
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
// example -- example usage of the quality program
//

void example(void) {
	cout <<
	"                                                                         \n"
	"# example usage of the spinetrace program.                               \n"
	"     spinetrace chor217.krn                                              \n"
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// printSpineAnalysis --
//

void printSpineAnalysis(HumdrumFile& infile) {
	int linecount = infile.getNumLines();
	int linecount2 = 0;
	for (int i=0; i<linecount; i++) {
		if (infile[i].getType() != E_humrec_data) {
			cout << infile[i] << '\n';
		} else {
			linecount2 = infile.getSpineCount(i);
			for (int j=0; j<linecount2; j++) {
				cout << infile[i].getSpineInfo(j);
				if (j < linecount2 - 1) {
					cout << '\t';
				}
			}
			cout << '\n';
		}
	}
}



//////////////////////////////
//
// usage -- gives the usage statement for the meter program
//

void usage(const string& command) {
	cout <<
	"                                                                         \n"
	"Displays the spine memberships of humdrum data tokens.                   \n"
	"                                                                         \n"
	"Usage: " << command << " [input1 [input2 ...]]\n"
	"                                                                         \n"
	"Options:                                                                 \n"
	"   --options = list of all options, aliases and default values           \n"
	"                                                                         \n"
	<< endl;
}



