//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Oct 27 10:42:08 PST 2008
// Last Modified: Fri Jun 26 11:23:35 PDT 2009 converted from original version
// Last Modified: Thu Jul  2 15:59:04 PDT 2009 intial PCRE implementation
// Last Modified: Wed Aug 25 15:55:23 PDT 2010 made -t input case insensitive
// Last Modified: Thu Aug 26 14:32:01 PDT 2010 add cleaning for pitch queries
// Last Modified: Wed Sep  1 15:43:34 PDT 2010 added metric position
// Last Modified: Thu Sep  2 17:40:01 PDT 2010 added feature linking
// Last Modified: Fri Sep  3 13:31:22 PDT 2010 added --count feature
// Last Modified: Wed Sep  8 19:24:08 PDT 2010 added --limit
// Last Modified: Mon Nov 22 09:24:00 PST 2010 added -B
// Last Modified: Wed Nov 24 17:59:20 PST 2010 fixed -p/--location interaction
// Last Modified: Sun Nov 28 13:03:28 PST 2010 added -f option
// Last Modified: Tue Jan 11 19:32:58 PST 2011 added --location2 option
// Last Modified: Mon Jan 17 04:12:01 PST 2011 switched --loc and --loc2
// Last Midified: Tue Jan 18 08:16:26 PST 2011 added --overlap option
// Last Midified: Sat Apr  2 18:01:12 PDT 2011 added L=long B=breve durations
// Last Midified: Mon Nov  7 10:40:00 PST 2011 added + == # for pitch search
// Last Midified: Mon Nov 12 17:09:30 PST 2012 added note offsets
// Last Midified: Thu Nov 14 02:31:24 WET 2019 convert to STL
// Filename:      ...museinfo/examples/all/themax.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/themax.cpp
// Syntax:        C++; museinfo
//
// Description:   searches an index created by tindex.  Themax
//                is a C++ implementation of the original thema command
//                which was written by David Huron in 1996/1998, and
//                modified with a few bug fixes during the implementation
//                of Themefinder.org by Craig Sapp (1999-2001).
//
// Classical themebuilder (AWK version of tindex) entry order:
//    fileid [Zz] { # : % } j J M
// Additional rhythmic marks:
//    ~ ^ ! & @ ` '
//
// Meaning of the tracer symbols:
//  [Zz] = major/minor key
//  {    = 12-tone interval
//  #    = pitch refined contour
//  :    = pitch gross contour
//  %    = scale degree
//  }    = musical interval
//  j    = 12-tone pitch
//  J    = absolute pitch
//  M    = metric description
// Added rhythmic markers:
//  ~    = duration gross contour
//  ^    = duration refined contour
//  ;    = duration (IOI)
//  &    = beat level
//  @    = metric gross contour
//  `    = metric refined contour
//  '    = metric level
//
// Options:
// -a   anchor to start          (implemented)
// -M   major keys               (implemented)
// -m   minor keys               (implemented)
// -t   tonic                    (implemented)
// -T   meter                    (implemented)
//
// PITCH FEATURES:
//
// -i { 12-tone interval         (implemented)
// -c # refined contour          (implemented)
// -C : gross contour            (implemented)
// -d % scale degree             (implemented)
// -I } musical interval         (implemented)
// -P j 12-tone pitch class      (implemented)
// -p J pitch-class name         (implemented)
//   -D diatonic pitch-class name(implemented)
//
// RHYTHM FEATURES:
//
// -u ; duration (IOI)           (implemented)
// -R ~ duration gross contour   (implemented)
// -r ^ duration refined contour (implemented)
// -b & beat level               (implemented)
// -l = metric position          (implemented)
// -E @ metric gross contour     (implemented)
// -e ` metric refined contour   (implemented)
// -L ' metric level             (implemented)
//
// Todo: Add --repeat option which allows for any repeated notes
// between pitch features.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;


// character markers in index file (as used in tindex)
#define P_PITCH_CLASS_MARKER              'J'
#define P_DIATONIC_INTERVAL_MARKER        '}'
#define P_SCALE_DEGREE_MARKER             '%'
#define P_12TONE_INTERVAL_MARKER          '{'
#define P_REFINED_CONTOUR_MARKER          '#'
#define P_GROSS_CONTOUR_MARKER            ':'
#define P_12TONE_PITCH_CLASS_MARKER       'j'
#define R_DURATION_GROSS_CONTOUR_MARKER   '~'
#define R_DURATION_REFINED_CONTOUR_MARKER '^'
#define R_DURATION_MARKER                 ';'
#define R_BEAT_LEVEL_MARKER               '&'
#define R_METRIC_POSITION_MARKER          '='
#define R_METRIC_LEVEL_MARKER             '\''
#define R_METRIC_GROSS_CONTOUR_MARKER     '@'
#define R_METRIC_REFINED_CONTOUR_MARKER   '`'


// function declarations:
void      checkOptions           (Options& opts, int argc, char** argv);
void      example                (void);
void      usage                  (const string& command);
void      appendToSearchString   (string& ss, const string& string,
                                  char marker, int anchor);
void      showCleanedParameters  (void);
int       searchForMatches       (const string& filename, string& ss,
                                  PerlRegularExpression& re, int mcount);
int       searchForMatches       (istream& inputfile, string& ss,
                                  PerlRegularExpression& re, int mcount);
void      prepareInterval        (string& data);
int       checkLink              (string& line, int offset);
void      getSimpleLocationINT   (vector<int>& positions, string& line,
                                  string& feature, char searchanchor,
                                  vector<int>& checklocs, int featurewidth = 1);
void      getSimpleLocationFET   (vector<int>& positions, string& line,
                                  string& feature, char searchanchor,
                                  vector<int>& checklocs, int featurewidth = 1);
void      getSeparatorLocationINT(vector<int>& positions, string& line,
                                  string& feature, char searchanchor,
                                  vector<int>& checklocs, char separator = ' ');
void      getSeparatorLocationFET(vector<int>& positions, string& line,
                                  string& feature, char searchanchor,
                                  vector<int>& checklocs, char separator = ' ');
void      getMusicalIntervalLocation(vector<int>& positions, string& line,
                                  string& feature, char searchanchor,
                                  vector<int>& checklocs);
void      processKernString      (const string& string);
void      removeBoundaryCharacters(string& line);
void      getTargetEnds           (vector<int>& targetend, vector<int>& target,
                                  string& line);
void      adjustForInterleavedQuery(Options& opts);
void      removeOverlappedMatches (vector<int>& target, vector<int>& targetend);

// location2Q functions
void getSeparatorLocationFETEnd(vector<int>& positions, string& line,
	string& feature, char searchanchor, vector<int>& startlocs,
	char separator);
int countElementsA(const string& str, int ending, char separator);
void getSimpleLocationINTEnd(vector<int>& positions, string& line,
	string& feature, char searchanchor, vector<int>& checklocs,
	int featurewidth = 1);
int countElementsB(const string& str, int ending, int width);
void getMusicalIntervalLocationEnd(vector<int>& positions, string& line,
	string& feature, char searchanchor, vector<int>& checklocs);
int countElementsC(const string& str, int ending);


// user input sanitation functions:
void      cleanPpitchClass      (string& data);
void      cleanPmusicalInterval (string& data);
void      cleanPscaleDegree     (string& data);
void      cleanPgrossContour    (string& data);
void      cleanPrefinedContour  (string& data);
void      cleanP12tonePitchClass(string& data);
void      cleanP12toneInterval  (string& data);
void      cleanUpRangeSyntaxNoOutsideDigitsOrComma(string& data);
void      cleanRbeatLevel       (string& data);
void      cleanRduration        (string& data);
void      cleanRgrossContour    (string& data);
void      cleanRrefinedContour  (string& data);
void      cleanRmgc             (string& data);
void      cleanRmrc             (string& data);
void      cleanRmetricLevel     (string& data);
void      cleanRmetricPosition  (string& data);


// User interface variables:
Options   options;

int         showcleanQ   = 0;       // used with --cleaned
int         verboseQ     = 0;       // used with --verbose option
int         verbose2Q    = 0;       // used with --verbose2 option
int         cleanQ       = 1;       // used with --no-clean option
int         anchoredQ    = 0;       // used with -a option
int         keyfilterQ   = 0;       // used with -m -M and -t options
int         minorQ       = 0;       // used with -m option
int         majorQ       = 0;       // used with -M option
int         debugQ       = 0;       // used with --debug option
int         regexQ       = 0;       // used with --regex option
int         quietQ       = 0;       // used with -q option
int         boundaryQ    = 1;       // used with -B option
int         shortQ       = 0;       // used with --short option
int         tonicQ       = 0;       // used with -t option
int         diatonicQ    = 0;       // used with -D option
string      tonicstring;            // used with -t option
int         meterQ       = 0;       // used with -T option
string      meterstring  = "";      // used with -T option
string      meterss;
int         totalQ       = 0;       // used with --total option
int         countQ       = 0;       // used with --count option
int         locationQ    = 0;       // used with --locstart option
int         location2Q   = 0;       // used with --loc option
int         printendQ    = 0;       // used with --overlap and --loc options
int         overlapQ     = 0;       // used with --overlap
int         notQ         = 0;       // used with --not option
int         unlinkQ      = 0;       // used with --unlink option
int         featureCount = 0;       // used with --unlink option
int         smartQ       = 0;       // used with --smart option
int         kernQ        = 0;       // used with -k option
string      kernstring   = "";      // used with -k option
int         limitQ       = 0;       // used with --limit option
int         limitval     = 0;       // used with --limit option
string      filetag;                // used with -f option
int         TOTALCOUNT   = 0;       // used for --total option, hack for some problem where count is
                                    //    returning file count instead of match count.

string tonicss; // tonic search string

// classical searches
int P12toneintervalQ              = 0;    // used with -i option
int PgrosscontourQ                = 0;    // used with -c option
int PrefinedcontourQ              = 0;    // used with -C option
int PscaledegreeQ                 = 0;    // used with -d option
int PmusicalintervalQ             = 0;    // used with -I option
int P12tonepitchclassQ            = 0;    // used with -P option
int PpitchclassQ                  = 0;    // used with -p option

string P12toneinterval;              // used with -i option
string Pgrosscontour;                // used with -c option
string Prefinedcontour;              // used with -C option
string Pscaledegree;                 // used with -d option
string Pmusicalinterval;             // used with -I option
string P12tonepitchclass;            // used with -P option
string Ppitchclass;                  // used with -p option

// extended rhythm searches
int RgrosscontourQ                = 0;    // used with -R option
int RrefinedcontourQ              = 0;    // used with -r option
int RdurationQ                    = 0;    // used with -u option
int RbeatlevelQ                   = 0;    // used with -b option
int RmetriclevelQ                 = 0;    // used with -L option
int RmetricpositionQ              = 0;    // used with -l option
int RmetricrefinedcontourQ        = 0;    // used with -e option
int RmetricgrosscontourQ          = 0;    // used with -E option

string Rgrosscontour;                // used with -R option
string Rrefinedcontour;              // used with -r option
string Rduration;                    // used with -u option
string Rbeatlevel;                   // used with -b option
string Rmetriclevel;                 // used with -L option
string Rmetricposition;              // used with -l option
string Rmetricrefinedcontour;        // used with -e option
string Rmetricgrosscontour;          // used with -E option


// final search feature strings for use with link checking
string xP12toneinterval;
string xPgrosscontour;
string xPrefinedcontour;
string xPscaledegree;
string xPmusicalinterval;
string xP12tonepitchclass;
string xPpitchclass;
string xRgrosscontour;
string xRrefinedcontour;
string xRduration;
string xRbeatlevel;
string xRmetriclevel;
string xRmetricposition;
string xRmetricrefinedcontour;
string xRmetricgrosscontour;


