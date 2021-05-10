//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Oct  9 18:44:49 PDT 2013
// Last Modified: Mon May 10 11:36:15 PDT 2021 added -w option
// Filename:      ...museinfo/examples/all/voicecount.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/voicecount.cpp
// Syntax:        C++; museinfo
//
// Description:   Count the active number of voices/parts sounding at any
//                given moment in the score.
//

#include "humdrum.h"
#include "PerlRegularExpression.h"

#include <stdlib.h>

#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const char* command);
void      processFile          (HumdrumFile& infile, const string& filename);
int       getVoiceCount        (HumdrumFile& infile, int line);
int       getNoteCount         (HumdrumFile& infile, int line);
void      printExclusiveInterpretation(void);
int       doAnalysis           (HumdrumFile& infile, int line);
int       isAttack             (const string& token);
void      printMeasureData     (vector<int>& analysis, HumdrumFile& infile,
                                int line);
void      printSummary         (vector<double>& Summary);
int       isValidFile          (HumdrumFile& infile);
void      waterFill            (vector<int>& analysis, HumdrumFile& infile, double water);
void      checkForFill         (vector<int>& analysis, int start, vector<int>& index, HumdrumFile& infile, double duration);

// global variables
Options   options;             // database for command-line arguments
int       debugQ       = 0;    // used with --debug option
int       appendQ      = 0;    // used with -a option
int       prependQ     = 0;    // used with -p option
int       spineQ       = 0;    // used with -s option
int       pcQ          = 0;    // used with --pc option
int       twelveQ      = 0;    // used with --12 option
int       fortyQ       = 0;    // used with --40 option
int       sevenQ       = 0;    // used with -7 option
int       attackQ      = 0;    // used with --attack option
int       trackQ       = 0;    // used with --track option
int       allQ         = 0;    // used with --all option
int       measureQ     = 0;    // used with -m option
int       mdurQ        = 0;    // used with -M option
int       noteQ        = 0;    // used with -n option
int       kernQ        = 0;    // used with -k option
int       nograceQ     = 0;    // used with -G option
int       validQ       = 0;    // used with -v option
int       uniqueQ      = 0;    // used with -u option
int       summaryQ     = 0;    // used with --summary option
double    water        = 0.0;  // used with --water-fill
vector<double> Summary;        // used with --summary option
int       SEGMENTS     = 0;    // used if there are more than one segment.

