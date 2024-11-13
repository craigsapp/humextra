//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu Dec 15 15:00:17 PST 2011
// Last Modified: Thu Dec 15 15:00:22 PST 2011
// Last Modified: Wed Jun 20 16:22:44 PDT 2012 Added instruments abbreviations
// Last Modified: Tue Aug 28 10:24:57 PDT 2012 Default metric bug fix
// Last Modified: Fri Jul 19 13:39:52 PDT 2013 MenC2 implicit barlines:2,dash
// Filename:      ...sig/examples/all/jrpize.cpp
// Web Address:   http://sig.sapp.org/examples/museinfo/humdrum/jrpize.cpp
// Syntax:        C++; museinfo
//
// Description:   
//
// Things this program does:
// (1) Add "l" markers on terminal and medial longs
// (2) Adds a RWG entry for terminal longs
// (3) Adds % RWG entry if "%" enhanced rhythmic values are used
// (4) default mensuration signs.
// (5) Section: text to !!section: comments and !!!OMD: records
//
// Things to add in the future:
// (a) automatic triplet brackets?
// (b) textual items.
// (c) mensuration sign exceptions.
//

#include "PerlRegularExpression.h"
#include "humdrum.h"

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
void      checkOptions          (Options& opts, int argc, char* argv[]);
void      example               (void);
void      usage                 (const char* command);
int       addTerminalLongs      (HumdrumFile& infile);
int       addTrackLongs         (HumdrumFile& infile, int track);
void      printOutput           (HumdrumFile& infile, int terminalQ);
int       hasEditorial          (HumdrumFile& infile);
int       hasEditorialRDF       (HumdrumFile& infile);
int       hasLongRDF            (HumdrumFile& infile);
void      handleMensuration     (HumdrumFile& infile, int line);
void      printAbbreviation     (const char* fullname);
void      checkAndPrintInstAbbr (HumdrumFile& infile, int line);
void      checkAndPrintVox      (HumdrumFile& infile, int line);
int       hasSectionLabel       (HumdrumFile& infile, int line);
void      printSectionLabel     (HumdrumFile& infile, int line);
void      printDefaultKeyTimeMet(HumdrumFile& infile, int line);
void      processMensuration    (HumdrumFile& infile, int line);
char*     getMensuration        (char* buffer, HumdrumFile& infile, 
                                 int line, int spine);
char*     convertMenIntoMet     (char* buffer, const char* mensur);
void      clearMensurationComment(HumdrumFile& infile, int line);
void      setItem               (Array<Array<char> >& anArray, int i, 
                                 const char* aString);
void      printSectionInfo      (void);
int       wrongSideOfBarline    (HumdrumFile& infile, int line);
int       convertRscaleTextToInterpretation(HumdrumFile& infile, int line);
void      processBarlineComment (HumdrumFile& infile);
void      applyBarlineMarker    (HumdrumFile& infile, int line, int column, 
                                 int barjump, int dashQ);
void      makeBarlineInvisible  (HumdrumFile& infile, int line, int track, 
                                 char addchar, int dashQ);
void      printLabel            (HumdrumFile& infile, int line);
void      printDefaultLabelExpansion(HumdrumFile& infile, int line);
int       hasDataBelow          (HumdrumFile& infile, int line);
void      turnOffAnyActiveRscales(HumdrumFile& infile, int line);
void      mergeAdjacentNotes    (HumdrumFile& infile, int line, int track);
void      fixAccentedText       (HumdrumRecord& aRecord);
void      checkText             (HumdrumRecord& aRecord, int index);
void      checkForPrimaryMensurationNeed(HumdrumFile& infile, int line);
int       cleanDefaultInterpretations(HumdrumFile& infile, int line);
void      absorbMensurations    (HumdrumFile& infile, int line);

// global variables
Options   options;             // database for command-line arguments
int       hasTerminalLong = 0; // true if "l" marker added to data.
Array<Array<char> > Clef;      // used for prevailing clef by voice
Array<Array<char> > Keysig;    // used for prevailing key signature by voice
Array<Array<char> > Timesig;   // used for prevailing time signature by voice
Array<Array<char> > Metsig;    // used for prevailing meter signature by voice
Array<RationalNumber> Rscale;  // used to turn off *rscale:

Array<char > Section;   // used to flip on other side of measure line
char       SectionLabel = 'A'-1;
int        debugQ = 0;

///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
   HumdrumFile infile;
   Section.setSize(1);
   Section[0] = '\0';

   // process the command-line options
   checkOptions(options, argc, argv);

   // figure out the number of input files to process
   int numinputs = options.getArgCount();

   for (int i=0; i<numinputs || i==0; i++) {
      infile.clear();

      // if no command-line arguments read data file from standard input
      if (numinputs < 1) {
         infile.read(cin);
      } else {
         infile.read(options.getArg(i+1));
      }

      hasTerminalLong = addTerminalLongs(infile);

      processBarlineComment(infile);

      printOutput(infile, hasTerminalLong);

   }

   return 0;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// processBarlineComment --
//

