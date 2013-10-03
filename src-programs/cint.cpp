//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Sep 16 13:53:47 PDT 2013
// Last Modified: Thu Sep 19 16:10:27 PDT 2013
// Filename:      ...museinfo/examples/all/cint.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/cint.cpp
// Syntax:        C++; museinfo
//
// Description:   Calculates counterpoint interval modules in polyphonic
// 		  music.
//
// 		  Crossing code not yet finished.
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

class NoteNode {
   public:
      int b40;      // base-40 pitch number or 0 if a rest, negative if tied
      int line;     // line number in original score of note
      int spine;    // spine number in original score of note
      int measure;  // measure number of note
      int serial;   // number number 
      int mark;     // for marking search matches
      double beatsize; // time signature bottom value which or
                       // 3 times the bottom if compound meter
      NoteNode(void)      { clear(); }
      void clear(void)    { mark = measure = beatsize = serial = b40 = 0; 
                            line = spine = -1; }
      int isRest(void)    { return b40 == 0 ? 1 : 0; }
      int isSustain(void) { return b40 < 0 ? 1 : 0; }
      int isAttack(void)  { return b40 > 0 ? 1 : 0; }
};

#define REST 0
#define RESTINT -1000000
#define RESTSTRING "R"
#define INTERVAL_HARMONIC 1
#define INTERVAL_MELODIC  2
#define MARKNOTES  1

///////////////////////////////////////////////////////////////////////////

// function declarations
void      checkOptions         (Options& opts, int argc, char* argv[]);
void      example              (void);
void      usage                (const char* command);
int       processFile          (HumdrumFile& infile, const char* filename);
void      getKernTracks        (Array<int>& ktracks, HumdrumFile& infile);
int       validateInterval     (Array<Array<NoteNode> >& notes, 
                                int i, int j, int k);
void      printIntervalInfo    (HumdrumFile& infile, int line, 
                                int spine, Array<Array<NoteNode> >& notes, 
                                int noteline, int noteindex, 
                                Array<Array<char> >& abbr);
void      getAbbreviations     (Array<Array<char> >& abbreviations, 
                                Array<Array<char> >& names);
void      getAbbreviation      (Array<char>& abbr, Array<char>& name);
void      extractNoteArray     (Array<Array<NoteNode> >& notes, 
                                HumdrumFile& infile, Array<int>& ktracks, 
                                Array<int>& reverselookup);
int       onlyRests            (Array<NoteNode>& data);
int       hasAttack            (Array<NoteNode>& data);
int       allSustained         (Array<NoteNode>& data);
void      printPitchGrid       (Array<Array<NoteNode> >& notes, 
                                HumdrumFile& infile);
void      getNames             (Array<Array<char> >& names, 
                                Array<int>& reverselookup, HumdrumFile& infile);
void      printLattice         (Array<Array<NoteNode> >& notes, 
                                HumdrumFile& infile, Array<int>& ktracks, 
                                Array<int>& reverselookup, int n);
void      printSpacer          (ostream& out);
int       printInterval        (ostream& out, NoteNode& note1, NoteNode& note2,
                                int type, int octaveadjust = 0);
int       printLatticeItem     (Array<Array<NoteNode> >& notes, int n, 
                                int currentindex, int fileline);
int       printLatticeItemRows (Array<Array<NoteNode> >& notes, int n, 
                                int currentindex, int fileline);
int       printLatticeModule   (ostream& out, Array<Array<NoteNode> >& notes, 
                                int n, int startline, int part1, int part2);
void      printInterleaved     (HumdrumFile& infile, int line, 
                                Array<int>& ktracks, Array<int>& reverselookup, 
                                const char* interstring);
void      printLatticeInterleaved(Array<Array<NoteNode> >& notes, 
                                HumdrumFile& infile, Array<int>& ktracks, 
                                Array<int>& reverselookup, int n);
int       printInterleavedLattice(HumdrumFile& infile, int line, 
                                Array<int>& ktracks, Array<int>& reverselookup,
                                int n, int currentindex,
                                Array<Array<NoteNode> >& notes);
int       printCombinations    (Array<Array<NoteNode> >& notes, 
                                HumdrumFile& infile, Array<int>& ktracks, 
                                Array<int>& reverselookup, int n,
                                Array<Array<SigString> >& retrospective);
void      printAsCombination   (HumdrumFile& infile, int line, 
                                Array<int>& ktracks, Array<int>& reverselookup,
                                const char* interstring);
int       printModuleCombinations(HumdrumFile& infile, int line, 
                                Array<int>& ktracks, Array<int>& reverselookup,
                                int n, int currentindex, 
                                Array<Array<NoteNode> >& notes, 
                                int& matchcount, 
                                Array<Array<SigString> >& retrospective);
int       printCombinationsSuspensions(Array<Array<NoteNode> >& notes, 
                                HumdrumFile& infile, Array<int>& ktracks, 
                                Array<int>& reverselookup, int n,
                                Array<Array<SigString> >& retrospective);
int       printCombinationModule(ostream& out, Array<Array<NoteNode> >& notes, 
                                int n, int startline, int part1, int part2,
                                Array<Array<SigString> >& retrospective,
                                int markstate = 0);
int       printCombinationModulePrepare(ostream& out, 
                                Array<Array<NoteNode> >& notes, int n, 
                                int startline, int part1, int part2,
                                Array<Array<SigString> >& retrospective);
int       getOctaveAdjustForCombinationModule(Array<Array<NoteNode> >& notes, 
                                int n, int startline, int part1, int part2);
void      addMarksToInputData  (HumdrumFile& infile, 
                                Array<Array<NoteNode> >& notes,
                                Array<int>& ktracks,
                                Array<int>& reverselookup);
void      markNote             (HumdrumFile& infile, int line, int col);
void      initializeRetrospective(Array<Array<SigString> >& retrospective, 
                                HumdrumFile& infile, Array<int>& ktracks);
int       getTriangleIndex     (int number, int num1, int num2);

// global variables
Options   options;             // database for command-line arguments
int       debugQ       = 0;      // used with --debug option
int       base40Q      = 0;      // used with --40 option
int       base12Q      = 0;      // used with --12 option
int       base7Q       = 0;      // used with -7 option
int       pitchesQ     = 0;      // used with --pitches option
int       rhythmQ      = 0;      // used with -r option
int       latticeQ     = 0;      // used with -l option
int       interleavedQ = 0;      // used with -L option
int       Chaincount   = 1;      // used with -n option
int       chromaticQ   = 0;      // used with --chromatic option
int       sustainQ     = 0;      // used with -s option
int       zeroQ        = 0;      // used with -z option
int       topQ         = 0;      // used with -t option
int       toponlyQ     = 0;      // used with -T option
int       hparenQ      = 0;      // used with -h option
int       mparenQ      = 0;      // used with -y option
int       parenQ       = 0;      // used with -p option
int       rowsQ        = 0;      // used with --rows option
int       hmarkerQ     = 0;      // used with -h option
int       mmarkerQ     = 0;      // used with -m option
int       attackQ      = 0;      // used with --attacks option
int       rawQ         = 0;      // used with --raw option
int       xoptionQ     = 0;      // used with -x option
int       octaveallQ   = 0;      // used with -O option
int       octaveQ      = 0;      // used with -o option
int       noharmonicQ  = 0;      // used with -H option
int       nomelodicQ   = 0;      // used with -M option
int       norestsQ     = 0;      // used with -R option
int       nounisonsQ   = 0;      // used with -U option
int       filenameQ    = 0;      // used with -f option
int       searchQ      = 0;      // used with --search option
int       markQ        = 0;      // used with --mark option
int       countQ       = 0;      // used with --count option
int       suspensionsQ = 0;      // used with --suspensions option
int       uncrossQ     = 0;      // used with -c option
int       retroQ       = 0;      // used with --retro option
PerlRegularExpression SearchString;


///////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv) {
   checkOptions(options, argc, argv);
   HumdrumStream streamer(options.getArgList());
   HumdrumFile infile;

   int count = 0;
   int totalcount = 0;
   while (streamer.read(infile)) {
      count = processFile(infile, infile.getFileName());
      totalcount += count;
      if (countQ) {
         if (filenameQ) {
            cout << infile.getFileName() << "\t";
            cout << count << endl;
         }
      }
   }

   if (countQ) {
      if (filenameQ) {
         cout << "TOTAL:\t";
      }
      cout << totalcount << endl;
   }

}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processFile -- Do requested analysis on a given file.
//

