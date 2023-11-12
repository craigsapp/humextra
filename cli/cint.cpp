//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Sep 16 13:53:47 PDT 2013
// Last Modified: Thu Sep 19 16:10:27 PDT 2013
// Filename:      ...museinfo/examples/all/cint.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/cint.cpp
// Syntax:        C++; museinfo
//
// Description:   Calculates counterpoint interval modules in polyphonic
//                music.
//
// Crossing code not yet finished.
//

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "humdrum.h"
#include "PerlRegularExpression.h"

using namespace std;

#define EMPTY_ID ""
#define REST 0
#define RESTINT -1000000
#define RESTSTRING "R"
#define INTERVAL_HARMONIC 1
#define INTERVAL_MELODIC  2
#define MARKNOTES  1

class NoteNode {
	public:
		int b40;         // base-40 pitch number or 0 if a rest, negative if tied
		int line;        // line number in original score of note
		int spine;       // spine number in original score of note
		int measure;     // measure number of note
		int serial;      // serial number
		int mark;        // for marking search matches
		string notemarker;  // for pass-through of marks
		double beatsize; // time signature bottom value which or
                       // 3 times the bottom if compound meter
		RationalNumber duration;  // duration

		void      clear               (void);
		          NoteNode            (void)      { clear(); }
		          NoteNode            (const NoteNode& anode);
		          NoteNode            (NoteNode& anode);
		          NoteNode& operator= (NoteNode& anode);
		         ~NoteNode            (void);
		int      isRest               (void) { return b40 == 0 ? 1 : 0; }
		int      isSustain            (void) { return b40 < 0 ? 1 : 0; }
		int      isAttack             (void) { return b40 > 0 ? 1 : 0; }
		int      getB40               (void) { return abs(b40); }
		void     setId                (const string& anid);
		string   getIdString          (void);
		string   getId                (void);

	protected:
		string   protected_id; // id number provided by data
};


NoteNode::NoteNode(NoteNode& anode) {
	b40          = anode.b40;
	line         = anode.line;
	spine        = anode.spine;
	measure      = anode.measure;
	serial       = anode.serial;
	mark         = anode.mark;
	notemarker   = anode.notemarker;
	beatsize     = anode.beatsize;
	duration     = 0;
	protected_id = anode.protected_id;
}

NoteNode::NoteNode(const NoteNode& anode) {
	b40          = anode.b40;
	line         = anode.line;
	spine        = anode.spine;
	measure      = anode.measure;
	serial       = anode.serial;
	mark         = anode.mark;
	notemarker   = anode.notemarker;
	beatsize     = anode.beatsize;
	duration     = 0;
	protected_id = anode.protected_id;
}


NoteNode& NoteNode::operator=(NoteNode& anode) {
	if (this == &anode) {
		return *this;
	}
	b40          = anode.b40;
	line         = anode.line;
	spine        = anode.spine;
	measure      = anode.measure;
	serial       = anode.serial;
	mark         = anode.mark;
	notemarker   = anode.notemarker;
	beatsize     = anode.beatsize;
	duration     = anode.duration;
	protected_id = anode.protected_id;
	return *this;
}


void NoteNode::setId(const string& anid) {
		protected_id = anid;
}


NoteNode::~NoteNode(void) {
	// do nothing
}


void NoteNode::clear(void) {
	mark = measure = beatsize = serial = b40 = 0;
	notemarker = "";
	line = spine = -1;
	protected_id.clear();
}


string NoteNode::getIdString(void) {
	return protected_id;
}


string NoteNode::getId(void) {
	return protected_id;
}



///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const string& command);
int       processFile          (HumdrumFile& infile, const string& filename,
                                Options& opts);
void      getKernTracks        (vector<int>& ktracks, HumdrumFile& infile);
int       validateInterval     (vector<vector<NoteNode> >& notes,
                                int i, int j, int k);
void      printIntervalInfo    (HumdrumFile& infile, int line,
                                int spine, vector<vector<NoteNode> >& notes,
                                int noteline, int noteindex,
                                vector<string>& abbr);
void      getAbbreviations     (vector<string>& abbreviations,
                                vector<string>& names);
void      getAbbreviation      (string& abbr, string& name);
void      extractNoteArray     (vector<vector<NoteNode> >& notes,
                                HumdrumFile& infile, vector<int>& ktracks,
                                vector<int>& reverselookup);
int       onlyRests            (vector<NoteNode>& data);
int       hasAttack            (vector<NoteNode>& data);
int       allSustained         (vector<NoteNode>& data);
void      printPitchGrid       (vector<vector<NoteNode> >& notes,
                                HumdrumFile& infile);
void      getNames             (vector<string>& names,
                                vector<int>& reverselookup, HumdrumFile& infile);
void      printLattice         (vector<vector<NoteNode> >& notes,
                                HumdrumFile& infile, vector<int>& ktracks,
                                vector<int>& reverselookup, int n);
void      printSpacer          (ostream& out);
int       printInterval        (ostream& out, NoteNode& note1, NoteNode& note2,
                                int type, int octaveadjust = 0);
int       printLatticeItem     (vector<vector<NoteNode> >& notes, int n,
                                int currentindex, int fileline);
int       printLatticeItemRows (vector<vector<NoteNode> >& notes, int n,
                                int currentindex, int fileline);
int       printLatticeModule   (ostream& out, vector<vector<NoteNode> >& notes,
                                int n, int startline, int part1, int part2);
void      printInterleaved     (HumdrumFile& infile, int line,
                                vector<int>& ktracks, vector<int>& reverselookup,
                                const string& interstring);
void      printLatticeInterleaved(vector<vector<NoteNode> >& notes,
                                HumdrumFile& infile, vector<int>& ktracks,
                                vector<int>& reverselookup, int n);
int       printInterleavedLattice(HumdrumFile& infile, int line,
                                vector<int>& ktracks, vector<int>& reverselookup,
                                int n, int currentindex,
                                vector<vector<NoteNode> >& notes);
int       printCombinations    (vector<vector<NoteNode> >& notes,
                                HumdrumFile& infile, vector<int>& ktracks,
                                vector<int>& reverselookup, int n,
                                vector<vector<string> >& retrospective);
void      printAsCombination   (HumdrumFile& infile, int line,
                                vector<int>& ktracks, vector<int>& reverselookup,
                                const string& interstring);
int       printModuleCombinations(HumdrumFile& infile, int line,
                                vector<int>& ktracks, vector<int>& reverselookup,
                                int n, int currentindex,
                                vector<vector<NoteNode> >& notes,
                                int& matchcount,
                                vector<vector<string> >& retrospective);
int       printCombinationsSuspensions(vector<vector<NoteNode> >& notes,
                                HumdrumFile& infile, vector<int>& ktracks,
                                vector<int>& reverselookup, int n,
                                vector<vector<string> >& retrospective);
int       printCombinationModule(ostream& out, const string& filename,
                                vector<vector<NoteNode> >& notes,
                                int n, int startline, int part1, int part2,
                                vector<vector<string> >& retrospective,
                                string& notemarker, int markstate = 0);
int       printCombinationModulePrepare(ostream& out, const string& filename,
                                vector<vector<NoteNode> >& notes, int n,
                                int startline, int part1, int part2,
                                vector<vector<string> >& retrospective,
                                HumdrumFile& infile);
int       getOctaveAdjustForCombinationModule(vector<vector<NoteNode> >& notes,
                                int n, int startline, int part1, int part2);
void      addMarksToInputData  (HumdrumFile& infile,
                                vector<vector<NoteNode> >& notes,
                                vector<int>& ktracks,
                                vector<int>& reverselookup);
void      markNote             (HumdrumFile& infile, int line, int col);
void      initializeRetrospective(vector<vector<string> >& retrospective,
                                HumdrumFile& infile, vector<int>& ktracks);
int       getTriangleIndex     (int number, int num1, int num2);
void      adjustKTracks        (vector<int>& ktracks, const string& koption);
int       getMeasure           (HumdrumFile& infile, int line);

// global variables
Options   options;             // database for command-line arguments
int       debugQ       = 0;      // used with --debug option
int       base40Q      = 0;      // used with --40 option
int       base12Q      = 0;      // used with --12 option
int       base7Q       = 0;      // used with -7 option
int       pitchesQ     = 0;      // used with --pitches option
int       rhythmQ      = 0;      // used with -r option and others
int       durationQ    = 0;      // used with --dur option
int       latticeQ     = 0;      // used with -l option
int       interleavedQ = 0;      // used with -L option
int       Chaincount   = 1;      // used with -n option
int       chromaticQ   = 0;      // used with --chromatic option
int       sustainQ     = 0;      // used with -s option
int       zeroQ        = 0;      // used with -z option
int       topQ         = 0;      // used with -t option
int       toponlyQ     = 0;      // used with -T option
int       hparenQ      = 0;      // used with -h option
int       mparenQ      = 0;      // used with -y option
int       locationQ    = 0;      // used with --location option
int       koptionQ     = 0;      // used with -k option
int       parenQ       = 0;      // used with -p option
int       rowsQ        = 0;      // used with --rows option
int       hmarkerQ     = 0;      // used with -h option
int       mmarkerQ     = 0;      // used with -m option
int       attackQ      = 0;      // used with --attacks option
int       rawQ         = 0;      // used with --raw option
int       raw2Q        = 0;      // used with --raw2 option
int       xoptionQ     = 0;      // used with -x option
int       octaveallQ   = 0;      // used with -O option
int       octaveQ      = 0;      // used with -o option
int       noharmonicQ  = 0;      // used with -H option
int       nomelodicQ   = 0;      // used with -M option
int       norestsQ     = 0;      // used with -R option
int       nounisonsQ   = 0;      // used with -U option
int       filenameQ    = 0;      // used with -f option
int       searchQ      = 0;      // used with --search option
int       markQ        = 0;      // used with --mark option
int       countQ       = 0;      // used with --count option
int       suspensionsQ = 0;      // used with --suspensions option
int       uncrossQ     = 0;      // used with -c option
int       retroQ       = 0;      // used with --retro option
int       idQ          = 0;      // used with --id option
vector<string> Ids;              // used with --id option
string    NoteMarker;            // used with -N option
PerlRegularExpression SearchString;
string Spacer;


