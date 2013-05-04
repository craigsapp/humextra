//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Mar 19 15:51:09 PDT 2013
// Last Modified: Tue Apr 23 17:32:43 PDT 2013 Added colored notes
// Filename:      ...sig/examples/all/hum2enp.cpp 
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/hum2enp.cpp
// Syntax:        C++; museinfo
//
// Description:   Converts Humdrum files into ENP (Expressive Notation Package)
//                files.
//

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

#include "humdrum.h"
#include "PerlRegularExpression.h"

class Coordinate {
   public:
      int i, j;
};

//////////////////////////////////////////////////////////////////////////

// function declarations:
void  checkOptions             (Options& opts, int argc, char** argv);
void  example                  (void);
void  usage                    (const char* command);
void  convertHumdrumToEnp      (HumdrumFile& infile);
void  getKernTracks            (Array<int>& tracks, HumdrumFile& infile);
void  getPartNames             (HumdrumFile& infile, 
                                Array<Array<char> >& PartNames);
void  pline                    (int level, const char* string);
void  indent                   (int level);
void  printPart                (HumdrumFile& infile, int spine, 
                                Array<int>&  barlines);
void  getBarlines              (Array<int>& barlines, HumdrumFile& infile);
void  printMeasure             (HumdrumFile& infile, int spine, 
                                int voice, Array<int>& barlines, int index);
void  printInitialStaff        (HumdrumFile& infile, int spine);
void  printKeySignature        (HumdrumFile& infile, int spine, int line);
void  printTimeSignature       (HumdrumFile& infile, int spine, int line);
void  extractVoiceItems        (Array<Coordinate>& items, HumdrumFile& infile, 
                                int spine, int voice, int startbar, int endbar);
int   printSubBeatLevel        (HumdrumFile& infile, Array<Coordinate>& items, 
                                Array<int>& notes, int noteindex);
void  printMeasureContent      (HumdrumFile& infile, Array<Coordinate>& items);
void  printMidiNotes           (HumdrumFile& infile, int line, int field);
int   getBeatGroupCount        (HumdrumFile& infile, Array<Coordinate>& items,
                                Array<int>& notes, int noteindex);
RationalNumber getSmallestRhythm(HumdrumFile& infile, Array<Coordinate>& items,
                                Array<int>&  notes, int noteindex, 
                                int groupcount);
void  printChordArticulations  (HumdrumFile& infile, int line, int field);
void  printHeaderComments      (HumdrumFile& infile);
void  printTrailerComments     (HumdrumFile& infile);
void  printDataComments        (HumdrumFile& infile, Array<Coordinate>& items, 
                                int index);
void  printTieDot              (HumdrumFile& infile, int line, int field);
void  checkMarks               (HumdrumFile& infile, Array<char>& marks, 
                                Array<Array<char> >& markcolors);
void  getNoteAttributes        (SSTREAM& attributes, HumdrumFile& infile, 
                                int line, int field, int subfield, 
                                const char* buffer);
void  getNoteExpressions       (SSTREAM& expressions, HumdrumFile& infile, 
                                int line, int field, int subfield, 
                                const char* buffer);

// User interface variables:
Options options;
int    debugQ       = 0;          // used with --debug option
int    originalQ    = 0;          // used with --original option
int    sidecommentQ = 1;          // not hooked up to an option yet
const char*  INDENT = "\t";       // indentation for each level
int    LEVEL        = 0;          // used to indent the score

Array<char> marks;                // used to color notes
Array<Array<char> > markcolors;   // used to color notes
Array<int> markline;              // used to search for circles

// The instance ID works similar to XML::id
int InstanceIdCounter = 0;

//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
   HumdrumFile infile;

   // initial processing of the command-line options
   checkOptions(options, argc, argv);

   if (options.getArgCount() < 1) {
      infile.read(cin);
   } else {
      infile.read(options.getArg(1));
   }

   convertHumdrumToEnp(infile);

   return 0;
}


//////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// convertHumdrumToEnp --
//

