//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Sep  9 21:30:46 PDT 2004
// Last Modified: Thu Sep  9 21:30:48 PDT 2004
// Filename:      ...sig/examples/all/barnum.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/barnum.cpp
// Syntax:        C++; museinfo
//
// Description:   Number, renumber or remove measure numbers from Humdrum files.
//


#include <string.h>

#include <cctype>
#include <vector>
#include <string>

#include "humdrum.h"

using namespace std;
   

// function declarations
void      checkOptions            (Options& opts, int argc, char* argv[]);
void      example                 (void);
void      usage                   (const string& command);
void      processFile             (HumdrumFile& file);
void      removeBarNumbers        (HumdrumFile& infile);
void      printWithoutBarNumbers  (HumdrumRecord& humrecord);
void      printWithBarNumbers     (HumdrumRecord& humrecord, int measurenum);
void      printSingleBarNumber    (const string& astring, int measurenum);
int       getEndingBarline        (HumdrumFile& infile);

// global variables
Options   options;            // database for command-line arguments
int       removeQ  = 0;       // used with -r option
int       startnum = 1;       // used with -s option
int       allQ     = 0;       // used with -a option
int       debugQ   = 0;       // used with --debug option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFile infile, outfile;

   // process the command-line options
   checkOptions(options, argc, argv);

   // if no command-line arguments read data file from standard input
   int numinputs = options.getArgCount();
   if (numinputs < 1) {
      infile.read(cin);
   } else {
      infile.read(options.getArg(1));
   }
   
   if (removeQ) {
      removeBarNumbers(infile);
   } else{
      processFile(infile);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


///////////////////////////////
//
// removeBarNumbers -- You guessed it.
//

void removeBarNumbers(HumdrumFile& infile) {
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() != E_humrec_data_measure) {
         cout << infile[i] << "\n";
         continue;
      }
      printWithoutBarNumbers(infile[i]);
   }
}



//////////////////////////////
//
// printWithoutBarNumbers --
//

