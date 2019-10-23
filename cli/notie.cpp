//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Mar 12 10:51:09 PST 2016
// Last Modified: Sat Mar 12 10:51:12 PST 2016
// Filename:      ...sig/examples/all/notie.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/notie.cpp
// Syntax:        C++; museinfo
//
// Description:   Analyzes **kern data with serial descriptions.
// Reference: 	  http://solomonsmusic.net/pcsets.htm
//
// Note: does not deal with chord notes yet.
//

#include "humdrum.h"
#include <iostream>

// function declarations
void        checkOptions          (Options& opts, int argc, char* argv[]);
void        example               (void);
void        usage                 (const char* command);
void        processFile           (HumdrumFile& infile);

// global variables
Options     options;              // database for command-line arguments
int         markQ = 0;            // used with -m option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options);
   HumdrumFile infile;
   while (streamer.read(infile)) {
      processFile(infile);
   }
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile --
//

void processFile(HumdrumFile& infile) {
   int i, j;
   PerlRegularExpression pre;
   infile.analyzeRhythm();
   RationalNumber rn;
   char newrhy[128];
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         cout << infile[i] << endl;
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            cout << infile[i][j];
         } else {
            if (strchr(infile[i][j], 'r') != NULL) {
               cout << infile[i][j];
            } else if (strchr(infile[i][j], '_') != NULL) {
               if (markQ) {
                  cout << "rq";
               } else {
                  cout << ".";
               }
            } else if (strchr(infile[i][j], ']') != NULL) {
               if (markQ) {
                  cout << "rq";
               } else {
                  cout << ".";
               }
            } else if (strchr(infile[i][j], '[') != NULL) {
               rn = infile.getTiedDurationR(i, j, 0);
               Convert::durationRToKernRhythm(newrhy, rn);
               string newtoken = infile[i][j];
               pre.sar(newtoken, "[\\d%]+", newrhy, "g");
               if (!markQ) {
                  pre.sar(newtoken, "[[]", "", "g");
               }
               cout << newtoken;
            } else {
               cout << infile[i][j];
            }
         }
         if (j < infile[i].getFieldCount() - 1) {
            cout << "\t";
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
   opts.define("m|mark=b",     "leave [ marker on collapsed tied group note");
   opts.define("author=b",     "author of program");
   opts.define("version=b",    "compilation info");
   opts.define("example=b",    "example usages");
   opts.define("h|help=b",     "short description");
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Mar 2016" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: Mar 2016" << endl;
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


   markQ = opts.getBoolean("mark");
}



//////////////////////////////
//
// example -- example usage of the sonority program
//

void example(void) {
   cout <<
   "                                                                         \n"
   << endl;
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



