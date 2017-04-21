//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Aug 17 19:08:36 PDT 2014
// Last Modified: Sun Aug 17 19:08:39 PDT 2014
// Filename:      ...sig/examples/all/barline.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/barline.cpp
// Syntax:        C++; museinfo
//
// Description:   Add or remove barlines from music.  Grace notes not
//                handled.
//

#include "humdrum.h"

#include <string.h>
#include <cctype>

#ifndef OLDCPP
   #include <sstream>
   #define SSTREAM stringstream
   #define CSTRING str().c_str()
   using namespace std;
#else
   #ifdef VISUAL
      #include <strstrea.h>     /* for windows 95 */
   #else
      #include <strstream.h>
   #endif
   #define SSTREAM strstream
   #define CSTRING str()
#endif
   

// function declarations
void   checkOptions       (Options& opts, int argc, char* argv[]);
void   example            (void);
void   usage              (const char* command);
void   processFileAuto    (HumdrumFile& file);
void   processFileConvert (HumdrumFile& infile);
void   cutNotesAcrossBarline(HumdrumFile& infile, int line, int trackline, 
                           int fieldcount, RationalNumber& barabsbeat);
void   getTrackInfo       (HumdrumFile& infile, int trackline, 
                           Array<int>& trackcount, Array<int>& subtrackcount);
void   generateDataLine(HumdrumFile& infile, int line, Array<int>& trackcount, 
                           Array<int>& subtrackcount);
void   getAddress         (int& tline, int& tcol, HumdrumFile& infile, 
                           int barline, int track, int subtrack);
int    setNewDuration     (HumdrumFile& infile, int line, int col, 
                           RationalNumber& newdur, int order, int tiestate);

// global variables
Options   options;            // database for command-line arguments
int       autoQ    = 0;       // used with -a option
int       convertQ = 0;       // used with -c option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   // process the command-line options
   checkOptions(options, argc, argv);

   HumdrumFileSet infiles;
   infiles.read(options);

   for (int i=0; i<infiles.getCount(); i++) {
      if (autoQ) {
         processFileAuto(infiles[i]);
      } else if (convertQ) {
         processFileConvert(infiles[i]);
      }
   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFileConvert --
//

void processFileConvert(HumdrumFile& infile) {
   infile.analyzeRhythm("4");
   PerlRegularExpression pre;
   RationalNumber absbeat;
   int fieldcount = 0;
   int i, j;
   char buffer[1024] = {0};
   int trackline = 0;
   int dashQ = 0;

   for (i=0; i<infile.getNumLines(); i++) {

      if (infile[i].isData()) {
         fieldcount = infile[i].getFieldCount();
         trackline = i;
         continue;
      }

      if (infile[i].isInterpretation()) {
         fieldcount = infile[i].getFieldCount();
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (strcmp(infile[i][j], "*") == 0) {
               continue;
            } else if (strcmp(infile[i][j], "*^") == 0) {
               fieldcount++;
               trackline = i;
            } else if ((j > 0) && (strcmp(infile[i][j], "*v") == 0)) {
               if (strcmp(infile[i][j-1], "*v") == 0) {
                  fieldcount--;
                  trackline = i;
               }
            }
            // should check for *+, but not very common.
         }
         continue;
      }
      if (!infile[i].isGlobalComment()) {
         continue;
      }

      if (pre.search(infile[i][0], 
            "^!!barline:.*absbeat\\s*=\\s*(\\d+)/(\\d+)")) {
         absbeat = atoi(pre.getSubmatch(1));
         absbeat /= atoi(pre.getSubmatch(2));
         if (pre.search(infile[i][0], "dash", "i")) {
            dashQ = 1;
         } else {
            dashQ = 0;
         }
      } else if (pre.search(infile[i][0], 
            "^!!barline:.*absbeat\\s*=\\s*(\\d+)")) {
         absbeat = atoi(pre.getSubmatch(1));
         if (pre.search(infile[i][0], "dash", "i")) {
            dashQ = 1;
         } else {
            dashQ = 0;
         }
      } else {
         continue;
      }

      buffer[0] = '\0';
      for (j=0; j<fieldcount; j++) {
         strcat(buffer, "=");
         if (dashQ) {
            strcat(buffer, ".");
         }
         if (j < fieldcount - 1) {
            strcat(buffer, "\t");
         }
      }
      infile[i].setLine(buffer);
      infile[i].setAbsBeatR(absbeat);
      cutNotesAcrossBarline(infile, i, trackline, fieldcount, absbeat);
   }

   cout << infile;
}



//////////////////////////////
//
// cutNotesAcrossBarline --
//
// !!barline: dataline gets converted into a dataline
//