int processFile(HumdrumFile& infile, const char* filename) {

   Array<Array<NoteNode> > notes;
   Array<Array<char> >     names;
   Array<int>              ktracks;
   Array<int>              reverselookup;

   infile.getTracksByExInterp(ktracks, "**kern");
   notes.setSize(ktracks.getSize());
   reverselookup.setSize(infile.getMaxTracks()+1);
   reverselookup.setAll(-1);

   Array<Array<SigString>> retrospective;
   if (retroQ) {
      initializeRetrospective(retrospective, infile, ktracks);
   }

   if (rhythmQ) {
      infile.analyzeRhythm("4");
   }

   int i;
   for (i=0; i<ktracks.getSize(); i++) {
      reverselookup[ktracks[i]] = i;
      notes[i].setSize(infile.getNumLines());
      notes[i].setSize(0);
   }

   getNames(names, reverselookup, infile);

   PerlRegularExpression pre;

   extractNoteArray(notes, infile, ktracks, reverselookup);

   if (pitchesQ) {
      printPitchGrid(notes, infile); 
      exit(0);
   }

   int count = 0;
   if (latticeQ) {
      printLattice(notes, infile, ktracks, reverselookup, Chaincount);
   } else if (interleavedQ) {
      printLatticeInterleaved(notes, infile, ktracks, reverselookup, 
         Chaincount);
   } else if (suspensionsQ) {
      count = printCombinationsSuspensions(notes, infile, ktracks, 
            reverselookup, Chaincount, retrospective);
   } else {
      count = printCombinations(notes, infile, ktracks, reverselookup, 
            Chaincount, retrospective);
   }


   // handle search results here
   if (markQ) {
      if (count > 0) {
         addMarksToInputData(infile, notes, ktracks, reverselookup);
      }
      cout << infile;
      cout << "!!!RDF**kern: @ = matched note, color=\"#ff0000\"\n";
   } 

   return count;
}



//////////////////////////////
//
// initializeRetrospective --
//

void initializeRetrospective(Array<Array<SigString> >& retrospective, 
      HumdrumFile& infile, Array<int>& ktracks) {

   int columns = ktracks.getSize();
   columns = columns * (columns + 1) / 2; // triangle number of analysis cols.

   retrospective.setSize(columns);
   retrospective.allowGrowth(0);
   int i, j;

   for (i=0; i<retrospective.getSize(); i++) {
      retrospective[i].setSize(infile.getNumLines());
      retrospective.allowGrowth(0);
   }

   SigString token;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isLocalComment()) {
         token = "!";
      } else if (infile[i].isGlobalComment()) {
         token = "!";
      } else if (infile[i].isReferenceRecord()) {
         token = "!!";
      } else if (infile[i].isBarline()) {
         token = infile[i][0];
      } else if (infile[i].isData()) {
         token = ".";
      } else if (infile[i].isInterpretation()) {
         token = "*";
         if (infile[i].isExclusiveInterpretation(0)) {
            token = "**cint";
         }
      }

      for (j=0; j<retrospective.getSize(); j++) {
         retrospective[j][i] = token;
      }
   }

   for (i=0; i<retrospective[0].getSize(); i++) {
      for (j=0; j<retrospective.getSize(); j++) {
         cout << retrospective[j][i];
         if (j < retrospective.getSize() - 1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }
}



//////////////////////////////
//
// printCombinationsSuspensions --
//

int  printCombinationsSuspensions(Array<Array<NoteNode> >& notes, 
      HumdrumFile& infile, Array<int>& ktracks, Array<int>& reverselookup, 
      int n, Array<Array<SigString> >& retrospective) {

   char sbuffer[24096] = {0};

   int oldcountQ = countQ;
   countQ = 1;             // mostly used to suppress intermediate output

   int countsum = 0;

   searchQ    = 1;               // turn on searching

   // Suspensions with length-2 modules
   n = 2;                        // -n 2
   xoptionQ   = 1;               // -x
   strcpy(sbuffer, "");

   strcat(sbuffer, "^7xs 1 6sx -2 8xx$"); 	strcat(sbuffer, "|");
   strcat(sbuffer, "^2sx -2 3xs 2 1xx$"); 	strcat(sbuffer, "|");
   strcat(sbuffer, "^7xs 1 6sx 2 6xx$"); 	strcat(sbuffer, "|");
   strcat(sbuffer, "^11xs 1 10sx -5 15xx$"); 	strcat(sbuffer, "|");
   strcat(sbuffer, "^4xs 1 3sx -5 8xx$"); 	strcat(sbuffer, "|");
   strcat(sbuffer, "^2sx -2 3xs 2 3xx$");	strcat(sbuffer, "|");
   // "9xs 1 8sx -2 10xx" archetype: Jos1405 m10 A&B
   strcat(sbuffer, "^9xs 1 8sx -2 10xx$");	strcat(sbuffer, "|");
   // "4xs 1 3sx 5xx" archetype: Jos1713 m87-88 A&B
   strcat(sbuffer, "^4xs 1 3sx -2 5xx$");	strcat(sbuffer, "|");
   // "11xs 1 10sx 4 8xx" archetype: Jos1402 m23-24 S&B
   strcat(sbuffer, "^11xs 1 10sx 4 8xx$");	

   SearchString.initializeSearchAndStudy(sbuffer);
   countsum += printCombinations(notes, infile, ktracks, reverselookup, n,
                     retrospective);

   // Suspensions with length-3 modules /////////////////////////////////
   n = 3;                        // -n 2
   xoptionQ   = 1;               // -x
   strcpy(sbuffer, "");

   // "7xs 1 6sx 1 5sx 1 6sx" archetype: Jos2721 m27-78 S&T
   strcat(sbuffer, "^7xs 1 6sx 1 5sx 1 6sx$");	strcat(sbuffer, "|");
   // "7xs 1 6sx 1 6sx -2 8xx" archetype: Rue2018 m38-88 S&T
   strcat(sbuffer, "^7xs 1 6sx 1 6sx -2 8xx$");	strcat(sbuffer, "|");
   // "11xs 1 10sx 1 10sx -5 15xx" archetype: Rue2018 m38-88 S&B
   strcat(sbuffer, "^11xs 1 10sx 1 10sx -5 15xx$");	

   SearchString.initializeSearchAndStudy(sbuffer);
   countsum += printCombinations(notes, infile, ktracks, reverselookup, n,
                     retrospective);

   
   // Suspensions with rests modules


   // done with multiple searches.  Mark the notes in the score if required.

   countQ = oldcountQ;

   return countsum;
}



//////////////////////////////
//
// printCombinations --
//

int  printCombinations(Array<Array<NoteNode> >& notes, 
      HumdrumFile& infile, Array<int>& ktracks, Array<int>& reverselookup, 
      int n, Array<Array<SigString> >& retrospective) {
   int i;
   int currentindex = 0;
   int matchcount   = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].hasSpines()) {
         // print all lines here which do not contain spine 
         // information.
         if (!(rawQ || markQ || retroQ || countQ)) {
            cout << infile[i] << "\n";
         }
         continue;
      }

      // At this point there are only four types of lines:
      //    (1) data lines
      //    (2) interpretation lines (lines starting with *)
      //    (3) local comment lines (lines starting with single !)
      //    (4) barlines

      if (infile[i].isInterpretation()) {
         const char* pattern = "*";
         if (strncmp(infile[i][0], "**", 2) == 0) {
            pattern = "**cint";
         } else if (strcmp(infile[i][0], "*-") == 0) {
            pattern = "*-";
         }
         printAsCombination(infile, i, ktracks, reverselookup, pattern);
      } else if (infile[i].isLocalComment()) {
         printAsCombination(infile, i, ktracks, reverselookup, "!");
      } else if (infile[i].isBarline()) {
         printAsCombination(infile, i, ktracks, reverselookup, infile[i][0]);
      } else {
         // print combination data
         currentindex = printModuleCombinations(infile, i, ktracks, 
            reverselookup, n, currentindex, notes, matchcount, retrospective);
      }
      if (!(rawQ || markQ || retroQ || countQ)) {
            cout << "\n";
      }
   }

   return matchcount;
}



//////////////////////////////
//
// printModuleCombinations --
//

