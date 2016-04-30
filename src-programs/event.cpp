//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Apr 28 16:41:23 PDT 2016
// Last Modified: Thu Apr 28 16:41:25 PDT 2016
// Filename:      ...museinfo/examples/all/event.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/event.cpp
// Syntax:        C++; museinfo
//
// Description:   Enumerates note attack events in the file.
//

#include "humdrum.h"
#include <vector>
#include <algorithm>
#include <string.h>
#include "PerlRegularExpression.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const char* command);
void      processFile          (vector<int>& eventnums, HumdrumFile& infile);
void      printFile            (vector<int>& eventnums, HumdrumFile& infile);
void      printEvent           (vector<int>& events, HumdrumFile& infile, int index);
void      reduceKern           (HumdrumFile& infile);
string    getPitch             (const string& kern, int sustainQ);
void      printEmpty           (const string& str);
void      printAll             (const string& str);
void      printEventEI         (void);
double    getDuration          (vector<int> values, HumdrumFile& infile, int line);

// global variables
Options   options;             // database for command-line arguments
int       eventQ     = 0;      // used with -e option
int       appendQ    = 0;      // used with -a option
int       prependQ   = 0;      // used with -p option
int       reduceQ    = 0;      // used with -r option
int       meterQ     = 0;      // used with -m option
int       durationQ  = 0;      // used with -d option
int       attackQ    = 0;      // used with -t option
int       absoluteQ  = 0;      // used with -A option
int       onlyReduceQ = 0;

///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);
   HumdrumFile infile;
   vector<int> eventnums;
   while (streamer.read(infile)) {
      if (durationQ || meterQ) {
         infile.analyzeRhythm();
      }
      processFile(eventnums, infile);
      printFile(eventnums, infile);
   }
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

void processFile(vector<int>& eventnums, HumdrumFile& infile) {
   int i, j, k;
   eventnums.resize(infile.getNumLines());
   fill(eventnums.begin(), eventnums.end(), -1);

   int counter = 1;

   vector<string> toks;
   int attack = 0;
   
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      attack = 0;
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         infile[i].getTokens(toks, j);
         for (k=0; k<(int)toks.size(); k++) {
            if (toks[k].find("r") != string::npos) {
               continue;
            }
            if (toks[k].find("_") != string::npos) {
               continue;
            }
            if (toks[k].find("]") != string::npos) {
               continue;
            }
            attack = 1;
         }
         if (attack) {
            eventnums[i] = counter++;
            break;
         }
      }
      
   }
}



//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

void printFile(vector<int>& eventnums, HumdrumFile& infile) {
   if (reduceQ) {
      reduceKern(infile);
   }

   if (onlyReduceQ) {
      cout << infile;
      return;
   }

   for (int i=0; i<(int)infile.getNumLines(); i++) {
      if (!infile[i].hasSpines()) {
         cout << infile[i] << endl;
         continue;
      }
 

      if (infile[i].isData()) {
         if (prependQ) {
            printEvent(eventnums, infile, i);
            cout << "\t" << infile[i] << endl;
         } else if (appendQ) {
            cout << infile[i] << "\t";
            printEvent(eventnums, infile, i);
	 			cout << endl;
         } else {
            printEvent(eventnums, infile, i);
            cout << endl;
         }
      }

      if (infile[i].isLocalComment()) {
         if (prependQ) {
            printEmpty("!\t");
            cout << infile[i] << endl;
         } else if (appendQ) {
            cout << infile[i];
            printEmpty("\t!");
            cout << endl;
         } else {
            printAll("!");
            cout << endl;
         }
      }

      if (infile[i].isBarline()) {
         string item;
         if (prependQ) {
            item = infile[i][0];
            item += "\t";
            printEmpty(item);
            cout << infile[i] << endl;
         } else if (appendQ) {
            cout << infile[i];
            string item = "\t";
            item += infile[i][0];
            printEmpty(item);
            cout << endl;
         } else {
            printAll(infile[i][0]);
            cout << endl;
         }
      }

      if (infile[i].isInterpretation()) {
         if (strncmp(infile[i][0], "**", 2) == 0) {
            if (prependQ) {
               printEventEI();
               cout << "\t" << infile[i] << endl;
            } else if (appendQ) {
               cout << infile[i] << "\t";
               printEventEI();
               cout << endl;
            } else {
               printEventEI();
               cout << endl;
            }
         } else if (strcmp(infile[i][0], "*-") == 0) {
            if (prependQ) {
               printEmpty("*-\t");
               cout << infile[i] << endl;
            } else if (appendQ) {
               cout << infile[i];
               printEmpty("\t*-");
               cout << endl;
            } else {
               printAll("*-");
               cout << endl;
            }
         } else {
            if (prependQ) {
               printEmpty("*\t");
               cout << infile[i] << endl;
            } else if (appendQ) {
               cout << infile[i];
               printEmpty("\t*");
               cout << endl;
            } else {
               printAll("*");
               cout << endl;
            }
         }
      }
   }
}



