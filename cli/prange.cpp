//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Mar 31 21:51:34 PST 2005
// Last Modified: Tue Dec 19 15:11:21 PST 2023 Reorganize data into _VoiceInfo class.
// Filename:      ...sig/examples/all/range.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/range.cpp
// vim:           ts=3
// Syntax:        C++; museinfo
//
// Description:   Calculate pitch histograms in a score of **kern data.
//

#include "PerlRegularExpression.h"
#include "humdrum.h"

#include <math.h>
#include <numeric>
#include <string.h>
#include <string>
#include <vector>

#define OBJTAB "\t\t\t\t\t\t"
#define SVGTAG "_99%svg%";

#define SVGTEXT(text) \
	if (defineQ) { \
		cout << "SVG "; \
	} else { \
		cout << "t 1 1\n"; \
		cout << SVGTAG; \
	} \
	printScoreEncodedText((text)); \
	cout << "\n"; 

class _VoiceInfo {
	public:
		vector<vector<double>> diatonic;
		vector<double> midibins;
		string name;  // name for instrument name of spine
		string abbr;  // abbreviation for instrument name of spine
		int track;    // track number for spine
		bool kernQ;   // is spine a **kern spine?
		double hpos;  // horizontal position on system for pitch range data for spine
		vector<int> diafinal;    // finalis note diatonic pitch (4=middle-C octave)
		vector<int> accfinal;    // finalis note accidental (0 = natural)
		vector<string> namfinal; // name of voice for finalis note (for "all" display)
		int index = -1;

	public:
		_VoiceInfo(void) {
			clear();
		}

		void clear(void) {
			name = "";
			abbr = "";
			midibins.resize(128);
			fill(midibins.begin(), midibins.end(), 0.0);
			diatonic.resize(7 * 12);
			for (int i=0; i<(int)diatonic.size(); i++) {
				diatonic[i].resize(6);
				fill(diatonic[i].begin(), diatonic[i].end(), 0.0);
			}
			track = -1;
			kernQ = false;
			diafinal.clear();
			accfinal.clear();
			namfinal.clear();
			index = -1;
		}

		ostream& print(ostream& out) {
			out << "==================================" << endl;
			out << "track:  " << track << endl;
			out << " name:  " << name << endl;
			out << " abbr:  " << abbr << endl;
			out << " kern:  " << kernQ << endl;
			out << " final:";
			for (int i=0; i<(int)diafinal.size(); i++) {
				out << " " << diafinal.at(i) << "/" << accfinal.at(i);
			}
			out << endl;
			out << " midi:  ";
			for (int i=0; i<midibins.size(); i++) {
				if (midibins.at(i) > 0.0) {
					out << " " << i << ":" << midibins.at(i);
				}
			}
			out << endl;
			out << " diat:  ";
			for (int i=0; i<diatonic.size(); i++) {
				if (diatonic.at(i).at(0) > 0.0) {
					out << " " << i << ":" << diatonic.at(i).at(0);
				}
			}
			out << endl;
			out << "==================================" << endl;
			return out;
		}

};



// function declarations
void   processOptions             (Options& opts, int argc, char* argv[]);
void   example                    (void);
void   usage                      (const char* command);
void   printAnalysis              (vector<double>& rdata);
void   printPercentile            (vector<double>& midibins, double percentile);
double countNotesInRange          (vector<double>& midibins, int low, int high);
void   fillHistograms             (vector<_VoiceInfo>& voiceInfo, HumdrumFile& infile);
void   getRange                   (int& rangeL, int& rangeH,
                                   const char* rangestring);
int    getTessitura               (vector<double>& midibins);
double getMean12                  (vector<double>& midibins);
int    getMedian12                (vector<double>& midibins);
void   printScoreVoice            (_VoiceInfo& voiceInfo, double maxvalue);
void   getVoice                   (string& voicestring, HumdrumFile& infile, int kernspine);
int    getMaxDiatonicIndex        (vector<vector<double>>& diatonic);
int    getMinDiatonicIndex        (vector<vector<double>>& diatonic);
int    getMinDiatonicAcc          (vector<vector<double>>& midibins, int index);
int    getMaxDiatonicAcc          (vector<vector<double>>& midibins, int index);
int    getStaffBase7              (int base7);
double getVpos                    (double base7);
double getMaxValue                (vector<vector<double>>& bins);
void   printScoreFile             (vector<_VoiceInfo>& voiceInfo,
                                   HumdrumFile& infile);
void   getTitle                   (string& titlestring, HumdrumFile& infile);
int    getTopQuartile             (vector<double>& midibins);
int    getBottomQuartile          (vector<double>& midibins);
int    getDiatonicInterval        (int note1, int note2);
void   printFilenameBase          (const string& filename);
void   printXmlEncodedText        (const string& strang);
void   printScoreEncodedText      (const string& strang);
int    getKeySignature            (HumdrumFile& infile);
void   printHTMLStringEncodeSimple(const string& strang);
void   printDiatonicPitchName     (int base7, int acc);
string getDiatonicPitchName       (int base7, int acc);
string getNoteTitle               (double value, int diatonic, int acc);
void   getInstrumentNames         (vector<string>& nameByTrack,
                                   vector<int>& kernSpines,
                                   HumdrumFile& infile);
void   getVoiceInfo               (vector<_VoiceInfo>& voiceInfo, HumdrumFile& infile);
void   mergeAllVoiceInfo          (vector<_VoiceInfo>& voiceInfo);
void   assignHorizontalPosition   (vector<_VoiceInfo>& voiceInfo, int minval, int maxval);
void   mergeFinals                (vector<_VoiceInfo>& voiceInfo, 
                                   vector<vector<int>>& diafinal,
                                   vector<vector<int>>& accfinal);
void   printKeySigCompression     (int keysig, int extra);

