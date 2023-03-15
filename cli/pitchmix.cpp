//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed May  1 21:18:12 PDT 2002
// Last Modified: Mon Nov 18 15:41:51 PST 2013 Moved -t to -v
// Last Modified: Mon Nov 18 15:56:37 PST 2013 Moved -n to -t
// Last Modified: Mon Nov 18 15:56:37 PST 2013 -S reversed, depends on -s
// Filename:      ...sig/examples/all/pitchmix.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/pitchmix.cpp
// Syntax:        C++; museinfo
//
// Description:   Mix the ordering of pitches in a file, keeping the
//                durations in the same locations.
//

#include <string.h>
#include <time.h>
#include <ctype.h>

#include <vector>

#include "humdrum.h"

//////////////////////////////////////////////////////////////////////////

class NoteUnit {
	public:
		NoteUnit(void) { clear(); };
		void clear(void) {
			track = random = newpitch = pitch = line = spine = token = -1;
		};
		int    track;
		int    pitch;
		int    line;
		int    spine;
		int    token;
		int    random;
		int    newpitch;
};

//////////////////////////////////////////////////////////////////////////

// function declarations:
void      checkOptions          (Options& opts, int argc, char** argv);
void      example               (void);
void      usage                 (const string& command);
void      printNotes            (vector<NoteUnit>& notes);
void      getNotes              (vector<NoteUnit>& notes, HumdrumFile& infile);
int       compareNoteUnit       (const void* A, const void* B);
int       compareNoteUnitTrack  (const void* A, const void* B);
int       compareNoteSortTrack  (const void* A, const void* B);
void      scrambleNotes         (vector<NoteUnit>& notes);
void      replaceNotes          (HumdrumFile& infile, vector<NoteUnit>& notes);
void      replaceNote           (HumdrumFile& infile, int line, int spine,
                                 int token, int newpitch, int oldpitch = -1);
void      updateTiedNotes       (HumdrumFile& infile, int line, int spine,
                                 int token, int newpitch, int oldpitch);
int       nearestNeighbor       (int oldpitch, int newpitch);
int       throwDice             (double piecefraction);


// User interface variables:
Options   options;
int       debugQ          = 0;  // used with --debug option
int       restQ           = 0;  // used with -r option
int       seed            = 0;  // used with -s option
int       displaySeedQ    = 0;  // used with -S option
int       transQ          = 0;  // used with -m option
int       voiceQ          = 0;  // used with -v option
int       neighborQ       = 0;  // used with -n option
int       lowlimit        = 40; // lowest note when tranposing
EnvelopeString distring;        // used with -d option
int       distQ           = 0;  // used with -d option