int printModuleCombinations(HumdrumFile& infile, int line, Array<int>& ktracks,
      Array<int>& reverselookup, int n, int currentindex, 
      Array<Array<NoteNode> >& notes, int& matchcount, 
      Array<Array<SigString> >& retrospective) {

   int fileline = line;

   while ((currentindex < notes[0].getSize()) 
         && (fileline > notes[0][currentindex].line)) {
      currentindex++;
   }
   if (currentindex >= notes[0].getSize()) {
      if (!(rawQ || markQ || retroQ || countQ)) {
         cout << ".";
         printAsCombination(infile, line, ktracks, reverselookup, ".");
      }
      return currentindex;
   }
   if (notes[0][currentindex].line != fileline) {
      // should never get here.
      printAsCombination(infile, line, ktracks, reverselookup, "?");
      return currentindex;
   }

   // found the index into notes which matches to the current fileline.
   if (currentindex + n >= notes[0].getSize()) {
      // asking for chain longer than rest of available data.
      printAsCombination(infile, line, ktracks, reverselookup, ".");
      return currentindex;
   }

   // printAsCombination(infile, line, ktracks, reverselookup, ".");
   // return currentindex;

   int tracknext;
   int track;
   int j, jj;
   int count = 0;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         if (!(rawQ || markQ || retroQ || countQ)) {
            cout << infile[line][j];
            if (j < infile[line].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         continue;
      }
      track = infile[line].getPrimaryTrack(j);
      if (j < infile[line].getFieldCount() - 1) {
         tracknext = infile[line].getPrimaryTrack(j+1);
      } else {
         tracknext = -23525;
      }
      if (track == tracknext) {
         if (!(rawQ || markQ || retroQ || countQ)) {
            cout << infile[line][j];
            if (j < infile[line].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         continue;
      }

      // print the **kern spine, then check to see if there
      // is some **cint data to print
      if (!(rawQ || markQ || retroQ || countQ)) {
            cout << infile[line][j];
      }
      if ((track != ktracks.last()) && (reverselookup[track] >= 0)) {
         count = ktracks.getSize() - reverselookup[track] - 1;
         for (jj = 0; jj<count; jj++) {
            if (!(rawQ || markQ || retroQ || countQ)) {
               cout << "\t";
            }
            int part1 = reverselookup[track];
            int part2 = part1+1+jj;
            // cout << part1 << "," << part2;
            matchcount += printCombinationModulePrepare(cout, notes, n, 
                  currentindex, part1, part2, retrospective);
         }
      }

      if (!(rawQ || markQ || retroQ || countQ)) {
         if (j < infile[line].getFieldCount() - 1) {
            cout << "\t";
         }
      }
   }

   return currentindex;
}



//////////////////////////////
//
// printCombinationModulePrepare --
//

int printCombinationModulePrepare(ostream& out, Array<Array<NoteNode> >& notes,
       int n, int startline, int part1, int part2, 
       Array<Array<SigString> >& retrospective) {
   int count = 0;
   SSTREAM tempstream;
   int match;
   int status = printCombinationModule(tempstream, notes, n, startline, 
                   part1, part2, retrospective);
   if (status) { 
      tempstream << ends;
      if (searchQ) {
         // Check to see if the extracted module matches to the
         // search query.
         match = SearchString.search(tempstream.CSTRING);
         if (match) {
            count++;
            if (rawQ) {
               cout << tempstream.CSTRING;
               // newline already added somewhere previously.
               // cout << "\n";
            } else {
               // mark notes of the matched module(s) in the note array 
               // for later marking in input score.
               printCombinationModule(tempstream, notes, n, startline, 
                   part1, part2, retrospective, MARKNOTES);
            }
         }
      } else {
         if (retroQ) {
            int column = getTriangleIndex(notes.getSize(), part1, part2);
            retrospective[column][status] = tempstream.CSTRING;
         } else {
            out << tempstream.CSTRING;
         }
      }
   } else {
      if (!(rawQ || markQ || retroQ || countQ || searchQ)) {
         out << ".";
      }
   }

   return count;
}



//////////////////////////////
//
// getTriangleIndex --
//

int getTriangleIndex(int number, int num1, int num2) {
   int triangle = number * (number + 1) / 2;
   // intermediate code, not active yet
   return 0;
}



//////////////////////////////
//
// addMarksToInputData -- mark notes in the score which matched
//     to the search query.
//

void addMarksToInputData(HumdrumFile& infile, 
      Array<Array<NoteNode> >& notes, Array<int>& ktracks,
      Array<int>& reverselookup) {

   // first carry all marks from sustained portions of notes onto their
   // note attacks.
   int i, j;

   int mark = 0;
   int track = 0;
   int markpitch = -1;
 
   for (i=0; i<notes.getSize(); i++) {
      mark = 0;
      for (j=notes[i].getSize()-1; j>=0; j--) {
         if (mark && (-markpitch == notes[i][j].b40)) {
            // In the sustain region between a note
            // attack and the marked sustain. Mark the 
            // sustained region as well (don't know
            // if this behavior might change in the
            // future.
            notes[i][j].mark = mark; 
            continue;
         }
         if (mark && (markpitch == notes[i][j].b40)) {
            // At the start of a notes which was marked.
            // Mark the attack since only note attacks
            // will be marked in the score
            notes[i][j].mark = mark; 
            mark = 0;
            continue;
         }
         if (mark && (markpitch != notes[i][j].b40)) {
            // something strange happened.  Probably
            // an open tie which was not started
            // properly, so just clear mark.
            mark = 0;
         }
         if (notes[i][j].mark) {
            mark = 1;
            markpitch = abs(notes[i][j].b40);
         } else {
            mark = 0;
         }
         
      }
   }

   // a forward loop here into notes array to continue
   // marks to end of sutained region of marked notes
   for (i=0; i<notes.getSize(); i++)  {
      for (j=0; j<notes[i].getSize(); j++) {
         if (notes[i][j].mark) {
            markpitch = -abs(notes[i][j].b40);
            continue;
         } else if (notes[i][j].b40 == markpitch) {
            notes[i][j].mark = 1;
            continue;
         } else {
            markpitch = -1;
         }
      }
   }

   // print mark information:
   // for (j=0; j<notes[0].getSize(); j++) {
   //    for (i=0; i<notes.getSize(); i++) {
   //       cout << notes[i][j].b40;
   //       if (notes[i][j].mark) {
   //          cout << "m";
   //       }
   //       cout << " ";
   //    }
   //    cout << "\n";
   // }


   // now go through the input score placing user-markers onto notes
   // which were marked in the note array.
   int currentindex = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      while ((currentindex < notes[0].getSize()) 
            && (i > notes[0][currentindex].line)) {
         currentindex++;
      }
      if (currentindex >= notes[0].getSize()) {
         continue;
      }
      if (notes[0][currentindex].line != i) {
         continue;
      }

      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            // Don't mark null tokens.
            continue;
         }
         track = infile[i].getPrimaryTrack(j);
         if (reverselookup[track] < 0) {
            continue;
         }
         if (notes[reverselookup[track]][currentindex].mark != 0) {
            markNote(infile, i, j);
         }
      }
   }
}



//////////////////////////////
//
// markNote --
//

void markNote(HumdrumFile& infile, int line, int col) {
   char buffer[1024] = {0};
   strcpy(buffer, infile[line][col]);
   strcat(buffer, "@");
   infile[line].changeField(col, buffer);
}



//////////////////////////////
//
// getOctaveAdjustForCombinationModule -- Find the minim harmonic interval in 
//      the module chain.  If it is greater than an octave, then move it down 
//      below an octave.  If the minimum is an octave, then don't do anything.
//      Not considering crossed voices.
//

