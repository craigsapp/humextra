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


#define CLEF_UNKNOWN 0
#define CLEF_TREBLE  1
#define CLEF_BASS    2

class Coordinate {
   public:
      int i, j;
};

//////////////////////////////////////////////////////////////////////////

// function declarations:
void  checkOptions             (Options& opts, int argc, char** argv);
void  example                  (void);
void  usage                    (const char* command);
void  convertHumdrumToEnp      (ostream& out, HumdrumFile& infile);
void  getKernTracks            (Array<int>& tracks, HumdrumFile& infile);
void  getPartNames             (HumdrumFile& infile, 
                                Array<Array<char> >& PartNames);
void  pline                    (ostream& out, int level, const char* string);
void  plineStart               (ostream& out, int level, const char* string);
void  indent                   (ostream& out, int level);
void  printPart                (ostream& out, HumdrumFile& infile, int spine, 
                                int subspine, Array<int>&  barlines, 
                                int keysig, int clef);
void  getBarlines              (Array<int>& barlines, HumdrumFile& infile);
void  printMeasure             (ostream& out, HumdrumFile& infile, int spine, 
                                int voice, Array<int>& barlines, int index,
                                int defaulkeysig, int defaultclef, int& activeclef);
void  printInitialStaff        (ostream& out, HumdrumFile& infile, int spine, 
                                int& defaultClef);
int   printKeySignature        (ostream& out, HumdrumFile& infile, int spine, 
                                int line);
void  printTimeSignature       (ostream& out, HumdrumFile& infile, int spine, 
                                int line);
void  extractVoiceItems        (Array<Coordinate>& items, HumdrumFile& infile, 
                                int spine, int voice, int startbar, int endbar);
int   printSubBeatLevel        (ostream& out, HumdrumFile& infile, 
                                Array<Coordinate>& items, 
                                Array<int>& notes, int noteindex, int keysig,
                                int defaultclef, int currentclef);
void  printMeasureContent      (ostream& out, HumdrumFile& infile, 
                                Array<Coordinate>& items,
                                RationalNumber& starttime,
                                RationalNumber& endtime, int defaultkeysig, 
                                int defaultclef, int& activeclef);
void  printMidiNotes           (ostream& out, HumdrumFile& infile, int line, 
                                int field, int keysig);
int   getBeatGroupCount        (HumdrumFile& infile, Array<Coordinate>& items,
                                Array<int>& notes, int noteindex);
RationalNumber getSmallestRhythm(HumdrumFile& infile, Array<Coordinate>& items,
                                Array<int>&  notes, int noteindex, 
                                int groupcount);
void  printChordArticulations  (ostream& out, HumdrumFile& infile, int line, 
                                int field);
void  printHeaderComments      (ostream& out, HumdrumFile& infile);
void  printTrailerComments     (ostream& out, HumdrumFile& infile);
void  printDataComments        (ostream& out, HumdrumFile& infile, 
                                Array<Coordinate>& items, int index);
void  printTieDot              (ostream& out, HumdrumFile& infile, int line, 
                                int field);
void  checkMarks               (HumdrumFile& infile, Array<char>& marks, 
                                Array<Array<char> >& markcolors);
void  getNoteAttributes        (SSTREAM& attributes, HumdrumFile& infile, 
                                int line, int field, int subfield, 
                                const char* kernnote, int keysig);
void  getNoteExpressions       (SSTREAM& expressions, HumdrumFile& infile, 
                                int line, int field, int subfield, 
                                const char* kernnote);
void  getSubspines             (Array<int>& subtracks, HumdrumFile& infile, 
                                Array<int>& kerntracks);
void  printChord              (ostream& out, HumdrumFile& infile, int line, 
                               int field, RationalNumber& dur, int keysig,
                               int defaultclef, int currentclef);
void  printRest               (ostream& out, HumdrumFile& infile, int line, 
                               int field, RationalNumber& dur);
void  printStem               (ostream& out, HumdrumFile& infile, int line, 
                               int field);
void  getEnharmonic           (ostream& out, const char* note, int keysig);
ostream& printClefAttribute   (ostream& out, int activeclef);
ostream& printScoreInformation(ostream& out, HumdrumFile& infile);