///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
	checkOptions(options, argc, argv);
	HumdrumStream streamer(options);
	HumdrumFile infile;

	int count = 0;
	int totalcount = 0;
	while (streamer.read(infile)) {
		count = processFile(infile, infile.getFileName(), options);
		totalcount += count;
		if (countQ) {
			if (filenameQ && (count > 0)) {
				cout << infile.getFileName() << "\t";
				cout << count << endl;
			}
		}
	}

	if (countQ) {
		if (filenameQ) {
			cout << "TOTAL:\t";
		}
		cout << totalcount << endl;
	}

}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

int processFile(HumdrumFile& infile, const string& filename, Options& options) {

	vector<vector<NoteNode> > notes;
	vector<string>     names;
	vector<int>              ktracks;
	vector<int>              reverselookup;

	infile.getTracksByExInterp(ktracks, "**kern");
	if (koptionQ) {
		adjustKTracks(ktracks, options.getString("koption").c_str());
	}
	notes.resize(ktracks.size());
	reverselookup.resize(infile.getMaxTracks()+1);
	std::fill(reverselookup.begin(), reverselookup.end(), -1);

	vector<vector<string> > retrospective;
	if (retroQ) {
		initializeRetrospective(retrospective, infile, ktracks);
	}

	if (locationQ || rhythmQ || durationQ) {
		infile.analyzeRhythm("4");
	}

	int i;
	for (i=0; i<(int)ktracks.size(); i++) {
		reverselookup[ktracks[i]] = i;
		notes[i].reserve(infile.getNumLines());
		notes[i].resize(0);
	}

	getNames(names, reverselookup, infile);

	PerlRegularExpression pre;

	extractNoteArray(notes, infile, ktracks, reverselookup);

	if (pitchesQ) {
		printPitchGrid(notes, infile);
		exit(0);
	}

	int count = 0;
	if (latticeQ) {
		printLattice(notes, infile, ktracks, reverselookup, Chaincount);
	} else if (interleavedQ) {
		printLatticeInterleaved(notes, infile, ktracks, reverselookup,
			Chaincount);
	} else if (suspensionsQ) {
		count = printCombinationsSuspensions(notes, infile, ktracks,
				reverselookup, Chaincount, retrospective);
	} else {
		count = printCombinations(notes, infile, ktracks, reverselookup,
				Chaincount, retrospective);
	}


	// handle search results here
	if (markQ) {
		if (count > 0) {
			addMarksToInputData(infile, notes, ktracks, reverselookup);
		}
		cout << infile;
		cout << "!!!RDF**kern: @ = matched note, color=\"#ff0000\"\n";
	}

	if (debugQ) {
		int j;
		for (i=0; i<(int)retrospective[0].size(); i++) {
			for (j=0; j<(int)retrospective.size(); j++) {
				cout << retrospective[j][i];
				if (j < (int)retrospective.size() - 1) {
					cout << "\t";
				}
			}
			cout << "\n";
		}
	}

	return count;
}



//////////////////////////////
//
// adjustKTracks -- Select only two spines to do analysis on.
//

void adjustKTracks(vector<int>& ktracks, const string& koption) {
	PerlRegularExpression pre;
	if (!pre.search(koption, "(\\$|\\$?\\d*)[^\\$\\d]+(\\$|\\$?\\d*)")) {
		return;
	}
	int number1 = 0;
	int number2 = 0;
	PerlRegularExpression pre2;

	if (pre2.search(pre.getSubmatch(1), "\\d+")) {
		number1 = atoi(pre.getSubmatch(1));
		if (strchr(pre.getSubmatch(), '$') != NULL) {
			number1 = (int)ktracks.size() - number1;
		}
	} else {
		number1 = (int)ktracks.size();
	}

	if (pre2.search(pre.getSubmatch(2), "\\d+")) {
		number2 = atoi(pre.getSubmatch(2));
		if (strchr(pre.getSubmatch(), '$') != NULL) {
			number2 = (int)ktracks.size() - number2;
		}
	} else {
		number2 = (int)ktracks.size();
	}

	number1--;
	number2--;

	int track1 = ktracks[number1];
	int track2 = ktracks[number2];

	ktracks.resize(2);
	ktracks[0] = track1;
	ktracks[1] = track2;
}



//////////////////////////////
//
// initializeRetrospective --
//

void initializeRetrospective(vector<vector<string> >& retrospective,
		HumdrumFile& infile, vector<int>& ktracks) {

	int columns = (int)ktracks.size();
	columns = columns * (columns + 1) / 2; // triangle number of analysis cols.
	retrospective.resize(columns);

	for (int i=0; i<(int)retrospective.size(); i++) {
		retrospective[i].resize(infile.getNumLines());
	}

	string token;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isLocalComment()) {
			token = "!";
		} else if (infile[i].isGlobalComment()) {
			token = "!";
		} else if (infile[i].isReferenceRecord()) {
			token = "!!";
		} else if (infile[i].isBarline()) {
			token = infile[i][0];
		} else if (infile[i].isData()) {
			token = ".";
		} else if (infile[i].isInterpretation()) {
			token = "*";
			if (infile[i].isExclusiveInterpretation(0)) {
				token = "**cint";
			}
		}

		for (int j=0; j<(int)retrospective.size(); j++) {
			retrospective[j][i] = token;
		}
	}

	if (debugQ) {
		for (int i=0; i<(int)retrospective[0].size(); i++) {
			for (int j=0; j<(int)retrospective.size(); j++) {
				cout << retrospective[j][i];
				if (j < (int)retrospective.size() - 1) {
					cout << "\t";
				}
			}
			cout << "\n";
		}
	}
}



//////////////////////////////
//
// printCombinationsSuspensions --
//

int  printCombinationsSuspensions(vector<vector<NoteNode> >& notes,
		HumdrumFile& infile, vector<int>& ktracks, vector<int>& reverselookup,
		int n, vector<vector<string> >& retrospective) {

	char sbuffer[24096] = {0};

	int oldcountQ = countQ;
	countQ = 1;             // mostly used to suppress intermediate output

	int countsum = 0;

	searchQ    = 1;               // turn on searching

	// Suspensions with length-2 modules
	n = 2;                        // -n 2
	xoptionQ   = 1;               // -x
	strcpy(sbuffer, "");

	strcat(sbuffer, "^7xs 1 6sx -2 8xx$"); 	strcat(sbuffer, "|");
	strcat(sbuffer, "^2sx -2 3xs 2 1xx$"); 	strcat(sbuffer, "|");
	strcat(sbuffer, "^7xs 1 6sx 2 6xx$"); 	strcat(sbuffer, "|");
	strcat(sbuffer, "^11xs 1 10sx -5 15xx$"); 	strcat(sbuffer, "|");
	strcat(sbuffer, "^4xs 1 3sx -5 8xx$"); 	strcat(sbuffer, "|");
	strcat(sbuffer, "^2sx -2 3xs 2 3xx$");	strcat(sbuffer, "|");
	// "9xs 1 8sx -2 10xx" archetype: Jos1405 m10 A&B
	strcat(sbuffer, "^9xs 1 8sx -2 10xx$");	strcat(sbuffer, "|");
	// "4xs 1 3sx 5xx" archetype: Jos1713 m87-88 A&B
	strcat(sbuffer, "^4xs 1 3sx -2 5xx$");	strcat(sbuffer, "|");
	// "11xs 1 10sx 4 8xx" archetype: Jos1402 m23-24 S&B
	strcat(sbuffer, "^11xs 1 10sx 4 8xx$");

	SearchString.initializeSearchAndStudy(sbuffer);
	countsum += printCombinations(notes, infile, ktracks, reverselookup, n,
						   retrospective);

	// Suspensions with length-3 modules /////////////////////////////////
	n = 3;                        // -n 2
	xoptionQ   = 1;               // -x
	strcpy(sbuffer, "");

	// "7xs 1 6sx 1 5sx 1 6sx" archetype: Jos2721 m27-78 S&T
	strcat(sbuffer, "^7xs 1 6sx 1 5sx 1 6sx$");	strcat(sbuffer, "|");
	// "7xs 1 6sx 1 6sx -2 8xx" archetype: Rue2018 m38-88 S&T
	strcat(sbuffer, "^7xs 1 6sx 1 6sx -2 8xx$");	strcat(sbuffer, "|");
	// "11xs 1 10sx 1 10sx -5 15xx" archetype: Rue2018 m38-88 S&B
	strcat(sbuffer, "^11xs 1 10sx 1 10sx -5 15xx$");

	SearchString.initializeSearchAndStudy(sbuffer);
	countsum += printCombinations(notes, infile, ktracks, reverselookup, n,
						   retrospective);

	// Suspensions with length-5 modules /////////////////////////////////
	n = 5;                        // -n 2
	xoptionQ   = 1;               // -x
	strcpy(sbuffer, "");
	// "8xs 1 7sx 1 7sx 1 6sx 1 6sx 1 5sx -1 8xx" archetype: Duf3015a m94 S&T
	strcat(sbuffer, "^8xs 1 7sx 1 7sx 1 6sx 1 5sx -2 8xx$");

	SearchString.initializeSearchAndStudy(sbuffer);
	countsum += printCombinations(notes, infile, ktracks, reverselookup, n,
						   retrospective);

	// Suspensions with rests modules

	// done with multiple searches.  Mark the notes in the score if required.

	countQ = oldcountQ;

	return countsum;
}



//////////////////////////////
//
// printCombinations --
//

