//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Oct 17 14:50:33 PDT 2013
// Last Modified: Thu Oct 17 14:50:39 PDT 2013
// Filename:      ...museinfo/examples/all/location.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/location.cpp
// Syntax:        C++; museinfo
//
// Description:   Give the location of data lines in various formats.
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
void      processFile          (HumdrumFile& infile);
int       getVoiceCount        (HumdrumFile& infile, int line);
int       getNoteCount         (HumdrumFile& infile, int line);
void      printExclusiveInterpretation(void);
void      printLocation        (HumdrumFile& infile, int line, int measure);
int       getStartingMeasure   (HumdrumFile& infile);

// global variables
Options   options;             // database for command-line arguments
int       debugQ       = 0;    // used with --debug option
int       appendQ      = 0;    // used with -a option
int       prependQ     = 0;    // used with -p option
int       measureQ     = 0;    // used with -m option
int       doubleQ      = 0;    // used with -d option
int       labelQ       = 1;    // used with -L option
int       lineQ        = 0;    // used with -l option
int       fileQ        = 0;    // used with -f option
int       absQ         = 0;    // used with -q, -h, -w, -e, -x options
int       beatQ        = 0;    // used with -b option
int       rationalQ    = 0;    // used with -R option
int       spaceQ       = 0;    // used with -S option
int       quotesQ      = 0;    // used with -Q option
char      absChar      = 'Q';  // used with -e, -q, -h, -w options
RationalNumber absFactor = 1;  // used with -q, -h, -w, -e, -x options

///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options.getArgList());
   HumdrumFile infile;

   while (streamer.read(infile)) {
      if (beatQ || absQ) {
         infile.analyzeRhythm();
      }
      processFile(infile);
   }
  
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