//////////////////////////////
//
// printEvent --
//

void printEvent(vector<int>& values, HumdrumFile& infile, int line) {
   int suppressQ = 0;
   if (values[line] < 1) {
      suppressQ = 1;
   }
   if (eventQ) {
      if (values[line] > 0) {
         cout << values[line];
      } else {
         cout << ".";
      }
   }
   if (absoluteQ) {
      cout << "\t";
      if (suppressQ) {
         cout << ".";
      } else {
         cout << infile[line].getAbsBeat();
      }
   }
   if (meterQ) {
      cout << "\t";
      if (suppressQ) {
         cout << ".";
      } else {
         cout << infile[line].getBeat();
      }
   }
   if (durationQ) {
      cout << "\t";
      if (suppressQ) {
         cout << ".";
      } else {
         double duration = getDuration(values, infile, line);
         cout << duration;
      }
   }
}



//////////////////////////////
//
// getDuration --
//

double getDuration(vector<int> values, HumdrumFile& infile, int line) {
   double ts1 = infile[line].getAbsBeat();
   double ts2;
   int startindex = line+1;
   int endindex = -1;
   for (int i=startindex; i<(int)values.size(); i++) {
      if (values[i] > 0) {
         endindex = i;
         break;
      }
   }
   if (endindex >= 0) {
      ts2 = infile[endindex].getAbsBeat();
   } else { 
      ts2 = infile.getTotalDuration();
   }
   return ts2 - ts1;
}



//////////////////////////////
//
// printEventEI --
//

void printEventEI(void) {
   int count = 0;
   if (eventQ) {
      if (count) { cout << "\t"; }
      cout << "**event";
      count++;
   }
   if (absoluteQ) {
      if (count) { cout << "\t"; }
      cout << "**abeat";
      count++;
   }
   if (meterQ) {
      if (count) { cout << "\t"; }
      cout << "**beat";
      count++;
   }
   if (durationQ) {
      if (count) { cout << "\t"; }
      cout << "**sdur";
      count++;
   }
}



//////////////////////////////
//
// printEmpty --
//

void printEmpty(const string& str) {
   if (eventQ) {
      cout << str;
   }
   if (absoluteQ) {
      cout << str;
   }
   if (meterQ) {
      cout << str;
   }
   if (durationQ) {
      cout << str;
   }
}



//////////////////////////////
//
// printAll --
//

void printAll(const string& str) {
   int count = 0;
   if (eventQ) {
      if (count) { cout << "\t"; }
      cout << str;
      count++;
   }
   if (absoluteQ) {
      if (count) { cout << "\t"; }
      cout << str;
      count++;
   }
   if (meterQ) {
      if (count) { cout << "\t"; }
      cout << str;
      count++;
   }
   if (durationQ) {
      if (count) { cout << "\t"; }
      cout << str;
      count++;
   }
}



//////////////////////////////
//
// reduceKern -- Convert kern data into pitches only
//