// User interface variables:
Options options;
int    debugQ       = 0;          // used with --debug option
int    originalQ    = 0;          // used with --original option
int    labelQ       = 0;          // used with -L option
int    commentQ     = 0;          // used with -C option
int    humdrumQ     = 0;          // not hooked up to an option yet
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

   convertHumdrumToEnp(cout, infile);

   return 0;
}


//////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// getSubspineCount -- Return a count of each track's maximum sub-spine
//     count.
//

void getSubspines(Array<int>& subtracks, HumdrumFile& infile, 
      Array<int>& kerntracks) {
   int i;
   int j;

   int track;
   int maxtracks = infile.getMaxTracks();
   Array<int> maxvals(maxtracks+1);
   maxvals.setAll(0);

   Array<int> linevals(maxtracks+1);
   

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].hasSpines()) {
         continue;
      }
      linevals.setAll(0);
      for (j=0; j<infile[i].getFieldCount(); j++) {
         track = infile[i].getPrimaryTrack(j);
         linevals[track]++;
      }
      for (j=0; j<linevals.getSize(); j++) {
         if (linevals[j] > maxvals[j]) {
            maxvals[j] = linevals[j];
         }
      }
   }
   
   subtracks.setSize(kerntracks.getSize());
   subtracks.setAll(0);
   for (i=0; i<kerntracks.getSize(); i++) {
      subtracks[i] = maxvals[kerntracks[i]];
   }
}



//////////////////////////////
//
// convertHumdrumToEnp --
//

void convertHumdrumToEnp(ostream& out, HumdrumFile& infile) {
   infile.analyzeRhythm("4");
   Array<int> kerntracks;
   getKernTracks(kerntracks, infile);

   Array<int> subtracks;
   getSubspines(subtracks, infile, kerntracks);

   Array<Array<char> > partnames;
   getPartNames(infile, partnames);
   Array<int> barlines;
   getBarlines(barlines, infile);

   checkMarks(infile, marks, markcolors);

   if (commentQ) {
      printHeaderComments(out, infile);
   }

   LEVEL = 0;

   // Print the outer score parentheses
   plineStart(out, LEVEL++, "(");
   if (labelQ) {
      out << ":begin :score";
   }
   out << endl;
   printScoreInformation(out, infile);

   int i, j;
   int partnum = 0;
   for (i=kerntracks.getSize()-1; i>=0; i--) {
      partnum++;
      indent(out, LEVEL++);

      // print part-level parenthesis
      out << "(";
      if (labelQ) {
         out << ":begin :part" << partnum;
      }
      out << endl;

      int initialclef = CLEF_UNKNOWN;
      printInitialStaff(out, infile, kerntracks[i], initialclef);
      int initialkeysig = printKeySignature(out, infile, kerntracks[i], 0);
      printTimeSignature(out, infile, kerntracks[i], 0);
      printPart(out, infile, kerntracks[i], 0, barlines, initialkeysig, initialclef);
      // print voices/layers after the first one:
      for (j=1; j<subtracks[i]; j++) {
         printPart(out, infile, kerntracks[i], j, barlines, initialkeysig, initialclef);
      }

      plineStart(out, --LEVEL, ")"); // part-level parenthesis
      if (labelQ) {
         out << " ; end :part" << partnum;
      }
      out << endl;
   }
   plineStart(out, --LEVEL, ")");
   if (labelQ) {
      out << " ; end :score";  // score level parenthesis
   }
   out << endl;

   if (commentQ) {
      printTrailerComments(out, infile);
   }
}



//////////////////////////////
//
// printScoreInformation --
//
// :catalog-info (:TITLE		""
//                :COMPOSER		""
//                :SUBTITLE		""
//                :TEMPO-HEADING	""
//                :MOVEMENT-INFO	""
//                :INSTRUMENT		""
//  )
//

