//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Oct 22 15:33:41 PDT 2000
// Last Modified: Sun May 26 19:39:01 PDT 2002 Mostly finished
// Last Modified: Tue Mar 16 05:53:19 PST 2010 Added *M meter description
// Last Modified: Wed Apr 21 14:31:44 PDT 2010 Added search feature
// Last Modified: Wed May 19 15:30:49 PDT 2010 Added tick & rational values
// Last Modified: Sat Apr 28 09:03:39 PDT 2018 Converted to HumdrumStream input
// Last Modified: Sun Apr 29 15:07:07 PDT 2018 Added -m and --meter-bottom options
// Filename:      ...sig/examples/all/beat.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/beat.cpp
// Syntax:        C++; museinfo
// vim:           ts=3
//
// Description:   Generates metrical location data for **kern or **recip
//                entries.  Test functions for the built-in rhythmic analysis
//                in the HumdrumFile class.  Should give the same
//                output as the beat program.
//
// There are two cases when an incomplete measure needs to
//    be counted backwards.  These cases will be handled by
//    the beat program:
//    (1) an initial pickup beat
//    (2) a repeat sign breaks a true measure
//
// There is a bug that needs fixing:
//   The *M2/2 interpretations are erased (at least with the -s option)
//
// five types of outputs can be given:
//   -s   = sum the number of beats in a measure
//        = display the beat (default if no other output type given)
//   -d   = duration
//   -c   = cumulative running total beat/duration
//

#include "humdrum.h"

#include <math.h>
#include <string.h>

#include <cctype>
#include <vector>
#include <sstream>
#include <string>

using namespace std;


// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
void      printOutput        (HumdrumFile& file,
                              vector<RationalNumber>& Bfeatures,
                              vector<int>& Blines,
                              vector<RationalNumber>& Dfeatures,
                              vector<int>& Dlines, vector<int>& tickanalysis);
RationalNumber getPickupDuration (HumdrumFile& file);
void      fillSearchString   (vector<double>& searcher, const string& astring);
void      printSearchResults (HumdrumFile& infile,
                              vector<RationalNumber>& Bfeatures,
                              vector<int>& Blines,
                              vector<RationalNumber>& Dfeatures,
                              vector<int>& Dlines);
void      printSearchResultsFinal(vector<int>& linematch, HumdrumFile& infile,
                              vector<RationalNumber>& Bfeatures,
                              vector<int>& Blines,
                              vector<RationalNumber>& Dfeatures,
                              vector<int>& Dloines);
void      doBeatSearch       (vector<int>& results, HumdrumFile& infile,
                              vector<double> search,
                              vector<RationalNumber>& Bfeatures,
                              vector<int>& Blines);
void      doDurSearch        (vector<int>& results, HumdrumFile& infile,
                              vector<double> search,
                              vector<RationalNumber>& Dfeatures,
                              vector<int>& Dlines);
void      doDurSearch        (vector<int>& results, HumdrumFile& infile,
                              vector<double> search, vector<double>& Dfeatures,
                              vector<int>& Dlines);
void      mergeResults       (vector<int>& output, vector<int>& input1,
                              vector<int>& input2);
void      printSequence      (vector<double>& pattern);
void      printSequence      (vector<RationalNumber>& pattern);
void      fillMeasureInfo    (HumdrumFile& infile, vector<double>& measures);
void      doComparison       (vector<int>& results, vector<int>& line,
                              vector<double>& search, vector<double>& data,
                              HumdrumFile& infile);
int       checkForWildcard   (vector<double>& sequence);
void      extractBeatFeatures(HumdrumFile& infile, vector<int>& line,
                              vector<RationalNumber>& data);
void      extractDurFeatures (HumdrumFile& infile, vector<int>& line,
                              vector<RationalNumber>& data);
void      printSequence      (vector<double>& features, vector<int>& lines,
                              vector<double>& search, int startline);
void      printSequence      (vector<RationalNumber>& features,
                              vector<int>& lines, vector<RationalNumber>& search,
                              int startline);
void      printSequence      (vector<RationalNumber>& features,
                              vector<int>& lines, vector<double>& search,
                              int startline);
void      printMatchesWithData(vector<int>& linematch, HumdrumFile& infile);
void      fillAttackArray    (HumdrumFile& infile, vector<int>& attacks);
int       getCountForLine    (HumdrumFile& infile, int line);
int       doTickAnalysis     (vector<int>& tickanalysis, HumdrumFile& infile);
RationalNumber getDurationOfFirstMeasure(HumdrumFile& file);
void      analyzeFile        (HumdrumFile& infile);
void      prepareMeterData   (HumdrumFile& infile);