void convertHumdrumToEnp(HumdrumFile& infile) {
   infile.analyzeRhythm("4");
   Array<int> kerntracks;
   getKernTracks(kerntracks, infile);
   Array<Array<char> > partnames;
   getPartNames(infile, partnames);
   Array<int> barlines;
   getBarlines(barlines, infile);

   checkMarks(infile, marks, markcolors);

   printHeaderComments(infile);

   LEVEL = 0;
   // Print the outer score parentheses
   pline(LEVEL++, "(:begin :score");   // score-level parenthesis
   int i;
   int partnum = 0;
   for (i=kerntracks.getSize()-1; i>=0; i--) {
      partnum++;
      indent(LEVEL++);
      cout << "(:begin :part" << partnum << endl; // part-level parenthesis
      printInitialStaff(infile, kerntracks[i]);
      printKeySignature(infile, kerntracks[i], 0);
      printTimeSignature(infile, kerntracks[i], 0);
      printPart(infile, kerntracks[i], barlines);
      pline(--LEVEL, ") ; end :part");   // part-level parenthesis
   }
   pline(--LEVEL, ") ; end :score");  // score level parenthesis

   printTrailerComments(infile);
}



//////////////////////////////
//
// printHeaderComments --
//

void printHeaderComments(HumdrumFile& infile) {
   int i;
   for (i=0; i<infile.getNumLines(); i++ ) {
      if (infile[i].isBibliographic()) {
         cout << ";" << infile[i] << endl;
         continue;
      }
      if (infile[i].isGlobalComment()) {
         cout << ";" << infile[i] << endl;
         continue;
      }
      if (infile[i].isInterpretation()) {
         break;
      }
   }
}



//////////////////////////////
//
// printTrailerComments --
//

void printTrailerComments(HumdrumFile& infile) {
   int i;
   int endline = -1;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (infile[i].isInterpretation()) {
         break;
      }
      endline = i;
   }

   for (i=endline; i<infile.getNumLines(); i++ ) {
      if (infile[i].isBibliographic()) {
         cout << ";" << infile[i] << endl;
         continue;
      }
      if (infile[i].isGlobalComment()) {
         cout << ";" << infile[i] << endl;
         continue;
      }
   }
}



//////////////////////////////
//
// printTimeSignature --
//

void printTimeSignature(HumdrumFile& infile, int spine, int line) {
   Coordinate timesig;
   timesig.i = -1;
   timesig.j = -1;
   int track;
   PerlRegularExpression pre;
   int i, j;

   // first search backwards for time signature
   for (i=line; i>0; i--) {
      if (infile[i].isData()) {
         // quit loop when a data line is found.
         break;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         if (track != spine) {
            continue;
         };
         if (!infile[i].isInterpretation()) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*M\\d.*/\\d")) {
            timesig.i = i;
            timesig.j = j;
            break;
         } 
      }
   }

   // now search forwards for time signature
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         // quit loop when a data line is found.
         break;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         if (track != spine) {
            continue;
         };
         if (!infile[i].isInterpretation()) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*M\\d.*/\\d")) {
            timesig.i = i;
            timesig.j = j;
            break;
         } 
      }
   }

   if (timesig.i < 0) {
      // nothing to do
      return;
   }

   // for the moment, do not allow % within time signature
   pre.search(infile[timesig.i][timesig.j], "^\\*M(\\d+)/(\\d+)");

   indent(LEVEL);
   cout << ":time-signature (" 
        << pre.getSubmatch(1) 
        << " "
        << pre.getSubmatch(2);
   if (line == 0) {
      // printing initial time signature
      // check to see if the first measure is a pickup beat
      RationalNumber pickupdur;
      pickupdur = infile.getPickupDurationR();
      if (pickupdur > 0) {
         cout << " :kind :pickup";
      }
   }
   cout << ")" << endl;
}



//////////////////////////////
//
// printKeySignature --  Print a key signature for the given
//      voice.  Search anywhere within enclosing data lines for
//      the *k[] (key signature) and *C: (key) markers.
//