int getOctaveAdjustForCombinationModule(Array<Array<NoteNode> >& notes, int n, 
      int startline, int part1, int part2) {

   // if the current two notes are both sustains, then skip
   if ((notes[part1][startline].b40 <= 0) && 
       (notes[part2][startline].b40 <= 0)) {
      return 0;
   }

   if (norestsQ) {
      if (notes[part1][startline].b40 == 0) {
         return 0;
      }
      if (notes[part2][startline].b40 == 0) {
         return 0;
      }
   }

   int i;
   int count = 0;
   int attackcount = 0;
   int hint;

   Array<int> hintlist;
   hintlist.setSize(1000);
   hintlist.setSize(0);

   for (i=startline; i<notes[0].getSize(); i++) {
      if ((notes[part1][i].b40 <= 0) && (notes[part2][i].b40 <= 0)) {
         // skip notes if both are sustained
         continue;
      }
  
      if (attackQ && ((notes[part1][i].b40 <= 0) || 
                      (notes[part2][i].b40 <= 0))) {
         if (attackcount == 0) {
            // not at the start of a pair of attacks.
            return 0;
         }
      }

      // consider  harmonic interval
      if ((notes[part2][i].b40 != 0) && (notes[part1][i].b40 != 0)) {
         hint = abs(notes[part2][i].b40) - abs(notes[part1][i].b40);
         if (uncrossQ && (hint < 0)) {
            hint = -hint;
         }
         hintlist.append(hint);
      }

      // if count matches n, then exit loop
      if ((count == n) && !attackQ) {
         break;
      } 
      count++;

      if ((notes[part1][i].b40 > 0) && (notes[part2][i].b40 > 0)) {
         // keep track of double attacks
         if (attackcount >= n) {
            break;
         } else {
            attackcount++;
         }
      }

   }

   int minimum = 100000;

   for (i=0; i<hintlist.getSize(); i++) {
      if (hintlist[i] < minimum) {
         minimum = hintlist[i];
      }
   }

   if (minimum > 1000) {
     // no intervals found to consider
     return 0;
   }

   if ((minimum >= 0) && (minimum <= 40)) {
      // nothing to do
      return 0;
   }

   if (minimum > 40) {
      return -(minimum/40);
   } else if (minimum < 0) {
      // don't go positive, this will invert the interval.
      return (-minimum)/40;
   }

   //int octaveadjust = -(minimum / 40);

   //if (attackQ && (attackcount == n)) {
   //   return octaveadjust;
   //} else if (count == n) {
   //   return octaveadjust;
   //} else {
   //   // did not find the required number of modules.
   //   return 0;
   //}

   return 0;
}



//////////////////////////////
//
// printCombinationModule -- Similar to printLatticeModule, but harmonic 
//      intervals will not be triggered by a pair of sustained notes.  
//      Print a counterpoint module or module chain given the start notes 
//      and pair of parts to calculate the module (chains) from.  Will not 
//      print anything if the chain length is longer than the note array.  
//      The n parameter will be ignored if --attacks option is used 
//      (--attacks will gnereate a variable length module chain).
//

int printCombinationModule(ostream& out, Array<Array<NoteNode> >& notes, int n, 
      int startline, int part1, int part2, 
      Array<Array<SigString> >& retrospective,
      int markstate) {

   if (norestsQ) {
      if (notes[part1][startline].b40 == 0) {
         return 0;
      }
      if (notes[part2][startline].b40 == 0) {
         return 0;
      }
   }

   int crossing =  0;
   int oldcrossing =  0;

   int octaveadjust = 0;   // used for -o option
   if (octaveQ) {
      octaveadjust = getOctaveAdjustForCombinationModule(notes, n, startline, 
            part1, part2);
   }

   ostream *outp = &out;
   if (rawQ && !searchQ) {
      outp = &cout;
   }

   if (n + startline >= notes[0].getSize()) {
      // definitely nothing to do
      return 0;
   }

   if (notes.getSize() == 0) {
      // nothing to do
      return 0;
   }

   // if the current two notes are both sustains, then skip
   if ((notes[part1][startline].b40 <= 0) && 
       (notes[part2][startline].b40 <= 0)) {
      return 0;
   }

   if (parenQ) {
      (*outp) << "(";
   }

   int i;
   int count = 0;
   int countm = 0;
   int attackcount = 0;

   int lastindex = -1;
   int retroline = 0;

   for (i=startline; i<notes[0].getSize(); i++) {
      if ((notes[part1][i].b40 <= 0) && (notes[part2][i].b40 <= 0)) {
         // skip notes if both are sustained
         continue;
      }

      if (norestsQ) {
         if (notes[part1][i].b40 == 0) {
            return 0;
         }
         if (notes[part2][i].b40 == 0) {
            return 0;
         }
      }
  
      if (attackQ && ((notes[part1][i].b40 <= 0) || 
                      (notes[part2][i].b40 <= 0))) {
         if (attackcount == 0) {
            // not at the start of a pair of attacks.
            return 0;
         }
      }

      // print the melodic intervals (if not the first item in chain)
      if ((count > 0) && !nomelodicQ) {
         if (mparenQ) {
            (*outp) << "{";
         }

         if (nounisonsQ) {
            // suppress modules which contain melodic perfect unisons:
            if ((notes[part1][i].b40 != 0) && 
               (abs(notes[part1][i].b40) == abs(notes[part1][lastindex].b40))) {
               return 0;
            }
            if ((notes[part2][i].b40 != 0) && 
               (abs(notes[part2][i].b40) == abs(notes[part2][lastindex].b40))) {
               return 0;
            }
         }
         // bottom melodic interval:
         if (!toponlyQ) {
            printInterval((*outp), notes[part1][lastindex], 
                          notes[part1][i], INTERVAL_MELODIC);
         }
    
         // print top melodic interval here if requested
         if (topQ || toponlyQ) {
            if (!toponlyQ) {
               printSpacer((*outp));
            }
            // top melodic interval:
            printInterval((*outp), notes[part2][lastindex], 
                          notes[part2][i], INTERVAL_MELODIC);
            if (mmarkerQ) {
               (*outp) << "m";
            }
         }
      
         if (mparenQ) {
            (*outp) << "}";
         }
         printSpacer((*outp));
      }

      countm++;

      // print harmonic interval
      if (!noharmonicQ) {
         if (hparenQ) {
           (*outp) << "[";
         }
         if (markstate) {
            notes[part1][i].mark = 1;
            notes[part2][i].mark = 1;
         } else {
            oldcrossing = crossing;
            crossing = printInterval((*outp), notes[part1][i], notes[part2][i], 
                  INTERVAL_HARMONIC, octaveadjust);
         }
         if (hmarkerQ) {
            (*outp) << "h";
         }
         if (hparenQ) {
           (*outp) << "]";
         }
      }

      // if count matches n, then exit loop
      if ((count == n) && !attackQ) {
         retroline = i;
         break;
      } else {
         if (!noharmonicQ) {
            printSpacer((*outp));
         }
      }
      lastindex = i;
      count++;

      if ((notes[part1][i].b40 > 0) && (notes[part2][i].b40 > 0)) {
         // keep track of double attacks
         if (attackcount >= n) {
            retroline = i;
            break;
         } else {
            attackcount++;
         }
      }

   }

   if (parenQ) {
      (*outp) << ")";
   }

   if (rawQ) {
      (*outp) << "\n";
   }

   if (attackQ && (attackcount == n)) {
      return retroline;
   } else if ((countm>1) && (count == n)) {
      return retroline;
   } else {
      // did not print the required number of modules.
      return 0;
   }

   return 0;
}



//////////////////////////////
//
// printAsCombination --
//

void printAsCombination(HumdrumFile& infile, int line, Array<int>& ktracks, 
    Array<int>& reverselookup, const char* interstring) {

   if (rawQ || markQ || retroQ || countQ) {
      return;
   }

   Array<int> done;
   done.setSize(ktracks.getSize());
   done.setAll(0);
   int track;
   int tracknext;
   int count;

   int j, jj;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         cout << infile[line][j];
         if (j < infile[line].getFieldCount() - 1) {
            cout << "\t";
         }
         continue;
      }
      track = infile[line].getPrimaryTrack(j);
      if (j < infile[line].getFieldCount() - 1) {
         tracknext = infile[line].getPrimaryTrack(j+1);
      } else {
         tracknext = -23525;
      }
      if (track == tracknext) {
         cout << infile[line][j];
         if (j < infile[line].getFieldCount() - 1) {
            cout << "\t";
         }
         continue;
      }

      // print the **kern spine, then check to see if there
      // is some **cint data to print
      // ggg
      cout << infile[line][j];

      if (reverselookup[track] >= 0) {
         count = ktracks.getSize() - reverselookup[track] - 1;
         for (jj=0; jj<count; jj++) {
            cout << "\t" << interstring;
         }
      }

      if (j < infile[line].getFieldCount() - 1) {
         cout << "\t";
      }
   }
}