// global variables
Options   options;             // database for command-line arguments
int       appendQ  = 0;        // used with -a option
int       prependQ = 0;        // used with -p option
int       durQ     = 0;        // used with -d option
int       absQ     = 0;        // used with -t option
int       beatQ    = 0;        // used with -b option
int       sumQ     = 0;        // used with -s option
int       zeroQ    = 0;        // zero offset instead of 1 for first beat
int       nullQ    = 0;        // used with -n option
vector<double> Bsearch;        // used with -B option
vector<double> Dsearch;        // used with -D option
double    Rvalue   = -1.0;     // used with -R option
double    Tolerance = 0.001;   // used for rounding
int       Attack   = 1;        // used with -A option
vector<int> Attacks;           // used with -A option
int       tickQ    = 0;        // used with -t option
int       metertopQ= 0;        // used with -m option
int       meterbotQ= 0;        // used with --meter-bottom option
int       meterdurQ= 0;        // used with --meter-duration option
int       rationalQ= 0;        // used with -r option
int       tpwQ     = 0;        // used with --tpw option
int       tpqQ     = 0;        // used with --tpq option
int       fillQ    = 0;        // used with --fill option
string    beatbase = "4";      // used with --beatsize option
int       uQ       = 0;        // used for -f and -u interactions
int       debugQ   = 0;        // used with --debug option

vector<string> Metertop;      // used with -m option
vector<string> Meterbot;      // used with -meter-bottom option
vector<RationalNumber> Meterdur; // used with --meter-duration option

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	checkOptions(options, argc, argv);
	HumdrumStream streamer(options);
	HumdrumFile infile;

	while (streamer.read(infile)) {
		analyzeFile(infile);
	}

	return 0;
}



//////////////////////////////
//
// analyzeFile --
//

void analyzeFile(HumdrumFile& infile) {
	vector<RationalNumber> Bfeatures; // used to extract beat data from input
	vector<RationalNumber> Dfeatures; // used to extract duration data from input
	vector<int>    Blines;            // used to extract beat data from input
	vector<int>    Dlines;            // used to extract duration data from input

	infile.analyzeRhythm(beatbase.c_str());

	if (metertopQ || meterbotQ || meterdurQ) {
		prepareMeterData(infile);
	}

	vector<int> tickanalysis;
	tickanalysis.resize(infile.getNumLines());
	std::fill(tickanalysis.begin(), tickanalysis.end(), 0);
	int tickfactor = 1;

	if (tickQ) {
		tickfactor = doTickAnalysis(tickanalysis, infile);
	}

	if (tpwQ) {
		cout << infile.getMinTimeBase() * tickfactor << endl;
		exit(0);
	} else if (tpqQ) {
		cout << infile.getMinTimeBase() * tickfactor /4.0 << endl;
		exit(0);
	}

	fillAttackArray(infile, Attacks);
	extractBeatFeatures(infile, Blines, Bfeatures);
	if (debugQ) {
		cout << "BEAT FEATURES ====================" << endl;
		for (int ii=0; ii<(int)Bfeatures.size(); ii++) {
			cout << Bfeatures[ii].getFloat() << endl;
		}
		cout << "==================================" << endl;
	}
	extractDurFeatures(infile, Dlines, Dfeatures);

	if (Bsearch.size() > 0 || Dsearch.size() > 0) {
		printSearchResults(infile, Bfeatures, Blines, Dfeatures, Dlines);
	} else {
		printOutput(infile, Bfeatures, Blines, Dfeatures, Dlines,
				tickanalysis);
	}
}


///////////////////////////////////////////////////////////////////////////



//////////////////////////////
//
// prepareMeterData --
//

void prepareMeterData(HumdrumFile& infile) {
	Metertop.resize(infile.getNumLines());
	Meterbot.resize(infile.getNumLines());
   Meterdur.resize(infile.getNumLines());
	fill(Meterdur.begin(), Meterdur.end(), 0);
	std::fill(Metertop.begin(), Metertop.end(), ".");
	std::fill(Meterbot.begin(), Meterbot.end(), ".");
	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isInterpretation()) {
			for (int j=0; j<infile[i].getFieldCount(); j++) {
				if (!infile[i].isTimeSig(j)) {
					continue;
				}
				int top, bot;
				int count = sscanf(infile[i][j], "*M%d/%d", &top, &bot);
				if (count == 2) {
               Metertop[i] = to_string(top);
               Meterbot[i] = to_string(bot);
					Meterdur[i] = top;
					Meterdur[i] /= bot;
					Meterdur[i] *= 4;
				}
			}
		}
	}

	for (int i=1; i<(int)Metertop.size(); i++) {
		if (Metertop[i] == ".") {
			if (Metertop[i-1] != ".") {
				Metertop[i] = Metertop[i-1];
			}
		}
		if (Meterbot[i] == ".") {
			if (Meterbot[i-1] != ".") {
				Meterbot[i] = Meterbot[i-1];
			}
		}
		if (Meterdur[i] == 0) {
			if (Meterdur[i-1] != 0) {
				Meterdur[i] = Meterdur[i-1];
			}
		}
	}

	if (!fillQ) {
		for (int i=0; i<infile.getNumLines(); i++) {
			if (infile[i].getBeatR() != 1) {
				Metertop[i] = ".";
				Meterbot[i] = ".";
				Meterdur[i] = 0;
			}
		}
	}
}



//////////////////////////////
//
// doTickAnalysis --
//