void printKeySignature(HumdrumFile& infile, int spine, int line) {
   Coordinate keysig;
   Coordinate key;
   keysig.i = -1;
   keysig.j = -1;
   key.i = -1;
   key.j = -1;
   int i, j;
   int track;
   PerlRegularExpression pre;

   // first search backwards from current location:
   for (i=line; i>0; i--) {
      if (infile[i].isData()) {
         // quit loop when a data line is found.
         break;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         if (track != spine) {
            continue;
         };
         if (!infile[i].isInterpretation()) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*k\\[.*\\]")) {
            keysig.i = i;
            keysig.j = j;
         } else if (pre.search(infile[i][j], "^\\*[A-G][n#-]*:", "i")) {
            key.i = i;
            key.j = j;
         }
      }
   }

   // now search forwards for key signature and key.
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         // quit loop when a data line is found.
         break;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         if (track != spine) {
            continue;
         };
         if (!infile[i].isInterpretation()) {
            continue;
         }
         if (pre.search(infile[i][j], "^\\*k\\[.*\\]")) {
            keysig.i = i;
            keysig.j = j;
         } else if (pre.search(infile[i][j], "^\\*[A-G][n#-]*:", "i")) {
            key.i = i;
            key.j = j;
         }
      }
   }

   int mode = 0;  // major by default.
   if (key.i > 0) {
      if (islower(infile[key.i][key.j][1])) {
         mode = 1;
      }
   }

   int fifthskey = 0;
   if (keysig.i > 0) {
      fifthskey = Convert::kernKeyToNumber(infile[keysig.i][keysig.j]);
   }

   indent(LEVEL);  cout << ":key-signature ";
   if (mode == 0) {   // major mode
      switch (fifthskey) {
         case -7: cout << ":c-flat-major"; break;
         case -6: cout << ":g-flat-major"; break;
         case -5: cout << ":d-flat-major"; break;
         case -4: cout << ":a-flat-major"; break;
         case -3: cout << ":e-flat-major"; break;
         case -2: cout << ":b-flat-major"; break;
         case -1: cout << ":f-major"; break;
         case  0: cout << ":c-major"; break;
         case +1: cout << ":g-major"; break;
         case +2: cout << ":d-major"; break;
         case +3: cout << ":a-major"; break;
         case +4: cout << ":e-major"; break;
         case +5: cout << ":b-major"; break;
         case +6: cout << ":f-sharp-major"; break;
         case +7: cout << ":c-sharp-major"; break;
      }
   } else if (mode == 1) {  // minor modes
      switch (fifthskey) {
         case -7: cout << ":a-flat-minor"; break;
         case -6: cout << ":e-flat-minor"; break;
         case -5: cout << ":b-flat-minor"; break;
         case -4: cout << ":f-minor"; break;
         case -3: cout << ":c-minor"; break;
         case -2: cout << ":g-minor"; break;
         case -1: cout << ":d-minor"; break;
         case  0: cout << ":a-minor"; break;
         case +1: cout << ":e-minor"; break;
         case +2: cout << ":b-minor"; break;
         case +3: cout << ":f-sharp-minor"; break;
         case +4: cout << ":c-sharp-minor"; break;
         case +5: cout << ":g-sharp-minor"; break;
         case +6: cout << ":d-sharp-minor"; break;
         case +7: cout << ":a-sharp-minor"; break;
      }
   }
   cout << endl;
}



//////////////////////////////
//
// printInitialStaff -- print the starting staff for a part (if any).
//

void printInitialStaff(HumdrumFile& infile, int spine) {
   int i, j;
   int track;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      for (j=0; j<infile[i].getFieldCount(); j++ ) {
         track = infile[i].getPrimaryTrack(j);
         if (track != spine) {
            continue;
         }
         if (strncmp(infile[i][j], "*clef", 5) != 0) {
            // only checking the first subspine for clef information:
            break;
         }
         if (strcmp(infile[i][j], "*clefG2") == 0) {
            pline(LEVEL, ":staff :treble-staff");
         } else if (strcmp(infile[i][j], "*clefF4") == 0) {
            pline(LEVEL, ":staff :bass-staff");
         } else if (strcmp(infile[i][j], "*clefGv2") == 0) {
            pline(LEVEL, ":staff :tenor-staff");  // vocal-tenor clef?
         } else if (strcmp(infile[i][j], "*clefC3") == 0) {
            pline(LEVEL, ":staff :alto-staff");
         } else if (strcmp(infile[i][j], "*clefX") == 0) {
            pline(LEVEL, ":staff :percussion-staff");
         }
         // only checking the first subspine for clef information:
         break; 
      }
   }
}



