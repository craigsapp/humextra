//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Aug 30 10:51:26 PDT 2011
// Last Modified: Fri Sep  2 18:25:34 PDT 2011
// Last Modified: Tue Sep 13 13:33:52 PDT 2011 Added -k option
// Last Modified: Thu Sep 15 01:36:49 PDT 2011 Added -D option
// Last Modified: Thu Oct 20 22:23:27 PDT 2011 Fixed init bug
// Last Modified: Sun Oct 20 17:41:10 PDT 2013 Fixed tie problem
// Last Modified: Tue Nov 12 14:37:11 PST 2013 Added column for measure duration
// Last Modified: Sat Mar 12 20:41:25 PST 2016 Switched to STL
// Filename:      ...sig/examples/all/notearray.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/notearray.cpp
// Syntax:        C++; museinfo
// 
// Description:   Generate a two-dimensional numeric array containing
//                notes in the score in base-40, base-12, or base-7 
//                representation.  Each data line represents a sonority
//                with attacked notes in that sonority being positive
//                numbers, and sustained notes from previous sonorities
//                being negative.
//

#include "humdrum.h"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;
   
#define STYLE_BASE40 40
#define STYLE_BASE12 12
#define STYLE_BASE7   7

#define TYPE_MIN         999
#define TYPE_NOTES	1000
#define TYPE_KERN       1000
#define TYPE_LINE	(2000-1)  /* +1 will be added later to make 2000 */
#define TYPE_MEASURE	3000
#define TYPE_BARDUR	3100
#define TYPE_BEATDUR	3200
#define TYPE_BEAT 	4000
#define TYPE_ABSOLUTE	5000
#define TYPE_LINEDUR    6000
#define TYPE_ATTACK	7100
#define TYPE_LAST    	7200
#define TYPE_NEXT	7300

// function declarations
void getNoteArray           (vector<vector<int> >& notes, vector<int>& measpos, 
                             vector<int>& linenum, HumdrumFile& infile, 
                             int base, int flags);
void printNoteArray         (vector<vector<int> >& notes, vector<int>& measpos, 
                             vector<int>& linenum, HumdrumFile& infile,
                             vector<double>& bardur, vector<double>& beatdur);
void printComments          (HumdrumFile& infile, int startline, int stopline, 
                             int style);
void printExclusiveInterpretations(int basecount);
void printLine              (vector<vector<int> >& notes, 
                             vector<vector<int> >& attacks, 
                             vector<vector<int> >& lasts, 
                             vector<vector<int> >& nexts, vector<int>& measpos, 
                             vector<int>& linenum, vector<double>& bardur, 
                             vector<double>& beatdur, HumdrumFile& infile, 
                             int index, int style);
void usage                  (const char* command);
void example                (void);
void checkOptions           (Options& opts, int argc, char* argv[]);
void getNoteAttackIndexes   (vector<vector<int> >& attacks, 
                             vector<vector<int> >& notes, int offst);
void getLastAttackIndexes   (vector<vector<int> >& lasts, 
                             vector<vector<int> >& notes, int offset);
void getNextAttackIndexes   (vector<vector<int> >& lasts, 
                             vector<vector<int> >& notes, int offset);
int  noteStartMarker        (vector<vector<int> >& notes, int line, int column);
int  noteEndMarker          (vector<vector<int> >& notes, int line, int column);
int  noteContinueMarker     (vector<vector<int> >& notes, int line, int column);
int  singleNote             (vector<vector<int> >& notes, int line, int column);
void getMeasureDurations    (vector<double>& bardur, HumdrumFile& infile);
void getBeatDurations       (vector<double>& beatdur, HumdrumFile& infile);


// global variables
Options   options;             // database for command-line arguments
int       humdrumQ  = 0;       // used with -H option
int       base7Q    = 0;       // used with -d option
int       base12Q   = 0;       // used with -m option
int       base40Q   = 1;       // default output type
int       base      = STYLE_BASE40;
int       measureQ  = 1;       // used with -M option
int       beatQ     = 1;       // used with -B option
int       commentQ  = 1;       // used with -C option
int       rationalQ = 0;       // used with -r option
int       fractionQ = 0;       // used with -f option
int       absoluteQ = 0;       // used with -a option
int       linedurQ  = 0;       // used with -D option
int       measuredurQ = 1;     // used with --no-measure-duration
int       beatdurQ  = 1;       // used with --no-beat-duration
int       doubleQ   = 0;       // used with --double option
int       lineQ     = 0;       // used with -l option
int       mathQ     = 0;       // used with --mathematica option
int       susxQ     = 1;       // used with -S option
int       bibQ      = 0;       // used with -b option
int       infoQ     = 1;       // used with -I option
int       octadj    = 0;       // used with -o option
int       endQ      = 0;       // used with -e option
int       typeQ     = 0;       // used with -c option
int       oneQ      = 0;       // used with -1 option
int       sepQ      = 0;       // used with --sep option
int       Offset    = 0;       // used with -1/--offset option
int       OffsetSum = 0;       // used for multiple input files
int       attackQ   = 0;       // used with --attack option
int       nextQ     = 0;       // used with --last option
int       lastQ     = 0;       // used with --next option
int       indexQ    = 0;       // used with -i option
int       saQ       = 0;       // used with --sa option
int       quoteQ    = 0;       // used with --quote option
int       Count     = 0;       // count of how many input files
int       Current   = 0;       // used with --math option
int       moQ       = 0;       // used with --mo option
int       Measure   = 0;       // additive value for measure number
int       Mincrement= 0;       // used to increment between pieces/movements
int       kernQ     = 0;       // used with -k option
int       kerntieQ  = 1;       // used with --no-tie option
int       doubletieQ= 0;       // used with -T option
int       zeroQ     = 1;       // used with -Z option
RationalNumber Absoffset;      // used with --sa option