ostream& printScoreInformation(ostream& out, HumdrumFile& infile) {
   int otl = -1;
   int opr = -1;
   int com = -1;
   int omd = -1;
   int yec = -1;
   int dataQ = 0;
   int i;
   char key[1024] = {0};

   PerlRegularExpression pre;

   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      infile[i].getBibKey(key);
      if ((otl < 0) && pre.search("^OTL", key)) {
         otl = i; dataQ = 1; continue;
      }
      if ((opr < 0) && pre.search("^OPR", key)) {
         opr = i; dataQ = 1; continue;
      }
      if ((omd < 0) && pre.search("^OMD", key)) {
         omd = i; dataQ = 1; continue;
      }
      if ((com < 0) && pre.search("^COM", key)) {
         com = i; dataQ = 1; continue;
      }
      if ((yec < 0) && pre.search("^YEC", key)) {
         yec = i; dataQ = 1; continue;
      }
   }
 

   if (!dataQ) {
      return out;
   }
   
   indent(out, LEVEL++);
   out << ":catalog-info (" << endl;

   Array<char> value1;
   Array<char> value2;
  
   if (opr >= 0) {
      if (otl >= 0) {
         infile[opr].getBibValue(value1);
         infile[otl].getBibValue(value2);
         pre.sar(value1, "\"", "\'", "g");
         pre.sar(value2, "\"", "\'", "g");

         indent(out, LEVEL);
         out << "(:title \"";
         out << value1;
         out << "\")" << endl;

         indent(out, LEVEL);
         out << "(:subtitle \"";
         out << value2;
         out << "\")" << endl;
      } else {
         infile[opr].getBibValue(value1);
         pre.sar(value1, "\"", "\'", "g");

         indent(out, LEVEL);
         out << "(:title \"";
         out << value1;
         out << "\")" << endl;
      }
   } else if (otl >= 0) {
         infile[otl].getBibValue(value1);
         pre.sar(value1, "\"", "\'", "g");

         indent(out, LEVEL);
         out << "(:title \"";
         out << value1;
         out << "\")" << endl;
   }

   if (com >= 0) {
         infile[com].getBibValue(value1);
         pre.sar(value1, "\"", "\'", "g");

         indent(out, LEVEL);
         out << "(:composer \"";
         out << value1;
         out << "\")" << endl;
   }

   if (omd >= 0) {
         infile[omd].getBibValue(value1);
         pre.sar(value1, "\"", "\'", "g");

         indent(out, LEVEL);
         out << "(:tempo-heading \"";
         out << value1;
         out << "\")" << endl;
   }

   if (yec >= 0) {
         infile[yec].getBibValue(value1);
         pre.sar(value1, "\"", "\'", "g");

         indent(out, LEVEL);
         out << "(:copyright \"";
         out << value1;
         out << "\")" << endl;
   }



   indent(out, --LEVEL);
   out << ")" << endl;

   return out;
}



//////////////////////////////
//
// printHeaderComments --
//