//////////////////////////////
//
// printPart -- print a particular part.  Just the first one for now.
//

void printPart(HumdrumFile& infile, int spine, Array<int>& barlines) {
   int i;
   int voice = 0;
   indent(LEVEL++);
   cout << "(:begin :voice" << voice+1 << endl;
   for (i=0; i<barlines.getSize()-1; i++) {
      printMeasure(infile, spine, voice, barlines, i);
   }
   pline(--LEVEL, ") ; end :voice");
}



//////////////////////////////
//
// printMeasure --
//

void printMeasure(HumdrumFile& infile, int spine, int voice, 
      Array<int>& barlines, int index) {
   if (voice != 0) {
      // multiple voices are not allowed at the moment.
      exit(1);
   }
   int startbar = barlines[index];
   int endbar = barlines[index+1];
   int i;
   
   Array<Coordinate> items;
   extractVoiceItems(items, infile, spine, voice, startbar, endbar);

   indent(LEVEL++); 
   int barnum = -1;
   cout << "(";
   if (sscanf(infile[items[0].i][items[0].j], "=%d", &barnum)) {
      cout << ":begin :measure" << barnum;
      if (sidecommentQ && infile[items[0].i].isMeasure()) {
         cout << "\t; " << infile[items[0].i][0];
      }
   }
   cout << endl;

   for (i=0; i<items.getSize(); i++) {
      if ((i == items.getSize()-1) && infile[items[i].i].isMeasure()) {
         break;
      }
      indent(LEVEL); cout << "; " << infile[items[i].i][items[i].j] << endl;
   }
   printMeasureContent(infile, items);

   pline(--LEVEL, ")");
}



//////////////////////////////
//
// printMeasureContent --
//

void printMeasureContent(HumdrumFile& infile, Array<Coordinate>& items) {
   int i;
   Array<int> notes;
   notes.setSize(items.getSize());
   notes.setSize(0);
   for (i=0; i<items.getSize(); i++) {
      if (!infile[items[i].i].isData()) {
         continue;
      }
      if (strcmp(infile[items[i].i][items[i].j], ".") != 0) {
         notes.append(i);
      }      
   }

   RationalNumber start, dur;
   int ii, jj;
   for (i=0; i<notes.getSize(); i++) {
      printDataComments(infile, items, notes[i]);
      ii = items[notes[i]].i;
      jj = items[notes[i]].j;
      start = infile[ii].getBeatR();
      dur   = Convert::kernToDurationR(infile[ii][jj]);
      if (!start.isInteger()) {
         cout << "RHYTHM ERROR at token (" << ii << "," << jj << "): " 
              << infile[ii][jj] << endl;
         exit(1);
      }
      if (dur.isInteger()) {
         // simple case where the note is an integer number of beats.
         indent(LEVEL);
         if (strchr(infile[ii][jj], 'r') != NULL) {
            // this rest has no attributes so not adding an extra paren set
            // otherwise it would be "((-".
            cout << "(" << dur << " (-" << 1;
            printTieDot(infile, ii, jj);
            cout << "))";
            if (sidecommentQ) {
               cout << "\t; " << infile[ii][jj];
            }
            cout << endl;
         } else {
            cout << "(" << dur << " ((" << 1;
            printTieDot(infile, ii, jj);
            cout << " :notes (";
            printMidiNotes(infile, ii, jj);
            cout << ")";  // end of notes list
            printChordArticulations(infile, ii, jj);
            cout << "))"; // end of beat list
            cout << ")";  // end of beat group
            if (sidecommentQ) {
               cout << "\t; " << infile[ii][jj];
            }
            cout << endl;
         }
      } else {
         i = printSubBeatLevel(infile, items, notes, i);
      }
   }
}