void processBarlineComment(HumdrumFile& infile) {
   int i, j;
   PerlRegularExpression pre;
   PerlRegularExpression pre2;
   PerlRegularExpression pre3;
   int number;
   int dashQ = 0;
   int allQ = 0;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isLocalComment()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (pre.search(infile[i][j], "\\ball\\b(:|(&colon;)).*MenC2", "i")) {
            for (int jj=0; jj<infile[i].getFieldCount(); jj++) {
               if (!infile[i].isExInterp(jj, "**kern")) {
                  continue;
               }
               applyBarlineMarker(infile, i, jj, 2, 1);
            }
            break;
         } else if (pre.search(infile[i][j], "\\bMenC2\\b")) {
            applyBarlineMarker(infile, i, j, 2, 1);
            continue;
         }
         if (!pre.search(infile[i][j], 
               "!LO:TX:.*t=[^:]*barline[s]&colon;\\s*(\\d+[^:]*)", "i")) {
            continue;
         }
         if (pre2.search(pre.getSubmatch(1), "/")) {
            cerr << "ERROR: barlines directive has fraction: "
                 << pre.getSubmatch() << " on line " << i+1 << endl;
            exit(1);
         }
         if (pre3.search(infile[i][j], "dash", "i")) {
            dashQ = 1;
         }
         if (pre3.search(infile[i][j], "all(&colon;|:)", "i")) {
// What is this for?
cerr << "GOT HERE XXXXX" << endl;
            allQ = 1;
         }
         number = atoi(pre.getSubmatch());
         if (allQ) {
            for (int x=0; x<infile[i].getFieldCount(); x++) {
               if (infile[i].isExInterp(x,"**kern")) {
                  applyBarlineMarker(infile, i, j, number, dashQ);
               }
            }
         } else {
            applyBarlineMarker(infile, i, j, number, dashQ);
         }
         infile[i].setToken(j, "!");
      }
   }
}



//////////////////////////////
//
// applyBarlineMarker --
//

void applyBarlineMarker(HumdrumFile& infile, int line, int column, 
      int barjump, int dashQ) {
   int i;
   int visibleQ;
   int barcount = 0;
   int track = infile[line].getPrimaryTrack(column);
   PerlRegularExpression pre;
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (!infile[i].isMeasure()) {
         continue;
      }
      // exit loop if end of music or section is found
      if (pre.search(infile[i][0], "=="))     { break; }
      if (pre.search(infile[i][0], "\\|\\|")) { break; }
      if (pre.search(infile[i][0], ":"))      { break; }

      barcount++;
      visibleQ = !(barcount % barjump);
      if (visibleQ) {
         // or maybe make sure that barline is visible...
         continue;
      }
      if (dashQ) {
         makeBarlineInvisible(infile, i, track, '.', dashQ);
      } else {
         makeBarlineInvisible(infile, i, track, '-', dashQ);
      }
   }
}



//////////////////////////////
//
// makeBarlineInvisible --
//

void makeBarlineInvisible(HumdrumFile& infile, int line, int track,
      char addchar, int dashQ) {
   if (!infile[line].isMeasure()) {
      return;
   }
   char charstring[2] = {0};
   charstring[0] = addchar;
   PerlRegularExpression pre;
   char buffer[1024] = {0};
   int j;
   int t;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      t = infile[line].getPrimaryTrack(j);
      if (t != track) {
         continue;
      }
      if (strchr(infile[line][j], addchar) != NULL) {
         // already is invisible
         continue;
      }
      strcpy(buffer, infile[line][j]);
      strcat(buffer, charstring);
      infile[line].setToken(j, buffer);
      if (!dashQ) {
         mergeAdjacentNotes(infile, line, t);
      }
   }
}


//////////////////////////////
//
// mergeAdjacentNotes --
//

void mergeAdjacentNotes(HumdrumFile& infile, int line, int track) {
   int lasti = 0;
   int lastj = 0;
   int i, j;
   int nexti = 0;
   int nextj = 0;
   int t;

   for (i=line+1; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         t = infile[i].getPrimaryTrack(j);
         if (t != track) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         }
         nexti = i;
         nextj = j;
         break;
      }
      if (nexti > 0) {
         break;
      }
   }

   for (i=line-1; i>0; i--) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         t = infile[i].getPrimaryTrack(j);
         if (t != track) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         }
         lasti = i;
         lastj = j;
         break;
      }
      if (lasti > 0) {
         break;
      }
   }

   if ((nexti == 0) || (lasti == 0)) {
      return;
   }

   char buffer[1024] = {0};
   PerlRegularExpression pre;
   Array<char> buffer2;

   if ((strchr(infile[lasti][lastj], 'r') != NULL) &&
       (strchr(infile[nexti][nextj], 'r') != NULL) ) {
      if ((strchr(infile[nexti][nextj], 'y') != NULL) &&
         (strchr(infile[lasti][lastj], 'y') != NULL)) {
         return;
      }
      strcpy(buffer, infile[nexti][nextj]);
      strcat(buffer, "yy");
      infile[nexti].setToken(nextj, buffer);
      return;
   }

   if (strchr(infile[lasti][lastj], 'l') != NULL) {
      // avoid terminal nulls
      return;
   }

   if ((strchr(infile[lasti][lastj], '[') != NULL) &&
       (strchr(infile[nexti][nextj], ']') != NULL) ) {

      if ((strchr(infile[nexti][nextj], 'y') != NULL) &&
         (strchr(infile[lasti][lastj], 'y') != NULL)) {
         return;
      }

      // hide tie on first note
      buffer2.setSize(strlen(infile[lasti][lastj]) + 1);
      strcpy(buffer2.getBase(), infile[lasti][lastj]);
      pre.sar(buffer2, "\\[", "[y", "g");
      infile[lasti].setToken(lastj, buffer2.getBase());
       
       // hide the entire second note
       strcpy(buffer, infile[nexti][nextj]);
       strcat(buffer, "yy");
       infile[nexti].setToken(nextj, buffer);
   }
}



//////////////////////////////
//
// addTerminalLongs -- place "l" marker on the last note of each voice 
//     (or chord in each voice).  Also the last note befour any 
//     double barlines.
//

int addTerminalLongs(HumdrumFile& infile) {
   int result = 0;
  
   Array<int> ktracks;
   infile.getTracksByExInterp(ktracks, "**kern");

   int i;
   for (i=0; i<ktracks.getSize(); i++) {
      result |= addTrackLongs(infile, ktracks[i]);
   }

   return result;
}



//////////////////////////////
//
// addTrackLongs -- add long markers to a specific track.
//