//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
	// process the command-line options
	checkOptions(options, argc, argv);

	HumdrumFile infile;
	infile.read(options.getArg(1));
	if (distQ) {
		infile.analyzeRhythm();
	}
	vector<NoteUnit> notes;
	getNotes(notes, infile);
	scrambleNotes(notes);
	if (transQ) {
		printNotes(notes);
	} else {
		replaceNotes(infile, notes);
		cout << infile;
	}

	if (displaySeedQ) {
		cout << "!!!seed: " << seed << endl;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////



//////////////////////////////
//
// replaceNotes --
//

void replaceNotes(HumdrumFile& infile, vector<NoteUnit>& notes) {
	int i;
	for (i=0; i<(int)notes.size(); i++) {
		replaceNote(infile, notes[i].line, notes[i].spine,
			notes[i].token, notes[i].newpitch);
	}
}



//////////////////////////////
//
// replaceNote --
//

void replaceNote(HumdrumFile& infile, int line, int spine, int token,
		int newpitch, int oldpitch) {
	// just assume one token in spine for now
	HumdrumRecord& record = infile[line];
	char prebuffer[128] = {0};
	char postbuffer[128] = {0};
	char pbuffer[128] = {0};
	Convert::base40ToKern(pbuffer, 128, newpitch);
	if (oldpitch == -1) {
		oldpitch = Convert::kernToBase40(infile[line][spine]);
	}
	char tokenbuffer[128] = {0};
	infile[line].getToken(tokenbuffer, spine, token);
	int state = 0;
	int length = strlen(tokenbuffer);
	int i;
	for (i=0; i<length; i++) {
		if (state == 0) {
			if ((tolower(tokenbuffer[i]) == 'a') ||
					(tolower(tokenbuffer[i]) == 'b') ||
					(tolower(tokenbuffer[i]) == 'c') ||
					(tolower(tokenbuffer[i]) == 'd') ||
					(tolower(tokenbuffer[i]) == 'e') ||
					(tolower(tokenbuffer[i]) == 'f') ||
					(tolower(tokenbuffer[i]) == 'g') ||
					(tolower(tokenbuffer[i]) == '-') ||
					(tolower(tokenbuffer[i]) == '#') ||
					(tolower(tokenbuffer[i]) == 'n')) {
				state = 1;
				prebuffer[i] = '\0';
			} else {
				prebuffer[i] = tokenbuffer[i];
			}
		} else if (state == 1) {
			if ((tolower(tokenbuffer[i]) == 'a') ||
					(tolower(tokenbuffer[i]) == 'b') ||
					(tolower(tokenbuffer[i]) == 'c') ||
					(tolower(tokenbuffer[i]) == 'd') ||
					(tolower(tokenbuffer[i]) == 'e') ||
					(tolower(tokenbuffer[i]) == 'f') ||
					(tolower(tokenbuffer[i]) == 'g') ||
					(tolower(tokenbuffer[i]) == '-') ||
					(tolower(tokenbuffer[i]) == '#') ||
					(tolower(tokenbuffer[i]) == 'n')) {
			} else {
				strcpy(postbuffer, &tokenbuffer[i]);
				state = 2;
				break;
			}
		} else {
			strcpy(postbuffer, &tokenbuffer[i]);
			break;
		}
	}

	// int newlen = strlen(prebuffer) + strlen(pbuffer) + strlen(postbuffer);

	// strcpy(newitem, prebuffer);
	strcat(prebuffer, pbuffer);
	strcat(prebuffer, postbuffer);
	record.changeToken(spine, token, prebuffer);

	if (strchr(prebuffer, '[') != NULL) {
		updateTiedNotes(infile, line, spine, token, newpitch, oldpitch);
	}
}



//////////////////////////////
//
// scrambleNotes --
//

void scrambleNotes(vector<NoteUnit>& notes) {
	vector<NoteUnit> notes2;
	notes2 = notes;
	int i;
	for (i=0; i<(int)notes2.size(); i++) {
		notes2[i].random = rand();
	}

	if (voiceQ) {
		qsort(notes.data(), notes.size(), sizeof(NoteUnit),
			compareNoteSortTrack);
		qsort(notes2.data(), notes2.size(), sizeof(NoteUnit),
			compareNoteUnitTrack);
	} else {
		qsort(notes2.data(), notes2.size(), sizeof(NoteUnit),
			compareNoteUnit);
	}

	if (neighborQ) {
		for (i=0; i<(int)notes.size(); i++) {
			notes[i].newpitch = nearestNeighbor(notes[i].pitch, notes2[i].pitch);
		}
	} else {
		for (i=0; i<(int)notes.size(); i++) {
			notes[i].newpitch = notes2[i].pitch;
		}
	}
}



//////////////////////////////
//
// getNotes --
//

void getNotes(vector<NoteUnit>& notes, HumdrumFile& infile) {
	notes.resize(0);
	notes.reserve(100000);
	NoteUnit tempnote;
	int tokencount;
	int track = 0;
	string buffer;
	int i, j, k;
	int dice = 0;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (strcmp("**kern", infile[i].getExInterp(j)) != 0) {
				continue;
			}
			tokencount = infile[i].getTokenCount(j);
			track = infile[i].getPrimaryTrack(j);
			for (k=0; k<tokencount; k++) {
				infile[i].getToken(buffer, j, k);
				if (strcmp(buffer.c_str(), ".") == 0) {
					continue;
				} else {
				}
				if (buffer.find('_') != std::string::npos) {
					continue;
				}
				if (buffer.find(']') != std::string::npos) {
					continue;
				}
				tempnote.pitch = Convert::kernToBase40(buffer);
				if (restQ == 0 && tempnote.pitch < 0) {
					continue;
				}
				if (distQ) {
					dice = throwDice((double)infile[i].getAbsBeat()/
							infile.getTotalDuration());
					if (!dice) {
						continue;
					}
				}
				tempnote.line  = i;
				tempnote.spine = j;
				tempnote.token = k;
				tempnote.track = track;
				notes.push_back(tempnote);
			}
		}
	}
}