//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
	checkOptions(options, argc, argv); // process the command-line options

	string searchstring;
	string& ss = searchstring;
	ss.reserve(10000);

	if (filetag.size() > 1) {
		PerlRegularExpression fre;

		// convert dots to literal dots (not regular expression dot)
		fre.sar(filetag, "\\.", "\\.", "g");

		// convert starts to match anything except tabs or colon
		fre.sar(filetag, "\\*", "[^\\t:]*", "g");

		ss += "^[^:\\t]*";
		ss += filetag;
		ss += "[:]?[^\\t]*\\t";
		ss += ".*";
	}

// order of data in index file:
//  [Zz] = major/minor key  //////////////////////////////////////////////////

	tonicss.clear();
	if (keyfilterQ) {
		if (majorQ && !minorQ) {
			tonicss += "Z";
		} else if (minorQ && !majorQ) {
			tonicss += "z";
		} else {
			tonicss += "[Zz]";
		}
		// place the tonic search next if given
		if (tonicQ) {
			tonicss += tonicstring;
		} else {
			tonicss += "[^=]*";
		}
		tonicss += "=";
		// place the end of key information marker:
		ss += tonicss;
		ss += ".*";
	}

	//  {    = 12-tone interval //////////////////////////////////////////////////
	if (P12toneintervalQ) {
		if (cleanQ) { cleanP12toneInterval(P12toneinterval); }
		appendToSearchString(ss, P12toneinterval, P_12TONE_INTERVAL_MARKER, anchoredQ);
	}

	//  #    = pitch refined contour /////////////////////////////////////////////
	if (PrefinedcontourQ) {
		if (cleanQ) { cleanPrefinedContour(Prefinedcontour); }
		appendToSearchString(ss, Prefinedcontour, P_REFINED_CONTOUR_MARKER, anchoredQ);
	}

	//  :    = pitch gross contour ///////////////////////////////////////////////
	if (PgrosscontourQ) {
		if (cleanQ) { cleanPgrossContour(Pgrosscontour); }
		appendToSearchString(ss, Pgrosscontour, P_GROSS_CONTOUR_MARKER, anchoredQ);
	}

	//  %    = scale degree //////////////////////////////////////////////////////
	if (PscaledegreeQ) {
		if (cleanQ) { cleanPscaleDegree(Pscaledegree); }
		appendToSearchString(ss, Pscaledegree, P_SCALE_DEGREE_MARKER, anchoredQ);
	}

	//  }    = musical interval //////////////////////////////////////////////////
	if (PmusicalintervalQ) {
		if (cleanQ) { cleanPmusicalInterval(Pmusicalinterval); }
		appendToSearchString(ss, Pmusicalinterval, P_DIATONIC_INTERVAL_MARKER, anchoredQ);
	}

	//  j    = 12-tone pitch class ///////////////////////////////////////////////
	if (P12tonepitchclassQ) {
		if (cleanQ) { cleanP12tonePitchClass(P12tonepitchclass); }
		appendToSearchString(ss, P12tonepitchclass, P_12TONE_PITCH_CLASS_MARKER, anchoredQ);
	}

	//  J    = pitch class name //////////////////////////////////////////////////
	if (PpitchclassQ) {
		if (cleanQ) { cleanPpitchClass(Ppitchclass); }
		appendToSearchString(ss, Ppitchclass, P_PITCH_CLASS_MARKER, anchoredQ);
	}

	//  M    = metric description ////////////////////////////////////////////////
	meterss.clear();
	if (meterQ) {
		meterss += "M";
		if (!std::isdigit(meterstring[0])) {
			meterss += "[^\\t]*?";
		}
		meterss += meterstring;
		ss += meterss;
		ss += ".*";
	}

	// Added rhythmic markers:

	//  ~    = duration gross contour  ///////////////////////////////////////////
	if (RgrosscontourQ) {
		if (cleanQ) { cleanRgrossContour(Rgrosscontour); }
		appendToSearchString(ss, Rgrosscontour, R_DURATION_GROSS_CONTOUR_MARKER, anchoredQ);
	}

	//  ^    = duration refined contour  /////////////////////////////////////////
	if (RrefinedcontourQ) {
		if (cleanQ) { cleanRrefinedContour(Rrefinedcontour); }
		appendToSearchString(ss, Rrefinedcontour, R_DURATION_REFINED_CONTOUR_MARKER, anchoredQ);
	}

	//  !    = duration (IOI)  ///////////////////////////////////////////////////
	if (RdurationQ) {
		if (cleanQ) { cleanRduration(Rduration); }
		appendToSearchString(ss, Rduration, R_DURATION_MARKER, anchoredQ);
	}

	//  &    = beat level  ///////////////////////////////////////////////////////
	if (RbeatlevelQ) {
		if (cleanQ) { cleanRbeatLevel(Rbeatlevel); }
		appendToSearchString(ss, Rbeatlevel, R_BEAT_LEVEL_MARKER, anchoredQ);
	}

	//  '    = metric level  /////////////////////////////////////////////////////
	if (RmetriclevelQ) {
		if (cleanQ) { cleanRmetricLevel(Rmetriclevel); }
		appendToSearchString(ss, Rmetriclevel, R_METRIC_LEVEL_MARKER, anchoredQ);
	}

	//  `    = metric refined contour  ///////////////////////////////////////////
	if (RmetricrefinedcontourQ) {
		if (cleanQ) { cleanRmrc(Rmetricrefinedcontour); }
		appendToSearchString(ss, Rmetricrefinedcontour, R_METRIC_REFINED_CONTOUR_MARKER, anchoredQ);
	}

	//  @    = metric gross contour  /////////////////////////////////////////////
	if (RmetricgrosscontourQ) {
		if (cleanQ) { cleanRmgc(Rmetricgrosscontour); }
		appendToSearchString(ss, Rmetricgrosscontour, R_METRIC_GROSS_CONTOUR_MARKER, anchoredQ);
	}

	//  =    = metric position ///////////////////////////////////////////////////
	if (RmetricpositionQ) {
		if (cleanQ) { cleanRmetricPosition(Rmetricposition); }
		appendToSearchString(ss, Rmetricposition, R_METRIC_POSITION_MARKER, anchoredQ);
	}