// global variables
Options      options;              // database for command-line arguments
int          allQ         = 0;     // used with -a option
bool         durationQ    = false; // used with -d option
bool         percentileQ  = false; // used with -p option
bool         addfractionQ = false; // used with -f option
double       percentile   = 0.0;   // used with -p option
bool         rangeQ       = false; // used with -r option
bool         localQ       = false; // used with -l option
bool         reverseQ     = false; // used with -r option
int          rangeL       = 0;     // used with -r option
bool         pitchQ       = false; // used with --pitch option
int          rangeH       = 0;     // used with -r option
bool         printQ       = false; // used with --print option
bool         normQ        = false; // used with -N option
bool         scoreQ       = false; // used with --score option
bool         diatonicQ    = false; // used with -D option
bool         hoverQ       = false; // used with --hover option
bool         keyQ         = true;  // used with --no-key option
bool         finalisQ     = false; // used with --finalis option
bool         quartileQ    = false; // used with --quartile option
bool         fillonlyQ    = false; // used with --fill option
bool         defineQ      = true;  // used with --no-define option
bool         debugQ       = false; // used with --debug option
bool         accQ         = false; // used with --acc option
bool         base40Q      = false; // used with --base40 option
bool         instrumentQ  = false; // used with -i option
bool         titleQ       = false; // used wit --title option
bool         notitleQ     = false; // used wit --no-title option
string       Title        = "";    // used with --title option
string       FILENAME     = "";

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	// process the command-line options
	processOptions(options, argc, argv);
	HumdrumStream streamer(options);
	HumdrumFile infile;
	vector<_VoiceInfo> voiceInfo;

	// Can only handle one input if SCORE display is being given.
	// if no command-line arguments read data file from standard input
	while (streamer.read(infile)) {
		getVoiceInfo(voiceInfo, infile);
		fillHistograms(voiceInfo, infile);
		if (debugQ) {
			for (int i=0; i<(int)voiceInfo.size(); i++) {
				voiceInfo[i].print(cerr);
			}
		}

		if (scoreQ) {
			printScoreFile(voiceInfo, infile);
		}
	}

	if (!scoreQ) {
		printAnalysis(voiceInfo[0].midibins);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// mergeAllVoiceInfo --
//

void mergeAllVoiceInfo(vector<_VoiceInfo>& voiceInfo) {
	voiceInfo.at(0).diafinal.clear();
	voiceInfo.at(0).accfinal.clear();

	for (int i=1; i<(int)voiceInfo.size(); i++) {
		if (!voiceInfo[i].kernQ) {
			continue;
		}
		for (int j=0; j<(int)voiceInfo.at(i).diafinal.size(); j++) {
			voiceInfo.at(0).diafinal.push_back(voiceInfo.at(i).diafinal.at(j));
			voiceInfo.at(0).accfinal.push_back(voiceInfo.at(i).accfinal.at(j));
			voiceInfo.at(0).namfinal.push_back(voiceInfo.at(i).name);
		}

		for (int j=0; j<(int)voiceInfo[i].midibins.size(); j++) {
			voiceInfo[0].midibins[j] += voiceInfo[i].midibins[j];
		}

		for (int j=0; j<(int)voiceInfo.at(i).diatonic.size(); j++) {
			for (int k=0; k<(int)voiceInfo.at(i).diatonic.at(k).size(); k++) {
				voiceInfo[0].diatonic.at(j).at(k) += voiceInfo.at(i).diatonic.at(j).at(k);
			}
		}
	}
}



//////////////////////////////
//
// getVoiceInfo -- get names and track info for **kern spines.
//

void getVoiceInfo(vector<_VoiceInfo>& voiceInfo, HumdrumFile& infile) {
	voiceInfo.clear();
	voiceInfo.resize(infile.getMaxTracks() + 1);
	for (int i=0; i<(int)voiceInfo.size(); i++) {
		voiceInfo.at(i).index = i;
	}
	voiceInfo[0].name  = "all";
	voiceInfo[0].abbr  = "all";
	voiceInfo[0].track = 0;

	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isData()) {
			break;
		}
		if (!infile[i].hasSpines()) {
			continue;
		}
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			int track = infile[i].getTrack(j);
			voiceInfo[track].track = track;
			if (strcmp(infile[i][j], "**kern") == 0) {
				voiceInfo[track].kernQ = true;
			}
			if (!voiceInfo[track].kernQ) {
				continue;
			}
			if (strncmp(infile[i][j], "*I\"", 3) == 0) {
				voiceInfo[track].name = &infile[i][j][3];
			}
			if (strncmp(infile[i][j], "*I'", 3) == 0) {
				voiceInfo[track].abbr = &infile[i][j][3];
			}
		}
	}
}




//////////////////////////////
//
// getInstrumentNames --  Find any instrument names which are listed
//      before the first data line.  Instrument names are in the form:
//
//      *I"name
//

void getInstrumentNames(vector<string>& nameByTrack, vector<int>& kernSpines,
		HumdrumFile& infile) {
	PerlRegularExpression pre;
	int i, j, k;
	int track;
	string name;
	// nameByTrack.resize(kernSpines.size());
	nameByTrack.resize(infile.getMaxTracks() + 1);
	fill(nameByTrack.begin(), nameByTrack.end(), "");
	nameByTrack.at(0) = "all";
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (pre.search(infile[i][j], "^\\*I\"(.*)\\s*")) {
				name = pre.getSubmatch(1);
				track = infile[i].getPrimaryTrack(j);
				for (k=0; k<(int)kernSpines.size(); k++) {
					if (track == kernSpines[k]) {
						nameByTrack[k] = name;
					}
				}
			}
		}
	}
}



//////////////////////////////
//
// fillHistograms -- Store notes in score by MIDI note number.
//

void fillHistograms(vector<_VoiceInfo>& voiceInfo, HumdrumFile& infile) {
	// storage for finals info:
	vector<vector<int>> diafinal;
	vector<vector<int>> accfinal;
	diafinal.resize(infile.getMaxTracks() + 1);
	accfinal.resize(infile.getMaxTracks() + 1);

	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
			continue;
		}
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (!infile[i].isKern(j)) {
				continue;
			}
			if (strcmp(infile[i][j], ".") == 0) {
				continue;
			}
			int track = infile[i].getTrack(j);

			diafinal.at(track).clear();
			accfinal.at(track).clear();

			vector<string> tokens;
			infile[i].getTokens(tokens, j);
			for (int k=0; k<(int)tokens.size(); k++) {
				if (strchr(tokens[k].c_str(), 'r') != NULL) {
					continue;
				}
				int octave = Convert::kernToOctave(tokens[k]) + 3;
				if (octave < 0) {
					cerr << "Note too low: " << tokens[k] << endl;
					continue;
				}
				if (octave >= 12) {
					cerr << "Note too high: " << tokens[k] << endl;
					continue;
				}
				int dpc    = (Convert::kernToDiatonicPitchClass(tokens[k]) - 'a' - 2 + 7) % 7;;
				int acc    = Convert::kernToDiatonicAlteration(tokens[k]);
				if (acc < -2) {
					cerr << "Accidental too flat: " << tokens[k] << endl;
					continue;
				}
				if (acc > +2) {
					cerr << "Accidental too sharp: " << tokens[k] << endl;
					continue;
				}
				int diatonic = dpc + 7 * octave;
				int realdiatonic = dpc + 7 * (octave-3);

				diafinal.at(track).push_back(realdiatonic);
				accfinal.at(track).push_back(acc);

				acc += 3;
				int midi = Convert::kernToMidiNoteNumber(tokens[k]);
				if (midi < 0) {
					cerr << "MIDI pitch too low: " << tokens[k] << endl;
				}
				if (midi > 127) {
					cerr << "MIDI pitch too high: " << tokens[k] << endl;
				}
				if (durationQ) {
					double duration = Convert::kernToDuration(tokens[k]);
					voiceInfo[track].diatonic.at(diatonic).at(0) += duration;
					voiceInfo[track].diatonic.at(diatonic).at(acc) += duration;
					voiceInfo[track].midibins.at(midi) += duration;
				} else {
					if (strchr(tokens[k].c_str(), ']') != NULL) {
						continue;
					}
					if (strchr(tokens[k].c_str(), '_') != NULL) {
						continue;
					}
					voiceInfo[track].diatonic.at(diatonic).at(0)++;
					voiceInfo[track].diatonic.at(diatonic).at(acc)++;
					voiceInfo[track].midibins.at(midi)++;
				}
			}
		}
	}

	mergeFinals(voiceInfo, diafinal, accfinal);

	// Sum all voices into midibins and diatonic arrays of vector position 0:
	mergeAllVoiceInfo(voiceInfo);
}



