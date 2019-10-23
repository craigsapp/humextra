//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Jul 18 11:23:42 PDT 2005
// Last Modified: Mon Jul 18 11:23:46 PDT 2005
// Last Modified: Sun Mar  2 18:58:48 PST 2008 Added -l and -i options
// Last Modified: Mon Mar  3 13:46:34 PST 2008 Added -r option
// Last Modified: Tue Apr  9 08:18:06 PDT 2013 Enabled multiple segment input
// Filename:      ...sig/examples/all/thrux.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/thrux.cpp
// Syntax:        C++; museinfo
//
// Description:   C++ implementation of the Humdrum Toolkit thru command.
//

#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "humdrum.h"


// function declarations
void      checkOptions        (Options& opts, int argc, char* argv[]);
void      example             (void);
void      processData         (HumdrumFile& infile);
void      usage               (const char* command);
void      getLabelSequence    (vector<string>& labelsequence,
                               const string& astring);
int       getLabelIndex       (vector<string>& labels, string& key);
void      printLabelList      (HumdrumFile& infile);
void      printLabelInfo      (HumdrumFile& infile);
int       getBarline          (HumdrumFile& infile, int line);
int       adjustFirstBarline  (HumdrumFile& infile);


// global variables
Options      options;            // database for command-line arguments
string       variation = "";     // used with -v option
int          listQ = 0;          // used with -l option
int          infoQ = 0;          // used with -i option
int          keepQ = 0;          // used with -k option
string       realization = "";   // used with -r option


///////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
	HumdrumFileSet infiles;

	// process the command-line options
	checkOptions(options, argc, argv);

	// figure out the number of input files to process
	int numinputs = options.getArgCount();

	int i;
	if (numinputs < 1) {
		infiles.read(cin);
	} else {
		for (i=0; i<numinputs; i++) {
			infiles.readAppend(options.getArg(i+1));
		}
	}

	for (i=0; i<infiles.getCount(); i++) {
		if (listQ) {
			printLabelList(infiles[i]);
			exit(0);
		}
		if (infoQ) {
			printLabelInfo(infiles[i]);
			exit(0);
		}

		// analyze the input file according to command-line options
		if (infiles.getCount() > 1) {
			infiles[i].printNonemptySegmentLabel(cout);
		}
		processData(infiles[i]);

	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// printLabelList -- print a list of the thru labels.
//


void printLabelList(HumdrumFile& infile) {
	int length;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_interpretation) {
			continue;
		}
		if (strncmp(infile[i][0], "*>", 2) != 0) {
			continue;   // ignore non-labels
		}
		//if (strchr(infile[i][0], '[') != NULL) {
		//   continue;   // ignore realizations
		//}
		length = strlen(infile[i][0]);
		for (int j=2; j<length; j++) {
			if (infile[i][0][j] == '\0') {
				break;
			}
			cout << infile[i][0][j];
		}
		cout << '\n';
	}

}



//////////////////////////////
//
// printLabelInfo -- print a list of the thru labels.
//


void printLabelInfo(HumdrumFile& infile) {
	int i;
	int j;
	int length;

	infile.analyzeRhythm("4");

	vector<int> labellines;
	labellines.reserve(1000);

	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_interpretation) {
			continue;
		}
		if (strncmp(infile[i][0], "*>", 2) != 0) {
			continue;   // ignore non-labels
		}
		if (strchr(infile[i][0], '[') != NULL) {
			cout << "!!>";
			length = strlen(infile[i][0]);
			for (int j=2; j<length; j++) {
				if (infile[i][0][j] == '\0') {
					break;
				}
				cout << infile[i][0][j];
			}
			cout << '\n';
			continue;   // ignore realizations
		}
		labellines.push_back(i);
	}


	vector<int> barlines(1000, -1);
	for (i=0; i<(int)labellines.size(); i++) {
		barlines[i] = getBarline(infile, labellines[i]);
	}

	if (barlines.size() > 0) {
		barlines[0] = adjustFirstBarline(infile);
	}

	int startline;
	int endline;
	double startbeat;
	double endbeat;
	double duration;

	cout << "**label\t**sline\t**eline\t**sbeat\t**ebeat\t**dur\t**bar\n";
	for (i=0; i<(int)labellines.size(); i++) {
		startline = labellines[i];
		if (i<(int)labellines.size()-1) {
			endline = labellines[i+1]-1;
		} else {
			endline = infile.getNumLines() - 1;
		}
		startbeat = infile[startline].getAbsBeat();
		endbeat = infile[endline].getAbsBeat();
		duration = endbeat - startbeat;
		duration = int(duration * 10000.0 + 0.5) / 10000.0;
		length = strlen(infile[startline][0]);
		for (j=2; j<length; j++) {
			if (infile[startline][0][j] == '\0') {
				break;
			}
			cout << infile[startline][0][j];
		}
		cout << '\t';
		cout << startline + 1;
		cout << '\t';
		cout << endline + 1;
		cout << '\t';
		cout << startbeat;
		cout << '\t';
		cout << endbeat;
		cout << '\t';
		cout << duration;
		cout << '\t';
		cout << barlines[i];
		cout << '\n';

	}
	cout << "*-\t*-\t*-\t*-\t*-\t*-\t*-\n";

}



