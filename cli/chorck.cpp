//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Apr 14 09:41:45 PDT 2000
// Last Modified: Sun May 24 09:15:43 PDT 2009 Para octs removed from #6
// Last Modified: Sun May 24 09:15:43 PDT 2009 Added -s, -d, and -d options
// Last Modified: Sun May 24 19:23:l2 PDT 2009 Added -f option
// Last Modified: Sun May 24 19:23:l2 PDT 2009 Exclude unison motion in #3
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class
// Last Modified: Fri Dec 27 17:36:01 PST 2013 Added note IDs
// Last Modified: Thu May 10 06:14:19 PDT 2018 Added new -m option
// Filename:      ...sig/examples/all/chorck.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/chorck.cpp
// Syntax:        C++; museinfo
//
// Description:   Analyzes choral-style harmony exercises for possible
//		  errors such as parallel fifths and octaves.  The input
//		  Humdrum data must contain 4 spines of kern data which
//		  represents the four voices: bass, tenor, alto and soprano.
//		  If there are more than 4 **kern spines, then the first
//		  4 **kern spines are assumed to be the SATB voices.
//		  The order of the voice spines does not matter, the program
// 		  will automatically sort them before analysis starts.
//		  Errors are reported as global comments before the line
//		  on which offending error starts, and is of the form:
//		  !! Warning: <Error description and location>
//
// Error types detected by this program:
//
//	1. Parallel 5ths between two voices when moving to
//	   different pitch classes.
//	2. Parallel Octaves between two voices when moving to
//	   different pitch classes.
//	3. Contrary parallel 5ths -- when two voices move in
//	   parallel 5ths displaced by an octave.
//	4. Unequal 5ths -- when the bass part and another
//	   voice move from dim 5ths to perfect 5ths or vice versa.
//	5. Hidden 5ths -- when the soprano moves in similar
//	   motion with another voice and the soprano leaps
//	   to a perfect 5th with that voice.
//	6. Hidden 8va -- when the soprano moves in similar
//	   motion with another voice and the soprano leaps
//	   to a perfect octave with that voice.
//	7. Voice crossing -- when an inner voice goes above
//	   the soprano voice or below the bass voice.
//	8. Open spacing -- when the interval between successive
//	   voices other than the bass exceeds an octave.
//

#include <cctype>
#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "humdrum.h"

#define ERROR_PARA5     1
#define ERROR_PARA8     2
#define ERROR_CPARA5    3
#define ERROR_NEQ5      4
#define ERROR_HIDDEN5   5
#define ERROR_HIDDEN8   6
#define ERROR_VOICEX    7
#define ERROR_OPEN      8

class Error {
	public:
		int line;            // line number on which the error occurs
		char message[128];   // error message to display
		Error(void) { line = -1; message[0] = '\0';}
};


// function declarations
void   checkForErrors      (HumdrumFile& infile);
void   checkOptions        (Options& opts, int argc, char* argv[]);
int    errorCompare        (const void* a, const void* b);
void   errormessage        (int errornumber, const string& voice1,
							      const string& voice2, int linenumber, int endline,
                           HumdrumFile& infile);
void   example             (void);
void   initialize          (HumdrumFile& infile);
void   prepareVoices       (void);
void   printRules          (const string& ruleString);
void   printVoiceArray     (void);
void   processRecords      (HumdrumFile& infile);
void   usage               (const string& command);
void   sortErrorMessages   (Error* errors, int size);
void   sortVoices          (void);
void   writeoutput         (HumdrumFile& infile);
int    getVoiceCount       (HumdrumRecord& record);
void   marknote            (HumdrumFile& infile, int line, int spine, 
                            const string& mark);

void   error1(HumdrumFile& infile);
void   error2(HumdrumFile& infile);
void   error3(HumdrumFile& infile);
void   error4(HumdrumFile& infile);
void   error5(HumdrumFile& infile);
void   error6(HumdrumFile& infile);
void   error7(HumdrumFile& infile);
void   error8(HumdrumFile& infile);

// global variables
Options      options;              // database for command-line arguments
int          chordinit;            // for initializing chord detection function
int          errorCheck[20] = {0}; // command line check for errors exclusion
string       header;               // error message start string
string       marker;               // Warning string start
int          voicemin = 0;         // used with -s, -d, -t option
int          fileQ    = 0;         // used with -f option
int          idQ      = 0;         // used with --id option
int          rawQ     = 0;         // used with --raw option
int          markQ    = 1;         // used with -m option
string       Filename;             // used with -f option

// Analysis variables
vector<Error>        errorList;            // a list of detected errors in chorale
vector<int>          linenum;              // line number in file of given pitch set
vector<vector<string> > ids;               // used with --id option
vector<int>          voices[4];            // pitches from SATB lines
int                  voiceloc[4];          // SATB voice spine locations
int                  locvoice[1000] = {0}; // SATB voice spine locations