const char* commentStart = "%";
const char* commentStop  = "";
const char* mathvar  = "data"; // used with --mathematica option
const char* beatbase = "";     // used with -t option


///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

   vector<vector<int> > notelist;
   vector<double>      bardur;
   vector<double>      beatdur;
   vector<int>         measpos;
   vector<int>         linenum;
 
   HumdrumFile infile;

   // process the command-line options
   checkOptions(options, argc, argv);

   // figure out the number of input files to process
   int numinputs = options.getArgCount();
   Count = numinputs;

   Absoffset = 0;

   for (int i=0; i<numinputs || i==0; i++) {
      Current = i;
      if (moQ) {
         Measure = (i+1) * Mincrement;
      }
      infile.clear();

      // if no command-line arguments read data file from standard input
      if (numinputs < 1) {
         infile.read(cin);
      } else {
         infile.read(options.getArg(i+1));
      }
      // analyze the input file according to command-line options
      infile.analyzeRhythm(beatbase);
      getMeasureDurations(bardur, infile);
      getBeatDurations(beatdur, infile);

      getNoteArray(notelist, measpos, linenum, infile, base, doubleQ);
      printNoteArray(notelist, measpos, linenum, infile, bardur, beatdur);
      OffsetSum += notelist.size();
    
      if (!saQ) {
         Absoffset += infile.getTotalDurationR();
      }

      if (sepQ && (Count > 1) && (Current < Count - 1)) {
         // add a separate between input file analyses:
         if (mathQ) {
            cout << "(* ********** *)\n";
         } else if (humdrumQ) {
            cout << "!!!!!!!!!!\n";         
         } else {
            cout << "%%%%%%%%%%\n";
         }
      }
   }
   
   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// getMeasureDurations -- Calculate the duration of a measure for each
//      line in the music.
//

void getMeasureDurations(vector<double>& bardur, HumdrumFile& infile) {
   int i;
   double value = infile.getPickupDuration();
   bardur.resize(infile.getNumLines());
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isBarline()) {
         value = infile[i].getBeat();
      } 
      bardur[i] = value;      
   }
}



//////////////////////////////
//
// getBeatDurations -- Extract the duration of a beat for each
//      line in the music.
//

void getBeatDurations(vector<double>& bardur, HumdrumFile& infile) {
   int i;
   double value = 1.0; // quarter note
   bardur.resize(infile.getNumLines());
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isInterpretation()) {
         if (pre.search(infile[i][0], "^\\*M(\\d+)/(\\d+)%(\\d+)")) {
            
            value = 4.0 / atoi(pre.getSubmatch(2)) * atoi(pre.getSubmatch(3));
         } else if (pre.search(infile[i][0], "^\\*M(\\d+)/(\\d+)")) {
            value = 4.0 / atoi(pre.getSubmatch(2));
         }
      } 
      bardur[i] = value;      
   }
}



//////////////////////////////
//
// getNoteArray -- 
//    style:
//       STYLE_BASE40 40: print as base-40 pitches
//       STYLE_BASE12 12: print as base-12 pitches
//       STYLE_BASE7   7: print as base-7  pitches
//
//    flags:
//    	0: all off:
//    	1: turn on double-barline rest markers.
//

