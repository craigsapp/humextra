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

using namespace std;

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const char* command);
void      processFile          (vector<int>& eventnums, HumdrumFile& infile);
void      printFile            (vector<int>& eventnums, HumdrumFile& infile);
void      printEvent           (int value);

// global variables
Options   options;             // database for command-line arguments
int       appendQ    = 0;      // used with -a option
int       prependQ   = 0;      // used with -p option

///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);
   HumdrumFile infile;
   vector<int> eventnums;
   while (streamer.read(infile)) {
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
   int i;
   for (i=0; i<(int)infile.getNumLines(); i++) {
      if (!infile[i].hasSpines()) {
         cout << infile[i] << endl;
         continue;
      }

      if (infile[i].isData()) {
         if (prependQ) {
            printEvent(eventnums[i]);
            cout << "\t" << infile[i] << endl;
         } else if (appendQ) {
            cout << infile[i] << "\t" << endl;
            printEvent(eventnums[i]);
         } else {
            printEvent(eventnums[i]);
            cout << endl;
         }
      }

      if (infile[i].isLocalComment()) {
         if (prependQ) {
            cout << "!\t" << infile[i] << endl;
         } else if (appendQ) {
            cout << infile[i] << "\t!" << endl;
         } else {
            cout << "!" << endl;
         }
      }

      if (infile[i].isBarline()) {
         if (prependQ) {
            cout << infile[i][0] << "\t" << infile[i] << endl;
         } else if (appendQ) {
            cout << infile[i] << "\t" << infile[i][0] << endl;
         } else {
            cout << infile[i][0] << endl;
         }
      }

      if (infile[i].isInterpretation()) {
         if (strncmp(infile[i][0], "**", 2) == 0) {
            if (prependQ) {
               cout << "**event\t" << infile[i] << endl;
            } else if (appendQ) {
               cout << infile[i] << "\t**event" << endl;
            } else {
               cout << "**event" << endl;
            }
         } else if (strcmp(infile[i][0], "*-") == 0) {
            if (prependQ) {
               cout << "*-\t" << infile[i] << endl;
            } else if (appendQ) {
               cout << infile[i] << "\t*-" << endl;
            } else {
               cout << "*-" << endl;
            }
         } else {
            if (prependQ) {
               cout << "*\t" << infile[i] << endl;
            } else if (appendQ) {
               cout << infile[i] << "\t*" << endl;
            } else {
               cout << "*" << endl;
            }
         }
      }
   }
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
   opts.define("p|prepend=b", "Output is prepended to input data lines");
   opts.define("a|append=b", "Output is appended to input data lines");
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