int  printCombinations(vector<vector<NoteNode> >& notes,
		HumdrumFile& infile, vector<int>& ktracks, vector<int>& reverselookup,
		int n, vector<vector<string> >& retrospective) {
	int i;
	int currentindex = 0;
	int matchcount   = 0;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].hasSpines()) {
			// print all lines here which do not contain spine
			// information.
			if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
				cout << infile[i] << "\n";
			}
			continue;
		}

		// At this point there are only four types of lines:
		//    (1) data lines
		//    (2) interpretation lines (lines starting with *)
		//    (3) local comment lines (lines starting with single !)
		//    (4) barlines

		if (infile[i].isInterpretation()) {
			string pattern = "*";
			if (strncmp(infile[i][0], "**", 2) == 0) {
				pattern = "**cint";
			} else if (strcmp(infile[i][0], "*-") == 0) {
				pattern = "*-";
			} else if (strncmp(infile[i][0], "*>", 2) == 0) {
				pattern = infile[i][0];
			}
			printAsCombination(infile, i, ktracks, reverselookup, pattern);
		} else if (infile[i].isLocalComment()) {
			printAsCombination(infile, i, ktracks, reverselookup, "!");
		} else if (infile[i].isBarline()) {
			printAsCombination(infile, i, ktracks, reverselookup, infile[i][0]);
		} else {
			// print combination data
			currentindex = printModuleCombinations(infile, i, ktracks,
				reverselookup, n, currentindex, notes, matchcount, retrospective);
		}
		if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
				cout << "\n";
		}
	}

	return matchcount;
}



//////////////////////////////
//
// printModuleCombinations --
//

int printModuleCombinations(HumdrumFile& infile, int line, vector<int>& ktracks,
		vector<int>& reverselookup, int n, int currentindex,
		vector<vector<NoteNode> >& notes, int& matchcount,
		vector<vector<string> >& retrospective) {

	int fileline = line;
	string filename = infile.getFilename();

	while ((currentindex < (int)notes[0].size())
			&& (fileline > notes[0][currentindex].line)) {
		currentindex++;
	}
	if (currentindex >= (int)notes[0].size()) {
		if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
			cout << ".";
			printAsCombination(infile, line, ktracks, reverselookup, ".");
		}
		return currentindex;
	}
	if (notes[0][currentindex].line != fileline) {
		// This section occurs when two voices are both sustaining
		// at the start of the module.  Print a "." to indicate that
		// the counterpoint module is continuing from a previous line.
		printAsCombination(infile, line, ktracks, reverselookup, ".");
		return currentindex;
	}

	// found the index into notes which matches to the current fileline.
	if (currentindex + n >= (int)notes[0].size()) {
		// asking for chain longer than rest of available data.
		printAsCombination(infile, line, ktracks, reverselookup, ".");
		return currentindex;
	}

	// printAsCombination(infile, line, ktracks, reverselookup, ".");
	// return currentindex;

	int tracknext;
	int track;
	int j, jj;
	int count = 0;
	for (j=0; j<infile[line].getFieldCount(); j++) {
		if (!infile[line].isExInterp(j, "**kern")) {
			if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
				cout << infile[line][j];
				if (j < infile[line].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			continue;
		}
		track = infile[line].getPrimaryTrack(j);
		if (j < infile[line].getFieldCount() - 1) {
			tracknext = infile[line].getPrimaryTrack(j+1);
		} else {
			tracknext = -23525;
		}
		if (track == tracknext) {
			if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
				cout << infile[line][j];
				if (j < infile[line].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			continue;
		}

		// print the **kern spine, then check to see if there
		// is some **cint data to print
		if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
				cout << infile[line][j];
		}
		if ((track != ktracks.back()) && (reverselookup[track] >= 0)) {
			count = (int)ktracks.size() - reverselookup[track] - 1;
			for (jj = 0; jj<count; jj++) {
				if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
					cout << "\t";
				}
				int part1 = reverselookup[track];
				int part2 = part1+1+jj;
				// cout << part1 << "," << part2;
				matchcount += printCombinationModulePrepare(cout, filename,
						notes, n, currentindex, part1, part2, retrospective, infile);
			}
		}

		if (!(raw2Q || rawQ || markQ || retroQ || countQ)) {
			if (j < infile[line].getFieldCount() - 1) {
				cout << "\t";
			}
		}
	}

	return currentindex;
}



//////////////////////////////
//
// printCombinationModulePrepare --
//

int printCombinationModulePrepare(ostream& out, const string& filename,
		vector<vector<NoteNode> >& notes, int n, int startline, int part1,
		int part2, vector<vector<string> >& retrospective,
		HumdrumFile& infile) {
	int count = 0;
	stringstream tempstream;
	int match;
	string notemarker;
	int status = printCombinationModule(tempstream, filename, notes,
			n, startline, part1, part2, retrospective, notemarker);
	if (status) {
		if (raw2Q || rawQ) {
			tempstream << "\n";
		}
		tempstream << ends;
		if ((!NoteMarker.empty()) && (notemarker == NoteMarker)) {
			out << NoteMarker;
		}
		if (searchQ) {
			// Check to see if the extracted module matches to the
			// search query.
			string tstring = tempstream.str();
			string newstring;
			for (int i=0; i<(int)tstring.size(); i++) {
				if (tstring[i] != '\0') {
					newstring += tstring[i];
				}
			}
			match = SearchString.search(newstring);
			if (match) {
				count++;
				if (locationQ) {
					int line = notes[0][startline].line;
					double loc = infile[line].getAbsBeat() /
							infile[infile.getNumLines()-1].getAbsBeat();
					loc = int(100.0 * loc + 0.5)/100.0;
					cout << "!!LOCATION:"
							<< "\t"  << loc
							<< "\tm" << getMeasure(infile, line)
							<< "\tv" << ((int)notes.size() - part2)
							<< ":v"  << ((int)notes.size() - part1)
							<< "\t"  << infile.getFilename()
							<< endl;
				}
				if (raw2Q || rawQ) {
					string tempstring = tempstream.str();
					for (int i=0; i<(int)tempstring.size(); i++) {
						if (tempstring[i] != '\0') {
							out << tempstring[i];
						}
					}
					// newline already added somewhere previously.
					// cout << "\n";
				} else {
					// mark notes of the matched module(s) in the note array
					// for later marking in input score.
					status = printCombinationModule(tempstream, filename,
							notes, n, startline, part1, part2, retrospective,
							notemarker, MARKNOTES);
					if (status && (raw2Q || rawQ)) {
						tempstream << "\n";
					}
				}

			}
		} else {
			if (retroQ) {
				int column = getTriangleIndex((int)notes.size(), part1, part2);
				string tempstring = tempstream.str();
				string newstring;
				for (int i=0; i<(int)tempstring.size(); i++) {
					if (tempstring[i] != '\0') {
						newstring += tempstring[i];
					}
				}
				retrospective[column][status] = newstring;
			} else {
				string tempstring = tempstream.str();
				for (int i=0; i<(int)tempstring.size(); i++) {
					if (tempstring[i] != '\0') {
						out << tempstring[i];
					}
				}
			}
		}
	} else {
		if (!(raw2Q || rawQ || markQ || retroQ || countQ || searchQ)) {
			out << ".";
		}
	}

	return count;
}



//////////////////////////////
//
// getMeasure -- return the last measure number of the given line index.
//

int getMeasure(HumdrumFile& infile, int line) {
	int i;
	int measure = 0;
	for (i=line; i>=0; i--) {
		if (!infile[i].isBarline()) {
			continue;
		}
		if (std::isdigit(infile[i][0][1])) {
			int flag = sscanf(infile[i][0], "=%d", &measure);
			if (flag > 0) {
				return measure;
			}
		}
	}
	return 0;
}



//////////////////////////////
//
// getTriangleIndex --
//

int getTriangleIndex(int number, int num1, int num2) {
	// int triangle = number * (number + 1) / 2;
	// intermediate code, not active yet
	return 0;
}



//////////////////////////////
//
// addMarksToInputData -- mark notes in the score which matched
//     to the search query.
//

void addMarksToInputData(HumdrumFile& infile,
		vector<vector<NoteNode> >& notes, vector<int>& ktracks,
		vector<int>& reverselookup) {

	// first carry all marks from sustained portions of notes onto their
	// note attacks.
	int mark      = 0;
	int track     = 0;
	int markpitch = -1;

	for (int i=0; i<(int)notes.size(); i++) {
		mark = 0;
		for (int j=(int)notes[i].size()-1; j>=0; j--) {
			if (mark && (-markpitch == notes[i][j].b40)) {
				// In the sustain region between a note
				// attack and the marked sustain. Mark the
				// sustained region as well (don't know
				// if this behavior might change in the
				// future.
				notes[i][j].mark = mark;
				continue;
			}
			if (mark && (markpitch == notes[i][j].b40)) {
				// At the start of a notes which was marked.
				// Mark the attack since only note attacks
				// will be marked in the score
				notes[i][j].mark = mark;
				mark = 0;
				continue;
			}
			if (mark && (markpitch != notes[i][j].b40)) {
				// something strange happened.  Probably
				// an open tie which was not started
				// properly, so just clear mark.
				mark = 0;
			}
			if (notes[i][j].mark) {
				mark = 1;
				markpitch = abs(notes[i][j].b40);
			} else {
				mark = 0;
			}

		}
	}

	// a forward loop here into notes array to continue
	// marks to end of sutained region of marked notes
	for (int i=0; i<(int)notes.size(); i++)  {
		for (int j=0; j<(int)notes[i].size(); j++) {
			if (notes[i][j].mark) {
				markpitch = -abs(notes[i][j].b40);
				continue;
			} else if (notes[i][j].b40 == markpitch) {
				notes[i][j].mark = 1;
				continue;
			} else {
				markpitch = -1;
			}
		}
	}

	// print mark information:
	// for (j=0; j<(int)notes[0].size(); j++) {
	//    for (i=0; i<(int)notes.size(); i++) {
	//       cout << notes[i][j].b40;
	//       if (notes[i][j].mark) {
	//          cout << "m";
	//       }
	//       cout << " ";
	//    }
	//    cout << "\n";
	// }


	// now go through the input score placing user-markers onto notes
	// which were marked in the note array.
	int currentindex = 0;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
			continue;
		}
		while ((currentindex < (int)notes[0].size())
				&& (i > notes[0][currentindex].line)) {
			currentindex++;
		}
		if (currentindex >= (int)notes[0].size()) {
			continue;
		}
		if (notes[0][currentindex].line != i) {
			continue;
		}

		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			if (strcmp(infile[i][j], ".") == 0) {
				// Don't mark null tokens.
				continue;
			}
			track = infile[i].getPrimaryTrack(j);
			if (reverselookup[track] < 0) {
				continue;
			}
			if (notes[reverselookup[track]][currentindex].mark != 0) {
				markNote(infile, i, j);
			}
		}
	}
}