void processFile(HumdrumFile& infile) {
   PerlRegularExpression pre;
   int measure = getStartingMeasure(infile);
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (appendQ) { cout << infile[i]; }
      if (infile[i].isData()) {
         if (appendQ)  { cout << '\t'; }
         printLocation(infile, i, measure);
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
         if (pre.search(infile[i][0], "=([\\d.]+)")) {
            measure = atoi(pre.getSubmatch(1));
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
// getStartingMeasure --
//

int getStartingMeasure(HumdrumFile& infile) {
   int i;
   int measure = -1;
   int datafound = 0;
   int measurefound = 0;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         datafound = 1;
         break;
      }
      if (infile[i].isMeasure()) {
         measurefound = 1;
         if (pre.search(infile[i][0], "=([\\d]+)")) {
            measure = atoi(pre.getSubmatch(1));
         }
      }
      if (measurefound) {
         break;
      }
   }

   if (datafound) {
      // data has a pickup measure, so subtract one from measure
      // number if not already negative.
      if (measure < 0) {
         return 0;
      } else {
         return measure - 1;
      }
   } else {
      // data not found before barline, so use the measure number
      if (measure < 0) {
         return 1;
      } else {
         return measure;
      }
   }
 
   // shouldn't get here, but return 1 if it happens.
   return 1;
}



//////////////////////////////
//
// printLocation --
//

void printLocation(HumdrumFile& infile, int line, int measure) {
   RationalNumber rat;
   int startQ = 0;
   if (fileQ) {
      if (startQ && spaceQ) { cout << ' '; } startQ++;
      if (quotesQ) {
         if (labelQ) { cout << '"'; }
      }
      cout << infile.getFilename();
      if (quotesQ) {
         if (labelQ) { cout << '"'; }
      }
   }
   if (lineQ) {
      if (startQ && spaceQ) { cout << ' '; } startQ++;
      if (labelQ) { cout << 'L'; }
      cout << line  + 1;
      if (labelQ && doubleQ) { cout << 'L'; }
   }
   if (measureQ) {
      if (startQ && spaceQ) { cout << ' '; } startQ++;
      if (labelQ) { cout << 'M'; }
      cout << measure;
      if (labelQ && doubleQ) { cout << 'M'; }
   }

   if (beatQ) {
      if (startQ && spaceQ) { cout << ' '; } startQ++;
      if (labelQ) { cout << 'B'; }
      if (rationalQ) {
         rat = infile[line].getBeatR(); 
         rat *= absFactor;
         rat.printTwoPart(cout);
      } else {
         cout << infile[line].getBeat();
      }
      if (labelQ && doubleQ) { cout << 'B'; }
   }

   if (absQ) {
      if (startQ && spaceQ) { cout << ' '; } startQ++;
      if (labelQ) { cout << absChar; }
      if (rationalQ) {
         rat = infile[line].getAbsBeatR(); 
         rat *= absFactor;
         rat.printTwoPart(cout);
      } else {
         cout << infile[line].getAbsBeat() * absFactor.getFloat();
      }
      if (labelQ && doubleQ) { cout << absChar; }
   }

}



//////////////////////////////
//
// printExclusiveInterpretation --
//

void printExclusiveInterpretation(void) {
   cout << "**location";
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|append=b",       "append analysis data to input"); 
   opts.define("p|prepend=b",      "prepend analysis data to input"); 
   opts.define("b|beat=b",         "list beat in measure"); 
   opts.define("q|quarter=b",      "list absolute beat in quarter notes"); 
   opts.define("h|half=b",         "list absolute beat in half notes"); 
   opts.define("w|whole=b",        "list absolute beat in whole notes"); 
   opts.define("e|eighth=b",       "list absolute beat in eighth notes"); 
   opts.define("x|sixteenth=b",    "list absolute beat in sixteenth notes"); 
   opts.define("S|no-space=b",     "do not put spaces between location items"); 
   opts.define("Q|no-quotes=b",    "do not put quotes around filenames"); 
   opts.define("R|rational=b",     "use rational numbers instead of floats"); 

   opts.define("m|measure|bar|barlines=b",   "display measure numbers"); 
   opts.define("L|no-labels=b",    "do not display labels on data");
   opts.define("d|double=b",       "double labels before and after data");
   opts.define("f|filename=b",     "show filename");
   opts.define("l|line=b",         "show line numbers indexed from 1");

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
      usage(opts.getCommand().data());
      exit(0);
   } else if (opts.getBoolean("example")) {
      example();
      exit(0);
   }

   appendQ    =  opts.getBoolean("append");
   prependQ   =  opts.getBoolean("prepend");
   measureQ   =  opts.getBoolean("measure");
   doubleQ    =  opts.getBoolean("double");
   labelQ     = !opts.getBoolean("no-labels");
   lineQ      =  opts.getBoolean("line");
   fileQ      =  opts.getBoolean("filename");
   beatQ      =  opts.getBoolean("beat");
   spaceQ     = !opts.getBoolean("no-space");
   quotesQ    = !opts.getBoolean("no-quotes");
   rationalQ  =  opts.getBoolean("rational");

   int counter = 0;
   if (opts.getBoolean("quarter")) {
      absQ = 1;
      absFactor = 1;
      absChar = 'Q';
      counter++;
   }

   if (opts.getBoolean("half")) {
      absQ = 1;
      absFactor.setValue(1,2);
      absChar = 'H';
      counter++;
   }

   if (opts.getBoolean("whole")) {
      absQ = 1;
      absFactor.setValue(1,4);
      absChar = 'W';
      counter++;
   }

   if (opts.getBoolean("eighth")) {
      absQ = 1;
      absFactor = 2;
      absChar = 'E';
      counter++;
   }

   if (opts.getBoolean("sixteenth")) {
      absQ = 1;
      absFactor = 4;
      absChar = 'X';
      counter++;
   }

   if (counter > 1) {
      // use only quarter notes if more than one timebase is given.
      absFactor = 1;
   }

   if (appendQ) {
      // mutually exclusive options
      prependQ = 0;
   }

   if (!(measureQ || lineQ || fileQ)) {
      // if no location type selected, turn them all on
      fileQ     = 1;
      lineQ     = 1;
      measureQ  = 1;
      beatQ     = 1;
      absQ      = 1;
      absFactor = 1;
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


// md5sum: cde7fed6b654fb814d2b9ea960ccfa3e location.cpp [20131108]