void reduceKern(HumdrumFile& infile) {
   int i, j, k;
   int ii, jj;

   string newtoken;
   string temptok;
   vector<string> toks;
   int nullQ = 0;
   int tieQ = 0;
   
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if ((strncmp(infile[i][0], "**", 2) == 0))  {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (strcmp(infile[i][j], "**kern") == 0) {
               infile[i].setToken(j, "**pitch");
            }
         }
      }
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            nullQ = 1;
            ii = infile[i].getDotLine(j);
            jj = infile[i].getDotSpine(j);
         } else {
            nullQ = 0;
            ii = i;
            jj = j;
         }
         infile[ii].getTokens(toks, jj);
         newtoken = "";
         if (nullQ) {
            for (k=0; k<(int)toks.size(); k++) {
               newtoken += getPitch(toks[k], true);
               if (k < (int)toks.size() - 1) {
                  newtoken += " ";
               }
            }
         } else {
            for (k=0; k<(int)toks.size(); k++) {
               tieQ = 0;
               if (toks[k].find("_") != string::npos) {
                  tieQ = 1;
               }
               if (toks[k].find("]") != string::npos) {
                  tieQ = 1;
               }
               if (tieQ) {
                  newtoken += getPitch(toks[k], true);
               } else {
                  newtoken += getPitch(toks[k], false);
               }
               if (k < (int)toks.size() - 1) {
                  newtoken += " ";
               }
            }
         }
         infile[i].setToken(j, newtoken);
      }
   }
}



//////////////////////////////
//
// getPitch --
//

string getPitch(const string& kern, int sustainQ) {
   string output;
   if (attackQ && sustainQ) {
      return ".";
   }
   if (kern.find("r") != string::npos) {
      sustainQ = 0;
      if (attackQ) {
         return ".";
      }
   }
   if (sustainQ) {
		output += "(";
   }
   output += Convert::kernToScientificNotation(kern);
   if (kern.find("@") != string::npos) {
      output += "@";
   }
   if (sustainQ) {
		output += ")";
   }
   return output;
}



//////////////////////////////
//
// printEvent --
//

void printEvent(int value) {
   if (value > 0) {
      cout << value;
   } else {
      cout << ".";
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("p|prepend=b",  "output is prepended to input data lines");
   opts.define("a|append=b",   "output is appended to input data lines");
   opts.define("r|reduce=b",   "reduce kern to pitches only");
   opts.define("m|meter=b",    "include metric position");
   opts.define("d|duration=b", "include event duration");
   opts.define("e|event=b",    "include event enumerations");
   opts.define("t|attack=b",   "only output attacks in reduction");
   opts.define("A|absolute=b", "display absbeat info");
   opts.define("all=b",        "display all output info");

   opts.define("debug=b");              // determine bad input line num
   opts.define("author=b");             // author of program
   opts.define("version=b");            // compilation info
   opts.define("example=b");            // example usages
   opts.define("help=b");               // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, April 2016" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 28 April 2016" << endl;
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

   appendQ      = opts.getBoolean("append");
   prependQ     = opts.getBoolean("prepend");
   reduceQ      = opts.getBoolean("reduce");
   meterQ       = opts.getBoolean("meter");
   durationQ    = opts.getBoolean("duration");
   eventQ       = opts.getBoolean("event");
   attackQ      = opts.getBoolean("attack");
   absoluteQ    = opts.getBoolean("absolute");
   int allQ     = opts.getBoolean("all");

   if (!(eventQ || absoluteQ || durationQ || meterQ || reduceQ)) {
      allQ = 1;
   }

   if (allQ) {
      eventQ    = 1;
      absoluteQ = 1;
      durationQ = 1;
      meterQ    = 1;
      reduceQ   = 1;
   }


   if (!(eventQ || absoluteQ || durationQ || meterQ || reduceQ)) {
      eventQ = 1;
   }
   if (reduceQ && !(eventQ || absoluteQ || durationQ || meterQ)) {
      onlyReduceQ = 1;
   }

   if ((eventQ || absoluteQ || durationQ || meterQ) && reduceQ && !(appendQ || prependQ)) {
      prependQ = 1;
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


// md5sum: aab1a9354e3ef11a038a283948c4e8b3 event.cpp [20160217]