void getNoteArray(vector<vector<int> >& notes, vector<int>& measpos, 
      vector<int>& linenum, HumdrumFile& infile, int style, int flags) {

   notes.reserve(infile.getNumLines());
   notes.resize(0);

   measpos.reserve(infile.getNumLines());
   measpos.resize(0);

   linenum.reserve(infile.getNumLines());
   linenum.resize(0);

   int measnum = 0;
   int rest    = 0;
   int i, j, ii, jj;
   int negQ    = 0;

   // prestorage for note lists so that --no-sustain option can be applied
   vector<int> templist;
   templist.reserve(1000);

   vector<int> templistI;
   templistI.reserve(1000);

   int value;
   int firstQ = 1;
   if (typeQ) {
      // store even if lineQ is zero!
      value = TYPE_LINE;
      linenum.push_back(value);
   }
   if (typeQ) {
      value = TYPE_MEASURE;
      measpos.push_back(value);
   }

   // increment measure number in case of pickups
   measnum = Measure;
   for (i=0; i<infile.getNumLines(); i++) {

      if (infile[i].isMeasure()) {
         if (doubleQ && (templist.size() == 0)) {
            if (strstr(infile[i][0], "||") != NULL) {
               // insert a rest sonority to break music
               // from previous measure
               notes.resize(notes.size() + 1);
               notes.back().reserve(infile[i].getFieldCount());
               notes.back().resize(0);
               for (j=0; j<infile[i].getFieldCount(); j++) {
                  if (!infile[i].isExInterp(j, "**kern")) {
                     continue;
                  }
                  notes.back().push_back(rest);
                  linenum.push_back(i);
               }
               measpos.push_back(measnum);
            }
         }

         // store new measure number (avoiding double code above)
         sscanf(infile[i][0], "=%d", &measnum);
         measnum += Measure;
      }
      if (!infile[i].isData()) {
         continue;
      }

      templist.resize(0);


      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strchr(infile[i].getSpineInfo(j).c_str(), 'b') != NULL) {
            // ignore notes non-primary tracks
            continue;
         }
         int attack = 1;
         if (strcmp(infile[i][j],".")==0) {
            attack = -1;
            ii = infile[i].getDotLine(j);
            jj = infile[i].getDotSpine(j);
         } else {
            ii = i;
            jj = j;
            if (strchr(infile[ii][jj], '_') != NULL) { 
               attack = -1;
            }
            if (strchr(infile[ii][jj], ']') != NULL) { 
               attack = -1;
            }
         }
         int baseval = Convert::kernToBase40(infile[ii][jj]);
         baseval += 40 * octadj;
         if (strchr(infile[ii][jj], 'r') != NULL) {
            baseval = 0;
         } else {
            // now storing base-40, and converting to base-12/base-7 later
            // switch (style) {
            //    case STYLE_BASE12: 
            //       baseval = Convert::base40ToMidiNoteNumber(baseval);
            //       break;
            //    case STYLE_BASE7: 
            //       baseval = Convert::base40ToDiatonic(baseval);
            //       break;
            // }
         }
         if (style == STYLE_BASE7) {
            baseval = Convert::base40ToDiatonic(baseval);
         } else if (style == STYLE_BASE12) {
            baseval = Convert::base40ToMidiNoteNumber(baseval);
         }
         baseval = attack * baseval;
         templist.push_back(baseval);
      }

      negQ = 1;
      if (susxQ) {
         // if all notes are negative, then do not store the line
         for (int m=0; m<(int)templist.size(); m++) {
            if (templist[m] >= 0) {
               negQ = 0;
               break;
            }
         }
         if (negQ) {
            continue;
         }
      }

      if (firstQ && typeQ) {
         firstQ = 0;
         templistI.resize(templist.size());
         int value = TYPE_NOTES;
         switch (style) {
            case STYLE_BASE40: value += 40; break;
            case STYLE_BASE12: value += 12; break;
            case STYLE_BASE7:  value +=  7; break;
         }
         fill(templistI.begin(), templistI.end(), value);
         notes.push_back(templistI);
      }

      notes.push_back(templist);
      measpos.push_back(measnum);
      linenum.push_back(i);
   }

   if (endQ) {
      notes.resize(notes.size()+1);
      notes.back().resize(notes[notes.size()-2].size());
      fill(notes.back().begin(), notes.back().end(), 0);
      // sustains instead of rests (have to copy .last(-1):
      // for (i=0; i<(int)notes.last().size(); i++) {
      //    if (notes.back()[i] > 0) {
      //       notes.back()[i] *= -1;
      //    }
      // }
      measpos.push_back(measpos.back());
      linenum.push_back(linenum.back());
      // store the data termination line as the line number of the sonority
      for (i=infile.getNumLines()-1; i>=0; i--) {
         if (infile[i].isInterpretation()) {
            linenum.back() = i;
            break;
         }
      }
 
      // increment the measure number if the beat of the end
      // is at 1.  This will work unless the last sonority is a 
      // grace note.
      if (infile[linenum.back()].getBeat() == 1.0) {
         measpos.back()++; 
      }
   }

}



//////////////////////////////
//
// printNoteArray --
//