void cutNotesAcrossBarline(HumdrumFile& infile, int barline, int trackline,
      int fieldcount, RationalNumber& barabsbeat) {

   Array<int> tracknum;
   Array<int> subtracknum;
   int i;

   getTrackInfo(infile, trackline, tracknum, subtracknum);

   PerlRegularExpression pre;
   int dataline = -1;
   
   for (i=barline+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         dataline = i;
         break;
      } else if (infile[i].isGlobalComment()) {
         if (pre.search(infile[i][0], "!!barline:.*dataline")) {
            generateDataLine(infile, i, tracknum, subtracknum);
            infile[i].setAbsBeatR(barabsbeat);
            dataline = i;
            break;
         }
      }
   }
   
   RationalNumber nabsbeat; // absolute beat of starting note;
   RationalNumber dabsbeat; // absolute beat of data line;
   RationalNumber firstdur, seconddur;
   RationalNumber olddur;
   int ii, jj;
   int tiestate;
//   int track; 
   int j;
   for (j=0; j<infile[dataline].getFieldCount(); j++) {
      if (strcmp(infile.getTrackExInterp(tracknum[j]).c_str(), "**kern") != 0) {
         continue;
      }
      if (strcmp(infile[dataline][j], ".") != 0) {
         continue;
      }
      getAddress(ii, jj, infile, barline, tracknum[j], subtracknum[j]);
      
      nabsbeat = infile[ii].getAbsBeatR();
      dabsbeat = infile[dataline].getAbsBeatR();
      firstdur = dabsbeat - nabsbeat;
      olddur = Convert::kernToDurationR(infile[ii][jj]);
      seconddur = olddur - firstdur;

      infile[dataline].setToken(j, infile[ii][jj]);
      tiestate = setNewDuration(infile, ii, jj, firstdur, -1, 0);
      setNewDuration(infile, dataline, j, seconddur, +1, tiestate);
   }
}



//////////////////////////////
//
// setNewDuration -- Does not handle chords with different tie states
//     for different notes in the chord.
//

int setNewDuration(HumdrumFile& infile, int line, int col, 
   RationalNumber& newdur, int order, int tiestate) {

   PerlRegularExpression pre;
   Array<char> token;
   token = infile[line][col];
   char buffer[1024] = {0};
   Convert::durationRToKernRhythm(buffer, newdur);
   pre.sar(token, "[\\d%.]+", buffer, "g");
   infile[line].setToken(col, token.getBase());
   if (strchr(infile[line][col], 'r') != NULL) {
      // don't care about ties for rests
      return 0;
   }

   if (order < 0) {
      tiestate = 0;
      // Starting note which will be tied to the next note.
      // If the note is the start or a continue of a tie, then
      // set tiestate to 1:
      if (pre.search(infile[line][col], "[_[]")) {
      }
      if (pre.search(infile[line][col], "[[]")) {
         // start of a tie, so don't alter that
         return 1;
      } else if (pre.search(infile[line][col], "_")) {
         // continuation of a tie, so don't alter that
         return 1;
      } else if (pre.search(infile[line][col], "[]]")) {
         // end of a tie so convert "]" to "_" and tell
         // second note to terminate the tie.
         pre.sar(token, "[]]", "_", "g");
         infile[line].setToken(col, token.getBase());
         return -1;
      } else {
         // Not in a tie group, so start one, and tell the
         // second note to terminate it.
         pre.sar(token, " ", " [", "g");
         pre.sar(token, "^", "[", "");
         infile[line].setToken(col, token.getBase());
         return -1;
      }
   } else if (order > 0) {
      // Ending note.  If tiestate == 1, then this note should 
      // continue a tie.  If tiestate == -1, then this note should
      // terminate a tie.
      pre.sar(token, "[_\\][]", "", "g");
      if (tiestate == 1) {
         // continue tie
         pre.sar(token, " ", "_ ", "g");
         pre.sar(token, "$", "_", "");
         infile[line].setToken(col, token.getBase());
         return 0;
      } else if (tiestate == -1) {
         // end tie
         pre.sar(token, " ", "] ", "g");
         pre.sar(token, "$", "]", "");
         infile[line].setToken(col, token.getBase());
         return 0;
      } else {
         cerr << "Error: should not get here" << endl;
         exit(1);
      }
   }

   cerr << "Error: should not get here 2" << endl;
   return 0;
}



//////////////////////////////
//
// getAddress --
//

void getAddress(int& tline, int& tcol, HumdrumFile& infile, int barline, 
      int track, int subtrack) {
   int ptrack;
   int ptrackcount;
   int i, j;
   for (i=barline-1; i>=0; i--) {
      if (infile[i].isBarline()) {
         break;
      }
      if (!infile[i].isData()) {
         continue;
      }
      ptrackcount = 0;
      for (j=0; j<infile[i].getFieldCount(); j++) {
         ptrack = infile[i].getPrimaryTrack(j);
         if (ptrack != track) {
            continue;
         }
         if (ptrackcount == subtrack) {
            // found matching track.  If it contains something
            // other than a null token, return its address.
            if (strcmp(infile[i][j], ".") == 0) {
               break;
            }
            // return the address
            tline = i;
            tcol = j;
            return;
         }
         ptrackcount++;
      }
   }

   cerr << "Could not find previous note to cut." << endl;
   exit(1);
}



