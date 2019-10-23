//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Feb 15 17:09:16 PST 2011
// Last Modified: Tue Feb 15 17:09:23 PST 2011
// Filename:      ...sig/examples/all/fixdyna.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/autodynam.cpp
// Syntax:        C++; museinfo
//
// Description:   Fix crescendo and decrescendo symbols (wedges, hairpins).
//

#include <string>
#include <vector>

#include "humdrum.h"
#include "PerlRegularExpression.h"

using namespace std;


class Coord {
	public:
		int i, j;
				Coord(void) { }
			  ~Coord()     { }
		void  clear(void) { i = j = -1; }
};

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const string& command);
void      processFile        (HumdrumFile& infile);
void      processDynamicSpine(HumdrumFile& infile, int track);
void      storeDynamSpine    (vector<vector<Coord> >& data, HumdrumFile& infile,
							         int track);


// global variables
Options   options;            // database for command-line arguments
int       debugQ = 0;         // used with --debug option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	checkOptions(options, argc, argv);
	HumdrumStream streamer(options);
	HumdrumFile infile;

	while (streamer.read(infile)) {
		processFile(infile);
		cout << infile;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
	int i, j;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**dynam")) {
				continue;
			}
			processDynamicSpine(infile, infile[i].getPrimaryTrack(j));
		}
		break;  // just procesing the first exclusive interpretation line.
	}
}



//////////////////////////////
//
// processDynamicSpine -- Only looking at first layer of spine.
//