int addTrackLongs(HumdrumFile& infile, int track) {
   int i, j;
   int ptrack;
   int output = 0;
   int addQ = 1;
   char buffer[1024] = {0};
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (addQ == 0) {
         if (infile[i].isMeasure()) {
            for (j=0; j<infile[i].getFieldCount(); j++) {
               ptrack = infile[i].getPrimaryTrack(j);
               if (ptrack != track) {
                  continue;
               }
               if (strstr(infile[i][j], "||")) {
                  addQ = 1;
                  break;
               }
            }
         }
         continue;
      }
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         ptrack = infile[i].getPrimaryTrack(j);
         if (ptrack != track) {
            continue;
         }
         if (strchr(infile[i][j], '_') != NULL) {
            continue;
         }
         if (strchr(infile[i][j], ']') != NULL) {
            continue;
         }
         if (strchr(infile[i][j], 'r') != NULL) {
            continue;
         }
         if (strcmp(infile[i][j], ".") == 0) {
            continue;
         }
         if (strchr(infile[i][j], 'l') != NULL) {
            // don't mark a note which is already marked.
            addQ = 0; // clear the add state since already marked
            continue;
         }
         // found a note which should be marked.
         strcpy(buffer, infile[i][j]);
         strcat(buffer, "l");
         output = 1;
         infile[i].changeField(j, buffer);
         
         // disable addQ, but keep going on current line.
         addQ = 0;
      }
      
   }

   return output; 
}



//////////////////////////////
//
// setItem --
//

void setItem(Array<Array<char> >& anArray, int i, const char* aString) {
   anArray[i].setSize(strlen(aString)+1);
   strcpy(anArray[i].getBase(), aString);
}



//////////////////////////////
//
// printSectionInfo --
//

void printSectionInfo(void) {
   if (Section[0] == '\0') {
      return;
   }
   cout << "!!section: " << Section << "\n";
   cout << "!!!OMD:\t" << Section << "\n";
   Section[0] = '\0';
   Section.setSize(1);
}



//////////////////////////////
//
// convertRscaleTextToInterpretation --
//

int convertRscaleTextToInterpretation(HumdrumFile& infile, int line) {
   if (!infile[line].isComment()) {
      return 0;
   }
   int j;
   char buffer[1024] = {0};
   int hasRscale = 0;
   PerlRegularExpression pre;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], 
            "^!LO:TX:.*t=rscale&colon;\\s*(\\d+[^:]*)", "i")) {
         strcpy(buffer, "!rscale:");
         strcat(buffer, pre.getSubmatch(1));
         infile[line].setToken(j, buffer);
         hasRscale = 1;
      } else {
      }
   }

   PerlRegularExpression pre2;
   int track;
   if (hasRscale) {
      for (j=0; j<infile[line].getFieldCount(); j++) {
         if (pre.search(infile[line][j], "^!rscale:\\s*(.*)\\s*", "i")) {
            track = infile[line].getPrimaryTrack(j);
            cout << "*rscale:" << pre.getSubmatch(1);
            if (pre2.search(pre.getSubmatch(), "(\\d+)/(\\d+)")) {
               Rscale[track] = atoi(pre2.getSubmatch(1));
               Rscale[track] /= atoi(pre2.getSubmatch(2));
            } else {
               Rscale[track] = atoi(pre2.getSubmatch(1));
            }
            infile[line].setToken(j, "!");
         } else {
            cout << "*";
         }
         if (j<infile[line].getFieldCount() - 1) {
            cout << "\t";
         }
      }
      cout << "\n";
   }

   return hasRscale;
}





//////////////////////////////
//
// printDefaultLabelExpansion --
//

void printDefaultLabelExpansion(HumdrumFile& infile, int line) {
   int i;
   char label[2] = {0};
   label[0] = ++SectionLabel;
   char buffer[1024] = {0};
   strcpy(buffer, "*>[");
   strcat(buffer, label);
   int count = 1;
   PerlRegularExpression pre;
   
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isMeasure()) {
         continue;
      }
     
      if (pre.search(infile[i][0], "\\|\\||==|:")) {
         if (hasDataBelow(infile, i)) {
            count++;
            label[0]++;
            strcat(buffer, ",");
            strcat(buffer, label);
         }
      }
   }

   if (count <= 1) {
      return;
   }

   strcat(buffer, "]");
   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      cout << buffer;
      if (j < infile[line].getFieldCount() - 1) {
         cout << "\t";
      }
   }
   cout << "\n";

   // print *>A label now
   for (j=0; j<infile[line].getFieldCount(); j++) {
      cout << "*>A";
      if (j < infile[line].getFieldCount() - 1) {
         cout << "\t";
      }
   }
   cout << "\n";
}



//////////////////////////////
//
// hasDataBelow -
//

int hasDataBelow(HumdrumFile& infile, int line) {
   int i;
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         return 1;
      }
   }

   return 0;
}



//////////////////////////////
//
// printOutput --
//