//////////////////////////////
//
// printLatticeInterleaved --
//

void printLatticeInterleaved(Array<Array<NoteNode> >& notes, 
      HumdrumFile& infile, Array<int>& ktracks, Array<int>& reverselookup, 
      int n) {
   int currentindex = 0;
   int i;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].hasSpines()) {
         // print all lines here which do not contain spine 
         // information.
         if (!rawQ) {
            cout << infile[i] << "\n";
         }
         continue;
      }

      // At this point there are only four types of lines:
      //    (1) data lines
      //    (2) interpretation lines (lines starting with *)
      //    (3) local comment lines (lines starting with single !)
      //    (4) barlines

      if (infile[i].isInterpretation()) {
         const char* pattern = "*";
         if (strncmp(infile[i][0], "**", 2) == 0) {
            pattern = "**cint";
         } else if (strcmp(infile[i][0], "*-") == 0) {
            pattern = "*-";
         }
         printInterleaved(infile, i, ktracks, reverselookup, pattern);
      } else if (infile[i].isLocalComment()) {
         printInterleaved(infile, i, ktracks, reverselookup, "!");
      } else if (infile[i].isBarline()) {
         printInterleaved(infile, i, ktracks, reverselookup, infile[i][0]);
      } else {
         // print interleaved data
         currentindex = printInterleavedLattice(infile, i, ktracks, 
            reverselookup, n, currentindex, notes);
      }
      if (!rawQ) {
         cout << "\n";
      }
   }
}



//////////////////////////////
//
// printInterleavedLattice --
//