//////////////////////////////
//
// printNotes --
//

void printNotes(vector<NoteUnit>& notes) {
	int i;
	for (i=0; i<(int)notes.size(); i++) {
		cout << "P" << notes[i].pitch << "\tL" << notes[i].line << "\tS"
			  << notes[i].spine << "\tT" << notes[i].token
			  << "\tR" << notes[i].track
			  << "\tN" << notes[i].newpitch
			  << "\n";
	}
}



//////////////////////////////
//
// checkOptions --
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("debug=b",          "print debug information");
	opts.define("r|rests=b",        "mixup rest as well as pitches");
	opts.define("s|seed=i:0",       "seed the random number generator");
	opts.define("v|voice|track=b",  "randomize by voice");
	opts.define("m|mapping=b",      "display mapping of pitches");
	opts.define("d|distribution=s:0 1 1 1", "random mixing amount in file");
	opts.define("t|n|transpose|neighbor=b",
			"move random note octave to be near old note ");
	opts.define("S|no-display-seed=b",
			"do not print seed used in random number generator");
	opts.define("author=b",  "author of program");
	opts.define("version=b", "compilation info");
	opts.define("example=b", "example usages");
	opts.define("h|help=b",  "short description");
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, May 2002" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 5 May 2002" << endl;
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

	if (opts.getBoolean("distribution")) {
		distring.setEnvelope(opts.getString("distribution").data());
		distQ = 1;
	} else {
		distQ = 0;
//      distring.setEnvelope("0 1 1 0 2 1");
//      distQ = 1;
	}
	voiceQ       =  opts.getBoolean("voice");
	displaySeedQ = !opts.getBoolean("no-display-seed");
	transQ       =  opts.getBoolean("mapping");
	neighborQ    =  opts.getBoolean("neighbor");
	debugQ       = opts.getBoolean("debug");
	restQ        = opts.getBoolean("rests");
	seed         = opts.getInteger("seed");
	if (seed <= 0) {
		seed = time(NULL);
		srand(seed);
	} else {
		srand(seed);
	}
}



//////////////////////////////
//
// example --
//

void example(void) {


}



//////////////////////////////
//
// usage --
//

void usage(const string& command) {

}



//////////////////////////////
//
// compareNoteUnit -- sort by random number.
//

int compareNoteUnit(const void* A, const void* B) {
	NoteUnit& a = *((NoteUnit*)A);
	NoteUnit& b = *((NoteUnit*)B);

	if (a.random < b.random) {
		return -1;
	} else if (a.random > b.random) {
		return 1;
	} else {
		return 0;
	}
}




//////////////////////////////
//
// compareNoteUnitTrack -- sort by random number within each track
//

int compareNoteUnitTrack(const void* A, const void* B) {
	NoteUnit& a = *((NoteUnit*)A);
	NoteUnit& b = *((NoteUnit*)B);

	if (a.track < b.track) {
		return -1;
	} else if (a.track > b.track) {
		return 1;
	}

	if (a.random < b.random) {
		return -1;
	} else if (a.random > b.random) {
		return 1;
	} else {
		return 0;
	}
}



//////////////////////////////
//
// compareNoteUnitTrack -- sort by random number within each track
//

int compareNoteSortTrack(const void* A, const void* B) {
	NoteUnit& a = *((NoteUnit*)A);
	NoteUnit& b = *((NoteUnit*)B);

	if (a.track < b.track) {
		return -1;
	} else if (a.track > b.track) {
		return 1;
	} else {
		return 0;
	}
}