//////////////////////////////
//
// mergeFinals --
//

void mergeFinals(vector<_VoiceInfo>& voiceInfo, vector<vector<int>>& diafinal,
		vector<vector<int>>& accfinal) {
	for (int i=0; i<(int)voiceInfo.size(); i++) {
		voiceInfo.at(i).diafinal = diafinal.at(i);
		voiceInfo.at(i).accfinal = accfinal.at(i);
	}
}



//////////////////////////////
//
// printFilenameBase --
//

void printFilenameBase(const string& filename) {
	PerlRegularExpression pre;
	if (pre.search(filename, "([^/]+)\\.([^.]*)", "")) {
		if (strlen(pre.getSubmatch(1)) <= 8) {
			printXmlEncodedText(pre.getSubmatch(1));
		} else {
			// problem with too long a name (MS-DOS will have problems).
			// optimize to chop off everything after the dash in the
			// name (for Josquin catalog numbers).
			PerlRegularExpression pre2;
			string shortname = pre.getSubmatch();
			if (pre2.sar(shortname, "-.*", "", "")) {
				printXmlEncodedText(shortname);
			} else {
				printXmlEncodedText(shortname);
			}
		}
	}
}


//////////////////////////////
//
// printReferenceRecords --
//

void printReferenceRecords(HumdrumFile& infile) {
	int i;
	char buffer[1024] = {0};
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isBibliographic()) {
			continue;
		}
		cout <<  "\t\t\t\t\t\t<?Humdrum key=\"";
		printXmlEncodedText(infile[i].getBibKey(buffer));
		cout << "\" value=\"";
		printXmlEncodedText(infile[i].getBibValue(buffer));
		cout << "\"?>\n";
	}
}



//////////////////////////////
//
// printScoreEncodedText -- print SCORE text string
//    See SCORE 3.1 manual additions (page 19) for more.
//

void printScoreEncodedText(const string& strang) {
	string newstring = strang;
	PerlRegularExpression pre;
	// pre.sar(newstring, "&(.)acute;", "<<$1", "g");
	pre.sar(newstring, "&aacute;", "<<a", "g");
	pre.sar(newstring, "&eacute;", "<<e", "g");
	pre.sar(newstring, "&iacute;", "<<i", "g");
	pre.sar(newstring, "&oacute;", "<<o", "g");
	pre.sar(newstring, "&uacute;", "<<u", "g");
	pre.sar(newstring, "&Aacute;", "<<A", "g");
	pre.sar(newstring, "&Eacute;", "<<E", "g");
	pre.sar(newstring, "&Iacute;", "<<I", "g");
	pre.sar(newstring, "&Oacute;", "<<O", "g");
	pre.sar(newstring, "&Uacute;", "<<U", "g");

	// pre.sar(newstring, "&(.)grave;", ">>$1", "g");
	pre.sar(newstring, "&agrave;", ">>a", "g");
	pre.sar(newstring, "&egrave;", ">>e", "g");
	pre.sar(newstring, "&igrave;", ">>i", "g");
	pre.sar(newstring, "&ograve;", ">>o", "g");
	pre.sar(newstring, "&ugrave;", ">>u", "g");
	pre.sar(newstring, "&Agrave;", ">>A", "g");
	pre.sar(newstring, "&Egrave;", ">>E", "g");
	pre.sar(newstring, "&Igrave;", ">>I", "g");
	pre.sar(newstring, "&Ograve;", ">>O", "g");
	pre.sar(newstring, "&Ugrave;", ">>U", "g");

	// pre.sar(newstring, "&(.)uml;",   "%%$1", "g");
	pre.sar(newstring, "&auml;",   "%%a", "g");
	pre.sar(newstring, "&euml;",   "%%e", "g");
	pre.sar(newstring, "&iuml;",   "%%i", "g");
	pre.sar(newstring, "&ouml;",   "%%o", "g");
	pre.sar(newstring, "&uuml;",   "%%u", "g");
	pre.sar(newstring, "&Auml;",   "%%A", "g");
	pre.sar(newstring, "&Euml;",   "%%E", "g");
	pre.sar(newstring, "&Iuml;",   "%%I", "g");
	pre.sar(newstring, "&Ouml;",   "%%O", "g");
	pre.sar(newstring, "&Uuml;",   "%%U", "g");

	// pre.sar(newstring, "&(.)circ;",  "^^$1", "g");
	pre.sar(newstring, "&acirc;",  "^^a", "g");
	pre.sar(newstring, "&ecirc;",  "^^e", "g");
	pre.sar(newstring, "&icirc;",  "^^i", "g");
	pre.sar(newstring, "&ocirc;",  "^^o", "g");
	pre.sar(newstring, "&ucirc;",  "^^u", "g");
	pre.sar(newstring, "&Acirc;",  "^^A", "g");
	pre.sar(newstring, "&Ecirc;",  "^^E", "g");
	pre.sar(newstring, "&Icirc;",  "^^I", "g");
	pre.sar(newstring, "&Ocirc;",  "^^O", "g");
	pre.sar(newstring, "&Ucirc;",  "^^U", "g");

	// pre.sar(newstring, "&(.)cedil;", "##$1", "g");
	pre.sar(newstring, "&ccedil;", "##c", "g");
	pre.sar(newstring, "&Ccedil;", "##C", "g");

	pre.sar(newstring, "\\|",        "?|",   "g");
	pre.sar(newstring, "\\\\",       "?\\",  "g");
	pre.sar(newstring, "---",        "?m",   "g");
	pre.sar(newstring, "--",         "?n",   "g");
	pre.sar(newstring, "-sharp",     "?2",   "g");
	pre.sar(newstring, "-flat",      "?1",   "g");
	pre.sar(newstring, "-natural",   "?3",   "g");
	pre.sar(newstring, "/",          "\\",   "g");
	pre.sar(newstring, "\\[",        "?[",   "g");
	pre.sar(newstring, "\\]",        "?]",   "g");

	cout << newstring;
}



//////////////////////////////
//
// printXmlEncodedText -- convert
//    & to &amp;
//    " to &quot;
//    ' to &spos;
//    < to &lt;
//    > to &gt;
//

void printXmlEncodedText(const string& strang) {
	PerlRegularExpression pre;
	string astring = strang;

	pre.sar(astring, "&",  "&amp;",  "g");
	pre.sar(astring, "'",  "&apos;", "g");
	pre.sar(astring, "\"", "&quot;", "g");
	pre.sar(astring, "<",  "&lt;",   "g");
	pre.sar(astring, ">",  "&gt;",   "g");

	cout << astring;
}



//////////////////////////////
//
// printScoreFile --
//