//////////////////////////////
//
// printTieDot -- print a dot after a rhythm to indicate that it is
//      tied to a previous note.  Do not put a marker on the first note
//      in a tied group.  Ties are created by converting a rhythmic
//      value into a floating-point number by adding ".0" to the end
//      of the rhythm.  Note that adding "." by itself does not work.
//

void printTieDot(HumdrumFile& infile, int line, int field) {
   int tieQ = 0;
   if (strchr(infile[line][field], ']') != NULL) {
      tieQ = 1;
   } else if (strchr(infile[line][field], '_') != NULL) {
      tieQ = 1;
   }

   if (tieQ) {
      cout << ".0";
   }
}



/////////////////////////////
//
// printChordArticulations --
//

void  printChordArticulations(HumdrumFile& infile, int line, int field) {
   int fermataQ = 0;

   if (strchr(infile[line][field], ';') != NULL) {
      fermataQ = 1;
   }

   int expressionQ = 0;
   expressionQ |= fermataQ;

   int counter = 0;
   if (expressionQ) {
      cout << " :expressions (";
      if (fermataQ) {
         if (counter++ != 0) { cout << " "; }
         cout << ":fermata";
      }
      cout << ")";
   }
}



//////////////////////////////
//
// printDataComments --  Print any comments before the current
//      line of data for the voice within the part's voice.  Stop
//      if a previous note/rest is found.
//

void printDataComments(HumdrumFile& infile, Array<Coordinate>& items, 
      int index) {

   int start = -1;
   int ii;
   int i;
   for (i=index-1; i>=0; i--) {
      ii = items[i].i;
      if (infile[ii].isComment()) {
         start = i;
         continue;
      }
      break;
   }

   if (start < 0) {
      // nothing to do
      return;
   }

   for (i=start; i<items.getSize(); i++) {
      ii = items[i].i;
      if (infile[ii].isComment()) {
         indent(LEVEL);
         cout << ";" << infile[ii] << endl;
         continue;
      }
      break;
   }
}



//////////////////////////////
//
// printMidiNotes --
//

void printMidiNotes(HumdrumFile& infile, int line, int field) {
   int tokencount = infile[line].getTokenCount(field);
   int k;
   int base12;
   char buffer[1024] = {0};
   for (k=0; k<tokencount; k++) {
      infile[line].getToken(buffer, field, k);
      base12 = Convert::kernToMidiNoteNumber(buffer);
      if (k > 0) {
         cout << " ";
      }
      SSTREAM slots;
      getNoteAttributes(slots, infile, line, field, k, buffer);
      slots << ends;
      if (strlen(slots.CSTRING) > 0) {
         cout << "(";
         cout << base12;
         cout << slots.CSTRING;
         cout << ")";
      } else {
         cout << base12;
      }
   }
}


//////////////////////////////
//
// getNoteAttributes -- returns a list of attributes for a note (if any)
//

void getNoteAttributes(SSTREAM& attributes, HumdrumFile& infile, int line, 
      int field, int subfield, const char* buffer) {

   // if the note is supposed to be shows as a flatted note, then
   // add an attribute which says to display it as a flat (otherwise
   // ENP will always show accidentals as sharped notes).
   if (strchr(buffer, '-') != NULL) {
      // indicate the the MIDI pitch should be displayed as a diatonic flat
      attributes << " :enharmonic :flat";
   }

   // check for colored notes based on !!!RDF: entries in the file.
   int i;
   for (i=0; i<marks.getSize(); i++) {
      if (marks[i] == '\0') {
         // ignore any null-character
         continue;
      }
      if (strchr(buffer, marks[i]) != NULL) {
         attributes << " :color :" << markcolors[i];
      }
   }

   SSTREAM expressions;
   getNoteExpressions(expressions, infile, line, field, subfield, buffer);
   
   expressions << ends;
   if (strlen(expressions.CSTRING) > 0) {
      attributes << " :expressions (";
      attributes << expressions.CSTRING;      
      attributes << ")";
   }

}