//////////////////////////////
//
// markNote --
//

void markNote(HumdrumFile& infile, int line, int col) {
	char buffer[1024] = {0};
	strcpy(buffer, infile[line][col]);
	strcat(buffer, "@");
	infile[line].changeField(col, buffer);
}



//////////////////////////////
//
// getOctaveAdjustForCombinationModule -- Find the minim harmonic interval in
//      the module chain.  If it is greater than an octave, then move it down
//      below an octave.  If the minimum is an octave, then don't do anything.
//      Not considering crossed voices.
//

int getOctaveAdjustForCombinationModule(vector<vector<NoteNode> >& notes, int n,
		int startline, int part1, int part2) {

	// if the current two notes are both sustains, then skip
	if ((notes[part1][startline].b40 <= 0) &&
			(notes[part2][startline].b40 <= 0)) {
		return 0;
	}

	if (norestsQ) {
		if (notes[part1][startline].b40 == 0) {
			return 0;
		}
		if (notes[part2][startline].b40 == 0) {
			return 0;
		}
	}

	int i;
	int count = 0;
	int attackcount = 0;
	int hint;

	vector<int> hintlist;
	hintlist.reserve(1000);

	for (i=startline; i<(int)notes[0].size(); i++) {
		if ((notes[part1][i].b40 <= 0) && (notes[part2][i].b40 <= 0)) {
			// skip notes if both are sustained
			continue;
		}

		if (attackQ && ((notes[part1][i].b40 <= 0) ||
					(notes[part2][i].b40 <= 0))) {
			if (attackcount == 0) {
				// not at the start of a pair of attacks.
				return 0;
			}
		}

		// consider  harmonic interval
		if ((notes[part2][i].b40 != 0) && (notes[part1][i].b40 != 0)) {
			hint = abs(notes[part2][i].b40) - abs(notes[part1][i].b40);
			if (uncrossQ && (hint < 0)) {
				hint = -hint;
			}
			hintlist.push_back(hint);
		}

		// if count matches n, then exit loop
		if ((count == n) && !attackQ) {
			break;
		}
		count++;

		if ((notes[part1][i].b40 > 0) && (notes[part2][i].b40 > 0)) {
			// keep track of double attacks
			if (attackcount >= n) {
				break;
			} else {
				attackcount++;
			}
		}

	}

	int minimum = 100000;

	for (i=0; i<(int)hintlist.size(); i++) {
		if (hintlist[i] < minimum) {
			minimum = hintlist[i];
		}
	}

	if (minimum > 1000) {
		// no intervals found to consider
		return 0;
	}

	if ((minimum >= 0) && (minimum <= 40)) {
		// nothing to do
		return 0;
	}

	if (minimum > 40) {
		return -(minimum/40);
	} else if (minimum < 0) {
		// don't go positive, this will invert the interval.
		return (-minimum)/40;
	}

	//int octaveadjust = -(minimum / 40);

	//if (attackQ && (attackcount == n)) {
	//   return octaveadjust;
	//} else if (count == n) {
	//   return octaveadjust;
	//} else {
	//   // did not find the required number of modules.
	//   return 0;
	//}

	return 0;
}



//////////////////////////////
//
// printCombinationModule -- Similar to printLatticeModule, but harmonic
//      intervals will not be triggered by a pair of sustained notes.
//      Print a counterpoint module or module chain given the start notes
//      and pair of parts to calculate the module (chains) from.  Will not
//      print anything if the chain length is longer than the note array.
//      The n parameter will be ignored if --attacks option is used
//      (--attacks will gnereate a variable length module chain).
//

int printCombinationModule(ostream& out, const string& filename,
		vector<vector<NoteNode> >& notes, int n, int startline, int part1,
		int part2, vector<vector<string> >& retrospective, string& notemarker,
		int markstate) {

	notemarker = "";

	if (norestsQ) {
		if (notes[part1][startline].b40 == 0) {
			return 0;
		}
		if (notes[part2][startline].b40 == 0) {
			return 0;
		}
	}

	stringstream idstream;

	// int crossing =  0;
	//int oldcrossing =  0;

	int octaveadjust = 0;   // used for -o option
	if (octaveQ) {
		octaveadjust = getOctaveAdjustForCombinationModule(notes, n, startline,
				part1, part2);
	}

	ostream *outp = &out;
	// if (rawQ && !searchQ) {
	//    outp = &cout;
	// }

	if (n + startline >= (int)notes[0].size()) { // [20150202]
		// definitely nothing to do
		return 0;
	}

	if ((int)notes.size() == 0) {
		// nothing to do
		return 0;
	}

	// if the current two notes are both sustains, then skip
	if ((notes[part1][startline].b40 <= 0) &&
			(notes[part2][startline].b40 <= 0)) {
		return 0;
	}

	if (raw2Q) {
		// print pitch of first bottom note
		if (filenameQ) {
			(*outp) << "file_" << filename;
			(*outp) << " ";
		}

		(*outp) << "v_" << part1 << " v_" << part2 << " ";

		if (base12Q) {
			(*outp) << "base12_";
			(*outp) << Convert::base40ToMidiNoteNumber(fabs(notes[part1][startline].b40));
		} else if (base40Q) {
			(*outp) << "base40_";
			(*outp) << fabs(notes[part1][startline].b40);
		} else {
			(*outp) << "base7_";
			(*outp) << Convert::base40ToDiatonic(fabs(notes[part1][startline].b40));
		}
		(*outp) << " ";
	}

	if (parenQ) {
		(*outp) << "(";
	}

	int i;
	int count = 0;
	int countm = 0;
	int attackcount = 0;
	int idstart = 0;

	int lastindex = -1;
	int retroline = 0;

	for (i=startline; i<(int)notes[0].size(); i++) {
		if ((notes[part1][i].b40 <= 0) && (notes[part2][i].b40 <= 0)) {
			// skip notes if both are sustained
			continue;
		}

		if (norestsQ) {
			if (notes[part1][i].b40 == 0) {
				return 0;
			}
			if (notes[part2][i].b40 == 0) {
				return 0;
			}
		}

		if (attackQ && ((notes[part1][i].b40 <= 0) ||
					(notes[part2][i].b40 <= 0))) {
			if (attackcount == 0) {
				// not at the start of a pair of attacks.
				return 0;
			}
		}

		// print the melodic intervals (if not the first item in chain)
		if ((count > 0) && !nomelodicQ) {
			if (mparenQ) {
				(*outp) << "{";
			}

			if (nounisonsQ) {
				// suppress modules which contain melodic perfect unisons:
				if ((notes[part1][i].b40 != 0) &&
					(abs(notes[part1][i].b40) == abs(notes[part1][lastindex].b40))) {
					return 0;
				}
				if ((notes[part2][i].b40 != 0) &&
					(abs(notes[part2][i].b40) == abs(notes[part2][lastindex].b40))) {
					return 0;
				}
			}
			// bottom melodic interval:
			if (!toponlyQ) {
				printInterval((*outp), notes[part1][lastindex],
								  notes[part1][i], INTERVAL_MELODIC);
				if (mmarkerQ) {
					(*outp) << "m";
				}
			}

			// print top melodic interval here if requested
			if (topQ || toponlyQ) {
				if (!toponlyQ) {
					printSpacer((*outp));
				}
				// top melodic interval:
				printInterval((*outp), notes[part2][lastindex],
								  notes[part2][i], INTERVAL_MELODIC);
				if (mmarkerQ) {
					(*outp) << "m";
				}
			}

			if (mparenQ) {
				(*outp) << "}";
			}
			printSpacer((*outp));
		}

		countm++;

		// print harmonic interval
		if (!noharmonicQ) {
			if (hparenQ) {
				(*outp) << "[";
			}
			if (markstate) {
				notes[part1][i].mark = 1;
				notes[part2][i].mark = 1;
			} else {
				// oldcrossing = crossing;
				//crossing = printInterval((*outp), notes[part1][i],
				//      notes[part2][i], INTERVAL_HARMONIC, octaveadjust);
				printInterval((*outp), notes[part1][i],
						notes[part2][i], INTERVAL_HARMONIC, octaveadjust);
			}

			if (durationQ) {
				if (notes[part1][i].isAttack()) {
					(*outp) << "D" << notes[part1][i].duration;
				}
				if (notes[part2][i].isAttack()) {
					(*outp) << "d" << notes[part1][i].duration;
				}
			}

			if (hmarkerQ) {
				(*outp) << "h";
			}
			if (hparenQ) {
				(*outp) << "]";
			}
		}

		// prepare the ids string if requested
		if (idQ) {
		//   if (count == 0) {
				// insert both first two notes, even if sustain.
				if (idstart != 0) { idstream << ':'; }
				idstart++;
				idstream << notes[part1][i].getId() << ':'
						   << notes[part2][i].getId();
		//   } else {
		//      // only insert IDs if an attack
		//      if (notes[part1][i].b40 > 0) {
		//         if (idstart != 0) { idstream << ':'; }
		//         idstart++;
		//         idstream << notes[part1][i].getId();
		//      }
		//      if (notes[part2][i].b40 > 0) {
		//         if (idstart != 0) { idstream << ':'; }
		//         idstart++;
		//         idstream << notes[part2][i].getId();
		//      }
		//   }
		}

		// keep track of notemarker state
		if (notes[part1][i].notemarker == NoteMarker) {
			notemarker = NoteMarker;
		}
		if (notes[part2][i].notemarker == NoteMarker) {
			notemarker = NoteMarker;
		}

		// if count matches n, then exit loop
		if ((count == n) && !attackQ) {
			retroline = i;
			break;
		} else {
			if (!noharmonicQ) {
				printSpacer((*outp));
			}
		}
		lastindex = i;
		count++;

		if ((notes[part1][i].b40 > 0) && (notes[part2][i].b40 > 0)) {
			// keep track of double attacks
			if (attackcount >= n) {
				retroline = i;
				break;
			} else {
				attackcount++;
			}
		}

	}

	if (parenQ) {
		(*outp) << ")";
	}

	if (idQ && idstart) {
		idstream << ends;
		(*outp) << " ID:" << idstream.str();
	}

	if (attackQ && (attackcount == n)) {
		return retroline;
	} else if ((countm>1) && (count == n)) {
		return retroline;
	} else if (n == 0) {
		return retroline;
	} else {
		// did not print the required number of modules.
		return 0;
	}

	return 0;
}