void processDynamicSpine(HumdrumFile& infile, int track) {
	vector<vector<Coord> > data;
	storeDynamSpine(data, infile, track);
	int i;
	int ii, jj;
	int crescX       = 0;  // interpreted crescendo (cresc. word)
	int decrescX     = 0;  // interpreted decrescendo (decresc. word)
	int crescy       = 0;  // hidden crescendo
	int decrescy     = 0;  // hidden decrescendo
	int crescState   = 0;  // active crescendo
	int decrescState = 0;  // active decrescendo
	char buffer[1024] = {0};
	string buf;
	PerlRegularExpression pre;

	int xx, yy;

	if (debugQ) {
		cerr << "DATA SIZE = " << data.size() << endl;
	}

	for (i=0; i<(int)data.size(); i++) {
		ii = data[i][0].i;
		jj = data[i][0].j;

		if (debugQ) {
			cerr << "PROCESSING I = " << i << " " << infile[ii][jj] << endl;
		}
		if (crescState) {
			if (debugQ) {
				cerr << "\tACTIVE CRESC" << endl;
			}
			// if currently a crescendo, then either continue or end
			if (strcmp(infile[ii][jj], ".") == 0) {
				// convert "." to "(" to continue the crescendo
				if (debugQ) {
					cerr << "\tChanging . to (" << endl;
				}
				strcpy(buffer, "(");
				if (crescX) {
					strcat(buffer, "X");
				}
				if (crescy) {
					strcat(buffer, "yy");
				}
				infile[ii].changeField(jj, buffer);
			} else {
				// end the crescendo
				xx = data[i-1][0].i;
				yy = data[i-1][0].j;
				if (debugQ) {
					cerr << "GOT HERE " << infile[xx][yy] << endl;
				}
				if ((xx < 0) || (yy < 0)) {
					continue;
				}
				// this is a dynamic object of some sort so terminate
				// the crescendo on the previous item
				if (strchr(infile[xx][yy], '(') != NULL) {
					if (debugQ) {
						cerr << "GOT HERE " << infile[xx][yy] << endl;
					}
					// replace the '(' found on the previous data line
					buf = infile[xx][yy];
					pre.sar(buf, "\\(", "[", "");  // should only apply to last
					if (crescX) {
						pre.sar(buf, "\\[", "[", "");
						crescX = 0;
					}
					if (crescy) {
						pre.sar(buf, "\\[", "[", "");
						crescy = 0;
					}
					infile[xx].changeField(yy, buf.c_str());
					crescState = 0;
					if (debugQ) {
						cerr << "DEACTIVATING CRESC 1" << endl;
					}
				} else {
					// there is something other than ( on the last line:
					// append " ["
					strcpy(buffer, infile[xx][yy]);
					strcat(buffer, " [");
					//if (crescy) {
					//   strcat(buffer, "yy");
					//   crescy = 0;
					//} if (crescX) {
					//   strcat(buffer, "X");
					//   crescX = 0;
					//}
					infile[xx].changeField(yy, buffer);
					crescState = 0;
					if (debugQ) {
						cerr << "DEACTIVATING CRESC 2" << endl;
					}
				}
			}

		} else if (decrescState) {

			// if currently a decrescendo, then either continue or end
			if (strcmp(infile[ii][jj], ".") == 0) {
				// convert "." to "(" to continue the decrescendo
				strcpy(buffer, ")");
				if (decrescX) {
					strcat(buffer, "X");
				}
				if (decrescy) {
					strcat(buffer, "yy");
				}
				infile[ii].changeField(jj, buffer);
			} else {
				// end the decrescendo
				xx = data[i-1][0].i;
				yy = data[i-1][0].j;
				if ((xx < 0) || (yy < 0)) {
					continue;
				}
				// this is a dynamic object of some sort so terminate
				// the decrescendo on the previous item
				if (strchr(infile[xx][yy], ')') != NULL) {
					// replace the '(' found on the previous data line
					buf = infile[xx][yy];
					pre.sar(buf, "\\)", "]", "");  // should only apply to last
					//if (decrescX) {
					//   pre.sar(buf, "\\]", "]X", "");
					//   decrescX = 0;
					//}
					//if (decrescy) {
					//   pre.sar(buf, "\\]", "]y", "");
					//   decrescy = 0;
					//}
					infile[xx].changeField(yy, buf.c_str());
					decrescState = 0;
				} else {
					// there is something other than ( on the last line:
					// append " ["
					strcpy(buffer, infile[xx][yy]);
					strcat(buffer, " ]");
					if (decrescy) {
						strcat(buffer, "yy");
						decrescy = 0;
					} if (decrescX) {
						strcat(buffer, "X");
						decrescX = 0;
					}
					infile[xx].changeField(yy, buffer);
					decrescState = 0;
				}
			}

		}

		// search for < or > marker to initiate de/cresc.
		if (strchr(infile[ii][jj], '<')  != NULL) { crescState   = 1; }
		if (strchr(infile[ii][jj], '>')  != NULL) { decrescState = 1; }
		if (strstr(infile[ii][jj], "<X") != NULL) { crescX       = 1; }
		if (strstr(infile[ii][jj], ">X") != NULL) { decrescX     = 1; }
		if (strstr(infile[ii][jj], "<yy") != NULL) { crescy      = 1; }
		if (strstr(infile[ii][jj], ">yy") != NULL) { decrescy    = 1; }
	}


	// handle case where cresc/decresc ends data with nothing after it.
	if (crescState) {

		// end the crescendo
		xx = data.back()[0].i;
		yy = data.back()[0].j;
		if (debugQ) {
			cerr << "GOT HERE " << infile[xx][yy] << endl;
		}
		if ((xx < 0) || (yy < 0)) {
			return;
		}
		// this is a dynamic object of some sort so terminate
		// the crescendo on the previous item
		if (strchr(infile[xx][yy], '(') != NULL) {
			if (debugQ) {
				cerr << "GOT HERE " << infile[xx][yy] << endl;
			}
			// replace the '(' found on the previous data line
			buf = infile[xx][yy];
			pre.sar(buf, "\\(", "[", "");  // should only apply to last
			if (crescX) {
				pre.sar(buf, "\\[", "[", "");
				crescX = 0;
			}
			if (crescy) {
				pre.sar(buf, "\\[", "[", "");
				crescy = 0;
			}
			infile[xx].changeField(yy, buf.c_str());
			crescState = 0;
			if (debugQ) {
				cerr << "DEACTIVATING CRESC 1" << endl;
			}
		}

	} else if (decrescState) {

		// end the decrescendo
		xx = data.back()[0].i;
		yy = data.back()[0].j;
		if (debugQ) {
			cerr << "GOT HERE " << infile[xx][yy] << endl;
		}
		if ((xx < 0) || (yy < 0)) {
			return;
		}
		// this is a dynamic object of some sort so terminate
		// the decrescendo on the previous item
		if (strchr(infile[xx][yy], ')') != NULL) {
			if (debugQ) {
				cerr << "GOT HERE " << infile[xx][yy] << endl;
			}
			// replace the '(' found on the previous data line
			buf = infile[xx][yy];
			pre.sar(buf, "\\)", "]", "");  // should only apply to last
			if (decrescX) {
				pre.sar(buf, "\\]", "]", "");
				decrescX = 0;
			}
			if (decrescy) {
				pre.sar(buf, "\\]", "]", "");
				decrescy = 0;
			}
			infile[xx].changeField(yy, buf.c_str());
			decrescState = 0;
			if (debugQ) {
				cerr << "DEACTIVATING DECRESC 1" << endl;
			}
		}

	}

}



//////////////////////////////
//
// storeDynamSpine -- make a list of the data spines for the given track
//

void storeDynamSpine(vector<vector<Coord> >& data, HumdrumFile& infile,
		int track) {
	data.reserve(infile.getNumLines());
	data.resize(0);
	Coord coord;

	int i, j;
	for (i=0; i<infile.getNumLines(); i++){
		if (!infile[i].isData()) {
			continue;
		}
		data.resize(data.size()+1);
		data.back().resize(0);
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (track != infile[i].getPrimaryTrack(j)) {
				continue;
			}
			coord.i = i;
			coord.j = j;
			data.back().push_back(coord);
		}
	}
}




//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("debug=b");           // determine bad input line num
	opts.define("author=b");          // author of program
	opts.define("version=b");         // compilation info
	opts.define("example=b");         // example usages
	opts.define("h|help=b");          // short description
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, Oct 2004" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 3 May 2018" << endl;
		cout << "compiled: " << __DATE__ << endl;
		cout << MUSEINFO_VERSION << endl;
		exit(0);
	} else if (opts.getBoolean("help")) {
		usage(opts.getCommand().c_str());
		exit(0);
	} else if (opts.getBoolean("example")) {
		example();
		exit(0);
	}

	debugQ = opts.getBoolean("debug");
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

void usage(const string& command) {
	cout <<
	"                                                                         \n"
	<< endl;
}