int printInterleavedLattice(HumdrumFile& infile, int line, Array<int>& ktracks,
      Array<int>& reverselookup, int n, int currentindex, 
      Array<Array<NoteNode> >& notes) {

   int fileline = line;

   while ((currentindex < notes[0].getSize()) 
         && (fileline > notes[0][currentindex].line)) {
      currentindex++;
   }
   if (currentindex >= notes[0].getSize()) {
      if (!rawQ) {
         cout << ".";
         printInterleaved(infile, line, ktracks, reverselookup, ".");
      }
      return currentindex;
   }
   if (notes[0][currentindex].line != fileline) {
      // should never get here.
      printInterleaved(infile, line, ktracks, reverselookup, "?");
      return currentindex;
   }

   // found the index into notes which matches to the current fileline.
   if (currentindex + n >= notes[0].getSize()) {
      // asking for chain longer than rest of available data.
      printInterleaved(infile, line, ktracks, reverselookup, ".");
      return currentindex;
   }

   int tracknext;
   int track;
   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         if (!rawQ) {
            cout << infile[line][j];
            if (j < infile[line].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         continue;
      }
      track = infile[line].getPrimaryTrack(j);
      if (j < infile[line].getFieldCount() - 1) {
         tracknext = infile[line].getPrimaryTrack(j+1);
      } else {
         tracknext = -23525;
      }
      if (track == tracknext) {
         if (!rawQ) {
            cout << infile[line][j];
            if (j < infile[line].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         continue;
      }

      // print the **kern spine, then check to see if there
      // is some **cint data to print
      if (!rawQ) {
         cout << infile[line][j];
      }
      if ((track != ktracks.last()) && (reverselookup[track] >= 0)) {
         if (!rawQ) {
            cout << "\t";
         }
         int part1 = reverselookup[track];
         int part2 = part1+1;
         // cout << part1 << "," << part2;
         printLatticeModule(cout, notes, n, currentindex, part1, part2);
      }

      if (!rawQ) {
         if (j < infile[line].getFieldCount() - 1) {
            cout << "\t";
         }
      }
   }

   return currentindex;
}



//////////////////////////////
//
// printInterleaved --
//

void printInterleaved(HumdrumFile& infile, int line, Array<int>& ktracks, 
    Array<int>& reverselookup, const char* interstring) {

   Array<int> done;
   done.setSize(ktracks.getSize());
   done.setAll(0);
   int track;
   int tracknext;

   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (!infile[line].isExInterp(j, "**kern")) {
         if (!rawQ) {
            cout << infile[line][j];
            if (j < infile[line].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         continue;
      }
      track = infile[line].getPrimaryTrack(j);
      if (j < infile[line].getFieldCount() - 1) {
         tracknext = infile[line].getPrimaryTrack(j+1);
      } else {
         tracknext = -23525;
      }
      if (track == tracknext) {
         if (!rawQ) {
            cout << infile[line][j];
            if (j < infile[line].getFieldCount() - 1) {
               cout << "\t";
            }
         }
         continue;
      }

      // print the **kern spine, then check to see if there
      // is some **cint data to print
      if (!rawQ) {
         cout << infile[line][j];

         if ((track != ktracks.last()) && (reverselookup[track] >= 0)) {
            cout << "\t" << interstring;
         }

         if (j < infile[line].getFieldCount() - 1) {
            cout << "\t";
         }
      }
   }
}



//////////////////////////////
//
// printLattice --
//

void printLattice(Array<Array<NoteNode> >& notes, HumdrumFile& infile, 
      Array<int>& ktracks, Array<int>& reverselookup, int n) {

   int i;
   int ii = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!rawQ) {
         cout << infile[i];
      }
      if (strncmp(infile[i][0], "**", 2) == 0) {
         if (!rawQ) {
            cout << "\t**cint\n";
         }
         continue;
      }
      if (infile[i].isData()) {
         if (!rawQ) {
            cout << "\t";
         }
         if (rowsQ) {
            ii = printLatticeItemRows(notes, n, ii, i);
         } else {
            ii = printLatticeItem(notes, n, ii, i);
         }
         if (!rawQ) {
            cout << "\n";
         }
         continue;
      }
      if (infile[i].isBarline()) {
         if (!rawQ) {
            cout << "\t" << infile[i][0] << "\n";
         }
         continue;
      }
      if (infile[i].isInterpretation()) {
         if (!rawQ) {
            cout << "\t*\n";
         }
         continue;
      }
      if (infile[i].isLocalComment()) {
         if (!rawQ) {
            cout << "\t!\n";
         }
         continue;
      }
   }

}


//////////////////////////////
//
// printLatticeModule -- print a counterpoint module or module chain given
//      the start notes and pair of parts to calculate the module
//      (chains) from.  Will not print anything if the chain length
//      is longer than the note array.
//

int printLatticeModule(ostream& out, Array<Array<NoteNode> >& notes, int n, 
      int startline, int part1, int part2) {

   if (n + startline >= notes[0].getSize()) {
      return 0;
   }

   if (parenQ) {
      out << "(";
   }

   int i;
   for (i=0; i<n; i++) {
      // print harmonic interval
      if (hparenQ) {
         out << "[";
      }
      printInterval(out, notes[part1][startline+i], 
         notes[part2][startline+i], INTERVAL_HARMONIC);
      if (hmarkerQ) {
         out << "h";
      }
      if (hparenQ) {
         out << "]";
      }
      printSpacer(out);

      // print melodic interal(s)
      if (mparenQ) {
         out << "{";
      }
      // bottom melodic interval:
      if (!toponlyQ) {
         printInterval(out, notes[part1][startline+i], 
                       notes[part1][startline+i+1], INTERVAL_MELODIC);
      }
 
      // print top melodic interval here if requested
      if (topQ || toponlyQ) {
         if (!toponlyQ) {
            printSpacer(out);
         }
         // top melodic interval:
         printInterval(out, notes[part2][startline+i], 
                       notes[part2][startline+i+1], INTERVAL_MELODIC);
         if (mmarkerQ) {
            out << "m";
         }
      }

      if (mparenQ) {
         out << "}";
      }
      printSpacer(out);
   }

   // print last harmonic interval
   if (hparenQ) {
     out << "[";
   }
   printInterval(out, notes[part1][startline+n], 
         notes[part2][startline+n], INTERVAL_HARMONIC);
   if (hmarkerQ) {
      out << "h";
   }
   if (hparenQ) {
     out << "]";
   }

   if (parenQ) {
      out << ")";
   }

   return 1;
}



//////////////////////////////
//
// printLatticeItemRows -- Row form of the lattice.
//

int printLatticeItemRows(Array<Array<NoteNode> >& notes, int n, 
      int currentindex, int fileline) {

   while ((currentindex < notes[0].getSize()) 
         && (fileline > notes[0][currentindex].line)) {
      currentindex++;
   }
   if (currentindex >= notes[0].getSize()) {
      if (!rawQ) {
         cout << ".";
      }
      return currentindex;
   }
   if (notes[0][currentindex].line != fileline) {
      // should never get here.
      if (!rawQ) {
         cout << "?";
      }
      return currentindex;
   }

   // found the index into notes which matches to the current fileline.
   if (currentindex + n >= notes[0].getSize()) {
      // asking for chain longer than rest of available data.
      if (!rawQ) {
         cout << ".";
      }
      return currentindex;
   }

   SSTREAM tempstream;
   int j;
   int counter = 0;

   for (j=0; j<notes.getSize()-1; j++) {
      // iterate through each part, printing the module
      // for adjacent parts.
      counter += printLatticeModule(tempstream, notes, n, currentindex, j, j+1);
      if (j < notes.getSize()-2) {
         printSpacer(tempstream);
      }
   }

   if (!rawQ) {
      if (counter == 0) {
         cout << ".";
      } else {
         tempstream << ends;
         cout << tempstream.CSTRING;
      }
   }

   return currentindex;
}



//////////////////////////////
//
// printLatticeItem --
//

int printLatticeItem(Array<Array<NoteNode> >& notes, int n, int currentindex, 
      int fileline) {
   while ((currentindex < notes[0].getSize()) 
         && (fileline > notes[0][currentindex].line)) {
      currentindex++;
   }
   if (currentindex >= notes[0].getSize()) {
      if (!rawQ) {
         cout << ".";
      }
      return currentindex;
   }
   if (notes[0][currentindex].line != fileline) {
      // should never get here.
      if (!rawQ) {
         cout << "??";
      }
      return currentindex;
   }

   // found the index into notes which matches to the current fileline.
   if (currentindex + n >= notes[0].getSize()) {
      // asking for chain longer than rest of available data.
      if (!rawQ) {
         cout << ".";
      }
      return currentindex;
   }

   int count;
   int melcount;
   int j;
   if (parenQ) {
      cout << "(";
   }
   for (count = 0; count < n; count++) {
      // print harmonic intervals
      if (hparenQ) {
         cout << "[";
      }
      for (j=0; j<notes.getSize()-1; j++) {
         printInterval(cout, notes[j][currentindex+count], 
               notes[j+1][currentindex+count], INTERVAL_HARMONIC);
         if (j < notes.getSize()-2) {
            printSpacer(cout);
         }
      }
      if (hparenQ) {
         cout << "]";
      }
      printSpacer(cout);

      // print melodic intervals
      if (mparenQ) {
         cout << "{";
      }
      melcount = notes.getSize()-1;
      if (topQ) {
         melcount++;
      }
      for (j=0; j<melcount; j++) {
         printInterval(cout, notes[j][currentindex+count], 
               notes[j][currentindex+count+1], INTERVAL_MELODIC);
         if (j < melcount-1) {
            printSpacer(cout);
         }
      }
      if (mparenQ) {
         cout << "}";
      }
      printSpacer(cout);

   }
   // print last sequence of harmonic intervals
   if (hparenQ) {
      cout << "[";
   }
   for (j=0; j<notes.getSize()-1; j++) {
      printInterval(cout, notes[j][currentindex+n], 
            notes[j+1][currentindex+n], INTERVAL_HARMONIC);
      if (j < notes.getSize()-2) {
         printSpacer(cout);
      }
   }
   if (hparenQ) {
      cout << "]";
   }
   if (parenQ) {
      cout << ")";
   }

   if (rawQ) {
      cout << "\n";
   }

   return currentindex;
}



//////////////////////////////
//
// printInterval --
//

int printInterval(ostream& out, NoteNode& note1, NoteNode& note2,
      int type, int octaveadjust) {
   if ((note1.b40 == REST) || (note2.b40 == REST)) {
      out << RESTSTRING;
      return 0;
   }
   int cross = 0;
   int pitch1 = abs(note1.b40);
   int pitch2 = abs(note2.b40);
   int interval = pitch2 - pitch1;

   if ((type == INTERVAL_HARMONIC) && (interval < 0)) {
      cross = 1;
      if (uncrossQ) {
         interval = -interval;
      }
   } else {
      interval = interval + octaveadjust  * 40;
   }

   if ((type == INTERVAL_HARMONIC) && (octaveallQ)) {
      if (interval <= -40) {
         interval = interval + 4000;
      }
      if (interval > 40) {
         if (interval % 40 == 0) {
            interval = 40;
         } else {
            interval = interval % 40;
         }
      } else if (interval < 0) {
         interval = interval + 40;
      }
   }
   if (base12Q && !chromaticQ) {
      interval = Convert::base40ToMidiNoteNumber(interval + 40*4 + 2) - 12*5;
      if ((type == INTERVAL_HARMONIC) && (octaveallQ)) {
         if (interval <= -12) {
            interval = interval + 1200;
         }
         if (interval > 12) {
            if (interval % 12 == 0) {
               interval = 12;
            } else {
               interval = interval % 12;
            }
         } else if (interval < 0) {
            interval = interval + 12;
         }
      }
      interval = interval + octaveadjust  * 12;
   } else if (base7Q && !chromaticQ) {
      interval = Convert::base40ToDiatonic(interval + 40*4 + 2) - 7*4;
      if ((type == INTERVAL_HARMONIC) && (octaveallQ)) {
         if (interval <= -7) {
            interval = interval + 700;
         }
         if (interval > 7) {
            if (interval % 7 == 0) {
               interval = 7;
            } else {
               interval = interval % 7;
            }
         } else if (interval < 0) {
            interval = interval + 7;
         }
      }
      interval = interval + octaveadjust  * 7;
   }


   if (chromaticQ) {
      char buffer[1024] = {0};
      out << Convert::base40ToIntervalAbbr(buffer, interval);
   } else {
      int negative = 1;
      if (interval < 0) {
         negative = -1; 
         interval = -interval;
      }
      if (base7Q && !zeroQ) {
         out << negative * (interval+1);
      } else {
         out << negative * interval;
      }
   }

   if (sustainQ || ((type == INTERVAL_HARMONIC) && xoptionQ)) {
      // print sustain/attack information of intervals.
      if (note1.b40 < 0) {
         out << "s";
      } else {
         out << "x";
      }
      if (note2.b40 < 0) {
         out << "s";
      } else {
         out << "x";
      }
   }

   return cross;
}



//////////////////////////////
//
// printSpacer -- space or comma...
//

void printSpacer(ostream& out) {
   out << " ";
}



//////////////////////////////
//
// printPitchGrid -- print the pitch grid from which all counterpoint
//      modules are calculated.
//

void printPitchGrid(Array<Array<NoteNode> >& notes, HumdrumFile& infile) {
   int i = 0;
   int j = 0;
   int pitch;
   int abspitch;
   int newpitch;
   int partcount;
   int line;
   double beat;

   if (base40Q) {
      partcount = notes.getSize();

      if (rhythmQ) {
         cout << "**absq\t";
         cout << "**bar\t";
         cout << "**beat\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "**b40";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;
      for (i=0; i<notes[0].getSize(); i++) {
         if (rhythmQ) {
            line = notes[0][i].line;
            beat = (infile[line].getBeat()-1.0) * notes[0][i].beatsize + 1; 
            cout << infile[line].getAbsBeat() << "\t";
            cout << notes[0][i].measure << "\t";
            cout << beat << "\t";
         }
         for (j=0; j<notes.getSize(); j++) {
            cout << notes[j][i].b40;
            if (j < notes.getSize()-1) {
               cout << "\t";
            }
         }
         cout << endl;
      }
      if (rhythmQ) {
         cout << "*-\t";
         cout << "*-\t";
         cout << "*-\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "*-";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;
   } else if (base7Q) {
      partcount = notes.getSize();

      if (rhythmQ) {
         cout << "**absq\t";
         cout << "**bar\t";
         cout << "**beat\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "**b7";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;

      for (i=0; i<notes[0].getSize(); i++) {
         if (rhythmQ) {
            line = notes[0][i].line;
            beat = (infile[line].getBeat()-1.0) * notes[0][i].beatsize + 1; 
            cout << infile[line].getAbsBeat() << "\t";
            cout << notes[0][i].measure << "\t";
            cout << beat << "\t";
         }
         for (j=0; j<notes.getSize(); j++) {
            pitch = notes[j][i].b40;
            abspitch = abs(pitch);
            if (pitch == 0) {
               // print rest
               cout << 0;
            } else {
               newpitch = Convert::base40ToDiatonic(abspitch);
               if (pitch < 0) {
                  newpitch = -newpitch;
               }
               cout << newpitch;
            }
            if (j < notes.getSize()-1) {
               cout << "\t";
            }
         }
         cout << endl;
      }
      if (rhythmQ) {
         cout << "*-\t";
         cout << "*-\t";
         cout << "*-\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "*-";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;
   } else if (base12Q) {
      partcount = notes.getSize();

      if (rhythmQ) {
         cout << "**absq\t";
         cout << "**bar\t";
         cout << "**beat\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "**b12";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;

      for (i=0; i<notes[0].getSize(); i++) {
         if (rhythmQ) {
            line = notes[0][i].line;
            beat = (infile[line].getBeat()-1) * notes[0][i].beatsize + 1; 
            cout << infile[line].getAbsBeat() << "\t";
            cout << notes[0][i].measure << "\t";
            cout << beat << "\t";
         }
         for (j=0; j<notes.getSize(); j++) {
            pitch = notes[j][i].b40;
            if (pitch == 0) {
               // print rest
               cout << 0;
            } else {
               abspitch = abs(pitch);
               newpitch = Convert::base40ToMidiNoteNumber(abspitch);
               if (pitch < 0) {
                  newpitch = -newpitch;
               }
               cout << newpitch;
            }
            if (j < notes.getSize()-1) {
               cout << "\t";
            }
         }
         cout << endl;
      }
      if (rhythmQ) {
         cout << "*-\t";
         cout << "*-\t";
         cout << "*-\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "*-";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;
   } else {
      // print as Humdrum **kern data
      char buffer[1024] = {0};
      partcount = notes.getSize();

      if (rhythmQ) {
         cout << "**absq\t";
         cout << "**bar\t";
         cout << "**beat\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "**kern";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;

      for (i=0; i<notes[0].getSize(); i++) {
         if (rhythmQ) {
            line = notes[0][i].line;
            beat = (infile[line].getBeat()-1) * notes[0][i].beatsize + 1; 
            cout << infile[line].getAbsBeat() << "\t";
            cout << notes[0][i].measure << "\t";
            cout << beat << "\t";
         }
         for (j=0; j<notes.getSize(); j++) {
            pitch = notes[j][i].b40;
            abspitch = abs(pitch);
            if (pitch == 0) {
               cout << "r";
            } else {
               if ((pitch > 0) && (i<notes[j].getSize()-1) && 
                   (notes[j][i+1].b40 == -abspitch)) {
                  // start of a note which continues into next 
                  // sonority.
                  cout << "[";
               }
               cout << Convert::base40ToKern(buffer, abspitch);
               // print tie continue/termination as necessary.
               if (pitch < 0) {
                  if ((i < notes[j].getSize() - 1) && 
                      (notes[j][i+1].b40 == notes[j][i].b40)) {
                    // note sustains further
                    cout << "_";
                  } else {
                    // note does not sustain any further.
                    cout << "]";
                  }
               }
            }
            if (j < notes.getSize()-1) {
               cout << "\t";
            }
         }
         cout << endl;
      }

      if (rhythmQ) {
         cout << "*-\t";
         cout << "*-\t";
         cout << "*-\t";
      }
      for (i=0; i<partcount; i++) {
         cout << "*-";
         if (i < partcount - 1) {
            cout << "\t";
         }
      }
      cout << endl;
   }
}



//////////////////////////////
//
// extractNoteArray --
//

void extractNoteArray(Array<Array<NoteNode> >& notes, HumdrumFile& infile,
      Array<int>& ktracks, Array<int>& reverselookup) {

   PerlRegularExpression pre;

   Array<NoteNode> current;
   current.setSize(ktracks.getSize());
  
   Array<double> beatsizes;
   beatsizes.setSize(infile.getMaxTracks()+1);
   beatsizes.setAll(1);

   int sign;
   int track = 0;
   int index;
   int i, j, ii, jj;

   int snum = 0;
   int measurenumber = 0;
   int tempmeasurenum = 0;
   double beatsize = 1.0;
   int topnum, botnum;
   
   for (i=0; i<infile.getNumLines(); i++) {
      if (debugQ) {
         cout << "PROCESSING LINE: " << i << "\t" << infile[i] << endl;
      }
      if (infile[i].isMeasure()) {
         tempmeasurenum = infile.getMeasureNumber(i);
         if (tempmeasurenum >= 0) {
            measurenumber = tempmeasurenum;
         }
      }
      for (j=0; j<current.getSize(); j++) {
         current[j].clear();
         current[j].measure = measurenumber;
         current[j].line = i;
      }

      if (infile[i].isMeasure() && (strstr(infile[i][0], "||") != NULL)) {
         // double barline (terminal for Josquin project), so add a row
         // of rests to prevent cint melodic interval identification between
         // adjacent notes in different sections.
         for (j=0; j<notes.getSize(); j++) {
            notes[j].append(current[j]);
         }
      }
      if (infile[i].isInterpretation()) {
         // search for time signatures from which to extract beat information.
         for (j=0; j<infile[i].getFieldCount(); j++) {
            track = infile[i].getPrimaryTrack(j);
            if (pre.search(infile[i][j], "^\\*M(\\d+)/(\\d+)")) {
               // deal with 3%2 in denominator later...
               topnum = atoi(pre.getSubmatch(1));
               botnum = atoi(pre.getSubmatch(2));
               beatsize = botnum;
               if (((topnum % 3) == 0) && (topnum > 3) && (botnum > 1)) {
                  // compound meter
                  // fix later
                  beatsize = botnum / 3;
               }
               beatsizes[track] = beatsize / 4.0;
            } else if (strcmp(infile[i][j], "*met(C|)") == 0) {
               // MenCutC, use 2 as the "beat"
               beatsizes[track] = 2.0 / 4.0;
            }
         }
      }
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         sign = 1;
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         track = infile[i].getPrimaryTrack(j);
         index = reverselookup[track];
         current[index].line  = i;
         current[index].spine = j;
         current[index].beatsize = beatsizes[track];
         if (strcmp(infile[i][j], ".") == 0) {
            sign = -1;
            ii = infile[i].getDotLine(j);
            jj = infile[i].getDotSpine(j);
         } else {
            ii = i;
            jj = j;
         }
         if (strchr(infile[ii][jj], 'r') != NULL) {
            current[index].b40 = 0;
            current[index].serial = ++snum;
            continue;
         }
         if (strcmp(infile[ii][jj], ".") == 0) {
            current[index].b40 = 0;
            current[index].serial = snum;
         }
         current[index].b40 = Convert::kernToBase40(infile[ii][jj]);
         if (strchr(infile[ii][jj], '_') != NULL) {
            sign = -1;
            current[index].serial = snum;
         }
         if (strchr(infile[ii][jj], ']') != NULL) {
            sign = -1;
            current[index].serial = snum;
         }
         current[index].b40 *= sign;
         if (sign > 0) {
            current[index].serial = ++snum;
         }
      }
      if (onlyRests(current) && onlyRests(notes.last())) {
         // don't store more than one row of rests in the data array.
         continue;
      }
      if (allSustained(current)) {
         // don't store sonorities which are purely sutained
         // (may need to be updated with a --sustain option implementation)
         continue;
      }
      for (j=0; j<notes.getSize(); j++) {
         notes[j].append(current[j]);
      }
   }
}



//////////////////////////////
//
// onlyRests -- returns true if all NoteNodes are for rests
//

int onlyRests(Array<NoteNode>& data) {
   int i;
   for (i=0; i<data.getSize(); i++) {
      if (!data[i].isRest()) {
         return 0;
      }
   }
   return 1;
}



//////////////////////////////
//
// hasAttack -- returns true if all NoteNodes are for rests
//

int hasAttack(Array<NoteNode>& data) {
   int i;
   for (i=0; i<data.getSize(); i++) {
      if (data[i].isAttack()) {
         return 1;
      }
   }
   return 0;
}




//////////////////////////////
//
// allSustained -- returns true if all NoteNodes are sustains
//    or rests (but not all rests).
//

int allSustained(Array<NoteNode>& data) {
   int i;
   int hasnote = 0;
   for (i=0; i<data.getSize(); i++) {
      if (data[i].b40 != 0) {
         hasnote = 1;
      }
      if (data[i].isAttack()) {
         return 0;
      }
   }
   if (hasnote == 0) {
      return 0;
   } 
   return 1;
}



//////////////////////////////
//
// getAbbreviations --
//

void getAbbreviations(Array<Array<char> >& abbreviations, 
      Array<Array<char> >& names) {


   abbreviations.setSize(names.getSize());
   int i;
   for (i=0; i<names.getSize(); i++) {
      getAbbreviation(abbreviations[i], names[i]);     
   }
}



//////////////////////////////
//
// getAbbreviation --
//

void getAbbreviation(Array<char>& abbr, Array<char>& name) {
   PerlRegularExpression pre;
   abbr.setSize(strlen(name.getBase())+1);
   strcpy(abbr.getBase(), name.getBase());
   pre.sar(abbr, "(?<=[a-zA-Z])[a-zA-Z]*", "", "");
   pre.tr(abbr, "123456789", "abcdefghi");
}



//////////////////////////////
//
// getKernTracks -- return a list of track number for **kern spines.
//

void getKernTracks(Array<int>& ktracks, HumdrumFile& infile) {
   int i, j;
   ktracks.setSize(infile.getMaxTracks());
   ktracks.setSize(0);
   int track;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (infile[i].isExInterp(j, "**kern")) {
            track = infile[i].getPrimaryTrack(j);
            ktracks.append(track);
         }
      }
      break;
   }
}



//////////////////////////////
//
// getNames -- get the names of each column if they have one.
//

void getNames(Array<Array<char> >& names, Array<int>& reverselookup, 
      HumdrumFile& infile) {

   names.setSize(reverselookup.getSize()-1);
   names.allowGrowth(0);
   char buffer[1024] = {0};
   int value;
   PerlRegularExpression pre;
   int i;
   int j;
   int track;
 
   for (i=0; i<names.getSize(); i++) {
      value = reverselookup.getSize() - i;
      sprintf(buffer, "%d", value);
      names[i].setSize(strlen(buffer)+1);
      strcpy(names[i].getBase(), buffer);
   }

   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         // stop looking for instrument name after the first data line
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*I\"(.*)", "")) {
            track = infile[i].getPrimaryTrack(j);
            strcpy(buffer, pre.getSubmatch(1));
            names[reverselookup[track]].setSize(strlen(buffer)+1);
            strcpy(names[reverselookup[track]].getBase(), buffer);
         }
      }
   }

   if (debugQ) {
      for (i=0; i<names.getSize(); i++) {
         cout << i << ":\t" <<  names[i] << endl;
      }
   }

}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("base-40|base40|b40|40=b", 
         "display pitches/intervals in base-40");
   opts.define("base-12|base12|b12|12=b", 
         "display pitches/intervals in base-12");
   opts.define("base-7|base7|b7|7|diatonic=b", 
         "display pitches/intervals in base-7");
   opts.define("pitch|pitches=b", 
         "display pitch grid used to calculate modules");
   opts.define("r|rhythm=b", "display rhythmic positions of notes");
   opts.define("f|filename=b", "display filenames with --count");
   opts.define("raw=b", "display only modules without formatting");
   opts.define("c|uncross=b", "uncross crossed voices when creating modules");
   opts.define("retro|retrospective=b", 
                  "Retrospective module display in the score");
   opts.define("suspension|suspensions=b", "mark suspensions");
   opts.define("rows|row=b", "display lattices in row form");
   opts.define("L|interleaved-lattice=b", "display interleaved lattices");
   opts.define("q|harmonic-parentheses=b", 
                  "put square brackets around harmonic intervals");
   opts.define("h|harmonic-marker=b", 
                  "put h character after harmonic intervals");
   opts.define("m|melodic-marker=b", 
                  "put m character after melodic intervals");
   opts.define("y|melodic-parentheses=b", 
                  "put curly braces around melodic intervals");
   opts.define("p|parentheses=b", "put parentheses around modules intervals");
   opts.define("l|lattice=b", "calculate lattice");
   opts.define("s|sustain=b", "display sustain/attack states of notes");
   opts.define("o|octave=b", "reduce compound intervals to within an octave");
   opts.define("H|no-harmonic=b", "don't display harmonic intervals");
   opts.define("M|no-melodic=b", "don't display melodic intervals");
   opts.define("t|top=b", "display top melodic interval of modules");
   opts.define("T|top-only=b", "display only top melodic interval of modules");
   opts.define("U|no-melodic-unisons=b", "no melodic perfect unisons");
   opts.define("attacks|attack=b", 
         "start/stop module chains on pairs of note attacks");
   opts.define("z|zero=b", "display diatonic intervals with 0 offset");
   opts.define("x|xoption=b", 
         "display attack/sustain information on harmonic intervals only");
   opts.define("n|chain=i:1", "number of sequential modules");
   opts.define("R|no-rest|no-rests|norest|norests=b", 
         "number of sequential modules");
   opts.define("O|octave-all=b", 
         "transpose all harmonic intervals to within an octave");
   opts.define("chromatic=b", 
         "display intervals as diatonic intervals with chromatic alterations");
   opts.define("search=s:", "search string");
   opts.define("mark=b", "mark matches notes from searches in data");
   opts.define("count=b", "count matched modules from search query");
   opts.define("debug=b");              // determine bad input line num
   opts.define("author=b");             // author of program
   opts.define("version=b");            // compilation info
   opts.define("example=b");            // example usages
   opts.define("help=b");               // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, September 2013" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 23 September 2013" << endl;
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


   // dispay as base-7 by default
   base7Q = 1;

   base40Q    = opts.getBoolean("base-40");
   base12Q    = opts.getBoolean("base-12");
   chromaticQ = opts.getBoolean("chromatic");
   zeroQ      = opts.getBoolean("zero");

   if (base40Q) {
      base12Q = 0;
      base7Q = 0;
      zeroQ = 0;
   }

   if (base12Q) {
      base40Q = 0;
      base7Q = 0;
      zeroQ = 0;
   }

   pitchesQ     = opts.getBoolean("pitches");
   debugQ       = opts.getBoolean("debug");
   rhythmQ      = opts.getBoolean("rhythm");
   latticeQ     = opts.getBoolean("lattice");
   sustainQ     = opts.getBoolean("sustain");
   topQ         = opts.getBoolean("top");
   toponlyQ     = opts.getBoolean("top-only");
   hparenQ      = opts.getBoolean("harmonic-parentheses");
   mparenQ      = opts.getBoolean("melodic-parentheses");
   parenQ       = opts.getBoolean("parentheses");
   rowsQ        = opts.getBoolean("rows");
   hmarkerQ     = opts.getBoolean("harmonic-marker");
   interleavedQ = opts.getBoolean("interleaved-lattice");
   mmarkerQ     = opts.getBoolean("melodic-marker");
   attackQ      = opts.getBoolean("attacks");
   rawQ         = opts.getBoolean("raw");
   xoptionQ     = opts.getBoolean("x");
   octaveallQ   = opts.getBoolean("octave-all");
   octaveQ      = opts.getBoolean("octave");
   noharmonicQ  = opts.getBoolean("no-harmonic");
   nomelodicQ   = opts.getBoolean("no-melodic");
   norestsQ     = opts.getBoolean("no-rests");
   nounisonsQ   = opts.getBoolean("no-melodic-unisons");
   Chaincount   = opts.getInteger("n");
   searchQ      = opts.getBoolean("search");
   markQ        = opts.getBoolean("mark");
   countQ       = opts.getBoolean("count");
   filenameQ    = opts.getBoolean("filename");
   suspensionsQ = opts.getBoolean("suspensions");
   uncrossQ     = opts.getBoolean("uncross");
   retroQ       = opts.getBoolean("retrospective");
   if (Chaincount < 1) {
      Chaincount = 1;
   }
   
   if (searchQ) {
      // Automatically assume marking of --search is used
      // (may change in the future).
      markQ = 1;
   } 
   if (countQ) {
      searchQ = 1;
      markQ   = 0;
   }


   if (searchQ) {
      SearchString.initializeSearchAndStudy(opts.getString("search"));
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


// md5sum: 9ab7f5f4bcb7446842448bef74c01863 cint.cpp [20130927]