void printNoteArray(vector<vector<int> >& notes, vector<int>& measpos,
      vector<int>& linenum, HumdrumFile& infile, vector<double>& bardur, 
      vector<double>& beatdur) {

   vector<vector<int> > attacks;
   vector<vector<int> > lasts;
   vector<vector<int> > nexts;
   if (attackQ) {
      getNoteAttackIndexes(attacks, notes, Offset + OffsetSum);
   }
   if (lastQ) {
      getLastAttackIndexes(lasts, notes, Offset + OffsetSum);
   }
   if (nextQ) {
      getNextAttackIndexes(nexts, notes, Offset + OffsetSum);
   }

   int i, j;
   for (i=0; i<(int)notes.size(); i++) {
      if (i == 0) { 
         if (commentQ && !typeQ) {
            printComments(infile, 0, linenum[0], humdrumQ);
         } else if (commentQ && typeQ) {
            printComments(infile, 0, linenum[1], humdrumQ);
         }
         if (infoQ) {
            printExclusiveInterpretations(notes[0].size());
         } else if (humdrumQ) {
            // always print exclusive interpretations if Humdrum output
            printExclusiveInterpretations(notes[0].size());
         }
         if (mathQ) {
            if (Count <= 1) {
               cout << mathvar << " = {\n";
            } else if (Current == 0) {
               cout << mathvar << " = {{\n";
            } else {
               cout << "}, {\n";
            }
         }
      } else if (commentQ) {
         printComments(infile, linenum[i-1]+1, linenum[i], humdrumQ);
      }
      if (typeQ && i == 0) {
         printLine(notes, attacks, lasts, nexts, measpos, linenum, 
               bardur, beatdur, infile, i, 1);
      } else {
         printLine(notes, attacks, lasts, nexts, measpos, linenum, 
               bardur, beatdur, infile, i, 0);
      }
   }

   if (humdrumQ) {
      int width = 1;
      if (attackQ) { width++; }
      if (lastQ)   { width++; }
      if (nextQ)   { width++; }
      if (kernQ)   { width++; }
         
      int counter = notes[0].size();
      counter *= width;
      counter += indexQ + lineQ + measureQ + beatQ + absoluteQ + linedurQ;
      // if (attackQ) { counter += notes[0].size(); }
      // if (lastQ) { counter += notes[0].size(); }
      // if (nextQ) { counter += notes[0].size(); }
      for (j=0; j<counter; j++) {
         cout << "*-";
         if (j < counter-1) {
            cout << "\t";
         }
      }
      cout << endl;
   }

   if (mathQ) {
      if (Count <= 1) {
         cout << "};\n";
      } else if (Current <= Count - 2) {
         // cout << "}},\n";
      } else if (Current == Count - 1) {
         cout << "}};\n";
      }
   }

   if (commentQ) {
      printComments(infile, linenum.back(), infile.getNumLines()-1, humdrumQ);
   }
}



//////////////////////////////
//
// getLastAttackIndexes -- return an index of the attack portion
//     of notes (or the first rest in a series of rests) in each voice.
//

void getLastAttackIndexes(vector<vector<int> >& lasts, 
      vector<vector<int> >& notes, int offset) {

   int start = 0;
   lasts.resize(notes.size());
   if (typeQ) {
      fill(lasts[0].begin(), lasts[0].end(), TYPE_LAST);
      start = 1;
   }

   int i, j;

   fill(lasts[start].begin(), lasts[start].end(), -1);

   if ((int)notes.size() == start+1) {
      return;
   }

   vector<int> states;
   states.resize(notes[0].size());
   if (typeQ) {
      fill(states.begin(), states.end(), offset+1);
   } else {
      fill(states.begin(), states.end(), offset);
   }

   for (i=0; i<(int)notes.size(); i++) {
      lasts[i].resize(notes[i].size());
      if (i <= start) {
         fill(lasts[i].begin(), lasts[i].end(), -1);
         if (typeQ && (i==0)) {
            fill(lasts[i].begin(), lasts[i].end(), TYPE_LAST);
         }
         continue;
      }

      for (j=0; j<(int)notes[i].size(); j++) {
         if (notes[i][j] > 0) {
            // a new note attack, store the note attack index in
            // the states array after recording the index of the previous note
            lasts[i][j] = states[j];
            states[j] = i + offset;
         } else if (notes[i][j] < 0) {
           // note is sustaining, so copy the index of the last attack
           // from the previous line
           if (i > start) {
              lasts[i][j] = lasts[i-1][j];
           } else {
              lasts[i][j] = -1;
           }
         } else {
           // rest: if last line was a rest then this is a rest sustain:
           if (i > start) {
              if (notes[i-1][j] == 0) {
                 // rest sustain
                 lasts[i][j] = lasts[i-1][j];
              } else {
                 // rest attack
                 lasts[i][j] = states[j];
                 states[j] = i + offset;
              }  
           } else {
              lasts[i][j] = -1;
           }
         }
      }
   }

}



//////////////////////////////
//
// getNextAttackIndexes -- return an index of the attack portion
//     of notes (or the first rest in a series of rests) in each voice.
//

void getNextAttackIndexes(vector<vector<int> >& nexts, 
      vector<vector<int> >& notes, int offset) {

   int start = 0;
   nexts.resize(notes.size());
   if (typeQ) {
      nexts[0].resize(notes[0].size());
      fill(nexts[0].begin(), nexts[0].end(), TYPE_NEXT);
      start = 1;
   }

   int i, j;

   nexts.back().resize(notes.back().size());
   fill(nexts.back().begin(), nexts.back().end(), -1);

   if ((int)notes.size() == start+1) {
      return;
   }

   for (i=(int)notes.size()-2; i>=start; i--) {
      nexts[i].resize(notes[i].size());
      for (j=0; j<(int)notes[i].size(); j++) {
         if ((notes[i][j] != 0) && (notes[i+1][j] == 0)) {
            // next note is a "rest attack"
            nexts[i][j] = i + 1 + offset;
         } else if (notes[i+1][j] > 0) {
            // a new note attack, store the note attack index in
            // the states array after recording the index of the previous note
            nexts[i][j] = i + 1 + offset;
         } else {
            nexts[i][j] = nexts[i+1][j];
         }

      }
   }

}



//////////////////////////////
//
// getNoteAttackIndexes -- return an index of the attack portion
//     of notes (or the first rest in a series of rests) in each voice.
//