//////////////////////////////
//
// printAsCombination --
//

void printAsCombination(HumdrumFile& infile, int line, vector<int>& ktracks,
		vector<int>& reverselookup, const string& interstring) {

	if (raw2Q || rawQ || markQ || retroQ || countQ) {
		return;
	}

	vector<int> done(ktracks.size(), 0);
	int track;
	int tracknext;
	int count;

	for (int j=0; j<infile[line].getFieldCount(); j++) {
		if (!infile[line].isExInterp(j, "**kern")) {
			cout << infile[line][j];
			if (j < infile[line].getFieldCount() - 1) {
				cout << "\t";
			}
			continue;
		}
		track = infile[line].getPrimaryTrack(j);
		if (j < infile[line].getFieldCount() - 1) {
			tracknext = infile[line].getPrimaryTrack(j+1);
		} else {
			tracknext = -23525;
		}
		if (track == tracknext) {
			cout << infile[line][j];
			if (j < infile[line].getFieldCount() - 1) {
				cout << "\t";
			}
			continue;
		}

		// print the **kern spine, then check to see if there
		// is some **cint data to print
		// ggg
		cout << infile[line][j];

		if (reverselookup[track] >= 0) {
			count = (int)ktracks.size() - reverselookup[track] - 1;
			for (int jj=0; jj<count; jj++) {
				cout << "\t" << interstring;
			}
		}

		if (j < infile[line].getFieldCount() - 1) {
			cout << "\t";
		}
	}
}



//////////////////////////////
//
// printLatticeInterleaved --
//

void printLatticeInterleaved(vector<vector<NoteNode> >& notes,
		HumdrumFile& infile, vector<int>& ktracks, vector<int>& reverselookup,
		int n) {
	int currentindex = 0;
	int i;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].hasSpines()) {
			// print all lines here which do not contain spine
			// information.
			if (!(rawQ || raw2Q)) {
				cout << infile[i] << "\n";
			}
			continue;
		}

		// At this point there are only four types of lines:
		//    (1) data lines
		//    (2) interpretation lines (lines starting with *)
		//    (3) local comment lines (lines starting with single !)
		//    (4) barlines

		if (infile[i].isInterpretation()) {
			string pattern = "*";
			if (strncmp(infile[i][0], "**", 2) == 0) {
				pattern = "**cint";
			} else if (strcmp(infile[i][0], "*-") == 0) {
				pattern = "*-";
			} else if (strncmp(infile[i][0], "*>", 2) == 0) {
				pattern = infile[i][0];
			}
			printInterleaved(infile, i, ktracks, reverselookup, pattern);
		} else if (infile[i].isLocalComment()) {
			printInterleaved(infile, i, ktracks, reverselookup, "!");
		} else if (infile[i].isBarline()) {
			printInterleaved(infile, i, ktracks, reverselookup, infile[i][0]);
		} else {
			// print interleaved data
			currentindex = printInterleavedLattice(infile, i, ktracks,
				reverselookup, n, currentindex, notes);
		}
		if (!(rawQ || raw2Q)) {
			cout << "\n";
		}
	}
}



//////////////////////////////
//
// printInterleavedLattice --
//