//////////////////////////////
//
// getNoteExpressions --
//

void getNoteExpressions(SSTREAM& expressions, HumdrumFile& infile, int line, 
      int field, int subfield, const char* buffer) {

   PerlRegularExpression pre;
   
   int i;
   for (i=0; i<marks.getSize(); i++) {
      if (strchr(buffer, marks[i]) != NULL) {
         if (pre.search(infile[markline[i]][0], "circle")) {
            expressions << "(:score-expression/" << InstanceIdCounter++;
            expressions << " :kind :circled" << ")";
         }
      }
   }

}



//////////////////////////////
//
// printSubBeatLevel -- Print a list of notes which occur within a beat
//   (or more than one beat, possibly).  Noteindex is the index into 
//   the notes array for the first note of the group to process.  Keep
//   including notes until a beat boundary has been reached.  Return the index
//   of the next note to process after the sub-beat grouping (or the size
//   of notes if there is no more notes to process).
//

int printSubBeatLevel(HumdrumFile& infile, Array<Coordinate>& items, 
      Array<int>& notes, int noteindex) {

   // groupcount is the number of notes in an integer number
   // of beats within the measure.
   int groupcount = getBeatGroupCount(infile, items, notes, noteindex);
   RationalNumber groupduration;
   RationalNumber startpos;
   RationalNumber endpos;
   RationalNumber enddur;
   int ii, jj;
   ii = items[notes[noteindex]].i;
   jj = items[notes[noteindex]].j;
   startpos = infile[ii].getAbsBeatR();
   ii = items[notes[noteindex+groupcount-1]].i;
   jj = items[notes[noteindex+groupcount-1]].j;
   endpos   = infile[ii].getAbsBeatR();
   enddur   = Convert::kernToDurationR(infile[ii][jj]);
   groupduration = (endpos - startpos) + enddur;
   if (!groupduration.isInteger()) {
      cout << "Funny error: group duration is not an integer" << endl;
      exit(1);
   }

   RationalNumber minrhy;
   minrhy.setValue(1,2);
   // minrhy is smallest duration in terms of quarter notes.
   minrhy = getSmallestRhythm(infile, items, notes, noteindex, groupcount);
   indent(LEVEL++); cout << "(" << groupduration << " ("<< endl;
   RationalNumber quarternote(1,4);

   // print group notes
   int i;
   RationalNumber notediv;
   for (i=noteindex; i<noteindex+groupcount; i++) {
      ii = items[notes[i]].i;
      jj = items[notes[i]].j;

      indent(LEVEL);
      notediv = Convert::kernToDurationR(infile[ii][jj]) / minrhy;
      // cout << "(" << notediv.getNumerator();
      cout << "(" << notediv;
      cout << " :notes (";
      printMidiNotes(infile, ii, jj);
      cout << ")"; // end of MIDI pitch list
      cout << ")"; // end of note
      if (sidecommentQ) {
         cout << "\t; " << infile[ii][jj];
      }
      cout << endl;
   }
  
   indent(--LEVEL); cout << "))" << endl;  // end of beat group list

   return noteindex + groupcount - 1;
}



//////////////////////////////
//
// getSmallestRhythm --
//

RationalNumber getSmallestRhythm(HumdrumFile& infile, Array<Coordinate>& items,
     Array<int>&  notes, int noteindex, int groupcount) {
   int i;
   RationalNumber minrhy;
   RationalNumber testrhy;
   minrhy.setValue(1,1);
   int ii, jj;
   for (i=noteindex; i<noteindex+groupcount; i++) { 
      ii = items[notes[i]].i;
      jj = items[notes[i]].j;
      testrhy = Convert::kernToDurationR(infile[ii][jj]);
      if (testrhy.getNumerator() == 0) {
         cout << "ERROR: grace notes are not yet handled by program" << endl;
         exit(1);
      }
      if (minrhy > testrhy) {
         minrhy = testrhy;
      }
   }
   return minrhy;
}



