//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu May 12 20:08:36 PDT 2016
// Last Modified: Thu May 12 20:08:40 PDT 2016
// Filename:      ...museinfo/examples/all/rnn-input.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/rnn-input.cpp
// Syntax:        C++; museinfo
//
// Description:   Generate RNN input data from Humdum **kern data.
//

#include "humdrum.h"
#include <vector>
#include <stdlib.h>
#include "PerlRegularExpression.h"

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const char* command);
void      processFile          (HumdrumFile& infile);
void      printAnalysis        (vector<vector<vector<int> > >& data);
void      storeNote            (vector<vector<int> >& pcs, int startrow, int endrow, int pc);

// global variables
Options   options;             // database for command-line arguments
int       debugQ = 0;          // used with --debug option

///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);
   HumdrumFile infile;

   while (streamer.read(infile)) {
      processFile(infile);
      break; // only allowing one input at the moment
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

void processFile(HumdrumFile& infile) {
   infile.analyzeRhythm("4");
   vector<int> ktracks; 
   infile.getKernTracks(ktracks);
   int kcount = (int)ktracks.size();
   int maxtrack = infile.getMaxTracks();
   vector<int> rtracks;
   rtracks.resize(maxtrack+1);
   fill(rtracks.begin(), rtracks.end(), -1);
   int i, j, k;
   for (i=0; i<(int)ktracks.size(); i++) {
      rtracks[ktracks[i]] = i;
   }
   RationalNumber linedur(1, 2);
   RationalNumber totaldur = infile.getTotalDurationR();
   RationalNumber rframes = totaldur / linedur;
   int frames;
   if (rframes.getDenominator() == 1) {
      frames = rframes.getNumerator();
   } else {
      frames = int(rframes.getFloat()) + 1;
   }
   vector<vector<vector<int> > > pcs;
   pcs.resize(kcount);
   for (i=0; i<(int)pcs.size(); i++) {
      pcs[i].resize(frames);
      for (j=0; j<(int)pcs[i].size(); j++) {
         pcs[i][j].resize(12);
         fill (pcs[i][j].begin(), pcs[i][j].end(), 0);
      }
   }

   vector<string> tokens;
   int pc;
   int voice;
   int track;
   RationalNumber duration;
   RationalNumber tempr;
   int startrow;
   int endrow;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (infile[i].isNullToken(j)) {
            continue;
         }
         track = infile[i].getPrimaryTrack(j);
         voice = rtracks[track];
         infile[i].getTokens(tokens, j);
         tempr = infile[i].getAbsBeatR() / linedur;
         startrow = (int)(tempr.getFloat());
         for (k=0; k<(int)tokens.size(); k++) {
            if (tokens[k].find("r") != string::npos) {
               continue;
            }
            pc = Convert::kernToMidiNoteNumber(tokens[k].c_str());
            if (pc <= 0) {
               continue;
            }
            pc = pc % 12;
            duration = Convert::kernToDurationR(tokens[k].c_str());
            tempr = (duration + infile[i].getAbsBeat()) / linedur;
            if (tempr.getDenominator() == 1) {
               endrow = int(tempr.getFloat());
            } else {
               endrow = int(tempr.getFloat()) + 1;
            }
            storeNote(pcs[voice], startrow, endrow, pc);
            if (debugQ) {
				   cout << "token=" << tokens[k] << "\tvoice="<<voice << "\tpc="<< pc 
				        << "\tstart=" << startrow << "\tend=" << endrow << endl;
				}
         }
      }
   }
   

   printAnalysis(pcs);
}



//////////////////////////////
//
// storeNote --
//

void storeNote(vector<vector<int> >& pcs, int startrow, int endrow, int pc) {
   if (startrow == endrow) {
      endrow++;
   }
   for (int i=startrow; i<endrow; i++) {
      pcs[i][pc] = 1;
   }
}



//////////////////////////////
//
// printAnalysis --
//

void printAnalysis(vector<vector<vector<int> > >& data) {
   int i, j, k;
   for (j=0; j<(int)data[0].size(); j++) {
      for (i=0; i<(int)data.size(); i++) {
         for (k=0; k<(int)data[i][j].size(); k++) {
            cout << data[i][j][k];
            if (!((i==(int)data.size()-1) && (k==11))) {
               if (k < 11) {
                  cout << ",";
               } else {
                  cout << ", ";
               }
            }
         }
      }
      cout << endl;
   }
}




//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
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
      usage(opts.getCommand().data());
      exit(0);
   } else if (opts.getBoolean("example")) {
      example();
      exit(0);
   }

   debugQ = opts.getBoolean("debug");

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


// md5sum: d334763f49d6209e549af51cc1a72ce6 rnn-input.cpp [20170605]