void printWithoutBarNumbers(HumdrumRecord& humrecord) {
   int i;
   int j;
   int length;

   for (i=0; i<humrecord.getFieldCount(); i++) {
      if (humrecord[i][0] != '=') {
         cout << humrecord[i];
      } else {
         length = strlen(humrecord[i]);
         for (j=0; j<length; j++) {
            if (!std::isdigit(humrecord[i][j])) {
               cout << humrecord[i][j];
            }
         }
      }
      if (i < humrecord.getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";
}



//////////////////////////////
//
// getEndingBarline -- Return the index of the last barline,
//      returning -1 if none.  Ending barline is defined as the
//      last barline after all data records.
//

int getEndingBarline(HumdrumFile& infile) {
   int i;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (infile[i].isData()) {
         return -1;
      } else if (infile[i].isMeasure()) {
         return i;
      }
   }

   return -1; 
}



//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
   infile.analyzeRhythm("4");

   vector<int>    measureline;   // line number in the file where measure occur
   vector<double> measurebeats;  // duration of measure
   vector<double> timesigbeats;  // duration according to timesignature
   vector<int>    control;       // control = numbered measure
   vector<int>    measurenums;   // output measure numbers

   measureline.reserve(infile.getNumLines());
   measurebeats.reserve(infile.getNumLines());
   timesigbeats.reserve(infile.getNumLines());

   int i, j;
   double timesigdur = 0.0;
   double timetop = 4;
   double timebot = 1;
   double value   = 1;
   double lastvalue = 1;
   for (i=0; i<infile.getNumLines(); i++) {
      if (debugQ) {
         cout << "LINE " << i+1 << "\t" << infile[i] << endl;
      }
      if (infile[i].getType() == E_humrec_interpretation) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if ((strncmp(infile[i][j], "*M", 2) == 0) 
                  && (strchr(infile[i][j], '/') != NULL)) {
               timetop = Convert::kernTimeSignatureTop(infile[i][j]);
               timebot = Convert::kernTimeSignatureBottomToDuration(infile[i][j]);
               timesigdur = timetop * timebot;
               // fix last timesigbeats value
               if (timesigbeats.size() > 0) {
                  timesigbeats[(int)timesigbeats.size()-1] = timesigdur;
                  measurebeats[(int)measurebeats.size()-1] = lastvalue * timebot;
               }
            }
         }
      } else if (infile[i].getType() == E_humrec_data_measure) {
         measureline.push_back(i);
         lastvalue = infile[i].getBeat();
         // shouldn't use timebot (now analyzing rhythm by "4")
         // value = lastvalue * timebot;
         value = lastvalue;
         measurebeats.push_back(value);
         timesigbeats.push_back(timesigdur);
      }
   }

   if (debugQ) {
      cout << "measure beats / timesig beats" << endl;
      for (i=0; i<(int)measurebeats.size(); i++) {
         cout << measurebeats[i] << "\t" << timesigbeats[i] << endl;
      }
   }

   if (measurebeats.size() == 0) {
      // no barlines, nothing to do...
      cout << infile;
      return;
   }

   // Identify controlling/non-controlling barlines
   // at each measure line determine one of three cases:
   // 
   // (1) all ok -- the summation of durations in the measure
   //     matches the current time sign
   // (2) a partial measure -- the measure durations do not
   //     add up to the time signature, but the measure is
   //     at the start/end of a musical section such as the
   //     beginning of a piece, end of a piece, or between
   //     repeat bar dividing a full measure.
   // (3) the sum of the durations does not match the 
   //     time signature because the durations are incorrectly
   //     given.
   //

   control.resize(measureline.size());
   measurenums.resize(measureline.size());
	std::fill(control.begin(), control.end(), -1);
	std::fill(measurenums.begin(), measurenums.end(), -1);

   // If the time signature and the number of beats in a measure
   // agree, then the bar is worth numbering:
   for (i=0; i<(int)control.size(); i++) {
      if (measurebeats[i] == timesigbeats[i]) {
         control[i] = 1;
      }
   }

   // Determine first bar (which is marked with a negative value
   // if there is a pickup bar)
   if (measurebeats[0] < 0) {
      if (-measurebeats[0] == timesigbeats[0]) {
         control[0] = 1;
      }
   }

   // Check for intermediate barlines which split one measure
   for (i=0; i<(int)control.size()-2; i++) {
      if ((control[i] == 1) || (control[i+1] == 1)) {
         continue;
      }
      if (timesigbeats[i] != timesigbeats[i+1]) {
         continue;
      }
      if ((measurebeats[i]+measurebeats[i+1]) == timesigbeats[i]) {
         // found a barline which splits a complete measure
         // into two pieces.
         control[i] = 1;
         control[i+1] = 0;
         i++;
      }
   }

   // if two (or more) non-controlling bars occur in a row, then
   // make them controlling:
   //for (i=0; i<control.size()-1; i++) {
   //   if ((control[i] < 1) && (control[i+1] < 1)) {
   //      while ((i < control.size()) && (control[i] < 1)) {
   //         control[i++] = 1;
   //      }
   //   }
   //}

   for (i=0; i<(int)control.size()-1; i++) {
      if ((control[i] == 0) && (control[i+1] < 0)) {
         control[i+1] = 1;
      }
   }

   // if a measure contains no beats, then it is not a controlling barline
   for (i=0; i<(int)control.size(); i++) {
      if (measurebeats[i] == 0) {
         control[i] = 0;
      }
   }

   // if the first bar is not a pickup measure, but there is no
   // starting measure, then subtract one from the starting barline
   // count;
   int offset = 0;
   int dataq = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         dataq = 1;
         continue;
      }
      if (infile[i].getType() == E_humrec_data_measure) {
         if ((measurebeats[0] > 0) && dataq) {
            offset = 1;
         }
         break;
      }
   }

   // if the last bar is incomplete, but the bar before it
   // is not incomplete, then allow barline on last measure,
   // excluding any ending barlines with no data after them.
   for (i=(int)control.size()-1; i>=0; i--) {
      if (control[i] == 0) {
         continue;
      }
      if ((control[i] < 0) && (i > 0) && (control[i-1] > 0)) {
         control[i] = 1;
      }
      break;
   }

   if (allQ) {
		std::fill(control.begin(), control.end(), 1);
      offset = 0;
   }

   // if there is no time data, just label each barline
   // as a new measure. 
   if (infile[infile.getNumLines()-1].getAbsBeat() == 0.0) {
      for (i=0; i<(int)control.size(); i++) {
         control[i] = 1;
      }
      // don't mark the last barline if there is no data
      // line after it.
      for (i=infile.getNumLines()-1; i>=0; i--) {
         if (infile[i].isData()) {
            break;
         }
         if (infile[i].isBarline()) {
            control.back() = -1;
            break;
         }
      }
   }

   // assign the measure numbers;
   int mnum = startnum + offset;
   for (i=0; i<(int)measurenums.size(); i++) {
      if (control[i] == 1) {
         measurenums[i] = mnum++;
      }
   }

   if (debugQ) {
      cout << "cont\tnum\tbeats" << endl;
      for (i=0; i<(int)control.size(); i++) {
         cout << control[i] << "\t" << measurenums[i] << "\t"
              << measurebeats[i] << "\t" << timesigbeats[i] << endl;
      }
   }

   int endingbarline = getEndingBarline(infile);

   // ready to print the new barline numbers
   int mindex = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() != E_humrec_data_measure) {
         cout << infile[i] << "\n";
         continue;
      }

      if (endingbarline == i) {
         printWithoutBarNumbers(infile[i]);
      } else if (measurenums[mindex] < 0) {
         printWithoutBarNumbers(infile[i]);
      } else {
         printWithBarNumbers(infile[i], measurenums[mindex]);
      }

      mindex++;
   }

}



//////////////////////////////
//
// printWithBarNumbers --
//

void printWithBarNumbers(HumdrumRecord& humrecord, int measurenum) {
   int i;
   for (i=0; i<humrecord.getFieldCount(); i++) {
       printSingleBarNumber(humrecord[i], measurenum);
       if (i < humrecord.getFieldCount() -1) {
          cout << "\t";
       }
   }
   cout << "\n";
}



//////////////////////////////
//
// printSingleBarNumber --
//

void printSingleBarNumber(const string& astring, int measurenum) {
   for (int i=0; i<(int)astring.size(); i++) {
      if ((astring[i] == '=') && (astring[i+1] != '=')) {
         cout << astring[i] << measurenum;
      } else if (!isdigit(astring[i])) {
         cout << astring[i];
      }
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

  
void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("r|remove=b", "Remove barlines from the file");             
   opts.define("s|start=i:1", "starting barline number");             
   opts.define("a|all=b",     "print numbers on all barlines");

   opts.define("debug=b");                // print debug info
   opts.define("author=b");               // author of program
   opts.define("version=b");              // compilation info
   opts.define("example=b");              // example usages
   opts.define("h|help=b");               // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Sep 2004" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 10 Sep 2004" << endl;
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

   removeQ  = opts.getBoolean("remove");
   startnum = opts.getInteger("start");
   debugQ   = opts.getBoolean("debug");
   allQ     = opts.getBoolean("all");

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