void getNoteAttackIndexes(vector<vector<int> >& attacks, 
      vector<vector<int> >& notes, int offset) {

   attacks.resize(notes.size());
   int i, j;

   attacks[0].resize(notes[0].size());
   fill (attacks[0].begin(), attacks[0].end(), offset);

   for (i=1; i<(int)attacks.size(); i++) {
      attacks[i].resize(notes[i].size());
      for (j=0; j<(int)attacks[i].size(); j++) {
         if (notes[i][j] < 0) {
            // a sustained note, so store index of attack note
            attacks[i][j] = attacks[i-1][j];
         } else if (notes[i][j] > 0) {
            // note being attacked, so store its index
            attacks[i][j] = i + offset;
         } else {
            // a rest: check to see if last position was a rest.
            // if so, then this is a "tied rest"; otherwise it is
            // a "attacked rest".
            if (notes[i-1][j] == 0) {
               attacks[i][j] = attacks[i-1][j];
            } else {
               attacks[i][j] = i + offset;
            }
         }
      }
   }

   if (typeQ) {
      fill(attacks[0].begin(), attacks[0].end(), TYPE_ATTACK);
   }
}



//////////////////////////////
//
// printLine -- print a line of the note array.
//    style == 0: regular line
//    style == 1: index line (don't extract beat or absbeat from infile)
//