//////////////////////////////
//
// getBeatGroupCount -- return the number of notes in an integer number
// of beats within the measure.
//

int getBeatGroupCount(HumdrumFile& infile, Array<Coordinate>& items, 
      Array<int>& notes, int noteindex) {
   int output = 0;
   int i;
   RationalNumber dursum;
   dursum.setValue(0,1);
   int ii, jj;
   for (i=noteindex; i<notes.getSize(); i++) {
      ii = items[notes[i]].i;
      jj = items[notes[i]].j;
      dursum += Convert::kernToDurationR(infile[ii][jj]);
      output++;
      if (dursum.isInteger()) {
         return output;
      }
   }
   cout << "ERROR: measure does not sum to an integer amount of beats." << endl;
   cout << "Instead the group duration is: " << dursum << endl;
   exit(1);
   return -1;
}



//////////////////////////////
//
// extractVoiceItems -- get a list of non-measure, non-null tokens for
//    part/voice within the given line range.
//

void extractVoiceItems(Array<Coordinate>& items, HumdrumFile& infile, 
      int spine, int voice, int startbar, int endbar) {

   items.setSize(endbar-startbar+1);
   items.setSize(0);
   int i, j;
   int track;
   int voicenum;
   Coordinate loc;
   for (i=startbar; i<=endbar; i++ ) {
      voicenum = 0;
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         if (spine != track) {
            continue;
         }
         if (voicenum++ != voice) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {  // Null token
            continue;
         }
         if (strcmp(infile[i][j], "*") == 0) {  // Null interpretation
            continue;
         }
         if (strcmp(infile[i][j], "!") == 0) {  // Empty local comment
            continue;
         }
         // found something, so store its location
         loc.i = i;
         loc.j = j;
         items.append(loc);
      }
   }
}



//////////////////////////////
//
// getBarlines -- returns the line numbers in the score where there
//    are barlines.  The barlines cannot be intermingled with other
//    data types.
//

void getBarlines(Array<int>& barlines, HumdrumFile& infile) {
   int i;
   int zero = 0;
   int foundstartdata = 0;
   int founddata = 0;
   barlines.setSize(infile.getNumLines());
   barlines.setSize(0);
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isMeasure()) {
         if ((barlines.getSize() == 0) && foundstartdata) {
            // pickup measure, so include start of file.
            barlines.append(zero);
         }
         barlines.append(i);
         founddata = 0;
      } else if (infile[i].isData()) {
         foundstartdata = 1;
         founddata = 1;
      }
   }
   if (founddata) {
      // data after last barline, so include last line of file
      int lastline = infile.getNumLines() - 1;
      barlines.append(lastline);
   }
}



//////////////////////////////
//
// pline -- Print a line of data.
//

void pline(int level, const char* string) {
   indent(level);
   cout << string;
   cout << endl;
} 



//////////////////////////////
//
// indent -- indent the line the specified level
//

void indent(int level) {
   for (int i=0; i<level; i++) {
      cout << INDENT;
   }
} 



//////////////////////////////
//
// getPartNames --
//