///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);
   HumdrumFile infile;

   if (summaryQ) {
      Summary.clear();
      Summary.reserve(1000);
   }

   while (streamer.read(infile)) {
      if (!streamer.eof()) {
         // if there are multiple segments, store a segement marker
         // for each segment.  Do not store if only a single segment,
         // unless --segement option is given.
         SEGMENTS = 1;
      }
      processFile(infile, infile.getFileName());
   }

   if (summaryQ) {
      printSummary(Summary);
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

void processFile(HumdrumFile& infile, const string& filename) {
   int i;
   if (SEGMENTS && !summaryQ) {
      cout << "!!!!SEGMENT: " << infile.getFileName() << endl;
   }

   if (debugQ) {
      cout << "!! file: " << infile.getFileName() << endl;
   }
   if (validQ && !isValidFile(infile)) {
      return;
   }

   if (kernQ) {
      vector<int> ktracks;
      infile.getTracksByExInterp(ktracks, "**kern");
      cout << ktracks.size() << endl;
      return;
   }

   if (mdurQ || summaryQ || nograceQ) {
      infile.analyzeRhythm("4");
   }

   vector<int> analysis(infile.getNumLines(), 0);

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      analysis[i] = doAnalysis(infile, i);
   }

	if (water > 0.0) {
		waterFill(analysis, infile, water);
	}

   if (summaryQ) {
      return;
   }

   // print analysis:

   int firstdata = 1;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (appendQ) { cout << infile[i]; }
      if (infile[i].isData()) {
         if (appendQ)  { cout << '\t'; }
         if (measureQ && firstdata) {
            printMeasureData(analysis, infile, i);
            firstdata = 0;
         } else if (measureQ) {
            cout << ".";
         } else {
            if (nograceQ && (infile[i].getDuration() == 0)) {
               cout << ".";
            } else {
               if (analysis[i] < 0) {
                  cout << -analysis[i] << "f";
               } else {
                  cout << analysis[i];
               }
            }
         }
         if (prependQ) { cout << '\t'; }
         if (appendQ)  { cout << '\n'; }
      } else if (infile[i].isInterpretation()) {
         if (appendQ)  { cout << '\t'; }
         if (strcmp(infile[i][0], "*-") == 0) {
            cout << "*-";
         } else if (strncmp(infile[i][0], "**", 2) == 0) {
            printExclusiveInterpretation();
         } else {
            cout << "*";
         }
         if (prependQ) { cout << '\t'; }
         if (appendQ)  { cout << '\n'; }
      } else if (infile[i].isBarline()) {
         if (pre.search(infile[i][0], "\\d")) {
            firstdata = 1;
         }
         if (appendQ)  { cout << '\t'; }
         cout << infile[i][0];
         if (prependQ) { cout << '\t'; }
         if (appendQ)  { cout << '\n'; }
      } else if (infile[i].isLocalComment()) {
         if (appendQ)  { cout << '\t'; }
         cout << "!";
         if (prependQ) { cout << '\t'; }
         if (appendQ)  { cout << '\n'; }
      } else {
         if (!(appendQ || prependQ)) { cout << infile[i]; }
         if (appendQ)  { cout << '\n'; }
      }
      if (prependQ) { cout << infile[i]; }
      if (!appendQ) { cout << '\n'; }
   }
}



//////////////////////////////
//
// isValidFile --
//

int isValidFile(HumdrumFile& infile) {
   PerlRegularExpression pre;
   vector<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");
   int actual = (int)ktracks.size();
   int i;
   int target = -1;
   for (i=0; i<infile.getNumLines(); i++) {
      if (pre.search(infile[i][0], "!!+voices\\s*:\\s*(\\d+)")) {
         target = atoi(pre.getSubmatch(1));
         break;
      }
   }

   if (target < 0) {
      // no !!voices: line, so presum valid
      return 1;
   }

   if (target == actual) {
      return 1;
   } else {
      return 0;
   }

}



//////////////////////////////
//
// printMeasureData -- Sum the analysis data from the current line
//     until the next measure which contains a measure number.
//

void printMeasureData(vector<int>& analysis, HumdrumFile& infile, int line) {
   int i;
   PerlRegularExpression pre;
   RationalNumber startdur;
   RationalNumber enddur;

   if (mdurQ) {
      startdur = infile[line].getAbsBeatR();
      enddur   = infile[infile.getNumLines()-1].getAbsBeatR();
   }
   int sum = 0;
   for (i=line; i<infile.getNumLines(); i++) {
      if (infile[i].isMeasure()) {
         if (pre.search(infile[i][0], "\\d")) {
            if (mdurQ) {
               enddur = infile[i].getAbsBeatR();
            }
            break;
         }
      }
      if (!infile[i].isData()) {
         continue;
      }
      if (nograceQ && (infile[i].getDuration() == 0)) {
         continue;
      }
      sum += abs(analysis[i]);
   }
   if (mdurQ) {
      RationalNumber duration;
      duration = enddur - startdur;
      duration.printTwoPart(cout);
      cout << ":";
   }
   cout << sum;
}



//////////////////////////////
//
// doAnalysis --
//

int doAnalysis(HumdrumFile& infile, int line) {
   int value = 0.0;

   if (nograceQ && (infile[line].getDuration() == 0.0)) {
      return -1;
   }
   if (uniqueQ || noteQ || twelveQ || fortyQ || sevenQ) {
      value = getNoteCount(infile, line);
   } else {
      value = getVoiceCount(infile, line);
   }
   if (summaryQ) {
      Summary[value] += infile.getDuration(line);
   }
   return value;
}