//////////////////////////////
//
// adjustFirstBarline --
//

int adjustFirstBarline(HumdrumFile& infile) {
	int i;
	int number = 0;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_data_measure) {
			continue;
		}
		if (infile[i].getAbsBeat() > 0) {
			break;
		}
		sscanf(infile[i][0], "=%d", &number);
		break;
	}

	return number;
}



//////////////////////////////
//
// getBarline --
//

int getBarline(HumdrumFile& infile, int line) {

	if (infile[line].getAbsBeat() == 0) {
		return 0;
	}

	int i;
	int missingcount = 0;
	int number = -1;
	for (i=line; i>0; i--) {
		if (infile[i].getType() != E_humrec_data_measure) {
			continue;
		}

		if (sscanf(infile[i][0], "=%d", &number) == 1) {
			break;
		} else {
			missingcount++;
		}

		if (missingcount > 1) {
			break;
		}
	}

	return number;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("v|variation=s:", "Choose the expansion variation");
	opts.define("l|list=b:", "Print list of labels in file");
	opts.define("k|keep=b:", "Keep variation interpretations");
	opts.define("i|info=b:", "Print info list of labels in file");
	opts.define("r|realization=s:", "alternate relaization label sequence");

	opts.define("d|debug=b");                    // determine bad input line num
	opts.define("author=b");                     // author of program
	opts.define("version=b");                    // compilation info
	opts.define("example=b");                    // example usages
	opts.define("h|help=b");                     // short description
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, May 1998" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: Nov 2000" << endl;
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

	variation   = opts.getString("variation");
	realization = opts.getString("realization");
	listQ       = opts.getBoolean("list");
	infoQ       = opts.getBoolean("info");
	keepQ       = opts.getBoolean("keep");

}



//////////////////////////////
//
// example -- example usage of the sonority program
//