void printScoreFile(vector<_VoiceInfo>& voiceInfo, HumdrumFile& infile) {
	string titlestring;
	if (titleQ) {
		titlestring = Title;
	} else if (notitleQ) {
		titlestring = "";
	} else {
		getTitle(titlestring, infile);
	}

	if (defineQ) {
		cout << "#define SVG t 1 1 \\n_99%svg%\n";
	}

	string acctext = "g.bar.doubleflat path&#123;color:darkorange;stroke:darkorange;&#125;g.bar.flat path&#123;color:brown;stroke:brown;&#125;g.bar.sharp path&#123;color:royalblue;stroke:royalblue;&#125;g.bar.doublesharp path&#123;color:aquamarine;stroke:aquamarine;&#125;";
	string hovertext = ".bar:hover path&#123;fill:red;color:red;stroke:red &#33;important&#125;";
	string hoverfilltext = hovertext;

	string text1 = "<style>";
	text1 += hoverfilltext;
	if (accQ) {
		text1 += acctext;
	}
	text1 += "g.labeltext&#123;color:gray;&#125;";
	text1 += "g.lastnote&#123;color:gray;&#125;";
	text1 += "</style>";
	string text2 = text1;
	

	// print CSS style information if requested
	if (hoverQ) {
		SVGTEXT(text1);
	}

	if (!titlestring.empty()) {
		// print title
		cout << "t 2 10 14 1 1 0 0 0 0 -1.35\n";
		// cout << "_03";
		printScoreEncodedText(titlestring);
		cout << "\n";
	}

	// print duration label if duration weighting is being used
	SVGTEXT("<g class=\"labeltext\">");
	if (durationQ) {
		cout << "t 2 185.075 14 1 0.738 0 0 0 0 0\n";
		cout << "_00(durations)\n";
	} else {
		cout << "t 2 185.075 14 1 0.738 0 0 0 0 0\n";
		cout << "_00(attacks)\n";
	}
	SVGTEXT("</g>");

	// print staff lines
	cout << "8 1 0 0 0 200\n";   // staff 1
	cout << "8 2 0 -6 0 200\n";   // staff 2

	int keysig = getKeySignature(infile);
	// print key signature
	if (keysig) {
		cout << "17 1 10 0 " << keysig << " 101.0";
		printKeySigCompression(keysig, 0);
		cout << endl;
		cout << "17 2 10 0 " << keysig;
		printKeySigCompression(keysig, 1);
		cout << endl;
	}

	// print barlines
	cout << "14 1 0 2\n";         // starting barline
	cout << "14 1 200 2\n";       // ending barline
	cout << "14 1 0 2 8\n";       // curly brace at start

	// print clefs
	cout << "3 2 2\n";            // treble clef
	cout << "3 1 2 0 1\n";        // bass clef

	assignHorizontalPosition(voiceInfo, 25.0, 170.0);

	double maxvalue = 0.0;
	for (int i=1; i<(int)voiceInfo.size(); i++) {
		double tempvalue = getMaxValue(voiceInfo.at(i).diatonic);
		if (tempvalue > maxvalue) {
			maxvalue = tempvalue;
		}
	}

	for (int i=(int)voiceInfo.size()-1; i>0; i--) {
		if (voiceInfo.at(i).kernQ) {
			printScoreVoice(voiceInfo.at(i), maxvalue);
		}
	}
	if (allQ) {
		printScoreVoice(voiceInfo.at(0), maxvalue);
	}
}



//////////////////////////////
//
// printKeySigCompression --
//

void printKeySigCompression(int keysig, int extra) {
	double compression = 0.0;
	switch (abs(keysig)) {
		case 0: compression = 0.0; break;
		case 1: compression = 0.0; break;
		case 2: compression = 0.0; break;
		case 3: compression = 0.0; break;
		case 4: compression = 0.9; break;
		case 5: compression = 0.8; break;
		case 6: compression = 0.7; break;
		case 7: compression = 0.6; break;
	}
	if (compression <= 0.0) {
		return;
	}
	for (int i=0; i<extra; i++) {
		cout << " 0";
	}
	cout << " " << compression;
}



//////////////////////////////
//
// assignHorizontalPosition --
//

void assignHorizontalPosition(vector<_VoiceInfo>& voiceInfo, int minval, int maxval) {
	int count = 0;
	for (int i=1; i<(int)voiceInfo.size(); i++) {
		if (voiceInfo[i].kernQ) {
			count++;
		}
	}
	if (allQ) {
		count++;
	}

	vector<double> hpos(count, 0);
	hpos[0] = maxval;
	hpos.back() = minval;

	if (hpos.size() > 2) {
		for (int i=1; i<(int)hpos.size()-1; i++) {
			int ii = hpos.size() - i - 1;
			hpos[i] = (double)ii / (hpos.size()-1) * (maxval - minval) + minval;
		}
	}

	int position = 0;
	if (allQ) {
		position = 1;
		voiceInfo[0].hpos = hpos[0];
	}
	for (int i=0; i<(int)voiceInfo.size(); i++) {
		if (voiceInfo.at(i).kernQ) {
			voiceInfo.at(i).hpos = hpos.at(position++);
		}
	}
}



//////////////////////////////
//
// getKeySignature -- find first key signature in file.
//

int getKeySignature(HumdrumFile& infile) {
	PerlRegularExpression pre;
	for (int i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			if (infile[i].isData()) {
				break;
			}
			continue;
		}
		for (int j=0; j<infile[i].getFieldCount(); j++) {
			if (pre.search(infile[i][j], "^\\*k\\[(.*)\\]", "")) {
				return Convert::kernKeyToNumber(infile[i][j]);
			}
		}
	}

	return 0; // C major key signature
}



//////////////////////////////
//
// printScoreVoice -- print the range information for a particular voice (in SCORE format).
//