//////////////////////////////
//
// generateDataLine --
//

void generateDataLine(HumdrumFile& infile, int line, Array<int>& tracknum, 
      Array<int>& subtracknum) {

   int j;
   char buffer[1024] = {0};
   for (j=0; j<tracknum.getSize(); j++) {
      strcat(buffer, ".");
      if (j < tracknum.getSize() - 1) {
         strcat(buffer, "\t");
      }
   }
   infile[line].setLine(buffer);
}



//////////////////////////////
//
// getTrackInfo --
//

void getTrackInfo(HumdrumFile& infile, int trackline, 
      Array<int>& trackcount, Array<int>& subtrackcount) {

   trackcount.setSize(1000);
   trackcount.setSize(0);
   subtrackcount.setSize(1000);
   subtrackcount.setSize(0);

   Array<int> ptrackcounter;
   ptrackcounter.setSize(infile.getMaxTracks()+1);
   ptrackcounter.setAll(0);
   int ptrack;

   int j;

   if (!infile[trackline].isInterpretation()) {
      for (j=0; j<infile[trackline].getFieldCount(); j++) {
         ptrack = infile[trackline].getPrimaryTrack(j);
         trackcount.append(ptrack);
         subtrackcount.append(ptrackcounter[ptrack]);
         ptrackcounter[ptrack]++;
      }
      return;
   }

   for (j=0; j<infile[trackline].getFieldCount(); j++) {
      ptrack = infile[trackline].getPrimaryTrack(j);
      if (strcmp(infile[trackline][j], "*^") == 0) {
         trackcount.append(ptrack);
      } else if ((j>0) && (strcmp(infile[trackline][j], "*v") == 0)) {
         if (strcmp(infile[trackline][j-1], "*v") == 0) { 
            continue;
         }
         ptrack = infile[trackline].getPrimaryTrack(j);
         trackcount.append(ptrack);
      }
   }

   for (j=0; j<trackcount.getSize(); j++) {
      subtrackcount.append(ptrackcounter[trackcount[j]]);
      ptrackcounter[trackcount[j]]++;
   }
}



//////////////////////////////
//
// processFileAuto --  Look for fractional barline directives in the file, 
//    such as:
//              !!BARLINES: 1/2
//    which will be used to add a barline 1/2 of the way through all
//    subsequent bars. This will add a marker:
//              !!barline: absbeat=41/2 
//    at the appropriate place in the music, where the absbeat parameter 
//    is the time position of the barline that will be placed.
//

void processFileAuto(HumdrumFile& infile) {
   infile.analyzeRhythm("4");
   PerlRegularExpression pre;
   RationalNumber fraction = 1;
   int barcounter = 0;
   RationalNumber barstarttime;
   RationalNumber barduration;
   RationalNumber nextbarmessage;
   int dashQ = 0;

   for (int i=0; i<infile.getNumLines(); i++) {

      if ((nextbarmessage > 0) && (infile[i].getAbsBeatR() >= nextbarmessage)) {
         cout << "!!barline: absbeat=" << nextbarmessage;
         if (dashQ) {
            cout << " dash";
         }
         cout << endl;
         if (infile[i].getAbsBeatR() > nextbarmessage) {
            cout << "!!barline: dataline" << endl;
         } 
         barcounter++;
         if (barcounter+1 == fraction.getDenominator()) {
            nextbarmessage = -1;
         } else {
            nextbarmessage = barstarttime + barduration * 
                  fraction * (barcounter + 1);
         }
      }

      if (infile[i].isGlobalComment()) {
         if (pre.search(infile[i][0], "!!BARLINES:\\s*(\\d+)/(\\d+)")) {
            fraction  = atoi(pre.getSubmatch(1));
            fraction /= atoi(pre.getSubmatch(2));
            if (pre.search(infile[i][0], "dash", "i")) {
               dashQ = 1;
            } else {
               dashQ = 0;
            }
            continue;
         }
      }
      if (fraction == 1) {
         cout << infile[i] << endl;
         continue;
      }
      if (infile[i].isBarline()) {
         barcounter = 0;
         barstarttime = infile[i].getAbsBeatR();
         barduration  = infile[i].getMeasureDuration();
         if (barduration <= 0) {
            nextbarmessage = -1;
         } else {
            nextbarmessage = barstarttime + barduration * fraction;
         }
         cout << infile[i] << endl;
         continue;
      }

      cout << infile[i] << endl;
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

  
void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|auto=b", "auto-mark measure subdivisions");
   opts.define("c|convert=b", "convert auto-mark output");

   opts.define("debug=b");            // print debug info
   opts.define("author=b");           // author of program
   opts.define("version=b");          // compilation info
   opts.define("example=b");          // example usages
   opts.define("h|help=b");           // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Aug 2014" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 18 Aug 2014" << endl;
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

   autoQ    = opts.getBoolean("auto");
   convertQ = opts.getBoolean("convert");

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



// md5sum: bc20fe488def24d8fc4d9bcfa45be13d barline.cpp [20140819]