int doTickAnalysis(vector<int>& tickanalysis, HumdrumFile& infile) {
	int i;
	tickanalysis.resize(infile.getNumLines());

	vector<RationalNumber> pretick(tickanalysis.size());

	int minrhy = infile.getMinTimeBase();
	if (minrhy <= 0.0) {
		return 1;
	}
	RationalNumber value;
	int monitor = 0;
	for (i=0; i<infile.getNumLines()-1; i++) {
		value = ((infile[i+1].getAbsBeatR()-infile[i].getAbsBeatR())/4)*minrhy;
		pretick[i] = value;
		if (value.getDenominator() != 1) {
			monitor = 1;
		}
	}

	if (monitor == 0) {
		for (i=0; i<(int)pretick.size(); i++) {
			tickanalysis[i] = pretick[i].getNumerator();
		}
		return 1;
	}

	for (i=0; i<(int)pretick.size(); i++) {
		// estimate a multiplication of 4 to remove fractional part.
		tickanalysis[i] = pretick[i].getNumerator() * 4;
	}

	return 4;
}



//////////////////////////////
//
// fillAttackArray --
//

void fillAttackArray(HumdrumFile& infile, vector<int>& attacks) {
	int i;
	attacks.clear();
	attacks.reserve(infile.getNumLines());

	if (Attack <= 0) {
		// don't need to waste time analyzing the attack structure of the data...
		return;
	}

	int count;
	for (i=0; i<infile.getNumLines(); i++) {
		count = 0;
		if (infile[i].isData()) {
			count = getCountForLine(infile, i);
		}
		attacks[i] = count;
	}
}



//////////////////////////////
//
// getCountForLine -- return the number of note attacks on the given
//   line in the Humdrum File.  Only **kern and **recip spines are examined.
//   Rests and tied notes are ignored when counting attacks.
//

int getCountForLine(HumdrumFile& infile, int line) {
	int j, k;
	char buffer[128] = {0};
	int output = 0;
	int tcount;
	for (j=0; j<infile[line].getFieldCount(); j++) {
		if (!(infile[line].isExInterp(j, "**kern") ||
				infile[line].isExInterp(j, "**recip"))) {
			continue;
		}
		tcount = infile[line].getTokenCount(j);
		for (k=0; k<tcount; k++) {
			infile[line].getToken(buffer, j, k);
			if (strchr(buffer, 'r') != NULL) {
				// ignore rests
				continue;
			}
			if (strcmp(buffer, ".") == 0) {
				// ignore null tokens
				continue;
			}
			if (strchr(buffer, '_') != NULL) {
				// ignore notes which are in the middle of a series of tied notes
				continue;
			}
			if (strchr(buffer, ']') != NULL) {
				// ignore notes which are at the end of a series of tied notes
				continue;
			}
			output++;
		}
	}

	return output;
}



//////////////////////////////
//
// printSearchResults -- Search for beat and/or duration patterns
//   in the composition rhythmic data
//

void printSearchResults(HumdrumFile& infile, vector<RationalNumber>& Bfeatures,
		vector<int>& Blines, vector<RationalNumber>& Dfeatures,
		vector<int>& Dlines) {
	vector<int> Bresults;
	vector<int> Dresults;
	Bresults.clear();
	Bresults.reserve(100000);
	Dresults.clear();
	Dresults.reserve(100000);

	if ((Bsearch.size() > 0) && (Dsearch.size() > 0)) {
		doBeatSearch(Bresults, infile, Bsearch, Bfeatures, Blines);
		doDurSearch(Dresults, infile, Dsearch, Dfeatures, Dlines);
		vector<int> finalresults;
		mergeResults(finalresults, Bresults, Dresults);

		cout << "!!parallel beat search: ";
		printSequence(Bsearch);
		cout << endl;
		cout << "!!parallel duration search: ";
		printSequence(Dsearch);
		cout << endl;

		printSearchResultsFinal(finalresults, infile, Bfeatures, Blines,
				Dfeatures, Dlines);
	} else if (Bsearch.size() > 0) {
		doBeatSearch(Bresults, infile, Bsearch, Bfeatures, Blines);
		cout << "!!beat search: ";
		printSequence(Bsearch);
		cout << endl;
		printSearchResultsFinal(Bresults, infile, Bfeatures, Blines,
				Dfeatures, Dlines);
	} else if (Dsearch.size() > 0) {
		doDurSearch(Dresults, infile, Dsearch, Dfeatures, Dlines);
		cout << "!!duration search: ";
		printSequence(Dsearch);
		cout << endl;
		printSearchResultsFinal(Dresults, infile, Bfeatures, Blines,
				Dfeatures, Dlines);
	} else {
		cout << "ERROR in search" << endl;
	}

}



//////////////////////////////
//
// printSequence --
//

void printSequence(vector<double>& pattern) {
	int i;
	for (i=0; i<(int)pattern.size(); i++) {
		if (pattern[i] < 0) {
			cout << "*";
		} else {
			cout << pattern[i];
		}
		if (i < (int)pattern.size()-1) {
			cout << ' ';
		}
	}
}

void printSequence(vector<RationalNumber>& pattern) {
	int i;
	for (i=0; i<(int)pattern.size(); i++) {
		if (pattern[i] < 0) {
			cout << "*";
		} else {
			cout << pattern[i];
		}
		if (i < (int)pattern.size()-1) {
			cout << ' ';
		}
	}
}