void printScoreVoice(_VoiceInfo& voiceInfo, double maxvalue) {
	int mini = getMinDiatonicIndex(voiceInfo.diatonic);
	int maxi = getMaxDiatonicIndex(voiceInfo.diatonic);
	// int minacci = getMinDiatonicAcc(voiceInfo.diatonic, mini);
	// int maxacci = getMaxDiatonicAcc(voiceInfo.diatonic, maxi);
	int mindiatonic = mini - 3 * 7;
	int maxdiatonic = maxi - 3 * 7;
	// int minacc = minacci - 3;
	// int maxacc = maxacci - 3;
	
	int    staff;
	double vpos;

	int voicevpos = -3;
	staff = getStaffBase7(mindiatonic);
	int lowestvpos = getVpos(mindiatonic);
	if ((staff == 1) && (lowestvpos <= 0)) {
		voicevpos += lowestvpos;
		voicevpos -= 1;
	}

	if (localQ || (voiceInfo.index == 0)) {
		double localmaxvalue = getMaxValue(voiceInfo.diatonic);
		maxvalue = localmaxvalue;
	}
	double width;
	double hoffset = 2.3333;
	double maxhist = 17.6;
	int i;
	int base7;


	// print histogram bars
	for (i=mini; i<=maxi; i++) {
		if (voiceInfo.diatonic.at(i).at(0) <= 0.0) {
			continue;
		}
		base7 = i - 3 * 7;
		staff = getStaffBase7(base7);
		vpos  = getVpos(base7);

		// staring positions of accidentals:
		vector<double> starthpos(6, 0.0);
		for (int j=1; j<(int)starthpos.size(); j++) {
			double width = maxhist * voiceInfo.diatonic.at(i).at(j)/maxvalue;
			starthpos[j] = starthpos[j-1] + width;
		}
		for (int j=(int)starthpos.size() - 1; j>0; j--) {
			starthpos[j] = starthpos[j-1];
		}

		// print chromatic alterations
		for (int j=(int)voiceInfo.diatonic.at(i).size()-1; j>0; j--) {
			if (voiceInfo.diatonic.at(i).at(j) <= 0.0) {
				continue;
			}
			int acc = 0;
			switch (j) {
				case 1: acc = -2; break;
				case 2: acc = -1; break;
				case 3: acc =  0; break;
				case 4: acc = +1; break;
				case 5: acc = +2; break;
			}

			width = maxhist * voiceInfo.diatonic.at(i).at(j)/maxvalue + hoffset;
			if (hoverQ) {
				string title = getNoteTitle((int)voiceInfo.diatonic.at(i).at(j), base7, acc);
				SVGTEXT(title);
			}
			cout << "1 " << staff << " " << (voiceInfo.hpos + starthpos.at(j) + hoffset) << " " << vpos;
			cout << " 0 -1 4 0 0 0 99 0 0 ";
			cout << width << "\n";
			if (hoverQ) {
				SVGTEXT("</g>");
			}
		}
	}
	
	string voicestring = voiceInfo.name;
	if (voicestring.empty()) {
		voicestring = voiceInfo.abbr;
	}
	if (!voicestring.empty()) {
		// print voice name
		double tvoffset = -2.0;
		cout << "t 1 " << voiceInfo.hpos << " " << voicevpos
			  << " 1 1 0 0 0 0 " << tvoffset;
		cout << "\n";
		if (voicestring == "all") {
			cout << "_02";
		} else {
			cout << "_00";
		}
		printScoreEncodedText(voicestring);
		cout << "\n";
	}

	// print the lowest pitch in range
	staff = getStaffBase7(mindiatonic);
	vpos = getVpos(mindiatonic);
	if (hoverQ) {
		string content = "<g><title>";
		content += getDiatonicPitchName(mindiatonic, 0);
		content += ": lowest note";
		if (!voicestring.empty()) {
			content += " of ";
			content += voicestring;
			content += "'s range";
		}
		content += "</title>";
		SVGTEXT(content);
	}
	cout << "1 " << staff << " " << voiceInfo.hpos << " " << vpos
		  << " 0 0 4 0 0 -2\n";
	if (hoverQ) {
		SVGTEXT("</g>");
	}

	// print the highest pitch in range
	staff = getStaffBase7(maxdiatonic);
	vpos = getVpos(maxdiatonic);
	if (hoverQ) {
		string content = "<g><title>";
		content += getDiatonicPitchName(maxdiatonic, 0);
		content += ": highest note";
		if (!voicestring.empty()) {
			content += " of ";
			content += voicestring;
			content += "'s range";
		}
		content += "</title>";
		SVGTEXT(content);
	}
	cout << "1 " << staff << " " << voiceInfo.hpos << " " << vpos
		  << " 0 0 4 0 0 -2\n";
	if (hoverQ) {
		SVGTEXT("</g>");
	}

	double goffset  = -1.66;
	double toffset  = 1.5;
	double median12 = getMedian12(voiceInfo.midibins);
	double median40 = Convert::base12ToBase40(median12);
	double median7  = Convert::base40ToDiatonic(median40);
	// int    acc      = Convert::base40ToAccidental(median40);

	staff = getStaffBase7(median7);
	vpos = getVpos(median7);

	// these offsets are useful when the quartile pitches are not shown...
	int vvpos = maxdiatonic - median7 + 1;
	int vvpos2 = median7 - mindiatonic + 1;
	double offset = goffset;
	if (vvpos <= 2) {
		offset += toffset;
	} else if (vvpos2 <= 2) {
		offset -= toffset;
	}

	if (hoverQ) {
		string content = "<g><title>";
		content += getDiatonicPitchName(median7, 0);
		content += ": median note";
		if (!voicestring.empty()) {
			content += " of ";
			content += voicestring;
			content += "'s range";
		}
		content += "</title>";
		SVGTEXT(content);
	}
	cout << "1 " << staff << " " << voiceInfo.hpos << " ";
	if (vpos > 0) {
		cout << vpos + 100;
	} else {
		cout << vpos - 100;
	}
	cout << " 0 1 4 0 0 " << offset << "\n";
	if (hoverQ) {
		SVGTEXT("</g>");
	}

	if (finalisQ) {
		for (int f=0; f<(int)voiceInfo.diafinal.size(); f++) {
			int diafinalis = voiceInfo.diafinal.at(f);
			int accfinalis = voiceInfo.accfinal.at(f);
			int staff = getStaffBase7(diafinalis);
			int vpos = getVpos(diafinalis);
			double goffset = -1.66;
			double toffset = 3.5;

			// these offsets are useful when the quartile pitches are not shown...
			double offset = goffset;
			offset += toffset;

			if (hoverQ) {
				string content = "<g class=\"lastnote\"><title>";
				content += getDiatonicPitchName(diafinalis, accfinalis);
				content += ": last note";
				if (!voicestring.empty()) {
					content += " of ";
					if (voiceInfo.index == 0) {
						content += voiceInfo.namfinal.at(f);
					} else {
						content += voicestring;
					}
				}
				content += "</title>";
				SVGTEXT(content);
			}
			cout << "1 " << staff << " " << voiceInfo.hpos << " ";
			if (vpos > 0) {
				cout << vpos + 100;
			} else {
				cout << vpos - 100;
			}
			cout << " 0 0 4 0 0 " << offset << "\n";
			if (hoverQ) {
				SVGTEXT("</g>");
			}
		}
	}

	/* Needs fixing
	int topquartile;
	if (quartileQ) {
		// print top quartile
		topquartile = getTopQuartile(voiceInfo.midibins);
		if (diatonicQ) {
			topquartile = Convert::base7ToBase12(topquartile);
		}
		staff = getStaffBase7(topquartile);
		vpos = getVpos(topquartile);
		vvpos = median7 - topquartile + 1;
		if (vvpos <= 2) {
			offset = goffset + toffset;
		} else {
			offset = goffset;
		}
		vvpos = maxdiatonic - topquartile + 1;
		if (vvpos <= 2) {
			offset = goffset + toffset;
		}

		if (hoverQ) {
			if (defineQ) {
				cout << "SVG ";
			} else {
				cout << "t 1 1\n";
				cout << SVGTAG;
			}
			printScoreEncodedText("<g><title>");
			printDiatonicPitchName(topquartile, 0);
			cout << ": top quartile note";
			if (voicestring.size() > 0) {
				cout <<  " of " << voicestring << "\'s range";
			}
			printScoreEncodedText("</title>\n");
		}
		cout << "1 " << staff << " " << voiceInfo.hpos << " ";
		if (vpos > 0) {
			cout << vpos + 100;
		} else {
			cout << vpos - 100;
		}
		cout << " 0 0 4 0 0 " << offset << "\n";
		if (hoverQ) {
			SVGTEXT("</g>");
		}
	}
	
	// print bottom quartile
	if (quartileQ) {
		int bottomquartile = getBottomQuartile(voiceInfo.midibins);
		if (diatonicQ) {
			bottomquartile = Convert::base7ToBase12(bottomquartile);
		}
		staff = getStaffBase7(bottomquartile);
		vpos = getVpos(bottomquartile);
		vvpos = median7 - bottomquartile + 1;
		if (vvpos <= 2) {
			offset = goffset + toffset;
		} else {
			offset = goffset;
		}
		vvpos = bottomquartile - mindiatonic + 1;
		if (vvpos <= 2) {
			offset = goffset - toffset;
		}
		if (hoverQ) {
			if (defineQ) {
				cout << "SVG ";
			} else {
				cout << "t 1 1\n";
				cout << SVGTAG;
			}
			printScoreEncodedText("<g><title>");
			printDiatonicPitchName(bottomquartile, 0);
			cout << ": bottom quartile note";
			if (voicestring.size() > 0) {
				cout <<  " of " << voicestring << "\'s range";
			}
			printScoreEncodedText("</title>\n");
		}
		cout << "1.0 " << staff << ".0 " << voiceInfo.hpos << " ";
		if (vpos > 0) {
			cout << vpos + 100;
		} else {
			cout << vpos - 100;
		}
		cout << " 0 0 4 0 0 " << offset << "\n";
		if (hoverQ) {
			SVGTEXT("</g>");
		}
	}
	*/

}