///////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
	checkOptions(options, argc, argv);
	HumdrumStream streamer(options);
	HumdrumFile infile;

	while (streamer.read(infile)) {
		Filename = infile.getFilename();
		initialize(infile);

		// build note array from the input file according to command-line options
		processRecords(infile);
		sortVoices();

		// make rests = 0, slured notes = -attack value
		// following line is no longer needed: done in processRecords()
		// prepareVoices();

		if (options.getBoolean("base40-array")) {
			printVoiceArray();
			return 0;
		}

		checkForErrors(infile);
		if (!rawQ) {
			writeoutput(infile);
		}
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// checkForErrors -- check the chorale notes for errors
//

void checkForErrors(HumdrumFile& infile) {
	if (errorCheck[0]) {
		error1(infile);
	}
	if (errorCheck[1]) {
		error2(infile);
	}
	if (errorCheck[2]) {
		error3(infile);
	}
	if (errorCheck[3]) {
		error4(infile);
	}
	if (errorCheck[4]) {
		error5(infile);
	}
	if (errorCheck[5]) {
		error6(infile);
	}
	if (errorCheck[6]) {
		error7(infile);
	}
	if (errorCheck[7]) {
		error8(infile);
	}
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("a|base40-array=b",  "show intermediate calculation array");
	opts.define("f|filename=b",      "display filename in warning");
	opts.define("l|line=b",          "display line nums with -a");
	opts.define("m|marker=s:Warning: ", "start of error message");
	opts.define("M|no-marks=b",      "no marks on notes involved in rule violation");
	opts.define("p|print-rules=s",   "print specified rule number");
	opts.define("r|rule=s:",         "include certain error checking rules");
	opts.define("s|no-single=b",     "exclude single note attacks");
	opts.define("d|no-double=b",     "exclude double note attacks and fewer");
	opts.define("t|no-triple=b",     "exclude triple note attacks and fewer");
	opts.define("w|warnings=b",      "show only warnings");
	opts.define("x|exclude=s:",      "exclude certain error checking rules");
	opts.define("id=b",              "add note ID numbers for each note");
	opts.define("raw=b",             "display error numbers and input note ids");

	opts.define("debug=b",           "determine bad input line num");
	opts.define("author=b",          "author of program");
	opts.define("version=b",         "compilation info");
	opts.define("example=b",         "example usages");
	opts.define("h|help=b",          "short description");
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, April 2000" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 10 May 2018" << endl;
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

	if (opts.getBoolean("no-single")) {
		voicemin = 2;
	}
	if (opts.getBoolean("no-double")) {
		voicemin = 3;
	}
	if (opts.getBoolean("no-triple")) {
		voicemin = 4;
	}

	for (int i=0; i<8; i++) {
		errorCheck[i] = 1;
	}
	if (opts.getBoolean("exclude")) {
		string pointer = opts.getString("exclude");
		for (int i=0; i<(int)pointer.size(); i++) {
			if (std::isdigit(pointer[i]) && pointer[i] != '0') {
				errorCheck[pointer[i]  - '1'] = 0;
			}
		}
	}
	if (opts.getBoolean("rule")) {
		// turn off all rules
		for (int i=0; i<8; i++) {
			errorCheck[i] = 0;
		}
		string pointer = opts.getString("rule");
		for (int i=0; i<(int)pointer.size(); i++) {
			if (std::isdigit(pointer[i]) && pointer[i] != '0') {
				errorCheck[pointer[i]  - '1'] = 1;
			}
		}
	}

	if (opts.getBoolean("print-rules")) {
		printRules(opts.getString("print-rules"));
		exit(0);
	}

	marker =  opts.getString("marker");
	fileQ  =  opts.getBoolean("filename");
	idQ    =  opts.getBoolean("id");
	rawQ   =  opts.getBoolean("raw");
	markQ  = !opts.getBoolean("no-marks");
	if (rawQ) {
		idQ = 1;
	}
	if (options.getBoolean("warnings")) {
		markQ = 0;
	}
}



//////////////////////////////
//
// determineChordQuality -- extracts notes from humdrum data record
//	and sends them to a chord identifier.  Notes from previous
//	records are remembered, and replace any null records in the
//	data.  Rests are represented by some negative value
//	and will be ignored by the chord identifier.
//

ChordQuality determineChordQuality(const HumdrumRecord& aRecord) {
	static vector<int> lastnotes;  // keeping track of null record notes
	int i;
	if (chordinit) {
		chordinit = 0;
		lastnotes.resize(aRecord.getFieldCount());
		for (i=0; i<aRecord.getFieldCount(); i++) {
			lastnotes[i] = E_root_rest;
		}
		// remove non-kern spines from note list
		for (i=0; i<aRecord.getFieldCount(); i++) {
			if (aRecord.getExInterpNum(i) != E_KERN_EXINT) {
				lastnotes.resize((int)lastnotes.size() - 1);
			}
		}
	}

	// determine chord notes and send to chord identifier
	vector<int> currentNotes(lastnotes.size());
	int count = 0;
	for (i=0; i<aRecord.getFieldCount(); i++) {
		if (aRecord.getExInterpNum(i) == E_KERN_EXINT) {
			if (strcmp(aRecord[i], ".") == 0) {
				currentNotes[count] = lastnotes[count];
			} else {
				currentNotes[count] = Convert::kernToBase40(aRecord[i]);
				lastnotes[count] = currentNotes[count];
			}
			count++;
		}
	}

	ChordQuality output;
	Convert::noteSetToChordQuality(output, currentNotes);
	return output;
}



//////////////////////////////
//
// errorCompare -- insert an error message into the error buffer
//    for printing later
//

int errorCompare(const void* a, const void* b) {
	Error& A = *((Error*)a);
	Error& B = *((Error*)b);

	if (A.line < B.line) {
		return -1;
	} else if (A.line > B.line) {
		return 1;
	} else {
		int x, y;
		x = atoi(A.message);
		y = atoi(B.message);
		if (x < y) {
			return -1;
		} else if (x > y) {
			return 1;
		} else {
			return 0;
		}
	}
}



//////////////////////////////
//
// errormessage -- insert an error message into the error buffer
//    for printing later
//

void errormessage(int errornumber, const string& voice1, const string& voice2,
		int linenumber, int endline, HumdrumFile& infile) {

	int v1 = 0;
	int v2 = 0;

	if (voice1 == "tenor")   { v1 = 1; }
	if (voice1 == "alto")    { v1 = 2; }
	if (voice1 == "soprano") { v1 = 3; }
	if (voice2 == "tenor")   { v2 = 1; }
	if (voice2 == "alto")    { v2 = 2; }
	if (voice2 == "soprano") { v2 = 3; }

	if (idQ) {
		if (rawQ) {
			cout << errornumber;
			cout << " ";
			cout << "ID";
			cout << ":" << ids[v1][linenumber];
			cout << ":" << ids[v2][linenumber];
			if (errornumber < 7) {
				cout << ":" << ids[v1][endline];
				cout << ":" << ids[v2][endline];
			}
		} else {
			cout << "!!ERROR: ";
			cout << errornumber;
//       cout << linenumber * 100 + v1 << " " << linenumber * 100 + v2;
			cout << " " << ids[v1][linenumber];
			cout << " " << ids[v2][linenumber];
		if (errornumber < 7) {
				// print ending note values
				//cout << " " << endline * 100 + v1;
				//cout << " " << endline * 100 + v2;
				cout << " " << ids[v1][endline];
				cout << " " << ids[v2][endline];
			}
		}
		cout << endl;
	}

	Error anError;
	anError.line = linenumber;
	switch (errornumber) {
		case 1:
			snprintf(anError.message, 128, "1. Parallel 5th between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 2:
			snprintf(anError.message, 128, "2. Parallel octave between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 3:
			snprintf(anError.message, 128, "3. Contrary parallel 5th between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 4:
			snprintf(anError.message, 128, "4. Unequal 5th between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 5:
			snprintf(anError.message, 128, "5. Hidden 5th between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 6:
			snprintf(anError.message, 128, "6. Hidden octave between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 7:
			snprintf(anError.message, 128, "7. Voice crossing between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		case 8:
			snprintf(anError.message, 128, "8. Open spacing between %s and %s",
				  voice1.c_str(), voice2.c_str());
			break;
		default:
			return;
	}

	errorList.push_back(anError);

	if (markQ) {
		marknote(infile, linenumber, v1, "@");
		marknote(infile, linenumber, v2, "@");
		marknote(infile, endline, v1, "N");
		marknote(infile, endline, v2, "N");
	}
}



//////////////////////////////
//
// marknote -- Mark note involved in a rule violation
//

void marknote(HumdrumFile& infile, int line, int spine, const string& mark) {
	string data = infile[line][spine];
	while ((line > 0) && (data == ".")) {
		line--;
		while ((line > 0) && !infile[line].isData()) {
			line--;
		}
		data = infile[line][spine];
	}
	if (data.find(mark) == std::string::npos) {
		data += mark;
	}
	if (line > 0) {
		infile[line].changeField(spine, data.c_str());
	}
}



//////////////////////////////
//
// example -- example usage of the quality program
//

void example(void) {
	cout <<
	"                                                                         \n"
	"# example usage of the quality program.                                  \n"
	"# analyze a Bach chorale for chord qualities:                            \n"
	"     quality chor217.krn                                                 \n"
	"                                                                         \n"
	"# display the chord analysis with original data:                         \n"
	"     quality -a chor217.krn                                              \n"
	"                                                                         \n"
	"# display only the roots of chords:                                      \n"
	"     quality -r chor217.krn                                              \n"
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// initialize -- setup for a new file
//

void initialize(HumdrumFile& infile) {
	int i;
	for (i=0; i<4; i++) {
		voices[i].reserve(infile.getNumLines());
		voices[i].resize(0);
	}
	linenum.reserve(infile.getNumLines());
	linenum.resize(0);

	if (idQ) {
		ids.resize(4);
		for (i=0; i<(int)ids.size(); i++) {
			ids[i].resize(infile.getNumLines());
		}
	}

	errorList.reserve(infile.getNumLines());
	errorList.resize(0);
}



//////////////////////////////
//
// printRules -- print the requested rules
//

void printRules(const string& ruleString) {
	for (int i=0; i<(int)ruleString.size(); i++) {
		if (std::isdigit(ruleString[i]) && ruleString[i] != '0') {
			switch (ruleString[i] - '0') {
				case 1:
					cout << "Rule 1: Parallel 5ths\n"
						  << "   Two voices moving to different pitch classes"
						  << " cannot form a perfect 5th\n"
						  << "   on two successive notes.\n\n";
					break;
				case 2:
					cout << "Rule 2: Parallel Octaves\n"
						  << "   Two voices moving to different pitch classes"
						  << " cannot form a perfect octave\n"
						  << "   on two successive notes.\n\n";
					break;
				case 3:
					cout << "Rule 3: Contrary parallel 5ths\n"
						  << "   Two voices moving to different pitch classes\n"
						  << "   in contrary motion cannot form a perfect 5ths\n"
						  << "   on two successive notes.\n\n";
					break;
				case 4:
					cout << "Rule 4: Unequal 5ths\n"
						  << "   The intervals between the bass and an upper\n"
						  << "   voice cannot form the successive pattern\n"
						  << "   perfect 5th/dim 5th or dim 5th/perfect 5th.\n\n";
					break;
				case 5:
					cout << "Rule 5: Hidden 5ths\n"
						  << "   When the soprano moves in similary motion\n"
						  << "   with another voice and the sporano leaps to \n"
						  << "   a perfect 5th with that voice.\n\n";
					break;
				case 6:
					cout << "Rule 6: Hidden Octave\n"
						  << "   When the soprano moves in similary motion\n"
						  << "   with another voice and the sporano leaps to \n"
						  << "   a perfect octave with that voice.\n\n";
					break;
				case 7:
					cout << "Rule 7: Voice Crossing\n"
						  << "   An inner voice (alto, tenor) may not go outside\n"
						  << "   the range of the encompassing "
						  << "voices (bass, soprano).\n\n";
					break;
				case 8:
					cout << "Rule 8: Open Spacing\n"
						  << "   The successive interval between upper voices\n"
						  << "   must not exceed an octave.\n\n";
					break;
			}
		}
	}

}



//////////////////////////////
//
// printVoiceArray -- print the intermediate voice array
//

void printVoiceArray(void) {
	int i;
	int lineQ = options.getBoolean("line");
	for (i=0; i<(int)voices[0].size(); i++) {
		if (lineQ) {
			cout << linenum[i]+1 << '\t';
		}
		cout << voices[0][i] << '\t'
			  << voices[1][i] << '\t'
			  << voices[2][i] << '\t'
			  << voices[3][i] << '\n';
	}
}



//////////////////////////////
//
// prepareVoices -- change rests to 0's and continued notes
//   to negative values for the attack note.
//

void prepareVoices(void) {
	int i, k;
	int current = 0;
	for (k=0; k<4; k++) {
		for (i=0; i<(int)voices[k].size(); i++) {
			if (voices[k][i] < 0) {
				voices[k][i] = 0;
				current = 0;
			} else if (voices[k][i] == 0) {
				voices[k][i] = -current;
			} else {
				current = voices[k][i];
			}
		}
	}
}



//////////////////////////////
//
// processRecords -- extract pitches from choral in preparations for
//      error analysis.
//

void processRecords(HumdrumFile& infile) {
	int base40pitch;
	int vcount;
	vector<string> currentId(infile.getMaxTracks()+10);
	PerlRegularExpression pre;
	int i, j, k;

	for (i=0; i<infile.getNumLines(); i++) {
		if (options.getBoolean("debug")) {
			cout << "processing line " << (i+1) << " of input ..." << endl;
		}
		switch (infile[i].getType()) {
			case E_humrec_none:
			case E_humrec_empty:
			case E_humrec_bibliography:
			case E_humrec_global_comment:
			case E_humrec_data_kern_measure:
				break;
			case E_humrec_data_comment:
			{
				for (j=0; j<infile[i].getFieldCount(); j++) {
					if (pre.search(infile[i][j], "!ID:\\s*([^\\s]+)")) {
						currentId[j] = pre.getSubmatch(1);
					}
				}
			}
				break;
			case E_humrec_interpretation:
			{
				if (strncmp(infile[i][0], "**", 2) != 0) {
					break;
				}
				int count = 0;
				for (k=0; k<infile[i].getFieldCount(); k++) {
					if (infile[i].getExInterpNum(k) == E_KERN_EXINT) {
						voiceloc[count] = k;
						locvoice[k] = count;
						count++;
					}
					if (count == 4) {
						break;
					}
				}
				if (count != 4) {
					cerr << "Error on line " << i+1 << " of input: "
						  << "not enought **kern spines for analysis." << endl;
					exit(1);
				}
			}
				break;
			case E_humrec_data:
			{
				const char* datap;
				int sign; // +1 = note attack, -1 = not
				vcount = getVoiceCount(infile[i]);
				if (vcount >= voicemin) {
					for (k=0; k<4; k++) {
						if (strcmp(infile[i][voiceloc[k]], ".") == 0) {
						   datap = infile.getDotValue(i, voiceloc[k]);
						   sign = -1;
						} else {
						   datap = infile[i][voiceloc[k]];
						   sign = +1;
						   if (strchr(datap, ']') != NULL) {
							  sign = -1;
						   } else if (strchr(datap, '_') != NULL) {
							  sign = -1;
						   }
						}
						if (strchr(infile[i][voiceloc[k]], 'r') != NULL) {
						   base40pitch = 0;
						} else {
						   base40pitch = sign * Convert::kernToBase40(datap);
						}
						voices[k].push_back(base40pitch);

						if (idQ) {
						   ids[k][i] = currentId[voiceloc[k]];
						}
					}
					linenum.push_back(i);

				}
			}
				break;
			default:
				cerr << "Error on line " << (i+1) << " of input" << endl;
				exit(1);
		}
	}
}



//////////////////////////////
//
// getVoiceCount -- return the number of note attacks on the line.
//    Excludes tied notes, but includes rests (but not continuing
//    rests which are not necessary).
//

int getVoiceCount(HumdrumRecord& record) {
	int output = 0;
	int i;
	for (i=0; i<record.getFieldCount(); i++) {
		if (strcmp(record.getExInterp(i), "**kern") != 0) {
			continue; // ignore non-kern spines
		}
		if (strcmp(record[i], ".") == 0) {
			continue; // ignore null tokens (sounding notes or continuing rests)
		}
		if (strchr(record[i], ']') != NULL) {
			continue; // ignore tied notes (endings)
		}
		if (strchr(record[i], '_') != NULL) {
			continue; // ignore tied notes (continuations)
		}
		output++;

	}
	return output;
}



//////////////////////////////
//
// sortErrorMessages -- put the bass voice in the first array location,
//	then the tenor, alto, and soprano.
//

void sortErrorMessages(Error* errors, int size) {
  qsort(errors, size, sizeof(Error), errorCompare);
}



//////////////////////////////
//
// sortVoices -- put the bass voice in the first array location,
//	then the tenor, alto, and soprano.
//

void sortVoices(void) {
	int length = (int)voices[0].size();
	int count = 0;
	double voiceordering[4] = {0.0};
	int i;
	int k;
	int min, max;
	int temp;
	int minindex, maxindex;
	int tenorindex = 0;
	int altoindex = 0;
	for (i=0; i<length; i++) {
		if (voices[0][i] > 0 && voices[1][i] > 0 &&
				voices[2][i] > 0 && voices[3][i] > 0) {

			// find lowest and highest notes
			max = voices[0][i];
			min = voices[0][i];
			maxindex = minindex = 0;
			for (k=1; k<4; k++) {
				if (voices[k][i] > max) {
					max = voices[k][i];
					maxindex = k;
				}
				if (voices[k][i] < min) {
					min = voices[k][i];
					minindex = k;
				}
			}
			switch (maxindex) {
				case 0: switch (minindex) {
							 case 0: tenorindex = -1; altoindex = -1; break;
							 case 1: tenorindex = 2; altoindex = 3; break;
							 case 2: tenorindex = 1; altoindex = 3; break;
							 case 3: tenorindex = 1; altoindex = 2; break;
						  }
						  break;
				case 1: switch (minindex) {
							 case 0: tenorindex = 2; altoindex = 3; break;
							 case 1: tenorindex = -1; altoindex = -1; break;
							 case 2: tenorindex = 0; altoindex = 3; break;
							 case 3: tenorindex = 0; altoindex = 2; break;
						  }
						  break;

				case 2: switch (minindex) {
							 case 0: tenorindex = 1; altoindex = 3; break;
							 case 1: tenorindex = 0; altoindex = 3; break;
							 case 2: tenorindex = -1; altoindex = -1; break;
							 case 3: tenorindex = 0; altoindex = 1; break;
						  }
						  break;

				case 3: switch (minindex) {
							 case 0: tenorindex = 1; altoindex = 2; break;
							 case 1: tenorindex = 0; altoindex = 2; break;
							 case 2: tenorindex = 0; altoindex = 1; break;
							 case 3: tenorindex = -1; altoindex = -1; break;
						  }
						  break;
			}
			if (voices[tenorindex][i] > voices[altoindex][i]) {
				 temp = tenorindex;
				 tenorindex = altoindex;
				 altoindex = temp;
			}

			voiceordering[minindex] += 0;
			voiceordering[tenorindex] += 1;
			voiceordering[altoindex] += 2;
			voiceordering[maxindex] += 3;
			count++;

		}
	}

	for (k=0; k<4; k++) {
		voiceordering[k] /= count;
		voiceordering[k] = (int)(voiceordering[k]+0.5);
	}

	int oldbass = 0;
	if (voiceordering[0] != 0) {
		if (voiceordering[1] == 0) {
			oldbass = 1;
		} else if (voiceordering[2] == 0) {
			oldbass = 2;
		} else {
			oldbass = 3;
		}

		for (i=0; i<(int)voices[0].size(); i++) {
			temp = voices[0][i];
			voices[0][i] = voices[oldbass][i];
			voices[oldbass][i] = temp;
		}
	}


	int oldtenor = 1;
	if (voiceordering[1] != 1) {
		if (voiceordering[2] == 1) {
			oldtenor = 2;
		} else {
			oldtenor = 3;
		}

		for (i=0; i<(int)voices[0].size(); i++) {
			temp = voices[1][i];
			voices[1][i] = voices[oldtenor][i];
			voices[oldtenor][i] = temp;
		}
	}

	if (voiceordering[2] > voiceordering[3]) {
		for (i=0; i<(int)voices[0].size(); i++) {
			temp = voices[2][i];
			voices[2][i] = voices[3][i];
			voices[3][i] = temp;
		}
	}

}



//////////////////////////////
//
// usage -- gives the usage statement for the quality program
//

void usage(const string& command) {
	cout <<
	"                                                                         \n"
	"Analyzes **kern data and generates a **qual spine which gives the chord  \n"
	"quality of the **kern spines.  Currently, input spines cannot split or   \n"
	"join.                                                                    \n"
	"                                                                         \n"
	"Usage: " << command << " [-a][-i|-r|-t] [input1 [input2 ...]]            \n"
	"                                                                         \n"
	"Options:                                                                 \n"
	"   --options = list of all options, aliases and default values           \n"
	"                                                                         \n"
	<< endl;
}



////////////////////////////
//
// writeoutput -- write the input file plus the errors which
//    were detected.
//

void writeoutput(HumdrumFile& infile) {
	sortErrorMessages(errorList.data(), errorList.size());

	int errorIndex = 0;

	if (options.getBoolean("warnings")) {
		for (int i=0; i<(int)errorList.size(); i++) {
			cout << header << errorList[i].message << " on line "
				  << errorList[i].line + 1;
			if (fileQ && !Filename.empty()) {
				cout << " (" << Filename << ")";
			}
			cout << '\n';
		}
	} else {
		for (int i=0; i<infile.getNumLines(); i++) {
			while (errorIndex < (int)errorList.size() &&
					errorList[errorIndex].line == i) {
				cout << "!! ";
				cout << marker;
				cout << errorList[errorIndex].message;
				if (fileQ && !Filename.empty()) {
					cout << " (" << Filename << ")";
				}
				cout << '\n';
				errorIndex++;
			}
//         if (idQ && infile[i].isData()) {
//            int j;
//            for (j=0; j<infile[i].getFieldCount(); j++) {
//               cout << "!";
//               if (infile[i].isExInterp(j, "**kern")) {
//                  if (strcmp(infile[i][j], ".") != 0) {
//                     cout << ids[locvoice[j]][i];
//                     // cout << "ID:" << i * 100 + j;
//                  }
//               }
//               if (j < infile[i].getFieldCount() - 1) {
//                  cout << "\t";
//               }
//            }
//            cout << "\n";
//         }
			cout << infile[i] << '\n';
		}
	}
	if (markQ) {
		cout << "!!!RDF**kern: @ = marked note, color=\"red\", start of rule violation" << endl;
		cout << "!!!RDF**kern: N = marked note, color=\"orange\", end of rule violation" << endl;
	}
}



///////////////////////////////////////////////////////////////////////////
//
// error-checking functions
//

////////////////////////////
//
//	1. Parallel 5ths between two voices when moving to
//	   different pitch classes.
//

void error1(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	int newbass, newtenor, newalto, newsoprano;
	int sdir, tdir, adir, bdir;   // melodic direction of the voices
	for (i=0; i<(int)voices[0].size()-1; i++) {
		bass = abs(voices[0][i]);
		tenor = abs(voices[1][i]);
		alto = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		newsoprano = abs(voices[3][i+1]);
		newbass = abs(voices[0][i+1]);
		newtenor = abs(voices[1][i+1]);
		newalto = abs(voices[2][i+1]);

		sdir = newsoprano - soprano;
		adir = newalto - alto;
		tdir = newtenor - tenor;
		bdir = newbass - bass;

		if (sdir < 0) sdir = -1;
		if (adir < 0) adir = -1;
		if (tdir < 0) tdir = -1;
		if (bdir < 0) bdir = -1;

		if (sdir > 0) sdir = 1;
		if (adir > 0) adir = 1;
		if (tdir > 0) tdir = 1;
		if (bdir > 0) bdir = 1;

		// check against bass voice
		if (newbass == 0 || bass == 0 || bdir == 0) {
			goto tenorvoice1;
		}

		if (tenor != 0 && newtenor != 0 && bdir == tdir) {
			if ((tenor-bass+40)%40 == 23 && (newtenor-newbass+40)%40 == 23) {
				errormessage(1, "bass", "tenor", linenum[i], linenum[i+1], infile);
			}
		}

		if (newalto != 0 && alto != 0 && bdir == adir) {
			if ((alto-bass+40)%40 == 23 && (newalto-newbass+40)%40 == 23) {
				errormessage(1, "bass", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0 && bdir == sdir) {
			if ((soprano-bass+40)%40 == 23 && (newsoprano-newbass+40)%40 == 23) {
				errormessage(1, "bass", "soprano", linenum[i], linenum[i+1], infile);
			}
		}

tenorvoice1:

		if (newtenor == 0 || tenor == 0 || tdir == 0) {
			goto altovoice1;
		}

		if (newalto != 0 && alto != 0 && adir == tdir) {
			if ((alto-tenor+40)%40 == 23 && (newalto-newtenor+40)%40 == 23) {
				errormessage(1, "tenor", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0 && adir == sdir) {
			if ((soprano-tenor+40)%40 == 23 && (newsoprano-newtenor+40)%40 == 23) {
				errormessage(1, "tenor", "soprano", linenum[i], linenum[i+1], infile);
			}
		}


altovoice1:
		if (newalto == 0 || alto == 0 || adir == 0) {
			continue;
		}

		if (newsoprano != 0 && soprano != 0 && adir == sdir) {
			if ((soprano-alto+40)%40 == 23 && (newsoprano-newalto+40)%40 == 23) {
				errormessage(1, "alto", "soprano", linenum[i], linenum[i+1], infile);
			}
		}
	}
}



////////////////////////////
//
//	2. Parallel Octaves between two voices when moving to
//	   different pitch classes.
//

void error2(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	int newbass, newtenor, newalto, newsoprano;
	int sdir, tdir, adir, bdir;   // melodic direction of the voices
	for (i=0; i<(int)voices[0].size()-1; i++) {
		bass    = abs(voices[0][i]);
		tenor   = abs(voices[1][i]);
		alto    = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		newsoprano = abs(voices[3][i+1]);
		newbass    = abs(voices[0][i+1]);
		newtenor   = abs(voices[1][i+1]);
		newalto    = abs(voices[2][i+1]);

		sdir = newsoprano - soprano;
		adir = newalto - alto;
		tdir = newtenor - tenor;
		bdir = newbass - bass;

		if (sdir < 0) sdir = -1;
		if (adir < 0) adir = -1;
		if (tdir < 0) tdir = -1;
		if (bdir < 0) bdir = -1;

		if (sdir > 0) sdir = 1;
		if (adir > 0) adir = 1;
		if (tdir > 0) tdir = 1;
		if (bdir > 0) bdir = 1;

		// check against bass voice
		if (newbass == 0 || bass == 0 || bdir == 0) {
			goto tenorvoice2;
		}

		if (tenor != 0 && newtenor != 0 && bdir == tdir && tenor != bass) {
			if ((tenor-bass+400)%40 == 0 && (newtenor-newbass+400)%40 == 0) {
				errormessage(2, "bass", "tenor", linenum[i], linenum[i+1], infile);
			}
		}

		if (newalto != 0 && alto != 0 && bdir == adir && alto != bass) {
			if ((alto-bass+400)%40 == 0 && (newalto-newbass+400)%40 == 0) {
				errormessage(2, "bass", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0 && bdir == sdir && soprano != bass) {
			if ((soprano-bass+400)%40 == 0 && (newsoprano-newbass+400)%40 == 0) {
				errormessage(2, "bass", "soprano", linenum[i], linenum[i+1], infile);
			}
		}

tenorvoice2:

		if (newtenor == 0 || tenor == 0 || tdir == 0) {
			goto altovoice2;
		}

		if (newalto != 0 && alto != 0 && adir == tdir && tenor != alto) {
			if ((alto-tenor+400)%40 == 0 && (newalto-newtenor+400)%40 == 0) {
				errormessage(2, "tenor", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0 && adir == sdir && tenor != soprano) {
			if ((soprano-tenor+400)%40 == 0 && (newsoprano-newtenor+400)%40 == 0) {
				errormessage(2, "tenor", "soprano", linenum[i], linenum[i+1], infile);
			}
		}


altovoice2:

		if (newalto == 0 || alto == 0 || adir == 0) {
			continue;
		}

		if (newsoprano != 0 && soprano != 0 && adir == sdir && alto != soprano) {
			if ((soprano-alto+400)%40 == 0 && (newsoprano-newalto+400)%40 == 0) {
				errormessage(2, "alto", "soprano", linenum[i], linenum[i+1], infile);
			}
		}
	}
}



////////////////////////////
//
//	3. Contrary parallel 5ths -- when two voices move in
//	   parallel 5ths displaced by an octave.
//

void error3(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	int newbass, newtenor, newalto, newsoprano;
	int sdir, tdir, adir, bdir;   // melodic direction of the voices
	for (i=0; i<(int)voices[0].size()-1; i++) {
		bass    = abs(voices[0][i]);
		tenor   = abs(voices[1][i]);
		alto    = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		newsoprano = abs(voices[3][i+1]);
		newbass    = abs(voices[0][i+1]);
		newtenor   = abs(voices[1][i+1]);
		newalto    = abs(voices[2][i+1]);

		sdir = newsoprano - soprano;
		adir = newalto - alto;
		tdir = newtenor - tenor;
		bdir = newbass - bass;

		if (sdir < 0) sdir = -1;
		if (adir < 0) adir = -1;
		if (tdir < 0) tdir = -1;
		if (bdir < 0) bdir = -1;

		if (sdir > 0) sdir = 1;
		if (adir > 0) adir = 1;
		if (tdir > 0) tdir = 1;
		if (bdir > 0) bdir = 1;

		// check against bass voice
		if (newbass == 0 || bass == 0 || bdir == 0) {
			goto tenorvoice3;
		}
		if ((bass-newbass+400)%40 == 0) {
			goto tenorvoice3;
		}
	

		if (tenor != 0 && newtenor != 0 && bdir != tdir) {
			if ((tenor-bass+400)%40 == 23 && (newtenor-newbass+400)%40 == 23) {
				errormessage(3, "bass", "tenor", linenum[i], linenum[i+1], infile);
			}
		}

		if (newalto != 0 && alto != 0 && bdir != adir) {
			if ((alto-bass+400)%40 == 23 && (newalto-newbass+400)%40 == 23) {
				errormessage(3, "bass", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0 && bdir != sdir) {
			if ((soprano-bass+400)%40 == 23 && (newsoprano-newbass+400)%40 == 23) {
				errormessage(3, "bass", "soprano", linenum[i], linenum[i+1], infile);
			}
		}

tenorvoice3:

		if (newtenor == 0 || tenor == 0 || tdir == 0) {
			goto altovoice3;
		}
		if ((tenor-newtenor+400)%40 == 0) {
			goto altovoice3;
		}

		if (newalto != 0 && alto != 0 && adir != tdir) {
			if ((alto-tenor+40)%40 == 23 && (newalto-newtenor+40)%40 == 23) {
				errormessage(3, "tenor", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0 && adir != sdir) {
			if ((soprano-tenor+40)%40 == 23 && (newsoprano-newtenor+40)%40 == 23) {
				errormessage(3, "tenor", "soprano", linenum[i], linenum[i+1], infile);
			}
		}


altovoice3:

		if (newalto == 0 || alto == 0 || adir == 0) {
			continue;
		}
		if ((alto-newalto+400)%40 == 0) {
			return;
		}

		if (newsoprano != 0 && soprano != 0 && adir != sdir) {
			if ((soprano-alto+40)%40 == 23 && (newsoprano-newalto+40)%40 == 23) {
				errormessage(3, "alto", "soprano", linenum[i], linenum[i+1], infile);
			}
		}
	}
}



////////////////////////////
//
//	4. Unequal 5ths -- when the bass part and another
//	   voice move from dim 5ths to perfect 5ths or vice versa.
//

void error4(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	int newbass, newtenor, newalto, newsoprano;
	for (i=0; i<(int)voices[0].size()-1; i++) {
		bass = abs(voices[0][i]);
		tenor = abs(voices[1][i]);
		alto = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		newsoprano = abs(voices[3][i+1]);
		newbass = abs(voices[0][i+1]);
		newtenor = abs(voices[1][i+1]);
		newalto = abs(voices[2][i+1]);

		if (bass == 0 || newbass == 0) {
			continue;
		}

		int interval1, interval2;

		if (newtenor != 0 && tenor != 0) {
			interval1 = (tenor-bass+40)%40;
			interval2 = (newtenor-newbass+40)%40;
			if (interval1 == 23 && interval2 == 22) {
				errormessage(4, "bass", "tenor", linenum[i], linenum[i+1], infile);
			}
		}

		if (newalto != 0 && alto != 0) {
			interval1 = (alto-bass+40)%40;
			interval2 = (newalto-newbass+40)%40;
			if (interval1 == 23 && interval2 == 22) {
				errormessage(4, "bass", "alto", linenum[i], linenum[i+1], infile);
			}
		}

		if (newsoprano != 0 && soprano != 0) {
			interval1 = (soprano-bass+40)%40;
			interval2 = (newsoprano-newbass+40)%40;
			if (interval1 == 23 && interval2 == 22) {
				errormessage(4, "bass", "soprano", linenum[i], linenum[i+1], infile);
			}
		}

	}
}



////////////////////////////
//
//	5. Hidden 5ths -- when the soprano moves in similar
//	   motion with another voice and the soprano leaps
//	   to a perfect 5th with that voice.
//

void error5(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	int newbass, newtenor, newalto, newsoprano;
	int sdir, tdir, adir, bdir;   // melodic direction of the voices
	for (i=0; i<(int)voices[0].size()-1; i++) {
		bass = abs(voices[0][i]);
		tenor = abs(voices[1][i]);
		alto = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		newsoprano = abs(voices[3][i+1]);
		newbass = abs(voices[0][i+1]);
		newtenor = abs(voices[1][i+1]);
		newalto = abs(voices[2][i+1]);

		if (soprano == 0 || newsoprano == 0) {
			continue;
		}

		sdir = newsoprano - soprano;
		adir = newalto - alto;
		tdir = newtenor - tenor;
		bdir = newbass - bass;

		if (sdir < 0) sdir = -1;
		if (adir < 0) adir = -1;
		if (tdir < 0) tdir = -1;
		if (bdir < 0) bdir = -1;

		if (sdir > 0) sdir = 1;
		if (adir > 0) adir = 1;
		if (tdir > 0) tdir = 1;
		if (bdir > 0) bdir = 1;

		if (abs(newsoprano - soprano) > 7) {
			if (sdir == adir && newalto != 0 && alto != 0) {
				if ((newsoprano-newalto+40)%40 == 23) {
					errormessage(5, "soprano", "alto", linenum[i], linenum[i+1], infile);
				}
			}

			if (sdir == tdir && newtenor != 0 && tenor != 0) {
				if ((newsoprano-newtenor+40)%40 == 23) {
					errormessage(5, "soprano", "tenor", linenum[i], linenum[i+1], infile);
				}
			}

			if (sdir == bdir && newbass != 0 && bass != 0) {
				if ((newsoprano-newbass+40)%40 == 23) {
					errormessage(5, "soprano", "bass", linenum[i], linenum[i+1], infile);
				}
			}

		}
	}
}



////////////////////////////
//
//	6. Hidden 8va -- when the soprano moves in similar
//	   motion with another voice and the soprano leaps
//	   to a perfect octave with that voice.
//         Excluding parallel octaves which are covered
//         by rule 2.
//

void error6(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	int newbass, newtenor, newalto, newsoprano;
	int sdir, tdir, adir, bdir;   // melodic direction of the voices
	for (i=0; i<(int)voices[0].size()-1; i++) {
		bass    = abs(voices[0][i]);
		tenor   = abs(voices[1][i]);
		alto    = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		newbass    = abs(voices[0][i+1]);
		newtenor   = abs(voices[1][i+1]);
		newalto    = abs(voices[2][i+1]);
		newsoprano = abs(voices[3][i+1]);

		if (soprano == 0 || newsoprano == 0) {
			continue;
		}

		sdir = newsoprano - soprano;
		adir = newalto    - alto;
		tdir = newtenor   - tenor;
		bdir = newbass    - bass;

		if (sdir < 0) sdir = -1;
		if (adir < 0) adir = -1;
		if (tdir < 0) tdir = -1;
		if (bdir < 0) bdir = -1;

		if (sdir > 0) sdir = 1;
		if (adir > 0) adir = 1;
		if (tdir > 0) tdir = 1;
		if (bdir > 0) bdir = 1;

		if (abs(newsoprano - soprano) > 7) {
			if ((sdir == adir) && (newalto != 0) && (alto != 0)) {
				if (((newsoprano-newalto+400)%40 == 0) && (newsoprano != newalto)) {
					if ((soprano - alto + 400)%40 != 0) { // exclude para octaves
						errormessage(6, "soprano", "alto", linenum[i], linenum[i+1], infile);
					}
				}
			}

			if ((sdir == tdir) && (newtenor != 0) && (tenor != 0)) {
				if (((newsoprano-newtenor+400)%40 == 0) &&
						(newsoprano != newtenor)) {
					if ((soprano - tenor + 400)%40 != 0) { // exclude para octaves
						errormessage(6, "soprano", "tenor", linenum[i], linenum[i+1], infile);
					}
				}
			}

			if ((sdir == bdir) && (newbass != 0) && (bass != 0)) {
				if (((newsoprano-newbass+400)%40 == 0) && (newsoprano != newbass)) {
					if ((soprano - bass + 400)%40 != 0) { // exclude para octaves
						errormessage(6, "soprano", "bass", linenum[i], linenum[i+1], infile);
					}
				}
			}

		}
	}
}



////////////////////////////
//
//	7. Voice crossing -- when an inner voice goes above
//	   the soprano voice or below the bass voice.
//

void error7(HumdrumFile& infile) {
	int i;
	int bass, tenor, alto, soprano;
	for (i=0; i<(int)voices[0].size(); i++) {
		bass = abs(voices[0][i]);
		tenor = abs(voices[1][i]);
		alto = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		if (tenor > 0 && bass > 0) {
			if (tenor - bass < 0) {
				errormessage(7, "tenor", "bass", linenum[i], linenum[i+1], infile);
			}
		}
		if (alto > 0 && bass > 0) {
			if (alto - bass < 0) {
				errormessage(7, "alto", "bass", linenum[i], linenum[i+1], infile);
			}
		}

		if (soprano > 0 && alto > 0) {
			if (soprano - alto < 0) {
				errormessage(7, "soprano", "alto", linenum[i], linenum[i+1], infile);
			}
		}
		if (soprano > 0 && tenor > 0) {
			if (soprano - tenor < 0) {
				errormessage(7, "soprano", "tenor", linenum[i], linenum[i+1], infile);
			}
		}
	}
}



////////////////////////////
//
//	8. Open spacing -- when the interval between successive
//	   voices other than the bass exceeds an octave.
//

void error8(HumdrumFile& infile) {
	int i;
	// int bass;
	int tenor, alto, soprano;
	// int temp;
	for (i=0; i<(int)voices[0].size(); i++) {
		// bass = abs(voices[0][i]);
		tenor = abs(voices[1][i]);
		alto = abs(voices[2][i]);
		soprano = abs(voices[3][i]);

		if (tenor > 0 && alto > 0) {
			if (alto - tenor > 40) {
				errormessage(8, "alto", "tenor", linenum[i], linenum[i+1], infile);
			} else if (alto - tenor < 0) {
				// temp = alto;
	    alto = tenor;
				tenor = alto;
			}
		}

		if (alto > 0 && soprano > 0) {
			if (soprano - alto > 40) {
				errormessage(8, "soprano", "alto", linenum[i], linenum[i+1], infile);
			}
		}
	}
}



