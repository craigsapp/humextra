//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun May  2 23:56:35 PDT 2004
// Last Modified: Sun May  2 23:56:37 PDT 2004
// Last Modified: Thu Jun  3 19:54:09 PDT 2004 (added polyphonic feature)
// Last Modified: Fri Jun 25 00:17:43 PDT 2004 allowed for correcting two
//                                             or more ties in a row: [[ to [_]
// Last Modified: Tue Sep  7 01:39:07 PDT 2004 (allowed for basic splitting
//                                             into two subspines)
// Filename:      ...sig/examples/all/tiefix.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/tiefix.cpp
// Syntax:        C++; museinfo
//
// Description:   Fix tie errors in essen music (single voice parts only)
//                Will usually work with upto two subspines as well.
//                Currently ignores the secondary subspine.
//

#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>             /* for qsort and bsearch functions */
#include <string.h>
#include <ctype.h>

#include "humdrum.h"

using namespace std;

#define TIENONE  0
#define TIESTART 1
#define TIECONT  2
#define TIESTOP  3


///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
void      processFile        (HumdrumFile& infile, int index);
void      getPitchesAndTies  (HumdrumFile& infile, vector<int>& lines,
                              vector<int>& pitches, vector<int>& tiestates,
                              int index, vector<int>& localindex);
int       getTieCorrections  (vector<int>& tiecorrections, vector<int>& pitches,
                              vector<int>& tiestates);
void      printTieCorrection (ostream& out, HumdrumRecord& line, int tiecorrection,
                              int index);
void      printStringWithoutTie(ostream& out, const char* string);
void      printStringWithTie (ostream& out, const char* string, int correction);
void      printFileWithCorrections(ostream& out, HumdrumFile& infile,
                              vector<int>& lines, vector<int>& tiecorrections,
                              vector<int>& localindex);
// global variables
Options   options;            // database for command-line arguments
int       testQ = 0;          // used with -t option
string    filename;

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	HumdrumFile infile;

	// process the command-line options
	checkOptions(options, argc, argv);

	infile.clear();
	// if no command-line arguments read data file from standard input
	int numinputs = options.getArgCount();
	if (numinputs < 1) {
		infile.read(cin);
	} else {
		filename = options.getArg(1);
		infile.read(options.getArg(1));
	}
	int i;
	for (i=0; i<infile.getMaxTracks(); i++) {
		processFile(infile, i);
	}

	cout << infile;
}

///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile, int index) {
	vector<int> lines;
	vector<int> pitches;
	vector<int> tiestates;
	vector<int> tiecorrections;
	vector<int> localindex;

	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() == E_humrec_interpretation) {
			if (strcmp(infile[i].getExInterp(index), "**kern") != 0) {
				// spine is not **kern data, so no tie fixing necessary
				return;
			} else {
				// primary spine is **kern data
				break;
			}
		}
	}

// cout << "Spine is not kern data: " << infile[i].getExInterp(index) << endl;
// cout << "LINE IS: " << infile[i] << endl;
// cout << "INDEX = " << index << endl;
// cout << "LINE = " << i << endl;
// cout << "SPINE INFO = " << infile[i].getSpineInfo(index) << endl;

	getPitchesAndTies(infile, lines, pitches, tiestates, index, localindex);
	int corr = getTieCorrections(tiecorrections, pitches, tiestates);

	if (testQ) {
		if (corr) {
			cout << filename << ": " << corr << " incorrect ties\n";
			exit(0);
		} else {
			exit(0);
		}
	}

	if (corr) {
		stringstream datastream;
		printFileWithCorrections(datastream, infile, lines,
				tiecorrections, localindex);
		datastream << ends;
		infile.clear();
		infile.read(datastream);
	} else {
		// no corrections needed: do not alter infile
	}
}



//////////////////////////////
//
// printFileWithCorrections --
//

void printFileWithCorrections(ostream& out, HumdrumFile& infile,
		vector<int>& lines, vector<int>& tiecorrections, vector<int>& localindex) {
	int i;
	int lindex = 0;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_data) {
			out << infile[i] << "\n";
			continue;
		}
		if (lindex >= (int)lines.size()) {
			out << infile[i] << "\n";
			continue;
		}
		if (lines[lindex] == i) {
			if (tiecorrections[lindex]) {
				// print a tie correction
				printTieCorrection(out, infile[i], tiecorrections[lindex],
					localindex[lindex]);
			} else {
				out << infile[i] << "\n";
			}
			lindex++;
		} else {
			out << infile[i] << "\n";
		}
	}
}