void printHeaderComments(ostream& out, HumdrumFile& infile) {
   int i;
   for (i=0; i<infile.getNumLines(); i++ ) {
      if (infile[i].isBibliographic()) {
         out << ";" << infile[i] << endl;
         continue;
      }
      if (infile[i].isGlobalComment()) {
         out << ";" << infile[i] << endl;
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

void printTrailerComments(ostream& out, HumdrumFile& infile) {
   int i;
   int endline = -1;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (infile[i].isInterpretation()) {
         break;
      }
      endline = i;
   }
   if (endline < 0) {
      return;
   }

   for (i=endline; i<infile.getNumLines(); i++ ) {
      if (infile[i].isBibliographic()) {
         out << ";" << infile[i] << endl;
         continue;
      }
      if (infile[i].isGlobalComment()) {
         out << ";" << infile[i] << endl;
         continue;
      }
   }
}



//////////////////////////////
//
// printTimeSignature --
//

void printTimeSignature(ostream& out, HumdrumFile& infile, int spine, 
      int line) {
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
   pre.search(infile[timesig.i][timesig.j], "^\\*M(\\d+)\\/(\\d+)");

   int topnum = atoi(pre.getSubmatch(1));
   int botnum = atoi(pre.getSubmatch(2));

   indent(out, LEVEL);
   out << ":time-signature (" 
        << topnum
        << " "
        << botnum;
   if (line == 0) {
      // printing initial time signature
      // check to see if the first measure is a pickup beat
      RationalNumber pickupdur;
      pickupdur = infile.getPickupDurationR();
      if (pickupdur > 0) {
         out << " :kind :pickup";
      }
   }
   out << ")" << endl;
}



//////////////////////////////
//
// printKeySignature --  Print a key signature for the given
//      voice.  Search anywhere within enclosing data lines for
//      the *k[] (key signature) and *C: (key) markers.
//
//      Returns the key signatures in terms of sharp/flat count.
//

int printKeySignature(ostream& out, HumdrumFile& infile, int spine, int line) {
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
      if (std::islower(infile[key.i][key.j][1])) {
         mode = 1;
      }
   }

   int fifthskey = 0;
   if (keysig.i > 0) {
      fifthskey = Convert::kernKeyToNumber(infile[keysig.i][keysig.j]);
   }

   indent(out, LEVEL);  out << ":key-signature ";
   if (mode == 0) {   // major mode
      switch (fifthskey) {
         case -7: out << ":c-flat-major"; break;
         case -6: out << ":g-flat-major"; break;
         case -5: out << ":d-flat-major"; break;
         case -4: out << ":a-flat-major"; break;
         case -3: out << ":e-flat-major"; break;
         case -2: out << ":b-flat-major"; break;
         case -1: out << ":f-major"; break;
         case  0: out << ":c-major"; break;
         case +1: out << ":g-major"; break;
         case +2: out << ":d-major"; break;
         case +3: out << ":a-major"; break;
         case +4: out << ":e-major"; break;
         case +5: out << ":b-major"; break;
         case +6: out << ":f-sharp-major"; break;
         case +7: out << ":c-sharp-major"; break;
      }
   } else if (mode == 1) {  // minor modes
      switch (fifthskey) {
         case -7: out << ":a-flat-minor"; break;
         case -6: out << ":e-flat-minor"; break;
         case -5: out << ":b-flat-minor"; break;
         case -4: out << ":f-minor"; break;
         case -3: out << ":c-minor"; break;
         case -2: out << ":g-minor"; break;
         case -1: out << ":d-minor"; break;
         case  0: out << ":a-minor"; break;
         case +1: out << ":e-minor"; break;
         case +2: out << ":b-minor"; break;
         case +3: out << ":f-sharp-minor"; break;
         case +4: out << ":c-sharp-minor"; break;
         case +5: out << ":g-sharp-minor"; break;
         case +6: out << ":d-sharp-minor"; break;
         case +7: out << ":a-sharp-minor"; break;
      }
   }
   out << endl;

   return fifthskey;
}



//////////////////////////////
//
// printInitialStaff -- print the starting staff for a part (if any).
//

void printInitialStaff(ostream& out, HumdrumFile& infile, int spine, int& defaultClef) {
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
            pline(out, LEVEL, ":staff :treble-staff");
            defaultClef = CLEF_TREBLE;
         } else if (strcmp(infile[i][j], "*clefF4") == 0) {
            pline(out, LEVEL, ":staff :bass-staff");
            defaultClef = CLEF_BASS;
         } else if (strcmp(infile[i][j], "*clefGv2") == 0) {
            pline(out, LEVEL, ":staff :tenor-staff");  // vocal-tenor clef?
         } else if (strcmp(infile[i][j], "*clefC3") == 0) {
            pline(out, LEVEL, ":staff :alto-staff");
         } else if (strcmp(infile[i][j], "*clefX") == 0) {
            pline(out, LEVEL, ":staff :percussion-staff");
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

void printPart(ostream& out, HumdrumFile& infile, int spine, 
      int subspine, Array<int>& barlines, int defaultkeysig, int defaultclef) {
   int i;
   int voice = subspine;
   plineStart(out, LEVEL++, "(");
   if (labelQ) {
      out << ":begin :voice" << voice+1;
   }
   out << endl;
   int activeclef = defaultclef;
   for (i=0; i<barlines.getSize()-1; i++) {
      printMeasure(out, infile, spine, voice, barlines, i, defaultkeysig, defaultclef, activeclef);
   }
   plineStart(out, --LEVEL, ")");
   if (labelQ) {
      out << " ; end :voice" << voice+1;
   }
   out << endl;
}



//////////////////////////////
//
// printMeasure --
//

void printMeasure(ostream& out, HumdrumFile& infile, int spine, int voice, 
      Array<int>& barlines, int index, int defaultkeysig, int defaultclef, int& activeclef) {

   int startbar = barlines[index];
   int endbar = barlines[index+1];
   int i;
   
   Array<Coordinate> items;
   extractVoiceItems(items, infile, spine, voice, startbar, endbar);

   indent(out, LEVEL++); 
   out << "(";
   if (labelQ) {
      int barnum = -1;
      if (items.getSize() == 0) {
         if (sscanf(infile[startbar][0], "=%d", &barnum)) {
            out << ":begin :measure" << barnum;
            if (humdrumQ && infile[items[0].i].isMeasure()) {
               out << "\t; " << infile[items[0].i][0];
            }
         }
      } else {
         if (sscanf(infile[items[0].i][items[0].j], "=%d", &barnum)) {
            out << ":begin :measure" << barnum;
            if (humdrumQ && infile[items[0].i].isMeasure()) {
               out << "\t; " << infile[items[0].i][0];
            }
         }
      }
   }
   out << endl;

   if (humdrumQ) {
      for (i=0; i<items.getSize(); i++) {
         if ((i == items.getSize()-1) && infile[items[i].i].isMeasure()) {
            break;
         }
         indent(out, LEVEL); 
         out << "; " << infile[items[i].i][items[i].j] << endl;
      }
   }

   RationalNumber starttime = infile[startbar].getAbsBeatR();
   RationalNumber endtime   = infile[endbar].getAbsBeatR();
   printMeasureContent(out, infile, items, starttime, endtime, defaultkeysig, defaultclef, activeclef);

   pline(out, --LEVEL, ")");
}



//////////////////////////////
//
// printMeasureContent --  Adds invisible rests if there is a gap
//    in the voice data.
//

void printMeasureContent(ostream& out, HumdrumFile& infile, 
      Array<Coordinate>& items, RationalNumber& starttime,
      RationalNumber& endtime, int keysig, int defaultclef, int& activeclef) {

   RationalNumber mdur;
   mdur = endtime - starttime;
   if (items.getSize() == 0) {
      // print invisible full-measure rest
      indent(out, LEVEL);
      out << "(" << mdur << " ((-1 :visible-p nil)))" << endl;
      return;
   }

   int i;
   Array<int> notes;

   notes.setSize(items.getSize());
   notes.setSize(0);

   Array<int> clefs;
   clefs.setSize(items.getSize());
   clefs.setSize(0);

   for (i=0; i<items.getSize(); i++) {
      if (infile[items[i].i].isInterpretation()) {
         const char* token = infile[items[i].i][items[i].j];
         if      (strcmp(token, "*clefG2") == 0) { activeclef = CLEF_TREBLE; }
         else if (strcmp(token, "*clefF4") == 0) { activeclef = CLEF_BASS;   }
      }
      if (!infile[items[i].i].isData()) {
         continue;
      }
      if (strcmp(infile[items[i].i][items[i].j], ".") != 0) {
         notes.append(i);
         clefs.append(activeclef);
      }      
   }

   RationalNumber start, dur;
   int ii, jj;
   for (i=0; i<notes.getSize(); i++) {
      printDataComments(out, infile, items, notes[i]);
      ii = items[notes[i]].i;
      jj = items[notes[i]].j;
      start = infile[ii].getBeatR();
      dur   = Convert::kernToDurationR(infile[ii][jj]);
      if (!start.isInteger()) {
         out << "RHYTHM ERROR at token (" << ii << "," << jj << "): " 
              << infile[ii][jj] << endl;
         exit(1);
      }
      if (dur.isInteger()) {
         printChord(out, infile, ii, jj, dur, keysig, defaultclef, clefs[i]);
      } else {
         i = printSubBeatLevel(out, infile, items, notes, i, keysig, defaultclef, clefs[i]);
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

int printSubBeatLevel(ostream& out, HumdrumFile& infile, 
      Array<Coordinate>& items, Array<int>& notes, int noteindex,
      int keysig, int defaultclef, int currentclef) {

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
      cerr << "Funny error: group duration is not an integer" << endl;
      exit(1);
   }

   RationalNumber minrhy;
   minrhy.setValue(1,2);
   // minrhy is smallest duration in terms of quarter notes.
   minrhy = getSmallestRhythm(infile, items, notes, noteindex, groupcount);
   indent(out, LEVEL++); out << "(" << groupduration << " ("<< endl;
   RationalNumber quarternote(1,4);

   // print group notes
   int i;
   RationalNumber notediv;
   for (i=noteindex; i<noteindex+groupcount; i++) {
      ii = items[notes[i]].i;
      jj = items[notes[i]].j;

      notediv = Convert::kernToDurationR(infile[ii][jj]) / minrhy;
      printChord(out, infile, ii, jj, notediv, keysig, defaultclef, currentclef);

/*
      if (strchr(infile[ii][jj], 'r') != NULL) {
         // out << "(" << notediv;
         // out << "(";   // add this paren if rest has attributes
         out << "-1"; // rest
         // out << ")";   // add this paren if rest has attributes
         // out << ")";
      } else {
         out << "(" << notediv;
         out << " :notes (";
         printMidiNotes(out, infile, ii, jj);
         out << ")"; // end of MIDI pitch list
         out << ")"; // end of note
      }
*/


   }
  
   indent(out, --LEVEL); out << "))" << endl;  // end of beat group list

   return noteindex + groupcount - 1;
}



///////////////////////////////
//
// printChord -- A chord, single note, or rest.
//

void printChord(ostream& out, HumdrumFile& infile, int line, int field, 
      RationalNumber& dur, int keysig, int defaultclef, int currentclef) {
   int& ii = line;
   int& jj = field;

   int tdur = dur.getNumerator();
   if (tdur == 0) {
      // grace notes are stored with duration of 1 (and :class :grace-beat)
      tdur = 1;
   }

   // simple case where the note is an integer number of beats.
   indent(out, LEVEL);

   if (strchr(infile[ii][jj], 'r') != NULL) {
      printRest(out, infile, line, field, dur);
   } else {
      out << "(" << tdur << " ((" << 1;

      printTieDot(out, infile, ii, jj);
      out << " :notes (";
      printMidiNotes(out, infile, ii, jj, keysig);
      out << ")";  // end of notes list
      
      printChordArticulations(out, infile, ii, jj);

      printStem(out, infile, ii, jj);
 
      if (defaultclef != currentclef) {
         printClefAttribute(out, currentclef);
      }

      out << ")"; // end of chord parentheses 
      out << ")"; // end of beat list 
      if (dur == 0) {
         out << " :class :grace-beat";
      }
      out << ")";  // end of beat group
   }

   if (humdrumQ) {
      out << "\t; " << infile[ii][jj];
   }
   out << endl;
}



//////////////////////////////
//
// printClefAttribute --
//

ostream& printClefAttribute(ostream& out, int activeclef) {
   switch(activeclef) {
      case CLEF_TREBLE: out << " :clef :treble-clef"; break;
      case CLEF_BASS:   out << " :clef :bass-clef";   break;
   }
      
   return out;
}



//////////////////////////////
//
// printStem -- / -> :stem-direction :up  & \ -> :stem-direction :down
// Can't have a chord with stems in both directions (will print with stem up).
//

void printStem(ostream& out, HumdrumFile& infile, int line, int field) {
   if (strchr(infile[line][field], '/') != NULL) {
      out << " :stem-direction :up";
   } else if (strchr(infile[line][field], '\\') != NULL) {
      out << " :stem-direction :down";
   }
}



//////////////////////////////
//
// printRest --
//

void printRest(ostream& out, HumdrumFile& infile, int line, int field, 
      RationalNumber& dur) {
   int& ii = line;
   int& jj = field;

   int tdur = dur.getNumerator();
   if (tdur == 0) {
      // grace notes are stored with duration of 1 (and :class :grace-beat)
      tdur = 1;
   }

   // this rest has no attributes so not adding an extra paren set
   // otherwise it would be "((-".
   out << "(" << tdur << " (-" << 1;
   printTieDot(out, infile, ii, jj);
   out << ")"; // paren for inner units
   if (dur == 0) {
      out << " :class :grace-beat";
   }
   out << ")"; // paren for outer unit
}



//////////////////////////////
//
// printTieDot -- print a dot after a rhythm to indicate that it is
//      tied to a previous note.  Do not put a marker on the first note
//      in a tied group.  Ties are created by converting a rhythmic
//      value into a floating-point number by adding ".0" to the end
//      of the rhythm.  Note that adding "." by itself does not work.
//

void printTieDot(ostream& out, HumdrumFile& infile, int line, int field) {
   int tieQ = 0;
   if (strchr(infile[line][field], ']') != NULL) {
      tieQ = 1;
   } else if (strchr(infile[line][field], '_') != NULL) {
      tieQ = 1;
   }

   if (tieQ) {
      out << ".0";
   }
}



/////////////////////////////
//
// printChordArticulations --
//

void  printChordArticulations(ostream& out, HumdrumFile& infile, int line, 
      int field) {
   int fermataQ   = 0;
   int staccatoQ  = 0;
   int accentQ    = 0;   // visually: >   in Humdrum: '
   int marcatoQ   = 0;   // visually: ^   in Humdrum: ^

   if (strchr(infile[line][field], ';') != NULL) {
      fermataQ = 1;
   }
   if (strchr(infile[line][field], '\'') != NULL) {
      staccatoQ = 1;
   }
   if (strchr(infile[line][field], '^') != NULL) {
      accentQ = 1;
   }
   if (strchr(infile[line][field], '`') != NULL) {
      marcatoQ = 1;
   }

   int expressionQ = 0;
   expressionQ |= fermataQ;
   expressionQ |= staccatoQ;
   expressionQ |= marcatoQ;
   expressionQ |= accentQ;

   int counter = 0;
   if (expressionQ) {
      out << " :expressions (";
      if (fermataQ) {
         if (counter++ != 0) { out << " "; }
         out << ":fermata";
      }

      if (staccatoQ && accentQ) {
         if (counter++ != 0) { out << " "; }
         out << ":accent+staccato";
      }

      if (staccatoQ && marcatoQ) {
         if (counter++ != 0) { out << " "; }
         out << ":marcato+staccato";
      }

      if (staccatoQ && !accentQ && !marcatoQ) {
         if (counter++ != 0) { out << " "; }
         out << ":staccato";
      }
   
      if (!staccatoQ && accentQ) {
         if (counter++ != 0) { out << " "; }
         out << ":accent";
      }

      if (!marcatoQ && accentQ) {
         if (counter++ != 0) { out << " "; }
         out << ":marcato";
      }

      out << ")";
   }
}



//////////////////////////////
//
// printDataComments --  Print any comments before the current
//      line of data for the voice within the part's voice.  Stop
//      if a previous note/rest is found.
//

void printDataComments(ostream& out, HumdrumFile& infile, 
      Array<Coordinate>& items, int index) {

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
         indent(out, LEVEL);
         out << ";" << infile[ii] << endl;
         continue;
      }
      break;
   }
}



//////////////////////////////
//
// printMidiNotes --
//

void printMidiNotes(ostream& out, HumdrumFile& infile, int line, int field,
         int keysig) {
   int tokencount = infile[line].getTokenCount(field);
   int k;
   int base12;
   char buffer[1024] = {0};
   for (k=0; k<tokencount; k++) {
      infile[line].getToken(buffer, field, k);
      base12 = Convert::kernToMidiNoteNumber(buffer);
      if (k > 0) {
         out << " ";
      }
      SSTREAM slots;
      getNoteAttributes(slots, infile, line, field, k, buffer, keysig);
      slots << ends;
      if (strlen(slots.CSTRING) > 0) {
         out << "(";
         out << base12;
         out << slots.CSTRING;
         out << ")";
      } else {
         out << base12;
      }
   }
}


//////////////////////////////
//
// getNoteAttributes -- returns a list of attributes for a note (if any)
//

void getNoteAttributes(SSTREAM& attributes, HumdrumFile& infile, int line, 
      int field, int subfield, const char* kernnote, int keysig) {

   // if the note is supposed to be shows as a flatted note, then
   // add an attribute which says to display it as a flat (otherwise
   // ENP will always show accidentals as sharped notes).

/* 
 * :enharmonic :flat will be in context of the key so 61 may be default
 * sharp if in A major, but 
   if (strchr(buffer, '-') != NULL) {
      // indicate the the MIDI pitch should be displayed as a diatonic flat
      attributes << " :enharmonic :flat";
   }
*/

/* Temporarily get rid of enharmonics:
   SSTREAM enharmonic;
   getEnharmonic(enharmonic, kernnote, keysig);
   enharmonic << ends;
   if (strlen(enharmonic.CSTRING) > 0) {
      attributes << " :enharmonic " << enharmonic.CSTRING;
   }
*/

   // check for cautionary accidentals.  These are marked with "X" immediately after the 
   // accidental.
   if ((strstr(kernnote, "nX") != NULL)
       || (strstr(kernnote, "#X") != NULL)
       || (strstr(kernnote, "-X") != NULL)) {
      attributes << " :draw-alteration-p :force";
   }
   
   // check for colored notes based on !!!RDF: entries in the file.
   int i;
   for (i=0; i<marks.getSize(); i++) {
      if (marks[i] == '\0') {
         // ignore any null-character
         continue;
      }
      if (strchr(kernnote, marks[i]) != NULL) {
         attributes << " :color :" << markcolors[i];
      }
   }

   SSTREAM expressions;
   getNoteExpressions(expressions, infile, line, field, subfield, kernnote);
   
   expressions << ends;
   if (strlen(expressions.CSTRING) > 0) {
      attributes << " :expressions (";
      attributes << expressions.CSTRING;      
      attributes << ")";
   }

}



//////////////////////////////
//
// getEnharmonic --
//

void getEnharmonic(ostream& out, const char* note, int keysig) {
   Array<int> diatonic(7);
   diatonic.setAll(0);
   if (keysig > 0) {
      switch (keysig) {
         case 7: diatonic[6] = 1;  // B
         case 6: diatonic[2] = 1;  // E
         case 5: diatonic[5] = 1;  // A
         case 4: diatonic[1] = 1;  // D
         case 3: diatonic[4] = 1;  // G
         case 2: diatonic[0] = 1;  // C
         case 1: diatonic[3] = 1;  // F
      }
   } else if (keysig < 0) {
      switch (keysig) {
         case -7: diatonic[3] = -1;  // F
         case -6: diatonic[0] = -1;  // C
         case -5: diatonic[4] = -1;  // G
         case -4: diatonic[1] = -1;  // D
         case -3: diatonic[5] = -1;  // A
         case -2: diatonic[2] = -1;  // E
         case -1: diatonic[6] = -1;  // B
      }
   }

   int notedia = Convert::kernToDiatonicPitch(note) % 7;
   int base40 = Convert::kernToBase40(note);
   int accid = Convert::base40ToAccidental(base40);
   
   //int midi    = Convert::kernToMidiNoteNumber(note);

   int difference = accid - diatonic[notedia];

   if (difference == 0) {
      // MIDI note is in the scale, so no need to alter it.
      return;
   } else if (difference == 1) {
      out << ":sharp"; 
      return;
   } else if (difference == -1) {
      out << ":flat"; 
      return;
   }

   // don't know what to do: some exotic accidental
}



//////////////////////////////
//
// getNoteExpressions --
//

void getNoteExpressions(SSTREAM& expressions, HumdrumFile& infile, int line, 
      int field, int subfield, const char* kernnote) {

   PerlRegularExpression pre;
   
   int i;
   for (i=0; i<marks.getSize(); i++) {
      if (strchr(kernnote, marks[i]) != NULL) {
         if (pre.search(infile[markline[i]][0], "circle")) {
            expressions << "(:score-expression/" << InstanceIdCounter++;
            expressions << " :kind :circled" << ")";
         }
      }
   }

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
         cerr << "ERROR: grace notes are not yet handled by program" << endl;
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
   cerr << "ERROR: measure does not sum to an integer amount of beats." << endl;
   cerr << "Instead the group duration is: " << dursum << endl;
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

void pline(ostream& out, int level, const char* string) {
   indent(out, level);
   out << string;
   out << endl;
} 



//////////////////////////////
//
// plineStart -- Like pline, but does not put a newline at end.
//

void plineStart(ostream& out, int level, const char* string) {
   indent(out, level);
   out << string;
} 



//////////////////////////////
//
// indent -- indent the line the specified level
//

void indent(ostream& out, int level) {
   for (int i=0; i<level; i++) {
      out << INDENT;
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
      cerr << "\t**kern tracks:\n";
      for (i=0; i<tracks.getSize(); i++) {
         cerr << "\t" << tracks[i] << endl;
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
         cerr << "MARK " << marks[i] << "\t" << markcolors[i] << endl;
      }
   }
}



//////////////////////////////
//
// checkOptions -- 
//

void checkOptions(Options& opts, int argc, char** argv) {
   opts.define("H|humdrum=b", "Display original Humdrum data in tandem with output data.");
   opts.define("L|no-labels=b",   "Suppress fuction labels");
   opts.define("C|no-comments=b", "Suppress reference record printing");
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
   humdrumQ      =  opts.getBoolean("humdrum");
   labelQ        = !opts.getBoolean("no-labels");
   commentQ      = !opts.getBoolean("no-comments");
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


// md5sum: 86ad0e9ff0720341c3aaafd6c3d1bc75 hum2enp.cpp [20131108]