//////////////////////////////
//
// printDiatonicPitchName --
//

void printDiatonicPitchName(int base7, int acc) {
	cout << getDiatonicPitchName(base7, acc);
}



//////////////////////////////
//
// getDiatonicPitchName --
//

string getDiatonicPitchName(int base7, int acc) {
	string output;
	int dpc = base7 % 7;
	char letter = (dpc + 2) % 7 + 'A';
	output += letter;
	switch (acc) {
		case -1: output += "&#9837;"; break;
		case +1: output += "&#9839;"; break;
		case -2: output += "&#119083;"; break;
		case +2: output += "&#119082;"; break;
	}
	int octave = base7 / 7;
	output += to_string(octave);
	return output;
}



//////////////////////////////
//
// printHTMLStringEncodeSimple --
//

void printHTMLStringEncodeSimple(const string& strang) {
	string newstring = strang;
	PerlRegularExpression pre;
	pre.sar(newstring, "&", "&amp;", "g");
	pre.sar(newstring, "<", "&lt;", "g");
	pre.sar(newstring, ">", "&lt;", "g");
	cout << newstring;
}



//////////////////////////////
//
// getNoteTitle -- return the title of the histogram bar.
//    value = duration or count of notes
//    diatonic = base7 value for note
//    acc = accidental for diatonic note.
//

string getNoteTitle(double value, int diatonic, int acc) {
	stringstream output;
	output << "<g class=\"bar";
	switch (acc) {
		case -2: output << " doubleflat";  break;
		case -1: output << " flat";        break;
		case  0: output << " natural";     break;
		case +1: output << " sharp";       break;
		case +2: output << " doublesharp"; break;
	}
	output << "\"";
	output << "><title>";
	if (durationQ) {
		output << value / 8.0;
		if (value/8.0 == 1.0) {
			output << " long on ";
		} else {
			output << " longs on ";
		}
		output << getDiatonicPitchName(diatonic, acc);
	} else {
		output << value;
		output << " ";
		output << getDiatonicPitchName(diatonic, acc);
		if (value != 1.0) {
			output << "s";
		}
	}
	output << "</title>";
	return output.str();
}



//////////////////////////////
//
// getDiatonicInterval --
//

int getDiatonicInterval(int note1, int note2) {
	int vpos1 = getVpos(note1);
	int vpos2 = getVpos(note2);
	return abs(vpos1 - vpos2) + 1;
}



//////////////////////////////
//
// getTopQuartile --
//

int getTopQuartile(vector<double>& midibins) {
	double sum = accumulate(midibins.begin(), midibins.end(), 0.0);

	double cumsum = 0.0;
	int i;
	for (i=midibins.size()-1; i>=0; i--) {
		if (midibins[i] <= 0.0) {
			continue;
		}
		cumsum += midibins[i]/sum;
		if (cumsum >= 0.25) {
			return i;
		}
	}

	return -1;
}



//////////////////////////////
//
// getBottomQuartile --
//

int getBottomQuartile(vector<double>& midibins) {
	double sum = accumulate(midibins.begin(), midibins.end(), 0.0);

	double cumsum = 0.0;
	int i;
	for (i=0; i<(int)midibins.size(); i++) {
		if (midibins[i] <= 0.0) {
			continue;
		}
		cumsum += midibins[i]/sum;
		if (cumsum >= 0.25) {
			return i;
		}
	}

	return -1;
}



//////////////////////////////
//
// getMaxValue --
//

double getMaxValue(vector<vector<double>>& bins) {
	double maxi = 0;
	for (int i=1; i<(int)bins.size(); i++) {
		if (bins.at(i).at(0) > bins.at(maxi).at(0)) {
			maxi = i;
		}
	}
	return bins.at(maxi).at(0);
}



//////////////////////////////
//
// getVpos == return the position on the staff given the diatonic pitch.
//     and the staff. 1=bass, 2=treble.
//     3 = bottom line of clef, 0 = space below first ledger line.
//

double getVpos(double base7) {
	if (base7 < 4 * 7) {
		// bass clef
		return base7 - (1 + 2*7);  // D2
	} else {
		// treble clef
		return base7 - (6 + 3*7);  // B3
	}
}



//////////////////////////////
//
// getStaffBase7 -- return 1 if less than middle C; otherwise return 2.
//

int getStaffBase7(int base7) {
	if (base7 < 4 * 7) {
		return 1;
	} else {
		return 2;
	}
}


//////////////////////////////
//
// getMaxDiatonicIndex -- return the highest non-zero content.
//

int getMaxDiatonicIndex(vector<vector<double>>& diatonic) {
	for (int i=diatonic.size()-1; i>=0; i--) {
		if (diatonic.at(i).at(0) != 0.0) {
			return i;
		}
	}
	return -1;
}



//////////////////////////////
//
// getMinDiatonicIndex -- return the lowest non-zero content.
//

int getMinDiatonicIndex(vector<vector<double>>& diatonic) {
	for (int i=0; i<(int)diatonic.size(); i++) {
		if (diatonic.at(i).at(0) != 0.0) {
			return i;
		}
	}
	return -1;
}



//////////////////////////////
//
// getMinDiatonicAcc -- return the lowest accidental.
//

int getMinDiatonicAcc(vector<vector<double>>& diatonic, int index) {
	for (int i=1; i<(int)diatonic.at(index).size(); i++) {
		if (diatonic.at(index).at(i) != 0.0) {
			return i;
		}
	}
	return -1;
}



//////////////////////////////
//
// getMaxDiatonicAcc -- return the highest accidental.
//

int getMaxDiatonicAcc(vector<vector<double>>& diatonic, int index) {
	for (int i=(int)diatonic.at(index).size() - 1; i>0; i--) {
		if (diatonic.at(index).at(i) != 0.0) {
			return i;
		}
	}
	return -1;
}



//////////////////////////////
//
// getVoice --
//