void getPartNames(HumdrumFile& infile, Array<Array<char> >& PartNames) {
   int i, j;
   PartNames.setSize(infile.getMaxTracks()+1);  //  0 = unused
   for (i=0; i<PartNames.getSize(); i++) {
      PartNames[i].setSize(1);
      PartNames[i][0]= '\0';
   }

   int abbreviationQ = 0;
   Array<int> ignore;
   ignore.setSize(infile.getMaxTracks()+1);
   ignore.setAll(0);

   PerlRegularExpression pre;
   int track;
   for (i=0; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         // stop looking when the first data line is found
         break;
      }
      if (!infile[i].isInterpretation()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (strcmp(infile[i][j], "*^") == 0) {
            // don't search for names after spine splits (there might
            // be two names, and one of them will be ignored).
            ignore[infile[i].getPrimaryTrack(j)] = 1;
         }
         if (ignore[infile[i].getPrimaryTrack(j)]) {
            continue;
         }

         if (!abbreviationQ) {
            if (pre.search(infile[i][j], "^\\*I\"\\s*(.*)\\s*$", "")) {
               track = infile[i].getPrimaryTrack(j);
               PartNames[track].setSize(strlen(pre.getSubmatch(1))+1);
               strcpy(PartNames[track].getBase(), pre.getSubmatch());
            }
         } else {
            if (pre.search(infile[i][j], "^\\*I\'\\s*(.*)\\s*$", "")) {
               track = infile[i].getPrimaryTrack(j);
               PartNames[track].setSize(strlen(pre.getSubmatch(1))+1);
               strcpy(PartNames[track].getBase(), pre.getSubmatch());
            }
         }

      }
   }
   
   // if no part name, set to "part name" (for debugging purposes):
   //for (i=1; i<=infile.getMaxTracks(); i++) {
   //   if (strcmp(PartNames[i].getBase(), "") == 0) {
   //      PartNames[i].setSize(strlen("part name")+1);
   //      strcpy(PartNames[i].getBase(), "part name");
   //   }
   // }

}



//////////////////////////////
//
// getKernTracks --  Return a list of the **kern primary tracks found
//     in the Humdrum data.  Currently all tracks are independent parts.
//     No grand staff parts are considered if the staves are separated 
//     into two separate spines.
//
//

void getKernTracks(Array<int>& tracks, HumdrumFile& infile) {
   tracks.setSize(infile.getMaxTracks());
   tracks.setSize(0);
   int i;
   for (i=1; i<=infile.getMaxTracks(); i++) {
      if (strcmp(infile.getTrackExInterp(i), "**kern") == 0) {
         tracks.append(i);
      }
   }

   if (debugQ) {
      cout << "\t**kern tracks:\n";
      for (i=0; i<tracks.getSize(); i++) {
         cout << "\t" << tracks[i] << endl;
      }
   }
}



//////////////////////////////
//
// checkMarks -- Check for notes which are marked with a particular
//       color.
//

void checkMarks(HumdrumFile& infile, Array<char>& marks, 
      Array<Array<char> >& markcolors) {
   int markQ = 1;
   if (!markQ) {
      marks.setSize(0);
      markline.setSize(0);
      markcolors.setSize(0);
      return;
   }

   marks.setSize(0);
   markline.setSize(0);
   markcolors.setSize(0);
   int i;
   char target;
   PerlRegularExpression pre;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (pre.search(infile[i][0], 
            "!!!RDF\\*\\*kern\\s*:\\s*([^=])\\s*=\\s*match", "i")) {
         target = pre.getSubmatch(1)[0];
         marks.append(target);
         markline.append(i);
         markcolors.setSize(markcolors.getSize()+1);
         markcolors.last() = "red";
      } else if (pre.search(infile[i][0], 
            "!!!RDF\\*\\*kern\\s*:\\s*([^=])\\s*=\\s*mark", "i")) {
         target = pre.getSubmatch(1)[0];
         marks.append(target);
         markline.append(i);
         markcolors.setSize(markcolors.getSize()+1);
         markcolors.last() = "red";
      }
   }

   if (debugQ) {
      for (i=0; i<marks.getSize(); i++) {
         cout << "MARK " << marks[i] << "\t" << markcolors[i] << endl;
      }
   }
}



//////////////////////////////
//
// checkOptions -- 
//

void checkOptions(Options& opts, int argc, char** argv) {
   opts.define("debug=b",     "Debugging flag");
   opts.define("author=b",    "Program author");
   opts.define("version=b",   "Program version");
   opts.define("example=b",   "Program examples");
   opts.define("h|help=b",    "Short description");
   opts.process(argc, argv);

   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, March 2013" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 19 March 2013" << endl;
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

   debugQ        =  opts.getBoolean("debug");
}



//////////////////////////////
//
// example -- example function calls to the program.
//

void example(void) {


}



//////////////////////////////
//
// usage -- command-line usage description and brief summary
//

void usage(const char* command) {

}


// md5sum: 22e60a81dd37bed86f7aa9e880f99dfe hum2enp.cpp [20130504]