//////////////////////////////
//
// printTieCorrection --
//

void printTieCorrection(ostream& out, HumdrumRecord& line, int tiecorrection, int index) {
	int i;
	for (i=0; i<line.getFieldCount(); i++) {
		if (i != index) {
			// not processing anything but the specified spine.
			out << line[i];
			if (i < line.getFieldCount() - 1) {
				out << "\t";
			}
			continue;
		}

		// need to print the corrected spine
		if (tiecorrection < 0) {
			printStringWithoutTie(out, line[index]);
		} else if (tiecorrection > 0) {
			printStringWithTie(out, line[index], tiecorrection);
		} else {
			out << line[index];
		}
		if (i < line.getFieldCount() - 1) {
			out << "\t";
		}
	}
	out << "\n";
}



//////////////////////////////
//
// printStringWithTie --
//

void printStringWithTie(ostream& out, const char* string, int correction) {
	int hasslurstart = 0;
	int hasslurstop = 0;
	if (strchr(string, '{') != NULL) {  // these are really phrase markers
		hasslurstart = 1;
	}
	if (strchr(string, '}') != NULL) {  // these are really phrase markers
		hasslurstop = 1;
	}
	int hasoldtie = 0;
	if (strchr(string, '[') != NULL) {
		hasoldtie = 1;
	}
	if (strchr(string, '_') != NULL) {
		hasoldtie = 2;
	}
	if (strchr(string, ']') != NULL) {
		hasoldtie = 3;
	}

	int i;
	int len = strlen(string);
	switch (correction) {
		case TIESTART:
			if (!hasslurstart) {
				out << "[" << string;
				return;
			}
			for (i=0; i<len; i++) {
				if (string[i] == '{') {
					out << "{[";
				} else {
					out << string[i];
				}
			}
			break;
		case TIECONT:
			if (!hasslurstop) {
				if (hasoldtie == TIESTOP) {
					for (i=0; i<len; i++) {
						if (string[i] == ']') {
							out << "_";
						} else {
							out << string[i];
						}
					}
				} else if (hasoldtie == TIESTART) {
					for (i=0; i<len; i++) {
						if (string[i] == '[') {
							// do nothing
						} else if (string[i] == '}') {
							// this case will not be reached
							out << "_}";
						} else {
							out << string[i];
						}
					}
					if (!hasslurstop) {
						out << "_";
					}
				} else {
					out << string << "_";
				}
				return;
			}
			for (i=0; i<len; i++) {
				if (string[i] == '}') {
					out << "_}"; // strange condition...
				} else {
					out << string[i];
				}
			}
			break;
		case TIESTOP:
			if (!hasslurstop) {
				out << string << "]";
				return;
			}
			for (i=0; i<len; i++) {
				if (string[i] == '}') {
					out << "]}";
				} else {
					out << string[i];
				}
			}
			break;
		default:
			out << string;
	}
}



//////////////////////////////
//
// printStringWithoutTie --
//

void printStringWithoutTie(ostream& out, const char* string) {
	int i;
	int len = strlen(string);
	for (i=0; i<len; i++) {
		if (string[i] == '[') continue;
		if (string[i] == '_') continue;
		if (string[i] == ']') continue;
		out << string[i];
	}
}



//////////////////////////////
//
// getTieCorrections -- returns true if there are any corrections to be made.
//