//////////////////////////////////////////////////////////////////////////////


	if ((!quietQ) && (overlapQ) && (locationQ || location2Q)) {
		cout << "#OVERLAP" << endl;
	}


	PerlRegularExpression pre;

	if (pre.search(ss, "\\.\\*$", "")) {
		ss.resize((int)ss.size()-2);
	}

	if (regexQ) {
		cout << ss << endl;
		exit(0);
	}

	if (showcleanQ) {
		showCleanedParameters();
		cout << "Final Regular Expression: " << ss << endl;
		exit(0);
	}

	pre.initializeSearchAndStudy(ss.c_str());
	int totalcount = 0;
	if (options.getArgCount() == 0) {
		// standard input
		totalcount += searchForMatches(cin, ss, pre, totalcount);
	} else {
		for (int i=1; i<=options.getArgCount(); i++) {
			totalcount += searchForMatches(options.getArgument(i), ss, pre, totalcount);
			if (limitQ && (totalcount >= limitval)) {
				break;
			}
		}
	}

	if (totalQ) {
		if (TOTALCOUNT > 0) {
			cout << TOTALCOUNT << endl;
		} else {
			cout << totalcount << endl;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// searchForMatches --
//

int searchForMatches(const string& filename, string& ss,
		PerlRegularExpression& pre, int mcount) {

	ifstream inputfile;
	inputfile.open(filename);
	if (!inputfile.is_open()) {
		return 0;
	}

	int count = searchForMatches(inputfile, ss, pre, mcount);
	inputfile.close();
	return count;
}



//////////////////////////////
//
// searchForMatches -- Should be merged with above function.
//

int searchForMatches(istream& inputfile, string& ss,
		PerlRegularExpression& pre, int mcount) {

	PerlRegularExpression blanktest;
	PerlRegularExpression messagetest;
	PerlRegularExpression noteoffsettest;
	noteoffsettest.initializeSearchAndStudy("[^\\t]+;(\\d+)\\t");
	string line;
	int offset = 1;
	int state;
	int i;
	int counter = 0;
	while (!inputfile.eof()) {
		getline(inputfile, line);
		if (!boundaryQ) {
			removeBoundaryCharacters(line);
		}
		if (blanktest.search(line, "^\\s*$", "")) {
			continue;
		}
		if (line[0] == '#') {
			if (!quietQ) {
				// Echo control messages in the index file.
				cout << line << "\n";
			}
			continue;
		}
		state = pre.search(line);
		if (!state) {
			continue;
		}

		if (state && (!unlinkQ) && (!anchoredQ) && (featureCount > 1)) {
		   if (noteoffsettest.search(line)) {
			   offset = atoi(noteoffsettest.getSubmatch(1));
		   } else {
			   offset = 1;
		   }
			state = checkLink(line, offset);
		} else if (state && (countQ || locationQ || location2Q)) {
		   if (noteoffsettest.search(line)) {
			   offset = atoi(noteoffsettest.getSubmatch(1));
		   } else {
			   offset = 1;
		   }
			counter += checkLink(line, offset);
			mcount++;
			continue;
		}
		if ((state && !notQ) || (notQ && !state)) {
			counter++;
			mcount++;
			if (verboseQ) {
				cout << "Matches in <STDIN>" << endl;
			}
			if (totalQ) {
				continue;
			}
			if (shortQ && (!countQ && !locationQ && !location2Q)) {
				i = 0;
				while ((i < (int)line.size()) && (line[i] != '\t')) {
					cout << line[i];
					i++;
				}
				cout << "\n";
			} else if (!countQ && !locationQ && !location2Q) {
				cout << line << "\n";
			}
		}
		if (limitQ && (mcount >= limitval)) {
			break;
		}
	}
	return counter;
}



//////////////////////////////
//
// removeBoundaryCharcters -- Remove "R", "r", "R ", or "r " after
//    the first tab character in the string.
//

void removeBoundaryCharacters(string& line) {
	auto tabind = line.find('\t');
	if (tabind == std::string::npos) {
		// no tab character found
		return;
	}

	PerlRegularExpression pre;
	if (!pre.search(line, "R ?", "gi")) {
		// no boundary markers to remove
		return;
	}

	string pretab = line.substr(0, tabind);
	string posttab = line.substr(tabind+1);
	pre.sar(posttab, "R ?", "", "gi");

	line = pretab;
	line += posttab;
}



//////////////////////////////
//
// checkLink -- make sure that every search feature starts at the same
//    note number in the data.  Returns the number of matches found
//    on the input line.
//

int checkLink(string& line, int offset) {
	vector<int> target;

	vector<int> temptarget;

	if (P12toneintervalQ) {
		getSimpleLocationINT(temptarget, line, P12toneinterval,
				P_12TONE_INTERVAL_MARKER, target, 2);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (PgrosscontourQ) {
		getSimpleLocationINT(temptarget, line, Pgrosscontour,
				P_GROSS_CONTOUR_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (PrefinedcontourQ) {
		getSimpleLocationINT(temptarget, line, Prefinedcontour,
				P_REFINED_CONTOUR_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (PscaledegreeQ) {
		getSimpleLocationFET(temptarget, line, Pscaledegree,
				P_SCALE_DEGREE_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (PmusicalintervalQ) {
		getMusicalIntervalLocation(temptarget, line, Pmusicalinterval,
				P_DIATONIC_INTERVAL_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (P12tonepitchclassQ) {
		getSimpleLocationFET(temptarget, line, P12tonepitchclass,
				P_12TONE_PITCH_CLASS_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (PpitchclassQ) {
		getSeparatorLocationFET(temptarget, line, Ppitchclass,
				P_PITCH_CLASS_MARKER, target, ' ');
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RgrosscontourQ) {
		getSimpleLocationINT(temptarget, line, Rgrosscontour,
				R_DURATION_GROSS_CONTOUR_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RrefinedcontourQ) {
		getSimpleLocationINT(temptarget, line, Rrefinedcontour,
				R_DURATION_REFINED_CONTOUR_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RdurationQ) {
		getSeparatorLocationFET(temptarget, line, Rduration,
				R_DURATION_MARKER, target, ' ');
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RbeatlevelQ) {
		getSimpleLocationFET(temptarget, line, Rbeatlevel,
				R_BEAT_LEVEL_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RmetriclevelQ) {
		getSeparatorLocationFET(temptarget, line, Rmetriclevel,
				R_METRIC_LEVEL_MARKER, target, ' ');
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RmetricpositionQ) {
		getSeparatorLocationFET(temptarget, line, Rmetricposition,
				R_METRIC_POSITION_MARKER, target, ' ');
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RmetricrefinedcontourQ) {
		getSimpleLocationINT(temptarget, line, Rmetricrefinedcontour,
				R_METRIC_REFINED_CONTOUR_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	if (RmetricgrosscontourQ) {
		getSimpleLocationINT(temptarget, line, Rmetricgrosscontour,
				R_METRIC_GROSS_CONTOUR_MARKER, target);
		if (temptarget.empty()) {
			return 0;
		} else {
			target = temptarget;
		}
	}

	// for use with --location
	vector<int> targetend;
	if (location2Q) {
		getTargetEnds(targetend, target, line);
	} else {
		// not necessary, but doing to anyway
		targetend.resize(target.size());
		fill(targetend.begin(), targetend.end(), -1);
	}

	if (!overlapQ) {
		removeOverlappedMatches(target, targetend);
	}

	int offsettoggle = 0;
	int coloncount = 0;
	if ((location2Q || locationQ || countQ) && !target.empty()) {
		int i = 0;
		while ((i < (int)line.size()) && (line[i] != '\t')) {
			if (line[i] == ':') {
				coloncount++;
			}
			if ((coloncount >= 2) && (line[i] == ';')) {
				offsettoggle = 1;
			}
			if (offsettoggle == 0) {
				cout << line[i];
			}
			i++;
		}
		if (countQ) {
			cout << '\t' << target.size();
			TOTALCOUNT += (int)target.size();
		} else {
			// locationQ
			cout << '\t';
			for (i=0; i<(int)target.size(); i++)  {
				cout << target[i] + offset;
				if (printendQ && (targetend[i] != target[i])) {
					cout << "-" << targetend[i] + offset;
				}
				if (i < (int)target.size() -1) {
					cout << " ";
				}
			}
		}
		cout << '\n';
	}

	// all position filters returned success
	return (int)target.size();
}


//////////////////////////////
//
// removeOverlappedMatches --  Input is presumed to be sorted by
//    the target array.
//

void removeOverlappedMatches(vector<int>& target, vector<int>& targetend) {
	vector<int> tstart;
	vector<int> tend;
	tstart.reserve(target.size());
	tend.reserve(targetend.size());

	int i;
	int emark = -1;
	for (i=0; i<(int)target.size(); i++) {
		if (target[i] > emark) {
			tstart.push_back(target[i]);
			tend.push_back(targetend[i]);
			if (targetend[i] > emark) {
				emark = targetend[i];
			}
		}
	}

	if (target.size() == tstart.size()) {
		// no overlaps, so just return
		return;
	}

	// copied the unoverlapped matches
	target = tstart;
	targetend = tend;
}



//////////////////////////////
//
// getTargetEnds -- return the ending point of a match which is the
//    highest note number from all searches which were done.
//

void getTargetEnds(vector<int>& targetend, vector<int>& target, string& line) {
	targetend.resize(target.size());
	fill(targetend.begin(), targetend.end(), -1);

	vector<int> temptarget(targetend.size(), -1);

	if (PgrosscontourQ) {
		getSimpleLocationINTEnd(temptarget, line, Pgrosscontour,
				P_GROSS_CONTOUR_MARKER, target, 1);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

	if (PrefinedcontourQ) {
		getSimpleLocationINTEnd(temptarget, line, Prefinedcontour,
				P_REFINED_CONTOUR_MARKER, target, 1);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

	if (PscaledegreeQ) {
		// getSimpleLocationFETEnd -> INTEnd for now.
		// check interaction with segmentation markers later...
		getSimpleLocationINTEnd(temptarget, line, Pscaledegree,
				P_SCALE_DEGREE_MARKER, target, 1);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (PmusicalintervalQ) {
		getMusicalIntervalLocationEnd(temptarget, line, Pmusicalinterval,
				P_DIATONIC_INTERVAL_MARKER, target);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

	if (P12tonepitchclassQ) {
		// getSimpleLocationFETEnd -> INTEnd for now.
		// check interaction with segmentation markers later...
		getSimpleLocationINTEnd(temptarget, line, P12tonepitchclass,
				P_12TONE_PITCH_CLASS_MARKER, target, 1);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (P12toneintervalQ) {
		getSeparatorLocationFETEnd(temptarget, line, P12toneinterval,
				P_12TONE_INTERVAL_MARKER, target, ' ');
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (PpitchclassQ) {
		getSeparatorLocationFETEnd(temptarget, line, Ppitchclass,
				P_PITCH_CLASS_MARKER, target, ' ');
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (RgrosscontourQ) {
		getSimpleLocationINTEnd(temptarget, line, Rgrosscontour,
				R_DURATION_GROSS_CONTOUR_MARKER, target);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

	if (RrefinedcontourQ) {
		getSimpleLocationINTEnd(temptarget, line, Rrefinedcontour,
				R_DURATION_REFINED_CONTOUR_MARKER, target);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

	if (RdurationQ) {
		getSeparatorLocationFETEnd(temptarget, line, Rduration,
				R_DURATION_MARKER, target, ' ');
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (RbeatlevelQ) {
		// getSimpleLocationFETEnd -> INTEnd for now.
		// check interaction with segmentation markers later...
		getSimpleLocationINTEnd(temptarget, line, Rbeatlevel,
				R_BEAT_LEVEL_MARKER, target);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (RmetriclevelQ) {
		getSeparatorLocationFETEnd(temptarget, line, Rmetriclevel,
				R_METRIC_LEVEL_MARKER, target, ' ');
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (RmetricpositionQ) {
		getSeparatorLocationFETEnd(temptarget, line, Rmetricposition,
				R_METRIC_POSITION_MARKER, target, ' ');
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i] > targetend[i]) {
				targetend[i] = temptarget[i];
			}
		}
	}

	if (RmetricrefinedcontourQ) {
		getSimpleLocationINTEnd(temptarget, line, Rmetricrefinedcontour,
				R_METRIC_REFINED_CONTOUR_MARKER, target);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

	if (RmetricgrosscontourQ) {
		getSimpleLocationINTEnd(temptarget, line, Rmetricgrosscontour,
				R_METRIC_GROSS_CONTOUR_MARKER, target);
		for (int i=0; i<(int)temptarget.size(); i++) {
			if (temptarget[i]+1 > targetend[i]) {
				targetend[i] = temptarget[i]+1;    // +1 for interval to note
			}
		}
	}

}



//////////////////////////////
//
// getMusicalIntervalLocationEnd --
//

void getMusicalIntervalLocationEnd(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& startlocs) {

	PerlRegularExpression pre;
	string startmarker;
	startmarker += "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);
	int i;

	string newfeature;
	newfeature += "(";
	newfeature += feature;
	newfeature += ")";

	// Check only locations specified by startlocs.
	// The positions in startlocs are presumed to be sorted.

	// This code probably will no longer work if "R" segmentations are
	// present in the data.

	PerlRegularExpression pokey;
	pokey.initializeSearchAndStudy(newfeature.c_str());
	i = startindex;
	int starting;
	int ending;
	int pos;
	int tindex = 0;
	int tsize = (int)startlocs.size();


	// Check only locations specified by startlocs.
	// The positions in startlocs are presumed to be sorted.
	const char* str = line.c_str();
	while ((i < (int)line.size()) && (line[i] != '\t') && (tindex < tsize)) {
		// search for the ith start position, and then extract the end position

		pos = pokey.search(str+i);
		if (pos == 0) {
			break;
		}
		starting = pokey.getSubmatchStart(1);
		ending = pokey.getSubmatchEnd(1);
		positions[tindex] = startlocs[tindex] +
				countElementsC(str+i+starting, ending-starting);
		tindex++;
		i += pokey.getSubmatchStart(1);
		i++;
	}
}



//////////////////////////////
//
// countElementsC --  Count the number of character segments separated
//    by the searchanchor, and stop when the ending point in the string
//    has been found (or excceeded).  Used with
//    getMusicalIntervalLocationEnd().
//

int countElementsC(const string& str, int ending) {
	int output = 0;
	for (int i=0; i<ending; i++) {
		// If a separator character is found, then increment location.
		// str is guarenteed to have valid data in the -1 index location.
		if ((toupper(str[i]) == 'X') ||
			 ((str[i] == 'P') && (toupper(str[i-1]) != 'X'))) {
			output++;
		}
	}

	return output - 1;
}



//////////////////////////////
//
// getSimpleLocationINTEnd -- return a list of the ending
//    note postions for matches which have a separator character
//    between notes.
//

void getSimpleLocationINTEnd(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& startlocs,
		int featurewidth) {

	fill(positions.begin(), positions.end(), -1);

	if (startlocs.empty()) {
		return;
	}

	PerlRegularExpression pre;
	string startmarker = "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);

	string newfeature;
	newfeature += "(";
	newfeature += feature;
	newfeature += ")";

	PerlRegularExpression pokey;
	pokey.initializeSearchAndStudy(newfeature.c_str());
	const char* str = line.c_str();

	int tsize = (int)startlocs.size();
	int i = startindex;
	int pos;    // start index position of match (plus 1)
	int ending;
	int starting;
	int tindex = 0;

	// Check only locations specified by startlocs.
	// The positions in startlocs are presumed to be sorted.
	while ((i < (int)line.size()) && (line[i] != '\t') && (tindex < tsize)) {
		// search for the ith start position, and then extract the end position
		pos = pokey.search(str+i);
		if (pos == 0) {
			break;
		}
		starting = pokey.getSubmatchStart(1);
		ending = pokey.getSubmatchEnd(1);
		positions[tindex] = startlocs[tindex] +
				countElementsB(str+i+starting, ending-starting, featurewidth);
		tindex++;
		i += pokey.getSubmatchStart(1);
		i += featurewidth;
	}
}



//////////////////////////////
//
// countElementsB --  Count the number of character segments separated
//    by the searchanchor, and stop when the ending point in the string
//    has been found (or excceeded).  Used with getSimpleLocationINTEnd().

int countElementsB(const string& str, int ending, int width) {
	int output = 0;
	for (int i=0; i<ending; i+=width) {
		output++;
		if (str[i] == '\t') {
			break;
		} else if (str[i] == '\0') {
			cerr << "GOT TO A STRANGE LOCATION IN countElementsA\n";
			break;
		}
	}
	return output - 1;
}



//////////////////////////////
//
// getSeparatorLocationFETEnd -- return a list of the ending
//    note postions for matches which have a separator character
//    between notes.
//

void getSeparatorLocationFETEnd(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& startlocs,
		char separator) {

	fill(positions.begin(), positions.end(), -1);

	if (startlocs.empty()) {
		return;
	}

	PerlRegularExpression pre;
	string startmarker = "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);

	string newfeature;
	newfeature += "(";
	newfeature += feature;
	newfeature += ")";

	// Check only locations specified by startlocs.
	// The positions in startlocs are presumed to be sorted.
	PerlRegularExpression pokey;
	pokey.initializeSearchAndStudy(newfeature.c_str());
	const char* str = line.c_str();
	int i = startindex;
	int pos;    // start index position of match (plus 1)
	int ending;
	int starting;
	int tindex = 0;
	int tsize = (int)startlocs.size();
	while ((i < (int)line.size()) && (line[i] != '\t') && (tindex < tsize)) {
		// search for the ith start position, and then extract the end position
		pos = pokey.search(str+i);
		if (pos == 0) {
			break;
		}
		starting = pokey.getSubmatchStart(1);
		ending = pokey.getSubmatchEnd(1);
		positions[tindex] = startlocs[tindex] +
				countElementsA(str+i+starting, ending-starting, separator);
		tindex++;
		i += pokey.getSubmatchStart(1);
		i++;
	}
}



//////////////////////////////
//
// countElementsA --  Count the number of character segments separated
//    by the searchanchor, and stop when the ending point in the string
//    has been found (or excceeded).  Used with getSeparatorLocationFETEnd().
//

int countElementsA(const string& str, int ending, char separator) {
	int output = 0;
	for (int i=0; i<ending; i++) {
		if (str[i] == separator) {
			output++;
		} else if (str[i] == '\t') {
			break;
		} else if (str[i] == '\0') {
			cerr << "GOT TO A STRANGE LOCATION IN countElementsA\n";
			break;
		}
	}
	return output - 1;
}



//////////////////////////////
//
// getMusicalIntervalLocation -- return the element number in the feature
//    list (offset from 1) on the given line in the specified feature,
//    when the features are sepearated by according to musical interval
//    feature format, such as:
//    }Xm2XM2P1xm2Xm2XM2xM2P1xM2P1P1P1P1xm3P1xm3xM3XM2XM2Xm2XP5P1XP4xm2xM2
//       *  * *  *  *  *  * *  * * * * *  * *  *  *  *  *  *  * *  *  *  *
//    (checking for segmentation when a digits is preceded by a non-digit.)
//
//    Do NOT Ignore "R *" markers when counting notes.  Add one for each
//    occurence of a rest.  Example test data to consider:
//
//    -m2     F       1
//    -M2     E       4
//    +M2     D       4
//    -M3     E       1.
//    +M2     C       2
//    -M2     D       1
//            C       1
//    R       R       R   (measure 125 of Jos0802d -- bassus part)
//            F       L
//    R       R       R
//    -m3     F       1
//    -M2     D       2
//    +P4     C       2
//    -m2     F       2.
//    -M3     E       4
//            C       L
//    R       R       R
//    +M2     D       2.
//    +m2     E       4
//    +M2     F       2
//    -P4     G       2
//    +P4     D       2
//    +M2     G       2.
//

void getMusicalIntervalLocation(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& checklocs) {

	positions.clear();
	PerlRegularExpression pre;
	string startmarker = "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);

	// If checklocs is non-zero, then only check the feature
	// at the given locations; otherwise, check every position.
	if (checklocs.empty()) {

		// check every location in feature for a match (allowing overlaps)
		PerlRegularExpression gumby;
		gumby.setAnchor();
		gumby.initializeSearchAndStudy(feature.c_str());

		PerlRegularExpression rest;
		rest.setAnchor();
		rest.initializeSearchAndStudy("(R *)");

		const char* str = line.c_str();
		int location = -1; // need to start at -1 since first spot will increment
		int i = startindex;
		while ((i < (int)line.size()) && (line[i] != '\t')) {
			if (rest.search(str+i)) {
				i += rest.getSubmatchEnd(1) - rest.getSubmatchStart(1);
				// do increment location
				if (location >= 0) {
					// but don't increment if the R marker is at the front
					// of the sequence.
					location++;
				}
				// continue; // maybe this is needed?
			}
			if ((toupper(line[i]) == 'X') ||
				 ((line[i] == 'P') && (i>=1) && (toupper(line[i-1]) != 'X'))) {
				location++;
			}
			if ((i > 1) && (toupper(line[i]) == 'R') && (toupper(line[i-1]) == 'R')) {
				location++;
			}
// cout << "I = " << i << "\t" << line[i] << line[i+1] << line[i+2] << line[i+3]  << " LOCATION = " << location << endl;
			if (gumby.search(str+i)) {
				positions.push_back(location);
			}
			i++;
		}

	} else {
		// Check only locations specified by checklocs.
		// The positions in checklocs are presumed to be sorted.

		// This code probably will no longer work if "R" segmentations are
		// present in the data.

		PerlRegularExpression pokey;
		pokey.setAnchor();
		pokey.initializeSearchAndStudy(feature.c_str());
		const char* str = line.c_str();
		int location = 0;
		int i = startindex;
		int targetindex = 0;
		int tsize = (int)checklocs.size();
		while ((i < (int)line.size()) && (line[i] != '\t') && (targetindex < tsize)) {
			// Increase the target location if it is smaller than
			// the current search location.
			if (checklocs[targetindex] < location) {
				targetindex++;
				continue;
			}
			// If a separator character is found, then increment location.
			if ((toupper(line[i]) == 'X') ||
				 ((line[i] == 'P') && (toupper(line[i-1]) != 'X'))) {
				location++;
			}
			if (location == checklocs[targetindex]) {
				if (pokey.search(str+i)) {
					positions.push_back(location);
					targetindex++;
				}
			}
			i++;
		}
	}

	if (verbose2Q) {
		cout << "ORIGINAL LINE: " << line << endl;
		cout << "\tCOUNT IS " << positions.size() << " FOR FEATURE SEARCH "
			  << feature << endl;
		cout << "\tMATCHES AT: ";
		int j;
		for (j=0; j<(int)positions.size(); j++) {
			cout << positions[j] << " ";
		}
		cout << endl;
	}

}



//////////////////////////////
//
// getSeparatorLocation -- return the element number in the feature
//    list (offset from 0) on the given line in the specified feature,
//    when the features are sepearated by spaces (or whatever the
//    separator character is).
//    Default value: separator = ' '
//
//    INT version counts "R *" markers
//    FET version does not count "R *" markers
//

void getSeparatorLocationINT(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& checklocs,
		char separator) {
	positions.clear();
	PerlRegularExpression pre;
	string startmarker;
	startmarker += "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);

	// If checklocs is non-zero, then only check the feature
	// at the given locations; otherwise, check every position.
	if (checklocs.empty()) {

		// check every location in feature for a match (allowing overlaps)
		PerlRegularExpression gumby;
		gumby.setAnchor();
		gumby.initializeSearchAndStudy(feature.c_str());

		// currently rest separator is either nothing or a space
		PerlRegularExpression rest;
		rest.setAnchor();
		rest.initializeSearchAndStudy("(R *)");

		const char* str = line.c_str();
		int location = 0;
		int i = startindex;
		while ((i < (int)line.size()) && (line[i] != '\t')) {
			if (line[i] == separator) {
				i++;
				location++;
				continue;
			}
			if (rest.search(str+i)) {
				i += rest.getSubmatchEnd(1) - rest.getSubmatchStart(1);
				location++;
				continue;
			}
			if (gumby.search(str+i)) {
				positions.push_back(location);
			}
			i++;
		}
	} else {
		// Check only locations specified by checklocs.
		// The positions in checklocs are presumed to be sorted.
		PerlRegularExpression pokey;
		pokey.setAnchor();
		pokey.initializeSearchAndStudy(feature.c_str());
		const char* str = line.c_str();
		int location = 0;
		int i = startindex;
		int targetindex = 0;
		int tsize = (int)checklocs.size();
		while ((i < (int)line.size()) && (line[i] != '\t') && (targetindex < tsize)) {
			// Increase the target location if it is smaller than
			// the current search location.
			if (checklocs[targetindex] < location) {
				targetindex++;
				continue;
			}
			// If a separator character is found, then increment location.
			if (line[i] == separator) {
				i++;
				location++;
				continue;
			}
			if (location == checklocs[targetindex]) {
				if (pokey.search(str+i)) {
					positions.push_back(location);
					targetindex++;
				}
			}
			i++;
		}
	}

	if (verbose2Q) {
		cout << "ORIGINAL LINE: " << line << endl;
		cout << "\tCOUNT IS " << positions.size() << " FOR FEATURE SEARCH "
			  << feature << endl;
		cout << "\tMATCHES AT: ";
		int j;
		for (j=0; j<(int)positions.size(); j++) {
			cout << positions[j] << " ";
		}
		cout << endl;
	}
}



void getSeparatorLocationFET(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& checklocs,
		char separator) {

	positions.clear();
	positions.reserve(checklocs.size());
	PerlRegularExpression pre;
	string startmarker;
	startmarker.reserve(256);
	startmarker += "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");

	// startindex is the first character in the search feature with
	// the full search index.  Matches will be counted from this
	// position.
	int startindex = pre.getSubmatchEnd(1);
	int j;

	vector<int> temppos(1000);

	// If checklocs is non-zero, then only check the feature
	// at the given locations; otherwise, check every position.
	if (checklocs.empty()) {

		// check every location in feature for a match (allowing overlaps)
		PerlRegularExpression gumby;
		gumby.setAnchor();
		gumby.initializeSearchAndStudy(feature.c_str());

		// currently rest separator is either nothing or a space
		PerlRegularExpression rest;
		rest.setAnchor();
		rest.initializeSearchAndStudy("(R *)");

		const char* str = line.c_str();
		int location = 0;
		int i = startindex;
		while ((i < (int)line.size()) && (line[i] != '\t')) {
			if (line[i] == separator) {
				i++;
				location++;
				continue;
			}
			if (rest.search(str+i)) {
				i += rest.getSubmatchEnd(1) - rest.getSubmatchStart(1);
				continue;
			}
			if (gumby.search(str+i)) {
				positions.push_back(location);
			}
			i++;
		}
	} else {
		// Check only locations specified by checklocs.
		// The positions in checklocs are presumed to be sorted.
		PerlRegularExpression pokey;
		pokey.setAnchor();
		pokey.initializeSearchAndStudy(feature.c_str());
		const char* str = line.c_str();
		int location = 0;
		//int lastpos = 0;
		int i = startindex;
		int targetindex = 0;
		int tsize = (int)checklocs.size();

		// currently rest separator is either nothing or a space
		PerlRegularExpression rest;
		rest.setAnchor();
		rest.initializeSearchAndStudy("(R *)");

		while ((i < (int)line.size()) && (line[i] != '\t') && (targetindex < tsize)) {
			// Increase the target location if it is smaller than
			// the current search location.
			if (checklocs[targetindex] < location) {
				targetindex++;
				continue;
			}
			// If a separator character is found, then increment location.
			if (line[i] == separator) {
				i++;
				location++;
				continue;
			}
			if (rest.search(str+i)) {
				i += rest.getSubmatchEnd(1) - rest.getSubmatchStart(1);
				continue;
			}
			if (pokey.search(str+i)) {
				//for (j=lastpos; j<(int)checklocs.size(); j++) {
				for (j=0; j<(int)checklocs.size(); j++) {
					if (location == checklocs[j]) {
						positions.push_back(location);
						// lastpos = j;
						break;
					}
					if (location < checklocs[j]) {
						// not in checklocs (provided that checklocs is sorted)
						break;
					}
				}
			}
			// The following line was causing a bug [fixed 20120607]
			// targetindex++;
			i++;
		}
	}


	if (verbose2Q) {
		cout << "ORIGINAL LINE: " << line << endl;
		cout << "\tCOUNT IS " << positions.size() << " FOR FEATURE SEARCH "
			  << feature << endl;
		cout << "\tMATCHES AT: ";
		int j;
		for (j=0; j<(int)positions.size(); j++) {
			cout << positions[j] << " ";
		}
		cout << endl;
	}
}



//////////////////////////////
//
// getSimpleLocation -- return the element number in the feature list
//   (offset from 0) on the given line in the specified feature
//   Default value: featurewidth = 1
//
//   INT version: intervallic features: count "R" as one note increment
//   FET version: real features: ignore "R" markers when counting.
//

void getSimpleLocationINT(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& checklocs,
		int featurewidth) {
	positions.clear();
	PerlRegularExpression pre;
	string startmarker = "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);

	// if checklocs is non-zero, then only check the feature
	// at the given locations; otherwise, check every position.
	if (checklocs.empty()) {
		// check every location in feature for a match (allowing overlaps)
		PerlRegularExpression gumby;
		gumby.setAnchor();
		gumby.initializeSearchAndStudy(feature.c_str());

		const char* str = line.c_str();
		int location;
		int charloc = 0;
		int i = startindex;
		while ((i < (int)line.size()) && (line[i] != '\t')) {
			if (gumby.search(str+i)) {
				location = charloc / featurewidth;
				positions.push_back(location);
			}
			charloc++;
			i++;
		}
	} else {
		// check only locations specified by checklocs
		// The feature is presumed to be of equal length
		// to other features, and an end of string problem
		// will therefore not be checked.  Only the start
		// of a feature will be checked (no partial
		// start features will be examined, but could be
		// added).
		PerlRegularExpression pokey;
		pokey.setAnchor();
		pokey.initializeSearchAndStudy(feature.c_str());
		const char* str = line.c_str() + startindex;
		for (int i=0; i<(int)checklocs.size(); i++) {
			if (pokey.search(str+(checklocs[i] * featurewidth))) {
				 positions.push_back(checklocs[i]);
			}

		}
	}

	if (verbose2Q) {
		cout << "ORIGINAL LINE: " << line << endl;
		cout << "\tCOUNT IS " << positions.size() << " FOR FEATURE SEARCH "
			  << feature << endl;
		cout << "\tMATCHES AT: ";
		for (int j=0; j<(int)positions.size(); j++) {
			cout << positions[j] << " ";
		}
		cout << endl;
	}
}

void getSimpleLocationFET(vector<int>& positions, string& line,
		string& feature, char searchanchor, vector<int>& checklocs,
		int featurewidth) {
	positions.clear();
	PerlRegularExpression pre;
	string startmarker = "(\\t";
	startmarker += searchanchor;
	startmarker += ")";
	pre.search(line, startmarker, "");
	int startindex = pre.getSubmatchEnd(1);

	// if checklocs is non-zero, then only check the feature
	// at the given locations; otherwise, check every position.
	if (checklocs.empty()) {
		// check every location in feature for a match (allowing overlaps)
		PerlRegularExpression gumby;
		gumby.setAnchor();
		gumby.initializeSearchAndStudy(feature.c_str());

		// currently rest separator is either nothing or a space
		PerlRegularExpression rest;
		rest.setAnchor();
		rest.initializeSearchAndStudy("R");

		const char* str = line.c_str();
		int location;
		int charloc = 0;
		int i = startindex;
		while ((i < (int)line.size()) && (line[i] != '\t')) {
			if (rest.search(str+i)) {
				i++;
				// don't increment charloc due to "R".
				continue;
			}
			if (gumby.search(str+i)) {
				location = charloc / featurewidth;
				positions.push_back(location);
			}
			charloc++;
			i++;
		}
	} else {
		// check only locations specified by checklocs
		// The feature is presumed to be of equal length
		// to other features, and an end of string problem
		// will therefore not be checked.  Only the start
		// of a feature will be checked (no partial
		// start features will be examined, but could be
		// added).

		PerlRegularExpression pokey;
		pokey.setAnchor();
		pokey.initializeSearchAndStudy(feature.c_str());

		// this code will not work with segmentation markers, so have
		// to fix.  Basically cannot speed up the search process, and
		// have to step through data.

		const char* str = line.c_str() + startindex;
		for (int i=0; i<(int)checklocs.size(); i++) {
			if (pokey.search(str+(checklocs[i] * featurewidth))) {
				 positions.push_back(checklocs[i]);
			}
		}
	}

	if (verbose2Q) {
		cout << "ORIGINAL LINE: " << line << endl;
		cout << "\tCOUNT IS " << positions.size() << " FOR FEATURE SEARCH "
			  << feature << endl;
		cout << "\tMATCHES AT: ";
		for (int j=0; j<(int)positions.size(); j++) {
			cout << positions[j] << " ";
		}
		cout << endl;
	}
}



//////////////////////////////
//
// showCleanedParameters --  Show what the post processed input search
//    parameters look like for each search field (if it was used).
//

void showCleanedParameters(void) {

	if (tonicss.size() > 1) {
		cout << "tonic:\t\t" << tonicss << endl;
	}
	if (P12toneinterval.size() > 1) {
		cout << "twelve-tone interval:\t" << P12toneinterval << endl;
	}
	if (Pgrosscontour.size() > 1) {
		cout << "Pitch gross contour:\t" << Pgrosscontour << endl;
	}
	if (Prefinedcontour.size() > 1) {
		cout << "Pitch refined contour:\t" << Prefinedcontour << endl;
	}
	if (Pscaledegree.size() > 1) {
		cout << "Scale-degree:\t" << Pscaledegree << endl;
	}
	if (Pmusicalinterval.size() > 1) {
		cout << "Interval:\t" << Pmusicalinterval << endl;
	}
	if (P12tonepitchclass.size() > 1) {
		cout << "Twelve-tone pitch-class:\t" << P12tonepitchclass
		     << endl;
	}
	if (Ppitchclass.size() > 1) {
		cout << "Pitch-class:\t" << Ppitchclass << endl;
	}
	if (meterss.size() > 1) {
		cout << "meter:\t\t" << meterss << endl;
	}
	if (Rgrosscontour.size() > 1) {
		cout << "Rhythm gross contour:\t" << Rgrosscontour << endl;
	}
	if (Rrefinedcontour.size() > 1) {
		cout << "Rhythm refined contour:\t" << Rrefinedcontour << endl;
	}
	if (Rduration.size() > 1) {
		cout << "Duration:\t" << Rduration << endl;
	}
	if (Rbeatlevel.size() > 1) {
		cout << "Beat level:\t" << Rbeatlevel << endl;
	}
	if (Rmetriclevel.size() > 1) {
		cout << "Metric level:\t" << Rmetriclevel << endl;
	}
	if (Rmetricrefinedcontour.size() > 1) {
		cout << "Metric refined contour:\t" << Rmetricrefinedcontour
			  << endl;
	}
	if (Rmetricgrosscontour.size() > 1) {
		cout << "Metric gross contour:\t" << Rmetricgrosscontour
			  << endl;
	}
	if (Rmetricposition.size() > 1) {
		cout << "Metric position:\t" << Rmetricposition << endl;
	}
}



//////////////////////////////
//
// appendToSearchString --
//

void appendToSearchString(string& ss, const string& astring, char marker, int anchor) {
	// Add back quote for certain markers:
	switch (marker) {
		case '{':
		case '}':
		case '^':
			ss += '\\';
	}
	ss += marker;

	// add [^\t]* if not anchored:
	//
	//
	if (!anchor) {
// The ? causes problems when searching for P1 in -I option:
//      ss +=  "[^\\t]*?";
		ss += "[^\\t]*";
	}
	ss += astring;

	// Moved to cleanPpitchClass:
	// // for pitch-class names, the next character after the search
	// // string must be a space or a tab to prevent accidentals
	// // from matching on natural-note search endings.
	// if (marker == P_PITCH_CLASS_MARKER) {
	//    ss +=  "[ \\t]";
	// }

	ss += ".*";
}



//////////////////////////////
//
// checkOptions --
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	PerlRegularExpression pre;

	opts.define("a|A|anchored=b",     "anchored search at start");
	opts.define("m|minor=b",          "search only minor keys");
	opts.define("M|major=b",          "search only minor keys");
	opts.define("t|tonic=s:",         "search only given tonic");
	opts.define("T|meter=s:",         "search only given meters");

	opts.define("i|12I|12i|pitch-12tone-interval=s:",   "12-tone interval");
	opts.define("C|PGC|pgc|GC|gc|CON|con|pitch-gross-contour=s:",
	                                  "pitch gross contour");
	opts.define("c|PRC|prc|RC|rc|pitch-refined-contour=s:",
	                                  "pitch refined contour");
	opts.define("d|SD|sd|pitch-scale-degree=s:",      "pitch scale degree");
	opts.define("I|MI|mi|DI|di|INT|int|pitch-musical-interval|interval=s:",
	                                  "musical interval");
	opts.define("P|12P|12p|pitch-12tonepc=s:",          "12-tone pitch class");
	opts.define("p|PCH|pch|PC|pc|pitch-class|pitch=s:",
	                                  "pitch class");
	opts.define("D|DPC|dpc|diatonic-pitch-class|diatonic=s:",
	                                  "diatonic pitch class");

	opts.define("R|DGC|dgc|duration-gross-contour=s:",
	                                  "duration gross contour");
	opts.define("r|DRC|drc|duration-refined-contour=s:",
	                                  "duration refined contour");
	opts.define("u|duration=s:",      "duration (IOI)");
	opts.define("b|beat-level=s:",    "beat level");
	opts.define("L|metric-level=s:",  "metric level");
	opts.define("l|metric-position=s:",  "metric position");
	opts.define("e|MRC|mrc|rrc|RRC|metric-refined-contour=s:",
	                                     "metric refined contour");
	opts.define("E|MGC|mgc|RGC|rgc|metric-gross-contour=s:",
	                                     "metric-gross contour");

	//opts.define("i|pitch-twelvetone-interval=s:", "12-tone interval");

	opts.define("q|query=s",          "interleaved search query");
	opts.define("B|no-boundary=b",    "ignore boundary markers");
	opts.define("k|kern=s",           "kern-based search");
	opts.define("f|file=s:",          "filename tag filter");
	opts.define("verbose=b",          "verbose display");
	opts.define("verbose2=b",         "verbose display");
	opts.define("raw|no-clean=b",     "do not preprocess searches");
	opts.define("cleaned|queries|features=b",  "show cleaned input fields");
	opts.define("short|trim=b",       "display only filename in output");
	opts.define("v|V|invert-match|not=b", "negate search query");

	// output formatting options
	opts.define("total=b",            "return a count of the matches");
	opts.define("count=b",            "return count in matched lines");
	opts.define("overlap=b",          "allow overlapped matches");
	opts.define("location|loc|locend=b", "return start/stop note of matchs");
	opts.define("location2|startloc|locstart|start|startining|loc2=b",
	                                  "return only starting note number of match");
	opts.define("limit=i:0",          "exit if limit count of matches exceeded");

	opts.define("debug=b",            "debugging statements");
	opts.define("regex=b",            "print regular expression construct");
	opts.define("unlink=b",           "unlink search features");
	opts.define("smart=b",            "do a smart search");
	opts.define("Q|no-messages=b",    "do not echo control messages from input data");

	opts.define("author=b",           "author of program");
	opts.define("version=b",          "compilation info");
	opts.define("example=b",          "example usages");
	opts.define("h|help=b",           "short description");
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
		     << "craig@ccrma.stanford.edu, Oct 2008" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 24 Nov 2008" << endl;
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

	////////////////////////////////////////////////

	if (opts.getBoolean("query")) {
		adjustForInterleavedQuery(opts);
	}

	quietQ                 =  opts.getBoolean("no-messages");
	shortQ                 =  opts.getBoolean("short");
	verboseQ               =  opts.getBoolean("verbose");
	verbose2Q              =  opts.getBoolean("verbose2");
	cleanQ                 = !opts.getBoolean("no-clean");
	showcleanQ             =  opts.getBoolean("cleaned");
	anchoredQ              =  opts.getBoolean("anchored");
	debugQ                 =  opts.getBoolean("debug");
	regexQ                 =  opts.getBoolean("regex");
	boundaryQ              = !opts.getBoolean("no-boundary");
	smartQ                 =  opts.getBoolean("smart");
	majorQ                 =  opts.getBoolean("major");
	minorQ                 =  opts.getBoolean("minor");
	tonicQ                 =  opts.getBoolean("tonic");
	unlinkQ                =  opts.getBoolean("unlink");
	notQ                   =  opts.getBoolean("not");
	overlapQ               =  opts.getBoolean("overlap");
	tonicstring = opts.getString("tonic");
	pre.sar(tonicstring, "-sharp", "#", "g");
	pre.sar(tonicstring, "-flat", "-", "g");
	pre.tr(tonicstring, "abcdefg", "ABCDEFG");
	totalQ                 = opts.getBoolean("total");
	countQ                 = opts.getBoolean("count");
	locationQ              = opts.getBoolean("location2");
	printendQ              = opts.getBoolean("location");
	overlapQ               = opts.getBoolean("overlap");
	kernQ                  = opts.getBoolean("kern");
	kernstring             = opts.getString("kern");
	limitQ                 = opts.getBoolean("limit");
	limitval               = opts.getInteger("limit");

	keyfilterQ             = majorQ || minorQ || tonicQ;
	meterQ                 =  opts.getBoolean("meter");
	meterstring            =  opts.getString("meter");

	if (opts.getBoolean("file")) {
		filetag = opts.getString("file");
	} else {
		filetag.clear();
	}
	P12toneintervalQ       =  opts.getBoolean("pitch-12tone-interval");
	PgrosscontourQ         =  opts.getBoolean("pitch-gross-contour");
	PrefinedcontourQ       =  opts.getBoolean("pitch-refined-contour");
	PscaledegreeQ          =  opts.getBoolean("pitch-scale-degree");
	PmusicalintervalQ      =  opts.getBoolean("pitch-musical-interval");
	P12tonepitchclassQ     =  opts.getBoolean("pitch-12tonepc");

	PpitchclassQ           =  opts.getBoolean("pitch-class");
	diatonicQ              =  opts.getBoolean("diatonic-pitch-class");
	if (diatonicQ) {
		PpitchclassQ = 1;
	}

	P12toneinterval        = opts.getString("pitch-12tone-interval");
	Pgrosscontour          = opts.getString("pitch-gross-contour");
	Prefinedcontour        = opts.getString("pitch-refined-contour");
	Pscaledegree           = opts.getString("pitch-scale-degree");
	Pmusicalinterval       = opts.getString("pitch-musical-interval");
	P12tonepitchclass      = opts.getString("pitch-12tonepc");

	if (diatonicQ) {
		Ppitchclass = opts.getString("diatonic-pitch-class");
	} else {
		Ppitchclass = opts.getString("pitch-class");
	}

	RgrosscontourQ         = opts.getBoolean("duration-gross-contour");
	RrefinedcontourQ       = opts.getBoolean("duration-refined-contour");
	RdurationQ             = opts.getBoolean("duration");
	RbeatlevelQ            = opts.getBoolean("beat-level");
	RmetriclevelQ          = opts.getBoolean("metric-level");
	RmetricpositionQ       = opts.getBoolean("metric-position");
	RmetricrefinedcontourQ = opts.getBoolean("metric-refined-contour");
	RmetricgrosscontourQ   = opts.getBoolean("metric-gross-contour");

	Rgrosscontour          = opts.getString("duration-gross-contour");
	Rrefinedcontour        = opts.getString("duration-refined-contour");
	Rduration              = opts.getString("duration");
	Rbeatlevel             = opts.getString("beat-level");
	Rmetriclevel           = opts.getString("metric-level");
	Rmetricposition        = opts.getString("metric-position");
	Rmetricrefinedcontour  = opts.getString("metric-refined-contour");
	Rmetricgrosscontour    = opts.getString("metric-gross-contour");

	if (kernQ) {
		processKernString(kernstring);
	}

	// count the number of parallel features being searched
	featureCount = 0;
	if (P12toneintervalQ)       { featureCount++; }
	if (PgrosscontourQ)         { featureCount++; }
	if (PrefinedcontourQ)       { featureCount++; }
	if (PscaledegreeQ)          { featureCount++; }
	if (PmusicalintervalQ)      { featureCount++; }
	if (P12tonepitchclassQ)     { featureCount++; }
	if (PpitchclassQ)           { featureCount++; }
	if (RgrosscontourQ)         { featureCount++; }
	if (RrefinedcontourQ)       { featureCount++; }
	if (RdurationQ)             { featureCount++; }
	if (RbeatlevelQ)            { featureCount++; }
	if (RmetriclevelQ)          { featureCount++; }
	if (RmetricpositionQ)       { featureCount++; }
	if (RmetricrefinedcontourQ) { featureCount++; }
	if (RmetricgrosscontourQ)   { featureCount++; }

	// note that overlapQ is only implemented for --location option
	// and not in other cases (yet).
	if (printendQ && !overlapQ) {
		location2Q = 1;
	}
}



//////////////////////////////
//
// adjustForInterleavedQuery -- parse the --query option for
//    content, and fill in the appropriate serial feature strings.
//

void adjustForInterleavedQuery(Options& opts) {
	if (!opts.getBoolean("query")) {
		return;
	}

	vector<string> tokens;
	PerlRegularExpression pre;
	pre.getTokens(tokens, "\\s+", opts.getString("query"));

	if (tokens.empty()) {
		return;
	}

	vector<string> featuretype;
	pre.getTokens(featuretype, ":", tokens[0]);

	if (featuretype.empty()) {
		return;
	}

	int featurecount = (int)featuretype.size();
	int seqcount = (int)tokens.size()-1;

	stringstream sptr[featurecount];

	vector<string> feature;
	int maxfet = featurecount;
	for (int i=0; i<seqcount; i++) {
		pre.getTokens(feature, ":", tokens[i+1]);
		if ((int)feature.size() != featurecount) {
			break;
		}
		if ((int)feature.size() < maxfet) {
			// shrink feature length if it narrows.
			maxfet = (int)feature.size();
		}
		for (int j=0; j<featurecount; j++) {
			sptr[j] << feature[j] << " ";
		}
	}

	// store the separate feature strings.
	for (int i=0; i<featurecount; i++) {
		opts.setModified(featuretype[i], sptr[i].str());
		if (debugQ) {
			cout << "SETTING " << featuretype[i] << " option to: " <<
					sptr[i].str() << endl;
		}
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

cout <<
"THEMA     : Search thematic database; identify the composer and origin.\n"
"\n"
"     This command searches a thematic database according to user-defined\n"
"     search keys.  Themes that match the search key are identified\n"
"     by composer, title, instrumentation, opus, movement, etc.\n"
"\n"
"     Search keys (\"incipits\") may be defined in several different ways, in-\n"
"     cluding rough up/down contour (-C), up/down contour with step and leap\n"
"     intervals distinguished (-c), by scale degree (-d), by semitone interval\n"
"     (-i), by interval-class (-I), by pitch (-p), by relative rhythm (-r),\n"
"     and by absolute rhythm (R).  Several search keys can be combined in a\n"
"     single search, and searches can be limited to structural tones only (-s).\n"
"\n"
"Syntax:\n"
"\n"
"     thema [-m|M] [-s] [-C incipit] [-c incipit] [-d incipit] [-i incipit]\n"
"      [-I incipit] [-p incipit] [-r incipit] [-R incipit] [-t tonic]\n"
"\n"
"Options:\n"
"\n"
"\n"
"  -C incipit  : identify according to up/down/unison(same) contour [UDS]\n"
"  -c incipit  : identify according to up/down/unison step/leap contour [UDuds]\n"
"  -d incipit  : identify according to scale degree [1234567]\n"
"  -i incipit  : identify according to semitone intervals [+-1234567890]\n"
"  -m          : limit search to themes in minor keys\n"
"  -M          : limit search to themes in major keys\n"
"  -P incipit  : identify according to pitch-class [#-abcdefg]\n"
"  -p incipit  : identify according to absolute pitch [#-ABCDEFGabcdefg]\n"
"  -r incipit  : identify according to longer/shorter/same rhythm [<>=]\n"
"  -R incipit  : identify according to long/short/medium rhythm [LSM]\n"
"  -s          : limit search to structural tones only\n"
"  -t tonic    : limit search to themes in key of \"tonic\" (major or minor)\n"
"\n"
"  Refer to reference manual for further details."
<<endl;

}


///////////////////////////////////////////////////////////////////////////
//
// Cleaning functions -- these functions will filter user input to
//    attempt to make it conformant to the index search
//


///////////////////////////////
//
// cleanRmgc -- metric gross contour
//

void cleanRmgc(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^UuDdRrEeSsHhwW=<>0-9,.{}()+*-]", "", "g");
	pre.tr(data, "udHhWw=Eesr", "UDUUDDSSSSR");
}



///////////////////////////////
//
// cleanRmrc -- metric refined contour
//

void cleanRmrc(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^UuDdRrEeSsHhwW=<>0-9,.{}()+*-]", "", "g");
	pre.tr(data, "HhWwEes=r", "UuDdSSSSR");
}



///////////////////////////////
//
// cleanRgrossContour --
//

void cleanRgrossContour(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^RrSsLl=<>0-9,.{}()+*-]", "", "g");
	pre.tr(data, "rSsLl", "R<<>>");
}



///////////////////////////////
//
// cleanRrefinedContour --
//

void cleanRrefinedContour(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^\\]\\[RrSsLl=<>0-9,.{}()+*-]", "", "g");
	pre.tr(data, "rsl", "R<>");
	pre.sar(data, "\\]", "\\]", "g");
	pre.sar(data, "\\[", "\\[", "g");
	pre.sar(data, "S", "\\[", "g");
	pre.sar(data, "L", "\\]", "g");
}


//////////////////////////////
//
// cleanRbeatLevel --
//

void cleanRbeatLevel(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^Rr0-9,.{}()|*+?BbSs]", "", "g");
	// convert B to 1 and S to 0 if they are used (derived from metric level)
	pre.tr(data, "rBbSs", "R1100");

	// adjust meaning of dot so that it does not match to tab character:
	pre.sar(data, "\\.", "[^\\t]", "g");

}



//////////////////////////////
//
// cleanRDuration --
//    work on allowing more regular expressions.
//

void cleanRduration(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^LlBb?Rr0-9Dd,.{}()|*+\\sxX]", "", "g");

	// change dots into "d", and "x" into "X":
	pre.tr(data, "r.Dxlb", "RddXLB");

	// convert "X" into the equivalent of regular-expression dot:
	pre.sar(data, "X", " \\d+d* ", "g");

	// change spaces to single, adding one at end, removing
	// any from start
	pre.sar(data, "\\s+", " ", "g");
	pre.sar(data, "^\\s+", "", "");
	pre.sar(data, "\\s*$", " ", "");

	// adjust for ? wildcard (more added later...)
	pre.sar(data, " ", " )(?:", "g");
	pre.sar(data, "^", "(?:", "");
	pre.sar(data, "$", ")", "");
	pre.sar(data, "\\? \\)", " )?", "g");
	pre.sar(data, "\\(\\?:\\)", "", "g");  // remove ending null case

}



//////////////////////////////
//
// cleanRmetricLevel --
//

void cleanRmetricLevel(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^Rr0-9,.{}()|*+\\s+-pmPM]", "", "g");

	// change dots into "d":
	pre.tr(data, "r+-PMbs", "RpmpmBS");

	pre.sar(data, "B", " B ", "g");
	pre.sar(data, "S", " S ", "g");

	// change spaces to single, adding one at end, removing
	// any from start
	pre.sar(data, "\\s+", " ", "g");
	pre.sar(data, "^\\s+", "", "");
	pre.sar(data, "\\s*$", " ", "");

	// expand B and S markers
	pre.sar(data, "B", "(?:p\\d+|0)", "g");
	pre.sar(data, "S", "m\\d+", "g");
}



//////////////////////////////
//
// cleanRmetricPosition --
//

void cleanRmetricPosition(string& data) {
	PerlRegularExpression pre;

	// remove invalid characters
	pre.sar(data, "[^Rr0-9,.{}()|*+\\~\\^\\s+_\\- \\/]", "", "g");

	// change dashes into underscores
	pre.tr(data, "r-", "R_");
	// make sure underscores do not have any adjacent spaces:
	pre.sar(data, "\\s*_\\s*", "_", "g");

	// make dots surrounded by spaces:
	pre.sar(data, "\\s*\\.\\s*", " . ", "g");

	// change spaces to single space, adding one at end, removing
	// any from start
	pre.sar(data, "\\s+~\\s+", "~ ", "g");  // remove any spaces before ~
	pre.sar(data, "\\s+\\^\\s+", "~ ", "g");  // remove any spaces before ^
	pre.sar(data, "\\s+", " ", "g");
	pre.sar(data, "^\\s+", "", "");
	pre.sar(data, "\\s*$", " ", "");

	// attach offbeat values to beats
	pre.sar(data, " (?=\\d+\\/\\d+)", "_", "g");

	// expand ~ marker (which means beat and any possible subbeat)
	pre.sar(data, "\\~", "(?:_\\d+\\/\\d+)?", "g");

	// expand ^ marker (which means any possible subbeat of a beat)
	pre.sar(data, "\\^", "_(?:\\d+\\/\\d+)?", "g");

	// add "x" markers to starts of beats
	pre.sar(data, "\\s+", " x", "g");
	pre.sar(data, "x$", "", "");
	pre.sar(data, "^", "x", "");

	// convert dots into form to match any beat position
	pre.sar(data, "\\.", "\\d+(_\\d+\\/\\d+)?", "g");
}



//////////////////////////////
//
// cleanPgrossContour --
//

void cleanPgrossContour(string& data) {
	PerlRegularExpression pre;
	// remove invalid characters
	pre.sar(data, "[^?0-9/\\\\UuDdSsRr*+.{},()|=-]", "", "g");

	// collapse alias chars
	pre.tr(data, "s=-", "SSS");
	pre.tr(data, "/udr\\\\",  "UUDRD");

	// convert "." into [^\t] since tab characters should not match "."
	pre.sar(data, "\\.", "[^\\t]", "g");
}



//////////////////////////////
//
// cleanPrefinedContour --
//

void cleanPrefinedContour(string& data) {
	PerlRegularExpression pre;
	// remove invalid characters
	pre.sar(data, "[^?0-9UuDdSsRr*+.{},()|-]", "", "g");

	// collapse alias chars
	pre.tr(data, "-sr",  "SSR");

	// convert "." into [^\t] since tab characters should not match "."
	pre.sar(data, "\\.", "[^\\t]", "g");
}



//////////////////////////////
//
// cleanPscaleDegree -- This function cleans user input so that it
//   can be used to search the data which is formatted in a particular way.
//

void cleanPscaleDegree(string& data) {
	PerlRegularExpression pre;

	if (pre.search(data, "[a-x]", "i")) {
		// convert pitch names to scale degrees

		// remove all accidentals
		pre.sar(data,  "( |-)?is",    "", "gi"); // replace German sharp
		pre.sar(data,  "( |-)?es",    "", "gi"); // replace German flat

		pre.sar(data,  "( |-)?sharp", "", "gi"); // replace acc names with syms
		pre.sar(data,  "( |-)?flat",  "", "gi"); // replace acc names with syms

		// convert solfege syllables into scale degrees.
		int solfege = 0;                    // true if using solfege syllables
		if (pre.sar(data, "ut|do",  "1", "gi")) {  solfege = 1; }
		if (pre.sar(data, "re",     "2", "gi")) {  solfege = 1; }
		if (pre.sar(data, "mi",     "3", "gi")) {  solfege = 1; }
		if (pre.sar(data, "sol?",   "5", "gi")) {  solfege = 1; }
		if (pre.sar(data, "la",     "6", "gi")) {  solfege = 1; }
		if (pre.sar(data, "[ts]i",  "7", "gi")) {  solfege = 1; }
		// if any other solfege syllables have been used, then convert
		// "fa" into F; otherwise, assume that "fa" means "F A";
		if (solfege) {
			pre.sar(data, "fa", "4", "gi");
		}

		// remove any plain accidentals
		pre.sar(data, "x", "", "gi");
		pre.sar(data, "#", "", "gi");
		pre.sar(data, "-", "", "gi");

		// convert pitches to scale degrees as if they were in C major:
		pre.sar(data, "c", "1", "gi");
		pre.sar(data, "d", "2", "gi");
		pre.sar(data, "e", "3", "gi");
		pre.sar(data, "f", "4", "gi");
		pre.sar(data, "g", "5", "gi");
		pre.sar(data, "a", "6", "gi");
		pre.sar(data, "b", "7", "gi");

	}

	// remove any characters which are not 1-7
	pre.sar(data, "[^Rr1-7@+.]", "", "g");
	pre.tr(data, "r@", "R+");
}



//////////////////////////////
//
// cleanP12toneInterval -- This function cleans user input so that it
//   can be used to search the data which is formatted in a particular way.
//

void cleanP12toneInterval(string& data) {
	PerlRegularExpression pre;

	// place plus in front of any intervals which don't have signs
	pre.sar(data, "\\s(?=\\d)", " p", "g");
	if (pre.search(data, "^\\d", "")) {
		pre.sar(data, "^", "p", "");
	}

	// change plus and minus into "p" and "m"
	pre.tr(data, "PM+\\-r", "pmpmR");

	// change any m0 to p0:
	pre.sar(data, "m0", "p0", "g");

	// remove any disallowed characters
	pre.sar(data, "[^Rr0-9pm~\\[\\]]", "", "g");

	// change tilde sign into [pm] which means either up or down.
	pre.sar(data, "~", "[pm]", "g");
}



//////////////////////////////
//
// cleanP12tonePitchClass -- This function cleans user input so that it
//   can be used to search the data which is formatted in a particular way.
//

void cleanP12tonePitchClass(string& data) {
	PerlRegularExpression pre;

	// change lowercase to upper case
	// Y => A, Z => B.  May use pitch classes in the future so
	// minimizing use of A and B in raw search from user.
	pre.tr(data, "yz", "YZ");

	if (!pre.search(data, "[YZ]", "")) {
		// convert "10" and "11" to "A" and "B" if they are present in query
		pre.sar(data, "\\b10\\b", "Y", "g");
		pre.sar(data, "\\b11\\b", "Z", "g");
	}

	if (pre.search(data, "[a-x]", "i")) {
		// convert pitch names to 12-tone intervals, no triple sharps/flats
		// or higher allowed.

		pre.sar(data,  "( |-)?is",    "#", "gi"); // replace German sharp
		pre.sar(data,  "( |-)?es",    "-", "gi"); // replace German flat

		pre.sar(data,  "( |-)?sharp", "#", "gi"); // replace acc names with syms
		pre.sar(data,  "( |-)?flat",  "-", "gi"); // replace acc names with syms

		// convert solfege syllables into English note names.
		int solfege = 0;                    // true if using solfege syllables
		if (pre.sar(data, "ut|do",  "C", "gi")) {  solfege = 1; }
		if (pre.sar(data, "re",     "D", "gi")) {  solfege = 1; }
		if (pre.sar(data, "mi",     "E", "gi")) {  solfege = 1; }
		if (pre.sar(data, "sol?",   "G", "gi")) {  solfege = 1; }
		if (pre.sar(data, "la",     "A", "gi")) {  solfege = 1; }
		if (pre.sar(data, "[ts]i",  "B", "gi")) {  solfege = 1; }
		// if any other solfege syllables have been used, then convert
		// "fa" into F; otherwise, assume that "fa" means "F A";
		if (solfege) {
			pre.sar(data, "fa", "F", "gi");
		}

		pre.sar(data, "x", "##", "gi");

		pre.sar(data, "c##", "2", "gi");
		pre.sar(data, "d##", "4", "gi");
		pre.sar(data, "e##", "6", "gi");
		pre.sar(data, "f##", "7", "gi");
		pre.sar(data, "g##", "9", "gi");
		pre.sar(data, "a##", "Z", "gi");
		pre.sar(data, "b##", "1", "gi");

		pre.sar(data, "c#", "1", "gi");
		pre.sar(data, "d#", "3", "gi");
		pre.sar(data, "e#", "5", "gi");
		pre.sar(data, "f#", "6", "gi");
		pre.sar(data, "g#", "8", "gi");
		pre.sar(data, "a#", "Y", "gi");
		pre.sar(data, "b#", "0", "gi");

		pre.sar(data, "c--", "Y", "gi");
		pre.sar(data, "d--", "0", "gi");
		pre.sar(data, "e--", "2", "gi");
		pre.sar(data, "f--", "3", "gi");
		pre.sar(data, "g--", "5", "gi");
		pre.sar(data, "a--", "7", "gi");
		pre.sar(data, "b--", "9", "gi");

		pre.sar(data, "c-", "Z", "gi");
		pre.sar(data, "d-", "1", "gi");
		pre.sar(data, "e-", "3", "gi");
		pre.sar(data, "f-", "4", "gi");
		pre.sar(data, "g-", "6", "gi");
		pre.sar(data, "a-", "8", "gi");
		pre.sar(data, "b-", "Y", "gi");

		pre.sar(data, "c", "0", "gi");
		pre.sar(data, "d", "2", "gi");
		pre.sar(data, "e", "4", "gi");
		pre.sar(data, "f", "5", "gi");
		pre.sar(data, "g", "7", "gi");
		pre.sar(data, "a", "9", "gi");
		pre.sar(data, "b", "Z", "gi");

	}

	// convert Y and Z symbols to A and B (pitches 10 and 11 in thema index
	pre.tr(data, "rYZ", "RAB");

	// remove any characters which are invalid
	pre.sar(data, "[^RrAB0-9]", "", "g");
}



//////////////////////////////
//
// cleanPmusicalInterval -- based on processIntervalClassQuary in original
//    PERL interface.  This function cleans user input so that it can be
//    used to search the data which is formatted in a particular way.
//
//    Todo: add ^ and # wildcards.
//

void cleanPmusicalInterval(string& data) {
	PerlRegularExpression pre;
	PerlRegularExpression pre2;

	vector<string> pieces;
	pieces.reserve(1000);

	// remove invalid characters
	pre.sar(data, "[^Rr0-9\\+\\-MmPpAaDd\\.\\*\\? ]", "", "g");

	// translate characters
	pre.tr(data, "\\*\\+\\-Dapr", "SXxdAPR");

	// extract individual intervals from data string:

	string ptr;
	int index;

	// the following while loop should be improved, so that the second
	// long regular expression is not needed (in other words, increment
	// the pointer into the data for the start of the search so that
	// the match does not need to be destroyed once it is found.
	while (pre.search(data,
	"(S|R|\\*\\s*|\\.\\s*\\??|[Xx]?[PdmMA]?\\d+\\s*\\??|[Xx]?[PdmMA]\\s*\\??|[Xx]\\s*\\?\?)",
		"")) {
		ptr = pre.getSubmatch(1);
		if (pre2.search(ptr, "^\\s*$")) {
			continue;
		}
		index = (int)pieces.size();
		pieces.resize(index+1);
		pieces[index].clear();
		pieces[index] += ptr;
		prepareInterval(pieces[index]);
		pre.sar(data,
		"(S|R|\\*\\s*|\\.\\s*\\??|[Xx]?[PdmMA]?\\d+\\s*\\??|[Xx]?[PdmMA]\\s*\\??|[Xx]\\s*\\?\?)",
			"", "");
	}

	// prevent perfect unison from being mixed with P15, etc.
	pre.sar(data, "1(?!\\d)", "1(?!\\d)", "gi");

	data.clear();
	for (int i=0; i<(int)pieces.size(); i++) {
		data += pieces[i];
	}

	// change * to any interval
	pre.sar(data, "S", "(?:[Xx][mMPAd][1-9][0-9]?)*", "g");

}



//////////////////////////////
//
// prepareInterval -- Process a single interval item from the function
//      cleanPmusicalInterval.  Original function in PERL interface has
//      the same name.
//

void prepareInterval(string& data) {
	PerlRegularExpression pre;

	// rests are uninteresting to prepare
	if (pre.search(data, "R", "")) {
		return;
	}

	// * characters are handled later
	if (pre.search(data, "S", "")) {
		return;
	}

	// step 1: check to see which components are present
	pre.search(data, "([Xx])?([MmPAd])?([0-9]+)?(.*)", "");
	string direction;
	string quality;
	string ssize;
	string other;

	direction += pre.getSubmatch(1);
	quality += pre.getSubmatch(2);
	ssize += pre.getSubmatch(3);
	other += pre.getSubmatch(4);

	if (other == ".") {
		data = "([Xx]?[mMPAd][1-9][0-9]?)";
		return;
	}

	if (other == "*") {
		data = "([Xx]?[mMPAd][1-9][0-9]?)*";
		return;
	}

	if (direction.empty()) {
		int isize = strtol(ssize.c_str(), NULL, 10);
		if ((quality == "P") && (isize == 1)) {
		  // do not add an interval direction for P1
		} else {
			direction = "[Xx]?";
		}
	}

	if (quality.empty()) {
		quality = "[mMPAd]";
	}

	if (ssize.empty()) {
		ssize = "[1-9][0-9]?";
	}

	data.clear();
	if (!direction.empty()) {
		data += direction;
	}
	if (!quality.empty()) {
		data += quality;
	}
	if (!ssize.empty()) {
		data += ssize;
	}

	PerlRegularExpression pre2;

	// Perfect unisons do not have a direction so remove if present
	pre2.sar(data, "[X|x]P1$", "P1", "");

	if (!other.empty()) {
		if (pre2.search(other, "\\?", "")) {
			string tempdata;
			tempdata += "(";
			tempdata += data;
			tempdata += ")?";
			data = tempdata;
		}
	}
	if (pre.search(data, "(?<!\\d)1$")) {
		data += "\\t*";
	}
}



//////////////////////////////
//
// cleanPpitchClass -- based on processPitchQuery in original
//   PERL interface.  This function cleans user input so that it
//   can be used to search the data which is formatted in a particular way.
//

void cleanPpitchClass(string& data) {
	PerlRegularExpression pre;

	pre.sar(data,  "\\+", "#", "g");          // allow for + to mean sharp

	pre.sar(data,  "( |-)?is",    "#", "gi"); // replace German sharp
	pre.sar(data,  "( |-)?es",    "-", "gi"); // replace German flat

	pre.sar(data,  "( |-)?sharp", "#", "gi"); // replace acc names with symbols
	pre.sar(data,  "( |-)?flat",  "-", "gi"); // replace acc names with symbols

	// convert solfege syllables into English note names.
	int solfege = 0;                    // true if using solfege syllables
	if (pre.sar(data, "ut|do",  "C", "gi")) {  solfege = 1; }
	if (pre.sar(data, "re",     "D", "gi")) {  solfege = 1; }
	if (pre.sar(data, "mi",     "E", "gi")) {  solfege = 1; }
	if (pre.sar(data, "sol?",   "G", "gi")) {  solfege = 1; }
	if (pre.sar(data, "la",     "A", "gi")) {  solfege = 1; }
	if (pre.sar(data, "[ts]i",  "B", "gi")) {  solfege = 1; }
	// if any other solfege syllables have been used, then convert
	// "fa" into F; otherwise, assume that "fa" means "F A";
	if (solfege) {
		pre.sar(data, "fa", "F", "gi");
	}

	// remove duplicate spaces
	pre.sar(data, "\\s+",  " ",  "g");

	// adjust for aliases
	pre.tr(data, "\n\txa-hsSrmM-",  "  XA-H##Rbbb");
	pre.tr(data, "@",  "+");

	// expand double sharps
	pre.sar(data, "X",  "##",  "gi");

	// remove invalid chars
	pre.sar(data, "[^nA-HRb# H(){},.?+^*0-9]",  "",  "g");

	// make sure {} operator has valid syntax
	cleanUpRangeSyntaxNoOutsideDigitsOrComma(data);

	// convert parentheses temporarily into letters and store
	// real regex operators in temporary letter values
	pre.sar(data, "\\(", "Q", "g");
	pre.sar(data, "\\)", "q", "g");
	pre.sar(data, "q\\*", "qS", "g");
	pre.sar(data, "\\*", "S", "g");
	pre.sar(data, "q\\+", "qP", "g");
	pre.sar(data, "\\+", "P", "g");
	pre.sar(data, "q\\?", "qN", "g");


	// if a German B-natural is presents, map B->Bb and H->B
	if (pre.search(data, "H")) {
		pre.sar(data, "B", "Bb", "gi");     // transmute German B
		pre.sar(data, "H", "B",  "gi");     // transmute German H
	}

	// add spaces between notes
	pre.sar(data, "(?<=[^ ])(?=[A-GRQq{.])",  " ",  "g");

	// change meaning of * which means match to zero or more notes.
	pre.sar(data, "\\*",  "[YZ]*",  "g");
	// change meaning of "." which means any one pitch.
	pre.sar(data, "\\.",  "[A-G][#-]*",  "g");

	// remove duplicate spaces again
	pre.sar(data, "\\s+", " ", "g");

	// put parens around each note, then clean up:

	// remove any leading spaces
	pre.sar(data, "^\\s+",  "");

	// remove any trailing spaces
	pre.sar(data, "\\s+$",  "");

	// if the -D option is specified, then than means the
	// pitch names can contain any accidental (the search query
	// should not have any accidentals listed).
	if (diatonicQ) {
		pre.sar(data, "(?<=[A-G])(?![#nb-])", "^", "g");
	}
	// remove natural sign (no longer needed after diatonic processing
	pre.sar(data, "n", "", "g");

	// put a parenthesis at very beginning and very end of string
	pre.sar(data, "^",  "(?:");
	pre.sar(data, "$",  ")");

	// now put parentheses at each space separating a pitch.
	pre.sar(data, " ",  ") (?:",  "g");

	// move "?" outside of parens
	pre.sar(data, "\\?\\) ",      ")? ",  "g");
	pre.sar(data, "\\?\\)$",      ")?"       );

	// fix the space/"?" association
	pre.sar(data, "\\)\\?\\s",    " )?",  "g");

	// fix the space/"+" association
	pre.sar(data, "\\+\\)\\s",    " )+",  "g");

	// change meaning of ^ which means any chromatic alteration of the note
	// (including no alteration), in other words search for diatonic part
	// with any chromatic alteration
	pre.sar(data, "\\^",  "(?:#+|b+)?",   "g");

	// finalize change of *
	pre.sar(data, "Z",         "\t",      "g");
	pre.sar(data, "Y",         "^",       "g");
	pre.sar(data, "\\*\\) ",   "*)",      "g");

	// change )( to ) (
	pre.sar(data, "\\)\\(",    ") (",     "g");

	// get rid of null notes
	pre.sar(data, "\\(\\)",    "",        "g");

	// get rid of parentheses around {} operator
	pre.sar(data, "\\(\\?:\\{","{",       "g");
	pre.sar(data, "\\}\\)",    "}",       "g");

	// get rid of space before {} operator
	pre.sar(data, " \\{", "{", "g");

	pre.sar(data, "\\( ",        "(",     "g");
	// This next conversion is a problem for expanding .?
	// pre.sar(data, " \\)",        ")",     "g");

	// convert Q and q back into parentheses
	pre.sar(data, "\\(\\?:Q\\) ",    "(", "g");
	pre.sar(data, "\\(\\?:q",        "q", "g");
	pre.sar(data, "(?<=q)\\) ?",     "",  "g");
	pre.sar(data, "(?<=q[^ )])\\) ?","",  "g");
	pre.sar(data, "q",               ")", "g");

	// put regex operators back to normal:
	pre.sar(data, "S\\) ", ") (?:[A-G][#b]* )*?", "g");
	pre.sar(data, "P\\) ", " )+?", "g");
	pre.sar(data, "P\\)$", " ?)+?", "g");
	pre.sar(data, "S\\)$", ") (?:[A-G][#b]* ?)*?", "g");
	pre.sar(data, "N", "?", "g");

	// fix a bug related to * operator:
	pre.sar(data, "\\(\\?:\\) ", "", "g");

	// thema command adds the space later, so get rid of any at the end
	pre.sar(data, " $", "");

	// added 10 Dec 2000
	pre.sar(data, "^ +", "");

	// moved from appendToSearchString [20101123]
	data += "[ \\t]";
}



//////////////////////////////
//
// cleanUpRangeSyntax: disallow digits or commas to exist
//   anywhere outside of the {} regular-expression operator.
//

void cleanUpRangeSyntaxNoOutsideDigitsOrComma(string& data) {
	int inside = 0;
	stringstream tempdata;

	for (int i=0; i<(int)data.size(); i++) {
		if ((inside == 0) && (data[i] == '{')) {
			inside = 1;
			tempdata << data[i];
			continue;
		}
		if ((inside == 1) && (data[i] == '}')) {
			inside = 0;
			tempdata << data[i];
		}
		if (inside) {
			if (std::isdigit(data[i]) || (data[i] == ',')) {
				tempdata << data[i];
			}
			continue;
		} else {
			if (std::isdigit(data[i]) || (data[i] == ',') || (data[i] == '}') ||
					(data[i] == '{')) {
				continue;
			} else {
				tempdata << data[i];
				continue;
			}
		}
	}

	data = tempdata.str();
}



////////////////////////////////////////////////////////////////////////////
//
// smart search code ideas
//
// If --smart is added, then second argument is a **kern file which
// represents a search query as **kern data.   (or if no second argument,
// then first argument is the **kern file, and standard input is the
// search index).
//
// * If there is a meter, then a search on pitches and metric position
// filtered for that meter will be done.
//
// * If there is a key designation, then that key will be used to filter
// the keys.
//
// For a smart search, if the most explicit search returns no results, then
// other less strict search features will be searched.  For example if the
// exact pitches, duration and metric positions are not generating a match,
// then try pitches and durations without metric position.  If that returns
// no matches, then try 12tone pitch classes (enharmonic spellings), then
// try pitch without durations, allowing repeated notes between pitches,
// allowing various chromatic alterations of the pitches, then musical
// intervals with durations, 12tone intervals with durations,
// musical intervals without durations, refined contour with durations,
// refined contour with refined duration contour, gross contour with refined
// duration contour, pitch gross contour with duration gross contour.
//

void processKernString(const string& astring) {
	vector<string> tokens;
	PerlRegularExpression::getTokens(tokens, "\\s+", astring);
	PerlRegularExpression pre;
	string tempc;
	int noteQ;

	vector<int> sequence;
	sequence.reserve(tokens.size());

	for (int i=0; i<(int)tokens.size(); i++) {
		tempc = tokens[i];
		pre.sar(tempc, "([A-Ga-g])+", "", "");
		pre.sar(tempc, "\\d+", "", "g");
		pre.sar(tempc, "\\.+", "", "g");
		pre.sar(tempc, "-+",   "", "g");
		pre.sar(tempc, "\\#+", "", "g");
		if (pre.search(tempc, "^\\s*$")) {
			noteQ = 1;
		} else {
			noteQ = 0;
		}
		if (noteQ) {
			sequence.push_back(i);
			// cout << tokens[i] << endl;
		}
	}

	vector<int> pitches(sequence.size());
	vector<double> durations(sequence.size());
	int pitchesq = 0;
	int dursq    = 0;

	for (int i=0; i<(int)sequence.size(); i++) {
		pitches[i]   = Convert::kernToBase40(tokens[sequence[i]]);
		if ((pitches[i] > 1000) || (pitches[i] < 0)) {
			pitches[i] = -1;
		} else {
			pitchesq = 1;
		}
		durations[i] = Convert::kernToDuration(tokens[sequence[i]]);
		if (durations[i] <= 0.0) {
		} else {
			dursq = 1;
		}
	}

	if (pitchesq) {
		PpitchclassQ = 1;    // activate -p option
	}
	if (dursq) {
		RdurationQ = 1;      // activate -u option
	}

	stringstream pitchseq;
	stringstream durseq;

	char buffer[1024] = {};

	for (int i=0; i<(int)sequence.size(); i++) {
	  // cout << tokens[sequence[i]] << "\t" << pitches[i]
	  // << "\t" << durations[i] << endl;
	  if (pitchesq) {
		  if (pitches[i] > 0) {
			  Convert::base40ToKern(buffer, pitches[i]%40 + 3*40);
			  pitchseq << buffer;
		  } else {
			  pitchseq << ".";
		  }
		  if (i < (int)sequence.size()-1) {
			  pitchseq << ' ';
		  }
	  }
	  if (dursq) {
		  if (durations[i] > 0) {
			  Convert::durationToKernRhythm(buffer, durations[i]);
			  if (pre.search(buffer, "-")) {
				  durseq << "x";
			  } else {
				  durseq << buffer;
			  }
		  } else {
			  durseq << "x";
		  }
		  if (i < (int)sequence.size()-1) {
			  durseq << ' ';
		  }
	  }
	}

	Ppitchclass = pitchseq.str();
	Rduration   = durseq.str();
}