void getVoice(string& voicestring, HumdrumFile& infile, int kernspine) {
	int i,j;
	voicestring = "";
	PerlRegularExpression pre;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isInterpretation()) {
			continue;
		}
		for (j=0; j<infile[i].getFieldCount(); j++) {
			if (kernspine != infile[i].getPrimaryTrack(j)) {
				continue;
			}
			if (strncmp(infile[i][j], "*I\"", strlen("*I\"")) == 0) {
				voicestring =  &(infile[i][j][3]);
				return;
			}
		}
	}

	// don't print editorial marks for voice names
	if (strchr(voicestring.c_str(), '[') == NULL) {
		return;
	}

	pre.sar(voicestring, "[\\[\\]]", "", "g");
}



//////////////////////////////
//
// getTitle --
//

void getTitle(string& titlestring, HumdrumFile& infile) {
	titlestring = "_00";

	char buffer[1024] = {0};
	infile.getBibValue(buffer, "SCT");
	if (strlen(buffer) > 0) {
		titlestring += buffer;
		titlestring += " ";
		infile.getBibValue(buffer, "OPR");
		if (strlen(buffer) > 0) {
			titlestring += buffer;
			titlestring += " _02";
		}
		infile.getBibValue(buffer, "OTL");
		if (strlen(buffer) > 0) {
			titlestring += buffer;
		}
		
	}
}



//////////////////////////////
//
// clearHistograms --
//

void clearHistograms(vector<vector<double> >& bins, int start) {
	int i;
	for (i=start; i<(int)bins.size(); i++) {
		bins[i].resize(40*11);
		fill(bins[i].begin(), bins[i].end(), 0.0);
		// bins[i].allowGrowth(0);
	}
	for (int i=0; i<(int)bins.size(); i++) {
		if (bins[i].size() == 0) {
			bins[i].resize(40*11);
			fill(bins[i].begin(), bins[i].end(), 0.0);
		}
	}
}




//////////////////////////////
//
// printAnalysis --
//

void printAnalysis(vector<double>& midibins) {
	if (percentileQ) {
		printPercentile(midibins, percentile);
		return;
	}  else if (rangeQ) {
		double notesinrange = countNotesInRange(midibins, rangeL, rangeH);
		cout << notesinrange << endl;
		return;
	}

	int i;
	double normval = 1.0;

	// print the pitch histogram

	double fracL = 0.0;
	double fracH = 0.0;
	double fracA = 0.0;
	double sum = accumulate(midibins.begin(), midibins.end(), 0.0);
	if (normQ) {
		normval = sum;
	}
	double runningtotal = 0.0;


	cout << "**keyno\t";
	if (pitchQ) {
		cout << "**pitch";
	} else {
		cout << "**kern";
	}
	cout << "\t**count";
	if (addfractionQ) {
		cout << "\t**fracL";
		cout << "\t**fracA";
		cout << "\t**fracH";
	}
	cout << "\n";


	int base12;
	char buffer[1024] = {0};

	if (!reverseQ) {
		for (i=0; i<(int)midibins.size(); i++) {
			if (midibins[i] <= 0.0) {
				continue;
			}
			if (diatonicQ) {
				base12 = Convert::base7ToBase12(i);
			} else {
				base12 = i;
			}
			cout << base12 << "\t";
			if (pitchQ) {
				cout << Convert::base12ToPitch(buffer, 1024, base12);
			} else {
				cout << Convert::base12ToKern(buffer, 1024, base12);
			}
			cout << "\t";
			cout << midibins[i] / normval;
			fracL = runningtotal/sum;
			runningtotal += midibins[i];
			fracH = runningtotal/sum;
			fracA = (fracH + fracL)/2.0;
			fracL = (int)(fracL * 10000.0 + 0.5)/10000.0;
			fracH = (int)(fracH * 10000.0 + 0.5)/10000.0;
			fracA = (int)(fracA * 10000.0 + 0.5)/10000.0;
			if (addfractionQ) {
				cout << "\t" << fracL;
				cout << "\t" << fracA;
				cout << "\t" << fracH;
			}
			cout << "\n";
		}
	} else {
		for (i=(int)midibins.size()-1; i>=0; i--) {
			if (midibins[i] <= 0.0) {
				continue;
			}
			if (diatonicQ) {
				base12 = Convert::base7ToBase12(i);
			} else {
				base12 = i;
			}
			cout << base12 << "\t";
			if (pitchQ) {
				cout << Convert::base12ToPitch(buffer, 1024, base12);
			} else {
				cout << Convert::base12ToKern(buffer, 1024, base12);
			}
			cout << "\t";
			cout << midibins[i] / normval;
			fracL = runningtotal/sum;
			runningtotal += midibins[i];
			fracH = runningtotal/sum;
			fracA = (fracH + fracL)/2.0;
			fracL = (int)(fracL * 10000.0 + 0.5)/10000.0;
			fracH = (int)(fracH * 10000.0 + 0.5)/10000.0;
			fracA = (int)(fracA * 10000.0 + 0.5)/10000.0;
			if (addfractionQ) {
				cout << "\t" << fracL;
				cout << "\t" << fracA;
				cout << "\t" << fracH;
			}
			cout << "\n";
		}
	}

	cout << "*-\t*-\t*-";
	if (addfractionQ) {
		cout << "\t*-";
		cout << "\t*-";
		cout << "\t*-";
	}
	cout << "\n";

	cout << "!!tessitura:\t" << getTessitura(midibins) << " semitones\n";

	double mean = getMean12(midibins);
	if (diatonicQ && (mean > 0)) {
		mean = Convert::base7ToBase12(mean);
	}
	cout << "!!mean:\t\t" << mean;
	cout << " (";
	if (mean < 0) {
		cout << "unpitched";
	} else {
		cout << Convert::base12ToKern(buffer, 1024, int(mean+0.5));
	}
	cout << ")" << "\n";

	int median12 = getMedian12(midibins);
	cout << "!!median:\t" << median12;
	cout << " (";
	if (median12 < 0) {
		cout << "unpitched";
	} else {
		cout << Convert::base12ToKern(buffer, 1024, median12);
	}
	cout << ")" << "\n";

}



//////////////////////////////
//
// getMedian12 -- return the pitch on which half of pitches are above
//     and half are below.
//

int getMedian12(vector<double>& midibins) {
	double sum = accumulate(midibins.begin(), midibins.end(), 0.0);

	double cumsum = 0.0;
	int i;
	for (i=0; i<(int)midibins.size(); i++) {
		if (midibins[i] <= 0.0) {
			continue;
		}
		cumsum += midibins[i]/sum;
		if (cumsum >= 0.50) {
			return i;
		}
	}

	return -1000;
}



//////////////////////////////
//
// getMean12 -- return the interval between the highest and lowest
//     pitch in terms if semitones.
//

double getMean12(vector<double>& midibins) {
	double top    = 0.0;
	double bottom = 0.0;

	int i;
	for (i=0; i<(int)midibins.size(); i++) {
		if (midibins[i] <= 0.0) {
			continue;
		}
		top += midibins[i] * i;
		bottom += midibins[i];
	
	}

	if (bottom == 0) {
		return -1000;
	}
	return top / bottom;
}



//////////////////////////////
//
// getTessitura -- return the interval between the highest and lowest
//     pitch in terms if semitones.
//

