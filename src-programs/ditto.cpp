//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Nov 14 16:32:36 PST 2000
// Last Modified: Tue Nov 14 16:32:39 PST 2000
// Last Modified: Sun Apr 14 21:25:48 PDT 2013 Enabled multiple segment input
// Filename:      ...sig/examples/all/ditto.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/ditto.cpp
// Syntax:        C++; museinfo
//
// Description:   Fills in the meaning of null tokens.
//

#include "humdrum.h"

// function declarations
void         checkOptions  (Options& opts, int argc, char* argv[]);
void         example       (void);
void         printOutput   (HumdrumFile& infile);
void         usage         (const char* command);

// global variables
Options      options;      // database for command-line arguments
int          parensQ = 0;  // used with the -p option

///////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
   HumdrumFileSet infiles;

   // process the command-line options
   checkOptions(options, argc, argv);

   // figure out the number of input files to process
   int numinputs = options.getArgCount();

   int i;
   if (numinputs < 1) {
      infiles.read(cin);
   } else {
      for (i=0; i<numinputs; i++) {
         infiles.readAppend(options.getArg(i+1));
      }
   }

   for (i=0; i<infiles.getCount(); i++) {
      printOutput(infiles[i]);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("p|parens=b", "print parentheses around ditto data");

   opts.define("author=b");                     // author of program
   opts.define("version=b");                    // compilation info
   opts.define("example=b");                    // example usages
   opts.define("h|help=b");                     // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Nov 2000" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 14 Nov 2000" << endl;
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

   parensQ = opts.getBoolean("parens");

}


//////////////////////////////
//
// example -- example usage of the ditto program
//

void example(void) {
   cout <<
   "                                                                         \n"
   << endl;
}


//////////////////////////////
//
// printOutput -- display the filled results
//

void printOutput(HumdrumFile& infile) {
   int i, j;
   infile.printNonemptySegmentLabel(cout);
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].getType() && (E_humrec_data == 0)) {
         cout << infile[i].getLine() << "\n";
      } else {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (strcmp(infile[i][j], ".") == 0) {
               if (parensQ) {
                  cout << "(";
               }
               cout << infile.getDotValue(i, j);
               if (parensQ) {
                  cout << ")";
               }
            } else {
               cout << infile[i][j];
            }
            if (j < infile[i].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         cout << "\n";
      }
   }
}



//////////////////////////////
//
// usage -- gives the usage statement for the ditto program
//

void usage(const char* command) {
   cout <<
   "                                                                         \n"
   << endl;
}



// md5sum: fd59e6c3941038fd1f49b8c0a15c8713 ditto.cpp [20050403]