int getTieCorrections(vector<int>& tiecorrections, vector<int>& pitches,
		vector<int>& tiestates) {
	int output = 0;

	tiecorrections.resize(pitches.size());
	std::fill(tiecorrections.begin(), tiecorrections.end(), 0);
	for (int i=0; i<(int)pitches.size(); i++) {
		if (tiestates[i] == TIESTART) {
			if (i==(int)pitches.size()-1) {
				tiecorrections[i] = -tiestates[i];
				output += 1;
			} else {
				if ((pitches[i] == pitches[i+1]) && (tiestates[i+1] == TIENONE)) {
					tiecorrections[i+1] = TIESTOP;
					output += 1;
					tiestates[i+1] = TIESTOP;
				} else if (pitches[i] != pitches[i+1]) {
					tiecorrections[i] = -TIESTART;
					output += 1;
				}
			}
		} else if (tiestates[i] == TIECONT) {
			if (i==(int)pitches.size()-1) {
				tiecorrections[i] = -tiestates[i];
				output += 1;
			} else {
				if ((pitches[i] == pitches[i+1]) && (tiestates[i+1] == TIENONE)) {
					tiecorrections[i+1] = TIESTOP;
					output += 1;
					tiestates[i+1] = TIESTOP;
				} else if (pitches[i] != pitches[i+1]) {
					tiecorrections[i] = -TIESTART;
					output += 1;
				}
			}

		} else if (tiestates[i] == TIESTOP) {
			if (i==0) {
				tiecorrections[i] = tiestates[i];
				output += 1;
			} else {
				if ((pitches[i] == pitches[i-1]) && (tiestates[i-1] == TIENONE)) {
					tiecorrections[i-1] = TIESTART;
					output += 1;
					tiestates[i-1] = TIESTART;
				} else if (pitches[i] !=pitches[i-1]) {
					tiecorrections[i] = -TIESTOP;
					output += 1;
				} else if ((pitches[i] == pitches[i-1]) &&
						(tiestates[i-1] == TIESTOP)) {
					tiecorrections[i-1] = TIECONT;
					tiestates[i-1] = TIECONT;
					output += 1;
				}
			}
		}
	}

	// readjust the ties taking into account continuing ties
	// which might need to be added.
	for (int i=(int)pitches.size()-1; i>=2; i--) {
		if ((tiestates[i] == TIESTOP) &&
				(tiestates[i-1] == TIESTART) &&
				(tiestates[i-2] == TIESTART)) {
			tiecorrections[i-1] = TIECONT;
			tiestates[i-1] = TIECONT;
			output += 1;
		} else if ((tiestates[i] == TIECONT) &&
				(tiestates[i-1] == TIESTART) &&
				(tiestates[i-2] == TIESTART)) {
			tiecorrections[i-1] = TIECONT;
			tiestates[i-1] = TIECONT;
			output += 1;
		}
	}

	return output;
}



//////////////////////////////
//
// getPitchesAndTies --
//

void getPitchesAndTies(HumdrumFile& infile, vector<int>& lines,
		vector<int>& pitches, vector<int>& tiestates, int index,
		vector<int>& localindex) {
	int pitch;
	int tiestate;

	lines.reserve(infile.getNumLines());
	lines.resize(0);
	pitches.reserve(infile.getNumLines());
	pitches.resize(0);
	tiestates.reserve(infile.getNumLines());
	tiestates.resize(0);
	localindex.reserve(infile.getNumLines());
	localindex.resize(0);

	int pindex = index;   // the primary index

	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_data) {
			continue;
		}

		// find the local index
		index = pindex;
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (infile[i].getPrimaryTrack(j) == pindex + 1) {
				index = j;
				break;
			}
		}

		if (strcmp(infile[i][index], ".") == 0) {
			// ignore null tokens.
			continue;
		}
		if (strchr(infile[i][index], 'r') != NULL) {
			// ignore rests
			continue;
		}

		pitch = Convert::kernToBase40(infile[i][index]);
		if (strchr(infile[i][index], '[') != NULL) {
			tiestate = TIESTART;
		} else if (strchr(infile[i][index], '_') != NULL) {
			tiestate = TIECONT;
		} else if (strchr(infile[i][index], ']') != NULL) {
			tiestate = TIESTOP;
		} else {
			tiestate = TIENONE;
		}
		lines.push_back(i);
		pitches.push_back(pitch);
		tiestates.push_back(tiestate);
		localindex.push_back(index);
	}

}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("t|test=b", "check to see if there are tie errors in the file");
	opts.define("debug=b");           // determine bad input line num
	opts.define("author=b");          // author of program
	opts.define("version=b");         // compilation info
	opts.define("example=b");         // example usages
	opts.define("h|help=b");          // short description
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, May 2004" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 3 May 2018" << endl;
		cout << "compiled: " << __DATE__ << endl;
		cout << MUSEINFO_VERSION << endl;
		exit(0);
	} else if (opts.getBoolean("help")) {
		usage(opts.getCommand().data());
		exit(0);
	} else if (opts.getBoolean("example")) {
		example();
		exit(0);
	}

	testQ = opts.getBoolean("test");
}



//////////////////////////////
//
// example -- example usage of the program
//

void example(void) {
	cout <<
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// usage -- gives the usage statement for the program
//

void usage(const char* command) {
	cout <<
	"                                                                         \n"
	<< endl;
}