//////////////////////////////
//
// checkForWildcard -- returns true if any of the values are negative.
//

int checkForWildcard(vector<double>& sequence) {
	int i;
	for (i=0; i<(int)sequence.size(); i++) {
		if (sequence[i] < 0.0) {
			return 1;
		}
	}
	return 0;
}



//////////////////////////////
//
// printMatchesWithData --
//

void printMatchesWithData(vector<int>& linematch, HumdrumFile& infile) {
	int i;
	int counter = 1;


	vector<int> lookup;
	lookup.resize(infile.getNumLines());
	std::fill(lookup.begin(), lookup.end(), -1);

	for (i=0; i<(int)linematch.size(); i++) {
		lookup[linematch[i]] = counter++;
	}

	for (int i=0; i<infile.getNumLines(); i++) {
		switch (infile[i].getType()) {
			/* case E_humrec_data_comment: break; */

			case E_humrec_data_kern_measure:
				if (prependQ) {
					cout << infile[i][0] << "\t";
					cout << infile[i] << "\n";
				} else if (appendQ) {
					cout << infile[i] << "\t";
					cout << infile[i][0] << "\n";
				} else {
					cout << infile[i][0] << "\n";
				}
				break;

			case E_humrec_interpretation:
				if (appendQ) {
					cout << infile[i] << "\t";
				}

				if (strncmp(infile[i][0], "**", 2) == 0) {
					cout << "**match";
				} else if (strcmp(infile[i][0], "*-") == 0) {
					cout << "*-";
				} else if (strncmp(infile[i][0], "*>", 2) == 0) {
					cout << infile[i][0];
				} else {
					cout << "*";
				}

				if (prependQ) {
					cout << "\t" << infile[i];
				}
				cout << "\n";

				break;

			case E_humrec_data:
				if (appendQ) {
					cout << infile[i] << "\t";
				}
				if (lookup[i] < 0) {
					cout << ".";
				} else {
					cout << lookup[i];
				}

				if (prependQ) {
					cout << "\t" << infile[i];
				}

				cout << "\n";

				break;
			case E_humrec_local_comment:
				if (appendQ) {
					cout << infile[i] << "\t";
				}
				cout << "!";
				if (prependQ) {
					cout << "\t" << infile[i];
				}
				cout << "\n";

				break;
			case E_humrec_none:
			case E_humrec_empty:
			case E_humrec_global_comment:
			case E_humrec_bibliography:
			default:
				cout << infile[i] << "\n";
				break;
		}
	}
}



//////////////////////////////
//
// printSearchResultsFinal --
//

void printSearchResultsFinal(vector<int>& linematch, HumdrumFile& infile,
		vector<RationalNumber>& Bfeatures, vector<int>& Blines,
		vector<RationalNumber>& Dfeatures, vector<int>& Dlines) {
	cout << "!!matches: " << (int)linematch.size() << "\n";

	if (appendQ || prependQ) {
		printMatchesWithData(linematch, infile);
		return;
	}

	vector<double> measures;
	fillMeasureInfo(infile, measures);


	int hasBWildcard = checkForWildcard(Bsearch);
	int hasDWildcard = checkForWildcard(Dsearch);

	int i;
	cout << "**line\t**bar\t**beat\t**absb";

	if (hasBWildcard) {
		cout << "\t**bseq";
	}
	if (hasDWildcard) {
		cout << "\t**dseq";
	}

	cout << "\n";
	for (i=0; i<(int)linematch.size(); i++) {
		cout << linematch[i]-1;
		cout << "\t" << measures[linematch[i]];
		if (zeroQ) {
			cout << "\t" << infile[linematch[i]].getBeat()-1;
		} else {
			cout << "\t" << infile[linematch[i]].getBeat();
		}
		cout << "\t" << infile[linematch[i]].getAbsBeat();
		if (hasBWildcard) {
			cout << "\t";
			printSequence(Bfeatures, Blines, Bsearch, linematch[i]);
		}
		if (hasDWildcard) {
			cout << "\t";
			printSequence(Dfeatures, Dlines, Dsearch, linematch[i]);
		}
		cout << endl;
	}
	cout << "*-\t*-\t*-\t*-";
	if (hasBWildcard) {
		cout << "\t*-";
	}
	if (hasDWildcard) {
		cout << "\t*-";
	}
	cout << "\n";
}



///////////////////////////////
//
// printSequence --
//

void printSequence(vector<double>& features, vector<int>& lines,
		vector<double>& search, int startline) {

	int index = -1;
	int i;
	for (i=0; i<(int)lines.size(); i++) {
		if (lines[i] == startline) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		cout << ".";
		return;
	}

	int stopindex = index + (int)search.size() - 1;
	for (i=index; (i<(int)features.size()) && (i<=stopindex); i++) {
		cout << features[i];
		if (i < stopindex) {
			cout << " ";
		}
	}
}



void printSequence(vector<RationalNumber>& features, vector<int>& lines,
		vector<double>& search, int startline) {

	int index = -1;
	int i;
	for (i=0; i<(int)lines.size(); i++) {
		if (lines[i] == startline) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		cout << ".";
		return;
	}

	int stopindex = index + (int)search.size() - 1;
	for (i=index; (i<(int)features.size()) && (i<=stopindex); i++) {
		cout << features[i];
		if (i < stopindex) {
			cout << " ";
		}
	}
}