void printOutput(HumdrumFile& infile, int terminalQ) {
   int firstBarline = 0;
   int dataFoundQ = 0;
   int i, j;
   int editorialQ = hasEditorial(infile);
   int insertedQ = 0;
   int labelinitQ = 0;
   int defaultPrinted = 0;

   Rscale.setSize(infile.getMaxTracks()+1);
   Rscale.setAll(1);

   PerlRegularExpression pre;

   Clef.setSize(infile.getMaxTracks()+1);
   Keysig.setSize(infile.getMaxTracks()+1);
   Timesig.setSize(infile.getMaxTracks()+1);
   Metsig.setSize(infile.getMaxTracks()+1);
   for (i=0; i<infile.getMaxTracks()+1; i++) {
      setItem(Clef, i, "*");
      setItem(Keysig, i, "*");
      setItem(Timesig, i, "*");
      setItem(Metsig, i, "*");
   }

   for (i=0; i<infile.getNumLines(); i++) {
      if (debugQ) {
         cout << "LINE " << i+1 << ": " << infile[i] << endl;
      }
      if (defaultPrinted && infile[i].isData()) {
         defaultPrinted = 0;
      }

      if ((defaultPrinted == 0) && infile[i].isInterpretation()) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (infile[i].isClef(j)) {
               setItem(Clef, infile[i].getPrimaryTrack(j), infile[i][j]);
            } else if (infile[i].isKeySig(j)) {
               setItem(Keysig, infile[i].getPrimaryTrack(j), infile[i][j]);
            } else if (infile[i].isTimeSig(j)) {
               setItem(Timesig, infile[i].getPrimaryTrack(j), infile[i][j]);
            } else if (infile[i].isMetSig(j)) {
               setItem(Metsig, infile[i].getPrimaryTrack(j), infile[i][j]);
            }
         }
      }

      if ((dataFoundQ == 0) && (firstBarline == 0) && infile[i].isMeasure()) {
         firstBarline = i;
         continue;;
      }

      if (infile[i].isMeasure() && hasDataBelow(infile, i) && 
          ((strstr(infile[i][0], "||") != NULL) ||
                                    (strstr(infile[i][0], ":") != NULL))) {
         if ((i > 0) && (strcmp(infile[i-1][0], "!!LO:LB:i:g=z") != 0)) {
            // printing line break provided that there is not one here already
            // (this filters out extra linebreaks for repeated use of jrpize program)
            cout << "!!LO:LB:i:g=z" << "\n";
         }
      }

      if (infile[i].isMeasure() && ((strstr(infile[i][0], "||") != NULL) ||
                                    (strstr(infile[i][0], ":") != NULL) ||
                                    (strstr(infile[i][0], "==") != NULL))) {
         turnOffAnyActiveRscales(infile, i);
      }

      if (infile[i].isInterpretation()) {
         checkAndPrintVox(infile, i);
      }
   
      if ((labelinitQ == 0) && infile[i].isAllClef()) {
         printDefaultLabelExpansion(infile, i);
      }

      convertRscaleTextToInterpretation(infile, i);

      if (infile[i].isSpineManipulator() && (firstBarline > 0)) {
         cout << infile[firstBarline] << "\n";
         firstBarline = 0;
      }

      if (hasSectionLabel(infile, i)) {
         printSectionLabel(infile, i);
         continue;
      }
   
      if ((infile[i].isLocalComment() || infile[i].isData()) 
            && (dataFoundQ == 0) && (firstBarline > 0)) {
         cout << infile[firstBarline] << "\n";
         firstBarline = 0;
         dataFoundQ = 1;
      }

      if (infile[i].isGlobalComment() || infile[i].isBibliographic()) {
         // filter out line/page breaks from the MusicXML, they are
         // not intended to be actual system breaks.
         if (pre.search(infile[i][0], "break:original")) {
            continue;
         }
      }

      clearMensurationComment(infile, i);

      if (infile[i].isNull()) {
         continue;
      }

      fixAccentedText(infile[i]);

      if (defaultPrinted) {
         if (cleanDefaultInterpretations(infile, i)) {
            continue;
         }
      }

      cout << infile[i] << "\n";

      if (infile[i].isMeasure()) {
         if (Section[0] != '\0') {
            printSectionInfo();
         }
      }

      if (defaultPrinted == 0) {
         if (infile[i].isAllTimeSig()) {
            processMensuration(infile, i);
         }
      }

      if (infile[i].isInterpretation()) {
         checkAndPrintInstAbbr(infile, i);
      }
      if ((!insertedQ) && (strcmp(infile[i][0], "*-") == 0)) {
         if (terminalQ && (!hasLongRDF(infile))) {
            cout << "!!!RDF**kern: l=long note in original notation" << "\n";
         }
         if (editorialQ && (!hasEditorialRDF(infile))) {
            cout << "!!!RDF**kern: i=editorial accidental" << "\n";
         }
         insertedQ = 1;
         for (int ii=i+1; ii<infile.getNumLines(); ii++) {
            cout << infile[ii] << "\n";
         }
         break;
      }

      // if (defaultPrinted == 0) {
      //    if (infile[i].isInterpretation()) {
      //       handleMensuration(infile, i);
      //    }
      // }

      if (infile[i].isMeasure() && ((strstr(infile[i][0], "||") != NULL) ||
                                     (strstr(infile[i][0], ":") != NULL))) {
         printLabel(infile, i);
         printDefaultKeyTimeMet(infile, i);
         defaultPrinted = 1;
      }

   }
}



//////////////////////////////
//
// cleanDefaultInterpretations --
//

int cleanDefaultInterpretations(HumdrumFile& infile, int line) {
   if (!infile[line].isInterpretation()) {
      return 0;
   }
   int j;
   int leftcount = 0;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (strcmp(infile[line][j], "*") == 0) {
         continue;
      }
      if (infile[line].isClef(j)) {
         infile[line].changeField(j, "*");
      } else if (infile[line].isKeySig(j)) {
         infile[line].changeField(j, "*");
      } else if (infile[line].isTimeSig(j)) {
         infile[line].changeField(j, "*");
      } else if (infile[line].isMetSig(j)) {
         infile[line].changeField(j, "*");
      } else {
         leftcount++;
      }
   }

   if (leftcount == 0) {
      return 1;
   } else {
      return 0;
   }
}


//////////////////////////////
//
// turnOffAnyActiveRscales --
//

void turnOffAnyActiveRscales(HumdrumFile& infile, int line) {
   int j;
   int track;
   int hasRscale = 0;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      track = infile[line].getPrimaryTrack(j);
      if (Rscale[track] != 1) {
         hasRscale = 1;
         break;
      }
   }

   if (hasRscale == 0) {
      return;
   }

   for (j=0; j<infile[line].getFieldCount(); j++) {
      track = infile[line].getPrimaryTrack(j);
      if (Rscale[track] != 1) {
         cout << "*rscale:1";
         Rscale[track] = 1;
      } else {
         cout << "*";
      }
      if (j < infile[line].getFieldCount() - 1) {
         cout << "\t";
      }
   }
   cout << "\n";
   

}



