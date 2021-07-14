//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Mar 17 07:46:28 PDT 2011
// Last Modified: Thu Mar 17 07:46:35 PDT 2011
// Last Modified: Mon Apr  1 12:06:53 PDT 2013 Enabled multiple segment input
// Last Modified: Mon Apr 15 18:24:31 PDT 2013 Added subset forms
// Last Modified: Tue Apr 16 23:04:00 PDT 2013 Added attack-only analysis
// Last Modified: Sat Oct 26 20:56:11 PDT 2013 Added --index option
// Filename:      ...sig/examples/all/tntype.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/tntype.cpp
// Syntax:        C++; museinfo
//
// Description:   Analyzes **kern data with serial descriptions.
// Reference:     http://solomonsmusic.net/pcsets.htm
//

#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "humdrum.h"
#include "PerlRegularExpression.h"

#define NORM_OPEN   "["
#define NORM_CLOSE  "]"
#define PRIME_OPEN  "("
#define PRIME_CLOSE ")"
#define TN_OPEN     "{"
#define TN_CLOSE    "}"
#define IV_OPEN     "<"
#define IV_CLOSE    ">"


// function declarations
void        checkOptions          (Options& opts, int argc, char* argv[]);
void        example               (void);
void        processRecords        (HumdrumFile& infile);
void        usage                 (const char* command);
void        fillStringWithNotes   (char* string, ChordQuality& quality,
                                   HumdrumFile& infile, int line);
int         identifyBassNote      (SigCollection<int>& notes,
                                   HumdrumFile& infile, int line,
                                   vector<int>& sounding);
int         transitionalSonority  (ChordQuality& quality, HumdrumFile& infile,
                                   int line);
double      getMeasureSize        (HumdrumFile& infile, int width);
void        printAttackMarker     (HumdrumFile& infile, int line);
void        printRotation         (HumdrumFile& infile, int line);
const char* getDescription        (const char* tntype);
int         getIndex              (const char* tntype);

// global variables
Options  options;            // database for command-line arguments
char     unknown[256] = {0}; // space for unknown chord simplification
int      chordinit;          // for initializing chord detection function
int      notesQ    = 0;      // used with --notes option
int      suppressQ = 0;      // used with -s option
int      transQ    = 0;      // used with -T option
int      parenQ    = 1;      // used with -P option
int      ivQ       = 0;      // used with --iv option
int      infoQ     = 0;      // used with -D option
int      forteQ    = 0;      // used with --forte option
int      tnQ       = 0;      // used with --tn option
int      normQ     = 0;      // used with -n option
int      subsetQ   = 0;      // used with -k option
int      attackQ   = 0;      // used with -A option
int      tnormQ    = 0;      // used with -t option
int      verboseQ  = 0;      // used with -v option
int      tniQ      = 0;      // used with --tni option
int      rotationQ = 0;      // used with -r option
int      xsattackQ = 0;      // used with -x option
int      appendQ   = 0;      // used with -a option
int      indexQ    = 0;      // used with --index option
string   notesep   = " ";    // used with -N option
string   colorindex[26];


///////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
	HumdrumFileSet infiles;

	// process the command-line options
	checkOptions(options, argc, argv);

	// figure out the number of input files to process
	int numinputs = options.getArgCount();

	if (numinputs < 1) {
		infiles.read(cin);
	} else {
		for (int i=0; i<numinputs; i++) {
			infiles.readAppend(options.getArg(i+1));
		}
	}

	for (int i=0; i<infiles.getCount(); i++) {
		chordinit = 1;
		processRecords(infiles[i]);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("a|assemble|append=b", "append analysis to input data");
	opts.define("debug=b",      "determine bad input line num");
	opts.define("d|notes=b",    "display pitch classes in sonority");
	opts.define("n|norm=b",     "display normal form of pitch sets");
	opts.define("D|description=b", "display musical Description of Tn type");
	opts.define("k|combinations|subsets=b", "display all subset forms");
	opts.define("A|attacks|attack=b", "consider only note attaks");
	opts.define("f|form|tnorm=b", "display transposed normal form of pitch sets");
	opts.define("i|iv|ic=b",    "print interval vector");
	opts.define("index=b",      "print Tn-type index");
	opts.define("x|sonor|suspension=b", "print marker if not all start attacks");
	opts.define("F|forte=b",      "print forte interval vector set name");
	opts.define("r|rotation=b", "mark which note is in the bass");
	opts.define("t|transpose=b", "indicate the tranposition value for Tn form");
	opts.define("Tn|tn=b",      "print forte set with subsets");
	opts.define("Tni|tni=b",    "print forte set with subsets/inversion");
	opts.define("v|verbose=b",  "print verbose label with extra info");
	opts.define("s|suppress=b", "suppress data if overlapping sonority");
	opts.define("S|paren-off=b","suppress parentheses for overlapping");

	opts.define("author=b",     "author of program");
	opts.define("version=b",    "compilation info");
	opts.define("example=b",    "example usages");
	opts.define("h|help=b",     "short description");
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, Mar 2011" << endl;
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

	ivQ       =  opts.getBoolean("iv");
	transQ    =  opts.getBoolean("transpose");
	xsattackQ =  opts.getBoolean("suspension");
	infoQ     =  opts.getBoolean("description");
	forteQ    =  opts.getBoolean("forte");
	rotationQ =  opts.getBoolean("rotation");
	tnQ       =  opts.getBoolean("Tn");
	subsetQ   =  opts.getBoolean("combinations");
	attackQ   =  opts.getBoolean("attacks");
	verboseQ  =  opts.getBoolean("verbose");
	if (tnQ) {
		forteQ = 1;
	}
	tniQ      =  opts.getBoolean("Tni");
	if (tniQ) {
		tnQ = 1;
		forteQ = 1;
	}
	normQ     =  opts.getBoolean("norm");
	tnormQ    =  opts.getBoolean("tnorm");
	notesQ    =  opts.getBoolean("notes");
	suppressQ =  opts.getBoolean("suppress");
	parenQ    = !opts.getBoolean("paren-off");
	appendQ   =  opts.getBoolean("append");
	indexQ    =  opts.getBoolean("index");
}



//////////////////////////////
//
// example -- example usage of the sonority program
//

