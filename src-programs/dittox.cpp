//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Nov 14 16:32:36 PST 2000
// Last Modified: Tue Nov 14 16:32:39 PST 2000
// Last Modified: Sun Apr 14 21:25:48 PDT 2013 Enabled multiple segment input
// Last Modified: Sun Apr 21 16:18:20 PDT 2013 Added -k option, -c option
// Filename:      ...sig/examples/all/dittox.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/dittox.cpp
// Syntax:        C++; museinfo
//
// Description:   Fills in the meaning of null tokens.
//

#include "humdrum.h"

// function declarations
void         checkOptions  (Options& opts, int argc, char* argv[]);
void         example       (void);
void         printOutput   (HumdrumFile& infile);
void         printKernOutput(HumdrumFile& infile);
void         printKernTokenLineDuration(HumdrumFile& infile, int line, 
                            int field);
void         usage         (const char* command);

// global variables
Options      options;          // database for command-line arguments
int          parensQ = 0;      // used with the -p option
int          rhythmQ   = 0;      // used with -k option
int          skipQ   = 0;      // used with -s option
const char*  skipString = "";  // used with -s option
int          charQ   = 0;      // used with -c option
Array<char> charString;        // used with -c option
int          xcharQ = 0;       // used with -c option
Array<char> xcharString;       // used with -C option

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
      if (rhythmQ) {
         printKernOutput(infiles[i]);
      } else {
         printOutput(infiles[i]);
      }
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("p|parens=b", "print parentheses around dittox data");
   opts.define("r|rhythm=b", "print keeping kern data rhythm parseable");
   opts.define("c|char|chars=s:[rA-Ga-g#-]", 
                             "print only characters in list when dittoing");
   opts.define("C|xchar|xchars=s", "remove characters in list when dittoing");
   opts.define("k|kern|pitches=b", "print only pitch names in **kern data");

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

   parensQ  = opts.getBoolean("parens");
   rhythmQ  = opts.getBoolean("rhythm");

   PerlRegularExpression pre;
   charQ    = opts.getBoolean("char");
   if (charQ) {
      charString = opts.getString("char");
      if (!pre.search(charString, "^\\[")) {
         pre.sar(charString, "^", "[");
         pre.sar(charString, "$", "]");
      }
      pre.sar(charString, "^\\[", "[^");
   }

   xcharQ    = opts.getBoolean("xchar");
   if (xcharQ) {
      xcharString = opts.getString("xchar");
      if (!pre.search(xcharString, "^\\[")) {
         pre.sar(xcharString, "^", "[");
         pre.sar(xcharString, "$", "]");
      }
   }

   if (opts.getBoolean("kern")) {
      charString = "[^rA-Ga-g#-]";
      charQ = 1;
   }
}


//////////////////////////////
//
// example -- example usage of the dittox program
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
   Array<char> data;
   PerlRegularExpression pre;
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
               data = infile.getDotValue(i, j);

               if (charQ) {
                  pre.sar(data, charString.getBase(), "", "g");
               }

               if (xcharQ) {
                  pre.sar(data, xcharString.getBase(), "", "g");
               }

               if (data == "") {
                  data = ".";
               } 
               cout << data;
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
// printKernOutput -- Notes are split into a sequence of tied notes.
//

void printKernOutput(HumdrumFile& infile) {
   int i, j;
   infile.analyzeRhythm("4");
   infile.printNonemptySegmentLabel(cout);
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         cout << infile[i].getLine() << "\n";
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
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
         } else { 
            // this is **kern data, so create tied notes if note duration
            // is longer than the current line's duration
            printKernTokenLineDuration(infile, i, j);
         }
         if (j < infile[i].getFieldCount() - 1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }
}



//////////////////////////////
//
// printKernTokenLineDuration -- print a kern token with only the
//     the duration of the line for the duration of the note(s).
//
//     Beaming information may become messed up by this function.
//

void printKernTokenLineDuration(HumdrumFile& infile, int line, int field) {
   RationalNumber notestartabsbeat; // starting absbeat of note
   RationalNumber noteendabsbeat;   // ending absbeat of note
   RationalNumber linedur;          // absbeat of current line
   RationalNumber notedur;          // duration of note
   int ii, jj;
   linedur = infile[line].getDurationR();
   Array<char> notebuffer;
   if (linedur == 0) {
      // don't bother with grace notes for now:
      cout << infile[line][field];
   }
   ii = line;
   jj = field;
   if (infile[line].isNullToken(field)) {
      ii = infile[line].getDotLine(field);
      jj = infile[line].getDotField(field);
   }
   PerlRegularExpression pre;
   RationalNumber linestartabsbeat = infile[line].getAbsBeatR();
   notestartabsbeat = infile[ii].getAbsBeatR();
   RationalNumber lineendabsbeat;
   char newdur[1024] = {0};
   lineendabsbeat = linestartabsbeat + linedur;
   notedur = Convert::kernToDurationR(infile[ii][jj]);
   if (notedur == linedur) {
      cout << infile[ii][jj];
      return;
   }
   noteendabsbeat = notestartabsbeat + notedur;
   Convert::durationToKernRhythm(newdur, linedur.getFloat());
   notebuffer = infile[ii][jj];
   pre.sar(notebuffer, "[\\d%.]+", newdur, "g");
   
   // handle tie structure:
   //   Chord notes are all presumed to be tied in the same way.  This
   //   may not be true, so to be fully generalized, keeping track
   //   of the tie states of notes in the chord should be done.

   // * If the original note duration is the same as the line duration
   //   just keep the original note (already taken care of above).

   // * If the note starts at this point, then add a "[" tie marker
   //   if there is not a "[" or "_" character already on the note(s)
   if ((notestartabsbeat == linestartabsbeat) 
         && (strchr(infile[ii][jj], '[') == NULL)
         && (strchr(infile[ii][jj], '_') == NULL)) {
      pre.sar(notebuffer, " ", " [", "g");
      cout << "[" << notebuffer;
      return;
   }

   // * If the linenote ends on this line, add "]" unless the original
   //   note had "_".
   if (lineendabsbeat == noteendabsbeat) {
      if (strchr(infile[ii][jj], '_') != NULL) {
         cout << notebuffer;
      } else if (strchr(infile[ii][jj], '[') != NULL) {
         pre.sar(notebuffer, "\\[", "", "g");
         pre.sar(notebuffer, " ", "_ ", "g");
         cout << notebuffer << "_";
      } else {
         pre.sar(notebuffer, "\\[", "", "g");
         pre.sar(notebuffer, " ", "\\] ", "g");
         cout << notebuffer << "]";
      }
      return;
   }

   
   if (strchr(notebuffer.getBase(), '[') != NULL) {
      cout << notebuffer;
   } else {
      pre.sar(notebuffer, " ", "_ ", "g");
      cout << notebuffer << "_";
   }
}



//////////////////////////////
//
// usage -- gives the usage statement for the dittox program
//

void usage(const char* command) {
   cout <<
   "                                                                         \n"
   << endl;
}



// md5sum: 51fd57b05e75ed2eb3ab1ca6cdfa2023 dittox.cpp [20130421]