//////////////////////////////
//
// printLabel --
//

void printLabel(HumdrumFile& infile, int line) {
   if (!infile[line].isMeasure()) {
      return;
   }

   SectionLabel++;

   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      cout << "*>" << SectionLabel;
      if (j < infile[line].getFieldCount() - 1) {
         cout << "\t";
      }
   }
   cout << "\n";

}



//////////////////////////////
//
// clearMensurationComment --
//

void clearMensurationComment(HumdrumFile& infile, int line) {
   int j;
   if (!infile[line].isLocalComment()) {
      return;
   }
   PerlRegularExpression pre;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], 
            "^!LO:TX:.*t=(\\s*all\\s*&colon;\\s*)?[mM]en[A-Z0-9]", "i")) {
         infile[line].setToken(j, "!");
      }
   }
}



//////////////////////////////
//
// processMensuration --  Time signature just printed, so
//     look for a metrical signature in each track, printing
//     *met(C|) if time signature is 2/1 and there is no mensuration
//     or
//     *met(O) if time signature is 3/1 and there is no mensuration
//

void processMensuration(HumdrumFile& infile, int line) {
   if (!infile[line].isInterpretation()) {
      return;
   }
   char buffer[1024] = {0};
   int j;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      getMensuration(buffer, infile, line, j);
      if (strcmp(buffer, "*ZZY") == 0) {
         cout << "*Q";
      } else {
         cout << buffer;
      }
      setItem(Metsig, infile[line].getPrimaryTrack(j), buffer);
      if (j < infile[line].getFieldCount() - 1) {
         cout << "\t";
      }
   }
   cout << "\n";

   checkForPrimaryMensurationNeed(infile, line);
}



//////////////////////////////
//
// absorbMensurations --  used by default mensuration processing
//

void absorbMensurations(HumdrumFile& infile, int line) {
   char buffer[1024] = {0};
   int i, j;
   for (i=line; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         break;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         getMensuration(buffer, infile, i, j);
         if (strlen(buffer) > 0) {
            if (strcmp(buffer, "*ZZY") == 0) {
               // no mensuration, handle default for meter elsewhere.
            } else {
               setItem(Metsig, infile[i].getPrimaryTrack(j), buffer);
            }
         }
      }
   }
}



//////////////////////////////
//
// checkForPrimaryMenurationNeed -- If all mensurations in **kern spines are 
//    not the same, then print the majority mensuration in a global comment.
//

void checkForPrimaryMensurationNeed(HumdrumFile& infile, int line) {
   int i, j;
   int track, lasttrack;

   Array<int> ktracks;
   infile[line].getTracksByExInterp(ktracks, "**kern");

   if (ktracks.getSize() <= 1) {
      return;
   }
 
   int equalQ = 1;
   for (i=1; i<ktracks.getSize(); i++) {
      track = ktracks[i];
      lasttrack = ktracks[i-1];
      if (strcmp(Metsig[track].getBase(), Metsig[lasttrack].getBase()) != 0) {
         equalQ = 0;
         break;
      }
   }

   if (equalQ) {
      // all mensurations are the same, so don't print a 
      // "!!primary-mensuration:" line.
      return;
   }

   Array<int> counts;
   counts.setSize(ktracks.getSize());
   counts.setAll(0);

   int ti;
   int tj;

   for (i=0; i<ktracks.getSize(); i++) {
      ti = ktracks[i];
      for (j=0; j<ktracks.getSize(); j++) {
         tj = ktracks[j];
         if (strcmp(Metsig[ti].getBase(), Metsig[tj].getBase()) == 0) {
            counts[j]++;
            break;
         }
      }
   }

   // find maximum count
   int maxi = 0;

   for (i=1; i<counts.getSize(); i++) {
      if (counts[i] > counts[maxi]) {
         maxi = i;
      }
   }

   if (strcmp(Metsig[ktracks[maxi]].getBase(), "*") == 0) {
      return;
   }
   if (strcmp(Metsig[ktracks[maxi]].getBase(), "") == 0) {
      return;
   }

   // found the most common mensuration, so assume it is the 
   // primary mensuration:
   cout << "!!primary-mensuration: " 
        << Metsig[ktracks[maxi]].getBase()+1 << endl;

}



//////////////////////////////
//
// getMensuration --
//