int getTessitura(vector<double>& midibins) {
	int minn = -1000;
	int maxx = -1000;
	int i;

	for (i=0; i<(int)midibins.size(); i++) {
		if (midibins[i] <= 0.0) {
			continue;
		}
		if (minn < 0) {
			minn = i;
		}
		if (maxx < 0) {
			maxx = i;
		}
		if (minn > i) {
			minn = i;
		}
		if (maxx < i) {
			maxx = i;
		}
	}
	if (diatonicQ) {
		maxx = Convert::base7ToBase12(maxx);
		minn = Convert::base7ToBase12(minn);
	}

	return maxx - minn + 1;
}



//////////////////////////////
//
// countNotesInRange --
//

double countNotesInRange(vector<double>& midibins, int low, int high) {
	int i;
	double sum = 0;
	for (i=low; i<=high; i++) {
		sum += midibins[i];
	}
	return sum;
}



//////////////////////////////
//
// printPercentile --
//

void printPercentile(vector<double>& midibins, double percentile) {
	double sum = accumulate(midibins.begin(), midibins.end(), 0.0);
	double runningtotal = 0.0;
	int i;
	for (i=0; i<(int)midibins.size(); i++) {
		if (midibins[i] <= 0) {
			continue;
		}
		runningtotal += midibins[i] / sum;
		if (runningtotal >= percentile) {
			cout << i << endl;
			return;
		}
	}

	cout << "unknown" << endl;
}



//////////////////////////////
//
// processOptions -- validate and process command-line options.
//

void processOptions(Options& opts, int argc, char* argv[]) {
	opts.define("a|all=b", "generate all-voice analysis");
	opts.define("c|range|count=s:60-71", "count notes in a particular range");
	opts.define("r|reverse=b", "Reverse list of notes in analysis from high to low");
	opts.define("d|duration=b",      "weight pitches by duration");
	opts.define("f|fraction=b",      "display histogram fractions");
	opts.define("fill=b",            "change color of fill only");
	opts.define("p|percentile=d:0.0","display the xth percentile pitch");
	opts.define("print=b",           "count printed notes rather than sounding");
	opts.define("pitch=b",           "display pitch info in **pitch format");
	opts.define("N|norm=b",          "normalize pitch counts");
	opts.define("score=b",           "convert range info to SCORE");
	opts.define("title=s:",          "Title for SCORE display");
	opts.define("T|no-title=b",      "Do not display a title");
	opts.define("q|quartile=b",      "display quartile notes");
	opts.define("i|instrument=b",    "categorize multiple inputs by instrument");
	opts.define("sx|scorexml|score-xml|ScoreXML|scoreXML=b",
					                     "output ScoreXML format");
	opts.define("hover=b",           "include svg hover capabilities");
	opts.define("no-key=b",          "do not display key signature");
	opts.define("finalis|final|last=b", "include finalis note by voice");
	opts.define("l|local|local-maximum|local-maxima=b",  "use maximum values by voice rather than all voices");
	opts.define("jrp=b",             "set options for JRP style");
	opts.define("acc|color-accidentals=b", "add color to accidentals in histogram");
	opts.define("D|diatonic=b",
			"diatonic counts ignore chormatic alteration");
	opts.define("no-define=b", "Do not use defines in output SCORE data");

	opts.define("debug=b",       "trace input parsing");
	opts.define("author=b",      "author of the program");
	opts.define("version=b",     "compilation information");
	opts.define("example=b",     "example usage");
	opts.define("h|help=b",      "short description");
	opts.process(argc, argv);
	
	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
			  << "craig@ccrma.stanford.edu, Mar 2005" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 31 Mar 2005" << endl;
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

	scoreQ       = opts.getBoolean("score");
	fillonlyQ    = opts.getBoolean("fill");
	quartileQ    = opts.getBoolean("quartile");
	debugQ       = opts.getBoolean("debug");
	normQ        = opts.getBoolean("norm");
	printQ       = opts.getBoolean("print");
	pitchQ       = opts.getBoolean("pitch");
	durationQ    = opts.getBoolean("duration");
	allQ         = opts.getBoolean("all");
	reverseQ     = opts.getBoolean("reverse");
	percentileQ  = opts.getBoolean("percentile");
	rangeQ       = opts.getBoolean("range");
	localQ       = opts.getBoolean("local-maximum");
	getRange(rangeL, rangeH, opts.getString("range").c_str());
	addfractionQ = opts.getBoolean("fraction");
	percentile   = opts.getDouble("percentile");
	hoverQ       = opts.getBoolean("hover");
	keyQ         = !opts.getBoolean("no-key");
	finalisQ     = opts.getBoolean("finalis");
	accQ         = opts.getBoolean("acc");
	diatonicQ    = opts.getBoolean("diatonic");
	instrumentQ  = opts.getBoolean("instrument");
	notitleQ     = opts.getBoolean("no-title");
	titleQ       = opts.getBoolean("title");
	Title        = opts.getString("title");

	if (opts.getBoolean("jrp")) {
		// default style settings for JRP range displays:
		scoreQ   = true;
		allQ     = true;
		hoverQ   = true;
		accQ     = true;
		finalisQ = true;
		notitleQ = true;
	}

	// the percentile is a fraction from 0.0 to 1.0.
	// if the percentile is above 1.0, then it is assumed
	// to be a percentage, in which case the value will be
	// divided by 100 to get it in the range from 0 to 1.
	if (percentile > 1) {
		percentile = percentile / 100.0;
	}

}



//////////////////////////////
//
// getRange --
//

void getRange(int& rangeL, int& rangeH, const char* rangestring) {
	rangeL = -1; rangeH = -1;
	if (rangestring == NULL) {
		return;
	}
	if (rangestring[0] == '\0') {
		return;
	}
	int length = strlen(rangestring);
	char* buffer = new char[length+1];
	strcpy(buffer, rangestring);
	char* ptr;
	if (std::isdigit(buffer[0])) {
		ptr = strtok(buffer, " \t\n:-");
		sscanf(ptr, "%d", &rangeL);
		ptr = strtok(NULL, " \t\n:-");
		if (ptr != NULL) {
			sscanf(ptr, "%d", &rangeH);
		}
	} else {
		ptr = strtok(buffer, " :");
		if (ptr != NULL) {
			rangeL = Convert::kernToMidiNoteNumber(ptr);
			ptr = strtok(NULL, " :");
			if (ptr != NULL) {
				rangeH = Convert::kernToMidiNoteNumber(ptr);
			}
		}
	}

	if (rangeH < 0) {
		rangeH = rangeL;
	}

	if (rangeL <   0) { rangeL =   0; }
	if (rangeH <   0) { rangeH =   0; }
	if (rangeL > 127) { rangeL = 127; }
	if (rangeH > 127) { rangeH = 127; }
	if (rangeL > rangeH) {
		int temp = rangeL;
		rangeL = rangeH;
		rangeH = temp;
	}

}



//////////////////////////////
//
// example -- example usage of the maxent program
//

void example(void) {
	cout <<
	"                                                                        \n"
	<< endl;
}




//////////////////////////////
//
// usage -- gives the usage statement for the quality program
//

void usage(const char* command) {
	cout <<
	"                                                                        \n"
	<< endl;
}