//////////////////////////////
//
// updatedTiedNotes -- change ending tied notes to the new random pitch
//

void updateTiedNotes(HumdrumFile& infile, int line, int spine, int token,
	int newpitch, int oldpitch) {

// not quite perfect: if two primary tracks with common ties, will have prob:

	int m;
	int ptrack = infile[line].getPrimaryTrack(spine);
	int currentLine = line + 1;
	int length = infile.getNumLines();
	int done = 0;
	// int matchpitch;

	while (!done && currentLine < length) {
		if (!infile[currentLine].isData()) {
			currentLine++;
			continue;
		}

		for (m=0; m<infile[currentLine].getFieldCount(); m++) {
			if (ptrack != infile[currentLine].getPrimaryTrack(m)) {
				continue;
			}

			if (strchr(infile[currentLine][m], '_')) {
				// switch pitches (single tokens only for now)
				replaceNote(infile, currentLine, m, 0, newpitch, oldpitch);
				break;
			} else if (strchr(infile[currentLine][m], ']')) {
				replaceNote(infile, currentLine, m, 0, newpitch, oldpitch);
				done = 1;
				break;
			}
		}
		currentLine++;
	}

}



//////////////////////////////
//
// nearestNeighbor -- find the closest pitch class of the newpitch to
//    the oldpitches octave location
//

int nearestNeighbor(int oldpitch, int newpitch) {
	if (newpitch < 0 || oldpitch < 0) {
		return newpitch;
	}
	int octave = oldpitch / 40;
	int pc = newpitch % 40;
	int pitch0 = pc + octave * 40;
	int pitch1 = pc + (octave-1) * 40;
	int pitch2 = pc + (octave+1) * 40;
	int diff0 = abs(pitch0 - oldpitch);
	int diff1 = abs(pitch1 - oldpitch);
	int diff2 = abs(pitch2 - oldpitch);

	if ((diff0<diff1) && (diff0<diff2)) {
		if (pitch0 < lowlimit) {
			return pitch0 + 40;
		} else {
			return pitch0;
		}
	}
	if (diff1<diff2) {
		if (pitch1 < lowlimit) {
			return pitch1 + 40;
		} else {
			return pitch1;
		}
	}
	if (pitch2 < lowlimit) {
		return pitch2 + 40;
	} else {
		return pitch2;
	}
}



//////////////////////////////
//
// throwDice --
//

int throwDice(double piecefraction) {
	if (piecefraction < 0.0) {
		piecefraction = 0.0;
	} else if (piecefraction > 1.0) {
		piecefraction = 1.0;
	}

	// find the current point
	int i;
	int curr = distring.getNumPoints() - 1;
	for (i=0; i<distring.getNumPoints(); i++) {
		if (piecefraction <= distring.getValue(i, 0)) {
			curr = i;
			break;
		}
	}

	double fraction = 1.0;
	double slope = 0;
	double y1 = 0;
	double y2 = 0;
	double x1 = 0;
	double x2 = 0;
	if (curr > distring.getNumPoints() - 1) {
		fraction = distring.getValue(distring.getNumPoints()-1, 1);
	} else if (curr <= 0) {
		fraction = distring.getValue(0, 1);
	} else if (distring.getValue(curr, 0) == piecefraction) {
		fraction = distring.getValue(curr, 1);
	} else {
		x1 = distring.getValue(curr-1, 0);
		x2 = distring.getValue(curr,   0);

		y1 = distring.getValue(curr-1, 1);
		y2 = distring.getValue(curr,   1);

		slope = (y2-y1)/(x2-x1);
		fraction = slope * piecefraction + y2 - slope * x2;
	}

	#ifndef VISUAL
		if (drand48() > fraction) {
			return 0;
		} else {
			return 1;
		}
	#else
		if ((1.0*rand()/INT_MAX) > fraction) {
			return 0;
		} else {
			return 1;
		}
	#endif
}