char* getMensuration(char* buffer, HumdrumFile& infile, int line, int spine) {
   strcpy(buffer, "*ZZY");
   int foundDataQ = 0;
   int i, j;
   PerlRegularExpression pre;

   if (!infile[line].isExInterp(spine, "**kern")) {
      strcpy(buffer, "*");
      return buffer;
   }

   int targettrack = infile[line].getPrimaryTrack(spine);
   int tracki = 0;
   int trackj = 0;
   
   int mencount = 0;
   int lasti = 0;
   int lastj = 0;

   int timesigi = 0;
   // int timesigj = 0;

   int track;

   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         foundDataQ = i;
         break;
      }
      if (!infile[i].isLocalComment()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strcmp(infile[i][j], "!") == 0) {
            continue;
         }
         track = infile[i].getPrimaryTrack(j);
         if (pre.search(infile[i][j], 
            "^!LO:TX:.*t=(\\s*all\\s*&colon;\\s*)?[mM]en[A-Z0-9]", "i")) {
            mencount++;
            lasti = i;
            lastj = j;
            if (track == targettrack) {
               tracki = i;
               trackj = j;
            }
         }
         if (pre.search(infile[i][j], "^\\*M\\d+/\\d+")) {
            if (track == targettrack) {
               timesigi = i;
               // timesigj = j;
            }
         }
      }
   }

   if (!foundDataQ) {
      strcpy(buffer, "*ZZZ");
      return buffer;
   }

   int ii = tracki; 
   int jj = trackj; 
   if (mencount == 1) {
      ii = lasti;
      jj = lastj;
   }
   if (pre.search(infile[ii][jj], 
         "^!LO:TX:.*t=(\\s*all\\s*&colon;\\s*)?([Mm]en[^\\s]*)", "i")) {
      if (infile[ii][jj][0] != '!') {
         cerr << "Funny error on line " << ii+1 << " column " << jj+1 << endl;
         cerr << infile[ii] << endl;
         exit(1);
      }
      convertMenIntoMet(buffer, pre.getSubmatch(2));
      if (mencount != 1) {
         infile[ii].setToken(jj, "!");
      }
      return buffer;
   }

   if ((ii == 0) || (jj == 0)) {
      // set a default mensuration since none found in data
      if (pre.search(infile[line][spine], "^\\*M2/1$")) {
         strcpy(buffer, "*met(C|)");
         return buffer;
      } else if (pre.search(infile[line][spine], "^\\*M3/1$")) {
         strcpy(buffer, "*met(O)");
         return buffer;
      } else {
         return buffer;
         if (timesigi != 0) {
            strcpy(buffer, "*met(___)");
         } else {
            strcpy(buffer, "*met(y)");
         }
      }
      return buffer;
   }

   return buffer;
}



//////////////////////////////
//
// convertMenIntoMet --
//

char*  convertMenIntoMet(char* buffer, const char* mensur) {
   PerlRegularExpression pre;

   if (pre.search(mensur, "MenCutC\\s*$", "i")) {
      strcpy(buffer, "*met(C|)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircle\\s*$", "i")) {
      strcpy(buffer, "*met(O)");
      return buffer;
   }
   if (pre.search(mensur, "MenC3\\s*$", "i")) {
      strcpy(buffer, "*met(C3)");
      return buffer;
   }
   if (pre.search(mensur, "MenOover3\\s*$", "i")) {
      strcpy(buffer, "*met(O/3)");
      return buffer;
   }
   if (pre.search(mensur, "MenC\\s*$", "i")) {
      strcpy(buffer, "*met(C)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircle2\\s*$", "i")) {
      strcpy(buffer, "*met(O2)");
      return buffer;
   }
   if (pre.search(mensur, "MenO2\\s*$", "i")) {
       // not legal, but include in case of data error:
      strcpy(buffer, "*met(O2)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutCircle\\s*$", "i")) {
      strcpy(buffer, "*met(O|)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutC3\\s*$", "i")) {
      strcpy(buffer, "*met(C|3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircleDot\\s*$", "i")) {
      strcpy(buffer, "*met(O.)");
      return buffer;
   }
   if (pre.search(mensur, "MenC2\\s*$", "i")) {
      strcpy(buffer, "*met(C2)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutC2\\s*$", "i")) {
      strcpy(buffer, "*met(C|2)");
      return buffer;
   }
   if (pre.search(mensur, "Men2\\s*$", "i")) {
      strcpy(buffer, "*met(2)");
      return buffer;
   }
   if (pre.search(mensur, "Men3\\s*$", "i")) {
      strcpy(buffer, "*met(3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCDot\\s*$", "i")) {
      strcpy(buffer, "*met(C.)");
      return buffer;
   }
   if (pre.search(mensur, "MenReverseC\\s*$", "i")) {
      strcpy(buffer, "*met(Cr)");
      return buffer;
   }
   if (pre.search(mensur, "MenCReverse\\s*$", "i")) {
      strcpy(buffer, "*met(Cr)");
      return buffer;
   }
   if (pre.search(mensur, "Men3over2\\s*$", "i")) {
      strcpy(buffer, "*met(3/2)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutCircle3\\s*$", "i")) {
      strcpy(buffer, "*met(O|3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutCircle3Over2\\s*$", "i")) {
      strcpy(buffer, "*met(O|3/2)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircle3\\s*$", "i")) {
      strcpy(buffer, "*met(O3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutCOver3\\s*$", "i")) {
      strcpy(buffer, "*met(C|/3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutCOver2\\s*$", "i")) {
      strcpy(buffer, "*met(C|/2)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircleOver3\\s*$", "i")) {
      strcpy(buffer, "*met(O/3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCutCDot\\s*$", "i")) {
      strcpy(buffer, "*met(C.|)");
      return buffer;
   }
   if (pre.search(mensur, "MenReverseCutC\\s*$", "i")) {
      strcpy(buffer, "*met(C|r)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircle3Over2\\s*$", "i")) {
      strcpy(buffer, "*met(O3/2)");
      return buffer;
   }
   if (pre.search(mensur, "MenC2Over3\\s*$", "i")) {
      strcpy(buffer, "*met(C2/3)");
      return buffer;
   }
   if (pre.search(mensur, "MenCircleDot3Over8\\s*$", "i")) {
      strcpy(buffer, "*met(O.3/8)");
      return buffer;
   }
   if (pre.search(mensur, "MenCDot3Over2\\s*$", "i")) {
      strcpy(buffer, "*met(C.3/2)");
      return buffer;
   }

   cerr << "ERROR: unknown mensuration: " << mensur << endl;
   exit(1);
}



//////////////////////////////
//
// printDefaultKeyTimeMet --
//