void example(void) {
	cout <<
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// processData --
//

void processData(HumdrumFile& infile) {

	vector<string> labelsequence;
	labelsequence.reserve(1000);

	vector<string> labels;
	labels.reserve(1000);

	vector<int> startline;
	startline.reserve(1000);

	vector<int> stopline;
	stopline.reserve(1000);

	int header = -1;
	int footer = -1;
	char labelsearch[1024] = {0};
	strcpy(labelsearch, "*>");
	strcat(labelsearch, variation.c_str());
	strcat(labelsearch, "[");
	int length = strlen(labelsearch);

	// check for label to expand
	int i;
	int foundlabel = 0;
	string tempseq;
	if (realization.size()  == 0) {
		for (i=0; i<infile.getNumLines(); i++) {
			if (infile[i].getType() != E_humrec_interpretation) {
				continue;
			}
			if (strncmp(labelsearch, infile[i][0], length) != 0) {
				continue;
			}

			tempseq = &(infile[i][0][length]);
			getLabelSequence(labelsequence, tempseq);
			foundlabel = 1;
			break;
		}
	} else {
		foundlabel = 1;
		getLabelSequence(labelsequence, realization);
	}


	int j;
	if (foundlabel == 0) {
		// did not find the label to expand, so echo the data back
		for (i=0; i<infile.getNumLines(); i++) {
			if (strcmp(infile[i][0], "*thru") == 0) {
				continue;
			}
			cout << infile[i] << "\n";
			if (strncmp(infile[i][0], "**", 2) == 0) {
				for (j=0; j<infile[i].getFieldCount(); j++) {
					cout << "*thru";
					if (j < infile[i].getFieldCount() - 1) {
						cout << "\t";
					}
				}
				cout << "\n";
			}
		}
		return;
	}

	// for (i=0; i<(int)labelsequence.size(); i++) {
	//    cout << i+1 << "\t=\t" << labelsequence[i] << endl;
	// }

	// search for the labeled sections in the music
	string label;
	int   location;
	int   index;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].getType() != E_humrec_interpretation) {
			continue;
		}
		if (strcmp("*-", infile[i][0]) == 0) {
			location = i-1;
			footer = i;
			stopline.push_back(location);
		}
		if (strncmp("*>", infile[i][0], 2) != 0) {
			continue;
		}
		if (strchr(infile[i][0], '[') != NULL) {
			continue;
		}
		if (strchr(infile[i][0], ']') != NULL) {
			continue;
		}

		if (labels.size() == 0) {
			header = i-1;
		}

		label = &(infile[i][0][2]);
		index = (int)labels.size();
		location = i-1;
		if (startline.size() > 0) {
			stopline.push_back(location);
		}
		labels.resize(index+1);
		labels[index] = label;
		startline.push_back(i);
	}

	// cout << "FOOTER = " << footer << endl;
	// cout << "HEADER = " << header << endl;
	// for (i=0; i<(int)labels.size(); i++) {
	//    cout << "\t" << i << "\t=\t" << labels[i]
	//         << "\t" << startline[i] << "\t" << stopline[i]
	//         << endl;
	// }

	// now ready to copy the labeled segements into a final file.


	// print header:
	for (i=0; i<=header; i++) {
		if (strcmp(infile[i][0], "*thru") == 0) {
			continue;
		}

		if (!keepQ) {
			if (infile[i].getType() == E_humrec_data_interpretation) {
				if (strncmp(infile[i][0], "*>", 2) == 0) {
					if (strchr(infile[i][0], '[') != NULL) {
						continue;
					}
				}
			}
		}

		cout << infile[i] << "\n";
		if (strncmp(infile[i][0], "**", 2) == 0) {
			for (j=0; j<infile[i].getFieldCount(); j++) {
				cout << "*thru";
				if (j < infile[i].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			cout << "\n";
		}
	}

	int start;
	int stop;
	for (i=0; i<(int)labelsequence.size(); i++) {
		index = getLabelIndex(labels, labelsequence[i]);
		if (index < 0) {
			cout << "!! THRU ERROR: label " << labelsequence[i]
				  << " does not exist, skipping.\n";
		}
		start = startline[index];
		stop  = stopline[index];
		for (j=start; j<=stop; j++) {

			if (!keepQ) {
				if (infile[j].getType() == E_humrec_data_interpretation) {
					if (strncmp(infile[j][0], "*>", 2) == 0) {
						if (strchr(infile[j][0], '[') != NULL) {
							continue;
						}
					}
				}
			}
			cout << infile[j] << "\n";
		}
	}

	// print footer:
	for (i=footer; i<infile.getNumLines(); i++) {
		if (!keepQ) {
			if (infile[i].getType() == E_humrec_data_interpretation) {
				if (strncmp(infile[i][0], "*>", 2) == 0) {
					if (strchr(infile[i][0], '[') != NULL) {
						continue;
					}
				}
			}
		}
		cout << infile[i] << "\n";
	}

}



//////////////////////////////
//
// getLabelIndex --
//

int getLabelIndex(vector<string>& labels, string& key) {
	for (int i=0; i<(int)labels.size(); i++) {
		if (key == labels[i]) {
			return i;
		}
	}
	return -1;
}



//////////////////////////////
//
// getLabelSequence --
//

void getLabelSequence(vector<string>& labelsequence,
		const string& astring) {
	int slength = (int)astring.size();
	char* sdata = new char[slength+1];
	strcpy(sdata, astring.c_str());
	const char* ignorecharacters = ", [] ";
	int index;

	char* strptr = strtok(sdata, ignorecharacters);
	while (strptr != NULL) {
		labelsequence.resize((int)labelsequence.size() + 1);
		index = (int)labelsequence.size() - 1;
		labelsequence[index] = strptr;
		strptr = strtok(NULL, ignorecharacters);
	}

	delete [] sdata;
}



//////////////////////////////
//
// usage -- gives the usage statement for the sonority program
//

void usage(const char* command) {
	cout <<
	"                                                                         \n"
	<< endl;
}