void printSequence(vector<RationalNumber>& features, vector<int>& lines,
		vector<RationalNumber>& search, int startline) {

	int index = -1;
	int i;
	for (i=0; i<(int)lines.size(); i++) {
		if (lines[i] == startline) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		cout << ".";
		return;
	}

	int stopindex = index + (int)search.size() - 1;
	for (i=index; (i<(int)features.size()) && (i<=stopindex); i++) {
		cout << features[i];
		if (i < stopindex) {
			cout << " ";
		}
	}
}



//////////////////////////////
//
// fillMeasureInfo --
//

void fillMeasureInfo(HumdrumFile& infile, vector<double>& measures) {
	int i;
	measures.resize(infile.getNumLines(), 0.0);

	double current = 0.0;
	for (i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isMeasure()) {
			sscanf(infile[i][0], "=%lf", &current);
		}
		measures[i] = current;
	}
}


//////////////////////////////
//
// mergeResults -- do an intersection of two lists of sorted integers
//

void mergeResults(vector<int>& output, vector<int>& input1, vector<int>& input2) {
	int i, j;
	int maxsize = (int)input1.size();
	if ((int)input2.size() < maxsize) {
		maxsize = (int)input2.size();
	}
	output.clear();
	output.reserve(maxsize);
	if (maxsize == 0) {
		return;
	}
	int similar;
	j=0;
	for (i=0; i<(int)input1.size(); i++) {
		while ((j<(int)input2.size()) && (input2[j] < input1[i])) {
			j++;
		}
		if (j >= (int)input2.size()) {
			break;
		}
		if (input2[j] == input1[i]) {
			similar = input2[j];
			output.push_back(similar);
		}
	}
}



//////////////////////////////
//
// doBeatSearch -- search for specific beat pattern in data.
//

void doBeatSearch(vector<int>& results, HumdrumFile& infile,
		vector<double> search, vector<RationalNumber>& Bfeatures,
		vector<int>& Blines) {
	// extractBeatFeatures(infile, Blines, Bfeatures);
	vector<double> doubleBfeatures(Bfeatures.size());
	int i;
	for (i=0; i<(int)doubleBfeatures.size(); i++) {
		doubleBfeatures[i] = Bfeatures[i].getFloat();
	}
	doComparison(results, Blines, search, doubleBfeatures, infile);
}



//////////////////////////////
//
// extractBeatFeatures --
//

void extractBeatFeatures(HumdrumFile& infile, vector<int>& line,
		vector<RationalNumber>& data) {
	line.clear();
	line.reserve(infile.getNumLines());
	data.clear();
	data.reserve(infile.getNumLines());

	int lval;
	RationalNumber bval;
	int i;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
			continue;
		}
		bval = infile[i].getBeatR();
		lval = i;
		if (Attacks[i] < Attack) {
			// ignore lines which do not have enough note onsets
			continue;
		}
		line.push_back(lval);
		data.push_back(bval);
	}
}



//////////////////////////////
//
// extractDurFeatures --
//

void extractDurFeatures(HumdrumFile& infile, vector<int>& line,
		vector<RationalNumber>& data) {
	line.clear();
	line.resize(infile.getNumLines());
	data.clear();
	data.resize(infile.getNumLines());

	int lval;
	RationalNumber bval;
	int i;
	for (i=0; i<infile.getNumLines(); i++) {
		if (!infile[i].isData()) {
			continue;
		}
		bval = infile[i].getDurationR();
		if (zeroQ) {
			bval = bval - 1;
		}
		lval = i;
		if (Attacks[i] < Attack) {
			// ignore lines which do not have enough note onsets,
			// adding duration of current line to last data line
			// which exceeds the onset threshold test.
			if ((int)data.size() > 0) {
			data[(int)data.size()-1] += bval;
			}
			continue;
		}
		line.push_back(lval);
		data.push_back(bval);
	}
}



//////////////////////////////
//
// doComparison --
//

void doComparison(vector<int>& results, vector<int>& line, vector<double>& search,
		vector<double>& data, HumdrumFile& infile) {
	results.clear();
	results.reserve((int)data.size() - (int)search.size() + 1);

	double startdur;
	double stopdur;

	int match;
	int i, j;
	for (i=0; i<(int)data.size() - (int)search.size() + 1; i++) {
		match = 1;
		for (j=0; j<(int)search.size(); j++) {
			if (search[j] < 0) {
				// for wildcard match (*)
				continue;
			}
			if (fabs(search[j] - data[i+j]) > Tolerance) {
				match = 0;
				break;
			}
		}
		if (match) {
			if (Rvalue > 0) {
				startdur = infile[line[i]].getAbsBeat();
				stopdur  = infile[line[i+(int)search.size()-1]].getAbsBeat();
				if (fabs(Rvalue - (stopdur-startdur)) < Tolerance) {
					results.push_back(line[i]);
				}
			} else {
				results.push_back(line[i]);
			}
		}
	}
}



//////////////////////////////
//
// doDurSearch -- search for specific beat pattern in data.
//