//////////////////////////////
//
// printExclusiveInterpretation --
//

void printExclusiveInterpretation(void) {
   if (noteQ) {
      if (allQ) {
         cout << "**p#";
      } else {
         cout << "**up#";
      }
   } else if (twelveQ) {
      if (pcQ) {
         cout << "**12pc#";
      } else if (allQ) {
         cout << "**12p#";
      } else {
         cout << "**12up#";
      }
   } else if (fortyQ) {
      if (pcQ) {
         cout << "**40pc#";
      } else if (allQ) {
         cout << "**40p#";
      } else {
         cout << "**40up#";
      }
   } else if (sevenQ) {
      if (pcQ) {
         cout << "**7pc#";
      } else if (allQ) {
         cout << "**7p#";
      } else {
         cout << "**7up#";
      }
   } else {
      cout << "**v#";
   }
}



//////////////////////////////
//
// getVoiceCount --
//

int getVoiceCount(HumdrumFile& infile, int line) {
   int i, j, k;
   int ii, jj;
   int count = 0;
   int track;
   int acount;
   vector<string> tokens;
   vector<int> tracks;
   tracks.reserve(infile.getMaxTracks()+1);
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         continue;
      }
      ii = line;
      jj = j;
      if (attackQ && (strcmp(infile[line][j], ".") == 0)) {
         continue;
      }
      if (infile[line].isNullToken(j)) {
        ii = infile[line].getDotLine(j);
        if (ii < 0) { continue; }  // . at start of data spine
        jj = infile[line].getDotSpine(j);
        if (jj < 0) { continue; }  // . at start of data spine
      }
      if (strchr(infile[ii][jj], 'r') != NULL) {
         continue;
      }
      if (attackQ) {
         infile[ii].getTokens(tokens, jj);
         acount = 0;
         for (k=0; k<(int)tokens.size(); k++) {
            if (isAttack(tokens[k])) {
               acount++;
            }
         }
         if (acount == 0) {
            continue;
         }
      }
      if (trackQ) {
         track = infile[ii].getPrimaryTrack(jj);
         tracks[track]++;
      } else {
         count++;
      }
   }

   if (trackQ) {
      count = 0;
      for (i=1; i<(int)tracks.size(); i++) {
         if (tracks[i]) {
            count++;
         }
      }
   }
   return count;
}



//////////////////////////////
//
// getNoteCount -- Get the number of pitch (classes) currently sounding
//

int getNoteCount(HumdrumFile& infile, int line) {
   vector<int> states(1000, 0);
   vector<string> tokens;
   int i, j, k;
   int ii, jj;
   int count = 0;
   int notenum;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         continue;
      }
      ii = line;
      jj = j;
      if (infile[line].isNullToken(j)) {
        if (attackQ) {
           // Null tokens never contain note attacks
           continue;
        }
        ii = infile[line].getDotLine(j);
        if (ii < 0) { continue; }  // . at start of data spine
        jj = infile[line].getDotSpine(j);
        if (jj < 0) { continue; }  // . at start of data spine
      }
      if (strchr(infile[ii][jj], 'r') != NULL) {
         continue;
      }
      infile[ii].getTokens(tokens, jj);
      for (k=0; k<(int)tokens.size(); k++) {
         if (attackQ && !isAttack(tokens[k])) {
            continue;
         }
         notenum = -1;
         if (twelveQ) {
            notenum = Convert::kernToMidiNoteNumber(tokens[k]);
            if (pcQ) { notenum = notenum % 12; }
         } else if (sevenQ) {
            notenum = Convert::kernToDiatonicPitch(tokens[k]);
            if (pcQ) { notenum = notenum % 7; }
         } else if (noteQ && !uniqueQ) {
            count++;
         } else {
            // assume fortyQ:
            notenum = Convert::kernToBase40(tokens[k]);
            if (pcQ) { notenum = notenum % 40; }
         }
         if (notenum < 0) {
            continue;
         }
         states[notenum]++;
      }
   }

   if (pcQ) {
      int pcount = 0;
      for (i=0; i<(int)states.size(); i++) {
         if (states[i]) {
            pcount++;
         }
         if (pcQ && i>40) {
            break;
         }
      }
      return pcount;
   } else if (uniqueQ) {
      int unique = 0;
      for (i=0; i<(int)states.size(); i++) {
         if (states[i]) {
            unique++;
         }
         if (pcQ && i>40) {
            break;
         }
      }
      return unique;
   } else if (allQ || noteQ) {
      return count;
   }

   // shouldn't get here...
   return -1;
}