int printInterleavedLattice(HumdrumFile& infile, int line, vector<int>& ktracks,
		vector<int>& reverselookup, int n, int currentindex,
		vector<vector<NoteNode> >& notes) {

	int fileline = line;

	while ((currentindex < (int)notes[0].size())
			&& (fileline > notes[0][currentindex].line)) {
		currentindex++;
	}
	if (currentindex >= (int)notes[0].size()) {
		if (!(rawQ || raw2Q)) {
			cout << ".";
			printInterleaved(infile, line, ktracks, reverselookup, ".");
		}
		return currentindex;
	}
	if (notes[0][currentindex].line != fileline) {
		// should never get here.
		printInterleaved(infile, line, ktracks, reverselookup, "?");
		return currentindex;
	}

	// found the index into notes which matches to the current fileline.
	if (currentindex + n >= (int)notes[0].size()) {
		// asking for chain longer than rest of available data.
		printInterleaved(infile, line, ktracks, reverselookup, ".");
		return currentindex;
	}

	int tracknext;
	int track;
	int j;
	for (j=0; j<infile[line].getFieldCount(); j++) {
		if (!infile[line].isExInterp(j, "**kern")) {
			if (!(rawQ || raw2Q)) {
				cout << infile[line][j];
				if (j < infile[line].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			continue;
		}
		track = infile[line].getPrimaryTrack(j);
		if (j < infile[line].getFieldCount() - 1) {
			tracknext = infile[line].getPrimaryTrack(j+1);
		} else {
			tracknext = -23525;
		}
		if (track == tracknext) {
			if (!(rawQ || raw2Q)) {
				cout << infile[line][j];
				if (j < infile[line].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			continue;
		}

		// print the **kern spine, then check to see if there
		// is some **cint data to print
		if (!(rawQ || raw2Q)) {
			cout << infile[line][j];
		}
		if ((track != ktracks.back()) && (reverselookup[track] >= 0)) {
			if (!(rawQ || raw2Q)) {
				cout << "\t";
			}
			int part1 = reverselookup[track];
			int part2 = part1+1;
			// cout << part1 << "," << part2;
			printLatticeModule(cout, notes, n, currentindex, part1, part2);
		}

		if (!(rawQ || raw2Q)) {
			if (j < infile[line].getFieldCount() - 1) {
				cout << "\t";
			}
		}
	}

	return currentindex;
}



//////////////////////////////
//
// printInterleaved --
//

void printInterleaved(HumdrumFile& infile, int line, vector<int>& ktracks,
		vector<int>& reverselookup, const string& interstring) {

	vector<int> done(ktracks.size(), 0);
	int track;
	int tracknext;

	int j;
	for (j=0; j<infile[line].getFieldCount(); j++) {
		if (!infile[line].isExInterp(j, "**kern")) {
			if (!(rawQ || raw2Q)) {
				cout << infile[line][j];
				if (j < infile[line].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			continue;
		}
		track = infile[line].getPrimaryTrack(j);
		if (j < infile[line].getFieldCount() - 1) {
			tracknext = infile[line].getPrimaryTrack(j+1);
		} else {
			tracknext = -23525;
		}
		if (track == tracknext) {
			if (!(rawQ || raw2Q)) {
				cout << infile[line][j];
				if (j < infile[line].getFieldCount() - 1) {
					cout << "\t";
				}
			}
			continue;
		}

		// print the **kern spine, then check to see if there
		// is some **cint data to print
		if (!(rawQ || raw2Q)) {
			cout << infile[line][j];

			if ((track != ktracks.back()) && (reverselookup[track] >= 0)) {
				cout << "\t" << interstring;
			}

			if (j < infile[line].getFieldCount() - 1) {
				cout << "\t";
			}
		}
	}
}



//////////////////////////////
//
// printLattice --
//

void printLattice(vector<vector<NoteNode> >& notes, HumdrumFile& infile,
		vector<int>& ktracks, vector<int>& reverselookup, int n) {

	int i;
	int ii = 0;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!(rawQ || raw2Q)) {
			cout << infile[i];
		}
		if (strncmp(infile[i][0], "**", 2) == 0) {
			if (!(rawQ || raw2Q)) {
				cout << "\t**cint\n";
			}
		} else if (strcmp(infile[i][0], "*-") == 0) {
			cout << "\t*-\n";
		} else if (infile[i].isData()) {
			if (!(rawQ || raw2Q)) {
				cout << "\t";
			}
			if (rowsQ) {
				ii = printLatticeItemRows(notes, n, ii, i);
			} else {
				ii = printLatticeItem(notes, n, ii, i);
			}
			if (!(rawQ || raw2Q)) {
				cout << "\n";
			}
		} else if (infile[i].isBarline()) {
			if (!(rawQ || raw2Q)) {
				cout << "\t" << infile[i][0] << "\n";
			}
		} else if (infile[i].isInterpretation()) {
			if (!(rawQ || raw2Q)) {
				cout << "\t*\n";
			}
		} else if (infile[i].isLocalComment()) {
			if (!(rawQ || raw2Q)) {
				cout << "\t!\n";
			}
		} else {
			// global comment, reference record, or empty line
			cout << "\n";
		}
	}

}


//////////////////////////////
//
// printLatticeModule -- print a counterpoint module or module chain given
//      the start notes and pair of parts to calculate the module
//      (chains) from.  Will not print anything if the chain length
//      is longer than the note array.
//

int printLatticeModule(ostream& out, vector<vector<NoteNode> >& notes, int n,
		int startline, int part1, int part2) {

	if (n + startline >= (int)notes[0].size()) {
		return 0;
	}

	if (parenQ) {
		out << "(";
	}

	int i;
	for (i=0; i<n; i++) {
		// print harmonic interval
		if (hparenQ) {
			out << "[";
		}
		printInterval(out, notes[part1][startline+i],
			notes[part2][startline+i], INTERVAL_HARMONIC);
		if (hmarkerQ) {
			out << "h";
		}
		if (hparenQ) {
			out << "]";
		}
		printSpacer(out);

		// print melodic interal(s)
		if (mparenQ) {
			out << "{";
		}
		// bottom melodic interval:
		if (!toponlyQ) {
			printInterval(out, notes[part1][startline+i],
						     notes[part1][startline+i+1], INTERVAL_MELODIC);
		}

		// print top melodic interval here if requested
		if (topQ || toponlyQ) {
			if (!toponlyQ) {
				printSpacer(out);
			}
			// top melodic interval:
			printInterval(out, notes[part2][startline+i],
						     notes[part2][startline+i+1], INTERVAL_MELODIC);
			if (mmarkerQ) {
				out << "m";
			}
		}

		if (mparenQ) {
			out << "}";
		}
		printSpacer(out);
	}

	// print last harmonic interval
	if (hparenQ) {
		out << "[";
	}
	printInterval(out, notes[part1][startline+n],
			notes[part2][startline+n], INTERVAL_HARMONIC);
	if (hmarkerQ) {
		out << "h";
	}
	if (hparenQ) {
		out << "]";
	}

	if (parenQ) {
		out << ")";
	}

	return 1;
}



//////////////////////////////
//
// printLatticeItemRows -- Row form of the lattice.
//

int printLatticeItemRows(vector<vector<NoteNode> >& notes, int n,
		int currentindex, int fileline) {

	while ((currentindex < (int)notes[0].size())
			&& (fileline > notes[0][currentindex].line)) {
		currentindex++;
	}
	if (currentindex >= (int)notes[0].size()) {
		if (!(rawQ || raw2Q)) {
			cout << ".";
		}
		return currentindex;
	}
	if (notes[0][currentindex].line != fileline) {
		// should never get here.
		if (!(rawQ || raw2Q)) {
			cout << "?";
		}
		return currentindex;
	}

	// found the index into notes which matches to the current fileline.
	if (currentindex + n >= (int)notes[0].size()) {
		// asking for chain longer than rest of available data.
		if (!(rawQ || raw2Q)) {
			cout << ".";
		}
		return currentindex;
	}

	stringstream tempstream;
	int j;
	int counter = 0;

	for (j=0; j<(int)notes.size()-1; j++) {
		// iterate through each part, printing the module
		// for adjacent parts.
		counter += printLatticeModule(tempstream, notes, n, currentindex, j, j+1);
		if (j < (int)notes.size()-2) {
			printSpacer(tempstream);
		}
	}

	if (!(rawQ || raw2Q)) {
		if (counter == 0) {
			cout << ".";
		} else {
			tempstream << ends;
			string tstring = tempstream.str();
			for (int i=0; i<(int)tstring.size(); i++) {
				if (tstring[i] != '\0') {
					cout << tstring[i];
				}
			}
		}
	}

	return currentindex;
}



//////////////////////////////
//
// printLatticeItem --
//

int printLatticeItem(vector<vector<NoteNode> >& notes, int n, int currentindex,
		int fileline) {
	while ((currentindex < (int)notes[0].size())
			&& (fileline > notes[0][currentindex].line)) {
		currentindex++;
	}
	if (currentindex >= (int)notes[0].size()) {
		if (!(rawQ || raw2Q)) {
			cout << ".";
		}
		return currentindex;
	}
	if (notes[0][currentindex].line != fileline) {
		// should never get here.
		if (!(rawQ || raw2Q)) {
			cout << "??";
		}
		return currentindex;
	}

	// found the index into notes which matches to the current fileline.
	if (currentindex + n >= (int)notes[0].size()) {
		// asking for chain longer than rest of available data.
		if (!(rawQ || raw2Q)) {
			cout << ".";
		}
		return currentindex;
	}

	int count;
	int melcount;
	int j;
	if (parenQ) {
		cout << "(";
	}
	for (count = 0; count < n; count++) {
		// print harmonic intervals
		if (hparenQ) {
			cout << "[";
		}
		for (j=0; j<(int)notes.size()-1; j++) {
			printInterval(cout, notes[j][currentindex+count],
					notes[j+1][currentindex+count], INTERVAL_HARMONIC);
			if (j < (int)notes.size()-2) {
				printSpacer(cout);
			}
		}
		if (hparenQ) {
			cout << "]";
		}
		printSpacer(cout);

		// print melodic intervals
		if (mparenQ) {
			cout << "{";
		}
		melcount = (int)notes.size()-1;
		if (topQ) {
			melcount++;
		}
		for (j=0; j<melcount; j++) {
			printInterval(cout, notes[j][currentindex+count],
					notes[j][currentindex+count+1], INTERVAL_MELODIC);
			if (j < melcount-1) {
				printSpacer(cout);
			}
		}
		if (mparenQ) {
			cout << "}";
		}
		printSpacer(cout);

	}
	// print last sequence of harmonic intervals
	if (hparenQ) {
		cout << "[";
	}
	for (j=0; j<(int)notes.size()-1; j++) {
		printInterval(cout, notes[j][currentindex+n],
				notes[j+1][currentindex+n], INTERVAL_HARMONIC);
		if (j < (int)notes.size()-2) {
			printSpacer(cout);
		}
	}
	if (hparenQ) {
		cout << "]";
	}
	if (parenQ) {
		cout << ")";
	}

	if ((rawQ || raw2Q)) {
		cout << "\n";
	}

	return currentindex;
}



//////////////////////////////
//
// printInterval --
//

int printInterval(ostream& out, NoteNode& note1, NoteNode& note2,
		int type, int octaveadjust) {
	if ((note1.b40 == REST) || (note2.b40 == REST)) {
		out << RESTSTRING;
		return 0;
	}
	int cross = 0;
	int pitch1 = abs(note1.b40);
	int pitch2 = abs(note2.b40);
	int interval = pitch2 - pitch1;

	if ((type == INTERVAL_HARMONIC) && (interval < 0)) {
		cross = 1;
		if (uncrossQ) {
			interval = -interval;
		}
	} else {
		interval = interval + octaveadjust  * 40;
	}

	if ((type == INTERVAL_HARMONIC) && (octaveallQ)) {
		if (interval <= -40) {
			interval = interval + 4000;
		}
		if (interval > 40) {
			if (interval % 40 == 0) {
				interval = 40;
			} else {
				interval = interval % 40;
			}
		} else if (interval < 0) {
			interval = interval + 40;
		}
	}
	if (base12Q && !chromaticQ) {
		interval = Convert::base40ToMidiNoteNumber(interval + 40*4 + 2) - 12*5;
		if ((type == INTERVAL_HARMONIC) && (octaveallQ)) {
			if (interval <= -12) {
				interval = interval + 1200;
			}
			if (interval > 12) {
				if (interval % 12 == 0) {
					interval = 12;
				} else {
					interval = interval % 12;
				}
			} else if (interval < 0) {
				interval = interval + 12;
			}
		}
		interval = interval + octaveadjust  * 12;
	} else if (base7Q && !chromaticQ) {
		interval = Convert::base40ToDiatonic(interval + 40*4 + 2) - 7*4;
		if ((type == INTERVAL_HARMONIC) && (octaveallQ)) {
			if (interval <= -7) {
				interval = interval + 700;
			}
			if (interval > 7) {
				if (interval % 7 == 0) {
					interval = 7;
				} else {
					interval = interval % 7;
				}
			} else if (interval < 0) {
				interval = interval + 7;
			}
		}
		interval = interval + octaveadjust  * 7;
	}


	if (chromaticQ) {
		char buffer[1024] = {0};
		out << Convert::base40ToIntervalAbbr(buffer, 1024, interval);
	} else {
		int negative = 1;
		if (interval < 0) {
			negative = -1;
			interval = -interval;
		}
		if (base7Q && !zeroQ) {
			out << negative * (interval+1);
		} else {
			out << negative * interval;
		}
	}

	if (sustainQ || ((type == INTERVAL_HARMONIC) && xoptionQ)) {
		// print sustain/attack information of intervals.
		if (note1.b40 < 0) {
			out << "s";
		} else {
			out << "x";
		}
		if (note2.b40 < 0) {
			out << "s";
		} else {
			out << "x";
		}
	}

	return cross;
}



//////////////////////////////
//
// printSpacer -- space or comma...
//

void printSpacer(ostream& out) {
	out << Spacer;
}



//////////////////////////////
//
// printPitchGrid -- print the pitch grid from which all counterpoint
//      modules are calculated.
//

void printPitchGrid(vector<vector<NoteNode> >& notes, HumdrumFile& infile) {
	int i = 0;
	int j = 0;
	int pitch;
	int abspitch;
	int newpitch;
	int partcount;
	int line;
	double beat;

	if (base40Q) {
		partcount = (int)notes.size();

		if (rhythmQ) {
			cout << "**absq\t";
			cout << "**bar\t";
			cout << "**beat\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "**b40";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;
		for (i=0; i<(int)notes[0].size(); i++) {
			if (rhythmQ) {
				line = notes[0][i].line;
				beat = (infile[line].getBeat()-1.0) * notes[0][i].beatsize + 1;
				cout << infile[line].getAbsBeat() << "\t";
				cout << notes[0][i].measure << "\t";
				cout << beat << "\t";
			}
			for (j=0; j<(int)notes.size(); j++) {
				if (!notes[j][i].notemarker.empty()) {
					cout << notes[j][i].notemarker;
				}
				cout << notes[j][i].b40;
				if (j < (int)notes.size()-1) {
					cout << "\t";
				}
			}
			cout << endl;
		}
		if (rhythmQ) {
			cout << "*-\t";
			cout << "*-\t";
			cout << "*-\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "*-";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;
	} else if (base7Q) {
		partcount = (int)notes.size();

		if (rhythmQ) {
			cout << "**absq\t";
			cout << "**bar\t";
			cout << "**beat\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "**b7";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;

		for (i=0; i<(int)notes[0].size(); i++) {
			if (rhythmQ) {
				line = notes[0][i].line;
				beat = (infile[line].getBeat()-1.0) * notes[0][i].beatsize + 1;
				cout << infile[line].getAbsBeat() << "\t";
				cout << notes[0][i].measure << "\t";
				cout << beat << "\t";
			}
			for (j=0; j<(int)notes.size(); j++) {
				if (!notes[j][i].notemarker.empty()) {
					cout << notes[j][i].notemarker;
				}
				pitch = notes[j][i].b40;
				abspitch = abs(pitch);
				if (pitch == 0) {
					// print rest
					cout << 0;
				} else {
					newpitch = Convert::base40ToDiatonic(abspitch);
					if (pitch < 0) {
						newpitch = -newpitch;
					}
					cout << newpitch;
				}
				if (j < (int)notes.size()-1) {
					cout << "\t";
				}
			}
			cout << endl;
		}
		if (rhythmQ) {
			cout << "*-\t";
			cout << "*-\t";
			cout << "*-\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "*-";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;
	} else if (base12Q) {
		partcount = (int)notes.size();

		if (rhythmQ) {
			cout << "**absq\t";
			cout << "**bar\t";
			cout << "**beat\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "**b12";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;

		for (i=0; i<(int)notes[0].size(); i++) {
			if (rhythmQ) {
				line = notes[0][i].line;
				beat = (infile[line].getBeat()-1) * notes[0][i].beatsize + 1;
				if (!notes[j][i].notemarker.empty()) {
					cout << notes[j][i].notemarker;
				}
				cout << infile[line].getAbsBeat() << "\t";
				cout << notes[0][i].measure << "\t";
				cout << beat << "\t";
			}
			for (j=0; j<(int)notes.size(); j++) {
				if (!notes[j][i].notemarker.empty()) {
					cout << notes[j][i].notemarker;
				}
				pitch = notes[j][i].b40;
				if (pitch == 0) {
					// print rest
					cout << 0;
				} else {
					abspitch = abs(pitch);
					newpitch = Convert::base40ToMidiNoteNumber(abspitch);
					if (pitch < 0) {
						newpitch = -newpitch;
					}
					cout << newpitch;
				}
				if (j < (int)notes.size()-1) {
					cout << "\t";
				}
			}
			cout << endl;
		}
		if (rhythmQ) {
			cout << "*-\t";
			cout << "*-\t";
			cout << "*-\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "*-";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;
	} else {
		// print as Humdrum **kern data
		char buffer[1024] = {0};
		partcount = (int)notes.size();

		if (rhythmQ) {
			cout << "**absq\t";
			cout << "**bar\t";
			cout << "**beat\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "**kern";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;

		for (i=0; i<(int)notes[0].size(); i++) {
			if (rhythmQ) {
				line = notes[0][i].line;
				beat = (infile[line].getBeat()-1) * notes[0][i].beatsize + 1;
				if (!notes[j][i].notemarker.empty()) {
					cout << notes[j][i].notemarker;
				}
				cout << infile[line].getAbsBeat() << "\t";
				cout << notes[0][i].measure << "\t";
				cout << beat << "\t";
			}
			for (j=0; j<(int)notes.size(); j++) {
				if (!notes[j][i].notemarker.empty()) {
					cout << notes[j][i].notemarker;
				}
				pitch = notes[j][i].b40;
				abspitch = abs(pitch);
				if (pitch == 0) {
					cout << "r";
				} else {
					if ((pitch > 0) && (i<(int)notes[j].size()-1) &&
							(notes[j][i+1].b40 == -abspitch)) {
						// start of a note which continues into next
						// sonority.
						cout << "[";
					}
					cout << Convert::base40ToKern(buffer, 1024, abspitch);
					// print tie continue/termination as necessary.
					if (pitch < 0) {
						if ((i < (int)notes[j].size() - 1) &&
								(notes[j][i+1].b40 == notes[j][i].b40)) {
							// note sustains further
							cout << "_";
						} else {
							// note does not sustain any further.
							cout << "]";
						}
					}
				}
				if (j < (int)notes.size()-1) {
					cout << "\t";
				}
			}
			cout << endl;
		}

		if (rhythmQ) {
			cout << "*-\t";
			cout << "*-\t";
			cout << "*-\t";
		}
		for (i=0; i<partcount; i++) {
			cout << "*-";
			if (i < partcount - 1) {
				cout << "\t";
			}
		}
		cout << endl;
	}
}



//////////////////////////////
//
// extractNoteArray --
//

void extractNoteArray(vector<vector<NoteNode> >& notes, HumdrumFile& infile,
		vector<int>& ktracks, vector<int>& reverselookup) {

	PerlRegularExpression pre;

	Ids.resize(infile.getMaxTracks()+1);
	int i, j, ii, jj;
	for (i=0; i<(int)Ids.size(); i++) {
		Ids[i] = EMPTY_ID;
	}

	vector<NoteNode> current(ktracks.size());
	vector<double> beatsizes(infile.getMaxTracks()+1, 1);

	int sign;
	int track = 0;
	int index;

	int snum = 0;
	int measurenumber = 0;
	int tempmeasurenum = 0;
	double beatsize = 1.0;
	int topnum, botnum;

	for (i=0; i<infile.getNumLines(); i++) {
		if (debugQ) {
			cout << "PROCESSING LINE: " << i << "\t" << infile[i] << endl;
		}
		if (infile[i].isMeasure()) {
			tempmeasurenum = infile.getMeasureNumber(i);
			if (tempmeasurenum >= 0) {
				measurenumber = tempmeasurenum;
			}
		}
		for (j=0; j<(int)current.size(); j++) {
			current[j].clear();
			current[j].measure = measurenumber;
			current[j].line = i;
		}

		if (infile[i].isMeasure() && (strstr(infile[i][0], "||") != NULL)) {
			// double barline (terminal for Josquin project), so add a row
			// of rests to prevent cint melodic interval identification between
			// adjacent notes in different sections.
			for (j=0; j<(int)notes.size(); j++) {
				notes[j].push_back(current[j]);
			}
		} else if (infile[i].isInterpretation()) {
			// search for time signatures from which to extract beat information.
			for (j=0; j<infile[i].getFieldCount(); j++) {
				track = infile[i].getPrimaryTrack(j);
				if (pre.search(infile[i][j], "^\\*M(\\d+)/(\\d+)")) {
					// deal with 3%2 in denominator later...
					topnum = atoi(pre.getSubmatch(1));
					botnum = atoi(pre.getSubmatch(2));
					beatsize = botnum;
					if (((topnum % 3) == 0) && (topnum > 3) && (botnum > 1)) {
						// compound meter
						// fix later
						beatsize = botnum / 3;
					}
					beatsizes[track] = beatsize / 4.0;
				} else if (strcmp(infile[i][j], "*met(C|)") == 0) {
					// MenCutC, use 2 as the "beat"
					beatsizes[track] = 2.0 / 4.0;
				}
			}
		} else if (idQ && infile[i].isLocalComment()) {
			for (j=0; j<infile[i].getFieldCount(); j++) {
				if (pre.search(infile[i][j], "^!ID:\\s*([^\\s]*)")) {
					int track = infile[i].getPrimaryTrack(j);
					Ids[track] = pre.getSubmatch(1);
				}
			}
		}

		if (!infile[i].isData()) {
			continue;
		}

		for (j=0; j<infile[i].getFieldCount(); j++) {
			sign = 1;
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			track = infile[i].getPrimaryTrack(j);
			index = reverselookup[track];
			if (index < 0) {
				continue;
			}
			if (idQ) {
				current[index].getId() = Ids[track];
				Ids[track] = "";  // don't assign to next item;
			}
			current[index].line  = i;
			current[index].spine = j;
			current[index].beatsize = beatsizes[track];
			if (strcmp(infile[i][j], ".") == 0) {
				sign = -1;
				ii = infile[i].getDotLine(j);
				jj = infile[i].getDotSpine(j);
			} else {
				ii = i;
				jj = j;
			}
			if (strstr(infile[ii][jj], NoteMarker.c_str()) != NULL) {
				current[index].notemarker = NoteMarker;
			}
			if (strchr(infile[ii][jj], 'r') != NULL) {
				current[index].b40 = 0;
				current[index].serial = ++snum;
				continue;
			}
			if (strcmp(infile[ii][jj], ".") == 0) {
				current[index].b40 = 0;
				current[index].serial = snum;
			}
			current[index].b40 = Convert::kernToBase40(infile[ii][jj]);
			if (strchr(infile[ii][jj], '_') != NULL) {
				sign = -1;
				current[index].serial = snum;
			}
			if (strchr(infile[ii][jj], ']') != NULL) {
				sign = -1;
				current[index].serial = snum;
			}
			current[index].b40 *= sign;
			if (sign > 0) {
				current[index].serial = ++snum;
				if (durationQ) {
					current[index].duration = infile.getTiedDurationR(ii, jj);
				}
			}
		}
		if (onlyRests(current) && onlyRests(notes.back())) {
			// don't store more than one row of rests in the data array.
			continue;
		}
		if (allSustained(current)) {
			// don't store sonorities which are purely sutained
			// (may need to be updated with a --sustain option implementation)
			continue;
		}
		for (j=0; j<(int)notes.size(); j++) {
			notes[j].push_back(current[j]);
		}
	}

	// attach ID tag to all sustain sections of notes
	if (idQ) {
		for (j=0; j<(int)notes.size(); j++) {
			for (i=1; i<(int)notes[j].size(); i++) {
				if (notes[j][i].isAttack()) {
					continue;
				}
				if ((int)notes[j][i].getId().size() > 0) {
					// allow for Ids on sustained notes which probably means
					// that there is a written tied note in the music.
					continue;
				}
				if (notes[j][i].getB40() == notes[j][i-1].getB40()) {
					notes[j][i].getId() = notes[j][i-1].getId();
				}
			}
		}
	}

}



//////////////////////////////
//
// onlyRests -- returns true if all NoteNodes are for rests
//

int onlyRests(vector<NoteNode>& data) {
	int i;
	for (i=0; i<(int)data.size(); i++) {
		if (!data[i].isRest()) {
			return 0;
		}
	}
	return 1;
}



//////////////////////////////
//
// hasAttack -- returns true if all NoteNodes are for rests
//

int hasAttack(vector<NoteNode>& data) {
	int i;
	for (i=0; i<(int)data.size(); i++) {
		if (data[i].isAttack()) {
			return 1;
		}
	}
	return 0;
}



//////////////////////////////
//
// allSustained -- returns true if all NoteNodes are sustains
//    or rests (but not all rests).
//

int allSustained(vector<NoteNode>& data) {
	int i;
	int hasnote = 0;
	for (i=0; i<(int)data.size(); i++) {
		if (data[i].b40 != 0) {
			hasnote = 1;
		}
		if (data[i].isAttack()) {
			return 0;
		}
	}
	if (hasnote == 0) {
		return 0;
	}
	return 1;
}



//////////////////////////////
//
// getAbbreviations --
//

void getAbbreviations(vector<string>& abbreviations,
		vector<string>& names) {
	abbreviations.resize(names.size());
	int i;
	for (i=0; i<(int)names.size(); i++) {
		getAbbreviation(abbreviations[i], names[i]);
	}
}



//////////////////////////////
//
// getAbbreviation --
//

void getAbbreviation(string& abbr, string& name) {
	PerlRegularExpression pre;
	abbr = name;
	pre.sar(abbr, "(?<=[a-zA-Z])[a-zA-Z]*", "", "");
	pre.tr(abbr, "123456789", "abcdefghi");
}



//////////////////////////////
//
// getKernTracks -- return a list of track number for **kern spines.
//

void getKernTracks(vector<int>& ktracks, HumdrumFile& infile) {
	ktracks.resize(infile.getMaxTracks());
	std::fill(ktracks.begin(), ktracks.end(), 0);
	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (infile[i].isExInterp(j, "**kern")) {
				int track = infile[i].getPrimaryTrack(j);
				ktracks.push_back(track);
			}
		}
		break;
	}
}



//////////////////////////////
//
// getNames -- get the names of each column if they have one.
//

void getNames(vector<string>& names, vector<int>& reverselookup,
		HumdrumFile& infile) {
	names.resize((int)reverselookup.size()-1);
	char buffer[1024] = {0};
	int value;
	PerlRegularExpression pre;
	int i;
	int j;
	int track;

	for (i=0; i<(int)names.size(); i++) {
		value = (int)reverselookup.size() - i;
		snprintf(buffer, 1024, "%d", value);
		names[i] = buffer;
	}

	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isData()) {
			// stop looking for instrument name after the first data line
			break;
		}
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (reverselookup[infile[i].getPrimaryTrack(j)] < 0) {
				continue;
			}
			if (!infile[i].isExInterp(j, "**kern")) {
				continue;
			}
			if (pre.search(infile[i][j], "^\\*I\"(.*)", "")) {
				track = infile[i].getPrimaryTrack(j);
				strcpy(buffer, pre.getSubmatch(1));
				names[reverselookup[track]] = buffer;
			}
		}
	}

	if (debugQ) {
		for (i=0; i<(int)names.size(); i++) {
			cout << i << ":\t" <<  names[i] << endl;
		}
	}

}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("base-40|base40|b40|40=b",
			"display pitches/intervals in base-40");
	opts.define("base-12|base12|b12|12=b",
			"display pitches/intervals in base-12");
	opts.define("base-7|base7|b7|7|diatonic=b",
			"display pitches/intervals in base-7");
	opts.define("g|grid|pitch|pitches=b",
			"display pitch grid used to calculate modules");
	opts.define("r|rhythm=b", "display rhythmic positions of notes");
	opts.define("f|filename=b", "display filenames with --count");
	opts.define("raw=b", "display only modules without formatting");
	opts.define("raw2=b", "display only modules formatted for Vishesh");
	opts.define("c|uncross=b", "uncross crossed voices when creating modules");
	opts.define("k|koption=s:", "Select only two spines to analyze");
	opts.define("C|comma=b", "separate intervals by comma rather than space");
	opts.define("retro|retrospective=b",
						"Retrospective module display in the score");
	opts.define("suspension|suspensions=b", "mark suspensions");
	opts.define("rows|row=b", "display lattices in row form");
	opts.define("dur|duration=b",
	            "display durations appended to harmonic interval note attacks");
	opts.define("id=b", "ids are echoed in module data");
	opts.define("L|interleaved-lattice=b", "display interleaved lattices");
	opts.define("q|harmonic-parentheses=b",
						"put square brackets around harmonic intervals");
	opts.define("h|harmonic-marker=b",
						"put h character after harmonic intervals");
	opts.define("m|melodic-marker=b",
						"put m character after melodic intervals");
	opts.define("y|melodic-parentheses=b",
						"put curly braces around melodic intervals");
	opts.define("p|parentheses=b", "put parentheses around modules intervals");
	opts.define("l|lattice=b", "calculate lattice");
	opts.define("loc|location=b", "displayLocation");
	opts.define("s|sustain=b", "display sustain/attack states of notes");
	opts.define("o|octave=b", "reduce compound intervals to within an octave");
	opts.define("H|no-harmonic=b", "don't display harmonic intervals");
	opts.define("M|no-melodic=b", "don't display melodic intervals");
	opts.define("t|top=b", "display top melodic interval of modules");
	opts.define("T|top-only=b", "display only top melodic interval of modules");
	opts.define("U|no-melodic-unisons=b", "no melodic perfect unisons");
	opts.define("attacks|attack=b",
			"start/stop module chains on pairs of note attacks");
	opts.define("z|zero=b", "display diatonic intervals with 0 offset");
	opts.define("N|note-marker=s:@", "pass-through note marking character");
	opts.define("x|xoption=b",
			"display attack/sustain information on harmonic intervals only");
	opts.define("n|chain=i:1", "number of sequential modules");
	opts.define("R|no-rest|no-rests|norest|norests=b",
			"number of sequential modules");
	opts.define("O|octave-all=b",
			"transpose all harmonic intervals to within an octave");
	opts.define("chromatic=b",
			"display intervals as diatonic intervals with chromatic alterations");
	opts.define("search=s:", "search string");
	opts.define("mark=b", "mark matches notes from searches in data");
	opts.define("count=b", "count matched modules from search query");
	opts.define("debug=b");              // determine bad input line num
	opts.define("author=b");             // author of program
	opts.define("version=b");            // compilation info
	opts.define("example=b");            // example usages
	opts.define("help=b");               // short description
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, September 2013" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 23 September 2013" << endl;
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

	koptionQ = opts.getBoolean("koption");

	if (opts.getBoolean("comma")) {
		Spacer = ',';
	} else {
		Spacer = ' ';
	}

	// dispay as base-7 by default
	base7Q = 1;

	base40Q    = opts.getBoolean("base-40");
	base12Q    = opts.getBoolean("base-12");
	chromaticQ = opts.getBoolean("chromatic");
	zeroQ      = opts.getBoolean("zero");

	if (base40Q) {
		base12Q = 0;
		base7Q = 0;
		zeroQ = 0;
	}

	if (base12Q) {
		base40Q = 0;
		base7Q = 0;
		zeroQ = 0;
	}

	pitchesQ     = opts.getBoolean("pitches");
	debugQ       = opts.getBoolean("debug");
	rhythmQ      = opts.getBoolean("rhythm");
	durationQ    = opts.getBoolean("duration");
	latticeQ     = opts.getBoolean("lattice");
	sustainQ     = opts.getBoolean("sustain");
	topQ         = opts.getBoolean("top");
	toponlyQ     = opts.getBoolean("top-only");
	hparenQ      = opts.getBoolean("harmonic-parentheses");
	mparenQ      = opts.getBoolean("melodic-parentheses");
	parenQ       = opts.getBoolean("parentheses");
	rowsQ        = opts.getBoolean("rows");
	hmarkerQ     = opts.getBoolean("harmonic-marker");
	interleavedQ = opts.getBoolean("interleaved-lattice");
	mmarkerQ     = opts.getBoolean("melodic-marker");
	attackQ      = opts.getBoolean("attacks");
	rawQ         = opts.getBoolean("raw");
	raw2Q        = opts.getBoolean("raw2");
	xoptionQ     = opts.getBoolean("x");
	octaveallQ   = opts.getBoolean("octave-all");
	octaveQ      = opts.getBoolean("octave");
	noharmonicQ  = opts.getBoolean("no-harmonic");
	nomelodicQ   = opts.getBoolean("no-melodic");
	norestsQ     = opts.getBoolean("no-rests");
	nounisonsQ   = opts.getBoolean("no-melodic-unisons");
	Chaincount   = opts.getInteger("n");
	searchQ      = opts.getBoolean("search");
	markQ        = opts.getBoolean("mark");
	idQ          = opts.getBoolean("id");
	countQ       = opts.getBoolean("count");
	filenameQ    = opts.getBoolean("filename");
	suspensionsQ = opts.getBoolean("suspensions");
	uncrossQ     = opts.getBoolean("uncross");
	locationQ    = opts.getBoolean("location");
	retroQ       = opts.getBoolean("retrospective");
	NoteMarker   = "";
	if (opts.getBoolean("note-marker")) {
		NoteMarker = opts.getString("note-marker");
	}
	if (Chaincount < 0) {
		Chaincount = 0;
	}

	if (searchQ) {
		// Automatically assume marking of --search is used
		// (may change in the future).
		markQ = 1;
	}
	if (countQ) {
		searchQ = 1;
		markQ   = 0;
	}

	if (raw2Q) {
		norestsQ = 1;
	}

	if (searchQ) {
		SearchString.initializeSearchAndStudy(opts.getString("search").c_str());
	}

}



//////////////////////////////
//
// example -- example usage of the quality program
//

void example(void) {
	cout <<
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
	<< endl;
}