void doDurSearch(vector<int>& results, HumdrumFile& infile,
		vector<double> search, vector<double>& Dfeatures, vector<int>& Dlines) {
	// extractDurFeatures(infile, Dlines, Dfeatures);
	doComparison(results, Dlines, search, Dfeatures, infile);
}

void doDurSearch(vector<int>& results, HumdrumFile& infile,
		vector<double> search, vector<RationalNumber>& Dfeatures,
		vector<int>& Dlines) {
	int i;
	vector<double> doubleDfeatures((int)Dfeatures.size());
	for (i=0; i<(int)doubleDfeatures.size(); i++) {
		doubleDfeatures[i] = Dfeatures[i].getFloat();
	}

	doComparison(results, Dlines, search, doubleDfeatures, infile);
}



//////////////////////////////
//
// getPickupDuration --
//

RationalNumber getPickupDuration(HumdrumFile& file) {
	int i;
	for (i=0; i<file.getNumLines(); i++) {
		if (!file[i].isMeasure()) {
			continue;
		}
		return file[i].getAbsBeatR();
	}

	RationalNumber nothing(-1,1);
	return nothing;
}



//////////////////////////////
//
// printOutput --
//

void printOutput(HumdrumFile& file, vector<RationalNumber>& Bfeatures,
		vector<int>& Blines, vector<RationalNumber>& Dfeatures, vector<int>& Dlines,
		vector<int>& tickanalysis) {
	int lastmeasureline = -1;
	int pickupstate = 0;
	int suppressreturn = 0;
	int i;
	string lastSum = ".";

	vector<unsigned long> abstick;
	if (tickQ) {
		unsigned long csum = 0;
		abstick.resize((int)tickanalysis.size());
		std::fill(abstick.begin(), abstick.end(), 0);
		for (i=0; i<(int)tickanalysis.size(); i++) {
			abstick[i] = csum;
			csum += tickanalysis[i];
		}
	}

	RationalNumber minrhy(file.getMinTimeBase(), 4);
	RationalNumber rat;
	vector<RationalNumber> Binfo(file.getNumLines(), -1);
	vector<RationalNumber> Dinfo(file.getNumLines(), -1);

	int measurecount = 0;

	for (i=0; i<(int)Blines.size(); i++) {
		Binfo[Blines[i]] = Bfeatures[i];
		if (Binfo[Blines[i]] == file[Blines[i]].getAbsBeatR()) {
			Binfo[Blines[i]]++;
			Binfo[Blines[i]] -= file.getPickupDurationR();
		}
		if (zeroQ) {
			Binfo[Blines[i]]--;
		}
	}
	for (i=0; i<(int)Dlines.size(); i++) {
		Dinfo[Dlines[i]] = Dfeatures[i];
	}

	for (i=0; i<file.getNumLines(); i++) {
		switch (file[i].getType()) {
			/*case E_humrec_data_comment:
				if (appendQ) {
					cout << file[i] << "\t" << "!" << "\n";
				}  else if (prependQ) {
					cout << "!\t" << file[i] << "\n";
				} else {
					cout << file[i] << "\n";
				}
				break;
			*/

			case E_humrec_data_kern_measure:
				if (prependQ) {
					cout << file[i][0] << "\t";
					cout << file[i] << "\n";
				} else if (appendQ) {
					cout << file[i] << "\t";
					cout << file[i][0] << "\n";
				} else {
					cout << file[i][0] << "\n";
				}
				lastmeasureline = i;
				measurecount++;
				break;

			case E_humrec_interpretation:
				if (appendQ) {
					cout << file[i] << "\t";
				}

				if (strncmp(file[i][0], "**", 2) == 0) {
					if (absQ && !tickQ) {
						cout << "**absb";
					} else if (absQ && tickQ && !rationalQ) {
						cout << "**atick";
					} else if (absQ && tickQ && rationalQ) {
						cout << "**adur";
					} else if (tickQ && durQ && !rationalQ) {
						cout << "**dtick";
					} else if (tickQ && durQ && rationalQ) {
						cout << "**dur";
					} else if (durQ && !tickQ) {
						cout << "**dur";
					} else if (sumQ) {
						cout << "**beatsum";
					} else if (metertopQ) {
						cout << "**mtop";
					} else if (meterbotQ) {
						cout << "**mbot";
					} else if (meterdurQ) {
						cout << "**mdur";
					} else {
						cout << "**beat";
					}
				} else if (strcmp(file[i][0], "*-") == 0) {
					cout << "*-";
				} else if (strncmp(file[i][0], "*>", 2) == 0) {
					cout << file[i][0];
				} else {
					if ((strncmp(file[i][0], "*M", 2) == 0) &&
						(strchr(file[i][0], '/') != NULL)) {
						cout << file[i][0];
					} else if (strncmp(file[i][0], "*MM", 3) == 0) {
						cout << file[i][0];
					} else if (appendQ || prependQ) {
						cout << "*";
					} else {
						cout << "*";
					}
				}

				if (prependQ) {
					cout << "\t" << file[i];
				}
				cout << "\n";

				break;

			case E_humrec_data:
				if (appendQ) {
					cout << file[i] << "\t";
				}
				if (file[i][0][0] == '=') {
					pickupstate++;
				}

				if (durQ) {
					// cout << file[i].getDuration();
					if (Dinfo[i] >= 0) {
						if (tickQ && !rationalQ) {
							cout << tickanalysis[i];
						} else if (tickQ && rationalQ) {
							rat.setValue(tickanalysis[i], file.getMinTimeBase());
						if (uQ) {
								rat *= 4;
							}
							cout << rat;
						} else {
							cout << Dinfo[i].getFloat();
						}
					} else {
						if (nullQ || appendQ || prependQ) {
							cout << ".";
						} else {
							suppressreturn = 1;
						}
					}
				} else if (metertopQ) {
					cout << Metertop[i];
				} else if (meterbotQ) {
					cout << Meterbot[i];
				} else if (meterdurQ) {
					if (Meterdur[i] == 0) {
						cout << ".";
					} else {
						cout << Meterdur[i];
					}
				} else if (absQ) {
					if (tickQ && !rationalQ) {
						cout << abstick[i];
					} else if (tickQ && rationalQ) {
						RationalNumber anumber(abstick[i], file.getMinTimeBase());
						if (uQ) {
							anumber *= 4;
						}
						anumber.printTwoPart(cout);
					} else {
						cout << file[i].getAbsBeat();
					}
				} else if (sumQ) {
					if (lastmeasureline > 0) {
						stringstream ss;
						ss << fabs(file[lastmeasureline].getBeat());
						cout << ss.str();
						lastSum = ss.str();
						ss.str("");
						pickupstate++;
						lastmeasureline = -1;
					} else if (pickupstate < 1) {
						if (!file.getPickupDurationR().isNegative()) {
							if (measurecount == 0) {
								stringstream ss;
								ss << getDurationOfFirstMeasure(file).getFloat();
								cout << ss.str();
								lastSum = ss.str();
								ss.str("");
							} else {
								stringstream ss;
								ss << file.getPickupDuration();
								cout << ss.str();
								lastSum = ss.str();
								ss.str("");
							}
						} else if (file.getPickupDurationR().isZero()) {
							stringstream ss;
							ss << file.getTotalDurationR().getFloat();
							cout << ss.str();
							lastSum = ss.str();
							ss.str("");
						} else {
							stringstream ss;
							ss << file.getTotalDurationR().getFloat();
							cout << ss.str();
							lastSum = ss.str();
							ss.str("");
						}
						pickupstate++;
						lastmeasureline = -1;
					} else {
						if (appendQ || prependQ) {
							if (fillQ) {
								cout << lastSum;
							} else {
								cout << ".";
							}
						} else {
							if (nullQ) {
								if (fillQ) {
									cout << lastSum;
								} else {
									cout << ".";
								}
							} else {
								suppressreturn = 1;
							}
						}
					}
				} else if (beatQ) {
					if (Binfo[i] >= 0) {
						if (!tickQ && !rationalQ) {
							cout << Binfo[i].getFloat();
						} else if (tickQ && !rationalQ) {
							cout << (Binfo[i] * minrhy);
						} else {
							Binfo[i].printTwoPart(cout);
						}
					} else {
						if (nullQ || appendQ || prependQ) {
							cout << ".";
						} else {
							suppressreturn = 1;
						}
					}
				}

				if (prependQ) {
					cout << "\t" << file[i];
				}

				if (suppressreturn) {
					suppressreturn = 0;
				} else {
					cout << "\n";
				}

				break;
			case E_humrec_local_comment:
				if (appendQ) {
					cout << file[i] << "\t";
				}
				cout << "!";
				if (prependQ) {
					cout << "\t" << file[i];
				}
				cout << "\n";

				break;
			case E_humrec_none:
			case E_humrec_empty:
			case E_humrec_global_comment:
			case E_humrec_bibliography:
			default:
				cout << file[i] << "\n";
				break;
		}
	}

}