void printDefaultKeyTimeMet(HumdrumFile& infile, int line) {
   if (!infile[line].isMeasure()) {
      return;
   }

   absorbMensurations(infile, line);

   int i, j;
   // int datafound = 0;
   int hasMetsig = 0;

   // search for new key,time,met data at the current
   // location and store it before printing the default
   // key,time,met.
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isData()) {
         // datafound = 1;
         break;
      }
      if (infile[i].isInterpretation()) {
         for (j=0; j<infile[i].getFieldCount(); j++) {
            if (infile[i].isClef(j)) {
               setItem(Clef, infile[i].getPrimaryTrack(j), infile[i][j]);
            } else if (infile[i].isKeySig(j)) {
               setItem(Keysig, infile[i].getPrimaryTrack(j), infile[i][j]);
            } else if (infile[i].isTimeSig(j)) {
               setItem(Timesig, infile[i].getPrimaryTrack(j), infile[i][j]);
            } else if (infile[i].isMetSig(j)) {
               hasMetsig = 1;
            }
         }
      }
   }

   int track;


   // print the prevailing key,time,met information
   for (j=0; j<infile[line].getFieldCount(); j++) {
      track = infile[line].getPrimaryTrack(j);
      cout << Clef[track];
      if (j<infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";

   for (j=0; j<infile[line].getFieldCount(); j++) {
      track = infile[line].getPrimaryTrack(j);
      cout << Keysig[track];
      if (j<infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";

   for (j=0; j<infile[line].getFieldCount(); j++) {
      track = infile[line].getPrimaryTrack(j);
      cout << Timesig[track];
      if (j<infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";

   if (hasMetsig == 0) {
      for (j=0; j<infile[line].getFieldCount(); j++) {
         track = infile[line].getPrimaryTrack(j);
         cout << Metsig[track];
         if (j<infile[line].getFieldCount()-1) {
            cout << "\t";
         }
      }
      cout << "\n";
      checkForPrimaryMensurationNeed(infile, line);
   }

}



//////////////////////////////
//
// hasSectionLabel -- true if text start with [Ss]ection:
//

int hasSectionLabel(HumdrumFile& infile, int line) {
   int j;
   PerlRegularExpression pre;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], 
            "^!LO:TX:.*t=\\s*[Ss][Ee][Cc][Tt][Ii][Oo][Nn]\\s*\\&colon;\\s*([^:]*)\\s*$")) {
         return 1;
      } 
   }

   return 0;
}



//////////////////////////////
//
// wrongSideOfBarline -- return true if current line is before a barline
//    (true) or dataline (false)
//

int wrongSideOfBarline(HumdrumFile& infile, int line) {
   int i;
   for (i=line+1; i<infile.getNumLines(); i++) {
      if (infile[i].isMeasure()) {
         return 1;
      } else if (infile[i].isData()) {
         return 0;
      }
   }
   return 0;
}



//////////////////////////////
//
// printSectionLabel --
//

void printSectionLabel(HumdrumFile& infile, int line) {
   // int other = 0;
   int j;
   Array<char> string;
   PerlRegularExpression pre;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], 
            "^!LO:TX:.*t=\\s*[Ss][Ee][Cc][Tt][Ii][Oo][Nn]\\s*\\&colon;\\s*([^:]*)\\s*$")) {
         string.setSize(strlen(pre.getSubmatch(1))+1);
         strcpy(string.getBase(), pre.getSubmatch(1));
         pre.sar(string, "&colon;", ":", "g");
         if (wrongSideOfBarline(infile, line)) {
            Section.setSize(strlen(string.getBase()) + 1);
            strcpy(Section.getBase(), string.getBase());
         } else {
            cout << "!!section: " << string << "\n";
            cout << "!!!OMD: " << string << "\n";
         }
         infile[line].setToken(j, "!");
      } else if (strcmp(infile[line][j], "!")) {
         // do nothing
      } else {
         // other = 1;
      }
   }

   if (!infile[line].isNull()) {
      cout << infile[line] << "\n";
   }
}




//////////////////////////////
//
// checkAndPrintVox -- look for instrument names, and add an
//    abbreviation underneath it.
//

void checkAndPrintVox(HumdrumFile& infile, int line) {
   int j;
   if (!infile[line].isInterpretation()) {
      return;
   }
   PerlRegularExpression pre;

   int hasInstrument = 0;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], "^\\*I\"")) {
         hasInstrument = 1;
         break;
      }
   }

   if (!hasInstrument) {
      return;
   }
 
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], "^\\*I\"\\s*(.*)\\s*")) {
         cout << "*Ivox";
      } else {
         cout << "*";
      }
      if (j<infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";
}



//////////////////////////////
//
// checkAndPrintInstAbbr -- look for instrument names, and add an
//    abbreviation underneath it.
//

void checkAndPrintInstAbbr(HumdrumFile& infile, int line) {
   int j;
   if (!infile[line].isInterpretation()) {
      return;
   }
   PerlRegularExpression pre;

   int hasInstrument = 0;
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], "^\\*I\"")) {
         hasInstrument = 1;
         break;
      }
   }

   if (!hasInstrument) {
      return;
   }
 
   for (j=0; j<infile[line].getFieldCount(); j++) {
      if (pre.search(infile[line][j], "^\\*I\"\\s*(.*)\\s*")) {
         printAbbreviation(pre.getSubmatch(1));
      } else {
         cout << "*";
      }
      if (j<infile[line].getFieldCount()-1) {
         cout << "\t";
      }
   }
   cout << "\n";
}



////////////////////////////////////////
//
//  printAbbreviation --
//