//////////////////////////////
//
// isAttack -- returns true if no r, _, or ] character in string.
//

int isAttack(const string& token) {
   if (token == ".") {
      return 0;
   }
   if (token.find('r') != std::string::npos) {
      return 0;
   }
   if (token.find('_') != std::string::npos) {
      return 0;
   }
   if (token.find(']') != std::string::npos) {
      return 0;
   }
   return 1;
}



//////////////////////////////
//
// printSummary -- Print the duration of each voice count and relative percent.
//

void printSummary(vector<double>& Summary) {
   double sum     = 0.0;
   double percent = 0.0;
   double weight  = 0.0;
   int maxx       = 0;
   int minn       = 1000;
   for (int i=0; i<(int)Summary.size(); i++) {
      if (Summary[i] != 0.0) {
         sum += Summary[i];
         if (i > maxx) { maxx = i; }
         if (i < minn) { minn = i; }
         weight += (i+1) * Summary[i];
      }
   }
   if (sum == 0.0) {
      return;
   }
   cout << "**dur\t**pcent\t";
   if (noteQ || twelveQ || fortyQ || sevenQ) {
      if (pcQ) {
         cout << "**";
         if (sevenQ)       { cout <<  "7"; }
         else if (twelveQ) { cout << "12"; }
         else              { cout << "40"; }
         cout << "pc#";
      } else {
         cout << "**";
         if (sevenQ)       { cout <<  "7"; }
         else if (twelveQ) { cout << "12"; }
         else              { cout << "40"; }
         cout << "p#";
      }
   } else {
      cout << "**v#";
   }
   cout << '\n';
   for (int i=minn; i<=maxx; i++) {
      if (Summary[i] == 0.0) {
         continue;
      }
      percent = Summary[i] / sum * 100.0;
      percent = int(percent*100.0+0.5)/100.0;
      cout << Summary[i] << '\t' << percent << '\t' << i << '\n';
   }
   cout << "*-\t*-\t*-\n";
   cout << "!!total-duration: " << sum << endl;
   if (pcQ) {
      cout << "!!average-pcs: " << (weight/sum-1) << endl;
   } else {
      cout << "!!average-voices: " << (weight/sum-1) << endl;
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|append=b",         "append analysis data to input");
   opts.define("u|uniq|unique=b",    "count unique number of notes");
   opts.define("p|prepend=b",        "prepend analysis data to input");
   opts.define("y|summary=b",        "list voice counts by durations");
   opts.define("c|pc|pitch-class=b", "pitch classes only; ignore octaves");
   opts.define("12|twelve-tone=b",   "count of twelvetone pitch classes");
   opts.define("40|base-40=b",       "count of base-40 pitches ");
   opts.define("G|no-grace-notes=b", "do not process lines with grace notes");
   opts.define("m|measure|b|bar=b",  "sum results for measure");
   opts.define("M|measure-duration=b", "list duration of measure");
   opts.define("7|diatonic=b",       "count of diatonic pitches ");
   opts.define("x|attack=b",         "only count note attacks");
   opts.define("n|notes|note=b",     "only count note attacks");
   opts.define("segment|segments=b","display segment marker for single input");
   opts.define("k|kern=b",           "count number of **kern spines ");
   opts.define("v|valid=b",          "only consider complete part segments");
   opts.define("w|water-fill=d:1.0", "only consider complete part segments");
   opts.define("s|spines|spine|tracks|track=b", "only count note attacks");

   opts.define("debug=b");              // determine bad input line num
   opts.define("author=b");             // author of program
   opts.define("version=b");            // compilation info
   opts.define("example=b");            // example usages
   opts.define("help=b");               // short description
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, October 2013" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 24 October 2013" << endl;
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

   debugQ     =  opts.getBoolean("debug");
   trackQ     =  opts.getBoolean("spine");
   appendQ    =  opts.getBoolean("append");
   prependQ   =  opts.getBoolean("prepend");
   pcQ        =  opts.getBoolean("pc");
   noteQ      =  opts.getBoolean("notes");
   twelveQ    =  opts.getBoolean("12");
   fortyQ     =  opts.getBoolean("40");
   nograceQ   =  opts.getBoolean("no-grace-notes");
   sevenQ     =  opts.getBoolean("7");
   attackQ    =  opts.getBoolean("attack");
   uniqueQ    =  opts.getBoolean("unique");
   measureQ   =  opts.getBoolean("measure");
   mdurQ      =  opts.getBoolean("measure-duration");
   kernQ      =  opts.getBoolean("kern");
   validQ     =  opts.getBoolean("valid");
   summaryQ   =  opts.getBoolean("summary");
   SEGMENTS   =  opts.getBoolean("segment");
	if (opts.getBoolean("water-fill")) {
		water = opts.getDouble("water-fill");
	}

   if (noteQ) {
      allQ = 1;
   }

   if (pcQ && !(twelveQ || fortyQ || sevenQ)) {
      // use base-12 as default if --pc option given
      twelveQ = 1;
   }

   if (!(pcQ || uniqueQ)) {
      // if --12 --14 or -7 is given but no -u
      // or --pc option given, then turn on noteQ
      // if one of the pitch options is given.
      if (twelveQ || fortyQ || sevenQ) {
         noteQ = 1;
      }
   }

   if (uniqueQ) {
      // if unique, then set to fortyQ if none of the
      // pitch systems are given.
      if (!(twelveQ || fortyQ || sevenQ)) {
         fortyQ = 1;
      }
   }

   if (appendQ) {
      // mutually exclusive options
      prependQ = 0;
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

void usage(const char* command) {
   cout <<
   "                                                                         \n"
   << endl;
}



//////////////////////////////
//
// waterFill --
//

void waterFill(vector<int>& analysis, HumdrumFile& infile, double water) {
	infile.analyzeRhythm();
	vector<int> index;
	index.reserve(analysis.size());
	for (int i=0; i<infile.getNumLines(); i++) {
		if (infile[i].isData()) {
			index.push_back(i);
		}
	}


	for (int i=0; i<(int)index.size() - 1; i++) {
		if (analysis.at(index.at(i)) > analysis.at(index.at(i+1))) {
			checkForFill(analysis, i, index, infile, water);
		}
	}
}



//////////////////////////////
//
// checkForFill --
//

void checkForFill(vector<int>& analysis, int starti, vector<int>& index, HumdrumFile& infile, double water) {
	if (starti >= (int)index.size() - 1) {
		return;
	}
	double starttime = infile[index.at(starti+1)].getAbsBeat();
	int target = analysis.at(index.at(starti));
	int fill = -1;
	for (int i=starti+1; i<(int)analysis.size(); i++) {
			double endtime = infile[index[i]].getAbsBeat();
			double duration = (endtime - starttime) / 4.0; // converting to whole note units
			int vcount = analysis.at(index.at(i));
			if (vcount >= target) {
				fill = i;
				break;
			}
			if (duration >= water) {
				break;
			}
	}
	if (fill < 0) {
		return;
	}

	for (int i=starti+1; i<=fill; i++) {
		if (analysis.at(index.at(i)) < target) {
			analysis.at(index.at(i)) = -target;
		}
	}
}