void example(void) {
	cout <<
	"                                                                         \n"
	"# example usage of the sonority program.                                 \n"
	"# analyze a Bach chorale for chord qualities:                            \n"
	"     sonority chor217.krn                                                \n"
	"                                                                         \n"
	"# display the chord analysis with original data:                         \n"
	"     sonority -a chor217.krn                                             \n"
	"                                                                         \n"
	"# display only the roots of chords:                                      \n"
	"     sonority -r chor217.krn                                             \n"
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// processRecords -- looks at humdrum records and determines pitch-class sets
//      from sonorities made of simultaneously sounding notes.
//

void processRecords(HumdrumFile& infile) {
	vector<ChordQuality> cq;
	infile.analyzeSonorityQuality(cq);
	ChordQuality quality;
	string data;
	PerlRegularExpression pre;

	int foundstart = 0;
	char aString[512] = {0};

	infile.printNonemptySegmentLabel(cout);

	for (int i=0; i<infile.getNumLines(); i++) {
		if (options.getBoolean("debug")) {
			cout << "processing line " << (i+1) << " of input ..." << endl;
			cout << "LINE IS: " << infile[i] << endl;
		}
		switch (infile[i].getType()) {
			case E_humrec_none:
			case E_humrec_empty:
			case E_humrec_bibliography:
			case E_humrec_global_comment:
				cout << infile[i] << endl;
				break;
			case E_humrec_data_comment:
				if (appendQ) {
					cout << infile[i] << "\t";
				}
				if (infile[i].equalFieldsQ("**kern")) {
					cout << infile[i][0];
				} else {
					cout << "!";
				}
				cout << endl;
				break;
			case E_humrec_data_interpretation:
				if (appendQ) {
					cout << infile[i] << "\t";
				}
				if (!foundstart && infile[i].hasExclusiveQ()) {
					foundstart = 1;
					if (tniQ) {
						cout << "**Tni";
					} else if (tnQ) {
						cout << "**Tn";
					} else if (forteQ) {
						cout << "**forte";
					} else if (tnormQ) {
						cout << "**tnf";
					} else if (notesQ) {
						cout << "**dpc";
					} else if (normQ) {
						cout << "**nf";
					} else if (ivQ) {
						cout << "**iv";
					} else if (infoQ) {
						cout << "**description";
					} else if (indexQ) {
						cout << "**tnidx";
					} else {
						cout << "**tnt";
					}
				} else {
					if (infile[i].equalFieldsQ("**kern") &&
							(!infile[i].isSpineManipulator(0))) {
						cout << infile[i][0];
					} else {
						cout << "*";
					}
				}
				cout << endl;
				break;
			case E_humrec_data_kern_measure:
				if (appendQ) {
					cout << infile[i] << "\t";
				}
				cout << infile[i][0];
				cout << endl;
				break;
			case E_humrec_data:
				if (appendQ) {
					cout << infile[i] << "\t";
				}
				// handle null fields
				if (infile[i].equalFieldsQ("**kern", ".")) {
					cout << "." << endl;
					break;
				}
				if (ivQ) {
					vector<int> iv;
					infile.getIntervalVector(iv, i);
					cout << IV_OPEN;
					for (int ii=0; ii<(int)iv.size(); ii++) {
						if (iv[ii] < 9) {
							cout << iv[ii];
							continue;
						}
						if (iv[ii] < 36) {
							cout << char(iv[ii]-10+'A');
							continue;
						}
						if (ii > 0) {
							cout << ",";
						}
						cout << iv[ii];
						if (ii < 5) {
							cout << ",";
						}
					}
					cout << IV_CLOSE << endl;

				} else if (forteQ) {
					const char* name = infile.getForteSetName(i);
					cout << name;
					if (tnQ) {
						if (strcmp(name, "3-11") == 0) {
							if (strcmp(cq[i].getTypeName(), "min") == 0) {
								cout << "A";
							} else if (strcmp(cq[i].getTypeName(), "maj") == 0) {
								cout << "B";
							}
						}
					}
					if (tniQ) {
						int inversion = -1;
						if (strcmp(name, "3-11") == 0) {
							if ((strcmp(cq[i].getTypeName(), "min") == 0) ||
									(strcmp(cq[i].getTypeName(), "maj") == 0)) {
								inversion = cq[i].getInversion();
							}
							if (inversion >= 0) {
								cout << char('a'+inversion);
							}
						}
					}
					if (xsattackQ) {
						printAttackMarker(infile, i);
					}
					cout << endl;

				} else if (normQ) {

					vector<int> norm;
					infile.getNormalForm(norm, i);
					cout << NORM_OPEN;
					for (int ii=0; ii<(int)norm.size(); ii++) {
						if (norm[ii] < 10) {
							cout << norm[ii];
						} else {
							cout << char('A' + norm[ii] - 10);
						}
					}
					cout << NORM_CLOSE;
					if (rotationQ) {
						printRotation(infile, i);
					}
					if (xsattackQ) {
						printAttackMarker(infile, i);
					}

					cout << endl;

				} else if (tnormQ) {
					vector<int> norm;
					infile.getNormalForm(norm, i);
					cout << TN_OPEN;
					int value;
					for (int ii=0; ii<(int)norm.size(); ii++) {
						value = (norm[ii] - norm[0] + 144) % 12;
						if (value < 10) {
							cout << value;
						} else {
							cout << char('A' + value - 10);
						}
					}
					cout << TN_CLOSE;
					if (rotationQ) {
						printRotation(infile, i);
					}
					if (xsattackQ) {
						printAttackMarker(infile, i);
					}
					if (transQ) {
						if (norm.size() > 0) {
							cout << "T";
							if (norm[0] < 10) {
								cout << "0";
							}
							cout << norm[0];
						}
					}
					cout << endl;

				} else if (notesQ) {
					quality = cq[i];
					fillStringWithNotes(aString, quality, infile, i);
					cout << aString << endl;
				} else if (infoQ) {
					string tnname = infile.getTnSetName(i, attackQ);
					data = tnname;
					pre.sar(data, "Z", "", "g");
					if (pre.search(data, "^(\\d+-\\d+[AB]?)", "")) {
						cout << getDescription(pre.getSubmatch(1));
					} else {
						cout << ".";
					}
					cout << endl;

				} else if (indexQ) {
					string tnname = infile.getTnSetName(i, attackQ);
					data = tnname;
					pre.sar(data, "Z", "", "g");
					if (pre.search(data, "^(\\d+-\\d+[AB]?)", "")) {
						cout << getIndex(pre.getSubmatch(1));
					} else {
						cout << ".";
					}
					cout << endl;
				} else if (subsetQ) {
					vector<int> tnnames;
					infile.getTnSetNameAllSubsets(tnnames, i, attackQ);
					int cardinality;
					int enumeration;
					int inversion;
					// int transpose;
					for (int ii=(int)tnnames.size()-1; ii>=0; ii--) {
						// tntypes are sorted from smallest subset to largest
						// but want to print from largest to smallest
						// 302200 --> 3-2B
						cardinality = (tnnames[ii] % 10000000) / 100000;
						enumeration = (tnnames[ii] % 100000  ) / 1000;
						inversion   = (tnnames[ii] % 1000    ) / 100;
						// transpose   = (tnnames[ii] % 100);
						cout << cardinality << "-" << enumeration;
						switch (inversion) {
							case 1: cout << "A"; break;
							case 2: cout << "B"; break;
						}
						// if (transQ) ...
						if (ii > 0) {
							cout << " ";
						}
					}

					// if (rotationQ) {
					//    printRotation(infile, i);
					// }
					// if (xsattackQ) {
					//    printAttackMarker(infile, i);
					// }
					// if (transQ) {
					//    vector<int> norm;
					//    infile.getNormalForm(norm, i);
					//    if (norm.size() > 0) {
					//       cout << "T";
					//       if (norm[0] < 10) {
					//          cout << "0";
					//       }
					//       cout << norm[0];
					//    }
					// }
					cout << endl;

				} else {
					string tnname = infile.getTnSetName(i, attackQ);
					if (verboseQ) {
						cout << tnname;
					} else {
						data = tnname;
						if (pre.search(data, "^(\\d+-\\d+[AB]?)", "")) {
							cout << pre.getSubmatch(1);
						} else if (pre.search(data, "^(\\d+-)Z(\\d+[AB]?)", "")) {
							cout << pre.getSubmatch(1);
							cout << pre.getSubmatch(2);
						} else {
							cout << tnname;
						}
					}
					if (rotationQ) {
						printRotation(infile, i);
					}
					if (xsattackQ) {
						printAttackMarker(infile, i);
					}
					if (transQ) {
						vector<int> norm;
						infile.getNormalForm(norm, i);
						if (norm.size() > 0) {
							cout << "T";
							if (norm[0] < 10) {
								cout << "0";
							}
							cout << norm[0];
						}
					}
					cout << endl;
				}

				break;
			default:
				cerr << "Error on line " << (i+1) << " of input" << endl;
				cerr << "record type = " << infile[i].getType() << endl;
				exit(1);
		}
	}

}



//////////////////////////////
//
// printRotation --
//

void printRotation(HumdrumFile& infile, int line) {
	vector<int> base12;
	infile.getBase12PitchList(base12, line);
	int i;
	if (base12.size() == 0) {
		return;
	}
	int min = base12[0];
	for (i=1; i<(int)base12.size(); i++) {
		if (base12[i] < min) {
			min = base12[i];
		}
	}
	min = min % 12;

	vector<int> norm;
	infile.getNormalForm(norm, line);

	for (i=0; i<(int)norm.size(); i++) {
		if (norm[i] == min)  {
			cout << char('a' + i);
			return;
		}
	}
	cout << '?';
}



//////////////////////////////
//
// printAttackMarker -- print an "s" if the there is any note at
//    the start of the region which is not attacked (i.e., suspended
//    from a previous sonority; otherwise, print an "x" which means
//    that all notes in the sonority are attacked at the start of the
//    region.
//

void printAttackMarker(HumdrumFile& infile, int line) {
	int j, ii, jj;
	int& i = line;
	int dotQ;
	for (j=0; j<infile[i].getFieldCount(); j++) {
		if (!infile[i].isExInterp(j, "**kern")) {
			continue;
		}
		if (strcmp(infile[i][j], ".") == 0) {
			ii = infile[i].getDotLine(j);
			jj = infile[i].getDotSpine(j);
			if (ii < 0 || jj < 0) {
				continue;
			}
			dotQ = 1;
		} else {
			ii = i;
			jj = j;
			dotQ = 0;
		}
		if (strchr(infile[ii][jj], 'r') != NULL) {
			continue;
		} else if (dotQ) {
			cout << "s";
			return;
		}
		if (strchr(infile[ii][jj], '_') != NULL) {
			cout << "s";
			return;
		}
		if (strchr(infile[ii][jj], ']') != NULL) {
			cout << "s";
			return;
		}
	}
	cout << "x";

}



//////////////////////////////
//
// transitionalSonority --
//

int transitionalSonority(ChordQuality& quality, HumdrumFile& infile, int line) {
	SigCollection<int> notes;
	quality.getNotesInChord(notes);
	vector<int> octave(notes.getSize(), 0);
	vector<int> sounding(notes.getSize(), 0);
	// int bassindex = identifyBassNote(notes, infile, line, sounding);
	for (int i=0; i<(int)sounding.size(); i++) {
		if (sounding[i] == 0) {
			return 1;
		}
	}
	return 0;
}



//////////////////////////////
//
// fillStringWithNotes --
//

void fillStringWithNotes(char* astring, ChordQuality& quality,
		HumdrumFile& infile, int line) {

	astring[0] = '\0';

	SigCollection<int> notes;
	quality.getNotesInChord(notes);

	vector<int> octave(notes.getSize(), 4);
	vector<int> sounding(notes.getSize(), 0);

	int bassindex = identifyBassNote(notes, infile, line, sounding);
	if (bassindex >= 0) {
		octave[bassindex] = 3;
		//if (notes[bassindex] >= 40) {
		//   octave[bassindex] += -2;
		//}
	}

	int i;
	if (suppressQ) {
		for (i=0; i<(int)sounding.size(); i++) {
			if (sounding[i] == 0) {
				strcpy(astring, ".");
				return;
			}
		}
	}

	astring[0] = '\0';
	char buffer[32] = {0};
	for (i=0; i<(int)notes.getSize(); i++) {
		//if (octaveVal >= 0) {
		//   Convert::base40ToKern(buffer, (notes[i]%40) + octaveVal * 40);
		//} else {
		//   Convert::base40ToKern(buffer, notes[i] + ((octave[i]+4) * 40));
		//}
		Convert::base40ToKern(buffer, notes[i]%40 + (octave[i] * 40));
		if (parenQ && (sounding[i] == 0)) {
			strcat(astring, "(");
		}
		strcat(astring, buffer);
		if (parenQ && (sounding[i] == 0)) {
			strcat(astring, ")");
		}
		if (i < (int)notes.getSize() - 1) {
			strcat(astring, notesep.c_str());
		}
	}

}



//////////////////////////////
//
// identifyBassnote --
//

int identifyBassNote(SigCollection<int>& notes, HumdrumFile& infile,
		int line, vector<int>& sounding) {
	int j, k;
	int output = -1;
	int minval = 1000000;
	int value;
	int tcount;
	char buffer[128] = {0};
	int pline;
	int pspine;
	vector<int> soundQ(40, 0);
	int dotQ = 0;

	sounding.resize(notes.getSize());
	std::fill(sounding.begin(), sounding.end(), 0);

	if (notes.getSize() == 0) {
		return -1;
	}

	for (j=0; j<infile[line].getFieldCount(); j++) {
		if (!infile[line].isExInterp(j, "**kern")) {
			continue;
		}
		dotQ = 0;
		if (strcmp(infile[line][j], ".") == 0) {
			pline  = infile[line].getDotLine(j);
			pspine = infile[line].getDotSpine(j);
			if (pline < 0 || pspine < 0) {
				continue;
			}
			dotQ = 1;
		} else {
			pline = line;
			pspine = j;
		}
		tcount = infile[pline].getTokenCount(pspine);
		for (k=0; k<tcount; k++) {
			infile[pline].getToken(buffer, pspine, k);
			if (strchr(buffer, 'r') != NULL) {
				continue;
			}
	if (strcmp(buffer, ".") == 0) {
				// shouldn't get here...
				continue;
			}
	value = Convert::kernToMidiNoteNumber(buffer);
			if (value < 0) {
				continue;
			}
			if (value < minval) {
				minval = value;
			}
			if (dotQ) {
				continue;
			}
			if (strchr(buffer, '_') != NULL) {
				continue;
			}
			if (strchr(buffer, ']') != NULL) {
				continue;
			}
	value = Convert::kernToBase40(buffer);
			if (value < 0) {
				continue;
			}
			soundQ[value % 40] = 1;
		}
	}

	if (minval > 100000) {
		return -1;
	}

	minval = minval % 12;
	int i;
	int tval;
	for (i=0; i<(int)notes.getSize(); i++) {
		if (notes[i] >= 0) {
			if (soundQ[notes[i]%40]) {
				sounding[i] = 1;
			}
		}
		tval = Convert::base40ToMidiNoteNumber(notes[i]);
		if (tval < 0) {
			continue;
		}
		tval = tval % 12;
		if (tval == minval) {
			output = i;
			// break;  need to supress this because of sounding tests
		}
	}
	return output;
}



//////////////////////////////
//
// getIndex -- return a tn-type index number when given its
//     Forte number.
//

int getIndex(const char* tntype) {
	int cardinality = 0;
	if (!sscanf(tntype, "%d", &cardinality)) {
		return -1;
	}
	switch (cardinality) {
		case 0:
			if (strcmp(tntype, "0-1") == 0)	return 0;
			break;

		case 1:
			if (strcmp(tntype, "1-1") == 0)	return 1;
			break;

		case 2:
			if (strcmp(tntype, "2-1") == 0)	return 2;
			if (strcmp(tntype, "2-2") == 0)	return 3;
			if (strcmp(tntype, "2-3") == 0)	return 4;
			if (strcmp(tntype, "2-4") == 0)	return 5;
			if (strcmp(tntype, "2-5") == 0)	return 6;
			if (strcmp(tntype, "2-6") == 0)	return 7;
			break;

		case 3:
			if (strcmp(tntype, "3-1") == 0)	return 8;
			if (strcmp(tntype, "3-2A") == 0)	return 9;
			if (strcmp(tntype, "3-2B") == 0)	return 10;
			if (strcmp(tntype, "3-3A") == 0)	return 11;
			if (strcmp(tntype, "3-3B") == 0)	return 12;
			if (strcmp(tntype, "3-4A") == 0)	return 13;
			if (strcmp(tntype, "3-4B") == 0)	return 14;
			if (strcmp(tntype, "3-5A") == 0)	return 15;
			if (strcmp(tntype, "3-5B") == 0)	return 16;
			if (strcmp(tntype, "3-6") == 0)	return 17;
			if (strcmp(tntype, "3-7A") == 0)	return 18;
			if (strcmp(tntype, "3-7B") == 0)	return 19;
			if (strcmp(tntype, "3-8A") == 0)	return 20;
			if (strcmp(tntype, "3-8B") == 0)	return 21;
			if (strcmp(tntype, "3-9") == 0)	return 22;
			if (strcmp(tntype, "3-10") == 0)	return 23;
			if (strcmp(tntype, "3-11A") == 0)	return 24;
			if (strcmp(tntype, "3-11B") == 0)	return 25;
			if (strcmp(tntype, "3-12") == 0)	return 26;
			break;

		case 4:
			if (strcmp(tntype, "4-1") == 0)	return 27;
			if (strcmp(tntype, "4-2A") == 0)	return 28;
			if (strcmp(tntype, "4-2B") == 0)	return 29;
			if (strcmp(tntype, "4-3") == 0)	return 30;
			if (strcmp(tntype, "4-4A") == 0)	return 31;
			if (strcmp(tntype, "4-4B") == 0)	return 32;
			if (strcmp(tntype, "4-5A") == 0)	return 33;
			if (strcmp(tntype, "4-5B") == 0)	return 34;
			if (strcmp(tntype, "4-6") == 0)	return 35;
			if (strcmp(tntype, "4-7") == 0)	return 36;
			if (strcmp(tntype, "4-8") == 0)	return 37;
			if (strcmp(tntype, "4-9") == 0)	return 38;
			if (strcmp(tntype, "4-10") == 0)	return 39;
			if (strcmp(tntype, "4-11A") == 0)	return 40;
			if (strcmp(tntype, "4-11B") == 0)	return 41;
			if (strcmp(tntype, "4-12A") == 0)	return 42;
			if (strcmp(tntype, "4-12B") == 0)	return 43;
			if (strcmp(tntype, "4-13A") == 0)	return 44;
			if (strcmp(tntype, "4-13B") == 0)	return 45;
			if (strcmp(tntype, "4-14A") == 0)	return 46;
			if (strcmp(tntype, "4-14B") == 0)	return 47;
			if (strcmp(tntype, "4-15A") == 0)	return 48;
			if (strcmp(tntype, "4-15B") == 0)	return 49;
			if (strcmp(tntype, "4-16A") == 0)	return 50;
			if (strcmp(tntype, "4-16B") == 0)	return 51;
			if (strcmp(tntype, "4-17") == 0)	return 52;
			if (strcmp(tntype, "4-18A") == 0)	return 53;
			if (strcmp(tntype, "4-18B") == 0)	return 54;
			if (strcmp(tntype, "4-19A") == 0)	return 55;
			if (strcmp(tntype, "4-19B") == 0)	return 56;
			if (strcmp(tntype, "4-20") == 0)	return 57;
			if (strcmp(tntype, "4-21") == 0)	return 58;
			if (strcmp(tntype, "4-22A") == 0)	return 59;
			if (strcmp(tntype, "4-22B") == 0)	return 60;
			if (strcmp(tntype, "4-23") == 0)	return 61;
			if (strcmp(tntype, "4-24") == 0)	return 62;
			if (strcmp(tntype, "4-25") == 0)	return 63;
			if (strcmp(tntype, "4-26") == 0)	return 64;
			if (strcmp(tntype, "4-27A") == 0)	return 65;
			if (strcmp(tntype, "4-27B") == 0)	return 66;
			if (strcmp(tntype, "4-28") == 0)	return 67;
			if (strcmp(tntype, "4-29A") == 0)	return 68;
			if (strcmp(tntype, "4-29B") == 0)	return 69;
			break;

		case 5:
			if (strcmp(tntype, "5-1") == 0)	return 70;
			if (strcmp(tntype, "5-2A") == 0)	return 71;
			if (strcmp(tntype, "5-2B") == 0)	return 72;
			if (strcmp(tntype, "5-3A") == 0)	return 73;
			if (strcmp(tntype, "5-3B") == 0)	return 74;
			if (strcmp(tntype, "5-4A") == 0)	return 75;
			if (strcmp(tntype, "5-4B") == 0)	return 76;
			if (strcmp(tntype, "5-5A") == 0)	return 77;
			if (strcmp(tntype, "5-5B") == 0)	return 78;
			if (strcmp(tntype, "5-6A") == 0)	return 79;
			if (strcmp(tntype, "5-6B") == 0)	return 80;
			if (strcmp(tntype, "5-7A") == 0)	return 81;
			if (strcmp(tntype, "5-7B") == 0)	return 82;
			if (strcmp(tntype, "5-8") == 0)	return 83;
			if (strcmp(tntype, "5-9A") == 0)	return 84;
			if (strcmp(tntype, "5-9B") == 0)	return 85;
			if (strcmp(tntype, "5-10A") == 0)	return 86;
			if (strcmp(tntype, "5-10B") == 0)	return 87;
			if (strcmp(tntype, "5-11A") == 0)	return 88;
			if (strcmp(tntype, "5-11B") == 0)	return 89;
			if (strcmp(tntype, "5-12") == 0)	return 90;
			if (strcmp(tntype, "5-13A") == 0)	return 91;
			if (strcmp(tntype, "5-13B") == 0)	return 92;
			if (strcmp(tntype, "5-14A") == 0)	return 93;
			if (strcmp(tntype, "5-14B") == 0)	return 94;
			if (strcmp(tntype, "5-15") == 0)	return 95;
			if (strcmp(tntype, "5-16A") == 0)	return 96;
			if (strcmp(tntype, "5-16B") == 0)	return 97;
			if (strcmp(tntype, "5-17") == 0)	return 98;
			if (strcmp(tntype, "5-18A") == 0)	return 99;
			if (strcmp(tntype, "5-18B") == 0)	return 100;
			if (strcmp(tntype, "5-19A") == 0)	return 101;
			if (strcmp(tntype, "5-19B") == 0)	return 102;
			if (strcmp(tntype, "5-20A") == 0)	return 103;
			if (strcmp(tntype, "5-20B") == 0)	return 104;
			if (strcmp(tntype, "5-21A") == 0)	return 105;
			if (strcmp(tntype, "5-21B") == 0)	return 106;
			if (strcmp(tntype, "5-22") == 0)	return 107;
			if (strcmp(tntype, "5-23A") == 0)	return 108;
			if (strcmp(tntype, "5-23B") == 0)	return 109;
			if (strcmp(tntype, "5-24A") == 0)	return 110;
			if (strcmp(tntype, "5-24B") == 0)	return 111;
			if (strcmp(tntype, "5-25A") == 0)	return 112;
			if (strcmp(tntype, "5-25B") == 0)	return 113;
			if (strcmp(tntype, "5-26A") == 0)	return 114;
			if (strcmp(tntype, "5-26B") == 0)	return 115;
			if (strcmp(tntype, "5-27A") == 0)	return 116;
			if (strcmp(tntype, "5-27B") == 0)	return 117;
			if (strcmp(tntype, "5-28A") == 0)	return 118;
			if (strcmp(tntype, "5-28B") == 0)	return 119;
			if (strcmp(tntype, "5-29A") == 0)	return 120;
			if (strcmp(tntype, "5-29B") == 0)	return 121;
			if (strcmp(tntype, "5-30A") == 0)	return 122;
			if (strcmp(tntype, "5-30B") == 0)	return 123;
			if (strcmp(tntype, "5-31A") == 0)	return 124;
			if (strcmp(tntype, "5-31B") == 0)	return 125;
			if (strcmp(tntype, "5-32A") == 0)	return 126;
			if (strcmp(tntype, "5-32B") == 0)	return 127;
			if (strcmp(tntype, "5-33") == 0)	return 128;
			if (strcmp(tntype, "5-34") == 0)	return 129;
			if (strcmp(tntype, "5-35") == 0)	return 130;
			if (strcmp(tntype, "5-36A") == 0)	return 131;
			if (strcmp(tntype, "5-36B") == 0)	return 132;
			if (strcmp(tntype, "5-37") == 0)	return 133;
			if (strcmp(tntype, "5-38A") == 0)	return 134;
			if (strcmp(tntype, "5-38B") == 0)	return 135;
			break;

		case 6:
			if (strcmp(tntype, "6-1") == 0)	return 136;
			if (strcmp(tntype, "6-2A") == 0)	return 137;
			if (strcmp(tntype, "6-2B") == 0)	return 138;
			if (strcmp(tntype, "6-3A") == 0)	return 139;
			if (strcmp(tntype, "6-3B") == 0)	return 140;
			if (strcmp(tntype, "6-4") == 0)	return 141;
			if (strcmp(tntype, "6-5A") == 0)	return 142;
			if (strcmp(tntype, "6-5B") == 0)	return 143;
			if (strcmp(tntype, "6-6") == 0)	return 144;
			if (strcmp(tntype, "6-7") == 0)	return 145;
			if (strcmp(tntype, "6-8") == 0)	return 146;
			if (strcmp(tntype, "6-9A") == 0)	return 147;
			if (strcmp(tntype, "6-9B") == 0)	return 148;
			if (strcmp(tntype, "6-10A") == 0)	return 149;
			if (strcmp(tntype, "6-10B") == 0)	return 150;
			if (strcmp(tntype, "6-11A") == 0)	return 151;
			if (strcmp(tntype, "6-11B") == 0)	return 152;
			if (strcmp(tntype, "6-12A") == 0)	return 153;
			if (strcmp(tntype, "6-12B") == 0)	return 154;
			if (strcmp(tntype, "6-13") == 0)	return 155;
			if (strcmp(tntype, "6-14A") == 0)	return 156;
			if (strcmp(tntype, "6-14B") == 0)	return 157;
			if (strcmp(tntype, "6-15A") == 0)	return 158;
			if (strcmp(tntype, "6-15B") == 0)	return 159;
			if (strcmp(tntype, "6-16A") == 0)	return 160;
			if (strcmp(tntype, "6-16B") == 0)	return 161;
			if (strcmp(tntype, "6-17A") == 0)	return 162;
			if (strcmp(tntype, "6-17B") == 0)	return 163;
			if (strcmp(tntype, "6-18A") == 0)	return 164;
			if (strcmp(tntype, "6-18B") == 0)	return 165;
			if (strcmp(tntype, "6-19A") == 0)	return 166;
			if (strcmp(tntype, "6-19B") == 0)	return 167;
			if (strcmp(tntype, "6-20") == 0)	return 168;
			if (strcmp(tntype, "6-21A") == 0)	return 169;
			if (strcmp(tntype, "6-21B") == 0)	return 170;
			if (strcmp(tntype, "6-22A") == 0)	return 171;
			if (strcmp(tntype, "6-22B") == 0)	return 172;
			if (strcmp(tntype, "6-23") == 0)	return 173;
			if (strcmp(tntype, "6-24A") == 0)	return 174;
			if (strcmp(tntype, "6-24B") == 0)	return 175;
			if (strcmp(tntype, "6-25A") == 0)	return 176;
			if (strcmp(tntype, "6-25B") == 0)	return 177;
			if (strcmp(tntype, "6-26") == 0)	return 178;
			if (strcmp(tntype, "6-27A") == 0)	return 179;
			if (strcmp(tntype, "6-27B") == 0)	return 180;
			if (strcmp(tntype, "6-28") == 0)	return 181;
			if (strcmp(tntype, "6-29") == 0)	return 182;
			if (strcmp(tntype, "6-30A") == 0)	return 183;
			if (strcmp(tntype, "6-30B") == 0)	return 184;
			if (strcmp(tntype, "6-31A") == 0)	return 185;
			if (strcmp(tntype, "6-31B") == 0)	return 186;
			if (strcmp(tntype, "6-32") == 0)	return 187;
			if (strcmp(tntype, "6-33A") == 0)	return 188;
			if (strcmp(tntype, "6-33B") == 0)	return 189;
			if (strcmp(tntype, "6-34A") == 0)	return 190;
			if (strcmp(tntype, "6-34B") == 0)	return 191;
			if (strcmp(tntype, "6-35") == 0)	return 192;
			if (strcmp(tntype, "6-36A") == 0)	return 193;
			if (strcmp(tntype, "6-36B") == 0)	return 194;
			if (strcmp(tntype, "6-37") == 0)	return 195;
			if (strcmp(tntype, "6-38") == 0)	return 196;
			if (strcmp(tntype, "6-39A") == 0)	return 197;
			if (strcmp(tntype, "6-39B") == 0)	return 198;
			if (strcmp(tntype, "6-40A") == 0)	return 199;
			if (strcmp(tntype, "6-40B") == 0)	return 200;
			if (strcmp(tntype, "6-41A") == 0)	return 201;
			if (strcmp(tntype, "6-41B") == 0)	return 202;
			if (strcmp(tntype, "6-42") == 0)	return 203;
			if (strcmp(tntype, "6-43A") == 0)	return 204;
			if (strcmp(tntype, "6-43B") == 0)	return 205;
			if (strcmp(tntype, "6-44A") == 0)	return 206;
			if (strcmp(tntype, "6-44B") == 0)	return 207;
			if (strcmp(tntype, "6-45") == 0)	return 208;
			if (strcmp(tntype, "6-46A") == 0)	return 209;
			if (strcmp(tntype, "6-46B") == 0)	return 210;
			if (strcmp(tntype, "6-47A") == 0)	return 211;
			if (strcmp(tntype, "6-47B") == 0)	return 212;
			if (strcmp(tntype, "6-48") == 0)	return 213;
			if (strcmp(tntype, "6-49") == 0)	return 214;
			if (strcmp(tntype, "6-50") == 0)	return 215;
			break;

		case 7:
			if (strcmp(tntype, "7-1") == 0)	return 216;
			if (strcmp(tntype, "7-2A") == 0)	return 217;
			if (strcmp(tntype, "7-2B") == 0)	return 218;
			if (strcmp(tntype, "7-3A") == 0)	return 219;
			if (strcmp(tntype, "7-3B") == 0)	return 220;
			if (strcmp(tntype, "7-4A") == 0)	return 221;
			if (strcmp(tntype, "7-4B") == 0)	return 222;
			if (strcmp(tntype, "7-5A") == 0)	return 223;
			if (strcmp(tntype, "7-5B") == 0)	return 224;
			if (strcmp(tntype, "7-6A") == 0)	return 225;
			if (strcmp(tntype, "7-6B") == 0)	return 226;
			if (strcmp(tntype, "7-7A") == 0)	return 227;
			if (strcmp(tntype, "7-7B") == 0)	return 228;
			if (strcmp(tntype, "7-8") == 0)	return 229;
			if (strcmp(tntype, "7-9A") == 0)	return 230;
			if (strcmp(tntype, "7-9B") == 0)	return 231;
			if (strcmp(tntype, "7-10A") == 0)	return 232;
			if (strcmp(tntype, "7-10B") == 0)	return 233;
			if (strcmp(tntype, "7-11A") == 0)	return 234;
			if (strcmp(tntype, "7-11B") == 0)	return 235;
			if (strcmp(tntype, "7-12") == 0)	return 236;
			if (strcmp(tntype, "7-13A") == 0)	return 237;
			if (strcmp(tntype, "7-13B") == 0)	return 238;
			if (strcmp(tntype, "7-14A") == 0)	return 239;
			if (strcmp(tntype, "7-14B") == 0)	return 240;
			if (strcmp(tntype, "7-15") == 0)	return 241;
			if (strcmp(tntype, "7-16A") == 0)	return 242;
			if (strcmp(tntype, "7-16B") == 0)	return 243;
			if (strcmp(tntype, "7-17") == 0)	return 244;
			if (strcmp(tntype, "7-18A") == 0)	return 245;
			if (strcmp(tntype, "7-18B") == 0)	return 246;
			if (strcmp(tntype, "7-19A") == 0)	return 247;
			if (strcmp(tntype, "7-19B") == 0)	return 248;
			if (strcmp(tntype, "7-20A") == 0)	return 249;
			if (strcmp(tntype, "7-20B") == 0)	return 250;
			if (strcmp(tntype, "7-21A") == 0)	return 251;
			if (strcmp(tntype, "7-21B") == 0)	return 252;
			if (strcmp(tntype, "7-22") == 0)	return 253;
			if (strcmp(tntype, "7-23A") == 0)	return 254;
			if (strcmp(tntype, "7-23B") == 0)	return 255;
			if (strcmp(tntype, "7-24A") == 0)	return 256;
			if (strcmp(tntype, "7-24B") == 0)	return 257;
			if (strcmp(tntype, "7-25A") == 0)	return 258;
			if (strcmp(tntype, "7-25B") == 0)	return 259;
			if (strcmp(tntype, "7-26A") == 0)	return 260;
			if (strcmp(tntype, "7-26B") == 0)	return 261;
			if (strcmp(tntype, "7-27A") == 0)	return 262;
			if (strcmp(tntype, "7-27B") == 0)	return 263;
			if (strcmp(tntype, "7-28A") == 0)	return 264;
			if (strcmp(tntype, "7-28B") == 0)	return 265;
			if (strcmp(tntype, "7-29A") == 0)	return 266;
			if (strcmp(tntype, "7-29B") == 0)	return 267;
			if (strcmp(tntype, "7-30A") == 0)	return 268;
			if (strcmp(tntype, "7-30B") == 0)	return 269;
			if (strcmp(tntype, "7-31A") == 0)	return 270;
			if (strcmp(tntype, "7-31B") == 0)	return 271;
			if (strcmp(tntype, "7-32A") == 0)	return 272;
			if (strcmp(tntype, "7-32B") == 0)	return 273;
			if (strcmp(tntype, "7-33") == 0)	return 274;
			if (strcmp(tntype, "7-34") == 0)	return 275;
			if (strcmp(tntype, "7-35") == 0)	return 276;
			if (strcmp(tntype, "7-36A") == 0)	return 277;
			if (strcmp(tntype, "7-36B") == 0)	return 278;
			if (strcmp(tntype, "7-37") == 0)	return 279;
			if (strcmp(tntype, "7-38A") == 0)	return 280;
			if (strcmp(tntype, "7-38B") == 0)	return 281;
	break;

		case 8:
			if (strcmp(tntype, "8-1") == 0)	return 282;
			if (strcmp(tntype, "8-2A") == 0)	return 283;
			if (strcmp(tntype, "8-2B") == 0)	return 284;
			if (strcmp(tntype, "8-3") == 0)	return 285;
			if (strcmp(tntype, "8-4A") == 0)	return 286;
			if (strcmp(tntype, "8-4B") == 0)	return 287;
			if (strcmp(tntype, "8-5A") == 0)	return 288;
			if (strcmp(tntype, "8-5B") == 0)	return 289;
			if (strcmp(tntype, "8-6") == 0)	return 290;
			if (strcmp(tntype, "8-7") == 0)	return 291;
			if (strcmp(tntype, "8-8") == 0)	return 292;
			if (strcmp(tntype, "8-9") == 0)	return 293;
			if (strcmp(tntype, "8-10") == 0)	return 294;
			if (strcmp(tntype, "8-11A") == 0)	return 295;
			if (strcmp(tntype, "8-11B") == 0)	return 296;
			if (strcmp(tntype, "8-12A") == 0)	return 297;
			if (strcmp(tntype, "8-12B") == 0)	return 298;
			if (strcmp(tntype, "8-13A") == 0)	return 299;
			if (strcmp(tntype, "8-13B") == 0)	return 300;
			if (strcmp(tntype, "8-14A") == 0)	return 301;
			if (strcmp(tntype, "8-14B") == 0)	return 302;
			if (strcmp(tntype, "8-15A") == 0)	return 303;
			if (strcmp(tntype, "8-15B") == 0)	return 304;
			if (strcmp(tntype, "8-16A") == 0)	return 305;
			if (strcmp(tntype, "8-16B") == 0)	return 306;
			if (strcmp(tntype, "8-17") == 0)	return 307;
			if (strcmp(tntype, "8-18A") == 0)	return 308;
			if (strcmp(tntype, "8-18B") == 0)	return 309;
			if (strcmp(tntype, "8-19A") == 0)	return 310;
			if (strcmp(tntype, "8-19B") == 0)	return 311;
			if (strcmp(tntype, "8-20") == 0)	return 312;
			if (strcmp(tntype, "8-21") == 0)	return 313;
			if (strcmp(tntype, "8-22A") == 0)	return 314;
			if (strcmp(tntype, "8-22B") == 0)	return 315;
			if (strcmp(tntype, "8-23") == 0)	return 316;
			if (strcmp(tntype, "8-24") == 0)	return 317;
			if (strcmp(tntype, "8-25") == 0)	return 318;
			if (strcmp(tntype, "8-26") == 0)	return 319;
			if (strcmp(tntype, "8-27A") == 0)	return 320;
			if (strcmp(tntype, "8-27B") == 0)	return 321;
			if (strcmp(tntype, "8-28") == 0)	return 322;
			if (strcmp(tntype, "8-29A") == 0)	return 323;
			if (strcmp(tntype, "8-29B") == 0)	return 324;
	break;

		case 9:
			if (strcmp(tntype, "9-1") == 0)	return 325;
			if (strcmp(tntype, "9-2A") == 0)	return 326;
			if (strcmp(tntype, "9-2B") == 0)	return 327;
			if (strcmp(tntype, "9-3A") == 0)	return 328;
			if (strcmp(tntype, "9-3B") == 0)	return 329;
			if (strcmp(tntype, "9-4A") == 0)	return 330;
			if (strcmp(tntype, "9-4B") == 0)	return 331;
			if (strcmp(tntype, "9-5A") == 0)	return 332;
			if (strcmp(tntype, "9-5B") == 0)	return 333;
			if (strcmp(tntype, "9-6") == 0)	return 334;
			if (strcmp(tntype, "9-7A") == 0)	return 335;
			if (strcmp(tntype, "9-7B") == 0)	return 336;
			if (strcmp(tntype, "9-8A") == 0)	return 337;
			if (strcmp(tntype, "9-8B") == 0)	return 338;
			if (strcmp(tntype, "9-9") == 0)	return 339;
			if (strcmp(tntype, "9-10") == 0)	return 340;
			if (strcmp(tntype, "9-11A") == 0)	return 341;
			if (strcmp(tntype, "9-11B") == 0)	return 342;
			if (strcmp(tntype, "9-12") == 0)	return 343;
	break;

		case 10:
			if (strcmp(tntype, "10-1") == 0)	return 344;
			if (strcmp(tntype, "10-2") == 0)	return 345;
			if (strcmp(tntype, "10-3") == 0)	return 346;
			if (strcmp(tntype, "10-4") == 0)	return 347;
			if (strcmp(tntype, "10-5") == 0)	return 348;
			if (strcmp(tntype, "10-6") == 0)	return 349;
	break;

		case 11:
			if (strcmp(tntype, "11-1") == 0)	return 350;
	break;

		case 12:
			if (strcmp(tntype, "12-1") == 0)	return 351;
			break;
	}

	// no index desciption of the Tn type
	return -1;
}



//////////////////////////////
//
// getDescription -- return a rough musical description of the Tn type
//

const char* getDescription(const char* tntype) {
	int cardinality = 0;
	if (!sscanf(tntype, "%d", &cardinality)) {
		return ".";
	}
	switch (cardinality) {
		case 0:
			if (strcmp(tntype, "0-1") == 0)	return "Rest";
			break;

		case 1:
			if (strcmp(tntype, "1-1") == 0)	return "Unison";
			break;

		case 2:
			if (strcmp(tntype, "2-1") == 0)	return "Semitone";
			if (strcmp(tntype, "2-2") == 0)	return "Whole-tone";
			if (strcmp(tntype, "2-3") == 0)	return "Minor Third";
			if (strcmp(tntype, "2-4") == 0)	return "Major Third";
			if (strcmp(tntype, "2-5") == 0)	return "Perfect Fourth";
			if (strcmp(tntype, "2-6") == 0)	return "Tritone";
			break;

		case 3:
			if (strcmp(tntype, "3-1") == 0)	return "BACH /Chromatic Trimirror";
			if (strcmp(tntype, "3-2A") == 0)	return "Phrygian Trichord";
			if (strcmp(tntype, "3-2B") == 0)	return "Minor Trichord";
			if (strcmp(tntype, "3-3A") == 0)	return "Major-minor Trichord.1";
			if (strcmp(tntype, "3-3B") == 0)	return "Major-minor Trichord.2";
			if (strcmp(tntype, "3-4A") == 0)	return "Incomplete Major-seventh Chord.1";
			if (strcmp(tntype, "3-4B") == 0)	return "Incomplete Major-seventh Chord.2";
			if (strcmp(tntype, "3-5A") == 0)	return "Rite chord.2, Tritone-fourth.1";
			if (strcmp(tntype, "3-5B") == 0)	return "Rite chord.1, Tritone-fourth.2";
			if (strcmp(tntype, "3-6") == 0)	return "Whole-tone Trichord";
			if (strcmp(tntype, "3-7A") == 0)	return "Incomplete Minor-seventh Chord";
			if (strcmp(tntype, "3-7B") == 0)	return "Incomplete Dominant-seventh Chord.2";
			if (strcmp(tntype, "3-8A") == 0)	return "Incomplete Dominant-seventh Chord.1/Italian-sixth";
			if (strcmp(tntype, "3-8B") == 0)	return "Incomplete Half-dim-seventh Chord";
			if (strcmp(tntype, "3-9") == 0)	return "Quartal Trichord";
			if (strcmp(tntype, "3-10") == 0)	return "Diminished Chord";
			if (strcmp(tntype, "3-11A") == 0)	return "Minor Chord";
			if (strcmp(tntype, "3-11B") == 0)	return "Major Chord";
			if (strcmp(tntype, "3-12") == 0)	return "Augmented Chord";
			break;

		case 4:
			if (strcmp(tntype, "4-1") == 0)	return "BACH /Chromatic Tetramirror";
			if (strcmp(tntype, "4-2A") == 0)	return "Major-second Tetracluster.2";
			if (strcmp(tntype, "4-2B") == 0)	return "Major-second Tetracluster.1";
			if (strcmp(tntype, "4-3") == 0)	return "Alternating Tetramirror";
			if (strcmp(tntype, "4-4A") == 0)	return "Minor Third Tetracluster.2";
			if (strcmp(tntype, "4-4B") == 0)	return "Minor Third Tetracluster.1";
			if (strcmp(tntype, "4-5A") == 0)	return "Major Third Tetracluster.2";
			if (strcmp(tntype, "4-5B") == 0)	return "Major Third Tetracluster.1";
			if (strcmp(tntype, "4-6") == 0)	return "Perfect Fourth Tetramirror";
			if (strcmp(tntype, "4-7") == 0)	return "Arabian Tetramirror";
			if (strcmp(tntype, "4-8") == 0)	return "Double Fourth Tetramirror";
			if (strcmp(tntype, "4-9") == 0)	return "Double Tritone Tetramirror";
			if (strcmp(tntype, "4-10") == 0)	return "Minor Tetramirror";
			if (strcmp(tntype, "4-11A") == 0)	return "Phrygian Tetrachord";
			if (strcmp(tntype, "4-11B") == 0)	return "Major Tetrachord";
			if (strcmp(tntype, "4-12A") == 0)	return "Harmonic-minor Tetrachord";
			if (strcmp(tntype, "4-12B") == 0)	return "Major-third Diminished Tetrachord";
			if (strcmp(tntype, "4-13A") == 0)	return "Minor-second Diminished Tetrachord";
			if (strcmp(tntype, "4-13B") == 0)	return "Perfect-fourth Diminished Tetrachord";
			if (strcmp(tntype, "4-14A") == 0)	return "Major-second Minor Tetrachord";
			if (strcmp(tntype, "4-14B") == 0)	return "Perfect-fourth Major Tetrachord";
			if (strcmp(tntype, "4-15A") == 0)	return "All-interval Tetrachord.1";
			if (strcmp(tntype, "4-15B") == 0)	return "All-interval Tetrachord.2";
			if (strcmp(tntype, "4-16A") == 0)	return "Minor-second Quartal Tetrachord";
			if (strcmp(tntype, "4-16B") == 0)	return "Tritone Quartal Tetrachord";
			if (strcmp(tntype, "4-17") == 0)	return "Major-minor Tetramirror";
			if (strcmp(tntype, "4-18A") == 0)	return "Major-diminished Tetrachord";
			if (strcmp(tntype, "4-18B") == 0)	return "Minor-diminished Tetrachord";
			if (strcmp(tntype, "4-19A") == 0)	return "Minor-augmented Tetrachord";
			if (strcmp(tntype, "4-19B") == 0)	return "Augmented-major Tetrachord";
			if (strcmp(tntype, "4-20") == 0)	return "Major-seventh Chord";
			if (strcmp(tntype, "4-21") == 0)	return "Whole-tone Tetramirror";
			if (strcmp(tntype, "4-22A") == 0)	return "Major-second Major Tetrachord";
			if (strcmp(tntype, "4-22B") == 0)	return "Perfect-fourth Minor Tetrachord";
			if (strcmp(tntype, "4-23") == 0)	return "Quartal Tetramirror";
			if (strcmp(tntype, "4-24") == 0)	return "Augmented Seventh Chord";
			if (strcmp(tntype, "4-25") == 0)	return "French-sixth Chord";
			if (strcmp(tntype, "4-26") == 0)	return "Minor-seventh Chord";
			if (strcmp(tntype, "4-27A") == 0)	return "Half-diminished Seventh Chord/Tristan Chord";
			if (strcmp(tntype, "4-27B") == 0)	return "Dominant-seventh/German-sixth Chord";
			if (strcmp(tntype, "4-28") == 0)	return "Diminished-seventh Chord";
			if (strcmp(tntype, "4-29A") == 0)	return "All-interval Tetrachord.3";
			if (strcmp(tntype, "4-29B") == 0)	return "All-interval Tetrachord.4";
			break;

		case 5:
			if (strcmp(tntype, "5-1") == 0)	return "Chromatic Pentamirror";
			if (strcmp(tntype, "5-2A") == 0)	return "Major-second Pentacluster.2";
			if (strcmp(tntype, "5-2B") == 0)	return "Major-second Pentacluster.1";
			if (strcmp(tntype, "5-3A") == 0)	return "Minor-second Major Pentachord";
			if (strcmp(tntype, "5-3B") == 0)	return "Spanish Pentacluster";
			if (strcmp(tntype, "5-4A") == 0)	return "Blues Pentacluster";
			if (strcmp(tntype, "5-4B") == 0)	return "Minor-third Pentacluster";
			if (strcmp(tntype, "5-5A") == 0)	return "Major-third Pentacluster.2";
			if (strcmp(tntype, "5-5B") == 0)	return "Major-third Pentacluster.1";
			if (strcmp(tntype, "5-6A") == 0)	return "Oriental Pentacluster.1, Raga Megharanji (13161)";
			if (strcmp(tntype, "5-6B") == 0)	return "Oriental Pentacluster.2";
			if (strcmp(tntype, "5-7A") == 0)	return "DoublePentacluster.1, Raga Nabhomani (11415)";
			if (strcmp(tntype, "5-7B") == 0)	return "Double Pentacluster.2";
			if (strcmp(tntype, "5-8") == 0)	return "Tritone-Symmetric Pentamirror";
			if (strcmp(tntype, "5-9A") == 0)	return "Tritone-Expanding Pentachord";
			if (strcmp(tntype, "5-9B") == 0)	return "Tritone-Contracting Pentachord";
			if (strcmp(tntype, "5-10A") == 0)	return "Alternating Pentachord.1";
			if (strcmp(tntype, "5-10B") == 0)	return "Alternating Pentachord.2";
			if (strcmp(tntype, "5-11A") == 0)	return "Center-cluster Pentachord.1";
			if (strcmp(tntype, "5-11B") == 0)	return "Center-cluster Pentachord.2";
			if (strcmp(tntype, "5-12") == 0)	return "Locrian Pentamirror";
			if (strcmp(tntype, "5-13A") == 0)	return "Augmented Pentacluster.1";
			if (strcmp(tntype, "5-13B") == 0)	return "Augmented Pentacluster.2";
			if (strcmp(tntype, "5-14A") == 0)	return "Double-seconds Triple-fourth Pentachord.1";
			if (strcmp(tntype, "5-14B") == 0)	return "Double-seconds Triple-fourth Pentachord.2";
			if (strcmp(tntype, "5-15") == 0)	return "Assymetric Pentamirror";
			if (strcmp(tntype, "5-16A") == 0)	return "Major-minor-dim Pentachord.1";
			if (strcmp(tntype, "5-16B") == 0)	return "Major-minor-dim Pentachord.2";
			if (strcmp(tntype, "5-17") == 0)	return "Minor-major Ninth Chord";
			if (strcmp(tntype, "5-18A") == 0)	return "Gypsy Pentachord.1";
			if (strcmp(tntype, "5-18B") == 0)	return "Gypsy Pentachord.2";
			if (strcmp(tntype, "5-19A") == 0)	return "Javanese Pentachord";
			if (strcmp(tntype, "5-19B") == 0)	return "Balinese Pentachord";
			if (strcmp(tntype, "5-20A") == 0)	return "Balinese Pelog Pentatonic (12414), Raga Bhupala, Raga Bibhas";
			if (strcmp(tntype, "5-20B") == 0)	return "Hirajoshi Pentatonic (21414), Iwato (14142), Sakura/Raga Saveri (14214)";
			if (strcmp(tntype, "5-21A") == 0)	return "Syrian Pentatonic/Major-augmented Ninth Chord, Raga Megharanji (13134)";
			if (strcmp(tntype, "5-21B") == 0)	return "Lebanese Pentachord/Augmented-minor Chord";
			if (strcmp(tntype, "5-22") == 0)	return "Persian Pentamirror, Raga reva/Ramkali (13314)";
			if (strcmp(tntype, "5-23A") == 0)	return "Minor Pentachord";
			if (strcmp(tntype, "5-23B") == 0)	return "Major Pentachord";
			if (strcmp(tntype, "5-24A") == 0)	return "Phrygian Pentachord";
			if (strcmp(tntype, "5-24B") == 0)	return "Lydian Pentachord";
			if (strcmp(tntype, "5-25A") == 0)	return "Diminished-major Ninth Chord";
			if (strcmp(tntype, "5-25B") == 0)	return "Minor-diminished Ninth Chord";
			if (strcmp(tntype, "5-26A") == 0)	return "Diminished-augmented Ninth Chord";
			if (strcmp(tntype, "5-26B") == 0)	return "Augmented-diminished Ninth Chord";
			if (strcmp(tntype, "5-27A") == 0)	return "Major-Ninth Chord";
			if (strcmp(tntype, "5-27B") == 0)	return "Minor-Ninth Chord";
			if (strcmp(tntype, "5-28A") == 0)	return "Augmented-sixth Pentachord.1";
			if (strcmp(tntype, "5-28B") == 0)	return "Augmented-sixth Pentachord.2";
			if (strcmp(tntype, "5-29A") == 0)	return "Kumoi Pentachord.2";
			if (strcmp(tntype, "5-29B") == 0)	return "Kumoi Pentachord.1";
			if (strcmp(tntype, "5-30A") == 0)	return "Enigmatic Pentachord.1";
			if (strcmp(tntype, "5-30B") == 0)	return "Enigmatic Pentachord.2, Altered Pentatonic (14223)";
			if (strcmp(tntype, "5-31A") == 0)	return "Diminished Minor-Ninth Chord";
			if (strcmp(tntype, "5-31B") == 0)	return "Ranjaniraga/Flat-Ninth Pentachord";
			if (strcmp(tntype, "5-32A") == 0)	return "Neapolitan Pentachord.1";
			if (strcmp(tntype, "5-32B") == 0)	return "Neapolitan Pentachord.2";
			if (strcmp(tntype, "5-33") == 0)	return "Whole-tone Pentamirror";
			if (strcmp(tntype, "5-34") == 0)	return "Dominant-ninth/major-minor/Prometheus Pentamirror, Dominant Pentatonic (22332)";
			if (strcmp(tntype, "5-35") == 0)	return "'Black Key' Pentatonic/Slendro/Bilahariraga/Quartal Pentamirror, Yo (23232)";
			if (strcmp(tntype, "5-36A") == 0)	return "Major-seventh Pentacluster.2";
			if (strcmp(tntype, "5-36B") == 0)	return "Minor-seventh Pentacluster.1";
			if (strcmp(tntype, "5-37") == 0)	return "Center-cluster Pentamirror";
			if (strcmp(tntype, "5-38A") == 0)	return "Diminished Pentacluster.1";
			if (strcmp(tntype, "5-38B") == 0)	return "Diminished Pentacluster.2";
			break;

		case 6:
			if (strcmp(tntype, "6-1") == 0)	return "Chromatic Hexamirror/1st ord. all-comb (P6, Ib, RI5)";
			if (strcmp(tntype, "6-2A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-2B") == 0)	return "comb I (1)";
			if (strcmp(tntype, "6-4") == 0)	return "comb RI (6)";
			if (strcmp(tntype, "6-5A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-5B") == 0)	return "comb I (3)";
			if (strcmp(tntype, "6-6") == 0)	return "Double-cluster Hexamirror";
			if (strcmp(tntype, "6-7") == 0)	return "Messiaen's mode 5 (114114), 2nd ord.all-comb(P3+9,I5,Ib,R6,RI2+8)";
			if (strcmp(tntype, "6-8") == 0)	return "1st ord.all-comb (P6, I1, RI7)";
			if (strcmp(tntype, "6-9A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-9B") == 0)	return "comb I (3)";
			if (strcmp(tntype, "6-13") == 0)	return "Alternating Hexamirror/comb RI7)";
			if (strcmp(tntype, "6-14A") == 0)	return "comb P (6)";
			if (strcmp(tntype, "6-14B") == 0)	return "comb P (6)";
			if (strcmp(tntype, "6-15A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-15B") == 0)	return "comb I (5)";
			if (strcmp(tntype, "6-16A") == 0)	return "comb I (3)";
			if (strcmp(tntype, "6-16B") == 0)	return "Megha or 'Cloud' Raga/comb.I (1)";
			if (strcmp(tntype, "6-18A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-18B") == 0)	return "comb I (5)";
			if (strcmp(tntype, "6-20") == 0)	return "Augmented scale, Genus tertium, 3rd ord. all-comb (P2+6+10, I3+7+b, R4+8, RI1+5+9)";
			if (strcmp(tntype, "6-21A") == 0)	return "comb I (1)";
			if (strcmp(tntype, "6-21B") == 0)	return "comb I (3)";
			if (strcmp(tntype, "6-22A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-22B") == 0)	return "comb I (5)";
			if (strcmp(tntype, "6-23") == 0)	return "Super-Locrian Hexamirror/comb RI (8)";
			if (strcmp(tntype, "6-24B") == 0)	return "Melodic-minor Hexachord";
			if (strcmp(tntype, "6-25A") == 0)	return "Locrian Hexachord/Suddha Saveriraga";
			if (strcmp(tntype, "6-25B") == 0)	return "Minor Hexachord";
			if (strcmp(tntype, "6-26") == 0)	return "Phrygian Hexamirror/comb RI (8)";
			if (strcmp(tntype, "6-27A") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-27B") == 0)	return "Pyramid Hexachord/comb I (1)";
			if (strcmp(tntype, "6-28") == 0)	return "Double-Phrygian Hexachord/comb RI (6)";
			if (strcmp(tntype, "6-29") == 0)	return "comb RI (9)";
			if (strcmp(tntype, "6-30A") == 0)	return "Minor-bitonal Hexachord/comb R (6), I (5,b)";
			if (strcmp(tntype, "6-30B") == 0)	return "Petrushka chord, Major-bitonal Hexachord, comb R (6), I (1,7)";
			if (strcmp(tntype, "6-31A") == 0)	return "comb I (7)";
			if (strcmp(tntype, "6-31B") == 0)	return "comb I (b)";
			if (strcmp(tntype, "6-32") == 0)	return "Arezzo major Diatonic (221223), major hexamirror, quartal hexamirror, 1st ord.all-comb P (6), I (3), RI (9)";
			if (strcmp(tntype, "6-33A") == 0)	return "Dorian Hexachord/comb I (1)";
			if (strcmp(tntype, "6-33B") == 0)	return "Dominant-11th/Lydian Hexachord/comb I (5)";
			if (strcmp(tntype, "6-34A") == 0)	return "Mystic Chord or Prometheus Hexachord/comb I (B)";
			if (strcmp(tntype, "6-34B") == 0)	return "Harmonic Hexachord/Augmented-11th/comb I (7)";
			if (strcmp(tntype, "6-35") == 0)	return "Wholetone scale/6th ord.all-comb.(P+IoddT, R+RIevenT)";
			if (strcmp(tntype, "6-37") == 0)	return "comb RI (4)";
			if (strcmp(tntype, "6-38") == 0)	return "comb RI (3)";
			if (strcmp(tntype, "6-42") == 0)	return "comb RI (3)";
			if (strcmp(tntype, "6-44A") == 0)	return "Schoenberg Anagram Hexachord";
			if (strcmp(tntype, "6-44B") == 0)	return "Bauli raga (133131)";
			if (strcmp(tntype, "6-45") == 0)	return "comb RI (6)";
			if (strcmp(tntype, "6-47B") == 0)	return "Blues mode.1 (321132)";
			if (strcmp(tntype, "6-48") == 0)	return "comb RI (2)";
			if (strcmp(tntype, "6-49") == 0)	return "Prometheus Neapolitan mode (132312), comb RI (4)";
			if (strcmp(tntype, "6-50") == 0)	return "comb RI (1)";
			break;

		case 7:
			if (strcmp(tntype, "7-1") == 0)	return "Chromatic Heptamirror";
			if (strcmp(tntype, "7-20A") == 0)	return "Chromatic Phrygian inverse (1123113)";
			if (strcmp(tntype, "7-20B") == 0)	return "Pantuvarali Raga (1321131), Chromatic Mixolydian (1131132), Chromatic Dorian/Mela Kanakangi (1132113)";
			if (strcmp(tntype, "7-21B") == 0)	return "Gypsy hexatonic (1312113)";
			if (strcmp(tntype, "7-22") == 0)	return "Persian, Major Gypsy, Hungarian Minor, Double Harmonic scale, Bhairav That, Mayamdavagaula Raga (all: 1312131), Oriental (1311312)";
			if (strcmp(tntype, "7-23B") == 0)	return "Tritone Major Heptachord";
			if (strcmp(tntype, "7-24B") == 0)	return "Enigmatic Heptatonic (1322211)";
			if (strcmp(tntype, "7-27B") == 0)	return "Modified Blues mode (2121132)";
			if (strcmp(tntype, "7-30A") == 0)	return "Neapolitan-Minor mode (1222131), Mela Dhenuka";
			if (strcmp(tntype, "7-31A") == 0)	return "Alternating Heptachord.1/Hungarian Major mode (3121212)";
			if (strcmp(tntype, "7-31B") == 0)	return "Alternating Heptachord.2";
			if (strcmp(tntype, "7-32A") == 0)	return "Harmonic-Minor mode (2122131), Spanish Gypsy, Mela Kiravani, Pilu That";
			if (strcmp(tntype, "7-32B") == 0)	return "Dharmavati Scale (2131221), Harmonic minor inverse (1312212), Mela Cakravana, Raga Ahir Bhairav";
			if (strcmp(tntype, "7-33") == 0)	return "Neapolitan-major mode (1222221)/Leading-Whole-tone mode (222211)";
			if (strcmp(tntype, "7-34") == 0)	return "Harmonic/Super-Locrian, Melodic minor ascending (1212222)/Aug.13th Heptamirror, Jazz Minor";
			if (strcmp(tntype, "7-35") == 0)	return "Major Diatonic Heptachord/Dominant-13th, Locrian (1221222), Phrygian (1222122), Major inverse";
			break;

		case 8:
			if (strcmp(tntype, "8-1") == 0)	return "Chromatic Octamirror";
			if (strcmp(tntype, "8-9") == 0)	return "Messiaen's mode 4 (11131113)";
			if (strcmp(tntype, "8-22B") == 0)	return "Spanish Octatonic Scale (r9) (12111222)";
			if (strcmp(tntype, "8-23") == 0)	return "Quartal Octachord, Diatonic Octad";
			if (strcmp(tntype, "8-25") == 0)	return "Messiaen mode 6 (11221122)";
			if (strcmp(tntype, "8-26") == 0)	return "Spanish Phrygian (r9) (12112122)/ Blues mode.2 (21211212)";
			if (strcmp(tntype, "8-28") == 0)	return "Alternating Octatonic or Diminished scale (12121212)";
			break;

		case 9:
			if (strcmp(tntype, "9-1") == 0)	return "Chromatic Nonamirror";
			if (strcmp(tntype, "9-7A") == 0)	return "Nonatonic Blues Scale (211111212)";
			if (strcmp(tntype, "9-9") == 0)	return "Raga Ramdasi Malhar (r2) (211122111)";
			if (strcmp(tntype, "9-11B") == 0)	return "Diminishing Nonachord";
			if (strcmp(tntype, "9-12") == 0)	return "Tsjerepnin/Messiaen mode 3 (112112112)";
			break;

		case 10:
			if (strcmp(tntype, "10-1") == 0)	return "Chromatic Decamirror";
			if (strcmp(tntype, "10-5") == 0)	return "Major-minor mixed (r7)";
			if (strcmp(tntype, "10-6") == 0)	return "Messiaen mode 7 (1111211112)";
			break;

		case 11:
			if (strcmp(tntype, "11-1") == 0)	return "Chromatic Undecamirror";
			break;

		case 12:
			if (strcmp(tntype, "12-1") == 0)	return "Chromatic Scale/Dodecamirror (111111111111)";
			break;
	}

	// no musical desciption of the Tn type
	return ".";

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



