//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Aug 25 16:56:33 PDT 2003
// Last Modified: Sat Nov  9 07:41:17 PST 2013 Added -n, -p, -i options.
// Filename:      ...sig/examples/all/blank.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/blank.cpp
// Syntax:        C++; museinfo
//
// Description:   Create a blank spine with the same number of lines
//                as the input file.
//

#include "humdrum.h"

// function declarations
void      checkOptions       (Options& opts, int argc, char* argv[]);
void      example            (void);
void      usage              (const char* command);
ostream&  printOutput        (ostream& out, HumdrumFile& infile);
ostream&  printBlanks        (ostream& out, HumdrumFile& infile, int line, 
                              const char* string, int count);
ostream&  printInterpretation(ostream& out, HumdrumFile& infile, int line, 
                              int count);
ostream&  printExclusiveInterpretations(ostream& out, HumdrumFile& infile, 
                              int line, int count, 
                              Array<SigString>& exinterps);

// global variables
Options   options;            // database for command-line arguments
int       appendQ  = 0;       // used with -a option
int       prependQ = 0;       // used with -p option
int       Count    = 0;       // used with -c option
Array<SigString> Exinterps;   // used with -i option

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   checkOptions(options, argc, argv);
   HumdrumFileSet infiles;
   infiles.read(options);

   for (int i=0; i<infiles.getCount(); i++) {
      printOutput(cout, infiles[i]);
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// printOutput --
//

ostream& printOutput(ostream& out, HumdrumFile& infile) {
   for (int i=0; i<infile.getNumLines(); i++) {
      switch (infile[i].getType()) {
         case E_humrec_data_comment:
            printBlanks(out, infile, i, "!", Count);
            break;
         case E_humrec_data_kern_measure:
            printBlanks(out, infile, i, infile[i][0], Count);
            break;
         case E_humrec_interpretation:
            printInterpretation(out, infile, i, Count);
            break;
         case E_humrec_data:
            printBlanks(out, infile, i, ".", Count);
            break;
         case E_humrec_none:
         case E_humrec_empty:
         case E_humrec_global_comment:
         case E_humrec_bibliography:
         default: // unknown line type, so just echo it to output
                  // (such as a technically illegal blank line)
            out << infile[i] << "\n";
            break;
      }
   }
   return out;
}



//////////////////////////////
//
// printInterpretation --
//

ostream& printInterpretation(ostream& out, HumdrumFile& infile, int line, 
      int count) {

   if (strncmp(infile[line][0], "**", 2) == 0) {
      printExclusiveInterpretations(out, infile, line, count, Exinterps);
   } else if (strcmp(infile[line][0], "*-") == 0) {
      printBlanks(out, infile, line, "*-", count);
   } else if (strncmp(infile[line][0], "*>", 2) == 0) {
      // expansion labels, such as *>[A,A,B], *>A, *B.
      printBlanks(out, infile, line, infile[line][0], count);
   } else {
      printBlanks(out, infile, line, "*", count);
   }
   return out;
}



//////////////////////////////
//
// printExclusiveInterpretations -- print exclusive interpretations
//    which may be the same or different for each new blank spine.
//

ostream& printExclusiveInterpretations(ostream& out, HumdrumFile& infile, 
      int line, int count, Array<SigString>& exinterps) {
   if (appendQ) {
      out << infile[line] << '\t';
   }
   int j, jj;
   for (j=0; j<count; j++) {
      jj = j;
      if (jj > exinterps.getSize() - 1) {
         jj = exinterps.getSize() - 1 ;
      }
      out << "**" << exinterps[jj];
      if (j < count - 1) {
         out << '\t';
      }
   }
   if (prependQ) {
      out << '\t' << infile[line];
   } 
   out << '\n';
   return out;
}



//////////////////////////////
//
// printBlanks --
//

ostream& printBlanks(ostream& out, HumdrumFile& infile, int line, 
     const char* string, int count) {
   if (appendQ) {
      out << infile[line] << '\t';
   }
   int j;
   for (j=0; j<count; j++) {
      out << string;
      if (j < count - 1) {
         out << '\t';
      }
   }
   if (prependQ) {
      out << '\t' << infile[line];
   } 
   out << '\n';
   return out;
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|assemble|append=b",  "append analysis spine to input data");
   opts.define("p|prepend=b",          "new spines at start of input lines");
   opts.define("n|count=i",            "number of spines to add");
   opts.define("i|x|exinterp=s:blank", "set column exclusive interpretations");

   opts.define("debug=b");                // determine bad input line num
   opts.define("author=b");               // author of program
   opts.define("version=b");              // compilation info
   opts.define("example=b");              // example usages
   opts.define("h|help=b");               // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Aug 2003" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: Nov 2013" << endl;
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

   appendQ   = opts.getBoolean("assemble");
   prependQ  = opts.getBoolean("prepend");
   if (appendQ) {
      // mutually exclusive options
      prependQ = 0;
   }
   Count     = opts.getInteger("count");
   if (Count < 1) {
      Count = 1;
   } else if (Count > 1000) {
      // don't allow a ridiculously large number
      // of blank spines to be generated.
      Count = 1000;
   }
   PerlRegularExpression pre;
   pre.getTokens(Exinterps, "[\\s,\\*]+", opts.getString("exinterp"));
}
  


//////////////////////////////
//
// example -- example usage of the program
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



// md5sum: ec5bfed5a848eeff7508e533e478fd5c blank.cpp [20131109]
