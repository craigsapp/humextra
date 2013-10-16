//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Oct  9 18:44:49 PDT 2013
// Last Modified: Wed Oct  9 18:44:52 PDT 2013
// Filename:      ...museinfo/examples/all/voicecount.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/voicecount.cpp
// Syntax:        C++; museinfo
//
// Description:   Count the active number of voices/parts sounding at any 
//                given moment in the score.
//

#include "humdrum.h"
#include <stdlib.h>
#include "PerlRegularExpression.h"

#ifndef OLDCPP
   #include <iostream>
   #include <fstream>
   #include <sstream>
   #define SSTREAM stringstream
   #define CSTRING str().c_str()
   using namespace std;
#else
   #include <iostream.h>
   #include <fstream.h>
   #ifdef VISUAL
      #include <strstrea.h>
   #else
      #include <strstream.h>
   #endif
   #define SSTREAM strstream
   #define CSTRING str()
#endif


///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const char* command);
void      processFile          (HumdrumFile& infile, const char* filename);
int       getVoiceCount        (HumdrumFile& infile, int line);
void      printExclusiveInterpretation(void);
void      printAnalysis        (HumdrumFile& infile, int line);


// global variables
Options   options;             // database for command-line arguments
int       debugQ       = 0;    // used with --debug option
int       appendQ      = 0;    // used with -a option
int       prependQ     = 0;    // used with -p option


///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options.getArgList());
   HumdrumFile infile;

   while (streamer.read(infile)) {
      processFile(infile, infile.getFileName());
   }
  
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

void processFile(HumdrumFile& infile, const char* filename) {
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (appendQ) { cout << infile[i]; }
      if (infile[i].isData()) {
         if (appendQ)  { cout << '\t'; }
         printAnalysis(infile, i);
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
         if (appendQ)  { cout << '\t'; }
         cout << infile[i][0];
         if (prependQ) { cout << '\t'; }
         if (appendQ)  { cout << '\n'; }
      } else if (infile[i].isLocalComment()) {
         if (appendQ)  { cout << '\t'; }
         cout << infile[i] << "!";
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
// printAnalysis --
//

void printAnalysis(HumdrumFile& infile, int line) {
   cout << getVoiceCount(infile, line);
}



//////////////////////////////
//
// printExclusiveInterpretation --
//

void printExclusiveInterpretation(void) {
   cout << "**vcount";
}



//////////////////////////////
//
// getVoiceCount --
//

int getVoiceCount(HumdrumFile& infile, int line) {
   int j;
   int ii, jj;
   int count = 0;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         continue;
      }
      ii = line;
      jj = j;
      if (infile[line].isNullToken(j)) {
        ii = infile[line].getDotLine(j);
        jj = infile[line].getDotSpine(j);
      }
      if (strchr(infile[ii][jj], 'r') != NULL) {
         continue;
      }
      count++;
   }
   return count;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|append=b", "append analysis data to input"); 
   opts.define("p|prepend=b", "prepend analysis data to input"); 

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
      cout << argv[0] << ", version: 10 October 2013" << endl;
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

   appendQ    = opts.getBoolean("append");
   prependQ   = opts.getBoolean("prepend");

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


// md5sum: 061bdcd3de38bb305b57576a99831ad8 voicecount.cpp [20131009]