void printAbbreviation(const char* fullname) {

   PerlRegularExpression pre;

   if (pre.search(fullname, "^Superius", "i")) {
      cout << "*I'S";
   } else if (pre.search(fullname, "^Soprano", "i")) {
      cout << "*I'S";
   } else if (pre.search(fullname, "^Discantus", "i")) {
      cout << "*I'D";
   } else if (pre.search(fullname, "^Altus", "i")) {
      cout << "*I'A";
   } else if (pre.search(fullname, "^Alto", "i")) {
      cout << "*I'A";
   } else if (pre.search(fullname, "^Tenor", "i")) {
      cout << "*I'T";
   } else if (pre.search(fullname, "^Tenore", "i")) {
      cout << "*I'T";
   } else if (pre.search(fullname, "^Contratenor", "i")) {
      cout << "*I'Ct";
   } else if (pre.search(fullname, "^Contra", "i")) {
      cout << "*I'C";
   } else if (pre.search(fullname, "^Cantus", "i")) {
      cout << "*I'Cn";
   } else if (pre.search(fullname, "^Canto", "i")) {
      cout << "*I'Cto";
   } else if (pre.search(fullname, "^Quinto", "i")) {
      cout << "*I'Qto";
   } else if (pre.search(fullname, "^Bassus", "i")) {
      cout << "*I'B";
   } else if (pre.search(fullname, "^Bass", "i")) {
      cout << "*I'B";
   } else if (pre.search(fullname, "^Vagans", "i")) {
      cout << "*I'V";
   } else {
      cout << "*I'XXX";
      cerr << "WARNING: unknown instrument name: " << fullname << endl;
   }

   if (pre.search(fullname, "(\\d+[^\\s]*)")) {
      cout << pre.getSubmatch(1);
   }

   return;
}



////////////////////////////////////////
//
// handleMensuration --
//

void handleMensuration(HumdrumFile& infile, int line) {
   int& i = line;
   int j;

   PerlRegularExpression pre;

   if (!infile[i].isInterpretation()) {
      return;
   }

   if (i >= infile.getNumLines()-1) {
      // should never get here, but just in case...
      return;
   }

   int hasmeter = 0;
   for (j=0; j<infile[i].getFieldCount(); j++) {
      if (pre.search(infile[i][j], "^\\*M\\d")) {
         hasmeter = 1;
         break;
      }
   }

   if (!hasmeter) {
      return;
   }
  
   if (strncmp(infile[i+1][0], "*met(", 5) == 0) {
      // don't add *met line if there already is one there.
      return;
   }

   /* Mensuration added in another location now ...
   for (j=0; j<infile[i].getFieldCount(); j++) {
      if (strcmp(infile[i][j], "*M2/1") == 0) {
         cout << "*met(C|)x";
      } else if (strcmp(infile[i][j], "*M3/1") == 0) {
         cout << "*met(O)x";
      } else {
         if (infile[i].isExInterp(j, "**kern")) {
            cout << "*met()x";
         } else {
            cout << "*";
         }
      }
      if (j<infile[i].getFieldCount() - 1) {
         cout << "\t";
      }
   }
   cout << "\n";
   */
}



//////////////////////////////
//
// hasLongRDF -- returns true if a bibliographic record starts with:
//      !!!RDF**kern: l=long
//

int hasLongRDF(HumdrumFile& infile) {
   int i;
   PerlRegularExpression pre;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (pre.search(infile[i][0], 
            "^!!!RDF\\*\\*kern\\s*:\\s*l\\s*=\\s*long", "i")) {
         return 1;
      }
   }

   return 0;
}



//////////////////////////////
//
// hadEditorial -- true if any **kern note has an "i" marker on it.
//

int hasEditorial(HumdrumFile& infile) {
   int i, j;
   for (i=0; i<infile.getNumLines(); i++) {
      if (!infile[i].isData()) {
         continue;
      }
      for (j=0; j<infile[i].getFieldCount(); j++) {
         if (!infile[i].isExInterp(j, "**kern")) {
            continue;
         }
         if (strchr(infile[i][j], 'i') != NULL) {
            return 1;
         }
      }
   }
   return 0;
}



//////////////////////////////
//
// hasEditorialRDF -- returns true if there is a bibliographic record
//       starting with:
//      !!!RDF**kern: i=edit
//

int hasEditorialRDF(HumdrumFile& infile) {
   int i;
   PerlRegularExpression pre;
   for (i=infile.getNumLines()-1; i>=0; i--) {
      if (!infile[i].isBibliographic()) {
         continue;
      }
      if (pre.search(infile[i][0], 
            "^!!!RDF\\*\\*kern\\s*:\\s*i\\s*=\\s*edit", "i")) {
         return 1;
      }
   }

   return 0;
}


////////////////////
//
// fixAccentedText --
//

void fixAccentedText(HumdrumRecord& aRecord) {
   int i;
   for (i=0; i<aRecord.getFieldCount(); i++) {
      if (!strcmp(aRecord[i], ".")) {
         continue;
      }
      if (aRecord.isExInterp(i, "**text") || aRecord.isExInterp(i, "**silbe")) {
         checkText(aRecord, i);         
      }
   }
}



//////////////////////////////
//
// checkText --
//

void checkText(HumdrumRecord& aRecord, int index) {
   PerlRegularExpression pre;
   if (!pre.search(aRecord[index], "[^a-zA-Z0-9,;:. ='\"!@#$%^&*(){}\\|?/><-]")) {
      return;
   }
   Array<char> text;
   text.setSize(strlen(aRecord[index] + 1));
   strcpy(text.getBase(), aRecord[index]);
   pre.sar(text, "\xc5\xbd", "&eacute;", "g");

   aRecord.setToken(index, text.getBase());
}



//////////////////////////////
//
// checkOptions -- validate and process command-line options.
//

void checkOptions(Options& opts, int argc, char* argv[]) {
   opts.define("debug=b");              // determine bad input line num
   opts.define("author=b");             // author of program
   opts.define("version=b");            // compilation info
   opts.define("example=b");            // example usages
   opts.define("h|help=b");             // short description
   opts.process(argc, argv);
   
   // handle basic options:
   if (opts.getBoolean("author")) {
      cout << "Written by Craig Stuart Sapp, "
           << "craig@ccrma.stanford.edu, Dec 2011" << endl;
      exit(0);
   } else if (opts.getBoolean("version")) {
      cout << argv[0] << ", version: 15 Dec 2011" << endl;
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

   debugQ = opts.getBoolean("debug");
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



// md5sum: 7d1c38ab2c56637ec88ce85bbad52bd4 jrpize.cpp [20160305]