void printLine(vector<vector<int> >& notes, vector<vector<int> >& attacks, 
      vector<vector<int> >& lasts, vector<vector<int> >& nexts, 
      vector<int>& measpos, vector<int>& linenum, vector<double>& bardur,
      vector<double>& beatdur, HumdrumFile& infile, 
      int index, int style) {

   int& i = index;
   int j;

   if (mathQ) {
      cout << "{";
   }

   if (indexQ) { 
      cout << i + Offset + OffsetSum;
   }

   if (lineQ) { 
      if (indexQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      cout << linenum[i]+1;
   }

   if (measureQ) { 
      if (indexQ || lineQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      cout << measpos[i];
   }

   if (measuredurQ) {
      if (indexQ || lineQ || measureQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      if ((i == 0) && (linenum[i] == TYPE_LINE)) {
         cout << TYPE_BARDUR;
      } else {
         cout << bardur[linenum[i]];
      }
   }

   if (beatdurQ) {
      if (indexQ || lineQ || measureQ || measuredurQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      if ((i == 0) && (linenum[i] == TYPE_LINE)) {
         cout << TYPE_BEATDUR;
      } else {
         cout << beatdur[linenum[i]];
      }
   }

   if (beatQ) {
      if (indexQ || lineQ || measuredurQ || measureQ || beatdurQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      if (style == 1) {
         cout << TYPE_BEAT;
      } else if (rationalQ) {
         if (fractionQ) {
            cout << infile[linenum[i]].getBeatR() - 1;
         } else {
            // switched to 0-offset metrical position
            RationalNumber value = infile[linenum[i]].getBeatR();
            value -= 1;
            value.printTwoPart(cout);
         }
      } else {
         // print beat position as a floating-point number:
         cout << infile[linenum[i]].getBeat() - 1;
      }
   }

   if (absoluteQ) {
      if (indexQ || lineQ || measuredurQ || measureQ || beatdurQ || beatQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      if (style == 1) {
         cout << TYPE_ABSOLUTE;
      } else if (rationalQ) {
         if (fractionQ) {
            cout << (Absoffset + infile[linenum[i]].getAbsBeatR());
         } else {
            RationalNumber sum = Absoffset + infile[linenum[i]].getAbsBeatR();
            sum.printTwoPart(cout);
         }
      } else {
         // print beat position as a floating-point number:
         cout << infile[linenum[i]].getAbsBeat() + Absoffset.getFloat();
      }
 
   }

   if (linedurQ) {
      if (indexQ || lineQ || measuredurQ || measureQ || beatdurQ || 
            beatQ || absoluteQ) {
         if (mathQ) {
            cout << ",";
         }
         cout << "\t";
      }
      if (style == 1) {
         cout << TYPE_LINEDUR;
      } else if (rationalQ) {
         if (fractionQ) {
            cout << (Absoffset + infile[linenum[i]].getDurationR());
         } else {
            RationalNumber num = infile[linenum[i]].getDurationR();
            num.printTwoPart(cout);
         }
      } else {
         // print beat position as a floating-point number:
         cout << infile[linenum[i]].getDuration();
      }
 
   }
   if (indexQ || lineQ || measuredurQ || measureQ || beatdurQ || 
         beatQ || absoluteQ || linedurQ) {
      if (mathQ) {
         cout << ",";
      }
   }


   vector<int> values;
   values.reserve(notes.size() * 10);

   int vv;
   int sign;
   for (j=0; j<(int)notes[i].size(); j++) {
      vv = notes[i][j];
      //if (kernQ && (style == 1) && typeQ) {
      // int ww = TYPE_KERN;
      //   values.push_back(ww);
      //} 
      if (vv < TYPE_NOTES) {
         if (vv < 0) {
            sign = -1;
            vv = -vv;
         } else {
            sign = +1;
         }
         if (vv != 0) { 
            switch (style) {
               case STYLE_BASE12: 
                  vv = Convert::base40ToMidiNoteNumber(vv);
                break;
               case STYLE_BASE7: 
                  vv = Convert::base40ToDiatonic(vv);
                  break;
            }
         }
         vv *= sign;
         values.push_back(vv);
      } else {
         values.push_back(vv);
      }

      if (attackQ) {
         values.push_back(attacks[i][j]);
      }
      if (lastQ) {
         values.push_back(lasts[i][j]);
      }
      if (nextQ) {
         values.push_back(nexts[i][j]);
      }
   }

   int width = 1;  // notes
   if (attackQ) { width++; }
   if (lastQ)   { width++; }
   if (nextQ)   { width++; }

   char buffer[1024] = {0};

   for (j=0; j<(int)values.size(); j++) {
      if (kernQ && ((j % width) == 0)) {
         //if (mathQ) {
         //   cout << ",";
         //}
         cout << "\t";
         if ((style == 1) && (i == 0)) {
            cout << TYPE_KERN;         
         } else {
            if (mathQ || quoteQ) {
               cout << "\"";
            }

            if (kerntieQ && noteStartMarker(notes, i, j / width)) {
               cout << "[";
            } 
            if (doubletieQ && singleNote(notes, i, j / width)) {
               cout << "["; 
            }

            if (values[j] == 0) {
               cout << "r";
            } else {
               cout << Convert::base40ToKern(buffer, abs(values[j]));
            }

            if (kerntieQ && noteContinueMarker(notes, i, j / width)) {
               cout << "_";
            } else if (kerntieQ && noteEndMarker(notes, i, j / width)) {
               cout << "]";
            } 
            if (doubletieQ && singleNote(notes, i, j / width)) {
               cout << "]"; 
            }

            if (mathQ || quoteQ) {
               cout << "\"";
            }
         }
         if (mathQ) {
            cout << ",";
         }
      }
      cout << "\t" << values[j];

      if (j < (int)values.size()-1) {
         if (mathQ) {
            cout << ",";
         }
         //cout << "\t";
      }
   }
   if (mathQ) {
      cout << "}";
      if (i < (int)linenum.size() - 1) {
         cout << ",";
      }
   }
   cout << endl;
}



//////////////////////////////
//
// singleNote -- true if the note in the column/line is uniq
//

int singleNote(vector<vector<int> >& notes, int line, int column) {
   int start = 0;
   if (typeQ) {
      start = 1;
   }
   if (line < start) {
      return 0;
   }

   int pitch = notes[line][column];
   int nextpitch = -1;
   int lastpitch = -1;

   if ((line > start) && ((int)notes.size() > 1+start)) {
      lastpitch = notes[line-1][column];
   }
   if (((int)notes.size() > start+1) && (line < (int)notes.size() - 1)) {
      nextpitch = notes[line+1][column];
   }
 
   if (pitch > TYPE_MIN) {
      return 0;
   }

   if (pitch < 0) {
      // note is a sustain, so not considered
      return 0;
   } else if (pitch == 0) {
      // if the rest is surrounded by non-rests, then mark
      if ((pitch != lastpitch) && (pitch != nextpitch)) {
         return 1;
      } else {
         return 0;
      }
   } else {
      if (pitch != -nextpitch) {
         // next sonority does not contain a sustain of the note
         return 1;
      } else {
         return 0;
      }
   }
}



//////////////////////////////
//
// noteStartMarker --
//

int noteStartMarker(vector<vector<int> >& notes, int line, int column) {
   int start = 0;
   if (typeQ) {
      start = 1;
   }
   if (line < start) {
      return 0;
   }

   int pitch = notes[line][column];
   int nextpitch = -1;
   int lastpitch = -1;

   if ((line > start) && ((int)notes.size() > 1+start)) {
      lastpitch = notes[line-1][column];
   }
   if (((int)notes.size() > start+1) && (line < (int)notes.size() - 1)) {
      nextpitch = notes[line+1][column];
   }

   if (pitch == 0) {
      // if the first rest in a row then true
      if ((lastpitch != 0) && (nextpitch == 0)) {
         // don't include rests which start and stop on the same line
         return 1;
      } else {
         return 0;
      }
   } else if (pitch < 0) {
      return 0;
   } else {
      if ((nextpitch < 0) && (nextpitch != -1)) {
         return 1;
      } else {
         return 0;
      }
   }
}



//////////////////////////////
//
// noteContinueMarker --
//

int noteContinueMarker(vector<vector<int> >& notes, int line, int column) {
   int start = 0;
   if (typeQ) {
      start = 1;
   }
   if (line < start) {
      return 0;
   }

   int pitch = notes[line][column];
   int nextpitch = -1;
   int lastpitch = -1;

   if ((line > start) && ((int)notes.size() > 1+start)) {
      lastpitch = notes[line-1][column];
   }
   if (((int)notes.size() > start+1) && (line < (int)notes.size() - 1)) {
      nextpitch = notes[line+1][column];
   }

   if (pitch == 0) {
      // if not the first or the last zero than a continue
      if ((lastpitch == 0) && (nextpitch == 0)) {
         return 1;
      } else {
         return 0;
      }
   } else if (pitch > 0) {
      return 0;
   } else {
      if (pitch == nextpitch ) {
         return 1;
      } else {
         return 0;
      }
   }
}



//////////////////////////////
//
// noteEndMarker --
//

int noteEndMarker(vector<vector<int> >& notes, int line, int column) {
   int start = 0;
   if (typeQ) {
      start = 1;
   }
   if (line < start) {
      return 0;
   }

   int pitch = notes[line][column];
   int nextpitch = -1;
   // int lastpitch = -1;

   if ((line > start) && ((int)notes.size() > 1+start)) {
      // lastpitch = notes[line-1][column];
   }
   if (((int)notes.size() > start+1) && (line < (int)notes.size() - 1)) {
      nextpitch = notes[line+1][column];
   }

   if (pitch == 0) {
      // if not the first or the last zero than a continue
      if (nextpitch != 0) {
         return 1;
      } else {
         return 0;
      }
   } else if (pitch > 0) {
      return 0;
   } else {
      if (pitch != nextpitch ) {
         return 1;
      } else {
         return 0;
      }
   }
}



//////////////////////////////
//
// printExclusiveInterpretations -- print ** markers at the start of 
//      each column of data.
//      
//      Order of optional prefix columns:
//      lineQ
//      measureQ
//      beatQ
//      absoluteQ
//      linedurQ
//

void printExclusiveInterpretations(int basecount) {

   char basename[1024] = {0};

   const char* prefix = "%%";
   if (humdrumQ || mathQ) {
      prefix = "**";
   } 

   if (kernQ) {
      strcat(basename, "kern");
      strcat(basename, "\t");
   }

   if (base7Q) {
      // strcat(basename, prefix);
      strcat(basename, "b7");
   } else if (base12Q) {
      // strcat(basename, prefix);
      strcat(basename, "b12");
   } else {
      // strcat(basename, prefix);
      strcat(basename, "b40");
   }

   if (attackQ) {
      strcat(basename, "\t");
      strcat(basename, prefix);
      strcat(basename, "attk");
   }
   if (lastQ) {
      strcat(basename, "\t");
      strcat(basename, prefix);
      strcat(basename, "last");
   }
   if (nextQ) {
      strcat(basename, "\t");
      strcat(basename, prefix);
      strcat(basename, "next");
   }

   int startmark = 0;

   if (indexQ) {
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "idx\t"; 
   }
   if (lineQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "line\t"; 
   }
   if (measureQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "bar\t"; 
   }

   if (measuredurQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "mdur\t"; 
   }

   if (beatdurQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "bdur\t"; 
   }

   if (beatQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "beat\t"; 
   }

   if (absoluteQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "abs\t"; 
   }

   if (linedurQ) { 
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << "ldur\t"; 
   }

   int i;
   for (i=0; i<basecount; i++) {
      if ((startmark == 0) && mathQ) {
         cout << "(*";
         startmark++;
      } else {
         cout << prefix;
      }
      cout << basename;
      if (i < basecount - 1) {
         cout << "\t";
      }
   }
   if (mathQ) {
      cout << " *)";
   }
   cout << "\n";

}



//////////////////////////////
//
//  
// printComments -- print any comments
//

void printComments(HumdrumFile& infile, int startline, int stopline, 
      int style) {

   int i;
   for (i=startline; i<=stopline; i++) {
      if (!infile[i].isComment()) {
         continue;
      }
      if (bibQ && infile[i].isGlobalComment()) {
         continue;
      }
      if (infile[i].isLocalComment()) {
         // don't know how to store local comments, ignore for now.
         continue;
      }
      if (style) {
         // print in Humdrum format:
         cout << infile[i] << "\n";
      } else {
         // print in Matlab format:
         cout << commentStart << infile[i] << commentStop << "\n";
      }
   }
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("a|absolute=b",    "print absolute beat numbers");
   opts.define("b|bib|bibliographic|reference=b", "display only bib record");
   opts.define("c|type=b",        "add a numeric index at start of data array");
   opts.define("d|diatonic=b",    "print output in absolute diatonic");
   opts.define("f|fraction=b",    "display rational number as fraction");
   opts.define("e|end|end-rest=b","store ending rest");
   opts.define("i|index=b",       "print a column with the index value");
   opts.define("j|josquin=b",     "default settings for josquin project");
   opts.define("k|kern=b",        "display kern spine");
   opts.define("no-tie|no-ties=b", "don't display kern tie information");
   opts.define("l|line=b",        "print original line of sonority in data");
   opts.define("m|midi=b",        "print output as MIDI key numbers");
   opts.define("o|octave=i:0",    "octave adjustment value");
   opts.define("r|rational=b",    "display metric position as rational number");
   opts.define("t|beat=s:",       "metric base for constant beat analysis");
   opts.define("1|one=b",         "offset index values by one");
   opts.define("M|no-measures=b", "don't print measure number column");
   opts.define("B|no-beats=b",    "don't print beat value column");
   opts.define("C|no-comments=b", "don't print comments in file");
   opts.define("D|linedur=b",     "display duration of line");
   opts.define("A|all=b",         "display all options prefix columns");
   opts.define("H|humdrum=b",     "print output in Humdrum format");
   opts.define("I|no-info=b",     "do not display information header");
   opts.define("S|no-sustain=b",  "suppress sonorities that are only sustains");
   opts.define("T|all-tie|all-ties=b",  "start/stop tie marks on all notes");
   opts.define("N|no-cols=b",     "turn off all information columns");
   opts.define("attack=b",        "display note-attack index values");
   opts.define("double=b",        "add rests at double barlines");
   opts.define("no-measure-duration=b",  "don't display duration of measures");
   opts.define("no-beat-duration=b",  "don't display duration of beats");
   opts.define("last=b",          "display previous note-attack index values");
   opts.define("math|mathematica=s:data", "print output data as Matlab array");
   opts.define("mel|melodic=b",    "display melodic note index columns");
   opts.define("mo|measure-offset=i:0", "bar num increase per file");
   opts.define("next=b",          "display following note-attack index values");
   opts.define("offset=i:0",      "starting index for first row");
   opts.define("sa|separate-absolute=b", "single absolute beat positions");
   opts.define("sep|separator=b",  "print a separator between input analyses");
   opts.define("quote=b",          "print quotes around kern names");
   opts.define("Z|no-zero-beat=b", "start first beat of measure at 1 rather than 0");

   opts.define("debug=b");        // determine bad input line num
   opts.define("author=b");       // author of program
   opts.define("version=b");      // compilation info
   opts.define("example=b");      // example usages
   opts.define("h|help=b");       // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Aug 2011" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 30 August 2011" << endl;
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

   if (opts.getBoolean("josquin")) {
      // fix analysis, until then this is turned off:
      // beatbase  = "1";  // beat is the whole note.
      doubleQ   = 1;
   } else {
      beatbase  = opts.getString("beat").c_str();
      doubleQ   = opts.getBoolean("double");
   }

   humdrumQ = opts.getBoolean("humdrum");
   base7Q   = opts.getBoolean("diatonic");
   base12Q  = opts.getBoolean("midi");
   if (base7Q) {
      base7Q  = 1;
      base12Q = 0;
      base40Q = 0;
   } else if (base12Q) {
      base7Q  = 0;
      base12Q = 1;
      base40Q = 0;
   } else {
      base7Q  = 0;
      base12Q = 0;
      base40Q = 1;
   }

   if (base7Q) {
      base = STYLE_BASE7;
   } else if (base12Q) {
      base = STYLE_BASE12;
   } else {
      base = STYLE_BASE40;
   }

   mathQ     =  opts.getBoolean("mathematica");
   mathvar   =  opts.getString("mathematica").c_str();
   commentStart = "(* ";
   commentStop  = " *)";

   if (!mathQ) {
      commentStart = "% ";
      commentStop  = "";
   }

   lineQ     =  opts.getBoolean("line");
   measureQ  = !opts.getBoolean("no-measures");
   beatQ     = !opts.getBoolean("no-beats");
   absoluteQ =  opts.getBoolean("absolute");
   linedurQ  =  opts.getBoolean("linedur");
   commentQ  = !opts.getBoolean("no-comments");
   rationalQ =  opts.getBoolean("rational");
   fractionQ =  opts.getBoolean("fraction");
   susxQ     =  opts.getBoolean("no-sustain");
   kernQ     =  opts.getBoolean("kern");
   kerntieQ  = !opts.getBoolean("no-tie");
   bibQ      =  opts.getBoolean("bibliographic");
   infoQ     = !opts.getBoolean("no-info");
   endQ      =  opts.getBoolean("end-rest");
   octadj    =  opts.getInteger("octave");
   typeQ     =  opts.getBoolean("type");
   attackQ   =  opts.getBoolean("attack");
   nextQ     =  opts.getBoolean("last");
   measuredurQ = !opts.getBoolean("no-measure-duration");
   beatdurQ    = !opts.getBoolean("no-beat-duration");
   lastQ     =  opts.getBoolean("next");
   indexQ    =  opts.getBoolean("index");
   sepQ      =  opts.getBoolean("sep");
   saQ       =  opts.getBoolean("separate-absolute");
   Mincrement=  opts.getInteger("measure-offset");
   moQ       =  opts.getBoolean("measure-offset");
   Offset    =  opts.getInteger("offset");
   zeroQ     = !opts.getInteger("no-zero-beat");
   quoteQ    =  opts.getBoolean("quote");
   doubletieQ=  opts.getBoolean("all-tie");
   if (doubletieQ) {
      kerntieQ = 1;
   }
   if (Offset < 0) {
      Offset = 0;
   }
   if (opts.getBoolean("mel")) {
      attackQ = 1;
      nextQ   = 1;
      lastQ   = 1;
   }

   if (opts.getBoolean("1")) {
      Offset = 1;
   }

   if (fractionQ) {
      rationalQ = 1;
   }

   if (opts.getBoolean("no-cols")) {
      measureQ = 0;
      beatQ    = 0;
   }

   if (opts.getBoolean("all")) {
      lineQ = measureQ = beatQ = absoluteQ = linedurQ = 1;
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