//////////////////////////////
//
// getDurationOfFirstMeasure --
//


RationalNumber getDurationOfFirstMeasure(HumdrumFile& file) {
	int i;
	RationalNumber output(0,1);
	for (i=0; i<file.getNumLines(); i++) {
		if (file[i].getType() == E_humrec_data_measure) {
			break;
		}
		if (file[i].getType() != E_humrec_data) {
			continue;
		}
		output += file[i].getDurationR();
	}
	return output;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
	opts.define("a|append=b");           // append analysis to input
	opts.define("p|prepend=b");          // prepend analysis to input
	opts.define("base|timebase=s:");     // rhythmic unit of one beat
	opts.define("b|beat=b");             // display beat of note in measure
	opts.define("c|abs|total-beat=b");   // cumulative, absolute beat location
	opts.define("d|dur|duration=b");     // display rhymic duration of records
	opts.define("C|meter-top=b");        // extract prevailing meter top number
	opts.define("U|meter-bottom=b");     // extract prevailing meter bottom number
	opts.define("F|fill-meter=b");       // display meter on evern data line
	opts.define("q|meter-duration=b");   // extract prevailing meter duration in quarters
	opts.define("s|sum=b");              // sum the duration of each measure
	opts.define("z|zero|zero-offset=b"); // first beat is represented by a 0
	opts.define("n|nulls|keep-nulls=b"); // doesn't remove nulls with -s option
	opts.define("B=s");                  // Do a composite beat search
	opts.define("D=s");                  // Do a composite duration search
	opts.define("R=d:-1.0");             // Limit total duration range of search
	opts.define("u|beatsize=s:4");       // beat unit
	opts.define("A|attacks|attack=i:1"); // Minimum num of note onsets for event
	opts.define("t|tick=b", "display durations as tick values");
	opts.define("f|rational=b", "display durations as rational values");
	opts.define("tpw=b", "display only ticks per whole note");
	opts.define("tpq=b", "display only ticks per quarter note");
	opts.define("debug=b");              // determine bad input line num
	opts.define("author=b");             // author of program
	opts.define("version=b");            // compilation info
	opts.define("example=b");            // example usages
	opts.define("h|help=b");             // short description
	opts.process(argc, argv);

	// handle basic options:
	if (opts.getBoolean("author")) {
		cout << "Written by Craig Stuart Sapp, "
		     << "craig@ccrma.stanford.edu, Oct 2000" << endl;
		exit(0);
	} else if (opts.getBoolean("version")) {
		cout << argv[0] << ", version: 21 April 2010" << endl;
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

	if (options.getBoolean("zero")) {
		zeroQ = 1;
	}
	appendQ   = opts.getBoolean("append");
	prependQ  = opts.getBoolean("prepend");
	durQ      = opts.getBoolean("duration");
	absQ      = opts.getBoolean("total-beat");
	beatQ     = opts.getBoolean("beat");
	sumQ      = opts.getBoolean("sum");
	metertopQ = opts.getBoolean("meter-top");
	meterbotQ = opts.getBoolean("meter-bottom");
	meterdurQ = opts.getBoolean("meter-duration");
	nullQ     = opts.getBoolean("keep-nulls");
	tickQ     = opts.getBoolean("tick");
	rationalQ = opts.getBoolean("rational");
	tpwQ      = opts.getBoolean("tpw");
	tpqQ      = opts.getBoolean("tpq");
	fillQ     = opts.getBoolean("fill-meter");
	Rvalue    = opts.getDouble("R");
	beatbase  = opts.getString("beatsize");
	uQ        = opts.getBoolean("beatsize");
	debugQ    = opts.getBoolean("debug");

	if (rationalQ) {
		tickQ = 1;
	}

	Attack = opts.getInteger("attack");
	if (Attack < 0) {
		Attack = 0;
	}

	Bsearch.resize(0);
	Dsearch.resize(0);

	if (opts.getBoolean("B")) {
		fillSearchString(Bsearch, opts.getString("B"));
	}
	if (opts.getBoolean("D")) {
		fillSearchString(Dsearch, opts.getString("D"));
	}

	if (prependQ && appendQ) {
		prependQ = 1;
		appendQ = 0;
	}

	if (prependQ || appendQ) {
		nullQ = 1;
	}

	// display beat information if no other output option is given.
	if (!(absQ || durQ)) {
		beatQ = 1;
	}
}



//////////////////////////////
//
// fillSearchString --
//

void fillSearchString(vector<double>& searcher, const string& astring) {
	int len = astring.size();
	char* tempstr;
	tempstr = new char[len+1];
	strcpy(tempstr, astring.c_str());
	char* ptr;
	ptr = strtok(tempstr, " \t\n:;,");
	double value;

	searcher.reserve(1000);

	while(ptr != NULL) {
		if (strcmp(ptr, "*") == 0) {
			value = -1;
		} else {
			value = atof(ptr);
		}
		searcher.push_back(value);
		ptr = strtok(NULL, " \t\n:;,");
	}

	delete [] tempstr;
}


//////////////////////////////
//
// example -- example usage of the quality program
//

void example(void) {
	cout <<
	"                                                                         \n"
	"# example usage of the meter program.                                    \n"
	"# analyze a Bach chorale for meter position:                             \n"
	"     meter chor217.krn                                                   \n"
	"                                                                         \n"
	"# display the metrical location spine with original data:                \n"
	"     meter -a chor217.krn                                                \n"
	"                                                                         \n"
	<< endl;
}



//////////////////////////////
//
// usage -- gives the usage statement for the meter program
//

void usage(const char* command) {
	cout <<
	"                                                                         \n"
	"Analyzes **kern data and generates a rhythmic analysis which gives       \n"
	"the beat location of **kern data records in the measure.  Currently,     \n"
	"input spines cannot split or join.                                       \n"
	"                                                                         \n"
	"Usage: " << command << " [-a][-b base-rhythm][-s|-d][input1 [input2 ...]]\n"
	"                                                                         \n"
	"Options:                                                                 \n"
	"   -a = assemble the analysis spine with the input data.                 \n"
	"   -b = set the base rhythm for analysis to specified kern rhythm value. \n"
	"   -d = gives the duration of each kern record in beat measurements.     \n"
	"   -s = sum the beat count in each measure.                              \n"
	"   --options = list of all options, aliases and default values           \n"
	"                                                                         \n"
	<< endl;
}



// md5sum: 3f05fc40ec75224a766159a6a217ce7b beat.cpp [20170605]
